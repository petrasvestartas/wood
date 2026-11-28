"""The GIFs of the template cards, docs/images/joint_library/card_<family>.gif: each family's assembly GIF at the size of a card, its
loop turned so that it starts with the finished structure, framed on it: the pieces come in from outside the picture.

    python3 tools/card_gifs.py"""

import pathlib

from PIL import Image, ImageChops

WOOD = pathlib.Path(__file__).resolve().parents[1]
IMAGES = WOOD / "docs/images"
SIZE = (540, 360)
CARDS = {
    "shells": "shells_1_translation_shell", "folding": "folding_1_reflex_fold", "cylindrical_folding": "cylindrical_folding_8_groin_vault",
    "developable_folding": "developable_folding_11_corrugated_dome", "chevron": "chevron_1_chevron_chevron", "cross": "cross_2_assembly",
    "module": "module_3_cassette_house", "reciprocal": "reciprocal_2_move", "grid": "grid_5_multistorey_five_bay", "frame": "frame_2_tiered_frames_tiered_clerestory_hall",
    "floor": "floor_7_contacts_cantilevers", "timber_floor": "timber_floor_1_structures_box", "truss": "truss_1_pratt", "vault": "vault_9_gothic_ribbed_vault_quadripartite_wells",
    "plaited": "plaited_4_hotwire_blocks_sphera_open", "vidy": "vidy_4_section_joints",
}


def main() -> None:
    """Writes every card's GIF."""
    for family, name in CARDS.items():
        source = Image.open(next(IMAGES.glob(f"*/assembly_templates_{name}.gif")))
        frames, durations = [], []
        for k in range(source.n_frames):
            source.seek(k)
            frames.append(source.convert("RGB"))
            durations.append(source.info["duration"])

        # framed on the finished structure with room round it, 3:2
        clear = Image.new("RGB", frames[0].size, frames[0].getpixel((2, 2)))
        x0, y0, x1, y1 = ImageChops.difference(frames[-1], clear).convert("L").point(lambda value: 255 if value > 12 else 0).getbbox()
        width = max((x1 - x0) * 1.25, (y1 - y0) * 1.3 * 1.5)
        left, top = (x0 + x1 - width) / 2, (y0 + y1 - width / 1.5) / 2
        crop = tuple(round(value) for value in (left, top, left + width, top + width / 1.5))
        small = []
        for frame in frames:
            canvas = Image.new("RGB", (crop[2] - crop[0], crop[3] - crop[1]), clear.getpixel((0, 0)))
            canvas.paste(frame, (-crop[0], -crop[1]))
            small.append(canvas.resize(SIZE, Image.Resampling.LANCZOS))

        # the finished frame first, then from the start of the loop on
        small, durations = small[-1:] + small[:-1], durations[-1:] + durations[:-1]
        sample = Image.new("RGB", (SIZE[0], SIZE[1] * len(small)))
        for k, frame in enumerate(small):
            sample.paste(frame, (0, SIZE[1] * k))
        palette = sample.quantize(colors=96, method=Image.Quantize.MEDIANCUT)
        drawn = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in small]
        out = IMAGES / f"joint_library/card_{family}.gif"
        drawn[0].save(out, save_all=True, append_images=drawn[1:], duration=durations, loop=0, optimize=True)
        print(out.name, len(small), out.stat().st_size // 1024, "K")


main()
