"""Build compas_tf plate geometry, convert each Brep to polylines, write one .pb.

Self-contained: needs only the .venv sitting next to this file
(uv venv --python 3.13 && uv pip install compas_tf compas_occt session_py).

Chain:
    compas_tf.PlateElement  ->  .get_brep()      (compas_occt.OCCBrep)
                            ->  .to_polygons()   (one closed polygon per face)
                            ->  session_py.Polyline
                            ->  Session.pb_dump("face_to_face_detection.pb")

Each element becomes one group in the session, holding that geometry's
polylines, so the per-geometry grouping survives into the .pb.
"""

from pathlib import Path

from compas.geometry import Polygon
from compas_tf import PlateElement, TFModel

import session_py as sp

HERE = Path(__file__).parent
OUT = HERE / "face_to_face_detection.pb"

THICKNESS = 15.0   # mm, matching wood's tolerances (GLOBALS::DISTANCE = 0.1 mm)


def plate(name, corners):
    """A PlateElement from a list of [x, y, z] corners."""
    return PlateElement(polygon=Polygon(corners), thickness=THICKNESS, name=name)


def build_model():
    """Plates that TOUCH, so the contact detector has something to find.

    Two coplanar plates meeting along y = 0, and a third folded up from the
    edge y = -500. Plates that interpenetrate (a wall sunk into a slab) share
    no coplanar face pair and are detected as nothing at all, which is why the
    layout matters here.
    """
    model = TFModel()
    model.add_element(plate("plate_a", [[-500, 0, 0], [500, 0, 0], [500, 500, 0], [-500, 500, 0]]))
    model.add_element(plate("plate_b", [[-500, -500, 0], [500, -500, 0], [500, 0, 0], [-500, 0, 0]]))
    model.add_element(plate("plate_c", [[-500, -500, 0], [500, -500, 0], [500, -1000, 500], [-500, -1000, 500]]))
    return model


def polygon_to_polyline(polygon):
    """compas Polygon -> closed session_py Polyline."""
    points = [sp.Point(float(p[0]), float(p[1]), float(p[2])) for p in polygon.points]
    points.append(sp.Point(points[0].x, points[0].y, points[0].z))  # close the loop
    return sp.Polyline(points)


def main():
    model = build_model()
    session = sp.Session("face_to_face_detection")

    total = 0
    for element in model.elements():
        brep = element.get_brep()
        if brep is None:
            print(f"  {element.name}: no brep, skipped")
            continue

        group = session.add_group(element.name)
        polygons = brep.to_polygons()
        for polygon in polygons:
            session.add_polyline(polygon_to_polyline(polygon), parent=group)

        total += len(polygons)
        print(f"  {element.name}: {len(polygons)} polylines")

    session.pb_dump(OUT)
    print(f"\n{total} polylines from {len(list(model.elements()))} geometries -> {OUT}")


if __name__ == "__main__":
    main()
