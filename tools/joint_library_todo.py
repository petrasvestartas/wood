"""The joints of the Joinery Compendium the beam joint library does not have yet: every line of data/joints/compendium_index.txt
whose page no data/joints/*.yaml record names, written to docs/joint_library_todo.md by chapter, the counts printed.

    python3 tools/joint_library_todo.py
"""

import pathlib
import re

WOOD = pathlib.Path(__file__).resolve().parents[1]
INDEX = WOOD / "data/joints/compendium_index.txt"
PAGE = WOOD / "docs/joint_library_todo.md"
CHAPTERS = {
    "splicing": "Timber splicing joints", "corner": "Timber corner joints", "t": "Timber T-joints", "angled": "Timber angled joints",
    "cross_lap": "Timber cross lap joints", "specialty": "Timber specialty joints", "case_edge": "Case edge joints",
    "case_end": "Case end joints", "case_corner": "Case corner joints", "case_t": "Case T-joints",
}


def main() -> None:
    """Writes the page and prints done, left and total per chapter."""
    done = set()
    for chapter in (WOOD / "data/joints").glob("*.yaml"):
        done.update(int(page) for page in re.findall(r'^  page: "(\d+)"', chapter.read_text(), re.M))
    joints = [line.split(" | ") for line in INDEX.read_text().splitlines() if line and not line.startswith("#")]
    left = [joint for joint in joints if int(joint[1]) not in done]
    lines = [
        "# Joint library: what is left {#elements_joint_library_todo}", "",
        f"The joints of the Joinery Compendium (Bauer, Pauli 2024) the beam joint library has no record of yet: {len(left)} of "
        f"{len(joints)}. Written by `python3 tools/joint_library_todo.py` from `data/joints/compendium_index.txt`; a joint leaves "
        "the list when a `data/joints/*.yaml` record names its page.", "",
    ]
    for key, name in CHAPTERS.items():
        chapter = [joint for joint in joints if joint[0] == key]
        missing = [joint for joint in left if joint[0] == key]
        print(f"{name}: {len(chapter) - len(missing)} done, {len(missing)} left of {len(chapter)}")
        lines += [f"## {name}: {len(missing)} left of {len(chapter)}", ""]
        lines += [f"- p. {page}, {title} ({german})" for _, page, title, german in missing] or ["All done."]
        lines.append("")
    PAGE.write_text("\n".join(lines))
    print(f"total: {len(joints) - len(left)} done, {len(left)} left of {len(joints)}")


if __name__ == "__main__":
    main()
