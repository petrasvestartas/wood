"""Move every permalinked file:lines citation on the floor pages from its old commit to a new one, the line numbers carried through the diff; print the citations whose lines changed."""
import difflib, pathlib, re, subprocess, sys

ROOT = pathlib.Path("/home/pv/brg/code_cpp/wood_research")
REPOS = {"wood": ROOT / "wood", "session_cpp": ROOT / "session/session_cpp"}
LINK = re.compile(r"\[(?P<text>[^\]]+)\]\(https://github\.com/petrasvestartas/(?P<repo>wood|session_cpp)/blob/(?P<sha>[0-9a-f]{40})/(?P<path>[^#)]+)#L(?P<a>\d+)(?:-L(?P<b>\d+))?\)")
NEW = {"wood": sys.argv[1], "session_cpp": sys.argv[2]}
cache = {}

def lines(repo, sha, path):
    key = (repo, sha, path)
    if key not in cache:
        out = subprocess.run(["git", "-C", str(REPOS[repo]), "show", f"{sha}:{path}"], capture_output=True, text=True)
        cache[key] = out.stdout.split("\n") if out.returncode == 0 else None
    return cache[key]

def mapping(repo, old, new, path):
    a, b = lines(repo, old, path), lines(repo, NEW[repo], path)
    if a is None or b is None:
        return None, set()
    m, changed = {}, set()
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, a, b, autojunk=False).get_opcodes():
        for k in range(i2 - i1):
            if tag == "equal":
                m[i1 + k + 1] = j1 + k + 1
            else:
                changed.add(i1 + k + 1)
                m[i1 + k + 1] = min(j1 + k, j2 - 1) + 1 if j2 > j1 else j1 + 1
    return m, changed

flagged = []
for page in sorted((ROOT / "wood/docs/templates").glob("floor_*.md")):
    text = page.read_text()
    def sub(match):
        repo, sha, path = match["repo"], match["sha"], match["path"]
        a = int(match["a"]); b = int(match["b"] or a)
        m, changed = mapping(repo, sha, sha, path) if False else mapping(repo, sha, NEW[repo], path)
        if m is None:
            flagged.append((page.name, match["text"], "file missing"))
            return match.group(0)
        na, nb = m.get(a, a), m.get(b, b)
        if any(l in changed for l in range(a, b + 1)):
            flagged.append((page.name, match["text"], f"{path}:{a}-{b} -> {na}-{nb} CHANGED"))
        t = match["text"]
        old_r = f"{a}-{b}" if match["b"] else f"{a}"
        new_r = f"{na}-{nb}" if match["b"] else f"{na}"
        t = t[: len(t) - len(old_r)] + new_r if t.endswith(old_r) else t
        anchor = f"#L{na}-L{nb}" if match["b"] else f"#L{na}"
        return f"[{t}](https://github.com/petrasvestartas/{repo}/blob/{NEW[repo]}/{path}{anchor})"
    page.write_text(LINK.sub(sub, text))
for f in flagged:
    print(*f, sep=" | ")
print(len(flagged), "flagged")
