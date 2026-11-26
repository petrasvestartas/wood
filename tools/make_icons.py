"""The icons of the docs, docs/doxygen/icons.svg, drawn as the Rhino plugin's (wood_nano/plugin_rhino/plugin/icons) are: a black
outline a unit wide with round ends over light grey faces, in a loose oblique view, black points where something is fixed. The
plugin's own keep their shapes; the others are laid out straight and then put into that hand (`by_hand`): tilted up to the right, the
far side a little smaller, no corner quite where a ruler would put it.

    python3 tools/make_icons.py"""

import hashlib
import math
import pathlib

WOOD = pathlib.Path(__file__).resolve().parents[1]
FILLS = {"a": "#ececec", "b": "#cfcfcf", "c": "#b2b2b2", "g": "#e4e4e4", "k": "#000000", "n": "none"}


def shape(fill: str, *numbers: float) -> tuple:
    """A polygon by its numbers, x and y in turn."""
    return (fill, list(zip(numbers[::2], numbers[1::2])))


def quad(fill: str, x0: float, y0: float, x1: float, y1: float) -> tuple:
    """A rectangle by two corners."""
    return shape(fill, x0, y0, x1, y0, x1, y1, x0, y1)


def line(*numbers: float) -> tuple:
    """An open line."""
    return ("line", list(zip(numbers[::2], numbers[1::2])))


def points(*numbers: float) -> list:
    """Black points."""
    return [("point", [place]) for place in zip(numbers[::2], numbers[1::2])]


def arc(x: float, y: float, rx: float, ry: float, start: float, end: float, count: int) -> list:
    """Points of an arc, degrees, up is negative y."""
    angles = [math.radians(start + (end - start) * k / count) for k in range(count + 1)]
    return [(x + rx * math.cos(angle), y - ry * math.sin(angle)) for angle in angles]


def band(fills: str, outer: list, inner: list) -> list:
    """The quads between two polylines."""
    return [(fills[k % len(fills)], [outer[k], outer[k + 1], inner[k + 1], inner[k]]) for k in range(len(outer) - 1)]


def by_hand(name: str, parts: list, tilt: float = 1.5, far: float = 0.11, loose: float = 0.34) -> list:
    """Shapes laid out straight, put into the plugin's hand; a corner two shapes share stays shared."""

    def moved(place: tuple) -> tuple:
        u, v = (place[0] - 12.0) / 12.0, (place[1] - 12.0) / 12.0
        noise = hashlib.md5(f"{name} {round(place[0] * 2)} {round(place[1] * 2)}".encode()).digest()
        off = [(value / 255.0 - 0.5) * 2.0 * loose for value in noise[:2]]
        return (12.0 + 11.2 * u + off[0] + 0.35 * v, 12.0 + 11.2 * v * (1.0 - far * u) - tilt * u + off[1])

    parts = [(fill, [moved(place) for place in places]) for fill, places in parts]

    # in the middle of the grid, all of a size; a point reaches two units round it
    reach = [(x + dx, y + dy) for fill, places in parts for x, y in places for dx, dy in ([(-2, -2), (2, 2)] if fill == "point" else [(0, 0)])]
    low, high = ([pick(place[k] for place in reach) for k in (0, 1)] for pick in (min, max))
    scale = 21.6 / max(high[0] - low[0], high[1] - low[1])
    return [(fill, [tuple(12.0 + (place[k] - (low[k] + high[k]) / 2.0) * scale for k in (0, 1)) for place in places]) for fill, places in parts]


# ------------------------------------------------------------------ the plugin's, their points as they are

PLUGIN = {
    "chevron": [shape("b", 10.2, 22.24, 2.91, 17.57, 3.99, 2.43, 11.86, 5.01), shape("a", 19.48, 19.89, 10.2, 22.24, 11.86, 5.01, 21.09, 4.08)],
    "connectors": [
        line(2.25, 4.52, 21, 21.86), shape("a", 11.15, 16.31, 11.72, 22.72, 18.65, 15.06, 18.63, 7.86), shape("a", 4.52, 10.76, 5.09, 17.17, 12.36, 9.18, 12.14, 2.13),
        line(20.32, 21.23, 15.08, 15.99), line(11.63, 13.19, 8.76, 10.32),
    ],
    "diamond_mesh": [
        shape("c", 11.98, 11.88, 12.06, 3.06, 22.6, 1.06), shape("a", 12.07, 11.9, 11.99, 3.09, 1.45, 1.09), shape("a", 12.1, 12.02, 11.94, 20.84, 1.4, 22.76),
        shape("c", 12, 12, 12.01, 20.81, 22.55, 22.89), shape("b", 1.45, 1.09, 1.4, 22.76, 11.98, 11.88), shape("a", 22.6, 1.06, 22.55, 22.89, 12.1, 12.02),
    ],
    "insertion_direction": [("thick", [(21.41, 8.5), (16.42, 8.5)]), ("thick", [(7.91, 17.94), (2.91, 17.94)]), shape("g", 12.3, 4.4, 21.06, 13.16, 12.3, 21.92, 3.54, 13.16)] + points(15.83, 8.5, 8.3, 17.94),
    "install": [line(12.11, 3.4, 12.11, 17.93), line(7.69, 12.08, 12.11, 17.93, 16.59, 12.08), ("path", "M21.35,15.71v2.03c0,1.58-1.28,2.85-2.85,2.85H5.5c-1.58,0-2.85-1.28-2.85-2.85v-2.03")],
    "joint_type": [shape("g", 12, 3.24, 20.76, 12, 12, 20.76, 3.24, 12)] + points(16.66, 7.34, 7.34, 7.34, 7.34, 16.78, 16.66, 16.78),
    "reciprocal_move": [
        shape("a", 19.92, 3.99, 19.96, 9.54, 2.72, 11.86, 2.53, 6.24), shape("b", 18.75, 11.91, 18.78, 19.37, 15.14, 9.07, 14.96, 1.56),
        shape("b", 10.65, 12.83, 10.68, 20.28, 7.05, 9.99, 6.86, 2.47), shape("a", 22.17, 14.68, 22.2, 20.23, 4.97, 22.55, 4.78, 16.93),
    ],
    "reciprocal_rotation": [shape("b", 14.52, 8.56, 14.4, 15.65, 3.78, 8.23, 3.75, 1.07), shape("a", 1.39, 15.83, 1.26, 22.93, 9.15, 12.41, 9.12, 5.25), shape("a", 22.79, 7.29, 22.67, 14.38, 5.51, 16.93, 5.48, 9.77)],
    "reflex_fold": [shape("b", 14.38, 4.02, 8.33, 2.06, 15.99, 16.79, 22.5, 18.24), shape("a", 9.85, 21.94, 3.19, 20.25, 8.11, 2.06, 14.38, 4.02)],
    "solver": [("solid", [(0, 6.63), (6.01, 12.73), (11.94, 6.77), (18.01, 12.73), (24, 6.61), (24, 0), (0, 0)]), ("solid", [(0, 12.74), (6, 18.72), (12.07, 12.78), (18, 18.71), (24, 12.73), (24, 24), (0, 24)])],
    "translation_shell": [
        shape("b", 10.91, 11.57, 2.76, 11.09, 5.91, 2.18, 13.9, 2.49), shape("a", 20.71, 13.14, 10.91, 11.57, 13.9, 2.49, 23.14, 4.39),
        shape("c", 9.36, 21.21, 1.49, 19.83, 2.76, 11.09, 10.91, 11.35), shape("a", 19.32, 21.71, 9.36, 21.21, 10.91, 11.57, 20.71, 13.14),
    ],
}


# ------------------------------------------------------------------ the others, laid out straight

DRAWN = {
    # chapters
    "architecture": [shape("b", 1, 17, 12, 11, 23, 17, 12, 23), shape("a", 1, 8, 12, 2, 23, 8, 12, 14)],
    "elements": [quad("a", 3, 2, 9, 16.5), quad("b", 12, 8, 20, 16.5), shape("a", 1, 16.5, 23, 16.5, 21, 22, 3, 22)],
    "classes": [line(7.5, 2, 3, 2, 3, 22, 7.5, 22), line(16.5, 2, 21, 2, 21, 22, 16.5, 22), quad("b", 8.5, 8, 15.5, 16)],
    # nodes, edges, faces
    "block": [shape("a", 1.5, 8, 14, 8, 15.5, 22.5, 1, 22.5), shape("b", 14, 8, 21, 2.5, 23, 16, 15.5, 22.5), shape("a", 1.5, 8, 9, 2, 21, 2.5, 14, 8)],
    "support": [quad("b", 10, 6, 14, 16), shape("a", 4, 2, 20, 2, 21, 6.5, 3, 6.5), shape("a", 2, 15.5, 22, 15.5, 23, 21.5, 1, 21.5)] + points(5, 18.5, 19, 18.5),
    "edge_straight": [shape("a", 1, 10, 17.5, 10, 17.5, 18, 1, 18), shape("a", 1, 10, 6, 5, 23, 5, 17.5, 10), shape("c", 17.5, 10, 23, 5, 23, 12.5, 17.5, 18)],
    "edge_curved": [("a", arc(12, 21, 11, 14, 172, 8, 7) + arc(12, 21, 8, 10, 8, 172, 7)), ("b", arc(12, 21, 8, 10, 172, 8, 7) + arc(12, 21, 4.6, 6, 8, 172, 7))],
    "edge_variable": [shape("a", 1, 6, 17.5, 11, 17.5, 18, 1, 21.5), shape("a", 1, 6, 6, 1.5, 23, 6.5, 17.5, 11), shape("c", 17.5, 11, 23, 6.5, 23, 13, 17.5, 18)],
    "edge_raw": [shape("a", 1.5, 13, 9.5, 10.5, 14, 3.5, 18, 5.5, 14.5, 11, 21, 10.5, 21, 16.5, 9.5, 17.5, 1.5, 19.5), ("c", arc(21, 13.5, 2.2, 3.0, 0, 360, 10))],
    "face_planar": [shape("a", 1.5, 15, 8, 4.5, 23, 5, 16.5, 15.5), shape("b", 1.5, 15, 16.5, 15.5, 16.5, 19, 1.5, 18.5), shape("c", 16.5, 15.5, 23, 5, 23, 8.5, 16.5, 19)],
    "face_curved": [shape("c", 1, 12, 5.5, 5, 5.5, 14.5, 1, 22), shape("a", 5.5, 5, 12, 2.5, 12, 12, 5.5, 14.5), shape("a", 12, 2.5, 18.5, 5, 18.5, 14.5, 12, 12), shape("b", 18.5, 5, 23, 12, 23, 22, 18.5, 14.5)],
    "cut_plane": [shape("a", 1, 8.5, 9, 8.5, 11.5, 16.5, 1, 16.5), shape("a", 13.5, 8.5, 23, 8.5, 23, 16.5, 16.5, 16.5), shape("b", 7, 1.5, 12.5, 4, 18, 22.5, 12.5, 20)],
    "pin": [shape("a", 3, 8, 20, 8, 20, 16, 3, 16), ("b", arc(20, 12, 2.6, 4, 0, 360, 10)), ("line", arc(3, 12, 2.4, 4, 90, 270, 6))],
    # joints of plates
    "plate_top_side": [shape("a", 1, 17, 23, 17, 21, 23, 3, 23), shape("k", 6.5, 18.6, 9.5, 18.6, 9.2, 20.8, 6.2, 20.8), shape("k", 14.5, 18.6, 17.5, 18.6, 17.8, 20.8, 14.8, 20.8), shape("b", 4, 1.5, 20, 1.5, 20, 10, 17.5, 10, 17.5, 14, 14.5, 14, 14.5, 10, 9.5, 10, 9.5, 14, 6.5, 14, 6.5, 10, 4, 10)],
    "plate_side_side_inplane": [quad("b", 8, 3, 23, 21), shape("a", 1, 3, 9, 3, 9, 6.5, 15, 5, 15, 10.5, 9, 9, 9, 15, 15, 13.5, 15, 19, 9, 17.5, 9, 21, 1, 21)],
    "plate_side_side_outplane": [shape("b", 23, 4, 13, 2, 13, 5.5, 10, 5.5, 10, 10, 13, 10, 13, 14.5, 10, 14.5, 10, 19, 13, 19, 13, 23, 23, 20), shape("a", 1, 4, 10, 2, 10, 5.5, 13, 5.5, 13, 10, 10, 10, 10, 14.5, 13, 14.5, 13, 19, 10, 19, 10, 23, 1, 20)],
    "plate_side_side_rotated": [shape("a", 1, 6, 12, 2, 12, 18, 1, 22), shape("b", 12, 2, 23, 6, 23, 22, 12, 18), shape("k", 8.5, 9, 12, 7.6, 15.5, 9, 15.5, 14.2, 12, 12.8, 8.5, 14.2)],
    "plate_cross": [shape("b", 1, 9, 23, 9, 23, 17, 1, 17), shape("a", 8, 1.5, 15, 1.5, 15, 12.5, 8, 12.5), shape("a", 9, 17, 14, 17, 14, 22.5, 9, 22.5)],
    "plate_top_top": [shape("a", 1, 17, 7, 12.5, 23, 12.5, 17, 17), shape("b", 1, 17, 17, 17, 17, 20.5, 1, 20.5), shape("c", 17, 17, 23, 12.5, 23, 16, 17, 20.5), line(6, 9, 6, 15.5), line(12, 9, 12, 15.5), line(18, 8, 18, 14), shape("a", 1, 7.5, 7, 3, 23, 3, 17, 7.5), shape("b", 1, 7.5, 17, 7.5, 17, 10.5, 1, 10.5), shape("c", 17, 7.5, 23, 3, 23, 6, 17, 10.5)],
    "plate_border": [shape("b", 1.5, 2, 22.5, 2, 17, 7, 7, 7), shape("a", 22.5, 2, 22.5, 22, 17, 17, 17, 7), shape("b", 22.5, 22, 1.5, 22, 7, 17, 17, 17), shape("a", 1.5, 22, 1.5, 2, 7, 7, 7, 17), quad("a", 7, 7, 17, 17)],
    # joints of beams
    "beam": [quad("b", 8.5, 8.5, 15.5, 23), quad("a", 1, 2, 23, 8.5)] + points(12, 5.2),
    "beam_cross": [shape("b", 5, 1, 23, 18.5, 19, 23, 1, 5.5), shape("a", 19, 1, 23, 5.5, 5, 23, 1, 18.5)] + points(12, 12),
    "beam_top_side": [quad("a", 1, 15, 23, 22.5), quad("k", 9.6, 15, 14.4, 18.6), shape("b", 7, 1.5, 17, 1.5, 17, 8, 14.2, 8, 14.2, 12.5, 9.8, 12.5, 9.8, 8, 7, 8)],
    "beam_bridle": [quad("a", 1, 13.5, 23, 20), shape("b", 6, 1.5, 18, 1.5, 18, 23, 14.5, 23, 14.5, 10, 9.5, 10, 9.5, 23, 6, 23)],
    "beam_column_head": [quad("b", 8.5, 8, 15.5, 23), shape("a", 2, 8, 8.5, 8, 8.5, 15.5), shape("a", 22, 8, 15.5, 8, 15.5, 15.5), quad("a", 1, 2, 23, 8)],
    "beam_compendium": [shape("a", 12, 5, 1.5, 2.5, 1.5, 19, 12, 22), shape("b", 12, 5, 22.5, 2.5, 22.5, 19, 12, 22), line(4.5, 8, 9, 9.2), line(4.5, 12.5, 9, 13.7)],
    "beam_todo": [quad("a", 3.5, 1.5, 20.5, 22.5), line(11, 6, 17, 6), line(11, 12, 17, 12), line(11, 18, 17, 18)] + points(7, 6, 7, 12, 7, 18),
    "beam_keyed_box": [quad("a", 1, 9, 23, 15.5), quad("b", 6, 1.5, 12, 22.5), shape("k", 15, 5, 18.6, 5, 18, 19.5, 15.4, 19.5)],
    "beam_end_to_end": [shape("a", 1, 7, 17.5, 7, 6.5, 17, 1, 17), shape("b", 23, 17, 6.5, 17, 17.5, 7, 23, 7)] + points(12, 12),
    "beam_butt": [quad("a", 12, 8, 23, 16), quad("b", 4.5, 1.5, 12, 22.5), ("thick", [(1, 12), (18, 12)])],
    "beam_pins": [quad("a", 1, 7, 23, 18)] + points(5.5, 12.5, 12, 12.5, 18.5, 12.5),
    # templates
    "cylindrical_folding": band("bab" + "a", [(1, 12), (6, 3.5), (12, 7), (18, 3.5), (23, 12)], [(1, 22), (6, 14), (12, 17.5), (18, 14), (23, 22)]),
    "developable_folding": [("a", [(9, 1.5), (11.4, 6), (12.6, 12), (11.6, 18), (9, 22.5), (1.5, 19.5), (3.6, 12), (1.5, 4.5)]), ("b", [(9, 1.5), (11.4, 6), (12.6, 12), (11.6, 18), (9, 22.5), (20, 21), (22.5, 12), (20, 3)])],
    "cross": [shape("a", 1, 6, 12, 2, 12, 18.5, 1, 22), shape("b", 12, 2, 23, 6, 23, 22, 12, 18.5), shape("c", 7.5, 7, 16.5, 7.5, 16.5, 10, 7.5, 9.5), shape("c", 7.5, 13, 16.5, 13.5, 16.5, 16, 7.5, 15.5)],
    "module": [shape("a", 2, 12, 14, 12, 14, 22.5, 2, 22.5), shape("b", 14, 12, 18, 4, 22.5, 11, 22.5, 20, 14, 22.5), shape("a", 1, 12, 5.5, 3.5, 18, 4, 14, 12), quad("k", 6, 16, 10, 22.5)],
    "grid": [line(2, 3, 2, 22.5), line(12, 3, 12, 22.5), line(22, 3, 22, 22.5), quad("a", 2, 3, 12, 12.5), quad("b", 12, 3, 22, 12.5)] + points(2, 3, 12, 3, 22, 3, 2, 12.5, 12, 12.5, 22, 12.5),
    "floor": [quad("b", 1.5, 11, 5.5, 22.5), quad("b", 18.5, 11, 22.5, 22.5), shape("a", 1.5, 3, 22.5, 3, 22.5, 11.5, 18, 11.5, 15, 9, 12, 8, 9, 9, 6, 11.5, 1.5, 11.5), line(12, 3, 12, 8)],
    "timber_floor": [quad("a", 1, 4, 23, 8), quad("b", 2, 8, 5, 18), quad("b", 10.5, 8, 13.5, 18), quad("b", 19, 8, 22, 18), quad("a", 1, 18, 23, 22)],
    "truss": [shape("a", 12, 3, 22, 20, 2, 20), line(12, 3, 12, 20), line(12, 20, 7, 11.5), line(12, 20, 17, 11.5)] + points(12, 3, 22, 20, 2, 20, 12, 20),
    "vault": band("abcba", arc(12, 13, 11, 11.5, 180, 0, 5), arc(12, 13, 5.6, 6, 180, 0, 5)) + [quad("b", 1, 13, 6.4, 22.5), quad("b", 17.6, 13, 23, 22.5)],
    "plaited": [quad("b", 1, 4, 23, 9.5), quad("b", 1, 14.5, 23, 20), quad("a", 4, 1, 9.5, 23), quad("a", 14.5, 1, 20, 23), quad("b", 12, 4, 23, 9.5), quad("b", 1, 14.5, 12, 20)],
    "vidy": band("abba", [(3.5, 22.5), (1, 10.5), (12, 1.5), (23, 10.5), (20.5, 22.5)], [(7.5, 22.5), (6, 12.6), (12, 7.5), (18, 12.6), (16.5, 22.5)]),
}


# ------------------------------------------------------------------ the sprite


def drawn(part: tuple) -> str:
    """A shape as SVG."""
    if part[0] == "path":
        return f'<path fill="none" d="{part[1]}"/>'
    listed = " ".join(f"{x:.2f} {y:.2f}" for x, y in part[1])
    if part[0] == "point":
        return f'<circle cx="{part[1][0][0]:.2f}" cy="{part[1][0][1]:.2f}" r="2" fill="#000" stroke="none"/>'
    if part[0] in ("line", "thick"):
        return f'<polyline fill="none" stroke-width="{1.1 if part[0] == "line" else 2}" points="{listed}"/>'
    if part[0] == "solid":
        return f'<polygon fill="#000" stroke="none" points="{listed}"/>'
    return f'<polygon fill="{FILLS[part[0]]}" points="{listed}"/>'


def main() -> None:
    """Writes the sprite."""
    icons = {**PLUGIN, **{name: by_hand(name, parts) for name, parts in DRAWN.items()}}
    icons["templates"], icons["examples"] = PLUGIN["diamond_mesh"], PLUGIN["install"]
    symbols = []
    for name, parts in sorted(icons.items()):
        inside = "".join(drawn(part) for part in parts)
        symbols.append(f'<symbol id="{name}" viewBox="-.6 -.6 25.2 25.2"><g stroke="#000" stroke-width="1.1" stroke-linecap="round" stroke-linejoin="round">{inside}</g></symbol>')
    (WOOD / "docs/doxygen/icons.svg").write_text('<svg xmlns="http://www.w3.org/2000/svg">\n' + "\n".join(symbols) + "\n</svg>\n")
    print(len(icons), "icons")


if __name__ == "__main__":
    main()
