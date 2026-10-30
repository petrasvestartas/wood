"""Write the galleries of docs/templates/truss.md, one card per truss showing the TrussGuide overload that makes it: the
library's designs with their defaults (between the library-gallery markers), from the design structs of
src/templates/truss/truss_guide.h and the pictures of tools/shoot_butt.sh; Bow's trusses (between the bow-gallery
markers), from data/templates/truss_bow_1873.json and the pictures of tools/shoot_truss_bow.sh, grouped by kind and by
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
    "Structures* (London: Spon, 1873), Part I, redrawn from the drawings of its classification tables. Each truss is "
    "stored as a graph in `data/templates/truss_bow_1873.json` (nodes in spans from the left support, x along the span "
    "and z up; chords as node sequences, webs and posts as node pairs) and built by the one `Truss` template: "
    "`TrussGuide(Bow{number})` draws its graph at its span with a section per member, and `Truss` makes the members and "
    "their butts and laps.",
    "",
    "```cpp",
    "for (const wood_truss::Bow& design : wood_truss::Bow::all()) {",
    "    wood_truss::Truss truss(wood_truss::TrussGuide(design), \"truss_\" + design.name());",
    "    truss.pb_dump(pb_path(\"truss_\" + design.name()));",
    "}",
    "```",
    "",
    "Each design was redrawn by hand from Bow's printed drawing as an exact construction: the few dimensions that "
    "define it read off the print (rise, collar height, camber, panel count), everything else constructed — equal panel "
    "points, members meeting at exact intersections, curved ribs on true circles or parabolas, a bar Bow draws through a "
    "joint kept as one member, symmetric designs mirrored exactly — and each checked over its scanned drawing. "
    "The numbers are Bow's; the page is that of his table. The book is transcribed in the BRG library "
    "(`framed-structures-bow`), where every table row links to its scan.",
    "",
    "### Joints {#truss_bow_joints}",
    "",
    "Every node is solved as carpentry by a hierarchy stored with the graph: each member is primary, secondary or "
    "tertiary (`chord_levels`, `web_levels`, `post_levels`), and at a node a member stops on every member of a higher "
    "level, butting on its surface. Joints are T joints wherever possible: a member that is continuous runs through, "
    "and at the ends of a truss the tie runs on past the rafter foot, the chords past the end posts, the rafters past "
    "a straining beam (`runs_on`), each cut flush with the far side of the member it carries, or square at shallow "
    "angles, and where a member carries others on both its sides its end is cut through their outer corners, so the outline "
    "of the truss runs on clean; the node where the axes meet always lies in wood. Miters are kept only for the gentle bends "
    "of one continuous member and for symmetric ridges.",
    "",
    "### Scale and sections {#truss_bow_sizing}",
    "",
    "Bow gives the span of only a few designs (Nos. 130, 137, 192, 195, 293, 332); his diagrams of forces are drawn to "
    "no absolute scale. Every other design is drawn at its number of sub-spans times 8 ft (2.44 m), the sub-span of his "
    "stated examples (No. 130: 86 1/4 ft over 12 sub-spans; No. 137: 97 1/2 ft over 12; No. 332: 204 ft over 24); the "
    "arched roofs whose sub-spans he calls irregular (Nos. 193, 194, 197) are taken at 100 ft. "
    "Each member's section is then sized, per truss, for timber trusses 3 m apart: roofs 1.5 kN/m2 on the roof outline "
    "(the upper chords), bridges 5 kN/m2 on the deck, both x 1.4. The forces are those of truss action, as Bow's diagrams "
    "of forces give them: the pin-jointed frame, a curved or bent chord drawn through nodes where nothing else meets it "
    "being one bar between the joints where others do; such a bar also carries the bending of the loads between its "
    "joints and of its axial force on the sagitta of its curve. A member that the carpentry lands on another's face just "
    "beside a joint, to keep it from running alongside, meets that joint in the force model, the member landed on bending "
    "under the landing force on its eccentricity. Only designs that are mechanisms when pin-jointed (Bow's "
    "deficient Class II, most of Class IV) are analysed as frames with stiff joints, carrying by bending. Sections are "
    "square GL24h, N/A + M/W within the design strength, struts with a buckling factor, at least 100 mm for chords and "
    "60 mm for webs, one section per member. "
    "The span, the load and the largest forces are stored with each graph; the cards show the span.",
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
structs = re.findall(r"/// (.*)\nstruct (\w+) \{\n((?:    .*\n)+?)\};", designs)
library = [
    f"The {len(structs)} designs of the library, each a struct of its parameters and its own `TrussGuide` overload, pictured "
    "with their defaults as `templates_truss_2_library` builds them, each its own scene `truss_<name>.pb`. Every design "
    "also takes `.chord_radius = 80` and `.web_radius = 60`, the half sections of the chords and posts and of the webs.",
    "",
    '<div class="wood-card-grid cols-4">',
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
