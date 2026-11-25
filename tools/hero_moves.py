"""How the floor of the landing page's film comes apart, and the proof that nothing runs into anything: every piece's moves,
written once for quarter 0 and turned for the other three, and a check that intersects every two solids that move against each
other at steps of at most a few millimetres.

    python3 tools/render_hero.py --check [step in mm]

The directions of the beds' lowest planks, the ribs and the screws are those ModuleSequence finds for the floor."""

import collections
import json
import pathlib
import re

import numpy as np

# out of the sequence of templates_floor_7_contacts_cantilevers: the lowest plank of each bed row, the ribs off the middle
# block, the screws of the inner ribs
PLANKS = [(0.94434, 0.11354, 0.30877), (0.68181, 0.68181, 0.26507), (0.11354, 0.94434, 0.30877)]
RIBS = [(0.56346, -0.82614, 0.0), (-0.82614, 0.56346, 0.0)]
SCREWS = [(0.82614, 0.56346, 0.0), (0.56346, 0.82614, 0.0)]
SQUARE = 0.70711


def up(height: float) -> tuple:
    """Straight up."""
    return (0.0, 0.0, height)


def shown(name: str) -> bool:
    """Whether an element is a piece: no cutter, no glued block (the column's solid has them), no drawing of a joint."""
    joint = name.startswith("connector") and not re.search(r"(part|pin)(_\d+)?$", name)
    return not (joint or "cutters" in name or "_head_" in name or name == "support")


def quarter(name: str) -> int:
    """The quarter of a piece: a connector's and a column plate's first number, a member's last."""
    first = re.match(r"(?:connector_[a-z_]*?|column_plate_)(\d)", name)
    return int(first[1]) if first else int(name.rsplit("_", 1)[1]) % 4


def turned(vector: tuple, quarters: int) -> np.ndarray:
    """A vector of quarter 0 in another quarter."""
    x, y, z = vector
    for _ in range(quarters):
        x, y = -y, x
    return np.array([x, y, z])


class Film:
    """Every piece's moves, each a vector made between two times with an eased start and end."""

    def __init__(self, names: list) -> None:
        self.moves = {name: [] for name in names}

        # beds: the planks rise one after another; the lowest slides out from under its block first, and back when it is clear
        for k in range(5, 0, -1):
            self.add(rf"beds_\d_{k}_\d", 0.12 * (5 - k), 1.1, up(2350.0 + 60.0 * k))
        for row, plank in enumerate(PLANKS):
            out = [420.0 * value for value in plank]
            self.add(rf"beds_{row}_0_\d", 0.75, 0.6, out)
            self.add(rf"beds_{row}_0_\d", 1.35, 0.85, up(2350.0 - out[2]))
            self.add(rf"beds_{row}_0_\d", 2.2, 0.4, (-out[0], -out[1], 0.0))

        # wedges between the quarters and to the oculus: their pins out along their axes, then both up
        self.add(r"connector_seam_wedge_\d_pin_\d", 1.5, 0.6, (-280.0, 0.0, 0.0))
        self.add(r"connector_oculus_wedge_\d_pin_\d", 1.5, 0.6, (-198.0, -198.0, 0.0))
        self.add(r"connector_(seam|oculus)_wedge_\d_(part|pin_\d)", 2.1, 1.0, up(1250.0))

        # quarters: apart
        members = r"(column|support)_\d|(beds_\d|outer_ribs|inner_ribs|wedges|inner_beams|tsections)_\d_\d|column_plate_\d_\d"
        pins = r"connector_(block_pins|column_plate)_\d_\d.*|connector_pins_(outer_rib|seam_beam|inner_rib)_.*"
        self.add(f"{members}|{pins}|" + r"connector_oculus_wedge_\d_(part|pin_\d)", 2.9, 1.0, (-620.0, -620.0, 0.0))

        # oculus: the ring up through the beds, its plate off it, then the wedges under the plate
        self.add(r"oculus_\d|connector_pins_ring_corner_\d_pin_\d", 3.7, 1.2, up(2700.0))
        self.add(r"oculus_8", 4.5, 0.8, up(420.0))
        self.add(r"oculus_[4-7]", 4.9, 0.6, up(180.0))

        # screws of the beams and pins of the column plates: out along their axes; beams and plates: up
        self.add(r"connector_pins_outer_rib_\d_0_pin_\d", 4.2, 0.55, (340.0, 0.0, 0.0))
        self.add(r"connector_pins_outer_rib_\d_1_pin_\d", 4.2, 0.55, (0.0, 340.0, 0.0))
        self.add(r"connector_pins_seam_beam_\d_0_pin_\d", 4.2, 0.55, (340.0 * SQUARE, -340.0 * SQUARE, 0.0))
        self.add(r"connector_pins_seam_beam_\d_1_pin_\d", 4.2, 0.55, (-340.0 * SQUARE, 340.0 * SQUARE, 0.0))
        for k, screw in enumerate(SCREWS):
            self.add(rf"connector_pins_inner_rib_\d_{k}_pin_\d", 4.65, 0.55, [340.0 * value for value in screw])
        self.add(r"connector_column_plate_\d_0_pin_\d", 4.2, 0.6, (0.0, -300.0, 0.0))
        self.add(r"connector_column_plate_\d_1_pin_\d", 4.2, 0.6, (-300.0, 0.0, 0.0))
        self.add(r"inner_beams_\d_\d|connector_pins_(outer_rib|seam_beam|inner_rib)_.*", 5.2, 0.8, up(1150.0))
        self.add(r"column_plate_\d_\d|connector_column_plate_\d_\d_pin_\d", 5.2, 0.8, up(1400.0))

        # ribs and blocks: off the column, then apart along the pins between them; the support off the column's foot
        self.add(r"(outer_ribs|inner_ribs|wedges|tsections)_\d_\d|connector_block_pins_.*", 5.5, 0.9, up(800.0))
        for side, (rib, block, sections, section) in enumerate(((0, 0, "[12]", 0), (1, 2, "[34]", 5))):
            way = np.array(RIBS[side])
            outward = np.array([0.0, -320.0, 0.0] if side == 0 else [-320.0, 0.0, 0.0])
            self.add(rf"inner_ribs_{rib}_\d|tsections_{sections}_\d", 6.3, 0.9, 260.0 * way)
            self.add(rf"wedges_{block}_\d|connector_block_pins_\d_{block}_.*", 6.3, 0.9, 520.0 * way)
            self.add(rf"outer_ribs_{side}_\d|tsections_{section}_\d", 6.3, 0.9, 520.0 * way + outward)
        self.add(r"support_\d", 5.8, 0.8, up(-420.0))

        self.length = max(start + lasts for moves in self.moves.values() for start, lasts, _ in moves)

    def add(self, pattern: str, start: float, lasts: float, vector: tuple) -> None:
        """Moves every piece whose whole name matches by a vector of quarter 0, turned into its quarter."""
        pieces = [name for name in self.moves if re.fullmatch(pattern, name)]
        if not pieces:
            raise ValueError(f"no piece matches {pattern}")
        for name in pieces:
            self.moves[name].append((start, lasts, turned(vector, quarter(name))))

    def displacement(self, name: str, time: float) -> np.ndarray:
        """Where a piece is at a time, from where it stands."""
        total = np.zeros(3)
        for start, lasts, vector in self.moves[name]:
            share = min(max((time - start) / lasts, 0.0), 1.0)
            total += vector * share * share * (3.0 - 2.0 * share)
        return total


def solids(path: pathlib.Path) -> dict:
    """The closed solid of every piece of a JSON of meshes (name, v, f); an open mesh is its hull less the pins and wedges in it."""
    import manifold3d

    found, opened = {}, []
    for piece in json.loads(path.read_text()):
        points = np.array(piece["v"])
        triangles = []
        for face in piece["f"]:
            corners = points[face]
            normal = sum(np.cross(a, b) for a, b in zip(corners, np.roll(corners, -1, axis=0)))
            if len(face) == 3 or np.linalg.norm(normal) == 0.0:
                triangles.append(face[:3])
                continue
            normal = normal / np.linalg.norm(normal)
            across = np.cross(normal, [1.0, 0.0, 0.0] if abs(normal[0]) < 0.8 else [0.0, 1.0, 0.0])
            across /= np.linalg.norm(across)
            flat = np.stack([corners @ across, corners @ np.cross(normal, across)], 1)
            triangles += [[face[i] for i in triangle] for triangle in manifold3d.triangulate([flat], epsilon=1e-6)]
        welded, index = np.unique(np.round(points, 3), axis=0, return_inverse=True)
        triangles = index.ravel()[np.array(triangles)]
        whole = (triangles[:, 0] != triangles[:, 1]) & (triangles[:, 1] != triangles[:, 2]) & (triangles[:, 0] != triangles[:, 2])
        solid = manifold3d.Manifold(manifold3d.Mesh(welded.astype(np.float32), triangles[whole].astype(np.uint32)))
        if solid.status() == manifold3d.Error.NoError and solid.volume() > 0.0:
            found[piece["name"]] = solid
        else:
            opened.append((piece["name"], manifold3d.Manifold.hull_points(welded)))
    for name, hull in opened:
        low, high = np.split(np.array(hull.bounding_box()), 2)
        for other, solid in list(found.items()):
            box = np.array(solid.bounding_box())
            if re.search("pin_|part", other) and np.all(box[:3] < high) and np.all(box[3:] > low):
                hull = hull - solid
        found[name] = hull
    return found


def shared(a, b) -> float:
    """The mean thickness of the wood two solids share, mm."""
    both = a ^ b
    return 0.0 if both.is_empty() else 2.0 * both.volume() / both.surface_area()


def is_screw_pulled(a: str, b: str, apart: np.ndarray) -> bool:
    """Whether the pair is a 4 mm screw of the beams' corners, level along its axis, and a beam it is driven through: the model
    drills that corner only in part, the screw cuts its own way."""
    screw = any(re.match(r"connector_pins_(inner_rib|seam_beam)_", name) for name in (a, b))
    return screw and any(name.startswith("inner_beams") for name in (a, b)) and abs(apart[2]) < 1e-6


def check(film: Film, pieces: dict, step: float = 4.0, play: float = 0.1) -> dict:
    """The pairs that run into each other, by the thickness of wood they share beyond what they share standing, with the time; no
    piece moves more than step against another between two looks."""
    names = list(pieces)
    boxes = np.array([pieces[name].bounding_box() for name in names])
    times = np.linspace(0.0, film.length, int(film.length * 4000))
    paths = np.array([[film.displacement(name, time) for time in times] if film.moves[name] else np.zeros((len(times), 3)) for name in names])
    fastest = np.max(np.linalg.norm(np.diff(paths, axis=1), axis=2), axis=0)
    looks, since = [0], 0.0
    for k, moved in enumerate(fastest):
        if since + 2.0 * moved > step:
            looks.append(k)
            since = 0.0
        since += 2.0 * moved
    looks.append(len(times) - 1)

    standing, seen, hits, screws = {}, set(), {}, 0.0
    for look in sorted(set(looks)):
        low, high = boxes[:, :3] + paths[:, look], boxes[:, 3:] + paths[:, look]
        order = np.argsort(low[:, 0])
        for place, i in enumerate(order):
            others = order[place + 1 :]
            others = others[low[others, 0] < high[i, 0]]
            others = others[np.all(low[others] < high[i], axis=1) & np.all(high[others] > low[i], axis=1)]
            for j in others:
                apart = paths[j, look] - paths[i, look]
                key = (i, j, tuple(np.round(apart, 2)))
                if np.abs(apart).max() < 1e-9 or key in seen:
                    continue
                seen.add(key)
                a, b = names[i], names[j]
                if (a, b) not in standing:
                    standing[a, b] = shared(pieces[a], pieces[b])
                beyond = shared(pieces[a], pieces[b].translate(apart)) - standing[a, b]
                if is_screw_pulled(a, b, apart):
                    screws = max(screws, beyond)
                elif beyond > play and beyond > hits.get((a, b), (0.0, 0.0))[0]:
                    hits[a, b] = (beyond, times[look])
    print(f"{len(names)} pieces, {len(set(looks))} looks, {len(seen)} pairs intersected: {len(hits)} pairs run into each other; screws pulled through undrilled wood, {screws:.1f} mm")
    by_kind = collections.Counter(tuple(re.sub(r"\d", "#", name) for name in pair) for pair in hits)
    for kind, count in by_kind.most_common():
        print(count, kind)
    return hits
