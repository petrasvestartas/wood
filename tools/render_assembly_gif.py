"""The assembly of a chevron template's 4 x 4 block as a GIF, docs/images/templates/<example>.gif (<example>_assembly.gif when the name does not end in _assembly): the example writes its scene and
data/output/pb/assembly.json (the reach, the order, every box's plates, vector and bounds, the block); every box of the block then slides in
along its vector into the ones placed before it, in the order, over `--slides` frames each, every frame a scene the viewer renders with the
camera fitted once to the block in place and every box at its start, so the frames hold still, all cropped alike to what they draw.

    python tools/render_assembly_gif.py templates_chevron_4_chevron_assembly [--slides 5] [--size 2400x1600] [--scale 0.5] [--view iso] [--opacity 1.0]
"""

import json
import os
import pathlib
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import render_template_docs as docs  # noqa: E402

FRAMES = r"""
import json, sys
from session_py.session import Session
from session_py.line import Line
from session_py.point import Point
from session_py.xform import Xform
scene = Session.pb_load(sys.argv[1])
plan = json.load(open(sys.argv[2]))
out = sys.argv[3]
slides = int(sys.argv[4])
reach = plan["reach"]
boxes = {}
for i in plan["block"]:
    branch = scene.get_branch("box_%d" % i)
    for element in branch.objects.elements:
        element.set_features([])
    for edge in list(branch.interactions):
        branch.interactions[edge] = []
    boxes[i] = branch
order = [i for i in plan["order"] if i in boxes]
# the bounds every frame keeps: the block in place and every box where it starts
low = [1e18] * 3
high = [-1e18] * 3
for i in order:
    box = plan["boxes"][i]
    for corner in (box["low"], box["high"]):
        for k in range(3):
            low[k] = min(low[k], corner[k], corner[k] - box["direction"][k] * reach)
            high[k] = max(high[k], corner[k], corner[k] - box["direction"][k] * reach)
frames = []
for n, i in enumerate(order):
    v = plan["boxes"][i]["direction"]
    names = set(plan["boxes"][i]["plates"])
    centre = [(plan["boxes"][i]["low"][k] + plan["boxes"][i]["high"][k]) / 2 for k in range(3)]
    for s in range(slides + 1):
        d = reach * (1 - s / slides)
        frame = Session("frame")
        for j in order[:n]:
            frame.graft(boxes[j], None)
        frame.graft(boxes[i], None)
        for element in frame.objects.elements:
            if element.name in names:
                element.place(Xform.translation(-v[0] * d, -v[1] * d, -v[2] * d))
        # the vector: a red line 400 mm long behind the box along it, ending where the box's bounds start
        behind = max(abs(sum((corner[k] - centre[k]) * v[k] for k in range(3))) for corner in [[plan["boxes"][i]["low" if bit else "high"][k] for k, bit in enumerate(bits)] for bits in ((0, 0, 0), (0, 0, 1), (0, 1, 0), (0, 1, 1), (1, 0, 0), (1, 0, 1), (1, 1, 0), (1, 1, 1))])
        arrow = Line.from_points(Point(*[centre[k] - v[k] * (d + behind + 400.0) for k in range(3)]), Point(*[centre[k] - v[k] * (d + behind) for k in range(3)]))
        arrow.width = 6.0
        from session_py.color import Color
        arrow.linecolor = Color(206, 16, 66, 255)
        frame.add_line(arrow, None)
        # the frame's bounds held by two short lines at the corners
        frame.add_line(Line.from_points(Point(*low), Point(low[0] + 1.0, low[1], low[2])), None)
        frame.add_line(Line.from_points(Point(*high), Point(high[0] - 1.0, high[1], high[2])), None)
        path = "%s/frame_%03d.pb" % (out, len(frames))
        frame.pb_dump(path)
        frames.append({"path": path, "landed": s == slides, "last": n == len(order) - 1 and s == slides})
print(json.dumps({"frames": frames, "target": [(low[k] + high[k]) / 2 for k in range(3)]}))
"""


def render_frame(
    pb: pathlib.Path, target: list, view: str, opacity: str, size: str
) -> Image.Image:
    """One frame as the docs renderer draws it, the camera aimed at target and fitted to the whole scene, nothing cropped."""
    width, height = (int(side) for side in size.split("x"))
    env = dict(
        os.environ,
        VIEWER_SHEETS="0",
        VIEWER_OPACITY=opacity,
        VIEWER_W=str(width),
        VIEWER_H=str(height),
        VIEWER_DPR="1",
        VIEWER_THICKNESS="1",
        VIEWER_VIEW=view,
        VIEWER_PLANE_SIZE="0",
        VIEWER_NO_GRID="1",
        VIEWER_AO="1",
        VIEWER_TARGET=",".join(f"{value:.3f}" for value in target),
    )
    env.setdefault("VIEWER_ADAPTER", "nvidia")

    with tempfile.TemporaryDirectory() as tmp:
        ppm = pathlib.Path(tmp) / "frame.ppm"
        subprocess.run(
            [str(docs.RENDERER), str(ppm), str(pb)],
            env=env,
            check=True,
            capture_output=True,
        )
        image = Image.open(ppm).convert("RGB")

    clear = image.getpixel((2, 2))
    return Image.merge(
        "RGB",
        [
            band.point(
                lambda value, channel=channel: min(
                    255, round(value * 255 / max(channel, 1))
                )
            )
            for band, channel in zip(image.split(), clear)
        ],
    )


def main(arguments: list) -> None:
    """Saves docs/images/templates/<example>_assembly.gif."""
    slides = int(docs.option(arguments, "--slides", "5"))
    size = docs.option(arguments, "--size", "2400x1600")
    scale = float(docs.option(arguments, "--scale", "0.5"))
    view = docs.option(arguments, "--view", "iso")
    opacity = docs.option(arguments, "--opacity", "1.0")
    name = arguments[0]
    docs.IMAGES.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp:
        scene = pathlib.Path(tmp) / "scene.pb"
        docs.run_example(name, scene)
        plan = docs.WOOD / "data" / "output" / "pb" / "assembly.json"
        listing = subprocess.run(
            [str(docs.PYTHON), "-c", FRAMES, str(scene), str(plan), tmp, str(slides)],
            check=True,
            capture_output=True,
            text=True,
        ).stdout
        frames = json.loads(listing.strip().splitlines()[-1])

        images = []
        durations = []
        for frame in frames["frames"]:
            images.append(
                render_frame(pathlib.Path(frame["path"]), frames["target"], view, opacity, size)
            )
            durations.append(2500 if frame["last"] else 600 if frame["landed"] else 120)

        # the frames cropped alike to what any of them draws, a margin around, then scaled
        box = None
        for image in images:
            drawn = ImageChops.difference(image, Image.new("RGB", image.size, image.getpixel((2, 2)))).getbbox()
            if drawn and box is None:
                box = list(drawn)
            elif drawn:
                box = [min(box[0], drawn[0]), min(box[1], drawn[1]), max(box[2], drawn[2]), max(box[3], drawn[3])]
        if box:
            margin = 40
            box = (max(0, box[0] - margin), max(0, box[1] - margin), min(images[0].width, box[2] + margin), min(images[0].height, box[3] + margin))
        for n, image in enumerate(images):
            if box:
                image = image.crop(box)
            image = image.resize((int(image.width * scale), int(image.height * scale)), Image.LANCZOS)
            images[n] = image.quantize(colors=64, method=Image.Quantize.MEDIANCUT)

    picture = docs.IMAGES / (f"{name}.gif" if name.endswith("_assembly") else f"{name}_assembly.gif")
    images[0].save(
        picture,
        save_all=True,
        append_images=images[1:],
        duration=durations,
        loop=0,
        optimize=True,
    )
    print(picture.relative_to(docs.WOOD), len(images), "frames")


if __name__ == "__main__":
    main(sys.argv[1:])
