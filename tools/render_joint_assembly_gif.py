"""The assembly and disassembly of a jointed model as a GIF, docs/images/templates/<example>.gif (<example>_assembly.gif when the name does not end in _assembly): the example writes
its scene and data/output/pb/joint_assembly.json (AssemblySequence::json_dump). The parts first stand apart, each `--reach` mm
back along the way it goes in, a black arrow on each; then they go in one by one in the order, the moving part blue and
the parts its joints engage light blue; the model holds assembled, then comes apart in the reverse order. Every frame is drawn by the
viewer crate's native renderer with one fixed camera.

    python3 tools/render_joint_assembly_gif.py templates_truss_5_carpentry_assembly [--reach 900] [--slides 10] [--size 3600x2100] [--view iso]
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

# One frame per entry of the plan: where every part stands and which are coloured, written by session_py.
FRAMES = r"""
import json, sys
from session_py.session import Session
from session_py.line import Line, Arrowhead
from session_py.point import Point
from session_py.xform import Xform
from session_py.color import Color
scene_path, plan_path, out = sys.argv[1], sys.argv[2], sys.argv[3]
plan = json.load(open(plan_path))
frames = []
for n, frame in enumerate(plan["frames"]):
    scene = Session.pb_load(scene_path)
    for edge in list(scene.interactions):
        scene.interactions[edge] = []
    for element in scene.objects.elements:
        element.set_features([])
        offset = frame["offsets"].get(element.name)
        if offset:
            element.place(Xform.translation(*offset))
        colour = frame["colours"].get(element.name)
        if colour:
            scene.set_node_color(scene.get_node(element.guid), Color(*colour))
    for arrow in frame["arrows"]:
        line = Line.from_points(Point(*arrow[0]), Point(*arrow[1]))
        line.width = 8.0
        line.arrowhead = Arrowhead.END
        line.linecolor = Color(0.0, 0.0, 0.0, 1.0)
        scene.add_line(line, None)
    low, high = plan["low"], plan["high"]
    scene.add_line(Line.from_points(Point(*low), Point(low[0] + 1.0, low[1], low[2])), None)
    scene.add_line(Line.from_points(Point(*high), Point(high[0] - 1.0, high[1], high[2])), None)
    path = "%s/frame_%03d.pb" % (out, n)
    scene.pb_dump(path)
    frames.append(path)
print(json.dumps(frames))
"""

MOVING = [33 / 255, 150 / 255, 234 / 255, 1.0]
ENGAGED = [170 / 255, 210 / 255, 245 / 255, 1.0]


def option(arguments: list, flag: str, default: str) -> str:
    """The value after flag, taken out of arguments, or default."""
    if flag not in arguments:
        return default
    index = arguments.index(flag)
    value = arguments[index + 1]
    del arguments[index : index + 2]
    return value


def plan_frames(sequence: dict, reach: float, slides: int) -> list:
    """Every frame as offsets, colours, arrows and a duration: apart, in one by one, assembled, out one by one in reverse."""
    steps = sequence["steps"]
    apart = {step["element"]: [-reach * v for v in step["insertion"]] for step in steps}

    def arrow(step: dict, distance: float) -> list:
        """A 1000 mm arrow behind the part along its insertion, ending where the part's box starts."""
        v = step["insertion"]
        centre = [(lo + hi) / 2 - distance * vk for lo, hi, vk in zip(step["low"], step["high"], v)]
        behind = sum(abs(hi - lo) / 2 * abs(vk) for lo, hi, vk in zip(step["low"], step["high"], v))
        return [[c - vk * (behind + 1000.0) for c, vk in zip(centre, v)], [c - vk * behind for c, vk in zip(centre, v)]]

    def frame(placed: int, moving: int, fraction: float, duration: int) -> dict:
        """Parts before `placed` in place, the part `moving` fraction of the way in, the rest apart with their arrows."""
        offsets, colours, arrows = {}, {}, []
        for i, step in enumerate(steps):
            if i == moving:
                offsets[step["element"]] = [(1.0 - fraction) * a for a in apart[step["element"]]]
                colours[step["element"]] = MOVING
                for partner in step["partners"]:
                    colours[partner] = ENGAGED
                if fraction < 1.0:
                    arrows.append(arrow(step, (1.0 - fraction) * reach))
            elif i >= placed and i != moving:
                offsets[step["element"]] = apart[step["element"]]
                if any(step["insertion"]):
                    arrows.append(arrow(step, reach))
        return {"offsets": offsets, "colours": colours, "arrows": arrows, "duration": duration}

    frames = [frame(1, -1, 0.0, 2000)]
    for k in range(1, len(steps)):
        for s in range(slides + 1):
            frames.append(frame(k, k, s / slides, 700 if s == slides else 70))
    frames.append(frame(len(steps), -1, 0.0, 2000))
    for k in reversed(range(1, len(steps))):
        for s in range(slides + 1):
            frames.append(frame(k, k, 1.0 - s / slides, 70))
    frames.append(frame(1, -1, 0.0, 1500))
    return frames


def render(pb: str, target: list, view: str, size: str) -> Image.Image:
    """One frame as the native renderer draws it in the viewer's Arctic look (ambient occlusion and outlines, the outline
    drawn at twice its width for the halving), aimed at target and fitted to the whole scene."""
    width, height = size.split("x")
    env = dict(
        os.environ, VIEWER_SHEETS="0", VIEWER_OPACITY="1.0", VIEWER_W=width, VIEWER_H=height, VIEWER_DPR="2",
        VIEWER_THICKNESS="1", VIEWER_VIEW=view, VIEWER_PLANE_SIZE="0", VIEWER_NO_GRID="1", VIEWER_AO="1", VIEWER_OUTLINES="1",
        VIEWER_TARGET=",".join(f"{value:.3f}" for value in target),
    )
    with tempfile.TemporaryDirectory() as tmp:
        ppm = pathlib.Path(tmp) / "frame.ppm"
        subprocess.run([str(RENDERER), str(ppm), pb], env=env, check=True, capture_output=True)
        return Image.open(ppm).convert("RGB")


def main(arguments: list) -> None:
    """Runs the example, plans the frames, renders them and saves the GIF."""
    reach = float(option(arguments, "--reach", "900"))
    slides = int(option(arguments, "--slides", "10"))
    size = option(arguments, "--size", "3600x2100")
    view = option(arguments, "--view", "iso")
    name = arguments[0]
    subprocess.run([str(WOOD / "build" / name)], cwd=WOOD, check=True, capture_output=True)
    sequence = json.loads((WOOD / "data/output/pb/joint_assembly.json").read_text())

    # the bounds every frame keeps: every part in place and apart
    low, high = [1e18] * 3, [-1e18] * 3
    for step in sequence["steps"]:
        for corner in (step["low"], step["high"]):
            for k in range(3):
                low[k] = min(low[k], corner[k], corner[k] - reach * step["insertion"][k])
                high[k] = max(high[k], corner[k], corner[k] - reach * step["insertion"][k])

    with tempfile.TemporaryDirectory() as tmp:
        plan = {"frames": plan_frames(sequence, reach, slides), "low": low, "high": high}
        plan_path = pathlib.Path(tmp) / "plan.json"
        plan_path.write_text(json.dumps(plan))
        listing = subprocess.run(
            [str(PYTHON), "-c", FRAMES, str(WOOD / "data/output/pb/live.pb"), str(plan_path), tmp],
            check=True, capture_output=True, text=True,
        ).stdout
        paths = json.loads(listing.strip().splitlines()[-1])
        target = [(lo + hi) / 2 for lo, hi in zip(low, high)]
        images = [render(path, target, view, size) for path in paths]

    # cropped alike to what any frame draws, halved for smooth edges, on one 256-colour palette taken from the assembled
    # frame and the first coloured one, undithered, so no frame flickers or bands
    box = None
    for image in images:
        drawn = ImageChops.difference(image, Image.new("RGB", image.size, image.getpixel((2, 2)))).getbbox()
        if drawn:
            box = drawn if box is None else (min(box[0], drawn[0]), min(box[1], drawn[1]), max(box[2], drawn[2]), max(box[3], drawn[3]))
    crop = (max(0, box[0] - 40), max(0, box[1] - 40), min(images[0].width, box[2] + 40), min(images[0].height, box[3] + 40))
    half = ((crop[2] - crop[0]) // 2, (crop[3] - crop[1]) // 2)
    images = [image.crop(crop).resize(half, Image.LANCZOS) for image in images]
    sample = Image.new("RGB", (half[0], half[1] * 2))
    sample.paste(images[slides + 1], (0, 0))
    sample.paste(images[len(images) // 2], (0, half[1]))
    palette = sample.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
    images = [image.quantize(palette=palette, dither=Image.Dither.NONE) for image in images]
    picture = WOOD / "docs/images/templates" / (f"{name}.gif" if name.endswith("_assembly") else f"{name}_assembly.gif")
    images[0].save(picture, save_all=True, append_images=images[1:], duration=[f["duration"] for f in plan["frames"]], loop=0, optimize=True)
    print(picture.relative_to(WOOD), len(images), "frames")


if __name__ == "__main__":
    main(sys.argv[1:])
