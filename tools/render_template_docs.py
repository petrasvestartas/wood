import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
import time

from PIL import Image

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import crop_viewer_shot  # noqa: E402

WOOD = pathlib.Path(__file__).resolve().parents[1]
WINDOWS = os.name == "nt"
# the laika_py environment that reads and writes scenes; WOOD_PYTHON names another when wood is not checked out beside session
PYTHON = pathlib.Path(os.environ["WOOD_PYTHON"]) if "WOOD_PYTHON" in os.environ else WOOD.parent / (
    "laika/laika_py/.venv/Scripts/python.exe"
    if WINDOWS
    else "laika/laika_py/.venv/bin/python"
)
IMAGES = WOOD / "docs" / "images" / "templates"
BUILD = pathlib.Path(
    os.environ.get("WOOD_BUILD", WOOD / "build" / "Release" if WINDOWS else WOOD / "build")
)
LOCK = pathlib.Path(tempfile.gettempdir()) / "wood_live_pb.lock"

BRANCH = r"""
import sys
from laika_py.session import Session
from laika_py.xform import Xform
scene = Session.pb_load(sys.argv[1])
names = sys.argv[2].split("+")
branch = scene if names[0] == "all" else scene.get_branch(names[0])
for name in names[1:]:
    branch.graft(scene.get_branch(name), None)
if sys.argv[4] == "elements":
    for element in branch.objects.elements:
        element.set_features([])
    for edge in list(branch.interactions):
        branch.interactions[edge] = []
if float(sys.argv[5]):
    for child in branch.tree.root.children:
        branch.set_xform(child.name, Xform.rotation_z(float(sys.argv[5]), True) * branch.xform(child.name))
branch.name = ""  # no document title over the scene
branch.pb_dump(sys.argv[3])
"""  # the scene, or groups of its tree joined by + with everything under them, written to their own file; the elements alone, their cut solids without
# their attributes, unless asked for: no features (the joint lines and volumes) and no interactions (the face contacts the viewer fills red); the whole turned about z by the degrees given


def run_example(name: str, out: pathlib.Path) -> None:
    """Runs the example and copies the live.pb it writes, locked so two runs never share the file; a scene already at out is kept."""
    live = WOOD / "data" / "output" / "pb" / "live.pb"

    with open(LOCK, "w") as lock:
        if WINDOWS:
            import msvcrt

            # LK_LOCK gives up after ten seconds, shorter than a slow example
            while True:
                try:
                    msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
                    break
                except OSError:
                    time.sleep(1)
        else:
            import fcntl

            fcntl.flock(lock, fcntl.LOCK_EX)

        if out.exists():
            return

        subprocess.run([str(BUILD / name)], check=True, capture_output=True, cwd=WOOD)
        shutil.copy(live, out)


def render(pb: pathlib.Path, view: str, opacity: str, size: str, wheel: str = "", attributes: bool = False, xray: bool = False, aspect: float = 0.0) -> Image.Image:
    """The scene drawn headlessly by laika-viewer-cli render-pb (orthographic shaded edges, white background, no grid),
    cropped to what it draws by tools/crop_viewer_shot.py."""
    import render_joint_library_gifs as library

    width, height = (int(side) for side in size.split("x"))
    env = dict(
        os.environ,
        VIEWER_W=str(max(width, 2400)),
        VIEWER_H=str(max(height, 1600)),
        VIEWER_OPACITY="0.45" if xray else opacity,
        VIEWER_VIEW=view,
        VIEWER_NO_GRID="1",
        VIEWER_OUTLINES="1",
    )
    with tempfile.TemporaryDirectory() as tmp:
        shot = pathlib.Path(tmp) / "shot.png"
        subprocess.run(
            [str(library.RENDERER), "render-pb", str(shot), str(pb)],
            env=env,
            check=True,
            capture_output=True,
        )
        crop_viewer_shot.crop(shot, bool(wheel), aspect)
        return Image.open(shot).convert("RGB")


def option(arguments: list, flag: str, default: str) -> str:
    """The value after flag, taken out of arguments, or default when flag is not given."""
    if flag not in arguments:
        return default

    index = arguments.index(flag)
    value = arguments[index + 1]
    del arguments[index : index + 2]
    return value


def main(arguments: list) -> None:
    """Saves docs/images/templates/<example>_<group>.webp for every group named after the example, `all` the whole scene, `mesh+planes` both groups in one picture named after the last.

    `--view top` for a plan, `--view eye` for a level perspective from the front at half the scene's height, `--turn 106` to turn the scene about z first, `--opacity 0.95` to see inside, `--size 4800x3200` to render larger, `--wheel 12,0.35,0.45` to turn the
    viewer's wheel that many ticks over that share of the canvas for a close-up, `--name detail` to save the one group as <example>_detail.webp, `--attributes` to draw the
    elements' attributes too, their features (joint lines and volumes) and interactions (the face contacts the viewer fills red), which are left out by default;
    `--scene file.pb` to keep the example's scene in that file and shoot from it when it is there, so a slow example runs once for all its pictures;
    `--aspect 1.5` to crop to that width / height, so the pictures of a card grid fill equal tiles.
    Every picture is a screenshot of the viewer, cropped, never scaled or recoloured, and saved lossless; a close-up is the viewer's own zoom.
    """
    view = option(arguments, "--view", "iso")
    opacity = option(arguments, "--opacity", "1.0")
    size = option(arguments, "--size", "2400x1600")
    wheel = option(arguments, "--wheel", "")
    turn = option(arguments, "--turn", "0")
    rename = option(arguments, "--name", "")
    kept = option(arguments, "--scene", "")
    aspect = float(option(arguments, "--aspect", "0"))
    shown = "attributes" if "--attributes" in arguments else "elements"
    arguments = [argument for argument in arguments if argument != "--attributes"]
    name, groups = arguments[0], arguments[1:] or ["all"]
    IMAGES.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp:
        scene = pathlib.Path(kept) if kept else pathlib.Path(tmp) / "scene.pb"
        run_example(name, scene)

        for index, group in enumerate(groups):
            branch = pathlib.Path(tmp) / f"branch_{index}.pb"
            subprocess.run(
                [str(PYTHON), "-c", BRANCH, str(scene), group, str(branch), shown, turn],
                check=True,
                capture_output=True,
            )
            picture = IMAGES / f"{name}_{rename or group.split('+')[-1]}.webp"
            image = render(branch, view, opacity, size, wheel, shown == "attributes", False, aspect)

            image.save(picture, lossless=True)
            print(picture.relative_to(WOOD))


if __name__ == "__main__":
    main(sys.argv[1:])
