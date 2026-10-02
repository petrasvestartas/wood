"""compas_tf example 6 without the viewer: wedge contacts, connectors and carved ring plates as `name volume cx cy cz`."""
import sys
exec(open("reference_models.py").read().split("with open(sys.argv[1]")[0])
from compas.geometry import Point, Vector
from compas_tf.connectors import ConnectorWedgeElement
from compas_tf.plate import PlateElement
from compas_tf.solid_difference_modifier import CylinderCutFeature, PrismCutFeature

floor = TFModel(name="floor_model").merge(models)

def is_joint_ring(element):
    if not isinstance(element, PlateElement):
        return False
    name = element.name or ""
    if name.startswith("inner_beams_"):
        return True
    if name.startswith("oculus_"):
        return int(name.rsplit("_", 1)[-1]) < 4
    return False

plates = [el for el in floor.elements() if is_joint_ring(el)]
contacts = []
for i in range(len(plates)):
    for j in range(i + 1, len(plates)):
        a, b = plates[i], plates[j]
        for contact in a.compute_contacts(b, tolerance=1.0, minimum_area=1.0, face_kinds={"top", "bottom"}) or []:
            contacts.append((a, b, contact))

out = open(sys.argv[1], "w")
out.write("contacts %d\n" % len(contacts))
parts = []
for k, (a, b, contact) in enumerate(contacts):
    thickness = max(a.computed_thickness, b.computed_thickness)
    wedge = ConnectorWedgeElement.from_contact(contact, length_margin=1.5 * thickness, cylinder_radius=10.0, cylinder_spacing=320.0, cylinder_sides=8, name=f"connector_wedge_{k}")
    cylinders = wedge.create_cylinders()
    out.write("wedge %d %s %s thickness %.6f length %.6f dowels %d area %.6f\n" % (k, a.name, b.name, thickness, wedge.length, len(cylinders), contact.polygon.area))
    floor.add_element(wedge)
    parts.append(wedge)
    for cylinder in cylinders:
        floor.add_element(cylinder)
    parts.extend(cylinders)
    outlines = wedge.inclined_face_box_outlines(depth=2.0 * thickness / 3.0)
    cnormal = Vector(*contact.polygon.normal)
    ccenter = Point(*contact.polygon.centroid)
    dowels = [(cyl.axis, cyl.radius, cyl.sides) for cyl in cylinders]
    for plate in (a, b):
        side = Vector.from_start_end(ccenter, Point(*plate.modelgeometry.centroid())).dot(cnormal)
        box_bottom, box_top = outlines[1] if side >= 0 else outlines[0]
        to_local = plate.modeltransformation.inverted()
        plate.add_feature(PrismCutFeature(box_bottom.transformed(to_local), box_top.transformed(to_local), name=f"connector_{k}_box"))
        for j, (axis, radius, sides) in enumerate(dowels):
            plate.add_feature(CylinderCutFeature(axis.transformed(to_local), radius, sides=sides, name=f"connector_{k}_dowel_{j}"))

for element in plates + parts:
    element._modelgeometry = None
    mesh = element.modelgeometry
    c = mesh.aabb().frame.point
    out.write("%s %.6f %.6f %.6f %.6f\n" % (element.name, abs(mesh.volume()), c[0], c[1], c[2]))
print("written", len(contacts), "contacts")
