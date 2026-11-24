"""The film of the landing page: the timber floor of templates_floor_7_contacts_cantilevers coming apart family by family, its
connectors brown, and going together again, a loop. Every frame is drawn by the viewer's native renderer from one isometric
camera and cropped alike; written as docs/doxygen/hero.mp4, hero_small.mp4
for phones and hero.webp, its still apart (the poster).

    uv run --with av --with pillow tools/render_hero.py [frames of the way apart]"""

import concurrent.futures
import os
import pathlib
import subprocess
import sys
import tempfile

import av
from PIL import Image, ImageChops

WOOD = pathlib.Path(__file__).resolve().parents[1]
SESSION = WOOD.parent / "session"
PYTHON = SESSION / "session_py/.venv/bin/python"
RENDERER = SESSION / "session_viewer/target/x86_64-unknown-linux-gnu/release/examples/selftest"
EXAMPLE = "templates_floor_7_contacts_cantilevers"
RENDERED = (3200, 2600)  # What is drawn, before the crop.
WIDTHS = {"hero.mp4": 1200, "hero_small.mp4": 640}  # Of the film and of its copy for phones, whose thin lines a browser would break up.
RATE = 30

# The frames as scenes: every element placed by its family's way apart, the family's share of it eased and delayed, so the beds
# lift first and the columns come last. Lines in the background's colour at the corners of every frame's bounds keep the camera.
SCENES = r"""
import sys
from session_py.session import Session
from session_py.xform import Xform
from session_py.color import Color
from session_py.line import Line
from session_py.point import Point
source, out, count, first, last = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])
BROWN = Color(0.4196, 0.2667, 0.1373, 1.0)  # #AD8D68, linear
OUT = 620.0  # every quarter away from the seams
# a family's name, its lift, how far out it goes (in OUT) and when it starts (of the whole way)
FAMILIES = [
    ("beds", 2350.0, 1.0, 0.00), ("tsections", 1500.0, 1.0, 0.10), ("connector_pins_outer", 1150.0, 1.0, 0.18), ("wedges", 1250.0, 1.0, 0.18),
    ("inner_ribs", 950.0, 1.0, 0.22), ("outer_ribs", 620.0, 1.0, 0.26), ("inner_beams", 620.0, 0.45, 0.26), ("connector_seam", 620.0, 0.0, 0.26),
    ("connector_oculus", 620.0, 0.0, 0.26), ("connector_column_plate", 420.0, 1.0, 0.30), ("column_plate", 420.0, 1.0, 0.30), ("column", 0.0, 1.0, 0.34), ("support", -420.0, 1.0, 0.34),
    ("connector", 620.0, 1.0, 0.26), ("", 620.0, 0.0, 0.26),
]
LONG = 1.0 - 0.34

def placement(element, t):
    family = next(f for f in FAMILIES if element.name.startswith(f[0]))
    share = min(max((t - family[3]) / LONG, 0.0), 1.0)
    share = share * share * (3.0 - 2.0 * share)
    # the quarter it lies in, none on a seam
    centre = element.aabb.center
    x, y = ((centre[k] > 1.0) - (centre[k] < -1.0) for k in (0, 1))
    return Xform.translation(x * OUT * family[2] * share, y * OUT * family[2] * share, family[1] * share)

paths = []
for frame in range(first, last):
    scene = Session.pb_load(source)
    for edge in list(scene.interactions):
        scene.interactions[edge] = []
    for drawn in list(scene.objects.lines) + list(scene.objects.planes) + list(scene.objects.polylines):
        scene.remove_object(drawn.guid)
    for element in list(scene.objects.elements):
        if "cutters" in element.name:
            scene.remove_object(element.guid)
            continue
        element.set_features([])
        element.place(placement(element, frame / (count - 1)))
        if element.name.startswith(("connector", "column_plate")):
            scene.set_node_color(scene.get_node(element.guid), BROWN)
    for a in ((-4200.0, -4200.0, -600.0), (4200.0, 4200.0, 6200.0)):
        mark = Line.from_points(Point(*a), Point(a[0] + 0.5, a[1], a[2]))
        mark.linecolor = Color(0.969, 0.969, 0.969, 1.0)
        scene.add_line(mark, None)
    paths.append("%s/frame_%03d.pb" % (out, frame))
    scene.pb_dump(paths[-1])
"""


def render(scene: pathlib.Path) -> Image.Image:
    """A scene drawn isometric in the Arctic look, as the joint library's pictures."""
    env = dict(
        os.environ, VIEWER_SHEETS="0", VIEWER_OPACITY="1.0", VIEWER_W=str(RENDERED[0]), VIEWER_H=str(RENDERED[1]), VIEWER_DPR="1",
        VIEWER_THICKNESS="0.5", VIEWER_VIEW="iso", VIEWER_PLANE_SIZE="0", VIEWER_NO_GRID="1", VIEWER_AO="1", VIEWER_OUTLINES="1",
    )
    picture = scene.with_suffix(".ppm")
    subprocess.run([str(RENDERER), str(picture), str(scene)], env=env, check=True, capture_output=True)
    image = Image.open(picture).convert("RGB")
    picture.unlink()
    return image


def write_film(path: pathlib.Path, images: list) -> None:
    """The images as a silent H.264 film every browser plays."""
    with av.open(str(path), "w", options={"movflags": "faststart"}) as film:
        stream = film.add_stream("libx264", rate=RATE)
        stream.width, stream.height, stream.pix_fmt = *images[0].size, "yuv420p"
        stream.options = {"crf": "24", "preset": "veryslow", "tune": "animation"}
        for image in images:
            film.mux(stream.encode(av.VideoFrame.from_image(image)))
        film.mux(stream.encode())


def main(count: int) -> None:
    """Draws count frames of the way apart and writes the film and its poster."""
    with tempfile.TemporaryDirectory() as tmp:
        # the floor's scene, written by its example to a folder of its own
        source = pathlib.Path(tmp) / "floor.pb"
        live = WOOD / "data/output/pb/live.pb"
        kept = live.read_bytes() if live.exists() else None
        subprocess.run([str(WOOD / "build" / EXAMPLE)], cwd=WOOD, check=True, capture_output=True)
        source.write_bytes(live.read_bytes())
        if kept is not None:
            live.write_bytes(kept)

        # the scenes, four processes at once, then the pictures one by one
        bounds = [round(k * count / 4) for k in range(5)]
        runs = [subprocess.Popen([str(PYTHON), "-c", SCENES, str(source), tmp, str(count), str(a), str(b)]) for a, b in zip(bounds, bounds[1:])]
        if any([run.wait() for run in runs]):
            raise SystemExit("a scene was not written")
        images = [render(pathlib.Path(tmp) / f"frame_{frame:03d}.pb") for frame in range(count)]

    # cropped alike to what any frame draws, with a margin, an even number of pixels each way
    clear = images[0].getpixel((2, 2))
    box = None
    for image in images:
        drawn = ImageChops.difference(image, Image.new("RGB", image.size, clear)).convert("L").point(lambda value: 255 if value > 14 else 0).getbbox()
        box = drawn if box is None else (min(box[0], drawn[0]), min(box[1], drawn[1]), max(box[2], drawn[2]), max(box[3], drawn[3]))
    box = (box[0] - 24, box[1] - 24, box[2] + 24, box[3] + 24)
    folder = WOOD / "docs/doxygen"
    for name, width in WIDTHS.items():
        size = (width, round((box[3] - box[1]) * width / (box[2] - box[0]) / 2) * 2)
        scaled = [image.crop(box).resize(size, Image.Resampling.LANCZOS) for image in images]
        write_film(folder / name, [scaled[-1]] * 50 + scaled[::-1] + [scaled[0]] * 34 + scaled)
        if name == "hero.mp4":
            scaled[-1].save(folder / "hero.webp", quality=90, method=6)
        print(name, size, (folder / name).stat().st_size)


main(int(sys.argv[1]) if len(sys.argv) > 1 else 48)
