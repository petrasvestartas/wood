import fcntl
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops

WOOD = pathlib.Path(__file__).resolve().parents[1]
RENDERER = (
    WOOD.parent
    / "session/session_viewer/target/x86_64-unknown-linux-gnu/release/examples/selftest"
)
PYTHON = WOOD.parent / "session/session_py/.venv/bin/python"
IMAGES = WOOD / "docs" / "images" / "templates"
BUILD = pathlib.Path(os.environ.get("WOOD_BUILD", WOOD / "build"))
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
        fcntl.flock(lock, fcntl.LOCK_EX)
        subprocess.run([str(BUILD / name)], check=True, capture_output=True, cwd=WOOD)
        shutil.copy(live, out)


def render(pb: pathlib.Path, view: str) -> Image.Image:
    """The scene at opacity 1 in the Arctic view, cropped to what it draws."""
    env = dict(
        os.environ,
        VIEWER_SHEETS="0",
        VIEWER_OPACITY="1",
        VIEWER_W="2400",
        VIEWER_H="1600",
        VIEWER_THICKNESS="2",
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
    margin = 40
    box = (
        max(0, box[0] - margin),
        max(0, box[1] - margin),
        min(image.width, box[2] + margin),
        min(image.height, box[3] + margin),
    )

    return image.crop(box)


def main(arguments: list) -> None:
    """Saves docs/images/templates/<example>_<group>.webp for every group named after the example, `all` the whole scene, `mesh+planes` both groups in one picture named after the last; `--view top` for a plan."""
    view = "iso"

    if "--view" in arguments:
        index = arguments.index("--view")
        view = arguments[index + 1]
        del arguments[index : index + 2]

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
            picture = IMAGES / f"{name}_{group.split('+')[-1]}.webp"
            render(branch, view).save(picture, quality=90)
            print(picture.relative_to(WOOD))


if __name__ == "__main__":
    main(sys.argv[1:])
