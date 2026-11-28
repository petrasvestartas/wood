"""Write the Assembly section of every template page, docs/templates/<page>.md, between the assembly markers: how the templates
of the page go together, and per example its modules' GIFs and the whole's (tools/render_template_assembly.py), with what its
ModuleSequence found (the elements, modules, moves and turns of build/assembly_scenes/<example>*.txt).
    python3 tools/assembly_docs.py
"""

import json
import os
import pathlib
import re

WOOD = pathlib.Path(__file__).resolve().parents[1]
SCENES = pathlib.Path(os.environ.get("WOOD_BUILD", WOOD / "build")) / "assembly_scenes"
IMAGES = WOOD / "docs/images/assembly"

# Per page: how its templates go together, then per example its title and what its modules are.
PAGES = {
    "shells": (
        "Two by two plates go together into one patch of four (three where a row is odd), plate by plate; the patches then go in one by one along "
        "the cross section, pair of rows after pair of rows, each down its miters against the patches beside it.",
        [("templates_shells_1_translation_shell", "Translation shell"), ("templates_shells_2_custom_curves", "Custom curves")],
    ),
    "folding": (
        "Plates meeting in miters, no joint. A row along the length goes together first, plate by plate, then the rows go in one after another "
        "from one foot over the crown to the other, each straight off the row before it; over a centred plan the modules are its sides and sectors.",
        [
            ("templates_folding_1_reflex_fold", "Reflex fold: a row per segment of the section, foot to foot"),
            ("templates_folding_2_diamond_mesh", "Diamond mesh: four rows, foot to foot"),
            ("templates_folding_3_prismatic_fold", "Prismatic fold: the diaphragms, then plate after plate from one side to the other"),
            ("templates_folding_4_folded_barrel", "Folded barrel: the two ends, then plate after plate from one foot to the other"),
            ("templates_folding_5_counter_fold", "Counter fold: a row per ridge with its flanks"),
            ("templates_folding_6_elevated_fold", "Elevated fold"),
            ("templates_folding_7_conical_fold", "Conical fold"),
            ("templates_folding_8_two_hinged_frame", "Two-hinged frame: four rows, foot to foot"),
            ("templates_folding_10_three_hinged_arch", "Three-hinged arch: four rows, foot to foot"),
            ("templates_folding_34_a_frame", "A-frame: two rows"),
            ("templates_folding_12_square_fold", "Square fold: the plan's four sides"),
            ("templates_folding_14_pyramid", "Pyramid"),
            ("templates_folding_15_truncated_pyramid", "Truncated pyramid"),
            ("templates_folding_36_folded_truncated_pyramid", "Folded truncated pyramid: four sides and the top"),
            ("templates_folding_20_triangle_square", "Triangles over a square: gables, corners, the top one module"),
            ("templates_folding_16_cross_fold", "Cross fold: four sectors"),
            ("templates_folding_19_fold_composition", "Fold composition: the star from its sectors, the pyramids around it"),
        ],
    ),
    "cylindrical_folding": (
        "The lamellas of one segment make one row along the profile; the rows go in one after another from one foot over the crown to the other. "
        "Curved plates are swept as closed polyhedra through their four meridians.",
        [
            ("templates_cylindrical_folding_1_reflection", "Reflection"),
            ("templates_cylindrical_folding_2_curved_folded_plate", "Curved folded plate arch: five rows of six lamellas"),
            ("templates_cylindrical_folding_3_mendrisio_pavilion", "Mendrisio pavilion"),
            ("templates_cylindrical_folding_5_two_hinged_arch", "Two-hinged arch"),
            ("templates_cylindrical_folding_7_a_frame", "A-frame"),
            ("templates_cylindrical_folding_8_groin_vault", "Groin vault"),
            ("templates_cylindrical_folding_9_pavilion_vault", "Pavilion vault: the walls stand, the lamellas lowered on them"),
            ("templates_cylindrical_folding_12_octagon_vault", "Octagon vault"),
            ("templates_cylindrical_folding_13_curved_folded_beam", "Curved folded beam: bottom, webs and sides, top"),
            ("templates_cylindrical_folding_14_corner_fan", "Corner fan"),
            ("templates_cylindrical_folding_16_cylinder_carving", "Cylinder carving: two and three carvers"),
            ("templates_cylindrical_folding_17_cylinder_carving_rings", "Carving rings"),
        ],
    ),
    "developable_folding": (
        "Two neighbours on their curved crease make one fold, and four walls around a space-curve parallelogram make one tube; "
        "the folds and tubes go round or side by side, and a flat-packed stack lifts off plate by plate.",
        [
            ("templates_developable_folding_1_curved_fold", "Curved fold tent roof"),
            ("templates_developable_folding_2_curved_crease_unfolding", "Curved crease arch"),
            ("templates_developable_folding_3_unfolding_sequence", "Accordion unfolding sequence"),
            ("templates_developable_folding_5_synclastic_unfolding", "Synclastic vault"),
            ("templates_developable_folding_6_cellular_tube_vault", "Cellular tube vault"),
            ("templates_developable_folding_9_alternating_unfolding", "Alternating vault"),
            ("templates_developable_folding_11_corrugated_dome", "Corrugated dome"),
            ("templates_developable_folding_12_corrugated_funnel", "Corrugated funnel"),
            ("templates_developable_folding_4_crease_quadrants", "Crease quadrants"),
            ("templates_developable_folding_14_curved_miura", "Curved Miura along a helix"),
            ("templates_developable_folding_15_smooth_space_curve_fold", "Smooth space curve fold"),
        ],
    ),
    "chevron": (
        "Every box goes together from four plates: the two walls on their corner fingers along the box's up, then the two plates "
        "along the box's insertion vector, their tenons into the walls. The boxes go in one after another along that one vector each. "
        "The 4 x 4 crown block is swept like everything else, the whole shell with 1 mm of play where the turned and moved edge planes make "
        "boxes bind; the MAX2 arch with 2.5 mm, its crown with 0.5 mm. The shifted chevron's even cells are boxes of five "
        "plates, the outward wall last, its odd cells shelves of three; its slots are opened along the boxes' ways in (`closed_boxes`).",
        [
            ("templates_chevron_1_chevron", "Chevron: box, crown block, whole shell"),
            ("templates_chevron_2_shifted", "Shifted: box, shelf, whole shell"),
            ("templates_chevron_3_max2_arch", "MAX2 arch: box, crown block, whole shell"),
        ],
    ),
    "cross": (
        "From one foot ring by ring up over the crown and down to the other foot, breadth first as the cardboard shell was built: a set is a "
        "plate with its connectors to the plates after it plugged in, and slides in its own plane onto the connectors of all the plates before "
        "it at once. `VdaMesh::assembly_order` gives the order and the slides, `VdaMesh::assembly_lines` turns every edge's connectors along "
        "the slide of its later plate. The last foot goes in from below, so the shell stands on props until then.",
        [("templates_cross_2_assembly", "VDA mesh: set, whole shell")],
    ),
    "module": (
        "Panels, cassettes and rooms as they leave the shop: every frame member by member, the sheets on it, then the panels into a module "
        "and the modules side by side and storey on storey.",
        [
            ("templates_module_1_wall", "Framed wall: rails, studs, headers and sills, a sheet on each face"),
            ("templates_module_2_cassette_shell", "Cassette shell: bottom plate, edge beams, top plate"),
            ("templates_module_3_cassette_house", "Cassette house"),
            ("templates_module_4_room_module", "Room module: floor, walls, posts, ceiling"),
            ("templates_module_5_module_block", "Module block"),
            ("templates_module_6_bank_pavilion", "Bank pavilion"),
            ("templates_module_7_module_types", "Module types: framed, solid, mixed"),
            ("templates_module_8_open_module", "Open modules: frame, braced"),
            ("templates_module_9_stacked_modules", "Stacked modules: bearings, modules, couplers"),
            ("templates_module_10_module_rack", "Rack: the skeleton, the modules slid in"),
            ("templates_module_11_module_frame", "Frame: a deck per storey, the modules on it"),
            ("templates_module_12_module_levels", "Levels: a deck every second storey"),
            ("templates_module_13_module_corridor", "Corridor between two rows"),
            ("templates_module_14_wet_room", "Wet room set in before the ceiling"),
        ],
    ),
    "timber_floor": (
        "The beams, then the ribs and the boards or the panels in their order, the concrete, what joins them last, all the layers on them as one; the earth rammed between its beams.",
        [
            ("templates_timber_floor_1_structures", "Structures"),
            ("templates_timber_floor_2_spans", "Sections from the span"),
            ("templates_timber_floor_3_joints", "Bearings and joints"),
            ("templates_timber_floor_4_earth", "Timber and earth"),
        ],
    ),
    "reciprocal": (
        "The frame beam by beam, then the beams from the frame to the middle. A beam slides in along the direction nearest to straight "
        "down that its neighbours' side faces leave it, and would turn first where no slide is left: no beam of the two needs to.",
        [("templates_reciprocal_1_rotation", "Rotation"), ("templates_reciprocal_2_move", "Move")],
    ),
    "grid": (
        "Storey by storey, in each the base supports, columns and walls, their heads and shear drop-panels, girders, beams, purlins, braces and decks.",
        [
            ("templates_grid_1_bay_framing", "Bay framing"),
            ("templates_grid_2_footprints", "Footprints"),
            ("templates_grid_3_facades", "Facades"),
            ("templates_grid_5_multistorey", "Multi-storey"),
            ("templates_grid_6_beam_grids", "Beam and grillage systems"),
            ("templates_grid_7_slabs", "Structural slab systems"),
        ],
    ),
    "frame": (
        "Stage by stage from the foundation up: steel base shoes, tapered legs and central piers, haunched and pitched rafters down their "
        "knee miters and half-lap seats, Vierendeel bottom chords, shear posts and top chords, floor and roof purlins, and cellular box-frame "
        "bottom slabs, side walls and top slabs.",
        [
            ("templates_frame_1_portal_frames", "Portal, gable and articulated T-frames"),
            ("templates_frame_2_tiered_frames", "Coupled and tiered hinged frames"),
            ("templates_frame_3_vierendeel_frames", "Shear-gradated and concentric Vierendeel frames"),
            ("templates_frame_4_box_frames", "Cellular slab box frames"),
        ],
    ),
    "vault": (
        "Block by block from the springings to the crown; the Gothic bays their support blocks, the ribs on them, the web panels on the "
        "ribs; the subdivided NURBS and sewn multi-surface NGon shells along their planarized side joints, including shifted and bevelled "
        "facets. A block is let down along the direction nearest to straight down its neighbours leave it. The web panels, trimmed surfaces, "
        "are let down on the ribs unswept.",
        [
            ("templates_vault_1_arch", "Arch"),
            ("templates_vault_2_barrel", "Barrel"),
            ("templates_vault_4_dome", "Dome"),
            ("templates_vault_5_wall", "Wall"),
            ("templates_vault_6_cross_vault", "Cross vault"),
            ("templates_vault_7_cloister_vault", "Cloister vault"),
            ("templates_vault_9_gothic_ribbed_vault", "Gothic ribbed vaults"),
            ("templates_vault_10_surface_subdivisions", "Surface subdivisions"),
            ("templates_vault_11_shifted_facets", "Shifted facets"),
        ],
    ),
    "plaited": (
        "The blocks interlock, so every one goes in along a direction of its own, leaning with its bevels, and some with a turn; the "
        "reciprocal arch its frame and its bars, then every joint's two pins, leaning in through the bar's cheek.",
        [
            ("templates_plaited_1_flat_floor", "Flat floor"),
            ("templates_plaited_2_arch", "Arch"),
            ("templates_plaited_3_dome", "Dome"),
            ("templates_plaited_4_hotwire_blocks", "Hot-wire blocks"),
            ("templates_plaited_5_reciprocal_arch", "Reciprocal arch"),
        ],
    ),
    "floor": (
        "Every quarter on its column: the ribs fanned around the head, the blocks between them, the beams, the sections; the "
        "oculus its ring beams, wedges and plate; then the wedges between the quarters, every pin along its own axis, and the beds over them.",
        [("templates_floor_7_contacts_cantilevers", "Square floor"), ("templates_floor_8_rectangle", "Rectangular floor")],
    ),
    "vidy": (
        "A row is two walls and one roof. Each of the three goes together plate by plate, its tenons into the plates before it, a shell plate "
        "along its one insertion vector where that is free. First the two walls go up, along the hall against the walls before them, then the roof is "
        "lowered on them, then the next row's walls and its roof, row after row. One corner of one roof grazes the next (46 mm3), so the whole is swept "
        "with 0.9 mm of play.",
        [
            ("templates_vidy_4_section_joints", "Pavilion: wall, roof, eleven rows"),
            ("templates_vidy_5_parametric", "Parametric family"),
            ("templates_vidy_6_origami_shapes", "Deep folded halls"),
        ],
    ),
}
COLUMNS = 3


def words(kind: str) -> str:
    """A module kind as its caption."""
    return kind.replace("_", " ")


def rows(example: str, title: str) -> list:
    """The example's line, what was found and its GIFs, COLUMNS to a row."""
    # the pictures there are, every assembly's modules in the order they go together, its whole last
    names = []
    for path in sorted(SCENES.glob(f"{example}.json")) + sorted(SCENES.glob(f"{example}_*.json")):
        assembly = json.loads(path.read_text())
        for module in assembly["modules"]:
            whole = module["name"] == assembly["root"]
            name = f"assembly_{path.stem}.gif" if whole else f"assembly_{path.stem}_{module['kind']}.gif"
            caption = words(module["name"] if whole else module["kind"])
            if (IMAGES / name).exists() and name not in [known for known, _ in names]:
                names.append((name, f"{caption}, {len(module['moves'])} pieces"))
    if not names:
        return []
    text = (SCENES / f"{example}.txt").read_text()
    counts = re.findall(r"^\S+: (\d+) elements, (\d+) modules, (\d+) moves \((\d+) with a turn\)", text, re.M)
    elements, _, moves, turns = [sum(int(count[k]) for count in counts) for k in range(4)]
    said = f"{elements} elements, {moves} moves" + (f", {turns} with a turn" if turns else "")
    lines = [f"| **{title}**, {said} |" + " |" * (COLUMNS - 1)]
    for start in range(0, len(names), COLUMNS):
        row = names[start:start + COLUMNS]
        cells = [f"![{caption}](assembly/{name}) {caption}" for name, caption in row]
        lines.append("| " + " | ".join(cells + [""] * (COLUMNS - len(row))) + " |")
    return lines


def main() -> None:
    """Writes the section of every page, in place of the one between the markers, else before its examples or at its end."""
    for page, (intro, examples) in PAGES.items():
        path = WOOD / f"docs/templates/{page}.md"
        text = path.read_text()
        table = [line for example, title in examples for line in rows(example, title)]
        if not table:
            continue
        anchor = f"{{#{page}_assembly_sequence}}"
        section = "\n".join(
            [
                "<!-- assembly:begin -->",
                f"## Assembly Sequence {anchor}",
                "",
                intro + " Every move is a `ModuleSequence` move: the module taken apart piece by piece, each piece swept as a solid against every "
                "piece still in, the moving ones drawn brown, a GIF per kind of module and one of the whole.",
                "",
                "| assembly |" + " |" * (COLUMNS - 1),
                "|---" * COLUMNS + "|",
                *table,
                "",
                "<!-- assembly:end -->",
                "",
            ]
        )
        if "<!-- assembly:begin -->" in text:
            text = re.sub(r"<!-- assembly:begin -->.*?<!-- assembly:end -->\n", lambda match: section, text, flags=re.S)
        else:
            place = re.search(r"^## Examples \{#", text, re.M)
            text = text[: place.start()] + section + "\n" + text[place.start():] if place else text.rstrip() + "\n\n" + section
        # a card for it in the page's first card grid, its picture the first example's last: the whole
        picture = []
        for line in table[1:]:
            if "](assembly/" not in line:
                break
            picture += re.findall(r"assembly/(assembly_\w+\.gif)", line)
        text = re.sub(r'  <a class="wood-card" href="#\w+_assembly_sequence">.*?</a>\n', "", text, flags=re.S)
        grid = re.search(r'<div class="wood-card-grid.*?\n</div>', text, re.S)
        if picture and grid and grid.start() < text.index("<!-- assembly:begin -->"):
            made = (
                f'  <a class="wood-card" href="#{page}_assembly_sequence">\n    <img src="{picture[-1]}" alt="Assembly Sequence">\n'
                '    <span class="wood-card-title">Assembly Sequence</span>\n  </a>\n'
            )
            text = text[: grid.end() - 6] + made + text[grid.end() - 6 :]
        path.write_text(text)
        print(path.relative_to(WOOD), len(table), "rows")


if __name__ == "__main__":
    main()
