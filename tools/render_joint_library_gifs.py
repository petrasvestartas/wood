"""The picture and the assembly GIF of every joint of the beam joint library, docs/images/joint_library/beam_<design>.png and
.gif, drawn alike: each design at the corner the book draws (leaning for an angled joint, else square where it has one) as element_joint_beam_library draws
it (data/output/pb/live.pb), see-through with its pegs, wedges and keys brown, one fixed camera, one 3:2 frame. The
picture is the GIF's first frame: every piece apart, back along its way in. The GIF then takes the pieces in one by one in
the assembly order (data/output/pb/joint_library_orders.json): each slides in along its way, then makes its further moves,
slides and turns about a line, each swept as a matrix; the moving piece brown, as the parts. Pieces pass through each other freely:
the GIF shows the order and the moves, not a collision check.

    python3 tools/render_joint_library_gifs.py [design ...] [--slides 6]
"""

import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops

WOOD = pathlib.Path(__file__).resolve().parents[1]
PYTHON = WOOD.parent / "laika/laika_py/.venv/bin/python"
RENDERER = pathlib.Path(
    os.environ["LAIKA_VIEWER_CLI"]
    if "LAIKA_VIEWER_CLI" in os.environ
    else next(
        (
            str(p)
            for p in (
                WOOD.parent / "laika/laika_viewer/target/x86_64-unknown-linux-gnu/release/laika-viewer-cli",
                WOOD.parent / "laika/laika_viewer/target/release/laika-viewer-cli",
                WOOD.parent / "laika/laika_viewer/target/x86_64-pc-windows-msvc/release/laika-viewer-cli.exe",
                pathlib.Path.home() / "code/code_rust/laika/laika_viewer/target/x86_64-unknown-linux-gnu/release/laika-viewer-cli",
                pathlib.Path.home() / "code/code_rust/laika/laika_viewer/target/release/laika-viewer-cli",
            )
            if p.exists()
        ),
        str(WOOD.parent / "laika/laika_viewer/target/x86_64-unknown-linux-gnu/release/laika-viewer-cli"),
    )
)
PICTURES = WOOD / "docs/images/joint_library"
RECORDS = "".join(path.read_text() for path in sorted((WOOD / "data/joints").glob("*.yaml")))  # Every record, for its family.
SIZE = (960, 640)  # Every picture, GIF and tile of the library the same size, so a row of the page reads as one.

# Every frame of the plan twice, see-through and solid: the design's corner alone, each piece placed by the moves the frame
# gives it in order (a slide {"t": [x, y, z]} or a turn {"line": [point, direction], "angle": degrees}, each with the share of
# it made), the moving piece and the parts brown (linear, as the renderer takes node colours), the wood white in the solid copy; two marks in the background's
# colour at the corners of every frame's bounds keep the camera still.
FRAMES = r"""
import json, re, sys
from laika_py.session import Session
from laika_py.xform import Xform
from laika_py.color import Color
from laika_py.line import Line
from laika_py.point import Point
from laika_py.vector import Vector
scene_path, design, variant, plan_path, out = sys.argv[1:6]
plan = json.load(open(plan_path))

def placement(moves):
    xform = Xform()
    for move in moves:
        share = move.get("share", 1.0)
        if "t" in move:
            step = Xform.translation(*[value * share for value in move["t"]])
        else:
            p, d = move["line"][:3], move["line"][3:]
            step = Xform.rotation_around_line(Line.from_points(Point(*p), Point(p[0] + d[0], p[1] + d[1], p[2] + d[2])), move["angle"] * share, True)
        xform = step * xform
    return xform

# a library design: its corner's group, the joint element named as the corner, its parts by name; an element example (design
# "-"): the whole scene, the joints and the parts the plan names
joints = plan.get("joints", [variant])
def is_part(name):
    return name in plan["parts"] if "parts" in plan else re.search(r"_(part|pin)(_[0-9]+)?$", name) is not None

branch = Session.pb_load(scene_path)
if design != "-":
    branch = branch.get_branch(design)
for edge in list(branch.interactions):
    branch.interactions[edge] = []
# no arrows, no base planes and no joint element: undrawn, they would still widen the camera's framing (the joint's empty
# box sits at the origin); the joint's parts, which hang under it, go back in at the root
for line in list(branch.objects.lines) + list(branch.objects.planes) + list(branch.objects.polylines) + [pc for pc in branch.objects.pointclouds if not pc.is_visible]:
    branch.remove_object(line.guid)
for pc in list(branch.objects.pointclouds):
    if design != "-" or not pc.name.startswith(variant):
        branch.remove_object(pc.guid)
for element in list(branch.objects.elements):
    if element.name in joints:
        branch.remove_object(element.guid)
for element in list(branch.objects.elements):
    if element.name.startswith(variant) and is_part(element.name):
        branch.remove_object(element.guid)
        branch.add_element(element)
kept = []
for element in list(branch.objects.elements):
    if not element.name.startswith(variant):
        branch.remove_object(element.guid)
        continue
    element.set_features([])
    world = branch.world_xform(element.guid)
    if world is not None:
        element.place(world)
        branch.remove_xform(element.guid)
    kept.append((element, element.aabb.get_corners(), is_part(element.name), branch.get_node(element.guid)))

# the bounds of every frame, so all are drawn from one camera
low, high = [1e18] * 3, [-1e18] * 3
for frame in plan["frames"]:
    for element, corners, _, _ in kept:
        xf = placement(frame["moves"].get(element.name) or [])
        for corner in corners:
            p = corner.transformed(xf)
            for k in range(3):
                low[k] = min(low[k], p[k])
                high[k] = max(high[k], p[k])
for pc in branch.objects.pointclouds:
    coords = pc.coords()
    for i in range(0, len(coords), 3):
        for k in range(3):
            low[k] = min(low[k], coords[i + k])
            high[k] = max(high[k], coords[i + k])
shift = [-(a + b) / 2 for a, b in zip(low, high)]
low = [a + d for a, d in zip(low, shift)]
high = [b + d for b, d in zip(high, shift)]
shift_xf = Xform.translation(*shift)
for pc in branch.objects.pointclouds:
    pc.transform(shift_xf)
for a, b in ((low, [low[0] + 0.5, low[1], low[2]]), (high, [high[0] - 0.5, high[1], high[2]])):
    mark = Line.from_points(Point(*a), Point(*b))
    mark.linecolor = Color(0.969, 0.969, 0.969, 1.0)
    branch.add_line(mark, None)

BROWN = Color(0.4196, 0.2667, 0.1373, 1.0)
GREY = Color(0.55, 0.55, 0.55, 1.0)
paths = []
for n, frame in enumerate(plan["frames"]):
    for element, _, _, _ in kept:
        branch.set_xform(element.guid, shift_xf * placement(frame["moves"].get(element.name) or []))
    pair = []
    for solid in (False, True):
        for element, _, part, node in kept:
            moving = element.name == frame["moving"]
            colour = BROWN if part or moving else GREY if solid else None
            branch.set_node_color(node, colour)
        path = "%s/frame_%03d%s.pb" % (out, n, "_solid" if solid else "")
        branch.pb_dump(path)
        pair.append(path)
    paths.append(pair)
print(json.dumps({"paths": paths, "target": [0.0, 0.0, 0.0]}))
"""


def option(arguments: list, flag: str, default: str) -> str:
    """The value after flag, taken out of arguments, or default."""
    if flag not in arguments:
        return default
    index = arguments.index(flag)
    value = arguments[index + 1]
    del arguments[index : index + 2]
    return value


def leg_move(leg) -> dict:
    """A further move of the orders as a frame move: a slide written as three numbers, or a turn about a line."""
    if isinstance(leg, list):
        return {"t": leg}
    if "shift" in leg:
        return {"t": leg["shift"]}
    return {"line": leg["line"], "angle": leg["angle"]}


def plan_frames(order: list, slides: int) -> list:
    """Every frame as the moves each piece has made from where the drawing set it, and the moving piece: all apart (the
    picture), then one by one in along the way, then the further moves, held at the end."""
    done = {step["element"]: [{"t": [-value for value in step["out"]]}] + [leg_move(leg) for leg in step["legs"]] for step in order}

    def frame(placed: int, moving: int, made: list, duration: int) -> dict:
        moves = {}
        for i, step in enumerate(order):
            moves[step["element"]] = done[step["element"]] if i < placed else made if i == moving else []
        return {"moves": moves, "moving": order[moving]["element"] if moving >= 0 else "", "duration": duration}

    frames = [frame(0, -1, [], 1500)]
    for k, step in enumerate(order):
        path = done[step["element"]]
        for m, move in enumerate(path):
            count = slides if m == 0 else slides // 2 + 1
            for s in range(1, count + 1):
                frames.append(frame(k, k, path[:m] + [dict(move, share=s / count)], 60))
        frames[-1]["duration"] = 400
    frames.append(frame(len(order), -1, [], 2000))
    return frames


def render(pb: str, target: list, opacity: str) -> Image.Image:
    """One pass drawn isometric at 3600 x 2400 in the Arctic look, aimed at target, the outline twice its width for the halving."""
    env = dict(
        os.environ, VIEWER_SHEETS="0", VIEWER_OPACITY=opacity,
        VIEWER_W=os.environ.get("VIEWER_W", "3600"), VIEWER_H=os.environ.get("VIEWER_H", "2400"),
        VIEWER_DPR="2", VIEWER_THICKNESS="1", VIEWER_VIEW="iso", VIEWER_PLANE_SIZE="0",
        VIEWER_NO_GRID="1", VIEWER_AO="1", VIEWER_OUTLINES="1",
        VIEWER_TARGET=",".join(f"{value:.3f}" for value in target),
    )
    with tempfile.TemporaryDirectory() as tmp:
        ppm = pathlib.Path(tmp) / "frame.ppm"
        subprocess.run([str(RENDERER), "render-pb", str(ppm), pb], env=env, check=True, capture_output=True)
        return Image.open(ppm).convert("RGB")


def drawing(pair: list, target: list) -> Image.Image:
    """The see-through pass as dark as the solid one (the darker of the two at every pixel: the wood keeps the solid's
    shading, the hidden edges still show), with the brown of the solid one laid over it where a part or the moving piece is."""
    strong = render(pair[1], target, "1.0")
    image = ImageChops.darker(render(pair[0], target, "0.45"), strong)
    red, green, blue = strong.split()
    brownish = ImageChops.subtract(red, blue).point(lambda value: 255 if value > 16 else 0)
    image.paste(strong, (0, 0), brownish)
    return image


def pictures(design: str, orders: dict, scene: pathlib.Path, slides: int) -> list:
    """The picture and the GIF of design at the corner the book draws: leaning for an angled joint, else square where it has one."""
    # the corner the book draws: an angled joint leaning, every other square where it can be
    family = re.search(r'^' + design + r':\n(?:  .*\n)*?  family: "(.*?)"', RECORDS, re.M)
    leaning = family is not None and family.group(1) == "angled" and f"{design}_inclined" in orders
    variant = f"{design}_inclined" if leaning or f"{design}_square" not in orders else f"{design}_square"
    return draw(scene, design, variant + "_", orders.get(variant, []), slides, f"beam_{design}", {"joints": [variant]})


def draw(scene: pathlib.Path, design: str, prefix: str, order: list, slides: int, name: str, names: dict, folder: pathlib.Path = PICTURES, animate: bool = True, size: tuple = SIZE) -> list:
    """The picture and the GIF of the pieces of scene whose names start with prefix (the group design, the whole scene for "-"),
    apart, then in by order, saved as <folder>/<name>.png and .gif (the picture alone when not animate) at size; names gives
    the joints to leave out and the parts to draw brown."""
    frames = plan_frames(order, slides)
    with tempfile.TemporaryDirectory() as tmp:
        plan_path = pathlib.Path(tmp) / "plan.json"
        plan_path.write_text(json.dumps(dict(names, frames=frames)))
        listing = subprocess.run([str(PYTHON), "-c", FRAMES, str(scene), design, prefix, str(plan_path), tmp], check=True, capture_output=True, text=True).stdout
        written = json.loads(listing.strip().splitlines()[-1])
        images = [drawing(pair, written["target"]) for pair in written["paths"]]

    # cropped alike to what any frame draws with a margin, framed 3:2, scaled to SIZE; the GIF on one palette, undithered
    clear = images[0].getpixel((2, 2))
    box = None
    for image in images:
        drawn = ImageChops.difference(image, Image.new("RGB", image.size, clear)).convert("L").point(lambda value: 255 if value > 12 else 0).getbbox()
        if drawn:
            box = drawn if box is None else (min(box[0], drawn[0]), min(box[1], drawn[1]), max(box[2], drawn[2]), max(box[3], drawn[3]))
    crop = (max(0, box[0] - 60), max(0, box[1] - 60), min(images[0].width, box[2] + 60), min(images[0].height, box[3] + 60))
    width = max(crop[2] - crop[0], int((crop[3] - crop[1]) * 1.5))
    height = max(crop[3] - crop[1], int(width / 1.5))
    framed = []
    for image in images:
        canvas = Image.new("RGB", (width, height), clear)
        piece = image.crop(crop)
        canvas.paste(piece, ((width - piece.width) // 2, (height - piece.height) // 2))
        framed.append(canvas.resize(size, Image.LANCZOS))
    picture = folder / f"{name}.png"
    framed[0].save(picture)
    if not animate:
        return [picture]
    # the palette from every frame, so the moving piece keeps its tint
    sample = Image.new("RGB", (framed[0].width, framed[0].height * len(framed)))
    for k, image in enumerate(framed):
        sample.paste(image, (0, framed[0].height * k))
    palette = sample.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
    animation = folder / f"{name}.gif"
    quantized = [image.quantize(palette=palette, dither=Image.Dither.NONE) for image in framed]
    quantized[0].save(animation, save_all=True, append_images=quantized[1:], duration=[f["duration"] for f in frames], loop=0, optimize=True)
    return [picture, animation]


def main(arguments: list) -> None:
    """Runs the library example once and saves the picture and the GIF of each design, every design when none is named."""
    slides = int(option(arguments, "--slides", "6"))
    listing = subprocess.run([str(WOOD / "build/element_joint_beam_library")], cwd=WOOD, check=True, capture_output=True, text=True)
    designs = arguments or [line.split()[1] for line in listing.stdout.splitlines() if line.startswith("DESIGN ")]
    orders = json.loads((WOOD / "data/output/pb/joint_library_orders.json").read_text())
    PICTURES.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        scene = pathlib.Path(tmp) / "scene.pb"
        scene.write_bytes((WOOD / "data/output/pb/live.pb").read_bytes())
        for design in designs:
            for path in pictures(design, orders, scene, slides):
                print(path.relative_to(WOOD), flush=True)


if __name__ == "__main__":
    main(sys.argv[1:])
