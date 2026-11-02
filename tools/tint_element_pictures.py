"""Turns the white faces of the element pictures light grey so they read against the white page: every near-white pixel the
background cannot reach from the picture's border, flood-filled with a small tolerance, is scaled towards grey; shading, outlines and the
background stay as they are. Run after tools/screenshot_element_docs.sh, on the pictures named or on all of them.

    python tools/tint_element_pictures.py [docs/images/elements/element_plate.png ...]
"""

import pathlib
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

WOOD = pathlib.Path(__file__).resolve().parents[1]
ELEMENTS = WOOD / "docs" / "images" / "elements"
WHITE = 240  # a pixel at or above this in every channel counts as white
GREY = 0.86  # what white becomes: 255 * 0.86 = 219


def tint(path: pathlib.Path) -> None:
    """Rewrites the picture with its enclosed white turned grey."""
    image = np.asarray(Image.open(path).convert("RGB")).astype(np.float32)
    white = (image >= WHITE).all(axis=2)

    # the background: the white connected to the border
    labels, _ = ndimage.label(white)
    border = np.unique(
        np.concatenate([labels[0, :], labels[-1, :], labels[:, 0], labels[:, -1]])
    )
    background = np.isin(labels, border[border > 0])

    # the faces: white the background does not reach, scaled towards grey with their shading kept
    faces = white & ~background
    image[faces] *= GREY
    Image.fromarray(image.round().astype(np.uint8)).save(path)
    print(path.name, "tinted", int(faces.sum()), "pixels")


def main(arguments: list) -> None:
    """Tints the pictures named, else every png under docs/images/elements."""
    paths = [pathlib.Path(argument) for argument in arguments] or sorted(
        ELEMENTS.glob("*.png")
    )
    for path in paths:
        tint(path)


if __name__ == "__main__":
    main(sys.argv[1:])
