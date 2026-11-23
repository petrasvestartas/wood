"""The element pages' pictures, docs/images/elements/<example>.png, drawn as the joint library draws its joints
(tools/render_joint_library_gifs.py): see-through and as dark as solid, one fixed isometric camera, 960 x 640, so every
picture of the docs reads alike. Each example is run, its scene (data/output/pb/live.pb) drawn whole.

    python3 tools/render_element_pictures.py [example ...]
    python3 tools/render_element_pictures.py --scenes <folder> <scene.pb ...>   each scene drawn into <folder>/<its name>.png
    python3 tools/render_element_pictures.py --close <folder> <scene.pb> <name> <x,y,z> <zoom>   a close-up of one point
"""

import os
import pathlib
import subprocess
import sys
import tempfile

import render_joint_library_gifs as library

WOOD = library.WOOD
FOLDER = WOOD / "docs/images/elements"
SKIPPED = {"element_joint_beam_library"}  # The compendium's overview: every design at once, drawn by its own example.
LARGE = (1920, 1280)  # A scene drawn whole (a truss of 12 m) or a point close up: twice the joint pictures, sharp on a wide page.


def draw_scenes(folder: pathlib.Path, scenes: list) -> None:
    """Draws each scene file whole into folder, named after it (the truss library's truss_<name>.pb as truss_<name>.png)."""
    for scene in scenes:
        for path in library.draw(pathlib.Path(scene), "-", "", [], 1, pathlib.Path(scene).stem, {"joints": [], "parts": []}, folder, animate=False, size=LARGE):
            print(path, flush=True)


# The scene without its lines, planes and polylines (a template's drawn guide), the interactions' features dropped: wood alone.
STRIP = r"""
import sys
from session_py.session import Session
scene = Session.pb_load(sys.argv[1])
for edge in list(scene.interactions):
    scene.interactions[edge] = []
for line in list(scene.objects.lines) + list(scene.objects.planes) + list(scene.objects.polylines):
    scene.remove_object(line.guid)
scene.pb_dump(sys.argv[2])
"""


def draw_close(folder: pathlib.Path, scene: str, name: str, target: str, zoom: str) -> None:
    """A close-up of the point target (x,y,z in mm) of the scene, zoom wheel steps in, drawn like every picture of the docs."""
    with tempfile.TemporaryDirectory() as tmp:
        wood = pathlib.Path(tmp) / "wood.pb"
        subprocess.run([str(library.PYTHON), "-c", STRIP, scene, str(wood)], check=True)
        os.environ["VIEWER_ZOOM"] = zoom
        image = library.drawing([str(wood), str(wood)], [float(value) for value in target.split(",")])
        del os.environ["VIEWER_ZOOM"]
    path = folder / f"{name}.png"
    image.resize(LARGE).save(path)
    print(path, flush=True)


def main(arguments: list) -> None:
    """Draws each example named, every example with a picture under docs/images/elements when none is; --scenes draws scene
    files instead."""
    if arguments[:1] == ["--scenes"]:
        draw_scenes(WOOD / arguments[1], arguments[2:])
        return
    if arguments[:1] == ["--close"]:
        draw_close(WOOD / arguments[1], *arguments[2:6])
        return
    examples = arguments or sorted(path.stem for path in FOLDER.glob("element_*.png") if path.stem not in SKIPPED)
    for example in examples:
        binary = WOOD / "build" / example
        if not binary.exists():
            print(f"skipped {example}: not built", flush=True)
            continue
        live = WOOD / "data/output/pb/live.pb"
        before = live.stat().st_mtime_ns if live.exists() else 0
        run = subprocess.run([str(binary)], cwd=WOOD, env=dict(os.environ), capture_output=True, text=True)
        if run.returncode != 0 or not live.exists() or live.stat().st_mtime_ns == before:
            print(f"failed {example}: exit {run.returncode}\n{run.stderr[-2000:]}", flush=True)
            continue
        with tempfile.TemporaryDirectory() as tmp:
            scene = pathlib.Path(tmp) / "scene.pb"
            scene.write_bytes(live.read_bytes())
            for path in library.draw(scene, "-", "", [], 1, example, {"joints": [], "parts": []}, FOLDER, animate=False):
                print(path.relative_to(WOOD), flush=True)


if __name__ == "__main__":
    main(sys.argv[1:])
