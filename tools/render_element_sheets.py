"""Compose the element contact sheet shown on docs/elements.md.

Lays the element pictures of docs/images/elements out five per row with a caption, in titled
groups: nodes, edges and faces.
Writes docs/images/diagrams/wood_elements_sheet_1_elements.webp (4400 px wide, quality 90).

Run from wood/:  python tools/render_element_sheets.py
"""

import os
import glob
import re
from PIL import Image, ImageDraw, ImageFont

ELEMENTS = "docs/images/elements"
DIAGRAMS = "docs/images/diagrams"
WIDTH = 4400
COLUMNS = 5
GAP = 36
TILE_W = (WIDTH - (COLUMNS + 1) * GAP) // COLUMNS
CAPTION_H = 110
TITLE_H = 120
PICTURE_H = round(TILE_W / 1.5)  # the pictures are 3:2, so each fills its tile with the viewer's own backdrop
TILE_H = PICTURE_H + CAPTION_H
FRAME = (205, 205, 212)
TEXT = (30, 30, 30)

SHEET_1 = [
    "element_node_block",
    "element_node_support",
    "element_edge_straight",
    "element_edge_curved",
    "element_edge_variable",
    "element_face_planar",
    "element_face_curved",
]
sheets = [("1_elements", SHEET_1)]

# each sheet's groups: a title, the part of the name that picks its pictures, the words the captions then leave out
GROUPS = {
    "1_elements": [
        ("Node", r"^element_node", "node "),
        ("Edge", r"^element_edge", "edge "),
        ("Face", r"^element_face", "face "),
    ],
}

# only the pictures of the examples: a family page's overview picture is a copy of one of them
sheets = [
    (sheet_name, [name for name in sheet if os.path.exists("examples/elements/" + name + ".cpp")])
    for sheet_name, sheet in sheets
]

# caption font: the first sans font found
font = None
for font_name in [
    "Roboto-Regular.ttf",
    "arial.ttf",
    "DejaVuSans.ttf",
    "LiberationSans-Regular.ttf",
]:
    try:
        font = ImageFont.truetype(font_name, 40)
        break
    except OSError:
        continue
if font is None:
    font = ImageFont.load_default(40)
title_font = font.font_variant(size=64)

for sheet_name, sheet in sheets:
    # the sheet's pictures by group, every picture in the first group that takes it
    left = list(sheet)
    groups = []
    for title, pattern, cut in GROUPS[sheet_name]:
        taken = [name for name in left if re.search(pattern, name)]
        left = [name for name in left if name not in taken]
        if taken:
            groups.append((title, taken, cut))
    assert not left, left

    height = GAP
    for title, taken, cut in groups:
        height += TITLE_H + ((len(taken) + COLUMNS - 1) // COLUMNS) * (TILE_H + GAP)
    canvas = Image.new("RGB", (WIDTH, height), (255, 255, 255))
    draw = ImageDraw.Draw(canvas)

    top = GAP
    for title, taken, cut in groups:
        draw.text((GAP, top + TITLE_H // 2), title, fill=TEXT, font=title_font, anchor="lm")
        top += TITLE_H
        for i, name in enumerate(taken):
            # the picture fills the tile above the caption
            picture = Image.open(os.path.join(ELEMENTS, name + ".png")).convert("RGB")
            picture = picture.resize((TILE_W, PICTURE_H), Image.LANCZOS)
            x = GAP + (i % COLUMNS) * (TILE_W + GAP)
            y = top + (i // COLUMNS) * (TILE_H + GAP)
            canvas.paste(picture, (x, y))
            draw.rectangle((x, y, x + TILE_W - 1, y + TILE_H - 1), outline=FRAME, width=3)
            caption = name
            for prefix in ["element_joint_plate_", "element_joint_beam_", "element_"]:
                if caption.startswith(prefix):
                    caption = caption[len(prefix) :]
                    break
            caption = caption.replace("_", " ")
            if cut and caption.startswith(cut):
                caption = caption[len(cut) :]
            draw.text(
                (x + TILE_W // 2, y + TILE_H - CAPTION_H // 2),
                caption,
                fill=TEXT,
                font=font,
                anchor="mm",
            )
        top += ((len(taken) + COLUMNS - 1) // COLUMNS) * (TILE_H + GAP)

    path = os.path.join(DIAGRAMS, "wood_elements_sheet_" + sheet_name + ".webp")
    canvas.save(path, "WEBP", quality=90)

for sheet_name, sheet in sheets:
    print("wood_elements_sheet_" + sheet_name + ".webp", len(sheet))
