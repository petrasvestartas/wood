"""The icons of the docs, docs/doxygen/icons.svg, one symbol each, a few flat shapes with black lines and square corners, filled
light grey, or black where they are what the page is about, and black points for nodes, pins and anchors. The Rhino plugin's icons
(wood_nano/plugin_rhino/plugin/icons) are among them.

    python3 tools/make_icons.py"""

import math
import pathlib

WOOD = pathlib.Path(__file__).resolve().parents[1]
FILLS = {"k": "#111111", "l": "#e2e2e2"}
LINE = 1.4


def quad(fill: str, x0: float, y0: float, x1: float, y1: float) -> tuple:
    """A rectangle by two corners."""
    return (fill, [(x0, y0), (x1, y0), (x1, y1), (x0, y1)])


def shape(fill: str, *numbers: float) -> tuple:
    """A polygon by its numbers, x and y in turn; "line" and "bar" are open, thin and thick."""
    return (fill, list(zip(numbers[::2], numbers[1::2])))


def points(*numbers: float) -> list:
    """Black points, x and y in turn."""
    return [("point", [place]) for place in zip(numbers[::2], numbers[1::2])]


def arc(x: float, y: float, radius: float, start: float, end: float, count: int) -> list:
    """Points of an arc as straight pieces, degrees, up is negative y."""
    angles = [math.radians(start + (end - start) * k / count) for k in range(count + 1)]
    return [(x + radius * math.cos(angle), y - radius * math.sin(angle)) for angle in angles]


def band(fills: str, outer: list, inner: list) -> list:
    """The quads between two polylines."""
    return [(fills[k % len(fills)], [outer[k], outer[k + 1], inner[k + 1], inner[k]]) for k in range(len(outer) - 1)]


def box(x0: float, y0: float, x1: float, y1: float, dx: float, dy: float) -> list:
    """A box seen from the front, its right side black."""
    return [quad("l", x0, y0, x1, y1), shape("l", x0, y0, x1, y0, x1 + dx, y0 + dy, x0 + dx, y0 + dy), shape("k", x1, y0, x1 + dx, y0 + dy, x1 + dx, y1 + dy, x1, y1)]


def icons() -> dict:
    """Every icon, its shapes back to front."""
    made = {}

    # the plugin's
    made["solver"] = [shape("k", 0, 6.63, 6.01, 12.73, 11.94, 6.77, 18.01, 12.73, 24, 6.61, 24, 0, 0, 0), shape("k", 0, 12.74, 6, 18.72, 12.07, 12.78, 18, 18.71, 24, 12.73, 24, 24, 0, 24)]
    made["chevron"] = [shape("l", 11, 23, 2, 17.6, 3.2, 1, 12.4, 4.2), shape("k", 22, 20.2, 11, 23, 12.4, 4.2, 23, 3)]
    made["connectors"] = [shape("line", 1, 3, 23, 23), shape("l", 4, 10.4, 4.6, 18, 13, 8.6, 12.7, 0.8), shape("l", 11, 15.8, 11.6, 23.4, 20, 14.2, 19.7, 6.2), shape("line", 8.6, 9.9, 11.4, 12.4), shape("line", 15.6, 16.3, 23, 23)]
    made["diamond_mesh"] = [
        shape("l", 1, 1, 12, 3.4, 12, 12), shape("k", 23, 1, 12, 3.4, 12, 12), shape("k", 1, 1, 12, 12, 1, 23), shape("l", 23, 1, 23, 23, 12, 12),
        shape("l", 1, 23, 12, 12, 12, 20.6), shape("k", 23, 23, 12, 20.6, 12, 12),
    ]
    made["insertion_direction"] = [shape("bar", 24, 8, 17, 8), shape("bar", 0, 17, 7, 17), shape("l", 12, 3.5, 21, 12.5, 12, 21.5, 3, 12.5)] + points(16.2, 8, 7.8, 17)
    made["install"] = [shape("k", 10, 1, 14, 1, 14, 9, 18.5, 9, 12, 16, 5.5, 9, 10, 9), shape("l", 1, 14, 5, 14, 5, 19, 19, 19, 19, 14, 23, 14, 23, 23, 1, 23)]
    made["joint_type"] = [shape("l", 12, 1, 23, 12, 12, 23, 1, 12)] + points(17.5, 6.5, 6.5, 6.5, 6.5, 17.5, 17.5, 17.5)
    made["reciprocal_move"] = [quad("l", 0.5, 3.5, 23.5, 8.5), quad("l", 0.5, 15.5, 23.5, 20.5), quad("k", 5, 8.5, 9, 23.5), quad("k", 15, 0.5, 19, 15.5)]
    made["reciprocal_rotation"] = [quad("l", 1, 5, 14, 10), quad("k", 14, 1, 19, 14), quad("l", 10, 14, 23, 19), quad("k", 5, 10, 10, 23)]
    made["reflex_fold"] = [shape("k", 14.6, 3.2, 8, 1, 16, 17.4, 23.4, 19), shape("l", 10, 23, 1.4, 20.8, 7.4, 1, 14.6, 3.2)]
    made["translation_shell"] = [shape("k", 12, 11.6, 2.6, 10.8, 6, 1, 14.6, 1.6), shape("l", 22.4, 13.2, 12, 11.6, 14.6, 1.6, 24, 3.8), shape("l", 10.4, 22.4, 1, 20.6, 2.6, 10.8, 12, 11.6), shape("k", 21, 23, 10.4, 22.4, 12, 11.6, 22.4, 13.2)]

    # chapters
    made["architecture"] = [shape("line", 12, 8, 12, 12.5), shape("line", 4.5, 16, 4.5, 12.5, 19.5, 12.5, 19.5, 16), quad("k", 8, 1, 16, 8), quad("l", 1, 16, 8, 23), quad("l", 16, 16, 23, 23)]
    made["elements"] = [quad("l", 3, 1, 10, 16), quad("k", 12, 8, 21, 16), quad("l", 1, 17, 23, 23)]
    made["templates"] = made["diamond_mesh"]
    made["examples"] = made["install"]
    made["classes"] = [shape("bar", 7.5, 1.5, 2, 1.5, 2, 22.5, 7.5, 22.5), shape("bar", 16.5, 1.5, 22, 1.5, 22, 22.5, 16.5, 22.5), quad("k", 8.5, 8.5, 15.5, 15.5)]

    # nodes, edges, faces
    made["block"] = box(1, 8, 16, 23, 7, -7)
    made["support"] = [quad("l", 4, 1, 20, 6), quad("k", 10, 6, 14, 16), quad("l", 0.5, 16, 23.5, 23)] + points(5, 19.5, 19, 19.5)
    made["edge_straight"] = box(0.5, 10.5, 18, 18.5, 5.5, -5.5)
    made["edge_curved"] = [("l", arc(12, 21, 11.5, 180, 0, 6) + arc(12, 21, 7.5, 0, 180, 6)), ("l", arc(12, 21, 7.5, 180, 0, 6) + arc(12, 21, 3.5, 0, 180, 6))]
    made["edge_variable"] = [shape("l", 0.5, 6, 18, 12, 18, 18.5, 0.5, 21.5), shape("l", 0.5, 6, 18, 12, 23.5, 7, 6, 1), shape("k", 18, 12, 23.5, 7, 23.5, 13.5, 18, 18.5)]
    made["face_planar"] = [shape("l", 0.5, 16, 8, 4, 23.5, 4, 16, 16), shape("l", 0.5, 16, 16, 16, 16, 20, 0.5, 20), shape("k", 16, 16, 23.5, 4, 23.5, 8, 16, 20)]
    made["face_curved"] = [shape("l", 0.5, 12, 6, 4.5, 6, 14.5, 0.5, 22), shape("l", 6, 4.5, 12, 2, 12, 12, 6, 14.5), shape("l", 12, 2, 18, 4.5, 18, 14.5, 12, 12), shape("k", 18, 4.5, 23.5, 12, 23.5, 22, 18, 14.5)]
    made["cut_plane"] = [quad("l", 0.5, 8, 23.5, 16), shape("k", 7, 0.5, 10, 0.5, 17, 23.5, 14, 23.5)]
    made["pin"] = [shape("k", 1, 10.5, 4, 8, 20, 8, 23, 10.5, 23, 13.5, 20, 16, 4, 16, 1, 13.5)]

    # joints of plates
    made["plate_top_side"] = [quad("l", 4, 1, 20, 9), quad("k", 6, 9, 10, 14), quad("k", 14, 9, 18, 14), shape("l", 0.5, 16, 6, 16, 6, 20, 10, 20, 10, 16, 14, 16, 14, 20, 18, 20, 18, 16, 23.5, 16, 23.5, 23.5, 0.5, 23.5)]
    made["plate_side_side_inplane"] = [quad("l", 0.5, 1, 23.5, 23), shape("line", 9, 1, 9, 23), shape("k", 9, 5.5, 15.5, 3.4, 15.5, 10, 9, 8), shape("k", 9, 16, 15.5, 14, 15.5, 20.6, 9, 18.5)]
    made["plate_side_side_outplane"] = [quad("l", 0.5, 0.5, 9, 23.5), quad("l", 15, 0.5, 23.5, 23.5), quad("k", 9, 4, 15, 9), quad("k", 9, 15, 15, 20)]
    made["plate_side_side_rotated"] = [shape("l", 0.5, 6, 12, 1, 12, 18.5, 0.5, 23.5), shape("l", 12, 1, 23.5, 6, 23.5, 23.5, 12, 18.5), shape("k", 7, 9.6, 12, 7.4, 17, 9.6, 17, 16.4, 12, 14.2, 7, 16.4)]
    made["plate_cross"] = [quad("l", 0.5, 8.5, 23.5, 15.5), quad("k", 8.5, 0.5, 15.5, 23.5)]
    made["plate_top_top"] = [quad("k", 5, 8, 8, 16), quad("k", 16, 8, 19, 16), quad("l", 0.5, 2, 23.5, 8), quad("l", 0.5, 16, 23.5, 22)]
    made["plate_border"] = [shape("l", 0.5, 0.5, 23.5, 0.5, 18, 6, 6, 6), shape("l", 23.5, 0.5, 23.5, 23.5, 18, 18, 18, 6), shape("l", 23.5, 23.5, 0.5, 23.5, 6, 18, 18, 18), shape("l", 0.5, 23.5, 0.5, 0.5, 6, 6, 6, 18), quad("k", 6, 6, 18, 18)]

    # joints of beams
    made["beam"] = [quad("l", 8, 9, 16, 23.5), quad("l", 0.5, 1, 23.5, 9)] + points(12, 5)
    made["beam_cross"] = [shape("l", 4.5, 0.5, 23.5, 19.5, 19.5, 23.5, 0.5, 4.5), shape("k", 19.5, 0.5, 23.5, 4.5, 4.5, 23.5, 0.5, 19.5)]
    made["beam_top_side"] = [quad("l", 7, 0.5, 17, 7), quad("k", 9.5, 7, 14.5, 13), shape("l", 0.5, 15, 9.5, 15, 9.5, 20, 14.5, 20, 14.5, 15, 23.5, 15, 23.5, 23.5, 0.5, 23.5)]
    made["beam_bridle"] = [quad("l", 0.5, 14, 23.5, 20), quad("l", 6, 0.5, 18, 9), quad("k", 6, 9, 9.5, 23.5), quad("k", 14.5, 9, 18, 23.5)]
    made["beam_column_head"] = [quad("l", 8.5, 8, 15.5, 23.5), shape("k", 1.5, 8, 8.5, 8, 8.5, 15), shape("k", 22.5, 8, 15.5, 8, 15.5, 15), quad("l", 0.5, 1, 23.5, 8)]
    made["beam_compendium"] = [shape("l", 12, 5, 0.5, 2, 0.5, 19, 12, 22), shape("l", 12, 5, 23.5, 2, 23.5, 19, 12, 22), shape("line", 4, 8, 8.5, 9.2), shape("line", 4, 12.5, 8.5, 13.7), shape("k", 15, 9.4, 20, 8.1, 20, 14, 15, 15.3)]
    made["beam_todo"] = points(4, 4, 4, 12, 4, 20) + [shape("bar", 10, y, 23, y) for y in (4, 12, 20)]
    made["beam_keyed_box"] = [quad("l", 0.5, 8.5, 23.5, 15.5), quad("l", 5, 0.5, 11.5, 23.5), shape("k", 14.5, 3, 19.5, 3, 18.5, 21, 14.5, 21)]
    made["beam_end_to_end"] = [shape("l", 0.5, 6, 17, 6, 7, 18, 0.5, 18), shape("l", 23.5, 18, 7, 18, 17, 6, 23.5, 6)] + points(12, 12)
    made["beam_butt"] = [quad("l", 12, 7, 23.5, 17), quad("l", 4, 0.5, 12, 23.5), quad("k", 0.5, 11, 19, 13)]
    made["beam_pins"] = [quad("l", 0.5, 6, 23.5, 18)] + points(5, 12, 12, 12, 19, 12)

    # templates
    fold = list(zip((0.5, 6.25, 12.0, 17.75, 23.5), (11, 1.5, 6, 1.5, 11)))
    made["cylindrical_folding"] = band("lklk", fold, [(x, y + 12.0) for x, y in fold])
    crease = [(9.5, 0.5), (12.2, 6.5), (13.2, 12), (12.2, 17.5), (9.5, 23.5)]
    made["developable_folding"] = [("l", crease + [(0.5, 20), (3, 12), (0.5, 4)]), ("k", crease + [(20, 22), (23.5, 12), (20, 2)])]
    made["cross"] = [shape("l", 0.5, 6, 12, 1, 12, 18.5, 0.5, 23.5), shape("l", 12, 1, 23.5, 6, 23.5, 23.5, 12, 18.5), quad("k", 6, 6.5, 18, 10.5), quad("k", 6, 13, 18, 17)]
    made["module"] = [quad("l", 3, 10.5, 21, 23.5), shape("k", 0.5, 10.5, 12, 0.5, 23.5, 10.5), quad("k", 9.5, 15.5, 14.5, 23.5)]
    made["grid"] = [quad("l", 1.5, 3, 22.5, 13), shape("line", 1.5, 23.5, 1.5, 3, 22.5, 3, 22.5, 23.5), shape("line", 12, 3, 12, 23.5), shape("line", 1.5, 13, 22.5, 13)] + points(1.5, 3, 12, 3, 22.5, 3, 1.5, 13, 12, 13, 22.5, 13)
    made["floor"] = [shape("k", 0.5, 2, 23.5, 2, 23.5, 11, 18.5, 11, 15, 8.4, 9, 8.4, 5.5, 11, 0.5, 11), quad("l", 0.5, 11, 5.5, 23.5), quad("l", 18.5, 11, 23.5, 23.5)]
    made["truss"] = [shape("l", 12, 3, 22, 19.5, 2, 19.5), shape("line", 12, 3, 12, 19.5), shape("line", 12, 19.5, 7, 11.2), shape("line", 12, 19.5, 17, 11.2)] + points(12, 3, 22, 19.5, 2, 19.5, 12, 19.5, 7, 11.2, 17, 11.2)
    ring, hole = arc(12, 13, 11.5, 180, 0, 5), arc(12, 13, 5.6, 180, 0, 5)
    made["vault"] = band("llkll", ring, hole) + [quad("l", 0.5, 13, 6.4, 23.5), quad("l", 17.6, 13, 23.5, 23.5)]
    made["plaited"] = [quad("l", 0.5, 4, 4, 10), quad("l", 10, 4, 23.5, 10), quad("l", 0.5, 14, 14, 20), quad("l", 20, 14, 23.5, 20), quad("k", 4, 0.5, 10, 14), quad("k", 4, 20, 10, 23.5), quad("k", 14, 0.5, 20, 4), quad("k", 14, 10, 20, 23.5)]
    made["vidy"] = band("llll", [(3, 23.5), (0.5, 10.5), (12, 0.5), (23.5, 10.5), (21, 23.5)], [(8, 23.5), (6.4, 13), (12, 8), (17.6, 13), (16, 23.5)]) + points(3.4, 11.8, 12, 4.2, 20.6, 11.8)
    return made


def drawn(part: tuple) -> str:
    """A shape as SVG."""
    listed = " ".join(f"{x:g} {y:g}" for x, y in part[1])
    if part[0] == "point":
        return f'<circle cx="{part[1][0][0]:g}" cy="{part[1][0][1]:g}" r="2.1" fill="#111111" stroke="none"/>'
    if part[0] in ("line", "bar"):
        return f'<polyline fill="none" stroke-width="{LINE if part[0] == "line" else 2.4}" points="{listed}"/>'
    bare = ' stroke="none"' if part[0] == "k" else ""
    return f'<polygon fill="{FILLS[part[0]]}"{bare} points="{listed}"/>'


def main() -> None:
    """Writes the sprite."""
    symbols = []
    for name, parts in sorted(icons().items()):
        inside = "".join(drawn(part) for part in parts)
        symbols.append(f'<symbol id="{name}" viewBox="-1 -1 26 26"><g stroke="#111111" stroke-width="{0 if name == "solver" else LINE}" stroke-linejoin="miter" stroke-miterlimit="2">{inside}</g></symbol>')
    (WOOD / "docs/doxygen/icons.svg").write_text('<svg xmlns="http://www.w3.org/2000/svg">\n' + "\n".join(symbols) + "\n</svg>\n")
    print(len(symbols), "icons")


if __name__ == "__main__":
    main()
