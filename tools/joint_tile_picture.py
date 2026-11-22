"""A joint library tile as a picture: the unit box drawn isometric, every cut's two outlines and the lines joining them, the male
member's (a) blue, the female's (b) red, other members' green, loose parts grey, labelled fixed or with the divisions a
member's unit repeats by along the joint (fingers, tenons, keys). Reads data/output/pb/joint_library_tiles.json
(element_joint_beam_library_tiles) and writes docs/images/joint_library/tile_<design>.png.

    python3 tools/joint_tile_picture.py [design ...]
"""

import json
import math
import pathlib
import sys

from PIL import Image, ImageDraw

WOOD = pathlib.Path(__file__).resolve().parents[1]
PICTURES = WOOD / "docs/images/joint_library"
COLOURS = [(33, 110, 220), (210, 50, 50), (40, 150, 70), (120, 60, 170), (200, 120, 0)]
PART = (90, 90, 90)
SIZE = (960, 640)  # The size of the library's pictures and GIFs, so a row of the page reads as one.
BACKGROUND = (247, 247, 247)  # The renders' background.


def project(point: list, scale: list) -> tuple:
    """A unit-box point drawn isometric in the box's true proportions (scale, its sides over the longest): x right-down, y
    left-down, z up, the box fitting the picture's height."""
    x, y, z = (c * k for c, k in zip(point, scale))
    u = (x - y) * math.cos(math.radians(30))
    v = (x + y) * math.sin(math.radians(30)) - z
    return (SIZE[0] / 2 + u * SIZE[1] * 0.42, SIZE[1] / 2 + v * SIZE[1] * 0.42)


def draw(design: str, tile: dict) -> Image.Image:
    """The tile picture: the box's edges thin grey, then each pair's outlines and the lines joining their points."""
    image = Image.new("RGB", SIZE, BACKGROUND)
    pen = ImageDraw.Draw(image)
    longest = max(tile["size"])
    scale = [side / longest for side in tile["size"]]
    corners = [[x, y, z] for x in (-0.5, 0.5) for y in (-0.5, 0.5) for z in (-0.5, 0.5)]
    for a in corners:
        for b in corners:
            if sum(abs(p - q) for p, q in zip(a, b)) == 1.0:
                pen.line([project(a, scale), project(b, scale)], fill=(200, 200, 200), width=1)
    groups = [(COLOURS[k % len(COLOURS)], pairs) for k, pairs in enumerate(tile["members"])] + [(PART, tile["parts"])]
    for colour, pairs in groups:
        for pair in pairs:
            for outline in pair:
                pen.line([project(p, scale) for p in outline], fill=colour, width=2)
            for p, q in zip(pair[0], pair[1]):
                pen.line([project(p, scale), project(q, scale)], fill=colour, width=1)
    # what the tile is: fixed, or a unit repeating by divisions along an axis (as the plate joints' fingers)
    rows = [f"member {'abcdefgh'[k]}: {n} x along {'xyz'[axis]}" for k, (n, axis) in enumerate(zip(tile.get("divisions", []), tile.get("axes", []))) if n > 1]
    pen.text((16, SIZE[1] - 26 - 16 * len(rows)), "fixed tile" if not rows else "parametric, repeats by divisions", fill=(0, 0, 0))
    for k, row in enumerate(rows):
        pen.text((16, SIZE[1] - 26 - 16 * (len(rows) - 1 - k)), row, fill=(0, 0, 0))
    return image


def main(arguments: list) -> None:
    """Saves a tile picture per design, every design of the JSON when none is named."""
    tiles = json.loads((WOOD / "data/output/pb/joint_library_tiles.json").read_text())
    for design in arguments or list(tiles):
        draw(design, tiles[design]).save(PICTURES / f"tile_{design}.png")
        print(f"docs/images/joint_library/tile_{design}.png")


if __name__ == "__main__":
    main(sys.argv[1:])
