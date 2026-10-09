import collections
import fcntl
import json
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops, ImageDraw, ImageFont

WOOD = pathlib.Path(__file__).resolve().parents[1]
RENDERER = (
    WOOD.parent
    / "session/session_viewer/target/x86_64-unknown-linux-gnu/release/examples/selftest"
)
PYTHON = WOOD.parent / "session/session_py/.venv/bin/python"
IMAGES = WOOD / "docs" / "images" / "elements"
BUILD = pathlib.Path(os.environ.get("WOOD_BUILD", WOOD / "build"))
LOCK = pathlib.Path(tempfile.gettempdir()) / "wood_live_pb.lock"
FONT = str(pathlib.Path.home() / ".local/share/fonts/Roboto-Regular.ttf")
BOLD = str(pathlib.Path.home() / ".local/share/fonts/Roboto-Medium.ttf")
SIZE = (1500, 750)  # the picture with its panel
PANEL = 470  # the panel's width
INK = (26, 26, 26)
MUTED = (115, 115, 115)

EXAMPLES = {  # every example: the box the camera frames in mm (x0, y0, z0, x1, y1, z1) and whether its features are drawn
    "element_plate": ((-50, -50, 0, 650, 450, 40), True),
    "element_plate_holes": ((-50, -50, -10, 650, 450, 50), True),
    "element_beam": ((-150, -150, -150, 1150, 450, 150), True),
    "element_beam_variable": ((-100, -100, -750, 3100, 100, 10), True),
    "element_beam_variable_cut": ((-100, -500, -950, 3100, 500, 200), True),
    "element_column": ((-150, -200, 0, 150, 200, 3500), True),
    "element_column_session": ((-150, -150, 0, 500, 500, 3600), False),  # feature mode would draw the cutters over the carved head
    "element_block": ((-50, -50, 0, 350, 350, 250), True),
    "element_support": ((-250, -250, 0, 250, 250, 300), True),
    "element_joint_plate_ts_e_p": ((-50, -750, 0, 1850, 450, 500), False),
    "element_joint_plate_ss_e_ip": ((-50, -950, 0, 2550, 450, 40), False),
    "element_joint_plate_ss_e_op": ((-50, -50, 0, 2300, 450, 400), False),
    "element_joint_plate_cr_c_ip": ((-50, -950, 0, 1850, 250, 450), False),
    "element_joint_plate_tt_e_p": ((-50, -750, 0, 1950, 350, 230), False),
    "element_joint_plate_parameters": ((-50, -750, 0, 1850, 450, 500), False),
    "element_joint_drill": ((-50, -50, -50, 350, 350, 250), True),
    "element_joint_cutter": ((-50, -150, 0, 1250, 150, 400), True),
    "element_joint_beam_from_contact": ((-550, -550, -100, 1950, 650, 100), True),
    "element_joint_beam_wedge": ((-50, -250, -50, 1650, 550, 350), False),
    "element_joint_beam_rectangle_plate": ((-150, -150, 0, 1450, 150, 1250), True),
    "element_joint_beam_tie": ((-50, -150, -50, 2450, 150, 350), False),
    "element_joint_beam_centred_pins": ((-50, -50, -50, 450, 450, 450), True),
    "element_joint_beam_headed_pins": ((-50, -100, -50, 2250, 750, 350), True),
    "element_pin": ((-50, -50, 0, 50, 50, 200), True),
    "element_connector_part": ((200, -100, 50, 1400, 100, 350), True),
    "element_profile": ((-350, -50, -200, 4550, 850, 200), True),
}

READ = r"""
import json, sys
from session_py.session import Session
s = Session.pb_load(sys.argv[1])
call = lambda v: v() if callable(v) else v
names = {call(e.guid): e.name for e in s.objects.elements}
elements = [[e.name, call(e.element_type_name), e.is_visible, [f.feature_type for f in call(e.features)]] for e in s.objects.elements]
edges = []
for u, v in s.graph.get_edges():
    edge = s.graph.edges[u][v]
    kinds = [call(i.interaction_type_name) for i in s.interactions.get(call(edge.guid), [])] if hasattr(s, "interactions") else []
    edges.append([names.get(edge.v0, "?"), names.get(edge.v1, "?"), kinds])
print(json.dumps({"elements": elements, "edges": edges}))
"""  # the scene's elements, their features and the interaction features between them, read by session_py


def render(pb: pathlib.Path, bounds: tuple, features: bool) -> Image.Image:
    """The scene at opacity 0.75, with its features when asked, in the Arctic view, cropped to what it draws."""
    env = dict(
        os.environ,
        VIEWER_SHEETS="0",
        VIEWER_OPACITY="0.75",
        VIEWER_W="2400",
        VIEWER_H="1600",
        VIEWER_THICKNESS="2",
        VIEWER_VIEW="iso",
        VIEWER_BOUNDS=",".join(str(v) for v in bounds),
        VIEWER_PLANE_SIZE="0",
        VIEWER_NO_GRID="1",
        VIEWER_AO="1",
        VIEWER_OUTLINES="1",
    )
    env.setdefault("VIEWER_ADAPTER", "nvidia")

    if features:
        env["VIEWER_FEATURES"] = "1"

    with tempfile.TemporaryDirectory() as tmp:
        ppm = pathlib.Path(tmp) / "frame.ppm"
        subprocess.run(
            [str(RENDERER), str(ppm), str(pb)], env=env, check=True, capture_output=True
        )
        image = Image.open(ppm).convert("RGB")

    background = Image.new("RGB", image.size, image.getpixel((2, 2)))
    box = (
        ImageChops.difference(image, background)
        .convert("L")
        .point(lambda v: 255 if v > 12 else 0)
        .getbbox()
    )

    return image.crop(box)


def panel_rows(scene: dict) -> list:
    """The panel's lines: each element with its type and counted features, alike hidden elements on one line, then the interaction features from the element on fewer edges to the one on more, alike ones on one line."""
    rows = [("Layers", BOLD, INK)]
    groups = collections.OrderedDict()

    many = len(scene["elements"]) > 8  # a gallery: alike elements on one line whatever their number

    for name, kind, visible, features in scene["elements"]:
        stem = name.rstrip("0123456789_")
        key = (kind, visible, tuple(sorted(collections.Counter(features).items())), stem if many or not visible else name)
        groups.setdefault(key, []).append(name)

    for (kind, visible, features, _), names in groups.items():
        label = names[0] if len(names) == 1 else f"{len(names)} x {names[0]}"
        rows.append((f"{label}  {kind}" + ("" if visible else "  (hidden)"), BOLD, INK if visible else MUTED))
        for feature, count in features:
            rows.append((f"    {feature}" + (f" x{count}" if count > 1 else ""), FONT, MUTED))

    degree = collections.Counter(name for a, b, _ in scene["edges"] for name in (a, b))
    counted = collections.Counter()

    for a, b, kinds in scene["edges"]:
        source, target = (b, a) if degree[a] > degree[b] else (a, b)
        for kind in kinds:
            if kind.startswith("InteractionFeature"):
                label = target.rstrip("0123456789_") + "_*" if many and target[-1:].isdigit() else target
                counted[(source.rstrip("0123456789_") + "_*" if source[-1:].isdigit() else source, label, kind.replace("InteractionFeature", "").lower())] += 1

    if counted:
        rows.append(("Interactions", BOLD, INK))
        for (a, b, kind), count in counted.items():
            rows.append((f"    {a} -> {b}: {kind}" + (f" x{count}" if count > 1 else ""), FONT, MUTED))

    return rows


def compose(picture: Image.Image, rows: list) -> Image.Image:
    """The picture fitted beside the panel."""
    width, height = SIZE
    out = Image.new("RGB", SIZE, picture.getpixel((0, 0)))
    room = (width - PANEL - 80, height - 80)
    scale = min(room[0] / picture.width, room[1] / picture.height)
    fitted = picture.resize(
        (max(1, int(picture.width * scale)), max(1, int(picture.height * scale))),
        Image.LANCZOS,
    )
    out.paste(
        fitted, (40 + (room[0] - fitted.width) // 2, (height - fitted.height) // 2)
    )

    draw = ImageDraw.Draw(out)
    left = width - PANEL
    draw.rectangle((left, 0, width, height), fill=(255, 255, 255))
    draw.line((left, 0, left, height), fill=(220, 220, 220), width=2)
    y = 28

    for text, font, colour in rows:
        draw.text((left + 24, y), text, font=ImageFont.truetype(font, 19), fill=colour)
        y += 30

    return out


def main(names: list) -> None:
    """Runs each named example, or every one, and saves its picture with its layer panel under docs/images/elements."""
    IMAGES.mkdir(parents=True, exist_ok=True)
    pb = WOOD / "data" / "output" / "pb" / "live.pb"

    for name in names or list(EXAMPLES):
        with tempfile.TemporaryDirectory() as tmp:
            scene_pb = pathlib.Path(tmp) / "scene.pb"

            # the run and its live.pb locked, as other runs write the same file
            with open(LOCK, "w") as lock:
                fcntl.flock(lock, fcntl.LOCK_EX)
                subprocess.run([str(BUILD / name)], check=True, capture_output=True, cwd=WOOD)
                shutil.copy(pb, scene_pb)

            scene = json.loads(
                subprocess.run(
                    [str(PYTHON), "-c", READ, str(scene_pb)],
                    check=True,
                    capture_output=True,
                    text=True,
                ).stdout
            )
            bounds, features = EXAMPLES[name]
            compose(render(scene_pb, bounds, features), panel_rows(scene)).save(
                IMAGES / f"{name}.png", optimize=True
            )
        print(f"{name}.png")


if __name__ == "__main__":
    main(sys.argv[1:])
