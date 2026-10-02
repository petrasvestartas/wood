"""compas_tf example 8, first half, without the viewer: column-rib contacts, the box connectors, the carved columns and outer ribs."""
import math, sys
src = open("reference_wedges.py").read()
exec(src.split("out = open(sys.argv[1]")[0])
from compas.geometry import Point, Rotation, Translation, Vector
from compas_tf.column import ColumnElement
from compas_tf.connectors import ConnectorElement
from compas_tf.plate import PlateElement
from compas_tf.support import SupportElement

# example 6 cuts on the ring plates, as reference_wedges does
for k, (a, b, contact) in enumerate(contacts):
    thickness = max(a.computed_thickness, b.computed_thickness)
    wedge = ConnectorWedgeElement.from_contact(contact, length_margin=1.5 * thickness, cylinder_radius=10.0, cylinder_spacing=320.0, cylinder_sides=8, name=f"connector_wedge_{k}")
    floor.add_element(wedge)
    cylinders = wedge.create_cylinders()
    outlines = wedge.inclined_face_box_outlines(depth=2.0 * thickness / 3.0)
    cnormal = Vector(*contact.polygon.normal); ccenter = Point(*contact.polygon.centroid)
    for plate in (a, b):
        side = Vector.from_start_end(ccenter, Point(*plate.modelgeometry.centroid())).dot(cnormal)
        bb, bt = outlines[1] if side >= 0 else outlines[0]
        to_local = plate.modeltransformation.inverted()
        plate.add_feature(PrismCutFeature(bb.transformed(to_local), bt.transformed(to_local)))
        for axis, radius, sides in [(c.axis, c.radius, c.sides) for c in cylinders]:
            plate.add_feature(CylinderCutFeature(axis.transformed(to_local), radius, sides=sides))

column_model = TFModel(name="column_model")
support = SupportElement(name="support")
column = ColumnElement(width=guide.size_column_head, depth=guide.size_column_head, height=guide.bay_height - SupportElement.HEIGHT, transformation=Translation.from_vector([0, 0, SupportElement.HEIGHT]), capitel_width=guide.size_column_head_chamfer, capitel_height=abs(guide.column_head_lowest_height), name="column")
column.add_cutters(guide.column_cutters_for(column.height))
column_model.add_element(support); column_model.add_element(column); column_model.add_interaction(support, column)
column_model.transformation = Translation.from_vector(guide.corner_point_column(guide.size_column_head))
column_models = []
for i in range(4):
    copy = column_model.duplicate()
    copy.transformation = Rotation.from_axis_and_angle(Vector(0, 0, 1), i * math.pi / 2, Point(0, 0, 0)) * copy.transformation
    copy.name = f"column_model_{i}"
    for element in copy.elements():
        element.name = f"{element.name}_{i}"
    column_models.append(copy)
columns_model = TFModel(name="columns_model").merge(column_models)

cantilever = TFModel(name="cantilever_model").merge([floor, columns_model])
for edge in list(cantilever.graph.edges()):
    cantilever.graph.edge_attribute(edge, name="contacts", value=[])
cantilever.compute_contacts_between_groups(["columns_model"], groups_b=[f"outer_ribs_{i}" for i in range(4)])

edge_contacts = []
for edge in cantilever.graph.edges():
    for contact in cantilever.graph.edge_attribute(edge, name="contacts") or []:
        edge_contacts.append((cantilever.graph.node_element(edge[0]), cantilever.graph.node_element(edge[1]), contact))

out = open(sys.argv[1], "w")
out.write("contacts %d\n" % len(edge_contacts))
touched = []
for i, (a, b, contact) in enumerate(edge_contacts):
    rib = a if isinstance(a, PlateElement) else b
    col = b if rib is a else a
    connector = ConnectorElement.from_contact(contact, toward=Point(*rib.modelgeometry.centroid()), name=f"connector_{i}")
    cantilever.add_element(connector)
    box = connector.cutter_mesh(overshoot=25.0)
    column_cylinders, rib_cylinders = connector.cylinder_cutters(rib.computed_thickness)
    col.add_cutters([box.transformed(col.modeltransformation.inverted())] + [c.transformed(col.modeltransformation.inverted()) for c in column_cylinders])
    rib.add_cutters([box.transformed(rib.modeltransformation.inverted())] + [c.transformed(rib.modeltransformation.inverted()) for c in rib_cylinders])
    o = Point(0, 0, 0).transformed(connector.transformation)
    x = Vector(1, 0, 0).transformed(connector.transformation)
    out.write("connector %d %s %s area %.6f thickness %.6f origin %.6f %.6f %.6f x %.6f %.6f %.6f\n" % (i, col.name, rib.name, contact.polygon.area, rib.computed_thickness, o[0], o[1], o[2], x[0], x[1], x[2]))
    touched += [col, rib]

for element in touched:
    element._modelgeometry = None
for element in {id(e): e for e in touched}.values():
    mesh = element.modelgeometry
    c = mesh.aabb().frame.point
    out.write("%s %.6f %.6f %.6f %.6f\n" % (element.name, abs(mesh.volume()), c[0], c[1], c[2]))
print("written", len(edge_contacts), "contacts")
