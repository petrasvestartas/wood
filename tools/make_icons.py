"""The icons of the docs, docs/doxygen/icons.svg, one symbol each: flat drawings on a 24 grid, a black outline with square corners,
light green fills, the part that joins the dark green. The Rhino plugin's own icons (wood_nano/plugin_rhino/plugin/icons) keep their points, their
dots square; the others are drawn after them.

    python3 tools/make_icons.py"""

import math
import pathlib

WOOD = pathlib.Path(__file__).resolve().parents[1]
FILLS = {"a": "#cbebdc", "b": "#8fd3b3", "c": "#57b98e", "g": "#e6e6e6", "j": "#007c56", "w": "#ffffff", "k": "#000000"}


def quad(fill: str, x0: float, y0: float, x1: float, y1: float) -> tuple:
    """A rectangle by two corners."""
    return (fill, [(x0, y0), (x1, y0), (x1, y1), (x0, y1)])


def shape(fill: str, *numbers: float) -> tuple:
    """A polygon by its numbers, x and y in turn; "line" is open."""
    return (fill, list(zip(numbers[::2], numbers[1::2])))


def band(fills: str, outer: list, inner: list) -> list:
    """The quads between two polylines."""
    return [(fills[k % len(fills)], [outer[k], outer[k + 1], inner[k + 1], inner[k]]) for k in range(len(outer) - 1)]


def arc(x: float, y: float, radius: float, start: float, end: float, count: int) -> list:
    """Points of an arc as straight pieces, degrees, up is negative y."""
    angles = [math.radians(start + (end - start) * k / count) for k in range(count + 1)]
    return [(x + radius * math.cos(angle), y - radius * math.sin(angle)) for angle in angles]


# ------------------------------------------------------------------ the plugin's icons


PLUGIN = {
    "chevron": [("b", "10.2 22.24 2.91 17.57 3.99 2.43 11.86 5.01"), ("a", "19.48 19.89 10.2 22.24 11.86 5.01 21.09 4.08")],
    "connectors": [
        ("line", "2.25 4.52 21 21.86"), ("a", "11.15 16.31 11.72 22.72 18.65 15.06 18.63 7.86"), ("a", "4.52 10.76 5.09 17.17 12.36 9.18 12.14 2.13"),
        ("line", "20.32 21.23 15.08 15.99"), ("line", "11.63 13.19 8.76 10.32"),
    ],
    "diamond_mesh": [
        ("c", "11.98 11.88 12.06 3.06 22.6 1.06"), ("a", "12.07 11.9 11.99 3.09 1.45 1.09"), ("a", "12.1 12.02 11.94 20.84 1.4 22.76"),
        ("c", "12 12 12.01 20.81 22.55 22.89"), ("c", "1.45 1.09 1.4 22.76 11.98 11.88"), ("a", "22.6 1.06 22.55 22.89 12.1 12.02"),
    ],
    "insertion_direction": [
        ("thick", "21.41 8.5 16.42 8.5"), ("thick", "7.91 17.94 2.91 17.94"), ("g", "12.3 4.4 21.06 13.16 12.3 21.92 3.54 13.16"),
        ("k", "13.83 6.5 17.83 6.5 17.83 10.5 13.83 10.5"), ("k", "6.3 15.94 10.3 15.94 10.3 19.94 6.3 19.94"),
    ],
    "install": [
        ("line", "12.11 3.4 12.11 17.93"), ("line", "7.69 12.08 12.11 17.93 16.59 12.08"),
        ("line", "21.35 15.71 21.35 20.59 2.65 20.59 2.65 15.71"),
    ],
    "joint_type": [
        ("g", "12 3.24 20.76 12 12 20.76 3.24 12"), ("k", "14.66 5.34 18.66 5.34 18.66 9.34 14.66 9.34"), ("k", "5.34 5.34 9.34 5.34 9.34 9.34 5.34 9.34"), ("k", "5.34 14.78 9.34 14.78 9.34 18.78 5.34 18.78"), ("k", "14.66 14.78 18.66 14.78 18.66 18.78 14.66 18.78"),
    ],
    "reciprocal_move": [
        ("a", "19.92 3.99 19.96 9.54 2.72 11.86 2.53 6.24"), ("a", "18.75 11.91 18.78 19.37 15.14 9.07 14.96 1.56"),
        ("a", "10.65 12.83 10.68 20.28 7.05 9.99 6.86 2.47"), ("a", "22.17 14.68 22.2 20.23 4.97 22.55 4.78 16.93"),
    ],
    "reciprocal_rotation": [
        ("b", "14.52 8.56 14.4 15.65 3.78 8.23 3.75 1.07"), ("a", "1.39 15.83 1.26 22.93 9.15 12.41 9.12 5.25"), ("a", "22.79 7.29 22.67 14.38 5.51 16.93 5.48 9.77"),
    ],
    "reflex_fold": [("c", "14.38 4.02 8.33 2.06 15.99 16.79 22.5 18.24"), ("a", "9.85 21.94 3.19 20.25 8.11 2.06 14.38 4.02")],
    "solver": [("solid", "0 6.63 6.01 12.73 11.94 6.77 18.01 12.73 24 6.61 24 0 0 0"), ("solid", "0 12.74 6 18.72 12.07 12.78 18 18.71 24 12.73 24 24 0 24")],
    "translation_shell": [
        ("b", "10.91 11.57 2.76 11.09 5.91 2.18 13.9 2.49"), ("a", "20.71 13.14 10.91 11.57 13.9 2.49 23.14 4.39"),
        ("c", "9.36 21.21 1.49 19.83 2.76 11.09 10.91 11.35"), ("a", "19.32 21.71 9.36 21.21 10.91 11.57 20.71 13.14"),
    ],
}


# ------------------------------------------------------------------ the ones drawn after them


def new_icons() -> dict:
    """Every other icon, its shapes back to front."""
    icons = {}

    # chapters
    icons["architecture"] = [shape("line", 12, 9, 12, 12.5), shape("line", 5.5, 15.5, 5.5, 12.5, 18.5, 12.5, 18.5, 15.5), quad("j", 8, 2, 16, 9), quad("a", 2, 15.5, 9, 22), quad("a", 15, 15.5, 22, 22)]
    icons["elements"] = [quad("a", 3, 2, 8.5, 17), quad("b", 11, 9, 20, 17), quad("j", 2, 17, 22, 22)]
    icons["classes"] = [shape("line", 8, 2.5, 3, 2.5, 3, 21.5, 8, 21.5), shape("line", 16, 2.5, 21, 2.5, 21, 21.5, 16, 21.5), quad("j", 8.5, 8.5, 15.5, 15.5)]

    # nodes, edges, faces
    icons["block"] = [shape("a", 6, 3, 18, 3, 22, 21, 2, 21)]
    icons["support"] = [quad("b", 10, 6, 14, 17), quad("a", 5, 2, 19, 6), quad("a", 2, 17, 22, 20.5), quad("j", 4.5, 20.5, 7, 23), quad("j", 17, 20.5, 19.5, 23)]
    icons["edge_straight"] = [quad("a", 1.5, 8, 18, 16), quad("c", 18, 8, 22.5, 16)]
    icons["edge_curved"] = [("a", arc(12, 20, 12, 165, 15, 6) + arc(12, 20, 9, 15, 165, 6)), ("b", arc(12, 20, 9, 165, 15, 6) + arc(12, 20, 6, 15, 165, 6))]
    icons["edge_variable"] = [shape("a", 1.5, 4, 18, 8, 18, 16, 1.5, 20), shape("c", 18, 8, 22.5, 8, 22.5, 16, 18, 16)]
    icons["face_planar"] = [shape("a", 7, 5, 22.5, 5, 17, 19, 1.5, 19)]
    icons["face_curved"] = [shape("c", 1.5, 20, 4.5, 11, 4.5, 4, 1.5, 13), shape("a", 4.5, 11, 12, 8, 12, 1.5, 4.5, 4), shape("b", 12, 8, 19.5, 11, 19.5, 4, 12, 1.5), shape("c", 19.5, 11, 22.5, 20, 22.5, 13, 19.5, 4)]
    icons["cut_plane"] = [shape("a", 1.5, 8, 8.4, 8, 12, 16, 1.5, 16), shape("b", 15.2, 8, 22.5, 8, 22.5, 16, 18.8, 16), shape("j", 8.4, 1.5, 11, 1.5, 20, 22.5, 17.4, 22.5)]
    icons["pin"] = [shape("j", 1.5, 10, 4, 8.5, 20, 8.5, 22.5, 10, 22.5, 14, 20, 15.5, 4, 15.5, 1.5, 14), shape("line", 4, 8.5, 4, 15.5), shape("line", 20, 8.5, 20, 15.5)]

    # joints of plates
    icons["plate_top_side"] = [quad("a", 5, 2, 19, 11), quad("j", 7, 11, 10, 15), quad("j", 14, 11, 17, 15), quad("b", 1.5, 16.5, 22.5, 22), quad("w", 7, 16.5, 10, 22), quad("w", 14, 16.5, 17, 22)]
    icons["plate_side_side_inplane"] = [quad("b", 10, 3, 22.5, 21), quad("a", 1.5, 3, 10, 21), shape("j", 10, 7, 15, 5.4, 15, 10.6, 10, 9), shape("j", 10, 15, 15, 13.4, 15, 18.6, 10, 17)]
    icons["plate_side_side_outplane"] = [quad("a", 1.5, 2, 9.5, 22), quad("b", 14.5, 2, 22.5, 22)] + [quad("j" if k % 2 else "a", 9.5, 2 + 4 * k, 14.5, 6 + 4 * k) for k in range(5)]
    icons["plate_side_side_rotated"] = [shape("a", 1.5, 7, 12, 3, 12, 18, 1.5, 22), shape("b", 12, 3, 22.5, 7, 22.5, 22, 12, 18), shape("j", 8, 8.6, 12, 7, 16, 8.6, 16, 14.6, 12, 13, 8, 14.6)]
    icons["plate_cross"] = [quad("a", 9, 1.5, 15, 22.5), quad("b", 1.5, 9, 22.5, 15), quad("j", 9, 9, 15, 15)]
    icons["plate_top_top"] = [quad("j", 4, 8, 7, 16), quad("j", 10.5, 8, 13.5, 16), quad("j", 17, 8, 20, 16), quad("a", 1.5, 2.5, 22.5, 8), quad("b", 1.5, 16, 22.5, 21.5)]
    icons["plate_border"] = [quad("j", 1.5, 1.5, 22.5, 22.5), shape("line", 1.5, 1.5, 7, 7), shape("line", 22.5, 1.5, 17, 7), shape("line", 22.5, 22.5, 17, 17), shape("line", 1.5, 22.5, 7, 17), quad("a", 7, 7, 17, 17)]

    # joints of beams
    icons["beam"] = [quad("b", 9, 9, 15, 22.5), quad("a", 1.5, 2.5, 22.5, 9), quad("j", 10.5, 4.2, 13.5, 7.2)]
    icons["beam_cross"] = [shape("a", 4.5, 1.5, 22.5, 19.5, 19.5, 22.5, 1.5, 4.5), shape("b", 19.5, 1.5, 22.5, 4.5, 4.5, 22.5, 1.5, 19.5), shape("j", 12, 9, 15, 12, 12, 15, 9, 12)]
    icons["beam_top_side"] = [quad("a", 1.5, 15, 22.5, 22), quad("w", 9.5, 15, 14.5, 19.5), quad("b", 7, 1.5, 17, 8), quad("j", 9.5, 8, 14.5, 13)]
    icons["beam_bridle"] = [quad("b", 7, 1.5, 17, 9), quad("j", 7, 9, 9.8, 22), quad("j", 14.2, 9, 17, 22), quad("a", 1.5, 14, 7, 20), quad("a", 9.8, 14, 14.2, 20), quad("a", 17, 14, 22.5, 20)]
    icons["beam_column_head"] = [shape("j", 2.5, 8.5, 9, 8.5, 9, 16), shape("j", 21.5, 8.5, 15, 8.5, 15, 16), quad("b", 9, 8.5, 15, 22.5), quad("a", 1.5, 2.5, 22.5, 8.5)]
    icons["beam_compendium"] = [shape("a", 12, 5, 2, 2.5, 2, 19, 12, 21.5), shape("b", 12, 5, 22, 2.5, 22, 19, 12, 21.5), shape("line", 4.8, 8, 9.2, 9.1), shape("line", 4.8, 12, 9.2, 13.1), shape("j", 14.6, 9.6, 19.4, 8.4, 19.4, 14.4, 14.6, 15.6)]
    icons["beam_todo"] = [quad("a", 3.5, 1.5, 20.5, 22.5)] + [quad("j", 6.4, y, 9.4, y + 3) for y in (4.6, 10.5, 16.4)] + [shape("line", 12, y + 1.5, 17.5, y + 1.5) for y in (4.6, 10.5, 16.4)]
    icons["beam_keyed_box"] = [quad("a", 1.5, 9, 21, 15), quad("b", 7, 1.5, 12, 22.5), shape("j", 14.5, 4.5, 18, 4.5, 17.2, 19.5, 14.5, 19.5)]
    icons["beam_end_to_end"] = [shape("a", 1.5, 7, 16.5, 7, 16.5, 9.5, 7.5, 14.5, 7.5, 17, 1.5, 17), shape("b", 16.5, 7, 22.5, 7, 22.5, 17, 7.5, 17, 7.5, 14.5, 16.5, 9.5), shape("j", 11, 9.4, 14.4, 9.4, 13, 14.6, 9.6, 14.6)]
    icons["beam_butt"] = [quad("a", 12, 8.5, 22.5, 15.5), quad("b", 5, 1.5, 12, 22.5), quad("j", 1.5, 9.4, 16.5, 11.2), quad("j", 1.5, 12.8, 16.5, 14.6)]
    icons["beam_pins"] = [quad("a", 1.5, 8, 22.5, 16)] + [quad("j", x, 4, x + 2.6, 13) for x in (4, 10.7, 17.4)]

    # templates
    fold = list(zip((1.5, 6.75, 12.0, 17.25, 22.5), (12.5, 4.0, 7.5, 4.0, 12.5)))
    icons["cylindrical_folding"] = band("baba", fold, [(x, y + 10.0) for x, y in fold])
    crease = [(9, 1.5), (11.6, 7), (12.6, 12), (11.6, 17), (9, 22.5)]
    icons["developable_folding"] = [("a", crease + [(1.5, 19.5), (3.4, 12), (1.5, 4.5)]), ("b", crease + [(19, 21), (22.5, 12), (19, 3)])]
    icons["cross"] = [shape("a", 1.5, 8, 12, 4, 12, 18, 1.5, 22), shape("b", 12, 4, 22.5, 8, 22.5, 22, 12, 18), shape("j", 7, 8.4, 17, 8.4, 17, 11, 7, 11), shape("j", 7, 14, 17, 14, 17, 16.6, 7, 16.6)]
    icons["module"] = [quad("a", 3.5, 11, 20.5, 22), shape("b", 1.5, 11, 12, 2, 22.5, 11), quad("j", 9.6, 15, 14.4, 22)]
    icons["grid"] = [quad("a", x, 3, x + 3, 22.5) for x in (1.5, 10.5, 19.5)] + [quad("b", 1.5, 1.5, 22.5, 5), quad("b", 1.5, 11, 22.5, 14.5)]
    icons["floor"] = [shape("b", 1.5, 4, 22.5, 4, 22.5, 12, 19, 12, 16, 9, 12, 8, 8, 9, 5, 12, 1.5, 12), quad("a", 1.5, 12, 5, 22.5), quad("a", 19, 12, 22.5, 22.5), shape("j", 10.4, 4, 13.6, 4, 12.6, 8, 11.4, 8)]
    icons["truss"] = [shape("line", 12, 5, 12, 18), shape("line", 12, 18, 6.6, 12), shape("line", 12, 18, 17.4, 12), shape("b", 12, 2, 22.5, 18.5, 19, 18.5, 12, 7, 5, 18.5, 1.5, 18.5), quad("a", 1.5, 18.5, 22.5, 22), quad("j", 10.4, 17, 13.6, 20.2)]
    icons["vault"] = band("abjba", arc(12, 13, 10.5, 180, 0, 5), arc(12, 13, 5.5, 180, 0, 5)) + [quad("b", 1.5, 13, 6.5, 22.5), quad("b", 17.5, 13, 22.5, 22.5)]
    icons["plaited"] = [quad("b", 1.5, 4, 22.5, 9), quad("b", 1.5, 15, 22.5, 20), quad("a", 4, 1.5, 9, 22.5), quad("a", 15, 1.5, 20, 22.5), quad("b", 13.5, 4, 22.5, 9), quad("b", 1.5, 15, 10.5, 20)]
    outside = [(3.5, 22.5), (1.5, 11), (12, 2), (22.5, 11), (20.5, 22.5)]
    inside = [(7, 22.5), (6, 12.6), (12, 7.4), (18, 12.6), (17, 22.5)]
    icons["vidy"] = band("abba", outside, inside)
    icons["templates"] = PLUGIN["diamond_mesh"]
    icons["examples"] = PLUGIN["install"]
    return icons


# ------------------------------------------------------------------ the sprite


def drawn(part: tuple) -> str:
    """A shape as SVG."""
    points = part[1]
    if isinstance(points, str):
        numbers = [float(value) for value in points.split()]
        points = list(zip(numbers[::2], numbers[1::2]))
    listed = " ".join(f"{x:.2f} {y:.2f}" for x, y in points)
    if part[0] == "line":
        return f'<polyline fill="none" points="{listed}"/>'
    if part[0] == "thick":
        return f'<polyline fill="none" stroke-width="2" points="{listed}"/>'
    if part[0] == "solid":
        return f'<polygon fill="#000" stroke="none" points="{listed}"/>'
    return f'<polygon fill="{FILLS[part[0]]}" points="{listed}"/>'


def main() -> None:
    """Writes the sprite."""
    icons = {**PLUGIN, **new_icons()}
    symbols = []
    for name, parts in sorted(icons.items()):
        inside = "".join(drawn(part) for part in parts)
        symbols.append(f'<symbol id="{name}" viewBox="0 0 24 24"><g stroke="#000" stroke-width="1.1" stroke-linecap="square" stroke-linejoin="miter" stroke-miterlimit="4">{inside}</g></symbol>')
    (WOOD / "docs/doxygen/icons.svg").write_text('<svg xmlns="http://www.w3.org/2000/svg">\n' + "\n".join(symbols) + "\n</svg>\n")
    print(len(icons), "icons")


if __name__ == "__main__":
    main()
