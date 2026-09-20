"""Writes docs/api/*.md from the /// docstrings and declarations of the headers under src/joinery_solver."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src" / "joinery_solver"
OUT = ROOT / "docs" / "api"

PAGES = {
    "session.md": ("Session", ["wood_session.h", "wood_view.h", "wood_io.h", "wood_test.h"]),
    "settings.md": ("Settings and config", ["wood_settings.h", "wood_config.h", "wood_serialization.h"]),
    "elements.md": ("Elements", sorted(p.relative_to(SRC).as_posix() for p in (SRC / "wood_elements").glob("*.h"))),
    "interaction.md": ("Interaction", sorted(p.relative_to(SRC).as_posix() for p in (SRC / "wood_interaction").rglob("*.h") if "plate_joints/" not in p.as_posix())),
    "algorithms.md": ("Algorithms", sorted(p.relative_to(SRC).as_posix() for p in (SRC / "wood_algorithms").glob("*.h"))),
}
JOINTS = SRC / "wood_interaction" / "wood_interaction_feature" / "wood_interaction_feature_plate_joints"

BANNER = "═══"


def clean(sig):
    sig = re.sub(r"\s+", " ", sig).strip()
    return sig.rstrip("{").strip()


def parse(path):
    """Items of one header: ('type', name, doc), ('banner', text), ('field', name, type, comment), ('method', signature, doc), ('enum_value', name, value, comment), ('end',)."""
    items = []
    doc = []
    depth = 0
    in_type = False
    pending = ""
    lines = path.read_text().split("\n")
    i = 0
    while i < len(lines):
        raw = lines[i]
        line = raw.strip()
        i += 1
        if not line or line.startswith("#") or line.startswith("namespace") or line.startswith("} // namespace") or line.startswith("}  // namespace") or line.startswith("using namespace"):
            continue
        if line.startswith("///"):
            doc.append(line[3:].strip())
            continue
        if line.startswith("//"):
            text = line[2:].strip()
            if text and BANNER not in text and in_type:
                items.append(("banner", text))
            continue
        if "(" in line and "{" in line and line.count("{") == line.count("}") and line.endswith("}") and not pending:
            items.append(("method", clean(line.split("{", 1)[0]), " ".join(doc)))
            doc = []
            continue
        if pending:
            pending += " " + line
            if line.endswith(";") or line.endswith("{"):
                sig = pending
                pending = ""
                items.append(("method", clean(sig), " ".join(doc)))
                doc = []
                if sig.rstrip().endswith("{"):
                    depth += 1
                    # skip the inline body
                    body = 1
                    while body > 0 and i < len(lines):
                        body += lines[i].count("{") - lines[i].count("}")
                        i += 1
                    depth -= 1
            continue
        m = re.match(r"^(struct|class|enum(?: class)?)\s+(\w+)", line)
        if m and (line.endswith("{") or line.endswith("{ public:")):
            items.append(("type", m.group(1), m.group(2), " ".join(doc), line))
            doc = []
            in_type = True
            depth = 1
            continue
        if in_type:
            if line.startswith("}") and line.endswith(";"):
                in_type = False
                items.append(("end",))
                doc = []
                continue
            if line in ("public:", "private:", "protected:"):
                items.append(("access", line[:-1]))
                continue
            mv = re.match(r"^(\w+)\s*=\s*([^,]+),\s*//\s*(.*)$", line)
            if mv and items and any(t[0] == "type" and t[1].startswith("enum") for t in items[-6:]):
                items.append(("enum_value", mv.group(1), mv.group(2).strip(), mv.group(3)))
                continue
            mf = re.match(r"^(.+?)\s+(\w+)(\s*=\s*[^;]+|\{[^;]*\})?;\s*//\s*(.*)$", line)
            if mf and "(" not in mf.group(1):
                items.append(("field", mf.group(2), mf.group(1), mf.group(4), " ".join(doc)))
                doc = []
                continue
        if line.endswith(";") or line.endswith("{"):
            if "(" in line and ")" in line or line.endswith(";"):
                if line.endswith("{") and "(" in line:
                    # inline method with body on following lines
                    items.append(("method", clean(line), " ".join(doc)))
                    doc = []
                    body = line.count("{") - line.count("}")
                    while body > 0 and i < len(lines):
                        body += lines[i].count("{") - lines[i].count("}")
                        i += 1
                    continue
                if line.endswith("{"):
                    continue
                items.append(("method", clean(line), " ".join(doc)))
                doc = []
                continue
        if "(" in line and not line.endswith(";"):
            pending = line
            continue
    return items


def render(rel, items):
    out = [f"## `{rel}`", ""]
    fields = []

    def flush_fields():
        nonlocal fields
        if fields:
            out.append("| Field | Type | Meaning |")
            out.append("|---|---|---|")
            for name, typ, comment, doc in fields:
                out.append(f"| `{name}` | `{typ}` | {comment} |")
            out.append("")
            fields = []

    enum_rows = []

    def flush_enum():
        nonlocal enum_rows
        if enum_rows:
            out.append("| Value | Code | Meaning |")
            out.append("|---|---|---|")
            for name, value, comment in enum_rows:
                out.append(f"| `{name}` | {value} | {comment} |")
            out.append("")
            enum_rows = []

    for it in items:
        kind = it[0]
        if kind == "type":
            flush_fields(); flush_enum()
            out.append(f"### `{it[2]}`")
            out.append("")
            if it[3]:
                out.append(it[3])
                out.append("")
        elif kind == "banner":
            flush_fields(); flush_enum()
            out.append(f"#### {it[1]}")
            out.append("")
        elif kind == "access":
            flush_fields(); flush_enum()
        elif kind == "field":
            fields.append((it[1], it[2], it[3], it[4]))
        elif kind == "enum_value":
            enum_rows.append((it[1], it[2], it[3]))
        elif kind == "method":
            flush_fields(); flush_enum()
            if it[2]:
                out.append(it[2])
                out.append("")
            out.append("```cpp")
            out.append(it[1])
            out.append("```")
            out.append("")
        elif kind == "end":
            flush_fields(); flush_enum()
    flush_fields(); flush_enum()
    return "\n".join(out)


def joints_table():
    rows = []
    for path in sorted(JOINTS.glob("*.h")):
        text = path.read_text()
        m = re.search(r"///\s*(.*?)\n(?:///.*\n)*static (?:void|bool|\w+) (\w+)\(", text)
        doc = m.group(1) if m else ""
        names = re.findall(r"^static \w+ (\w+)\(", text, re.M)
        rows.append((path.name, ", ".join(f"`{n}`" for n in names), doc))
    out = ["## Joint library", "", "One header per variant under `wood_interaction_feature_plate_joints/`, each a `static` function that fills a `FeaturePlate`; the registry in `wood_feature_solver.cpp` maps ids to them.", "", "| Header | Functions | What it builds |", "|---|---|---|"]
    for name, fns, doc in rows:
        out.append(f"| `{name}` | {fns} | {doc} |")
    out.append("")
    return "\n".join(out)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for page, (title, headers) in PAGES.items():
        parts = [f"# {title}", "", f"Generated by `tools/api_docs.py` from the headers; do not edit by hand.", ""]
        for rel in headers:
            parts.append(render(rel, parse(SRC / rel)))
            parts.append("")
        if page == "interaction.md":
            parts.append(joints_table())
        (OUT / page).write_text("\n".join(parts))


if __name__ == "__main__":
    main()
