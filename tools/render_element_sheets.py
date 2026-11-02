"""Compose the four element contact sheets shown on docs/elements.md.

Reads every docs/images/elements/*.png, crops each picture to its drawn content and lays the pictures out
five per row with a caption, one sheet per group: elements, plate joints, beam joints, connectors and pins.
Writes docs/images/diagrams/wood_elements_sheet_<n>_<name>.webp (4400 px wide, quality 90).

Run from wood/:  python tools/render_element_sheets.py
"""

import os
import glob
from PIL import Image, ImageDraw, ImageFont

ELEMENTS = "docs/images/elements"
DIAGRAMS = "docs/images/diagrams"
WIDTH = 4400
COLUMNS = 5
GAP = 36
TILE_W = (WIDTH - (COLUMNS + 1) * GAP) // COLUMNS
TILE_H = 680
CAPTION_H = 110
PAD = 30
FRAME = (205, 205, 212)
TEXT = (30, 30, 30)
CROP_MARGIN = 40

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

for sheet_name, sheet in sheets:
    rows = (len(sheet) + COLUMNS - 1) // COLUMNS
    height = rows * TILE_H + (rows + 1) * GAP
    canvas = Image.new("RGB", (WIDTH, height), (255, 255, 255))
    draw = ImageDraw.Draw(canvas)
    for i, name in enumerate(sheet):
        # crop to the drawn content: non-white pixels, or the dark outlines inside the viewport of a viewer screenshot
        picture = Image.open(os.path.join(ELEMENTS, name + ".png")).convert("RGB")
        w, h = picture.size
        grey = picture.convert("L")
        if picture.getpixel((0, 0)) == (255, 255, 255):
            box = grey.point(lambda p: 255 if p < 243 else 0).getbbox()
        else:
            viewport = grey.crop((0, 0, int(w * 0.66), int(h * 0.95)))
            box = viewport.point(lambda p: 255 if p < 60 else 0).getbbox()
        box = (
            max(box[0] - CROP_MARGIN, 0),
            max(box[1] - CROP_MARGIN, 0),
            min(box[2] + CROP_MARGIN, w),
            min(box[3] + CROP_MARGIN, h),
        )
        picture = picture.crop(box)

        # scale to fit the tile above the caption
        fit_w = TILE_W - 2 * PAD
        fit_h = TILE_H - CAPTION_H - PAD
        scale = min(fit_w / picture.width, fit_h / picture.height)
        picture = picture.resize(
            (round(picture.width * scale), round(picture.height * scale)), Image.LANCZOS
        )

        # tile: frame, picture centred, caption under it
        x = GAP + (i % COLUMNS) * (TILE_W + GAP)
        y = GAP + (i // COLUMNS) * (TILE_H + GAP)
        draw.rectangle((x, y, x + TILE_W - 1, y + TILE_H - 1), outline=FRAME, width=3)
        canvas.paste(
            picture,
            (
                x + (TILE_W - picture.width) // 2,
                y + PAD + (fit_h - picture.height) // 2,
            ),
        )
        caption = name
        for prefix in ["element_joint_plate_", "element_joint_beam_", "element_"]:
            if caption.startswith(prefix):
                caption = caption[len(prefix) :]
                break
        caption = caption.replace("_", " ")
        draw.text(
            (x + TILE_W // 2, y + TILE_H - CAPTION_H // 2),
            caption,
            fill=TEXT,
            font=font,
            anchor="mm",
        )

    path = os.path.join(DIAGRAMS, "wood_elements_sheet_" + sheet_name + ".webp")
    canvas.save(path, "WEBP", quality=90)

# every example picture is on exactly one sheet
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
