"""One picture per joint of the beam joint library, docs/images/joint_library/beam_<design>.png: runs element_joint_beam_library,
takes each design's group (the joint on a square corner and on an inclined one, as its record allows) and draws it with the
viewer crate's native renderer in the Arctic look, ambient occlusion and outlines, cropped to 3:2 with a margin; then rewrites
the table of docs/elements/element_joint_beam_compendium.md between its joint-table markers from the records, and the list of
the book's joints still to do (tools/joint_library_todo.py).

    python3 tools/render_joint_library.py [design ...]
"""

import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops

import joint_library_todo

WOOD = pathlib.Path(__file__).resolve().parents[1]
PYTHON = WOOD.parent / "session/session_py/.venv/bin/python"
RENDERER = WOOD.parent / "session/session_viewer/target/x86_64-unknown-linux-gnu/release/examples/selftest"
PICTURES = WOOD / "docs/images/joint_library"
PAGE = WOOD / "docs/elements/element_joint_beam_compendium.md"

# Each design's group written alone, its interaction features off, by session_py.
GROUPS = r"""
import json, sys
from session_py.session import Session
scene_path, out, names = sys.argv[1], sys.argv[2], sys.argv[3:]
written = []
for name in names:
    branch = Session.pb_load(scene_path).get_branch(name)
    for edge in list(branch.interactions):
        branch.interactions[edge] = []
    for element in branch.objects.elements:
        element.set_features([])
    path = "%s/%s.pb" % (out, name)
    branch.pb_dump(path)
    written.append([name, path])
print(json.dumps(written))
"""


def render(pb: str) -> Image.Image:
    """The scene drawn isometric at 3600 x 2400, the outline twice its width for the halving, cropped to 3:2 and halved."""
    env = dict(
        os.environ, VIEWER_SHEETS="0", VIEWER_OPACITY="1.0", VIEWER_W="3600", VIEWER_H="2400", VIEWER_DPR="2",
        VIEWER_THICKNESS="1", VIEWER_VIEW="iso", VIEWER_PLANE_SIZE="0", VIEWER_NO_GRID="1", VIEWER_AO="1", VIEWER_OUTLINES="1",
    )
    with tempfile.TemporaryDirectory() as tmp:
        ppm = pathlib.Path(tmp) / "frame.ppm"
        subprocess.run([str(RENDERER), str(ppm), pb], env=env, check=True, capture_output=True)
        image = Image.open(ppm).convert("RGB")
    clear = image.getpixel((2, 2))
    box = ImageChops.difference(image, Image.new("RGB", image.size, clear)).getbbox()
    image = image.crop((box[0] - 60, box[1] - 60, box[2] + 60, box[3] + 60))
    width = max(image.width, int(image.height * 1.5))
    height = max(image.height, int(width / 1.5))
    framed = Image.new("RGB", (width, height), clear)
    framed.paste(image, ((width - image.width) // 2, (height - image.height) // 2))
    return framed.resize((width // 2, height // 2), Image.LANCZOS)


def write_table() -> None:
    """The page's table between its joint-table markers: a row per record of data/joints/*.yaml, picture, names and page."""
    rows = ["| joint | compendium |", "|---|---|"]
    for chapter in sorted((WOOD / "data/joints").glob("*.yaml")):
        for match in re.finditer(r'^([a-z_0-9]+):\n  title: "(.*?)"\n  german: "(.*?)"\n  page: "(.*?)"', chapter.read_text(), re.M):
            design, title, german, page = match.groups()
            rows.append(f"| ![](joint_library/beam_{design}.png) | **{title}**<br>{german}, p. {page}<br>`{design}` |")
    text = PAGE.read_text()
    start = text.index("<!-- joint-table -->") + len("<!-- joint-table -->")
    PAGE.write_text(text[:start] + "\n" + "\n".join(rows) + "\n" + text[text.index("<!-- /joint-table -->"):])


def main(arguments: list) -> None:
    """Saves a picture per design, every design of the library when none is named."""
    listing = subprocess.run([str(WOOD / "build/element_joint_beam_library")], cwd=WOOD, check=True, capture_output=True, text=True)
    designs = arguments or [line.split()[1] for line in listing.stdout.splitlines() if line.startswith("DESIGN ")]
    PICTURES.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        scene = pathlib.Path(tmp) / "scene.pb"
        scene.write_bytes((WOOD / "data/output/pb/live.pb").read_bytes())
        written = subprocess.run([str(PYTHON), "-c", GROUPS, str(scene), tmp, *designs], check=True, capture_output=True, text=True).stdout
        for name, path in json.loads(written.strip().splitlines()[-1]):
            picture = PICTURES / f"beam_{name}.png"
            render(path).save(picture)
            print(picture.relative_to(WOOD))
    write_table()
    joint_library_todo.main()


if __name__ == "__main__":
    main(sys.argv[1:])
