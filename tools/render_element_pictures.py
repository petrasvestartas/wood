"""The element pages' pictures from the session viewer's own renderer, no browser: every example under examples/elements built, run and
drawn by render_template_docs.render, the elements alone at full opacity with the Arctic light, fitted and padded to 1.6 : 1 on white,
into docs/images/elements/<example>.png. An overview page's picture that has no example of its name (element_joint_beam.png,
element_joint_plate.png, ...) is the picture of the case it is a copy of today, so it is written again from that case.

    python tools/render_element_pictures.py                      every example
    python tools/render_element_pictures.py element_plate ...    the ones named
"""

import hashlib
import pathlib
import sys
import tempfile

from PIL import Image

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import render_template_docs as docs  # noqa: E402

ELEMENTS = docs.WOOD / "docs" / "images" / "elements"
EXAMPLES = docs.WOOD / "examples" / "elements"
RATIO = 1.6

TURN = r"""
import sys
from session_py.session import Session
from session_py.xform import Xform
scene = Session.pb_load(sys.argv[1])
for element in scene.objects.elements:
    element.place(Xform.rotation_z(180.0, degrees=True))
scene.pb_dump(sys.argv[1])
"""  # the scene turned half a circle about z, in place


def padded(image: Image.Image) -> Image.Image:
    """The picture centred on a white canvas of the pages' ratio."""
    width, height = image.size
    canvas = (max(width, int(height * RATIO)), max(height, int(width / RATIO)))
    out = Image.new("RGB", canvas, "white")
    out.paste(image, ((canvas[0] - width) // 2, (canvas[1] - height) // 2))
    return out


def main(arguments: list) -> None:
    """Renders the examples named, else every one, and the overview copies that follow from them."""
    names = arguments or sorted(path.stem for path in EXAMPLES.glob("element_*.cpp"))

    # the overview pictures: every png without an example, keyed by the example whose picture it equals
    digests = {}
    for path in ELEMENTS.glob("*.png"):
        digests.setdefault(hashlib.md5(path.read_bytes()).hexdigest(), []).append(
            path.stem
        )
    copies = {}
    for twins in digests.values():
        sources = [stem for stem in twins if (EXAMPLES / f"{stem}.cpp").exists()]
        for stem in twins:
            if stem not in sources and sources:
                copies.setdefault(sources[0], []).append(stem)

    for name in names:
        with tempfile.TemporaryDirectory() as tmp:
            scene = pathlib.Path(tmp) / "scene.pb"
            docs.run_example(name, scene)
            branch = pathlib.Path(tmp) / "branch.pb"
            docs.subprocess.run(
                [
                    str(docs.PYTHON),
                    "-c",
                    docs.BRANCH,
                    str(scene),
                    "all",
                    str(branch),
                    "elements",
                ],
                check=True,
                capture_output=True,
            )
            # a column's head and cuts face the inside of the floor, away from the isometric corner: the scene turned half a circle
            if name.startswith("element_column"):
                docs.subprocess.run(
                    [str(docs.PYTHON), "-c", TURN, str(branch)],
                    check=True,
                    capture_output=True,
                )
            image, factor = docs.render(branch, "iso", "1.0", "2000x1250")

        if factor > 1:
            image = image.resize(
                (image.width // factor, image.height // factor), Image.LANCZOS
            )
        picture = padded(image)

        for stem in [name] + copies.get(name, []):
            picture.save(ELEMENTS / f"{stem}.png")
            print(stem)


if __name__ == "__main__":
    main(sys.argv[1:])
