"""Every element and joint page: the constructors the only code block, every case a picture and a whole example."""
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent
targets = set(re.findall(r"ADD_EXE\((\w+)", (root / "CMakeLists.txt").read_text(encoding="utf-8")))
TEXT_ONLY = {"Constructors", "Parameters", "Families", "Contact Parameters", "Assembly"}
failures = []

for page in sorted((root / "docs" / "elements").glob("*.md")):
    text = page.read_text(encoding="utf-8")
    parts = re.split(r"(?m)^(##+ .+)$", text)
    for i in range(1, len(parts), 2):
        heading = parts[i].lstrip("#").strip()
        body = parts[i + 1]
        if heading in TEXT_ONLY:
            continue
        includes = re.findall(r"[\\@]include(?:\{lineno\})?\s+elements/(\w+)\.cpp", body)
        has_children = i + 2 < len(parts) and parts[i + 2].count("#") and parts[i].count("#") < parts[i + 2].split(" ")[0].count("#")
        if "```cpp" in body:
            failures.append(f"{page.name}: {heading}: a code fragment")
        if not includes and not has_children:
            failures.append(f"{page.name}: {heading}: no example")
        if includes and not re.search(r"!\[[^\]]*\]\(", body):
            failures.append(f"{page.name}: {heading}: no picture")
        for name in includes:
            if name not in targets:
                failures.append(f"{page.name}: {heading}: {name} is no target")

for failure in failures:
    print(failure)
print(f"{len(failures)} problems")
sys.exit(1 if failures else 0)
