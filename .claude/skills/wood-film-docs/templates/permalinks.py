"""Turn every file:lines citation on the Code: lines of the floor chapter pages into a GitHub permalink at a fixed commit."""
import pathlib
import re
import sys

ROOT = pathlib.Path("/home/pv/brg/code_cpp/wood_research")
WOOD_SHA = "0c9f4e49b3ac503e917b90c037f43f827533d641"
KERNEL_SHA = "9919e7f7fd09acd90feea9dccf72aa63cf083fbf"
CITE = re.compile(r"(?P<kernel>session_cpp )?(?P<file>[A-Za-z0-9_./]+\.(?:cpp|h)):(?P<ranges>\d+(?:-\d+)?(?:, ?\d+(?:-\d+)?)*)")


def locate(name: str, kernel: bool) -> str | None:
    """The GitHub URL of a cited file: in wood, src/ first when a name is also a test's; else in the session_cpp kernel; None when not found once."""
    if name.startswith("session_cpp/"):
        name, kernel = name[len("session_cpp/"):], True
    if kernel:
        hits = list((ROOT / "session/session_cpp/src").rglob(pathlib.Path(name).name))
        return f"https://github.com/petrasvestartas/session_cpp/blob/{KERNEL_SHA}/{hits[0].relative_to(ROOT / 'session/session_cpp')}" if len(hits) == 1 else None
    wood = ROOT / "wood"
    hits = [p for p in wood.rglob(pathlib.Path(name).name) if "build" not in p.parts and "_deps" not in p.parts and (name.count("/") == 0 or str(p).endswith(name))]
    hits = [p for p in hits if p.parts[len(wood.parts)] in ("src", "examples", "tests", "docs")]
    if len(hits) > 1:
        hits = [p for p in hits if p.parts[len(wood.parts)] == "src"]
    if not hits:
        return locate(name, True)
    return f"https://github.com/petrasvestartas/wood/blob/{WOOD_SHA}/{hits[0].relative_to(wood)}" if len(hits) == 1 else None


def link(match: re.Match, unresolved: set) -> str:
    url = locate(match["file"], bool(match["kernel"]))
    if url is None:
        unresolved.add(match.group(0))
        return match.group(0)
    parts = []
    for i, r in enumerate(re.split(r", ?", match["ranges"])):
        a, _, b = r.partition("-")
        anchor = f"#L{a}-L{b}" if b else f"#L{a}"
        text = f"{match['kernel'] or ''}{match['file']}:{r}" if i == 0 else r
        parts.append(f"[{text}]({url}{anchor})")
    return ", ".join(parts)


def main(paths: list) -> int:
    unresolved, count = set(), 0
    for path in map(pathlib.Path, paths):
        lines = path.read_text().split("\n")
        for i, line in enumerate(lines):
            if line.startswith("Code:") and "](http" not in line:
                new = CITE.sub(lambda m: link(m, unresolved), line)
                count += new != line
                lines[i] = new
        path.write_text("\n".join(lines))
    print(f"{count} Code: lines linked; unresolved: {sorted(unresolved)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
