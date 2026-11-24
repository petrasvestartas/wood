"""The drawing of the landing page: one tenon joint taken apart in an isometric view, written as docs/doxygen/hero.svg.

    python3 tools/render_hero.py

An upright with three tenons, the contact it makes with its base, and the base with the mortises cut. The classes are
styled by docs/doxygen/wood.css, so the drawing follows the scheme toggle; src/docs.md includes it in the page."""

import math
import pathlib

WOOD = pathlib.Path(__file__).resolve().parent.parent
LENGTH, THICKNESS, HEIGHT = 300.0, 52.0, 150.0  # the upright
TENONS = [(30.0, 90.0), (120.0, 180.0), (210.0, 270.0)]  # along the upright
DEPTH = 44.0  # of a tenon, the thickness of the base
BASE = (-40.0, 340.0, -84.0, 136.0)  # x from, x to, y from, y to
UPRIGHT_Z, CONTACT_Z, BASE_Z = 330.0, 130.0, 0.0  # the three levels, drawn apart


def project(x: float, y: float, z: float) -> tuple[float, float]:
    """The screen point of a model point, seen from (+, +, +)."""
    return (x - y) * math.cos(math.radians(30)), (x + y) * math.sin(math.radians(30)) - z


def path(points: list[tuple[float, float, float]]) -> str:
    """The closed path through model points."""
    return "M" + " L".join(f"{x:.1f} {y:.1f}" for x, y in (project(*point) for point in points)) + " Z"


def face(name: str, points: list[tuple[float, float, float]], extra: str = "") -> str:
    """A filled face of a css class."""
    return f'<path class="{name}" d="{path(points)}"{extra}/>'


def box(x0: float, x1: float, y0: float, y1: float, z0: float, z1: float, front: str, side: str) -> list[str]:
    """The two upright faces of a box that are seen: the one towards +y and the one towards +x."""
    return [
        face(side, [(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)]),
        face(front, [(x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1)]),
    ]


def top(x0: float, x1: float, y0: float, y1: float, z: float) -> list[tuple[float, float, float]]:
    """A rectangle at a height."""
    return [(x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z)]


def label(point: tuple[float, float, float], name: str, says: str) -> list[str]:
    """A leader from a model point out to the right, with the name of what it points at and what it is."""
    x, y = project(*point)
    end = 372.0
    return [
        f'<path class="leader" d="M{x:.1f} {y:.1f} H{end}"/>',
        f'<circle class="leader-end" cx="{x:.1f}" cy="{y:.1f}" r="3.5"/>',
        f'<text class="name" x="{end + 12}" y="{y + 1:.1f}">{name}</text>',
        f'<text class="says" x="{end + 12}" y="{y + 23:.1f}">{says}</text>',
    ]


def main() -> None:
    lines: list[str] = []

    # base: its sides, its top, then every mortise: the two walls seen through the opening, clipped by it
    x0, x1, y0, y1 = BASE
    lines += box(x0, x1, y0, y1, BASE_Z - DEPTH, BASE_Z, "side", "side")
    lines.append(face("face", top(x0, x1, y0, y1, BASE_Z)))
    for i, (a, b) in enumerate(TENONS):
        opening = top(a, b, 0.0, THICKNESS, BASE_Z)
        lines.append(f'<clipPath id="mortise-{i}"><path d="{path(opening)}"/></clipPath>')
        clip = f' clip-path="url(#mortise-{i})"'
        lines.append(face("cut", [(a, 0.0, BASE_Z), (a, THICKNESS, BASE_Z), (a, THICKNESS, BASE_Z - DEPTH), (a, 0.0, BASE_Z - DEPTH)], clip))
        lines.append(face("cut-side", [(a, 0.0, BASE_Z), (b, 0.0, BASE_Z), (b, 0.0, BASE_Z - DEPTH), (a, 0.0, BASE_Z - DEPTH)], clip))
        lines.append(f'<path class="guide" style="stroke-dasharray:none" d="{path(opening)}"/>')

    # contact: the face the two members share, with the guides it hangs between
    for x, y in ((0.0, THICKNESS), (LENGTH, 0.0), (LENGTH, THICKNESS)):
        a, b = project(x, y, BASE_Z), project(x, y, UPRIGHT_Z)
        lines.append(f'<path class="guide" d="M{a[0]:.1f} {a[1]:.1f} L{b[0]:.1f} {b[1]:.1f}"/>')
    lines.append(face("contact", top(0.0, LENGTH, 0.0, THICKNESS, CONTACT_Z)))

    # upright: its tenons from the back to the front, then its body
    for a, b in TENONS:
        lines += box(a, b, 0.0, THICKNESS, UPRIGHT_Z - DEPTH, UPRIGHT_Z, "cut", "cut-side")
    lines += box(0.0, LENGTH, 0.0, THICKNESS, UPRIGHT_Z, UPRIGHT_Z + HEIGHT, "face", "side")
    lines.append(face("face", top(0.0, LENGTH, 0.0, THICKNESS, UPRIGHT_Z + HEIGHT)))

    # labels
    lines += label((LENGTH, THICKNESS / 2, UPRIGHT_Z + HEIGHT / 2), "Elements", "the members, uncut")
    lines += label((LENGTH, THICKNESS / 2, CONTACT_Z), "Contacts", "where they touch")
    lines += label((x1, y0 + 30.0, BASE_Z), "Joints", "what they cut")

    title = "A tenon joint taken apart: the upright, the contact and the base"
    svg = f'<svg viewBox="-230 -420 760 660" role="img" aria-label="{title}">\n' + "\n".join(lines) + "\n</svg>\n"
    (WOOD / "docs/doxygen/hero.svg").write_text(svg)


main()
