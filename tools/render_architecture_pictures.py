"""The pictures of the docs' architecture and examples pages, every one a viewer screenshot cropped only (tools/render_template_docs.py):
a contact, a joint or an inside shown through the x-ray view or through transparent elements, an uncut or a cut member solid.

    python tools/render_architecture_pictures.py [examples] [pipeline] [landing] [wiring] [cutting] [sheets]

examples   docs/images/examples/ex_<n>_<name>.webp, the twelve numbered examples
pipeline   diagrams/wood_architecture.webp, one model through plates, contacts, joints and cut plates
landing    landing/landing_architecture.webp, the same model cut, at the landing page's 16:9
wiring     diagrams/wood_joint_unit_box.webp, three joints by three rows: the joint, the joint on its contact, the members it cut
cutting    diagrams/wood_boolean_cutting.webp, stock, cutter and result of four cuts
sheets     diagrams/wood_templates_pipeline.webp, one shot per family, and wood_beam_joints_connectors.webp, tiled from the element pictures
"""

import pathlib
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import render_template_docs as docs  # noqa: E402

WOOD = docs.WOOD
IMAGES = WOOD / "docs" / "images"
DIAGRAMS = IMAGES / "diagrams"
ASPECT = 1.5  # width / height of a tile's picture
GAP = 36
CAPTION = 110
FRAME = (205, 205, 212)
TEXT = (30, 30, 30)

SCENE = r"""
import sys
from session_py.session import Session
scene = Session.pb_load(sys.argv[1])
branch = scene if sys.argv[2] == "all" else scene.get_branch(sys.argv[2])
kinds = sys.argv[4].split(",") if sys.argv[4] else []
for element in branch.objects.elements:
    element.set_features([feature for feature in element.features if feature.feature_type in kinds and feature.name != "axis"])
branch.name = ""
branch.pb_dump(sys.argv[3])
"""  # one group of the scene with only the feature kinds asked for: the axis contacts, thousands of dots on a shell, never drawn

# example: attributes shown, opacity, x-ray
EXAMPLES = {
    "1_session_contacts": (True, "0.4", False),
    "2_contacts": (True, "1", True),
    "3_features": (True, "0.4", False),
    "4_traversal": (True, "0.4", False),
    "5_settings": (True, "0.4", False),
    "6_custom_joint": (True, "0.4", False),
    "7_assignment": (True, "0.4", False),
    "8_beams": (True, "0.4", False),
    "9_cross_joints": (True, "0.4", False),
    "10_profile_cuts": (False, "1", False),
    "11_drill_solids": (False, "1", False),
    "12_solid_features": (False, "1", False),
}
# caption, id of tools/joint_library_figures/joint_tiles.cpp, unit scene, example, group
WIRING = [
    (
        "side_side_outplane_fingers",
        "op/side_side_outplane_fingers/8/0.5",
        "op_side_side_outplane_fingers_8_0.5_unit",
        "element_joint_plate_side_side_outplane_fingers",
        "angle_90",
    ),
    (
        "top_side_tenons",
        "ts/top_side_tenons/8/0.5",
        "ts_top_side_tenons_8_0.5_unit",
        "element_joint_plate_top_side_tenons",
        "angle_90",
    ),
    (
        "cross_slot",
        "cr/cross_slot",
        "cr_cross_slot_unit",
        "element_joint_beam_cross_slot",
        "all",
    ),
]
CUTS = [
    ("10_profile_cuts", "Through slot / two separate pieces", "through cut"),
    ("10_profile_cuts", "Concave pocket / solid split", "blind pocket"),
    ("element_joint_drill", "all", "drill holes"),
    ("12_solid_features", "Difference / sloped solids", "solid pocket"),
]
# caption, example, group: one picture per family, the landing page's sheet
FAMILIES = [
    ("Shells", "templates_shells_1_translation_shell", "plates"),
    ("Folding Flat", "templates_folding_1_reflex_fold", "plates"),
    ("Folding Cylindrical", "templates_cylindrical_folding_1_reflection", "lamellas"),
    ("Folding Developable", "templates_developable_folding_1_curved_fold", "lamellas"),
    ("Chevron", "templates_chevron_1_chevron", "plates"),
    ("Cross", "templates_cross_1_vda_mesh", "plates"),
    ("Module", "templates_module_3_cassette_house", "all"),
    ("Reciprocal", "templates_reciprocal_1_rotation", "beams+frame"),
    ("Grid", "templates_grid_5_framing", "head_section"),
    ("Floor", "templates_floor_3_contacts", "all"),
    ("Truss", "templates_truss_1_pratt", "all"),
    ("Vault", "templates_vault_9_gothic_ribbed_vault", "tierceron_exeter"),
]
BEAM_SHEET = ["cross", "top_side", "end_to_end", "butt", "wedge", "centred_pins"]


def font() -> ImageFont.FreeTypeFont:
    """The first sans font found."""
    for name in [
        "Roboto-Regular.ttf",
        "arial.ttf",
        "DejaVuSans.ttf",
        "LiberationSans-Regular.ttf",
    ]:
        try:
            return ImageFont.truetype(name, 40)
        except OSError:
            continue
    return ImageFont.load_default(40)


def scene_of(example: str, tmp: pathlib.Path) -> pathlib.Path:
    """Runs the example and keeps the scene it writes."""
    kept = tmp / f"{example}.pb"
    docs.run_example(example, kept)
    return kept


def shot(
    scene: pathlib.Path,
    group: str,
    kinds: str,
    out: pathlib.Path,
    opacity: str = "1",
    xray: bool = False,
    aspect: float = ASPECT,
) -> Image.Image:
    """One group of a scene with the feature kinds asked for, shot from the viewer."""
    subprocess.run(
        [str(docs.PYTHON), "-c", SCENE, str(scene), group, str(out), kinds],
        check=True,
        capture_output=True,
    )
    return docs.render(out, "iso", opacity, "4800x3200", "", True, xray, aspect)


def sheet(tiles: list, columns: int, width: int, path: pathlib.Path) -> None:
    """Tiles of (picture, caption) with their captions on white, every picture filling its tile's width."""
    tile_width = (width - (columns + 1) * GAP) // columns
    tile_height = round(tile_width / ASPECT)
    rows = (len(tiles) + columns - 1) // columns
    canvas = Image.new(
        "RGB",
        (width, rows * (tile_height + CAPTION) + (rows + 1) * GAP),
        (255, 255, 255),
    )
    draw = ImageDraw.Draw(canvas)
    caption_font = font()

    for index, (picture, caption) in enumerate(tiles):
        x = GAP + (index % columns) * (tile_width + GAP)
        y = GAP + (index // columns) * (tile_height + CAPTION + GAP)
        scale = min(tile_width / picture.width, tile_height / picture.height)
        fitted = picture.resize(
            (round(picture.width * scale), round(picture.height * scale)), Image.LANCZOS
        )
        canvas.paste(
            fitted,
            (
                x + (tile_width - fitted.width) // 2,
                y + (tile_height - fitted.height) // 2,
            ),
        )
        draw.rectangle(
            (x, y, x + tile_width - 1, y + tile_height + CAPTION - 1),
            outline=FRAME,
            width=3,
        )
        draw.text(
            (x + tile_width // 2, y + tile_height + CAPTION // 2),
            caption,
            fill=TEXT,
            font=caption_font,
            anchor="mm",
        )

    canvas.save(path, "WEBP", quality=92)
    print(path.relative_to(WOOD), flush=True)


def examples(tmp: pathlib.Path) -> None:
    """The twelve numbered examples, the transparent ones showing what the solver found inside."""
    for example, (attributes, opacity, xray) in EXAMPLES.items():
        scene = scene_of(example, tmp)
        kept = tmp / f"{example}_shown.pb"
        picture = IMAGES / "examples" / f"ex_{example}.webp"
        kinds = "contact,joint,drill" if attributes else ""
        shot(scene, "all", kinds, kept, opacity, xray).save(picture, lossless=True)
        print(picture.relative_to(WOOD), flush=True)


def pipeline(tmp: pathlib.Path) -> None:
    """The same shell four times: its plates, the contacts found, the joints placed, the plates cut."""
    contacts = scene_of("2_contacts", tmp)
    features = scene_of("3_features", tmp)
    sheet(
        [
            (shot(contacts, "all", "", tmp / "plates.pb"), "1. plates"),
            (
                shot(contacts, "all", "contact", tmp / "contacts.pb", xray=True),
                "2. contacts",
            ),
            (shot(features, "all", "joint", tmp / "joints.pb", "0.4"), "3. joints"),
            (shot(features, "all", "", tmp / "cut.pb"), "4. cut plates"),
        ],
        2,
        2664,
        DIAGRAMS / "wood_architecture.webp",
    )


def landing(tmp: pathlib.Path) -> None:
    """The cut shell at the landing page's 16:9."""
    picture = IMAGES / "landing" / "landing_architecture.webp"
    features = scene_of("3_features", tmp)
    shot(features, "all", "", tmp / "landing.pb", aspect=16 / 9).save(picture, lossless=True)
    print(picture.relative_to(WOOD), flush=True)


def wiring(tmp: pathlib.Path) -> None:
    """Three joints, a column each, down the same three steps: the joint, the joint on its contact, the members it cut."""
    units = tmp / "units"
    units.mkdir()
    subprocess.run(
        [
            str(docs.BUILD / "joint_tiles"),
            str(units),
            *[column[1] for column in WIRING],
        ],
        check=True,
        capture_output=True,
        cwd=WOOD,
    )

    rows = [[], [], []]
    for caption, _, unit, example, group in WIRING:
        scene = scene_of(example, tmp)
        rows[0].append(
            (
                docs.render(
                    units / f"{unit}.pb",
                    "iso",
                    "1",
                    "4800x3200",
                    "",
                    True,
                    False,
                    ASPECT,
                ),
                f"{caption}: the joint",
            )
        )
        rows[1].append(
            (
                shot(scene, group, "contact,joint", tmp / f"{unit}_on.pb", xray=True),
                "on the contact",
            )
        )
        rows[2].append(
            (shot(scene, group, "", tmp / f"{unit}_cut.pb"), "the members cut")
        )

    sheet(rows[0] + rows[1] + rows[2], 3, 3600, DIAGRAMS / "wood_joint_unit_box.webp")


def cutting(tmp: pathlib.Path) -> None:
    """Stock, cutter and result of four cuts, the groups of the cutting examples."""
    tiles = []
    scenes = {}
    for example, group, caption in CUTS:
        scenes.setdefault(example, scene_of(example, tmp))
        tiles.append(
            (
                shot(
                    scenes[example], group, "", tmp / f"{caption.replace(' ', '_')}.pb"
                ),
                caption,
            )
        )
    sheet(tiles, 2, 2664, DIAGRAMS / "wood_boolean_cutting.webp")


def sheets(tmp: pathlib.Path) -> None:
    """The landing page's sheet of the template families, shot to the tile's shape, and the beam joints' sheet from the element pictures."""
    tiles = []
    for caption, example, group in FAMILIES:
        scene = tmp / f"{example}.pb"
        if not scene.exists():
            docs.run_example(example, scene)
        branch = tmp / f"{example}_{caption.replace(' ', '_')}.pb"
        subprocess.run([str(docs.PYTHON), "-c", docs.BRANCH, str(scene), group, str(branch), "elements"], check=True, capture_output=True)
        tiles.append((docs.render(branch, "iso", "1", "4800x3200", "", False, False, ASPECT), caption))
        print(caption, flush=True)
    sheet(tiles, 3, 3376, DIAGRAMS / "wood_templates_pipeline.webp")

    tiles = [
        (
            Image.open(IMAGES / "elements" / f"element_joint_beam_{name}.png").convert(
                "RGB"
            ),
            f"JointBeam {name}",
        )
        for name in BEAM_SHEET
    ]
    sheet(tiles, 3, 3376, DIAGRAMS / "wood_beam_joints_connectors.webp")


if __name__ == "__main__":
    wanted = sys.argv[1:] or ["examples", "pipeline", "landing", "wiring", "cutting", "sheets"]
    with tempfile.TemporaryDirectory() as folder:
        tmp = pathlib.Path(folder)
        for step in wanted:
            globals()[step](tmp)
