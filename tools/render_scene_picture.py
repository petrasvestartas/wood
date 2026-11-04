"""One written scene as a docs picture: a screenshot of the viewer, cropped, saved lossless.

    python tools/render_scene_picture.py <scene.pb> <out.webp> [opacity]
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import render_template_docs as docs  # noqa: E402

opacity = sys.argv[3] if len(sys.argv) > 3 else "1.0"
docs.render(pathlib.Path(sys.argv[1]).resolve(), "iso", opacity, "1600x1000").save(sys.argv[2], lossless=True)
