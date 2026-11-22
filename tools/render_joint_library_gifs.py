"""The picture and the assembly GIF of every joint of the beam joint library, docs/images/joint_library/beam_<design>.png and
.gif, drawn alike: the square corner of each design (its inclined one when it has none) as element_joint_beam_library draws
it (data/output/pb/live.pb), see-through with its pegs, wedges and keys a strong blue, one fixed camera, one 3:2 frame. The
picture is the GIF's first frame: every piece apart, back along its way in. The GIF then takes the pieces in one by one in
the assembly order (data/output/pb/joint_library_orders.json): each slides in along its way, then makes its further moves,
slides and turns about a line, each swept as a matrix; the moving piece tinted light blue, still see-through. Pieces pass through each other freely:
the GIF shows the order and the moves, not a collision check.

    python3 tools/render_joint_library_gifs.py [design ...] [--slides 6]
"""

import json
import os
import pathlib
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops

WOOD = pathlib.Path(__file__).resolve().parents[1]
PYTHON = WOOD.parent / "session/session_py/.venv/bin/python"
RENDERER = WOOD.parent / "session/session_viewer/target/x86_64-unknown-linux-gnu/release/examples/selftest"
PICTURES = WOOD / "docs/images/joint_library"

# Every frame of the plan twice, see-through and solid: the design's corner alone, each piece placed by the moves the frame
# gives it in order (a slide {"t": [x, y, z]} or a turn {"line": [point, direction], "angle": degrees}, each with the share of
# it made), the moving piece light blue, the parts a deep blue, the wood white in the solid copy; two marks in the background's
# colour at the corners of every frame's bounds keep the camera still.
FRAMES = r"""
import json, re, sys
from session_py.session import Session
from session_py.xform import Xform
from session_py.color import Color
from session_py.line import Line
from session_py.point import Point
from session_py.vector import Vector
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

def frame_branch(frame, solid):
    branch = Session.pb_load(scene_path).get_branch(design)
    for edge in list(branch.interactions):
        branch.interactions[edge] = []
    for element in list(branch.objects.elements):
        if not element.name.startswith(variant + "_") and element.name != variant:
            branch.remove_object(element.guid)
            continue
        element.set_features([])
        moves = frame["moves"].get(element.name)
        if moves:
            element.place(placement(moves))
        part = re.search(r"_(part|pin)(_[0-9]+)?$", element.name) is not None
        # the moving piece tinted in the see-through pass only, so it stays see-through; parts a strong blue in both
        moving = element.name == frame["moving"] and not part
        colour = [0.0, 0.32, 0.86, 1.0] if part else [0.97, 0.97, 0.97, 1.0] if solid else [0.55, 0.75, 0.95, 1.0] if moving else None
        if colour:
            branch.set_node_color(branch.get_node(element.guid), Color(*colour))
    for line in list(branch.objects.lines):
        branch.remove_object(line.guid)
    return branch

# the bounds of every frame, so all are drawn from one camera
low, high = [1e18] * 3, [-1e18] * 3
for frame in plan["frames"]:
    for element in frame_branch(frame, False).objects.elements:
        for corner in element.aabb.get_corners():
            for k in range(3):
                low[k] = min(low[k], corner[k])
                high[k] = max(high[k], corner[k])
paths = []
for n, frame in enumerate(plan["frames"]):
    pair = []
    for solid in (False, True):
        branch = frame_branch(frame, solid)
        for a, b in ((low, [low[0] + 0.5, low[1], low[2]]), (high, [high[0] - 0.5, high[1], high[2]])):
            mark = Line.from_points(Point(*a), Point(*b))
            mark.linecolor = Color(0.969, 0.969, 0.969, 1.0)
            branch.add_line(mark, None)
        path = "%s/frame_%03d%s.pb" % (out, n, "_solid" if solid else "")
        branch.pb_dump(path)
        pair.append(path)
    paths.append(pair)
print(json.dumps({"paths": paths, "target": [(a + b) / 2 for a, b in zip(low, high)]}))
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
        os.environ, VIEWER_SHEETS="0", VIEWER_OPACITY=opacity, VIEWER_W="3600", VIEWER_H="2400", VIEWER_DPR="2",
        VIEWER_THICKNESS="1", VIEWER_VIEW="iso", VIEWER_PLANE_SIZE="0", VIEWER_NO_GRID="1", VIEWER_AO="1", VIEWER_OUTLINES="1",
        VIEWER_TARGET=",".join(f"{value:.3f}" for value in target),
    )
    with tempfile.TemporaryDirectory() as tmp:
        ppm = pathlib.Path(tmp) / "frame.ppm"
        subprocess.run([str(RENDERER), str(ppm), pb], env=env, check=True, capture_output=True)
        return Image.open(ppm).convert("RGB")


def drawing(pair: list, target: list) -> Image.Image:
    """The see-through pass with the blue of the solid one laid over it where a part or the moving piece is in view."""
    image = render(pair[0], target, "0.45")
    strong = render(pair[1], target, "1.0")
    red, green, blue = strong.split()
    bluish = ImageChops.subtract(blue, red).point(lambda value: 255 if value > 60 else 0)
    image.paste(strong, (0, 0), bluish)
    return image


def pictures(design: str, orders: dict, scene: pathlib.Path, slides: int) -> list:
    """The picture and the GIF of design's square corner (its inclined corner when it has no square one)."""
    variant = f"{design}_square" if f"{design}_square" in orders or f"{design}_inclined" not in orders else f"{design}_inclined"
    order = orders.get(variant, [])
    frames = plan_frames(order, slides)
    with tempfile.TemporaryDirectory() as tmp:
        plan_path = pathlib.Path(tmp) / "plan.json"
        plan_path.write_text(json.dumps({"frames": frames}))
        listing = subprocess.run([str(PYTHON), "-c", FRAMES, str(scene), design, variant, str(plan_path), tmp], check=True, capture_output=True, text=True).stdout
        written = json.loads(listing.strip().splitlines()[-1])
        images = [drawing(pair, written["target"]) for pair in written["paths"]]

    # cropped alike to what any frame draws with a margin, framed 3:2, halved; the GIF on one palette, undithered
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
        framed.append(canvas.resize((width // 2, height // 2), Image.LANCZOS))
    picture = PICTURES / f"beam_{design}.png"
    framed[0].save(picture)
    # the palette from every frame, so the moving piece keeps its tint
    sample = Image.new("RGB", (framed[0].width, framed[0].height * len(framed)))
    for k, image in enumerate(framed):
        sample.paste(image, (0, framed[0].height * k))
    palette = sample.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
    animation = PICTURES / f"beam_{design}.gif"
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
