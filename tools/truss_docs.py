"""Write the galleries of docs/templates/truss.md, one card per truss showing the TrussGuide overload that makes it: the
library's designs with their defaults (between the library-gallery markers), from the design structs of
src/templates/truss/truss_guide.h and the pictures of `render_template_pictures.sh truss`; Bow's trusses (between the bow-gallery
markers), from data/templates/truss_bow_1873.json and the pictures of `render_template_pictures.sh bow`, grouped by kind and by
Bow's class.
    python3 -I wood/tools/truss_docs.py
"""
import json, pathlib, re

WOOD = pathlib.Path(__file__).resolve().parent.parent
CLASSES = {
    "I": "Class I: theoretically perfect frames: with every joint hinged they still cannot distort, and no part can be "
         "lengthened or shortened so as to induce stress.",
    "II": "Class II: a deficiency of parts: with hinged joints they hold their shape only under equilibrating loads, and "
          "owe their resistance to distortion to stiff joints and bending of the parts.",
    "III": "Class III: a redundancy of parts: no part can be made longer or shorter without inducing stress in others, so "
           "the stresses depend on the fitting, elasticity and temperature.",
    "IV": "Class IV: the defects of both Classes II and III: redundant in one region, deficient in another.",
}
KINDS = {"roof": "Roof trusses", "bridge": "Framed bridges"}

data = json.loads((WOOD / "data/templates/truss_bow_1873.json").read_text())["trusses"]
lines = [
    f"The {len(data)} roof trusses and framed bridges of R. H. Bow, *Economics of Construction in relation to Framed "
    "Structures* (1873), redrawn from his tables, each a graph in `data/templates/truss_bow_1873.json` built by "
    "`TrussGuide(Bow{number})` and `Truss`. Bow gives few spans: the rest are drawn at 8 ft (2.44 m) per sub-span, and every "
    "member is sized for trusses 3 m apart (roofs 1.5 kN/m2, bridges 5 kN/m2, x 1.4, square GL24h, chords at least 100 mm, "
    "webs 60 mm). The cards show the span.",
    "",
]
for kind in ("roof", "bridge"):
    for cls in ("I", "II", "III", "IV"):
        group = [t for t in data if t["kind"] == kind and t["class"] == cls]
        if not group:
            continue
        lines += [f"### {KINDS[kind]}, Class {cls} {{#truss_bow_{kind}_{cls.lower()}}}", "", CLASSES[cls], "",
                  '<div class="wood-card-grid cols-4">']
        for t in group:
            lines += [f'  <div class="wood-card">',
                      f'    <img src="truss_{t["name"]}.webp" alt="Bow No. {t["number"]}" loading="lazy">',
                      f'    <span class="wood-card-title">Bow{{"{t["number"]}"}} · {t["span"] / 1000:.1f} m · p. {t["page"]}</span>',
                      "  </div>"]
        lines += ["</div>", ""]

# the library: every design struct of the header, its parameters but the two half sections with their defaults
header = (WOOD / "src/templates/truss/truss_guide.h").read_text()
designs = header[header.index("// Designs"):header.index("struct Bow {")]
built = set(re.findall(r"wood_truss::(\w+)\{", (WOOD / "examples/templates_truss_2_library.cpp").read_text()))
structs = [s for s in re.findall(r"/// (.*)\nstruct (\w+) \{\n((?:    .*\n)+?)\};", designs) if s[1] in built]
library = [
    f"The {len(structs)} designs with their defaults; every design also takes `.chord_radius = 80` and `.web_radius = 60`.",
    "",
    '<div class="wood-card-grid cols-2">',
]
for doc, struct, fields in structs:
    name = re.sub(r"(?<!^)(?=[A-Z0-9])", "_", struct).lower()
    values = [re.match(r"\s*\w+ (\w+) = (.*);", f).groups() for f in fields.splitlines()]
    call = ", ".join(f".{n} = {v[:-2] if v.endswith('.0') else v}" for n, v in values if not n.endswith("_radius"))
    library += ['  <div class="wood-card">',
                f'    <img src="truss_{name}.png" alt="{struct}" loading="lazy">',
                f'    <span class="wood-card-title">{struct}{{{call}}}</span>',
                "  </div>"]
library += ["</div>", ""]

page = WOOD / "docs/templates/truss.md"
text = page.read_text()
for (start, end), block in ((("<!-- library-gallery -->", "<!-- /library-gallery -->"), library),
                            (("<!-- bow-gallery -->", "<!-- /bow-gallery -->"), lines)):
    text = text[:text.index(start) + len(start)] + "\n\n" + "\n".join(block) + "\n" + text[text.index(end):]
page.write_text(text)
print(len(structs), "designs,", len(data), "Bow trusses")
