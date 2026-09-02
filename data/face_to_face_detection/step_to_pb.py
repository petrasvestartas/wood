"""STEP -> one .pb of closed polylines, grouped one group per solid.

    .venv/bin/python step_to_pb.py                          # the compas_tf floor system
    .venv/bin/python step_to_pb.py <in.stp> <out.pb>        # anything else

That grouping IS the format main_face_to_face.cpp reads in `Case::Blocks`: each child of
the tree root is one element, its children are that element's closed face loops. Nothing
says which loop is bottom and which is top, which is exactly why those become BlockElements
and only contact detection runs on them.

Needs the .venv sitting next to this file:
    uv venv --python 3.13 && uv pip install compas compas_occt session_py
"""

import sys
import time
from pathlib import Path

from compas.geometry import Brep

import session_py as sp

HERE = Path(__file__).parent
REPO = HERE.parent.parent.parent                      # wood_research/

# The compas_tf floor system as fabricated: columns, ribs, beams, oculus, connectors.
DEFAULT_STEP = REPO / "compas_tf" / "data" / "fabrication" / "model_0_fab.stp"
DEFAULT_OUT = REPO / "wood" / "data" / "floor_model.pb"


def polygon_to_polyline(polygon):
    """compas Polygon -> closed session_py Polyline."""
    points = [sp.Point(float(p[0]), float(p[1]), float(p[2])) for p in polygon.points]
    points.append(sp.Point(points[0].x, points[0].y, points[0].z))   # close the loop
    return sp.Polyline(points)


def main():
    step = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_STEP
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_OUT
    if not step.exists():
        sys.exit(f"not found: {step}")

    t = time.time()
    brep = Brep.from_step(str(step))
    print(f"{step.name}: {len(brep.solids)} solids, {len(brep.faces)} faces "
          f"({time.time() - t:.1f} s)")

    session = sp.Session(step.stem)
    total = 0
    skipped = 0
    for i, solid in enumerate(brep.solids):
        group = session.add_group(f"solid_{i}")
        for polygon in solid.to_polygons():
            # A face that came back with fewer than three points is a degenerate loop -
            # a seam or a sliver - and would only be a zero-area face in the detector.
            if len(polygon.points) < 3:
                skipped += 1
                continue
            session.add_polyline(polygon_to_polyline(polygon), parent=group)
            total += 1

    out.parent.mkdir(parents=True, exist_ok=True)
    session.pb_dump(str(out))
    print(f"{total} polylines from {len(brep.solids)} solids -> {out} "
          f"({out.stat().st_size / 1e6:.1f} MB)" + (f", {skipped} degenerate skipped" if skipped else ""))


if __name__ == "__main__":
    main()
