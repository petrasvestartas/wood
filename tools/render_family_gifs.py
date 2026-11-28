"""The picture and the assembly GIF of every example of the plate and beam joint pages, docs/images/joint_library/
assembly_<example>.png and .gif, drawn as the joint library draws its joints (tools/render_joint_library_gifs.py): see-through,
the moving piece and the parts brown, one fixed camera, 960 x 640. Each example is run with WOOD_ASSEMBLY, so its
WoodSession::pb_dump also writes how it goes together (an AssemblySequence, its first jointed member staying); every other
member waits out along its way in, clear on the screen of the pieces waiting before it, and the GIF takes them in one by one.
An example without joints is stacked: its lowest member stays, the others come down onto it in the order of their height, or,
from four members on, in towards the middle. A jointed example is only ever drawn by its sequence, and every element needs
its own name: pieces are moved by name.

    python3 tools/render_family_gifs.py [example ...] [--slides 6] [--size 960x640] [--scenes | --draw]
"""

import json
import os
import pathlib
import re
import subprocess
import sys

import render_joint_library_gifs as library

WOOD = library.WOOD
PAGES = sorted((WOOD / "docs/elements").glob("element_joint_plate*.md")) + [
    page for page in sorted((WOOD / "docs/elements").glob("element_joint_beam*.md")) if page.stem != "element_joint_beam_compendium"
]

# Where every moving member of an example waits, as the joint library places its pieces: out along its way by its own depth
# and 40 mm a step of the order, then farther, 10 mm at a time, until its outline on the isometric screen (the viewer's
# View::Iso: right (0.866, -0.5, 0), up (0.25, 0.433, 0.866)) is 15 mm clear of every piece waiting before it and of the
# members that stay, a pair's own base aside. Writes the scene with each there and prints the order, the joints to leave out
# and the parts to draw brown.
OUTS = r"""
import json, sys
from laika_py.session import Session
from laika_py.xform import Xform
from laika_py.line import Line
from laika_py.point import Point
scene_path, assembly_path, apart_path = sys.argv[1:4]
scene = Session.pb_load(scene_path)
assembly = json.load(open(assembly_path))
elements = {element.name: element for element in scene.objects.elements}
RIGHT = (0.8660254, -0.5, 0.0)
UP = (0.25, 0.4330127, 0.8660254)

def points(element, xform):
    geometry = element.geometry.transformed(xform)
    return geometry.vertex_points() if hasattr(geometry, "vertex_points") else geometry.to_vertices_and_faces()[0]

def outline(element, xform):
    seen = []
    for p in points(element, xform):
        x = p[0] * RIGHT[0] + p[1] * RIGHT[1] + p[2] * RIGHT[2]
        y = p[0] * UP[0] + p[1] * UP[1] + p[2] * UP[2]
        for dx in (-7.5, 7.5):
            for dy in (-7.5, 7.5):
                seen.append((x + dx, y + dy))
    seen = sorted(set(seen))
    if len(seen) < 3:
        return seen
    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for p in seen:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(seen):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]

def overlap(a, b):
    for polygon in (a, b):
        for k in range(len(polygon)):
            p, q = polygon[k], polygon[(k + 1) % len(polygon)]
            axis = (q[1] - p[1], p[0] - q[0])
            pa = [axis[0] * v[0] + axis[1] * v[1] for v in a]
            pb = [axis[0] * v[0] + axis[1] * v[1] for v in b]
            if max(pa) < min(pb) or max(pb) < min(pa):
                return False
    return True

def undone(legs):
    xform = Xform()
    for leg in reversed(legs):
        if "shift" in leg:
            step = Xform.translation(*[-value for value in leg["shift"]])
        else:
            p, d = leg["line"][:3], leg["line"][3:]
            step = Xform.rotation_around_line(Line.from_points(Point(*p), Point(p[0] + d[0], p[1] + d[1], p[2] + d[2])), -leg["angle"], True)
        xform = step * xform
    return xform

steps = [step for step in assembly["steps"] if step["element"] in elements]
locked = [member for member in assembly.get("locked_members", []) if member["element"] in elements]
# the pieces of one pair or group: joined through their partners
group = {step["element"]: {step["element"]} for step in steps + locked}
for step in steps + locked:
    for partner in step["partners"]:
        if partner in group:
            joined = group[step["element"]] | group[partner]
            for name in joined:
                group[name] = joined

# a locked member (its joint fits too tight for the sweep, a milled corner or a leaning tenon in a square mortise) still comes
# out for the drawing: away from its partners' centre, down when the two centres meet; a group with no member staying keeps
# its first locked one in place
def centre(step):
    return [0.5 * (step["low"][k] + step["high"][k]) for k in range(3)]

kept = {name for step in steps if not any(abs(value) > 1e-9 for value in step["insertion"]) for name in group[step["element"]]}
for member in locked:
    if member["element"] not in kept:
        kept |= group[member["element"]]
        steps.append(dict(member, insertion=[0.0, 0.0, 0.0], legs=[], joints=[]))
        continue
    others = [centre(step) for step in steps + locked if step["element"] in member["partners"]]
    away = [centre(member)[k] - sum(other[k] for other in others) / len(others) for k in range(3)] if others else [0.0, 0.0, 0.0]
    length = sum(value * value for value in away) ** 0.5
    way = [-value / length for value in away] if length > 1.0 else [0.0, 0.0, 1.0]
    steps.append(dict(member, insertion=way, legs=[], joints=[]))
moving = [step for step in steps if any(abs(value) > 1e-9 for value in step["insertion"])]
staying = [step["element"] for step in steps if step not in moving]
waiting = []
for name in staying:
    waiting.append((name, outline(elements[name], Xform())))

order = []
for j, step in enumerate(moving, start=1):
    element = elements[step["element"]]
    way = step["insertion"]
    low, high = step["low"], step["high"]
    behind = sum(0.5 * (high[k] - low[k]) * abs(way[k]) for k in range(3))
    before = undone(step["legs"])
    distance = min(2.0 * behind, 160.0) + 40.0 * j
    # a pair's own base is no obstacle: the member slides straight out of it and reads apart
    others = [(name, shape) for name, shape in waiting if not (name in staying and name in group[step["element"]] and len(group[step["element"]]) == 2)]
    for push in range(300):
        shape = outline(element, Xform.translation(*[-value * distance for value in way]) * before)
        if not any(overlap(shape, other) for _, other in others):
            break
        distance += 10.0
    out = [-value * distance for value in way]
    waiting.append((step["element"], outline(element, Xform.translation(*out) * before)))
    element.place(Xform.translation(*out) * before)
    order.append({"element": step["element"], "way": way, "legs": step["legs"], "out": out})

# the joints: the ones the members engage, and every other element that is a mesh (a joint's drawing, not wood); the parts
# (pins, keys) the solids left
members = {step["element"] for step in steps}
joints = {joint for step in steps for joint in step["joints"]}
joints |= {name for name, element in elements.items() if name not in members and type(element.geometry).__name__ == "Mesh"}
joints = sorted(joints)
parts = [name for name in elements if name not in members and name not in joints]
# the scene with every moving member where it waits, as the library draws its corners: the frames bring each back in
scene.pb_dump(apart_path)
print(json.dumps({"order": order, "joints": joints, "parts": parts}))
"""

# The assembly of a scene without joints, as an AssemblySequence writes one: the lowest first, every member straight
# down, or in towards the scene's middle when there are four of them or more.
STACK = r"""
import json, sys
from laika_py.session import Session
scene = Session.pb_load(sys.argv[1])
members = []
for element in scene.objects.elements:
    corners = element.aabb.get_corners()
    low = [min(c[k] for c in corners) for k in range(3)]
    high = [max(c[k] for c in corners) for k in range(3)]
    if max(b - a for a, b in zip(low, high)) > 1.0:
        members.append((round(low[2], 1), round(high[2], 1), element.name, low, high))
members.sort()
middle = [sum(0.5 * (m[3][k] + m[4][k]) for m in members) / len(members) for k in range(3)]
steps = []
for k, (_, _, name, low, high) in enumerate(members):
    way = [0.0, 0.0, -1.0]
    if len(members) > 3:
        away = [middle[i] - 0.5 * (low[i] + high[i]) for i in range(3)]
        length = sum(value * value for value in away) ** 0.5
        way = [value / length for value in away] if length > 1.0 else way
    steps.append({"element": name, "insertion": way if k else [0.0, 0.0, 0.0], "low": low, "high": high, "legs": [], "partners": [], "joints": []})
json.dump({"steps": steps}, open(sys.argv[2], "w"))
"""


def examples() -> list:
    """The examples the plate and beam joint pages include, in page order."""
    found = []
    for page in PAGES:
        for example in re.findall(r"\\include\{lineno\} elements/([a-z0-9_]+)\.cpp", page.read_text()):
            if example not in found:
                found.append(example)
    return found


def main(arguments: list) -> None:
    """Runs each example (every example of the pages when none is named) and saves its picture and GIF. --scenes runs the
    examples alone, one after another (each writes data/output/pb/live.pb), keeping each scene and assembly in
    build/family_scenes; --draw draws from those alone, so several draws can run at once."""
    slides = int(library.option(arguments, "--slides", "6"))
    size = tuple(int(value) for value in library.option(arguments, "--size", "960x640").split("x"))
    scenes_only = "--scenes" in arguments
    draw_only = "--draw" in arguments
    arguments = [argument for argument in arguments if argument not in ("--scenes", "--draw")]
    kept = WOOD / "build/family_scenes"
    kept.mkdir(parents=True, exist_ok=True)
    for example in arguments or examples():
        scene = kept / f"{example}.pb"
        assembly = kept / f"{example}.json"
        if not draw_only:
            binary = WOOD / "build" / example
            if not binary.exists():
                print(f"skipped {example}: not built", flush=True)
                continue
            assembly.unlink(missing_ok=True)
            live = WOOD / "data/output/pb/live.pb"
            before = live.stat().st_mtime_ns if live.exists() else 0
            run = subprocess.run([str(binary)], cwd=WOOD, env=dict(os.environ, WOOD_ASSEMBLY=str(assembly)), capture_output=True, text=True)
            # a failed run, or one that wrote no scene of its own, is reported and drawn from nothing stale
            if run.returncode != 0 or not live.exists() or live.stat().st_mtime_ns == before:
                print(f"failed {example}: exit {run.returncode}\n{run.stderr[-2000:]}", flush=True)
                scene.unlink(missing_ok=True)
                assembly.unlink(missing_ok=True)
                continue
            scene.write_bytes(live.read_bytes())
            print(f"scene {example}", flush=True)
        if scenes_only:
            continue
        if not scene.exists():
            print(f"skipped {example}: no scene", flush=True)
            continue
        if not assembly.exists():
            subprocess.run([str(library.PYTHON), "-c", STACK, str(scene), str(assembly)], check=True)
        members = [step["element"] for step in json.loads(assembly.read_text())["steps"]]
        if len(set(members)) < len(members):
            print(f"skipped {example}: elements share a name", flush=True)
            continue
        if len(members) < 2:
            print(f"skipped {example}: nothing to take apart", flush=True)
            continue
        apart = kept / f"{example}_apart.pb"
        placed = json.loads(subprocess.run([str(library.PYTHON), "-c", OUTS, str(scene), str(assembly), str(apart)], check=True, capture_output=True, text=True).stdout.strip().splitlines()[-1])
        names = {"joints": placed["joints"], "parts": placed["parts"]}
        for path in library.draw(apart, "-", "", placed["order"], slides, f"assembly_{example}", names, size=size):
            print(path.relative_to(WOOD), flush=True)


if __name__ == "__main__":
    main(sys.argv[1:])
