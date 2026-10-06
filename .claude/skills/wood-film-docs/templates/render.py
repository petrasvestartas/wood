"""Render the floor film: every frame docs_floor_movie wrote, through session_viewer's headless renderer, into docs/images/floor.

The renderer draws the scene; this script reads the camera it logged, projects every labelled scene point onto the
picture, places each name plate where it overlaps no other plate, caption or labelled point, draws its leader, and
writes the caption in a band above the picture.

Run from wood/ after building docs_floor_movie and session_viewer's selftest example:
    ./build/docs_floor_movie && python3 docs/floor/render.py
"""

import json
import math
import os
import pathlib
import re
import subprocess
import sys
import tempfile
import textwrap

import numpy
from PIL import Image, ImageDraw, ImageFont

WOOD = pathlib.Path(__file__).resolve().parents[2]
FRAMES = WOOD / "data" / "output" / "floor_movie"
IMAGES = WOOD / "docs" / "templates" / "floor"
RENDERER = pathlib.Path(
    os.environ.get(
        "SELFTEST",
        WOOD.parent
        / "session/session_viewer/target/x86_64-unknown-linux-gnu/release/examples/selftest",
    )
)
FONT = str(pathlib.Path.home() / ".local/share/fonts/Roboto-Regular.ttf")  # Roboto, the font of the compas_wood docs
SIZE = (1280, 760)  # the scene in layout pixels; the caption band goes above it
SCALE = 2  # picture pixels per layout pixel: the renderer, its pens, the labels and the caption all drawn this much finer
BAND = 64
FOVY_DEG = 60.0  # session_viewer's camera
PLATE = (24, 24, 24)
FILM_WIDTH = SIZE[0] * SCALE  # the films at the pictures' full width
OVERSAMPLE = 4  # the labels are drawn this many times larger, then scaled down
FILM_MS = 2200
OPACITY = 1.0  # every surface solid: below 1 the viewer blends all faces and what lies behind shows through
ARCTIC = os.environ.get("FILM_ARCTIC", "1") == "1"  # the viewer's Arctic view: soft contact shading and outlines, so solids read by their form


def render_scene(notes: dict, pb: pathlib.Path) -> tuple[Image.Image, dict]:
    """The scene as the renderer draws it, and the camera it logged."""
    env = dict(
        os.environ,
        VIEWER_SHEETS="0",
        VIEWER_OPACITY=str(OPACITY),
        VIEWER_W=str(SIZE[0] * SCALE),
        VIEWER_H=str(SIZE[1] * SCALE),
        VIEWER_THICKNESS=str(SCALE),
        VIEWER_VIEW=notes["view"],
        VIEWER_BOUNDS=",".join(str(v) for v in notes["bounds"]),
        VIEWER_PLANE_SIZE=str(notes["plane_size"]),
    )
    env.setdefault("VIEWER_ADAPTER", "nvidia")

    if notes.get("orbit"):
        env["VIEWER_ORBIT"] = notes["orbit"]
    if notes.get("distance"):
        env["VIEWER_DISTANCE_SCALE"] = str(notes["distance"])
    if notes["features"]:
        env["VIEWER_FEATURES"] = "1"
    env["VIEWER_NO_GRID"] = "1"  # no grid: its red and green axes read as roles
    if ARCTIC:
        env["VIEWER_AO"] = "1"
        env["VIEWER_OUTLINES"] = "1"

    with tempfile.TemporaryDirectory() as tmp:
        ppm = pathlib.Path(tmp) / "frame.ppm"
        run = subprocess.run(
            [str(RENDERER), str(ppm), str(pb)],
            env=env,
            check=True,
            capture_output=True,
            text=True,
        )
        image = Image.open(ppm).convert("RGB")

    line = re.search(
        r"CENSUS_EYE=(\S+) CENSUS_FWD=(\S+) CENSUS_UP=(\S+) CENSUS_ORTHO_H=(\S+)",
        run.stderr + run.stdout,
    )
    camera = {
        key: [float(v) for v in value.split(",")]
        for key, value in zip(("eye", "forward", "up"), line.groups()[:3])
    }
    camera["ortho_h"] = float(line.group(4)) * 0.001  # logged in mm, the eye in m
    return image, camera


def projector(camera: dict):
    """Scene mm to picture pixels, as the renderer's camera sees them."""
    sub = lambda a, b: [a[i] - b[i] for i in range(3)]
    dot = lambda a, b: sum(a[i] * b[i] for i in range(3))
    cross = lambda a, b: [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    ]
    unit = lambda a: [v / math.sqrt(dot(a, a)) for v in a]
    forward = unit(camera["forward"])
    right = unit(cross(forward, camera["up"]))
    up = cross(right, forward)
    aspect = SIZE[0] / SIZE[1]
    half = math.tan(math.radians(FOVY_DEG * 0.5))

    def project(point: list) -> tuple[float, float]:
        d = sub([v * 0.001 for v in point], camera["eye"])
        x, y, z = dot(d, right), dot(d, up), dot(d, forward)
        if camera["ortho_h"] > 0.0:
            u, v = x / (camera["ortho_h"] * aspect), y / camera["ortho_h"]
        else:
            u, v = x / (z * half * aspect), y / (z * half)
        return SIZE[0] * 0.5 * (1.0 + u), SIZE[1] * 0.5 * (1.0 - v)

    return project


def overlaps(a: tuple, b: tuple, margin: float = 5.0) -> bool:
    return a[0] < b[2] + margin and b[0] < a[2] + margin and a[1] < b[3] + margin and b[1] < a[3] + margin


def ink_table(image: Image.Image) -> numpy.ndarray:
    """A summed-area table of the drawn pixels: anything not near white."""
    pixels = numpy.asarray(image.convert("L"), dtype=numpy.float64)
    ink = (pixels < 235.0).astype(numpy.float64)
    return numpy.pad(ink.cumsum(0).cumsum(1), ((1, 0), (1, 0)))


def ink_under(table: numpy.ndarray, rect: tuple) -> float:
    """The share of a rectangle's pixels that are drawn."""
    x0, y0 = max(int(rect[0]), 0), max(int(rect[1]), 0)
    x1, y1 = min(int(rect[2]), SIZE[0]), min(int(rect[3]), SIZE[1])
    if x1 <= x0 or y1 <= y0:
        return 1.0
    return float(table[y1, x1] - table[y0, x1] - table[y1, x0] + table[y0, x0]) / ((x1 - x0) * (y1 - y0))


def leader(point: tuple, rect: tuple) -> tuple:
    """The leader from a point to the nearest point of its plate."""
    return point, (min(max(point[0], rect[0]), rect[2]), min(max(point[1], rect[1]), rect[3]))


def crosses(a: tuple, b: tuple) -> bool:
    """Whether two segments cross away from their ends."""
    (p, q), (r, s) = a, b
    side = lambda o, u, v: (u[0] - o[0]) * (v[1] - o[1]) - (u[1] - o[1]) * (v[0] - o[0])
    d1, d2, d3, d4 = side(r, s, p), side(r, s, q), side(p, q, r), side(p, q, s)
    return d1 * d2 < 0.0 and d3 * d4 < 0.0


def through(segment: tuple, rect: tuple) -> bool:
    """Whether a segment passes through a rectangle (Liang-Barsky clipping)."""
    (x0, y0), (x1, y1) = segment
    t0, t1 = 0.0, 1.0
    for p, q in ((x0 - x1, x0 - rect[0]), (x1 - x0, rect[2] - x0), (y0 - y1, y0 - rect[1]), (y1 - y0, rect[3] - y0)):
        if p == 0.0:
            if q < 0.0:
                return False
            continue
        t = q / p
        t0, t1 = (max(t0, t), t1) if p < 0.0 else (t0, min(t1, t))
        if t0 > t1:
            return False
    return t1 - t0 > 0.05


def place(labels: list, font: ImageFont.FreeTypeFont, table: numpy.ndarray) -> list:
    """Label placement as a small optimisation, the classic point-feature labelling problem.

    Every label has candidate plates: on its point when centred, else on rings of growing radius around it in 24
    directions. A candidate is allowed only inside the picture, clear of every other plate and of every other
    labelled point. Among the allowed ones a label takes the cheapest: leader length, the drawing the plate covers
    (read off the rendered picture), and every leader crossing another leader or plate. A greedy pass places the
    most crowded labels first, then passes re-place each label against all the others until nothing moves.
    Returns the problems: labels that found no allowed place and overlap.
    """
    sizes = []
    for label in labels:
        left, top, right, bottom = font.getbbox(label["text"])
        sizes.append((right - left + 20, bottom - top + 12))

    dots = [(label["xy"][0] - 7, label["xy"][1] - 7, label["xy"][0] + 7, label["xy"][1] + 7) for label in labels]

    def candidates(i: int) -> list:
        (x, y), (w, h) = labels[i]["xy"], sizes[i]
        if labels[i]["centred"]:
            return [(x - w * 0.5, y - h * 0.5, x + w * 0.5, y + h * 0.5)]
        rects = []
        for r in (34, 52, 74, 100, 130, 166, 210, 260, 320):
            for k in range(24):
                a = k * math.pi / 12.0
                cx, cy = x + (r + w * 0.5) * math.cos(a), y + (r + h * 0.5) * math.sin(a)
                rects.append((cx - w * 0.5, cy - h * 0.5, cx + w * 0.5, cy + h * 0.5))
        return rects

    def cost(i: int, rect: tuple, placed: dict, hard: bool) -> float:
        inside = rect[0] > 4 and rect[1] > 4 and rect[2] < SIZE[0] - 4 and rect[3] < SIZE[1] - 4
        clash = sum(overlaps(rect, other) for j, other in placed.items() if j != i) + sum(overlaps(rect, dot, 1.0) for j, dot in enumerate(dots) if j != i)
        if hard and (not inside or clash):
            return math.inf
        line = leader(labels[i]["xy"], rect)
        length = math.dist(*line)
        tangle = 0
        for j, other in placed.items():
            if j == i:
                continue
            other_line = leader(labels[j]["xy"], other)
            tangle += crosses(line, other_line) + through(line, other) + through(other_line, rect)
        return length / 40.0 + 8.0 * ink_under(table, rect) + 4.0 * tangle + 1000.0 * (clash + (not inside))

    def best(i: int, placed: dict) -> tuple:
        options = candidates(i)
        allowed = min(options, key=lambda rect: cost(i, rect, placed, True))
        if cost(i, allowed, placed, True) < math.inf:
            return allowed, False
        return min(options, key=lambda rect: cost(i, rect, placed, False)), True

    crowd = [sum(math.dist(a["xy"], b["xy"]) < 160.0 for b in labels) for a in labels]
    order = sorted(range(len(labels)), key=lambda i: (not labels[i]["centred"], -crowd[i]))
    placed, failed = {}, set()

    for i in order:
        placed[i], bad = best(i, placed)
        if bad:
            failed.add(i)

    for _ in range(6):
        moved = False
        for i in order:
            rest = {j: rect for j, rect in placed.items() if j != i}
            rect, bad = best(i, rest)
            if rect != placed[i] and cost(i, rect, rest, False) < cost(i, placed[i], rest, False) - 1e-9:
                placed[i], moved = rect, True
            failed.discard(i) if not bad else failed.add(i)
        if not moved:
            break

    for i, label in enumerate(labels):
        label["rect"] = placed[i]

    return [labels[i]["text"] for i in sorted(failed)]


def annotate(image: Image.Image, notes: dict, project) -> tuple[Image.Image, list]:
    """The caption band above the picture, every label's leader and then its plate on it, and the labels that could not be placed clear."""
    font = ImageFont.truetype(FONT, 15)
    caption_font = ImageFont.truetype(FONT, 21 * SCALE)
    labels = [{"text": label["text"], "xy": project(label["at"]), "centred": label.get("centred", False)} for label in notes["labels"]]
    problems = place(labels, font, ink_table(image.resize(SIZE, Image.LANCZOS)))
    problems += [f"{a['text']} and {b['text']} point at one spot" for i, a in enumerate(labels) for b in labels[i + 1:] if math.dist(a["xy"], b["xy"]) < 14.0]
    # leaders, rings and plates drawn at OVERSAMPLE times the size and scaled down, so their edges are smooth
    k = OVERSAMPLE * SCALE
    overlay = Image.new("RGBA", (SIZE[0] * k, SIZE[1] * k), (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    big = ImageFont.truetype(FONT, 15 * k)
    up = lambda *values: tuple(v * k for v in values)

    for label in labels:
        if not label["centred"]:
            x, y = label["xy"]
            start, end = leader(label["xy"], label["rect"])
            draw.line(up(*start, *end), fill=PLATE, width=2 * k)
            draw.ellipse(up(x - 4, y - 4, x + 4, y + 4), fill=(255, 255, 255), outline=PLATE, width=2 * k)

    for label in labels:
        left, top, right, bottom = label["rect"]
        draw.rounded_rectangle(up(left, top, right, bottom), radius=11 * k, fill=PLATE)
        draw.text(up((left + right) * 0.5, (top + bottom) * 0.5), label["text"], font=big, fill=(255, 255, 255), anchor="mm")

    image = Image.alpha_composite(image.convert("RGBA"), overlay.resize(image.size, Image.LANCZOS)).convert("RGB")

    framed = Image.new("RGB", (SIZE[0] * SCALE, (SIZE[1] + BAND) * SCALE), (255, 255, 255))
    framed.paste(image, (0, BAND * SCALE))
    band = ImageDraw.Draw(framed)
    band.multiline_text(
        (22 * SCALE, BAND * SCALE * 0.5),
        "\n".join(textwrap.wrap(notes["caption"], width=110)),
        font=caption_font,
        fill=(20, 20, 20),
        anchor="lm",
        spacing=4 * SCALE,
    )
    band.line([(0, BAND * SCALE - 1), (SIZE[0] * SCALE, BAND * SCALE - 1)], fill=(215, 215, 215), width=SCALE)
    return framed, problems


def save_film(frames: list, path: pathlib.Path) -> None:
    """An animated picture of the frames, each shown FILM_MS."""
    frames[0].save(path, save_all=True, append_images=frames[1:], duration=FILM_MS, loop=0, quality=90)
    print(f"{path.relative_to(WOOD)}  {len(frames)} frames, {path.stat().st_size // 1024} KB")


def main() -> int:
    """Every frame into a picture, a film per chapter, and the overview film of the key frames; non-zero when a label had no clear place."""
    if not RENDERER.exists():
        print(f"no renderer at {RENDERER}: cargo build --release --target x86_64-unknown-linux-gnu --example selftest", file=sys.stderr)
        return 1

    IMAGES.mkdir(parents=True, exist_ok=True)
    only = sys.argv[1:]  # frame numbers to render alone, e.g. 023 087; no films then
    if not only:
        for old in IMAGES.glob("*.webp"):
            old.unlink()

    chapters, key = {}, []
    unplaced = 0

    for path in sorted(p for p in FRAMES.glob("*.json") if not only or p.stem[:3] in only):
        notes = json.loads(path.read_text())
        image, camera = render_scene(notes, path.with_suffix(".pb"))
        framed, problems = annotate(image, notes, projector(camera))
        unplaced += len(problems)
        if problems:
            print(f"{path.stem}: {problems}", file=sys.stderr)
        out = IMAGES / f"{path.stem}.webp"
        framed.save(out, quality=95)
        small = framed.resize((FILM_WIDTH, FILM_WIDTH * framed.height // framed.width), Image.LANCZOS)
        chapters.setdefault(notes["chapter"], []).append(small)
        if notes.get("key"):
            key.append(small)
        print(f"{out.relative_to(WOOD)}  {out.stat().st_size // 1024} KB")

    if only:
        print(f"label problems: {unplaced}")
        return 1 if unplaced else 0

    for chapter, frames in chapters.items():
        save_film(frames, IMAGES / f"film_{chapter}.webp")

    save_film(key or [frames[0] for frames in chapters.values()], IMAGES / "floor_film.webp")
    print(f"label problems (no clear place, or two labels on one spot): {unplaced}")
    return 1 if unplaced else 0


if __name__ == "__main__":
    sys.exit(main())
