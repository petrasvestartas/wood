"""The docs pages, checked without building anything.

Element and joint pages: the constructors the only code block, every case a picture and a whole example.
Every page: each picture it shows exists, each picture under docs/images is shown by a page, and each is made by a recipe
(tools/README.md says which command makes which folder).
"""
import pathlib
import re
import shlex
import sys
import urllib.parse

root = pathlib.Path(__file__).resolve().parent.parent
tools = root / "tools"
targets = set(re.findall(r"ADD_EXE\((\w+)", (root / "CMakeLists.txt").read_text(encoding="utf-8")))
TEXT_ONLY = {"Constructors", "Parameters", "Families", "Contact Parameters", "Assembly"}
IMAGE = r"[\w./%@\-]+\.(?:png|webp|gif|jpg)"
# pictures no recipe line makes yet (made by hand or by a tool outside tools/); each must go when its recipe lands
UNRECIPED = {
    "two_plates.png",
    "templates_shells_2_custom_curves.png",
    "templates_truss_4_custom_wireframe.png",
    "landing_elements.webp",
    "landing_templates.webp",
    "landing_examples.webp",
    "hilti_reference_photo.jpg",
}
failures = []


def check_element_page(page: pathlib.Path) -> None:
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


def recipe_pictures() -> set:
    """The picture names the lines of render_template_pictures.sh make: <example>_<name or last group>.webp."""
    made = set()
    values = {"--view", "--opacity", "--size", "--wheel", "--name", "--scene", "--aspect"}
    for line in (tools / "render_template_pictures.sh").read_text(encoding="utf-8").splitlines():
        if '"${RENDER[@]}"' not in line:
            continue
        words = shlex.split(line.split('"${RENDER[@]}"', 1)[1])
        options, rest, i = {}, [], 0
        while i < len(words):
            if words[i] in values:
                options[words[i]] = words[i + 1]
                i += 2
            elif words[i] == "--attributes":
                i += 1
            else:
                rest.append(words[i])
                i += 1
        for group in rest[1:] or ["all"]:
            made.add(f"{rest[0]}_{options.get('--name') or group.split('+')[-1]}.webp")
    # the vault: one run, a top and a bottom view of every bay named after the first colon in the loop
    loop = re.search(r"for bay in ([^;]+); do", (tools / "render_template_pictures.sh").read_text(encoding="utf-8"))
    for pair in loop.group(1).split():
        name = pair.split(":")[1]
        made |= {f"templates_vault_9_gothic_ribbed_vault_{name}.webp", f"templates_vault_9_gothic_ribbed_vault_{name}_bottom.webp"}
    return made


def element_pictures() -> set:
    """Every example of the element pages, shot by screenshot_element_docs.sh, and the overviews it copies."""
    script = (tools / "screenshot_element_docs.sh").read_text(encoding="utf-8")
    overviews = re.search(r"<<'OVERVIEWS'\n(.*?)\nOVERVIEWS", script, re.S).group(1)
    made = {f"{name}.png" for name in targets if name.startswith("element_")}
    made |= {f"{line.split()[0]}.png" for line in overviews.splitlines()}
    return made


def is_made(path: pathlib.PurePosixPath, recipes: dict) -> bool:
    folder, name = path.parts[0], path.name
    if folder == "templates":
        return (
            name in recipes["templates"]
            or (name.startswith("truss_") and (tools / "render_template_pictures.sh").exists())
            or (name.endswith("_assembly.gif") and name[:-4] in targets)
            or name == "templates_truss_1_pratt.png"
        )
    if folder == "elements":
        return name in recipes["elements"]
    if folder == "joint_library":
        return (tools / "joint_library_figures" / "make.sh").exists()
    if folder == "examples" and name.startswith("ex_"):
        return name[3:].rsplit(".", 1)[0] in recipes["examples"]
    # diagrams, landing and the rest: the name is written in a picture script
    return path.stem in recipes["scripts"] or name.startswith("wood_elements_sheet_")


def check_pictures() -> None:
    pages = [p for p in (root / "docs").rglob("*.md") if "plans" not in p.parts]
    pages += [root / "README.md", root / "src" / "docs.md"] + list((root / "docs" / "doxygen").glob("*.html"))
    shown = {}
    for page in pages:
        for ref in re.findall(IMAGE, page.read_text(encoding="utf-8")):
            shown.setdefault(urllib.parse.unquote(ref).split("/")[-1], page.name)
    images = {}
    for path in (root / "docs").rglob("*"):
        if path.suffix in (".png", ".webp", ".gif", ".jpg") and "plans" not in path.parts:
            images.setdefault(path.name, path)
    for name, page in sorted(shown.items()):
        if name not in images:
            failures.append(f"{page}: picture {name} does not exist")

    scripts = "".join(p.read_text(encoding="utf-8") for p in tools.glob("*.py"))
    architecture = (tools / "render_architecture_pictures.py").read_text(encoding="utf-8")
    recipes = {
        "templates": recipe_pictures(),
        "elements": element_pictures(),
        "examples": set(re.findall(r'^\s+"(\d+_\w+)": \(', architecture, re.M)),
        "scripts": scripts,
    }
    gallery = root / "docs" / "images"
    for path in sorted(gallery.rglob("*")):
        if not path.is_file():
            continue
        relative = pathlib.PurePosixPath(path.relative_to(gallery).as_posix())
        if path.name not in shown:
            failures.append(f"{relative}: no page shows this picture")
        if path.name not in UNRECIPED and not is_made(relative, recipes):
            failures.append(f"{relative}: no recipe line makes this picture")


for page in sorted((root / "docs" / "elements").glob("*.md")):
    check_element_page(page)
check_pictures()

for failure in failures:
    print(failure)
print(f"{len(failures)} problems")
sys.exit(1 if failures else 0)
