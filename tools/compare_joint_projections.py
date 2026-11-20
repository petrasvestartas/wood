"""A sheet per joint of the beam joint library that puts the book's projections next to ours: the bottom of the record's left
page from the Joinery Compendium scans above, and below it the member and the carrier on the corner the book draws (the inclined one
for the angled chapter or a record without a square one, else the square one), each alone in three orthographic views, visible edges solid and hidden edges dashed as the book draws them.

    python3 tools/compare_joint_projections.py <figures folder> <out folder> [design ...]

The figures folder holds the book's spreads as pp<left>-<right>.png (pp0420-0421.png); the scene is the one
build/element_joint_beam_library writes, run first. Every design of the library when none is named.
"""

import json
import pathlib
import subprocess
import sys

from PIL import Image, ImageDraw

WOOD = pathlib.Path(__file__).resolve().parents[1]
PYTHON = WOOD.parent / "session/session_py/.venv/bin/python"

# Each element's faces (an outer loop then its holes) and edges, in world coordinates, written by session_py; an element no
# joint cut is a plain mesh, its polygons the faces.
FACES = r"""
import json, sys
from session_py.session import Session
scene = Session.pb_load(sys.argv[1])
names = set(sys.argv[2:])
out = {}
for element in scene.objects.elements:
    if element.name not in names:
        continue
    brep = element.geometry
    if not hasattr(brep, "m_faces"):
        vertices, polygons = brep.to_vertices_and_faces()
        rings = [[[vertices[i][0], vertices[i][1], vertices[i][2]] for i in polygon] for polygon in polygons]
        out[element.name] = {"faces": [[ring] for ring in rings], "edges": [[ring[k - 1], ring[k]] for ring in rings for k in range(len(ring))]}
        continue
    points = [[v.point[0], v.point[1], v.point[2]] for v in brep.m_vertices]
    def loop(wire):
        pairs = [(brep.m_edges[ref.index].start_vertex, brep.m_edges[ref.index].end_vertex) for ref in brep.m_wires[wire.index].edges]
        ring = [pairs[0][0], pairs[0][1]]
        pairs = pairs[1:]
        while pairs:
            k = next(k for k, pair in enumerate(pairs) if ring[-1] in pair)
            a, b = pairs.pop(k)
            ring.append(b if a == ring[-1] else a)
        return [points[i] for i in ring[:-1]]
    faces = [[loop(wire) for wire in face.wires] for face in brep.m_faces]
    edges = [[points[edge.start_vertex], points[edge.end_vertex]] for edge in brep.m_edges]
    out[element.name] = {"faces": faces, "edges": edges}
print(json.dumps(out))
"""

# view name: the direction towards the viewer and the two world axes laid along the screen's x and y
VIEWS = {"top": ((0, 0, 1), 0, 1), "front": ((0, -1, 0), 0, 2), "side": ((1, 0, 0), 1, 2)}
SCALE = 0.6
CELL = (300, 300)


def sub(a: list, b: list) -> list:
    return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]


def dot(a: list, b: list) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a: list, b: list) -> list:
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def inside(point: tuple, ring: list) -> bool:
    """Whether a 2D point is inside a 2D ring, by crossings."""
    x, y = point
    result = False
    for k in range(len(ring)):
        (x1, y1), (x2, y2) = ring[k], ring[k - 1]
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
            result = not result
    return result


def hidden(point: list, toward: tuple, faces: list) -> bool:
    """Whether a face of the element lies between the point and the viewer."""
    start = [point[k] + toward[k] * 0.05 for k in range(3)]
    for loops in faces:
        outer = loops[0]
        normal = [0.0, 0.0, 0.0]
        for k in range(len(outer)):
            normal = [normal[j] + c for j, c in enumerate(cross(outer[k - 1], outer[k]))]
        if abs(dot(normal, toward)) < 1e-9 * max(1.0, dot(normal, normal) ** 0.5):
            continue
        t = dot(sub(outer[0], start), normal) / dot(toward, normal)
        if t <= 1e-6:
            continue
        hit = [start[k] + toward[k] * t for k in range(3)]
        u = sub(outer[1], outer[0])
        v = cross(normal, u)
        flat = lambda q: (dot(sub(q, outer[0]), u), dot(sub(q, outer[0]), v))
        at = flat(hit)
        if inside(at, [flat(q) for q in outer]) and not any(inside(at, [flat(q) for q in hole]) for hole in loops[1:]):
            return True
    return False


def draw(element: dict, view: str) -> Image.Image:
    """The element in one view at the one scale of every cell, centred on its cut (its vertices that are not corners of its
    bounds): edges split where they pass behind a face, the hidden parts dashed."""
    toward, i, j = VIEWS[view]
    xs = [p[i] for e in element["edges"] for p in e]
    ys = [p[j] for e in element["edges"] for p in e]
    scale = SCALE
    corners = [p for e in element["edges"] for p in e]
    lows = [min(p[k] for p in corners) for k in range(3)]
    highs = [max(p[k] for p in corners) for k in range(3)]
    cut = [p for p in corners if sum(abs(p[k] - lows[k]) < 1e-3 or abs(p[k] - highs[k]) < 1e-3 for k in range(3)) < 3] or corners
    cx, cy = sum(p[i] for p in cut) / len(cut), sum(p[j] for p in cut) / len(cut)
    image = Image.new("RGB", CELL, "white")
    pen = ImageDraw.Draw(image)
    screen = lambda p: (CELL[0] / 2 + (p[i] - cx) * scale, CELL[1] / 2 - (p[j] - cy) * scale)
    visible = []
    for a, b in element["edges"]:
        samples = max(2, int(((sum((a[k] - b[k]) ** 2 for k in range(3))) ** 0.5) / 4.0))
        points = [[a[k] + (b[k] - a[k]) * s / samples for k in range(3)] for s in range(samples + 1)]
        for s in range(samples):
            middle = [(points[s][k] + points[s + 1][k]) / 2 for k in range(3)]
            segment = (screen(points[s]), screen(points[s + 1]))
            if hidden(middle, toward, element["faces"]):
                if s % 2 == 0:
                    pen.line(segment, fill=(120, 120, 120), width=1)
            else:
                visible.append(segment)
    for segment in visible:
        pen.line(segment, fill="black", width=2)
    return image


def book(figures: pathlib.Path, page: int) -> Image.Image:
    """The bottom of the left page of the record's spread, where the book draws its projections."""
    spread = Image.open(figures / f"pp{page:04d}-{page + 1:04d}.png").convert("RGB")
    width, height = spread.size
    return spread.crop((0, int(height * 0.45), width // 2, int(height * 0.97)))


def main(arguments: list) -> None:
    """Writes <out>/<design>.png per design."""
    figures, out, names = pathlib.Path(arguments[0]), pathlib.Path(arguments[1]), arguments[2:]
    out.mkdir(parents=True, exist_ok=True)
    listing = subprocess.run([str(WOOD / "build/element_joint_beam_library")], cwd=WOOD, check=True, capture_output=True, text=True)
    designs = names or [line.split()[1] for line in listing.stdout.splitlines() if line.startswith("DESIGN ")]
    pages, square = {}, set()
    for chapter in sorted((WOOD / "data/joints").glob("*.yaml")):
        record = None
        for line in chapter.read_text().splitlines():
            if line and line[0].islower() and line.endswith(":"):
                record = line[:-1]
            elif line.startswith("  page:"):
                pages[record] = int(line.split('"')[1])
            elif line.startswith("  corners:") and "square" in line and not (chapter.stem == "angled" and "inclined" in line):
                square.add(record)
    elements = {}
    for design in designs:
        corner = "square" if design in square else "inclined"
        elements[design] = [f"{design}_{corner}_member", f"{design}_{corner}_carrier"]
    names = [name for pair in elements.values() for name in pair]
    written = subprocess.run([str(PYTHON), "-c", FACES, str(WOOD / "data/output/pb/live.pb"), *names], check=True, capture_output=True, text=True)
    geometry = json.loads(written.stdout.strip().splitlines()[-1])
    for design, pair in elements.items():
        page = book(pathlib.Path(figures), pages[design])
        page = page.resize((CELL[0] * 3, int(page.height * CELL[0] * 3 / page.width)))
        sheet = Image.new("RGB", (CELL[0] * 3, page.height + CELL[1] * 2 + 40), "white")
        sheet.paste(page, (0, 0))
        pen = ImageDraw.Draw(sheet)
        for row, name in enumerate(pair):
            y = page.height + 20 + row * CELL[1]
            for column, view in enumerate(VIEWS):
                sheet.paste(draw(geometry[name], view), (column * CELL[0], y))
                pen.text((column * CELL[0] + 4, y + 2), f"{name.split('_')[-1]} {view}", fill=(200, 60, 40))
        sheet.save(out / f"{design}.png")
        print(out / f"{design}.png")


if __name__ == "__main__":
    main(sys.argv[1:])
