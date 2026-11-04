import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

from PIL import Image

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import crop_viewer_shot  # noqa: E402

WOOD = pathlib.Path(__file__).resolve().parents[1]
WINDOWS = os.name == "nt"
RENDERER = (
    WOOD.parent / "session/session_viewer/target/x86_64-pc-windows-msvc/release/examples/selftest.exe"
    if WINDOWS
    else WOOD.parent
    / "session/session_viewer/target/x86_64-unknown-linux-gnu/release/examples/selftest"
)
if not RENDERER.exists():
    RENDERER = WOOD.parent / "session/session_viewer/target/course-runtime" / RENDERER.name
PYTHON = WOOD.parent / (
    "session/session_py/.venv/Scripts/python.exe"
    if WINDOWS
    else "session/session_py/.venv/bin/python"
)
IMAGES = WOOD / "docs" / "images" / "templates"
BUILD = pathlib.Path(
    os.environ.get("WOOD_BUILD", WOOD / "build" / "Release" if WINDOWS else WOOD / "build")
)
LOCK = pathlib.Path(tempfile.gettempdir()) / "wood_live_pb.lock"

BRANCH = r"""
import sys
from session_py.session import Session
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
branch.name = ""  # no document title over the scene
branch.pb_dump(sys.argv[3])
"""  # the scene, or groups of its tree joined by + with everything under them, written to their own file; the elements alone, their cut solids without
# their attributes, unless asked for: no features (the joint lines and volumes) and no interactions (the face contacts the viewer fills red)


def run_example(name: str, out: pathlib.Path) -> None:
    """Runs the example and copies the live.pb it writes, locked so two runs never share the file."""
    live = WOOD / "data" / "output" / "pb" / "live.pb"

    with open(LOCK, "w") as lock:
        if WINDOWS:
            import msvcrt

            msvcrt.locking(lock.fileno(), msvcrt.LK_LOCK, 1)
        else:
            import fcntl

            fcntl.flock(lock, fcntl.LOCK_EX)

        subprocess.run([str(BUILD / name)], check=True, capture_output=True, cwd=WOOD)
        shutil.copy(live, out)


def render(pb: pathlib.Path, view: str, opacity: str, size: str, wheel: str = "", attributes: bool = False, xray: bool = False, aspect: float = 0.0) -> Image.Image:
    """The scene as a screenshot of the viewer: the Arctic look with its soft shadows, no grid, the scene fitted, 1600 wide per
    scale step and at least 4800, cropped to what it draws by tools/crop_viewer_shot.py, which only crops."""
    width = int(size.split("x")[0])
    env = dict(
        os.environ,
        SHOT_PB=str(pb),
        SHOT_SCALE=str(max(3, min(5, width // 1600))),
        SHOT_QUERY=f"opacity={opacity}&nomarkers=1&thickness=0.7",
    )
    camera = {"top": "View Top", "front": "View Front", "right": "View Right", "bottom": "View Bottom"}.get(view, "View Isometric")
    shown = [] if attributes else ["Element Interactions Off", "Element Attributes Off"]
    commands = ["Layers Off", *shown, "Arctic On", "Outline Off", camera, "View Orthographic", *(["View Xray"] if xray else [])]
    if wheel:
        env["SHOT_WHEEL"] = wheel
    if int(os.environ.get("VIEWER_PLANE_SIZE", "0")):
        commands.append(f"View Plane Size {os.environ['VIEWER_PLANE_SIZE']}")
    # one selected object keeps its name bubble: a scene of one object is fitted whole, nothing selected
    commands += ["Fit"] if os.environ.get("SHOT_ONE_OBJECT") else ["Select All", "Fit", "Escape"]

    # a shot of a scene that had not loaded is taken again
    with tempfile.TemporaryDirectory() as tmp:
        shot = pathlib.Path(tmp) / "shot.png"
        for attempt in range(3):
            subprocess.run(
                ["node", "--experimental-websocket", str(WOOD / "tools" / "screenshot_viewer.mjs"), str(shot), *commands],
                env=env,
                check=True,
                capture_output=True,
            )
            try:
                crop_viewer_shot.crop(shot, bool(wheel), aspect)
                return Image.open(shot).convert("RGB")
            except SystemExit:
                if attempt == 2:
                    raise


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

    `--view top` for a plan, `--opacity 0.95` to see inside, `--size 4800x3200` to render larger, `--wheel 12,0.35,0.45` to turn the
    viewer's wheel that many ticks over that share of the canvas for a close-up, `--name detail` to save the one group as <example>_detail.webp, `--attributes` to draw the
    elements' attributes too, their features (joint lines and volumes) and interactions (the face contacts the viewer fills red), which are left out by default.
    Every picture is a screenshot of the viewer, cropped, never scaled or recoloured, and saved lossless; a close-up is the viewer's own zoom.
    """
    view = option(arguments, "--view", "iso")
    opacity = option(arguments, "--opacity", "1.0")
    size = option(arguments, "--size", "2400x1600")
    wheel = option(arguments, "--wheel", "")
    rename = option(arguments, "--name", "")
    shown = "attributes" if "--attributes" in arguments else "elements"
    arguments = [argument for argument in arguments if argument != "--attributes"]
    name, groups = arguments[0], arguments[1:] or ["all"]
    IMAGES.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp:
        scene = pathlib.Path(tmp) / "scene.pb"
        run_example(name, scene)

        for index, group in enumerate(groups):
            branch = pathlib.Path(tmp) / f"branch_{index}.pb"
            subprocess.run(
                [str(PYTHON), "-c", BRANCH, str(scene), group, str(branch), shown],
                check=True,
                capture_output=True,
            )
            picture = IMAGES / f"{name}_{rename or group.split('+')[-1]}.webp"
            image = render(branch, view, opacity, size, wheel, shown == "attributes")

            image.save(picture, lossless=True)
            print(picture.relative_to(WOOD))


if __name__ == "__main__":
    main(sys.argv[1:])
