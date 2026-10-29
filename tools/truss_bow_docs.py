"""Write docs/templates/truss_bow.md, the gallery of Bow's trusses, from data/templates/truss_bow_1873.json and the pictures
of tools/shoot_truss_bow.sh: one card per truss, grouped by kind and by Bow's class.
    python3 -I wood/tools/truss_bow_docs.py
"""
import json, pathlib

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
    "# Bow 1873 {#templates_truss_bow}",
    "",
    "[TOC]",
    "",
    f"The {len(data)} roof trusses and framed bridges of R. H. Bow, *Economics of Construction in relation to Framed "
    "Structures* (London: Spon, 1873), Part I, redrawn from the drawings of its classification tables. Each truss is "
    "stored as a graph in `data/templates/truss_bow_1873.json` (nodes in spans from the left support, x along the span "
    "and z up; chords as node sequences, webs and posts as node pairs) and built by the one `Truss` template: "
    "`TrussGraph::guide` draws the graph at its span with a section per member, and `Truss` makes the members and "
    "their butts and laps.",
    "",
    "```cpp",
    "const std::vector<wood_truss::TrussGraph> graphs = wood_truss::TrussGraph::library(",
    "    config::session_data_dir() / \"templates\" / \"truss_bow_1873.json\");",
    "for (const wood_truss::TrussGraph& graph : graphs) {",
    "    wood_truss::Truss truss(graph.guide(), \"truss_\" + graph.name);",
    "    truss.pb_dump(pb_path(\"truss_\" + graph.name));",
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
    "## Scale and sections {#truss_bow_sizing}",
    "",
    "Bow gives the span of only a few designs (Nos. 130, 137, 192, 195, 293, 332); his diagrams of forces are drawn to "
    "no absolute scale. Every other design is drawn at its number of sub-spans times 8 ft (2.44 m), the sub-span of his "
    "stated examples (No. 130: 86 1/4 ft over 12 sub-spans; No. 137: 97 1/2 ft over 12; No. 332: 204 ft over 24); the "
    "arched roofs whose sub-spans he calls irregular (Nos. 193, 194, 196, 197) are taken at 100 ft. "
    "Each member's section is then sized, per truss, for timber trusses 3 m apart: roofs 1.5 kN/m2 on the top chord, "
    "bridges 5 kN/m2 on the deck chord, both x 1.4; a frame analysis with stiff joints gives every member's axial force "
    "and bending moment (so the deficient Class II frames are sized for the bending they rely on, and the redundant "
    "Class III ones for the share their stiffness takes); square GL24h sections satisfy N/A + M/W within the design "
    "strength, struts with a buckling factor, at least 100 mm for chords and 60 mm for webs, one section per chord. "
    "The span, the load and the largest forces are stored with each graph; the cards show the span.",
    "",
]
for kind in ("roof", "bridge"):
    for cls in ("I", "II", "III", "IV"):
        group = [t for t in data if t["kind"] == kind and t["class"] == cls]
        if not group:
            continue
        lines += [f"## {KINDS[kind]}, Class {cls} {{#truss_bow_{kind}_{cls.lower()}}}", "", CLASSES[cls], "",
                  '<div class="wood-card-grid cols-4">']
        for t in group:
            lines += [f'  <div class="wood-card">',
                      f'    <img src="truss_{t["name"]}.webp" alt="Bow No. {t["number"]}" loading="lazy">',
                      f'    <span class="wood-card-title">No. {t["number"]} · {t["span"] / 1000:.1f} m · p. {t["page"]}</span>',
                      "  </div>"]
        lines += ["</div>", ""]
(WOOD / "docs/templates/truss_bow.md").write_text("\n".join(lines))
print(len(data), "cards")
