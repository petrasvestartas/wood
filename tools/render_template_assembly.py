"""The assembly GIFs of the templates, docs/images/assembly/<example>[_<assembly>][_<module>].gif and .png, drawn as the joint
library draws its joints (tools/render_joint_library_gifs.py): see-through, one fixed isometric camera, the moving pieces in
the strong colour. Each example is run with WOOD_ASSEMBLY, so its WoodSession::pb_dump also writes how every template in it
goes together (a ModuleSequence: every module taken apart on its own, every move swept against what is already in). A GIF per
kind of module, the first of its kind, and one of the whole; a module of few pieces starts with all of them apart, out along
their ways, in a larger one every piece comes in as it is its turn. The pieces that can go at the same time move together.

    python3 tools/render_template_assembly.py <example> ... [--only <name part>] [--size 960x640] [--scenes | --draw]
"""

import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import render_joint_library_gifs as library  # noqa: E402

WOOD = library.WOOD
BUILD = pathlib.Path(os.environ.get("WOOD_BUILD", WOOD / "build"))
PICTURES = WOOD / "docs/images/assembly"
STRONG = (107, 68, 35)  # The moving pieces: dark brown.
APART = 12  # The most pieces a module starts apart with.
FRAMES = 90  # About the most frames a GIF has.
RENDER = (2400, 1600)  # Every pass, before it is cropped and scaled.

# The frames of one module as scenes, written once loaded: per frame the pieces shown, each with its moves undone and its
# slide back, the moving ones in the strong colour, the rest see-through in one pass and grey in the other.
SCENES = r"""
import json, sys
from laika_py.session import Session
from laika_py.xform import Xform
from laika_py.color import Color
from laika_py.line import Line
from laika_py.point import Point
scene_path, module_path, out, strong, apart_most, frames_most = sys.argv[1:7]
strong = json.loads(strong)
module = json.load(open(module_path))
moves = module["moves"]
scene = Session.pb_load(scene_path)
wanted = {guid for move in moves for guid in move["elements"]}
elements = {element.guid: element for element in scene.objects.elements if element.guid in wanted}
for guid, element in elements.items():
    element.set_features([])
    world = scene.world_xform(guid)
    if world is not None:
        element.place(world)
        element.session_xform = None
RIGHT, UP = (0.8660254, -0.5, 0.0), (0.25, 0.4330127, 0.8660254)
low = [min(move["low"][k] for move in moves) for k in range(3)]
high = [max(move["high"][k] for move in moves) for k in range(3)]
size = sum((b - a) ** 2 for a, b in zip(low, high)) ** 0.5

def corners(move):
    return [(move["low" if i & 1 else "high"][0], move["low" if i & 2 else "high"][1], move["low" if i & 4 else "high"][2]) for i in range(8)]

def legs_undone(move):
    xform = Xform()
    for leg in reversed(move["legs"]):
        if "shift" in leg:
            step = Xform.translation(*[-value for value in leg["shift"]])
        else:
            p, d = leg["line"][:3], leg["line"][3:]
            step = Xform.rotation_around_line(Line.from_points(Point(*p), Point(p[0] + d[0], p[1] + d[1], p[2] + d[2])), -leg["angle"], True)
        xform = step * xform
    return xform

def part_undone(move, share):
    # the first share of the way out: the legs undone bit by bit
    xform = Xform()
    count = len(move["legs"])
    for n, leg in enumerate(reversed(move["legs"])):
        part = min(max(share * count - n, 0.0), 1.0)
        if "shift" in leg:
            step = Xform.translation(*[-value * part for value in leg["shift"]])
        else:
            p, d = leg["line"][:3], leg["line"][3:]
            step = Xform.rotation_around_line(Line.from_points(Point(*p), Point(p[0] + d[0], p[1] + d[1], p[2] + d[2])), -leg["angle"] * part, True)
        xform = step * xform
    return xform

def hull(points):
    points = sorted(set(points))
    if len(points) < 3:
        return points
    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    below, above = [], []
    for p in points:
        while len(below) >= 2 and cross(below[-2], below[-1], p) <= 0:
            below.pop()
        below.append(p)
    for p in reversed(points):
        while len(above) >= 2 and cross(above[-2], above[-1], p) <= 0:
            above.pop()
        above.append(p)
    return below[:-1] + above[:-1]

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

def vertices(guid):
    geometry = elements[guid].geometry
    return geometry.vertex_points() if hasattr(geometry, "vertex_points") else geometry.to_vertices_and_faces()[0]

def outline(move, slide):
    moved = []
    for guid in move["elements"]:
        for p in vertices(guid):
            q = [p[k] - move["insertion"][k] * slide for k in range(3)]
            x, y = sum(q[k] * RIGHT[k] for k in range(3)), sum(q[k] * UP[k] for k in range(3))
            moved += [(x - 0.008 * size, y - 0.008 * size), (x + 0.008 * size, y - 0.008 * size), (x + 0.008 * size, y + 0.008 * size), (x - 0.008 * size, y + 0.008 * size)]
    return hull(moved)

# how far out every piece starts: clear of the others' bounds, at least a share of the module, never farther than it was swept
for move in moves:
    move["still"] = move["travel"] == 0.0
    move["slide"] = 0.0 if move["still"] else min(max(move["travel"], 0.15 * size), max(move["swept"], move["travel"]))
waves = []
for move in moves:
    if move["still"]:
        continue
    if not waves or waves[-1][0]["wave"] != move["wave"]:
        waves.append([])
    waves[-1].append(move)
apart = len(moves) <= int(apart_most)
if apart:
    # all apart on the first frame: every piece out far enough that its outline on the screen is clear of the ones waiting before it
    waiting = [outline(move, 0.0) for move in moves if move["still"]]
    for move in moves:
        if move["still"]:
            continue
        slide, shape = move["slide"], outline(move, move["slide"])
        for push in range(40):
            if not any(overlap(shape, other) for other in waiting):
                break
            slide += 0.03 * size
            shape = outline(move, slide)
        move["slide"] = slide
        waiting.append(shape)

most = int(frames_most)
slides = max(2, min(7, most // max(len(waves), 1)))
# more waves than frames: several waves are drawn landing one after another in the same frames
together = max(1, -(-2 * len(waves) // most))
frames = []

def pose(move, share):
    # share of its way in, 0 out and 1 in place; the legs the last third of it
    legs = 0.3 if move["legs"] else 0.0
    if share >= 1.0 - legs and legs > 0.0:
        return part_undone(move, (1.0 - share) / legs)
    back = move["slide"] * (1.0 - share / (1.0 - legs))
    return Xform.translation(*[-value * back for value in move["insertion"]]) * legs_undone(move)

def frame(placed, moving, duration, others=()):
    frames.append({"placed": list(placed), "moving": list(moving), "waiting": list(others), "duration": duration})

index = {id(move): k for k, move in enumerate(moves)}
placed = [index[id(move)] for move in moves if move["still"]]
rest = [index[id(move)] for wave in waves for move in wave]
frame(placed, [], 1500, [(k, 0.0) for k in rest] if apart else [])
for start in range(0, len(waves), together):
    group = [index[id(move)] for wave in waves[start:start + together] for move in wave]
    rest = [k for k in rest if k not in group]
    for s in range(0 if not apart else 1, slides + 1):
        frame(placed, [(k, s / slides) for k in group], 70 if s < slides else (300 if len(waves) < 30 else 120), [(k, 0.0) for k in rest] if apart else [])
    placed += group
frame(placed, [], 2500)

# every frame's bounds, the camera kept still by two marks, all centred on the origin
for f in frames:
    for k, share in f["moving"] + f["waiting"]:
        for c in corners(moves[k]):
            p = Point(*c).transformed(pose(moves[k], share))
            for i in range(3):
                low[i], high[i] = min(low[i], p[i]), max(high[i], p[i])
shift = Xform.translation(*[-(a + b) / 2 for a, b in zip(low, high)])
half = [(b - a) / 2 for a, b in zip(low, high)]
at = {guid: Xform() for guid in elements}
home = set()

def put(guid, xform, placed):
    # a piece in its place stays put from frame to frame
    if placed and guid in home:
        return
    (home.add if placed else home.discard)(guid)
    target = shift * xform
    elements[guid].place(target * at[guid].inverse())
    at[guid] = target

paths = []
for n, f in enumerate(frames):
    standing = set(f["placed"])
    poses = {k: (Xform(), False) for k in standing}
    poses.update({k: (pose(moves[k], share), True) for k, share in f["moving"]})
    poses.update({k: (pose(moves[k], share), False) for k, share in f["waiting"]})
    shown = Session("frame")
    for k, (xform, moving) in poses.items():
        for guid in moves[k]["elements"]:
            put(guid, xform, k in standing)
            shown.add_element(elements[guid])
    for a, b in (([-h for h in half], [0.5 - half[0], -half[1], -half[2]]), (half, [half[0] - 0.5, half[1], half[2]])):
        mark = Line.from_points(Point(*a), Point(*b))
        mark.linecolor = Color(0.969, 0.969, 0.969, 1.0)
        shown.add_line(mark, None)
    pair = []
    for solid in (False, True):
        for k, (xform, moving) in poses.items():
            colour = Color(*[value / 255 for value in strong], 1.0) if moving else Color(0.55, 0.55, 0.55, 1.0) if solid else None
            for guid in moves[k]["elements"]:
                shown.set_node_color(shown.get_node(guid), colour)
        path = "%s/frame_%03d%s.pb" % (out, n, "_solid" if solid else "")
        shown.pb_dump(path)
        pair.append(path)
    paths.append({"pair": pair, "duration": f["duration"]})
print(json.dumps(paths))
"""


def render(pb: str, opacity: str) -> Image.Image:
    """One pass drawn isometric in the Arctic look, aimed at the origin, the outline twice its width for the halving."""
    env = dict(
        os.environ, VIEWER_SHEETS="0", VIEWER_OPACITY=opacity, VIEWER_W=str(RENDER[0]), VIEWER_H=str(RENDER[1]), VIEWER_DPR="2",
        VIEWER_THICKNESS="0.7", VIEWER_VIEW="iso", VIEWER_PLANE_SIZE="0", VIEWER_NO_GRID="1", VIEWER_AO="1", VIEWER_OUTLINES="1",
        VIEWER_TARGET="0.000,0.000,0.000",
    )
    with tempfile.TemporaryDirectory() as tmp:
        ppm = pathlib.Path(tmp) / "frame.ppm"
        subprocess.run([str(library.RENDERER), "render-pb", str(ppm), pb], env=env, check=True, capture_output=True)
        return Image.open(ppm).convert("RGB")


def drawing(pair: list) -> Image.Image:
    """The see-through pass as dark as the solid one, with the solid one's strong colour laid over it where a piece moves."""
    solid = render(pair[1], "1.0")
    image = ImageChops.darker(render(pair[0], "0.45"), solid)
    red, green, blue = solid.split()
    brown = ImageChops.subtract(red, blue).point(lambda value: 255 if value > 28 else 0)
    image.paste(solid, (0, 0), brown)
    return image


def draw(scene: pathlib.Path, module: dict, name: str, size: tuple) -> None:
    """Saves the module's GIF, and its picture: all its pieces apart, or the whole where it starts empty."""
    # the frames on disk, not in /tmp: a large template's are gigabytes
    with tempfile.TemporaryDirectory(dir=BUILD) as tmp:
        module_path = pathlib.Path(tmp) / "module.json"
        module_path.write_text(json.dumps(module))
        listing = subprocess.run(
            [str(library.PYTHON), "-c", SCENES, str(scene), str(module_path), tmp, json.dumps(STRONG), str(APART), str(FRAMES)],
            capture_output=True, text=True,
        )
        if listing.returncode:
            raise RuntimeError(listing.stderr[-3000:])
        written = json.loads(listing.stdout.strip().splitlines()[-1])
        images = [drawing(frame["pair"]) for frame in written]

    # cropped alike to what any frame draws with a margin, framed 3:2, scaled, the GIF on one undithered palette
    clear = images[0].getpixel((2, 2))
    box = None
    for image in images:
        drawn = ImageChops.difference(image, Image.new("RGB", image.size, clear)).convert("L").point(lambda value: 255 if value > 12 else 0).getbbox()
        if drawn:
            box = drawn if box is None else (min(box[0], drawn[0]), min(box[1], drawn[1]), max(box[2], drawn[2]), max(box[3], drawn[3]))
    crop = (max(0, box[0] - 40), max(0, box[1] - 40), min(RENDER[0], box[2] + 40), min(RENDER[1], box[3] + 40))
    width = max(crop[2] - crop[0], int((crop[3] - crop[1]) * 1.5))
    height = int(width / 1.5)
    framed = []
    for image in images:
        canvas = Image.new("RGB", (width, height), clear)
        piece = image.crop(crop)
        canvas.paste(piece, ((width - piece.width) // 2, (height - piece.height) // 2))
        framed.append(canvas.resize(size, Image.LANCZOS))
    PICTURES.mkdir(parents=True, exist_ok=True)
    picked = framed[:: max(1, len(framed) // 24)]
    sample = Image.new("RGB", (size[0], size[1] * len(picked)))
    for k, image in enumerate(picked):
        sample.paste(image, (0, size[1] * k))
    palette = sample.quantize(colors=64, method=Image.Quantize.MEDIANCUT)
    quantized = [image.quantize(palette=palette, dither=Image.Dither.NONE) for image in framed]
    quantized[0].save(
        PICTURES / f"{name}.gif", save_all=True, append_images=quantized[1:], duration=[frame["duration"] for frame in written], loop=0, optimize=True
    )
    print(f"docs/images/assembly/{name}.gif", len(framed), "frames", flush=True)


def main(arguments: list) -> None:
    """Runs each example and draws every kind of module and the whole. --scenes only runs, --draw only draws what a run kept."""
    size = tuple(int(value) for value in library.option(arguments, "--size", "960x640").split("x"))
    only = library.option(arguments, "--only", "")
    scenes_only, draw_only = "--scenes" in arguments, "--draw" in arguments
    kept = BUILD / "assembly_scenes"
    kept.mkdir(parents=True, exist_ok=True)
    for example in [argument for argument in arguments if not argument.startswith("--")]:
        scene = kept / f"{example}.pb"
        if not draw_only:
            for old in kept.glob(f"{example}*.json"):
                old.unlink()
            run = subprocess.run(
                [str(BUILD / example)], cwd=WOOD, env=dict(os.environ, WOOD_ASSEMBLY=str(kept / f"{example}.json")), capture_output=True, text=True
            )
            if run.returncode:
                print(f"failed {example}: exit {run.returncode}\n{run.stderr[-1500:]}", flush=True)
                continue
            scene.write_bytes((WOOD / "data/output/pb/live.pb").read_bytes())
            (kept / f"{example}.txt").write_text(run.stdout)
            print(f"scene {example}", flush=True)
        if scenes_only:
            continue
        for path in sorted(kept.glob(f"{example}.json")) + sorted(kept.glob(f"{example}_*.json")):
            assembly = json.loads(path.read_text())
            drawn = set()
            for module in assembly["modules"]:
                whole = module["name"] == assembly["root"]
                kind = module["kind"]
                if kind in drawn and not whole or len(module["moves"]) < 2:
                    continue
                drawn.add(kind)
                name = f"assembly_{path.stem}" if whole else f"assembly_{path.stem}_{kind}"
                if only and not re.search(only, name):
                    continue
                draw(scene, module, name, size)


if __name__ == "__main__":
    main(sys.argv[1:])
