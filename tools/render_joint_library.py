"""The pictures of the beam joint library and their page: each design's picture and assembly GIF, drawn alike by
tools/render_joint_library_gifs.py, then the table of docs/elements/element_joint_beam_compendium.md between its joint-table
markers (title, then GIF, picture and joinery tile, by page) and the list of the book's joints still to do (tools/joint_library_todo.py).

    python3 tools/render_joint_library.py [design ...]
"""

import pathlib
import re
import sys

import joint_library_todo
import render_joint_library_gifs

WOOD = pathlib.Path(__file__).resolve().parents[1]
PICTURES = WOOD / "docs/images/joint_library"
PAGE = WOOD / "docs/elements/element_joint_beam_compendium.md"


def write_table() -> None:
    """The page's table between its joint-table markers: per record of data/joints/*.yaml its title and page in a row of their
    own, then its assembly GIF, picture and tile side by side."""
    rows = ["| assembly | joint | tile |", "|---|---|---|"]
    records = []
    for chapter in sorted((WOOD / "data/joints").glob("*.yaml")):
        records += re.findall(r'^([a-z_0-9]+):\n  title: "(.*?)"\n  german: "(.*?)"\n  page: "(.*?)"', chapter.read_text(), re.M)
    # in the order of the book's pages
    for design, title, german, page in sorted(records, key=lambda record: int(record[3])):
        gif = f"![](joint_library/beam_{design}.gif)" if (PICTURES / f"beam_{design}.gif").exists() else ""
        tile = f"![](joint_library/tile_{design}.png)" if (PICTURES / f"tile_{design}.png").exists() else ""
        rows.append(f"| **{title}**, p. {page} | | |")
        rows.append(f"| {gif} | ![](joint_library/beam_{design}.png) | {tile} |")
    text = PAGE.read_text()
    start = text.index("<!-- joint-table -->") + len("<!-- joint-table -->")
    PAGE.write_text(text[:start] + "\n" + "\n".join(rows) + "\n" + text[text.index("<!-- /joint-table -->"):])


def main(arguments: list) -> None:
    """Saves the picture and the GIF of each design (every design when none is named) by render_joint_library_gifs, then
    rewrites the page's table and the list of joints still to do."""
    render_joint_library_gifs.main(arguments)
    write_table()
    joint_library_todo.main()


if __name__ == "__main__":
    main(sys.argv[1:])
