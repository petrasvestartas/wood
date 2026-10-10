#!/usr/bin/env python3
"""Dump the 2025 CGAL solver's answer for every wood dataset into tests/golden/reference_2025/<name>.json.

The reference solver is wood_nano 0.3.5 + compas_wood 2.4.0, the truth the port is scored against
by tests/reference_datasets.cpp. Both solvers read the same inputs: wood's data/<name>.obj outlines,
the yml tunables and the sidecars (insertion vectors, joint types, three valence, adjacency).

    /home/pv/.cache/wood_ref_2025/bin/python tools/reference_2025.py            # every plate dataset
    /home/pv/.cache/wood_ref_2025/bin/python tools/reference_2025.py inplane_hilti annen_box
    /home/pv/.cache/wood_ref_2025/bin/python tools/reference_2025.py --volumes outofplane_box   # detection dump to stdout

Per dataset the json holds, per plate in obj order, the merged outlines of output type 4 (the
legacy interleaved layout: hole pairs first, the outer top and bottom last) and, per plate again,
the joint polylines of output type 3.

A dataset whose yml has a beams block (phanomema_node) is solved by the beam solver instead, beam_volumes with its joints
computed: the json holds the beam pairs, the joint types, the four joint volume rectangles per pair and, per beam, the
joint outlines of output type 3 with their cut types.

--volumes prints the detection stage instead of writing json: per plate, the joint lines of output
type 1 (L) and the joint volumes of output type 2 (V), six decimals, to compare with wood's
InteractionFeaturePlate joint_lines and joint_volumes.
"""

import json
import os
import sys

from compas_wood.binding import get_connection_zones, wood_globals
from compas.geometry import Point, Polyline, Vector

WOOD = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(WOOD, "data")
OUT = os.path.join(WOOD, "tests", "golden", "reference_2025")

SEARCH_TYPES = {"face_to_face": 0, "cross_joint": 1, "face_to_face_then_cross": 2}

# wood's short name -> the 2024 dataset xml it was converted from, for the record only.
XML_NAMES = {
    "hexbox_and_corner": "type_plates_name_hexbox_and_corner",
    "vidy_corner": "type_plates_name_joint_linking_vidychapel_corner",
    "vidy_one_layer": "type_plates_name_joint_linking_vidychapel_one_layer",
    "vidy_one_axis_two_layers": "type_plates_name_joint_linking_vidychapel_one_axis_two_layers",
    "vidy_full": "type_plates_name_joint_linking_vidychapel_full",
    "inplane_butterflies": "type_plates_name_side_to_side_edge_inplane_2_butterflies",
    "inplane_hexshell": "type_plates_name_side_to_side_edge_inplane_hexshell",
    "inplane_differentdirections": "type_plates_name_side_to_side_edge_inplane_differentdirections",
    "vidy_folding": "type_plates_name_side_to_side_edge_outofplane_folding",
    "outofplane_box": "type_plates_name_side_to_side_edge_outofplane_box",
    "outofplane_box_miter": "type_plates_name_side_to_side_edge_outofplane_box",
    "outofplane_tetra": "type_plates_name_side_to_side_edge_outofplane_tetra",
    "outofplane_dodecahedron": "type_plates_name_side_to_side_edge_outofplane_dodecahedron",
    "outofplane_icosahedron": "type_plates_name_side_to_side_edge_outofplane_icosahedron",
    "outofplane_octahedron": "type_plates_name_side_to_side_edge_outofplane_octahedron",
    "simple_corners": "type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners",
    "simple_corners_combined": "type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_combined",
    "simple_corners_diff_lengths": "type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths",
    "inplane_hilti": "type_plates_name_side_to_side_edge_inplane_hilti",
    "top_to_top_pairs": "type_plates_name_top_to_top_pairs",
    "hexboxes": "type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes",
    "hex_block_rossiniere": "type_plates_name_hex_block_rossiniere",
    "top_to_side_box": "type_plates_name_top_to_side_box",
    "top_to_side_corners": "type_plates_name_top_to_side_corners",
    "top_to_side_test": "type_plates_name_top_to_side_test",
    "annen_corner": "type_plates_name_top_to_side_and_side_to_side_outofplane_annen_corner",
    "annen_box": "type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box",
    "annen_box_pair": "type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box_pair",
    "annen_grid_small": "type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_small",
    "vda_floor_0": "type_plates_name_vda_floor_0",
    "vda_floor_1": "type_plates_name_vda_floor_1",
    "vda_floor_2": "type_plates_name_vda_floor_2",
    "cross_and_sides_corner": "type_plates_name_cross_and_sides_corner",
    "cross_corners": "type_plates_name_cross_corners",
    "cross_vda_corner": "type_plates_name_cross_vda_corner",
    "cross_vda_hexshell": "type_plates_name_cross_vda_hexshell",
    "cross_vda_hexshell_reciprocal": "type_plates_name_cross_vda_hexshell_reciprocal",
    "cross_vda_single_arch": "type_plates_name_cross_vda_single_arch",
    "cross_vda_shell": "type_plates_name_cross_vda_shell",
    "cross_square_reciprocal_two_sides": "type_plates_name_cross_square_reciprocal_two_sides",
    "cross_square_reciprocal_iseya": "type_plates_name_cross_square_reciprocal_iseya",
    "cross_ibois_pavilion": "type_plates_name_cross_ibois_pavilion",
    "cross_brussels_sports_tower": "type_plates_name_cross_brussels_sports_tower",
    "cross_brg_slab_0": "type_plates_name_cross_brg_slab_0",
}


# ═══════════════════════════════════════════════════════════════════════════
# Inputs: the yml, the obj, the sidecars, as wood reads them
# ═══════════════════════════════════════════════════════════════════════════


def read_yml(path):
    """The flat yml of a dataset: scalars as strings, `- item` sequences as lists of strings; comments dropped."""
    values = {}
    key = None
    for raw in open(path, encoding="utf-8"):
        line = raw.split("#", 1)[0].rstrip()
        if not line.strip():
            continue
        if line.lstrip().startswith("- "):
            values.setdefault(key, []).append(line.strip()[2:].strip())
            continue
        name, _, rest = line.partition(":")
        key = name.strip()
        rest = rest.strip()
        values[key] = rest if rest else []
    return values


def read_obj(path):
    """The polylines of a wood obj: `v` records and `curv ... end` blocks of 1-based vertex indices, as file_obj does."""
    vertices = []
    polylines = []
    indices = None
    for raw in open(path, encoding="utf-8"):
        line = raw.rstrip("\n")
        if not line or line[0] == "#":
            continue
        if line.startswith("v "):
            parts = line[2:].split()
            vertices.append([float(parts[0]), float(parts[1]), float(parts[2])])
        elif line.startswith("curv "):
            indices = [int(p) for p in line[5:].split()[2:]]
        elif line.startswith("end") and indices is not None:
            points = [vertices[i - 1] for i in indices if 0 < i <= len(vertices)]
            if len(points) >= 2:
                polylines.append(points)
            indices = None
    return polylines


def remove_consecutive_duplicates(points, tolerance):
    """Consecutive points closer than tolerance collapsed, as load_obj does when duplicate_pts_tol > 0."""
    kept = [points[0]]
    for p in points[1:]:
        q = kept[-1]
        if sum((a - b) ** 2 for a, b in zip(p, q)) > tolerance * tolerance:
            kept.append(p)
    return kept


def read_rows(path, cast):
    """One list per non-empty line of a sidecar, every token cast."""
    rows = []
    for raw in open(path, encoding="utf-8"):
        tokens = raw.split()
        if tokens:
            rows.append([cast(t) for t in tokens])
    return rows


def sidecar(values, key, name):
    """The absolute path of a sidecar the yml names, or None."""
    if key not in values or not values[key]:
        return None
    return os.path.join(DATA, values[key])


def read_inputs(name):
    """Everything the reference solver takes for one dataset, from wood's data folder."""
    values = read_yml(os.path.join(DATA, name + ".yml"))
    tolerance = float(values.get("duplicate_pts_tol", "0.0") or 0.0)

    outlines = read_obj(os.path.join(DATA, values["obj"]))
    if tolerance > 0.0:
        outlines = [remove_consecutive_duplicates(p, tolerance) for p in outlines]
    pairs = [Polyline(points) for points in outlines]
    count = len(pairs) // 2

    vectors = []
    path = sidecar(values, "insertion_vectors", name)
    if path:
        for row in read_rows(path, float)[:count]:
            vectors.append([Vector(row[i], row[i + 1], row[i + 2]) for i in range(0, len(row) - 2, 3)])
        while len(vectors) < count:
            vectors.append([])

    types = []
    path = sidecar(values, "joints_types", name)
    if path:
        types = read_rows(path, int)[:count]
        while len(types) < count:
            types.append([])

    three = []
    path = sidecar(values, "three_valence", name)
    if path:
        three = read_rows(path, int)

    adjacency = []
    path = sidecar(values, "adjacency", name)
    if path:
        # a row of two elements, or two elements and their faces: a plate paired with itself on a face is a boundary joint
        for row in read_rows(path, int):
            adjacency.extend([row[0], row[1], row[2] if len(row) > 3 else -1, row[3] if len(row) > 3 else -1])

    return values, pairs, vectors, types, three, adjacency


# ═══════════════════════════════════════════════════════════════════════════
# The reference solve
# ═══════════════════════════════════════════════════════════════════════════


def apply_globals(values):
    """wood's yml tunables onto the reference solver's globals, every one set so datasets do not leak into each other."""
    wood_globals.distance = float(values.get("distance", 0.1))
    wood_globals.distance_squared = float(values.get("distance_squared", 0.01))
    wood_globals.angle = float(values.get("angle", 0.11))
    wood_globals.clipper_scale = int(float(values.get("clipper_scale", 1000000)))
    wood_globals.clipper_area = float(values.get("clipper_area", 0.01))
    wood_globals.limit_min_joint_length = float(values.get("limit_min_joint_length", 0.0))
    wood_globals.face_to_face_side_to_side_joints_dihedral_angle = float(values.get("face_to_face_side_to_side_joints_dihedral_angle", 150.0))
    wood_globals.face_to_face_side_to_side_joints_all_treated_as_rotated = str(values.get("face_to_face_side_to_side_joints_all_treated_as_rotated", "false")).lower() == "true"
    wood_globals.face_to_face_side_to_side_joints_rotated_joint_as_average = str(values.get("face_to_face_side_to_side_joints_rotated_joint_as_average", "false")).lower() == "true"
    wood_globals.joint_volume_extension = [0.0, 0.0, 0.0, 0.0, 0.0]


def polyline_coords(polyline):
    return [[round(p[0], 6), round(p[1], 6), round(p[2], 6)] for p in polyline.points]


def solve(name):
    """One dataset through the reference solver: the json record."""
    values, pairs, vectors, types, three, adjacency = read_inputs(name)
    apply_globals(values)

    parameters = [float(v) for v in values["joints_parameters_and_types"]]
    extension = [float(v) for v in values.get("joint_volume_extension", ["0", "0", "0"])]
    scale = [float(v) for v in values.get("joint_scale", ["1", "1", "1"])]
    search_type = SEARCH_TYPES[values.get("search_type", "face_to_face")]

    merged, _, _ = get_connection_zones(pairs, vectors, types, three, adjacency, parameters, search_type, scale, 4, extension, [], [], False)
    joints, _, _ = get_connection_zones(pairs, vectors, types, three, adjacency, parameters, search_type, scale, 3, extension, [], [], False)

    plates = []
    for i in range(len(pairs) // 2):
        outlines = merged[i] if i < len(merged) else []
        plates.append({"outlines": [polyline_coords(p) for p in outlines]})

    return {
        "dataset": name,
        "xml": XML_NAMES.get(name),
        "solver": "wood_nano 0.3.5 + compas_wood 2.4.0",
        "search_type": search_type,
        "plates": plates,
        "joints": [[polyline_coords(p) for p in joint] for joint in joints],
    }


def solve_beams(name):
    """One beam dataset through the reference beam solver: the json record."""
    from wood_nano import beam_volumes, int1, int2, double2, point2, point3, cut_type2
    from wood_nano.conversions_python import to_double2, to_int1, from_int1, from_int2, from_cut_type2
    from compas_wood.conversions_compas import to_point2, to_vector2, from_point3

    values = read_yml(os.path.join(DATA, name + ".yml"))
    apply_globals(values)
    radius, allowed, min_distance, volume_length, cross_or_side_to_end, flip_male = [float(v) for v in values["beams"]]
    axes = [Polyline([Point(*p) for p in points]) for points in read_obj(os.path.join(DATA, values["obj"]))]
    radii = [[radius] * len(axis.points) for axis in axes]
    # no segment directions, as 2024's test passed none: each pair's frame takes the normal of its two axes
    directions = []

    pairs, segments, distances, points, volumes, areas, types, outlines, cut_types = int2(), int2(), double2(), point2(), point3(), point2(), int1(), point3(), cut_type2()
    beam_volumes(
        to_point2(axes), to_double2(radii), to_vector2(directions), to_int1([int(allowed)]), min_distance, volume_length,
        cross_or_side_to_end, int(flip_male), pairs, segments, distances, points, volumes, areas, types, outlines, cut_types,
        True, 150.0, 0.5, 3, True,
    )

    return {
        "dataset": name,
        "solver": "wood_nano 0.3.5 + compas_wood 2.4.0, beam_volumes",
        "pairs": from_int2(pairs),
        "types": from_int1(types),
        "volumes": [[polyline_coords(Polyline(r)) for r in group] for group in from_point3(volumes)],
        "beams": [
            {"outlines": [polyline_coords(Polyline(p)) for p in group], "cut_types": [str(t) for t in kinds]}
            for group, kinds in zip(from_point3(outlines), from_cut_type2(cut_types))
        ],
    }


def print_detection(name):
    """One dataset's joint lines and joint volumes from the reference solver, per plate, to stdout."""
    values, pairs, vectors, types, three, adjacency = read_inputs(name)
    apply_globals(values)

    parameters = [float(v) for v in values["joints_parameters_and_types"]]
    extension = [float(v) for v in values.get("joint_volume_extension", ["0", "0", "0"])]
    scale = [float(v) for v in values.get("joint_scale", ["1", "1", "1"])]
    search_type = SEARCH_TYPES[values.get("search_type", "face_to_face")]

    lines, _, _ = get_connection_zones(pairs, vectors, types, three, adjacency, parameters, search_type, scale, 1, extension, [], [], False)
    volumes, _, _ = get_connection_zones(pairs, vectors, types, three, adjacency, parameters, search_type, scale, 2, extension, [], [], False)

    for i in range(len(pairs) // 2):
        print(f"PLATE {i}")
        for tag, polylines in (("L", lines), ("V", volumes)):
            for k, polyline in enumerate(polylines[i] if i < len(polylines) else []):
                coords = " ".join(f"{p[0]:.6f} {p[1]:.6f} {p[2]:.6f}" for p in polyline.points)
                print(f"{tag} {k}: {coords}")


def dataset_names():
    return sorted(f[:-4] for f in os.listdir(DATA) if f.endswith(".yml"))


def dump(name):
    """One dataset solved and written; the line the sweep prints."""
    beams = "beams" in read_yml(os.path.join(DATA, name + ".yml"))
    record = solve_beams(name) if beams else solve(name)
    path = os.path.join(OUT, name + ".json")
    with open(path, "w", encoding="utf-8") as out:
        json.dump(record, out, separators=(",", ":"))
    if beams:
        return f"{name}: {len(record['beams'])} beams, {len(record['pairs'])} pairs, {len(record['types'])} joints -> {os.path.relpath(path, WOOD)}"
    holes = sum(max(0, len(p["outlines"]) - 2) // 2 for p in record["plates"])
    return f"{name}: {len(record['plates'])} plates, {len(record['joints'])} joints, {holes} holes -> {os.path.relpath(path, WOOD)}"


def main(argv):
    os.makedirs(OUT, exist_ok=True)

    # the detection dump: no json, the lines and volumes of every named dataset to stdout
    if len(argv) > 2 and argv[1] == "--volumes":
        for name in argv[2:]:
            print(f"DATASET {name}")
            print_detection(name)
        return 0

    # one dataset: solve it here, so a crash of the reference solver is this process's exit code
    if len(argv) == 2:
        print(dump(argv[1]), flush=True)
        return 0

    # the sweep: every dataset in its own process, so one segfault of the reference solver costs one dataset
    import subprocess

    names = argv[1:] or dataset_names()
    crashed = []
    for name in names:
        result = subprocess.run([sys.executable, "-I", os.path.abspath(__file__), name], capture_output=True, text=True, timeout=600)
        lines = [l for l in result.stdout.splitlines() if l.startswith(name + ":")]
        if result.returncode == 0 and lines:
            print(lines[-1], flush=True)
        else:
            crashed.append(name)
            print(f"{name}: reference solver exit {result.returncode}, no json written", flush=True)
    print(f"{len(names) - len(crashed)} / {len(names)} datasets dumped; crashed: {', '.join(crashed) or 'none'}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
