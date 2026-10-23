import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops

WOOD = pathlib.Path(__file__).resolve().parents[1]
WINDOWS = os.name == "nt"
RENDERER = (
    WOOD.parent / "session/session_viewer/target/x86_64-pc-windows-msvc/release/examples/selftest.exe"
    if WINDOWS
    else WOOD.parent
    / "session/session_viewer/target/x86_64-unknown-linux-gnu/release/examples/selftest"
)
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
branch.pb_dump(sys.argv[3])
"""  # the scene, or groups of its tree joined by + with everything under them, written to their own file


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


LARGEST = 8000  # px, the widest frame the viewer renders; supersampling stays under it


def render(pb: pathlib.Path, view: str, opacity: str, size: str) -> tuple:
    """The scene at the given opacity in the Arctic view, rendered at up to twice size as a high-density screen draws it, so the Arctic
    outline and the pen keep the viewer's own widths when scaled down smooth, cropped to what it draws; with the factor it was rendered
    larger by."""
    width, height = (int(side) for side in size.split("x"))
    factor = max(1, min(2, LARGEST // width))
    env = dict(
        os.environ,
        VIEWER_SHEETS="0",
        VIEWER_OPACITY=opacity,
        VIEWER_W=str(width * factor),
        VIEWER_H=str(height * factor),
        VIEWER_DPR=str(factor),
        VIEWER_THICKNESS="1",
        VIEWER_VIEW=view,
        VIEWER_PLANE_SIZE=os.environ.get("VIEWER_PLANE_SIZE", "0"),
        VIEWER_NO_GRID="1",
        VIEWER_AO="1",
        VIEWER_OUTLINES="1",
    )
    env.setdefault("VIEWER_ADAPTER", "nvidia")

    with tempfile.TemporaryDirectory() as tmp:
        ppm = pathlib.Path(tmp) / "frame.ppm"
        subprocess.run(
            [str(RENDERER), str(ppm), str(pb)], env=env, check=True, capture_output=True
        )
        image = Image.open(ppm).convert("RGB")

    background = Image.new("RGB", image.size, image.getpixel((2, 2)))
    mask = (
        ImageChops.difference(image, background)
        .convert("L")
        .point(lambda v: 255 if v > 12 else 0)
    )
    box = mask.getbbox()
    margin = 40 * factor
    box = (
        max(0, box[0] - margin),
        max(0, box[1] - margin),
        min(image.width, box[2] + margin),
        min(image.height, box[3] + margin),
    )

    return image.crop(box), factor


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

    `--view top` for a plan, `--opacity 0.95` to see inside, `--size 4800x3200` to render larger, `--crop 0.4,0.3,0.6,0.5` to keep that
    share of the picture (left, top, right, bottom) for a close-up, `--name detail` to save the one group as <example>_detail.webp.
    Every picture renders at twice its size, up to LARGEST wide, at two pixels per CSS pixel as the viewer on a high-density screen, and
    is scaled down, so the Arctic outline stays bold and black and the edges thin and smooth.
    """
    view = option(arguments, "--view", "iso")
    opacity = option(arguments, "--opacity", "1")
    size = option(arguments, "--size", "2400x1600")
    crop = option(arguments, "--crop", "")
    rename = option(arguments, "--name", "")
    name, groups = arguments[0], arguments[1:] or ["all"]
    IMAGES.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp:
        scene = pathlib.Path(tmp) / "scene.pb"
        run_example(name, scene)

        for group in groups:
            branch = pathlib.Path(tmp) / f"{group}.pb"
            subprocess.run(
                [str(PYTHON), "-c", BRANCH, str(scene), group, str(branch)],
                check=True,
                capture_output=True,
            )
            picture = IMAGES / f"{name}_{rename or group.split('+')[-1]}.webp"
            image, factor = render(branch, view, opacity, size)

            if crop:
                left, top, right, bottom = (float(share) for share in crop.split(","))
                image = image.crop(
                    (
                        int(image.width * left),
                        int(image.height * top),
                        int(image.width * right),
                        int(image.height * bottom),
                    )
                )

            if factor > 1:
                image = image.resize((image.width // factor, image.height // factor), Image.LANCZOS)

            image.save(picture, quality=90)
            print(picture.relative_to(WOOD))


if __name__ == "__main__":
    main(sys.argv[1:])
