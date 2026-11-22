"""The assembly of every joint of the beam joint library as a GIF, docs/images/joint_library/beam_<design>.gif: the square corner of
each design as element_joint_beam_library draws it (data/output/pb/live.pb), every piece back in place by the offset the
drawing gave it (data/output/pb/joint_library_orders.json), then the pieces standing apart and going in one by one in the
assembly order, each along its way and then through its further moves (in, then along the slot), the moving piece blue. Pieces
pass through each other freely: the GIF shows the order and the ways, not a collision check. Every frame is drawn by the
viewer crate's native renderer with one fixed camera.

    python3 tools/render_joint_library_gifs.py [design ...] [--slides 6] [--size 1500x1000]
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

# One frame per entry of the plan: the design's square corner alone, every piece at the offset the frame gives it, the moving
# one blue, by session_py.
FRAMES = r"""
import json, re, sys
from session_py.session import Session
from session_py.xform import Xform
from session_py.color import Color
from session_py.line import Line
from session_py.point import Point
scene_path, design, variant, plan_path, out = sys.argv[1:6]
plan = json.load(open(plan_path))
frames = []
for n, frame in enumerate(plan["frames"]):
    branch = Session.pb_load(scene_path).get_branch(design)
    for edge in list(branch.interactions):
        branch.interactions[edge] = []
    for element in list(branch.objects.elements):
        if not element.name.startswith(variant + "_") and element.name != variant:
            branch.remove_object(element.guid)
            continue
        element.set_features([])
        offset = frame["offsets"].get(element.name)
        if offset:
            element.place(Xform.translation(*offset))
        part = re.search(r"_(part|pin)(_[0-9]+)?$", element.name) is not None
        colour = [0.13, 0.59, 0.92, 1.0] if element.name == frame["moving"] else [0.0, 0.32, 0.86, 1.0] if part else None
        if colour:
            branch.set_node_color(branch.get_node(element.guid), Color(*colour))
    for line in list(branch.objects.lines):
        branch.remove_object(line.guid)
    low, high = plan["low"], plan["high"]
    # two marks at the corners of the bounds every frame keeps, in the background's colour, so the camera never moves
    for a, b in ((low, [low[0] + 0.5, low[1], low[2]]), (high, [high[0] - 0.5, high[1], high[2]])):
        mark = Line.from_points(Point(*a), Point(*b))
        mark.linecolor = Color(0.969, 0.969, 0.969, 1.0)
        branch.add_line(mark, None)
    path = "%s/frame_%03d.pb" % (out, n)
    branch.pb_dump(path)
    frames.append(path)
print(json.dumps(frames))
"""


def option(arguments: list, flag: str, default: str) -> str:
    """The value after flag, taken out of arguments, or default."""
    if flag not in arguments:
        return default
    index = arguments.index(flag)
    value = arguments[index + 1]
    del arguments[index : index + 2]
    return value


def plan_frames(order: list, slides: int) -> list:
    """Every frame as offsets and the moving piece: all apart, then in one by one along the way and the further moves, held at
    the end. An offset is from where the drawing set the piece; in place is minus its drawing offset."""
    reach = max(200.0, max(sum(abs(v) for v in step["out"]) for step in order) * 1.6) if order else 200.0
    placed = {step["element"]: [-v for v in step["out"]] for step in order}
    apart = {}
    for step in order:
        legs = [sum(leg[k] for leg in step["legs"]) for k in range(3)]
        apart[step["element"]] = [placed[step["element"]][k] - legs[k] - reach * step["way"][k] for k in range(3)]

    def frame(done: int, moving: int, position: list, duration: int) -> dict:
        offsets = {}
        for i, step in enumerate(order):
            name = step["element"]
            offsets[name] = placed[name] if i < done else position if i == moving else apart[name]
        return {"offsets": offsets, "moving": order[moving]["element"] if moving >= 0 else "", "duration": duration}

    frames = [frame(0, -1, None, 1200)]
    for k, step in enumerate(order):
        start = apart[step["element"]]
        legs = step["legs"]
        approach_end = [placed[step["element"]][i] - sum(leg[i] for leg in legs) for i in range(3)]
        for s in range(1, slides + 1):
            t = s / slides
            frames.append(frame(k, k, [start[i] + (approach_end[i] - start[i]) * t for i in range(3)], 60))
        at = approach_end
        for leg in legs:
            for s in range(1, slides // 2 + 2):
                t = s / (slides // 2 + 1)
                frames.append(frame(k, k, [at[i] + leg[i] * t for i in range(3)], 60))
            at = [at[i] + leg[i] for i in range(3)]
        frames[-1]["duration"] = 400
    frames.append(frame(len(order), -1, None, 2000))
    return frames, reach


def render(pb: str, target: list, size: str) -> Image.Image:
    """One frame drawn isometric in the Arctic look, opaque, aimed at target."""
    width, height = size.split("x")
    env = dict(
        os.environ, VIEWER_SHEETS="0", VIEWER_OPACITY="1.0", VIEWER_W=width, VIEWER_H=height, VIEWER_DPR="2",
        VIEWER_THICKNESS="1", VIEWER_VIEW="iso", VIEWER_PLANE_SIZE="0", VIEWER_NO_GRID="1", VIEWER_AO="1", VIEWER_OUTLINES="1",
        VIEWER_TARGET=",".join(f"{value:.3f}" for value in target),
    )
    with tempfile.TemporaryDirectory() as tmp:
        ppm = pathlib.Path(tmp) / "frame.ppm"
        subprocess.run([str(RENDERER), str(ppm), pb], env=env, check=True, capture_output=True)
        return Image.open(ppm).convert("RGB")


def gif(design: str, orders: dict, scene: pathlib.Path, slides: int, size: str) -> pathlib.Path:
    """The GIF of design's square corner (its inclined corner when it has no square one)."""
    variant = f"{design}_square" if f"{design}_square" in orders else f"{design}_inclined"
    order = orders.get(variant, [])
    frames, reach = plan_frames(order, slides)
    with tempfile.TemporaryDirectory() as tmp:
        # the bounds every frame keeps: the design's own box widened by the reach every piece stands apart
        box_json = subprocess.run([str(PYTHON), "-c", BOX, str(scene), design, variant], check=True, capture_output=True, text=True).stdout
        boxes = json.loads(box_json.strip().splitlines()[-1])
        low, high = [1e18] * 3, [-1e18] * 3
        for name, (lo, hi) in boxes.items():
            for frame in frames:
                offset = frame["offsets"].get(name, [0.0, 0.0, 0.0])
                for k in range(3):
                    low[k] = min(low[k], lo[k] + offset[k])
                    high[k] = max(high[k], hi[k] + offset[k])
        plan = {"frames": frames, "low": low, "high": high}
        plan_path = pathlib.Path(tmp) / "plan.json"
        plan_path.write_text(json.dumps(plan))
        listing = subprocess.run([str(PYTHON), "-c", FRAMES, str(scene), design, variant, str(plan_path), tmp], check=True, capture_output=True, text=True).stdout
        paths = json.loads(listing.strip().splitlines()[-1])
        target = [(lo + hi) / 2 for lo, hi in zip(low, high)]
        images = [render(path, target, size) for path in paths]

    # cropped alike to what any frame draws, halved, on one palette, undithered, so no frame flickers
    box = None
    for image in images:
        drawn = ImageChops.difference(image, Image.new("RGB", image.size, image.getpixel((2, 2)))).convert("L").point(lambda value: 255 if value > 12 else 0).getbbox()
        if drawn:
            box = drawn if box is None else (min(box[0], drawn[0]), min(box[1], drawn[1]), max(box[2], drawn[2]), max(box[3], drawn[3]))
    crop = (max(0, box[0] - 30), max(0, box[1] - 30), min(images[0].width, box[2] + 30), min(images[0].height, box[3] + 30))
    half = ((crop[2] - crop[0]) // 2, (crop[3] - crop[1]) // 2)
    images = [image.crop(crop).resize(half, Image.LANCZOS) for image in images]
    sample = Image.new("RGB", (half[0], half[1] * 2))
    sample.paste(images[min(len(images) - 1, slides)], (0, 0))
    sample.paste(images[-1], (0, half[1]))
    palette = sample.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
    images = [image.quantize(palette=palette, dither=Image.Dither.NONE) for image in images]
    picture = PICTURES / f"beam_{design}.gif"
    images[0].save(picture, save_all=True, append_images=images[1:], duration=[f["duration"] for f in frames], loop=0, optimize=True)
    return picture


# Every element's box where the drawing set it, by name.
BOX = r"""
import json, sys
from session_py.session import Session
scene_path, design, variant = sys.argv[1:4]
branch = Session.pb_load(scene_path).get_branch(design)
boxes = {}
for element in branch.objects.elements:
    if not element.name.startswith(variant + "_"):
        continue
    corners = element.aabb.get_corners()
    boxes[element.name] = [[min(c[k] for c in corners) for k in range(3)], [max(c[k] for c in corners) for k in range(3)]]
print(json.dumps(boxes))
"""


def main(arguments: list) -> None:
    """Runs the library example once and saves a GIF per design, every design when none is named."""
    slides = int(option(arguments, "--slides", "6"))
    size = option(arguments, "--size", "2400x1600")
    listing = subprocess.run([str(WOOD / "build/element_joint_beam_library")], cwd=WOOD, check=True, capture_output=True, text=True)
    designs = arguments or [line.split()[1] for line in listing.stdout.splitlines() if line.startswith("DESIGN ")]
    orders = json.loads((WOOD / "data/output/pb/joint_library_orders.json").read_text())
    with tempfile.TemporaryDirectory() as tmp:
        scene = pathlib.Path(tmp) / "scene.pb"
        scene.write_bytes((WOOD / "data/output/pb/live.pb").read_bytes())
        for design in designs:
            print(gif(design, orders, scene, slides, size).relative_to(WOOD), flush=True)


if __name__ == "__main__":
    main(sys.argv[1:])
