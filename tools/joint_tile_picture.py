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


def project(point: list, size: int, scale: list) -> tuple:
    """A unit-box point drawn isometric in the box's true proportions (scale, its sides over the longest): x right-down, y
    left-down, z up, the box fitting size pixels."""
    x, y, z = (c * k for c, k in zip(point, scale))
    u = (x - y) * math.cos(math.radians(30))
    v = (x + y) * math.sin(math.radians(30)) - z
    return (size / 2 + u * size * 0.45, size / 2 + v * size * 0.45)


def draw(design: str, tile: dict, size: int = 600) -> Image.Image:
    """The tile picture: the box's edges thin grey, then each pair's outlines and the lines joining their points."""
    image = Image.new("RGB", (size, size), (255, 255, 255))
    longest = max(tile["size"])
    scale = [side / longest for side in tile["size"]]
    pen = ImageDraw.Draw(image)
    corners = [[x, y, z] for x in (-0.5, 0.5) for y in (-0.5, 0.5) for z in (-0.5, 0.5)]
    for a in corners:
        for b in corners:
            if sum(abs(p - q) for p, q in zip(a, b)) == 1.0:
                pen.line([project(a, size, scale), project(b, size, scale)], fill=(200, 200, 200), width=1)
    groups = [(COLOURS[k % len(COLOURS)], pairs) for k, pairs in enumerate(tile["members"])] + [(PART, tile["parts"])]
    for colour, pairs in groups:
        for pair in pairs:
            for outline in pair:
                pen.line([project(p, size, scale) for p in outline], fill=colour, width=2)
            for p, q in zip(pair[0], pair[1]):
                pen.line([project(p, size, scale), project(q, size, scale)], fill=colour, width=1)
    # what the tile is: fixed, or a unit repeating by divisions along an axis (as the plate joints' fingers)
    rows = [f"member {'abcdefgh'[k]}: {n} x along {'xyz'[axis]}" for k, (n, axis) in enumerate(zip(tile.get("divisions", []), tile.get("axes", []))) if n > 1]
    pen.text((12, size - 22 - 14 * len(rows)), "fixed tile" if not rows else "parametric, repeats by divisions", fill=(0, 0, 0))
    for k, row in enumerate(rows):
        pen.text((12, size - 22 - 14 * (len(rows) - 1 - k)), row, fill=(0, 0, 0))
    return image


def main(arguments: list) -> None:
    """Saves a tile picture per design, every design of the JSON when none is named."""
    tiles = json.loads((WOOD / "data/output/pb/joint_library_tiles.json").read_text())
    for design in arguments or list(tiles):
        draw(design, tiles[design]).save(PICTURES / f"tile_{design}.png")
        print(f"docs/images/joint_library/tile_{design}.png")


if __name__ == "__main__":
    main(sys.argv[1:])
