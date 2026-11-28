"""The film of the landing page: the timber floor of templates_floor_7_contacts_cantilevers coming apart piece by piece, its
connectors brown, and going together again, a loop. The moves are hero_moves.py's, checked there for collisions. Every frame is
drawn by the viewer's native renderer from one isometric camera and cropped alike; written as docs/doxygen/hero.mp4,
hero_2x.mp4 for dense screens, hero_small.mp4 for phones and hero.webp, its still apart (the poster).

    uv run --with av --with pillow --with numpy --with manifold3d tools/render_hero.py [--check [mm] | --frames n]"""

import concurrent.futures
import json
import os
import pathlib
import subprocess
import sys

import av
import hero_moves
from PIL import Image, ImageChops

import render_joint_library_gifs as library

WOOD = library.WOOD
LAIKA = WOOD.parent / "laika"
PYTHON = library.PYTHON
RENDERER = library.RENDERER
EXAMPLE = "templates_floor_7_contacts_cantilevers"
WORK = WOOD / "build/hero"  # The scenes and the frames: too many for a temporary folder in memory.
RENDERED = (8000, 6400)  # What is drawn, before the crop.
WIDTHS = {"hero_2x.mp4": 2160, "hero.mp4": 1080, "hero_small.mp4": 760}  # For dense screens, for others and for phones: a browser that shrinks one a lot breaks its thin lines up.
RATE = 30
BOUNDS = ((-4300.0, -4300.0, -500.0), (4300.0, 4300.0, 7000.0))  # Of every frame, so one camera frames them all.

# The meshes of a scene's pieces, where they stand.
MESHES = r"""
import json, sys
from laika_py.session import Session
scene, out = sys.argv[1:3]
pieces = []
scene = Session.pb_load(scene)
for element in scene.objects.elements:
    mesh = element.geometry.mesh() if type(element.geometry).__name__ == "BRep" else element.geometry
    if mesh.number_of_faces():
        points, faces = mesh.to_vertices_and_faces()
        world = scene.world_xform(element.guid)
        points = [point.transformed(world) for point in points] if world is not None else points
        pieces.append({"name": element.name, "v": [[p[0], p[1], p[2]] for p in points], "f": [list(face) for face in faces]})
json.dump(pieces, open(out, "w"))
"""

# Scenes of frames: every piece moved by its displacement in the plan, what is no piece left out, the connectors brown. Lines in
# the background's colour at the corners of the bounds keep the camera.
SCENES = r"""
import json, sys
from laika_py.session import Session
from laika_py.xform import Xform
from laika_py.color import Color
from laika_py.line import Line
from laika_py.point import Point
source, plan_path, out, first, last = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5])
BROWN = Color(0.4196, 0.2667, 0.1373, 1.0)  # #AD8D68, linear
plan = json.load(open(plan_path))
for frame in range(first, last):
    scene = Session.pb_load(source)
    for edge in list(scene.interactions):
        scene.interactions[edge] = []
    for drawn in list(scene.objects.lines) + list(scene.objects.planes) + list(scene.objects.polylines):
        scene.remove_object(drawn.guid)
    for element in list(scene.objects.elements):
        moved = plan["frames"][frame].get(element.name)
        if moved is None:
            if not element.name.startswith("connector"):
                scene.remove_object(element.guid)
            continue
        element.set_features([])
        element.place(Xform.translation(*moved))
        if element.name.startswith(("connector", "column_plate")):
            scene.set_node_color(scene.get_node(element.guid), BROWN)
    for corner in plan["bounds"]:
        mark = Line.from_points(Point(*corner), Point(corner[0] + 0.5, corner[1], corner[2]))
        mark.linecolor = Color(0.969, 0.969, 0.969, 1.0)
        scene.add_line(mark, None)
    scene.pb_dump("%s/frame_%03d.pb" % (out, frame))
"""


def render(scene: pathlib.Path) -> Image.Image:
    """A scene drawn isometric in the Arctic look, as the joint library's pictures."""
    env = dict(
        os.environ, VIEWER_SHEETS="0", VIEWER_OPACITY="1.0", VIEWER_W=str(RENDERED[0]), VIEWER_H=str(RENDERED[1]), VIEWER_DPR="1.5",
        VIEWER_THICKNESS="0.5", VIEWER_VIEW="iso", VIEWER_PLANE_SIZE="0", VIEWER_NO_GRID="1", VIEWER_AO="1", VIEWER_OUTLINES="1",
    )
    picture = scene.with_suffix(".ppm")
    subprocess.run([str(RENDERER), "render-pb", str(picture), str(scene)], env=env, check=True, capture_output=True)
    image = Image.open(picture).convert("RGB")
    picture.unlink()
    return image


def write_film(path: pathlib.Path, images: list) -> None:
    """The images as a silent H.264 film every browser plays."""
    with av.open(str(path), "w", options={"movflags": "faststart"}) as film:
        stream = film.add_stream("libx264", rate=RATE)
        stream.width, stream.height, stream.pix_fmt = *images[0].size, "yuv420p"
        stream.options = {"crf": "21", "preset": "veryslow", "tune": "animation"}
        for image in images:
            film.mux(stream.encode(av.VideoFrame.from_image(image)))
        film.mux(stream.encode())


def frame_picture(frame: int) -> None:
    """A frame's scene drawn, kept as a PNG; the scene goes."""
    scene = WORK / f"frame_{frame:03d}.pb"
    render(scene).save(WORK / f"frame_{frame:03d}.png", compress_level=1)
    scene.unlink()


def floor_scene() -> pathlib.Path:
    """The floor's scene, written by its example."""
    WORK.mkdir(parents=True, exist_ok=True)
    source = WORK / "floor.pb"
    live = WOOD / "data/output/pb/live.pb"
    kept = live.read_bytes() if live.exists() else None
    subprocess.run([str(WOOD / "build" / EXAMPLE)], cwd=WOOD, check=True, capture_output=True)
    source.write_bytes(live.read_bytes())
    if kept is not None:
        live.write_bytes(kept)
    return source


def main(arguments: list) -> None:
    """Checks the moves, or draws the frames of the way apart and writes the films and the poster."""
    source = floor_scene()
    subprocess.run([str(PYTHON), "-c", MESHES, str(source), str(WORK / "pieces.json")], check=True)
    pieces = [piece for piece in json.loads((WORK / "pieces.json").read_text()) if hero_moves.shown(piece["name"])]
    (WORK / "pieces.json").write_text(json.dumps(pieces))
    film = hero_moves.Film([piece["name"] for piece in pieces])
    if "--check" in arguments:
        step = float(arguments[-1]) if arguments[-1] != "--check" else 4.0
        raise SystemExit(len(hero_moves.check(film, hero_moves.solids(WORK / "pieces.json"), step)))

    # the scenes, four processes at once, then the pictures, two at once
    count = int(arguments[arguments.index("--frames") + 1]) if "--frames" in arguments else 168
    times = [film.length * frame / (count - 1) for frame in range(count)]
    plan = {"bounds": BOUNDS, "frames": [{name: list(film.displacement(name, time)) for name in film.moves} for time in times]}
    (WORK / "plan.json").write_text(json.dumps(plan))
    for first in range(0, count, 24):
        batch = range(first, min(first + 24, count))
        parts = [round(first + len(batch) * k / 4) for k in range(5)]
        runs = [subprocess.Popen([str(PYTHON), "-c", SCENES, str(source), str(WORK / "plan.json"), str(WORK), str(a), str(b)]) for a, b in zip(parts, parts[1:])]
        if any([run.wait() for run in runs]):
            raise SystemExit("a scene was not written")
        with concurrent.futures.ThreadPoolExecutor(2) as pool:
            list(pool.map(frame_picture, batch))

    # cropped alike to what any frame draws, with a margin, an even number of pixels each way
    paths = [WORK / f"frame_{frame:03d}.png" for frame in range(count)]
    box = None
    for path in paths:
        image = Image.open(path)
        clear = Image.new("RGB", image.size, image.getpixel((2, 2)))
        drawn = ImageChops.difference(image, clear).convert("L").point(lambda value: 255 if value > 14 else 0).getbbox()
        box = drawn if box is None else (min(box[0], drawn[0]), min(box[1], drawn[1]), max(box[2], drawn[2]), max(box[3], drawn[3]))
    box = (box[0] - 40, box[1] - 40, box[2] + 40, box[3] + 40)

    folder = WOOD / "docs/doxygen"
    for name, width in WIDTHS.items():
        size = (width, round((box[3] - box[1]) * width / (box[2] - box[0]) / 2) * 2)
        scaled = [Image.open(path).crop(box).resize(size, Image.Resampling.LANCZOS) for path in paths]
        write_film(folder / name, [scaled[-1]] * 45 + scaled[::-1] + [scaled[0]] * 40 + scaled)
        if name == "hero_2x.mp4":
            scaled[-1].save(folder / "hero.webp", quality=88, method=6)
        print(name, size, (folder / name).stat().st_size)
    print("cropped", box)


main(sys.argv[1:])
