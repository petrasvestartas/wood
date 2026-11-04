"""Compose the four element contact sheets shown on docs/elements.md.

Reads every docs/images/elements/*.png, the whole picture, and lays the pictures out
five per row with a caption, one sheet per kind: elements, plate joints, beam joints, connectors and pins,
each sheet in titled groups: plates, beams, columns; side to side, top to side; and so on.
Writes docs/images/diagrams/wood_elements_sheet_<n>_<name>.webp (4400 px wide, quality 90).

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
    "element_plate",
    "element_plate_holes",
    "element_plate_session",
    "element_plate_assign_joints",
    "element_plate_curved",
    "element_beam",
    "element_beam_curved",
    "element_beam_variable",
    "element_beam_variable_cut",
    "element_column",
    "element_column_session",
    "element_block",
    "element_support",
    "element_cut_plane",
    "element_profile",
    "element_pin",
]
BEAM_CUT_FAMILIES = [
    "cross",
    "side_side",
    "top_side",
    "jupiter_scarf",
    "butt",
    "end_to_end",
    "column_head",
    "column_head_rotated",
]
SHEET_4 = [
    "element_joint",
    "element_joint_cutter",
    "element_joint_drill",
    "element_joint_beam_butt_plates",
    "element_joint_beam_butterfly_key",
    "element_joint_beam_centred_pins",
    "element_joint_beam_headed_pins",
    "element_joint_beam_rectangle_plate",
    "element_joint_beam_wedge",
    "element_joint_beam_connectors",
    "element_joint_beam_pins",
    "element_joint_beam_keyed_box",
]

# all example names
names = []
for path in sorted(glob.glob(os.path.join(ELEMENTS, "*.png"))):
    names.append(os.path.splitext(os.path.basename(path))[0])

# plate joints: the overview first, then every plate joint
sheet_2 = ["element_joint_plate"]
for name in names:
    if name.startswith("element_joint_plate_"):
        sheet_2.append(name)

# beam joints: the overview first, then the cuts between beams family by family
sheet_3 = ["element_joint_beam"]
for family in BEAM_CUT_FAMILIES:
    for name in names:
        if name.startswith("element_joint_beam_"):
            rest = name[len("element_joint_beam_") :]
            if rest == family or (
                family in ["cross", "side_side", "top_side"]
                and rest.startswith(family + "_")
            ):
                sheet_3.append(name)

sheets = [
    ("1_elements", SHEET_1),
    ("2_plate_joints", sheet_2),
    ("3_beam_joints", sheet_3),
    ("4_connectors", SHEET_4),
]

# each sheet's groups: a title, the part of the name that picks its pictures, the words the captions then leave out
GROUPS = {
    "1_elements": [
        ("Plates", r"^element_plate", "plate "),
        ("Beams", r"^element_beam", "beam "),
        ("Columns", r"^element_column", "column "),
        ("Blocks, Supports, Profiles and Pins", r"^element_(block|support|profile|pin)", ""),
    ],
    "2_plate_joints": [
        ("Side to Side, In Plane", r"side_side_inplane", "side side inplane "),
        ("Side to Side, Out of Plane", r"side_side_outplane", "side side outplane "),
        ("Side to Side, Rotated", r"side_side_rotated", "side side rotated "),
        ("Top to Side", r"top_side", "top side "),
        ("Top to Top", r"top_top", "top top "),
        ("Cross", r"cross", "cross "),
        ("Border, Nodes and Parameters", r"", ""),
    ],
    "3_beam_joints": [
        ("Cross", r"beam_cross", "cross "),
        ("Top to Side", r"beam_top_side", "top side "),
        ("End to End", r"beam_(side_side|jupiter)", ""),
        ("Butt", r"beam_butt$", ""),
        ("Nodes", r"beam_column_head", ""),
    ],
    "4_connectors": [
        ("Pins", r"_pins$", ""),
        ("Connectors", r"_beam_", ""),
        ("Cutters and Drills", r"", "joint "),
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

# every example's picture is on exactly one sheet
names = [name for name in names if os.path.exists("examples/elements/" + name + ".cpp")]
placed = []
for sheet_name, sheet in sheets:
    placed.extend(sheet)
for name in names:
    assert placed.count(name) == 1, (
        name + " is on " + str(placed.count(name)) + " sheets"
    )
assert len(placed) == len(names), "a sheet lists a picture that does not exist"
for sheet_name, sheet in sheets:
    print("wood_elements_sheet_" + sheet_name + ".webp", len(sheet))
