import sys, xml.etree.ElementTree as ET
src, out_dir, short, title = sys.argv[1:5]
root = ET.parse(src).getroot()
kids = lambda tag: [e for e in root if e.tag.lower() == tag]
lines, n = [f"# {title}"], 0
for pl in kids("polyline"):
    pts = [(p.find("x").text, p.find("y").text, p.find("z").text) for p in pl.iter("point")]
    lines += [f"v {x} {y} {z}" for x, y, z in pts]
    lines += ["cstype bspline", "deg 1", "curv 0 %d %s" % (len(pts), " ".join(str(n + i + 1) for i in range(len(pts)))),
              "parm u " + " ".join(str(i) for i in [0] + list(range(len(pts)))), "end"]
    n += len(pts)
open(f"{out_dir}/{short}.obj", "w").write("\n".join(lines) + "\n")
def dump(name, rows):
    if rows: open(f"{out_dir}/{short}_{name}.txt", "w").write("\n".join(" ".join(r) for r in rows) + "\n")
dump("insertion_vectors", [[v.find(a).text for v in e.iter("vector") for a in "xyz"] for e in kids("insertion_vectors")])
dump("joints_types", [[i.text for i in e] for e in kids("joints_types")])
dump("three_valence", [[i.text for i in e] for e in kids("three_valence")])
ids = [i.text for e in kids("adjacency") for i in e]
dump("adjacency", [ids[i:i+2] for i in range(0, len(ids), 4)])
print(short, ":", len(kids("polyline")), "polylines,", len(kids("insertion_vectors")), "plates with vectors,", len(kids("three_valence")), "three-valence rows,", len(ids)//4, "adjacency pairs")
