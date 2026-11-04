"""A viewer screenshot cropped for the docs: the command bar under the canvas and the empty canvas around the drawing are cut off, a margin
kept. Cropping only: every pixel left is the pixel the viewer drew.

    python tools/crop_viewer_shot.py docs/images/elements/element_plate.png ...

SHOT_ASPECT=1.5 widens the crop to that width / height.
"""

import os
import pathlib
import sys

import numpy as np
from PIL import Image

MARGIN = 60  # px kept around the drawing
DRAWN = 3  # a pixel this far from the backdrop is part of the drawing, the soft shadows included


def crop(path: pathlib.Path, close_up: bool = False, aspect: float = 0.0) -> None:
    """Rewrites the screenshot cropped to its drawing; a close-up fills the canvas, only its command bar is cut off; an aspect widens
    the crop to width / height, so the pictures of a sheet fill equal tiles with nothing painted around them."""
    image = Image.open(path).convert("RGB")
    pixels = np.asarray(image).astype(np.int16)
    backdrop = pixels[2, 2]
    off = np.abs(pixels - backdrop).max(axis=2) > DRAWN

    # the canvas ends at the command bar's divider, the lowest full-width line
    full_rows = np.flatnonzero(off[image.height // 2 :].mean(axis=1) > 0.9)
    bottom = image.height // 2 + int(full_rows[0]) if full_rows.size else image.height

    rows = np.flatnonzero(off[:bottom].any(axis=1))
    columns = np.flatnonzero(off[:bottom].any(axis=0))
    if rows.size == 0:
        raise SystemExit(f"{path.name}: nothing drawn")

    # a fitted scene leaves canvas around it; a drawing touching left and right is the empty grid, shot before the scene had loaded
    if close_up:
        image.crop((0, 0, image.width, bottom)).save(path)
        print(path.name, "close-up")
        return

    if columns[0] <= 2 and columns[-1] >= image.width - 3:
        raise SystemExit(f"{path.name}: the scene had not loaded")

    box = (
        max(0, int(columns[0]) - MARGIN),
        max(0, int(rows[0]) - MARGIN),
        min(image.width, int(columns[-1]) + 1 + MARGIN),
        min(bottom, int(rows[-1]) + 1 + MARGIN),
    )
    if aspect:
        width, height = box[2] - box[0], box[3] - box[1]
        width, height = (
            (round(height * aspect), height)
            if width / height < aspect
            else (width, round(width / aspect))
        )
        left = max(0, min((box[0] + box[2] - width) // 2, image.width - width))
        top = max(0, min((box[1] + box[3] - height) // 2, bottom - height))
        box = (left, top, left + width, top + height)
    image.crop(box).save(path)
    print(path.name, box)


if __name__ == "__main__":
    for argument in sys.argv[1:]:
        crop(pathlib.Path(argument), False, float(os.environ.get("SHOT_ASPECT", 0)))
