"""compas_tf examples 4 and 5 without the viewer: every quarter and oculus element as `name volume cx cy cz` in world coordinates."""
import math, sys
from compas.geometry import Point, Rotation, Translation, Vector
from compas_model.elements import Group
from compas_tf.floor_guide import FloorGuide
from compas_tf.model import TFModel

guide = FloorGuide(size_grid_x=3000, size_grid_y=3000, size_column_head=220, size_column_head_chamfer=120, size_outer_ribs=100, size_inner_ribs=60, size_inner_beams=60, size_wedge=240, height=650, rise=453, size_oculus=1000)

quarter_model = TFModel(name="quarter_model")
for group_name, plates in [("beds", guide.beds), ("tsections", guide.tsections), ("outer_ribs", guide.outer_ribs), ("inner_ribs", guide.inner_ribs), ("wedges_inner_beams", guide.wedges_inner_beams), ("inner_beams", guide.inner_beams)]:
    group = quarter_model.add_group(group_name)
    if plates and hasattr(plates[0], "bed_row"):
        rows = {}
        for plate in plates:
            rows.setdefault(plate.bed_row, []).append(plate)
        for row_index in sorted(rows):
            rowgroup = quarter_model.add_element(Group(name=f"{group_name}_{row_index}"), parent=group)
            for i, plate in enumerate(rows[row_index]):
                plate.name = f"{group_name}_{row_index}_{i}"
                quarter_model.add_element(plate, parent=rowgroup)
    else:
        for i, plate in enumerate(plates):
            plate.name = f"{group_name}_{i}"
            quarter_model.add_element(plate, parent=group)
quarter_model.transformation = Translation.from_vector([0, 0, guide.bay_height])

models = []
for i in range(4):
    copy = quarter_model.duplicate()
    copy.transformation = Rotation.from_axis_and_angle(Vector(0, 0, 1), i * math.pi / 2, Point(0, 0, 0)) * copy.transformation
    for element in copy.elements():
        element.name = f"{element.name}_{i}"
    models.append(copy)

oculus_model = TFModel(name="oculus_model")
group = oculus_model.add_group("oculus")
for i, plate in enumerate(guide.oculus):
    plate.name = f"oculus_{i}"
    oculus_model.add_element(plate, parent=group)
oculus_model.transformation = Translation.from_vector([0, 0, guide.bay_height])
models.append(oculus_model)

with open(sys.argv[1], "w") as f:
    for model in models:
        for element in model.elements():
            if isinstance(element, Group):
                continue
            mesh = element.modelgeometry
            box = mesh.aabb()
            c = box.frame.point
            f.write("%s %.6f %.6f %.6f %.6f\n" % (element.name, abs(mesh.volume()), c[0], c[1], c[2]))
print("written")
