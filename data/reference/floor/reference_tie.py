"""compas_tf example 8, second half: rib seam contacts, the OBJ seam connectors, the outer ribs carved by both connectors."""
import sys
src = open("reference_rectangle.py").read()
exec(src.split("for element in touched:")[0].replace('out = open(sys.argv[1], "w")', 'out = open("/dev/null", "w")'))
from compas_tf.connectors import OuterRibConnectorElement

for edge in list(cantilever.graph.edges()):
    cantilever.graph.edge_attribute(edge, name="contacts", value=[])
cantilever.compute_contacts_between_groups([f"outer_ribs_{i}" for i in range(4)])
rib_contacts = []
for edge in cantilever.graph.edges():
    for contact in cantilever.graph.edge_attribute(edge, name="contacts") or []:
        rib_contacts.append((cantilever.graph.node_element(edge[0]), cantilever.graph.node_element(edge[1]), contact))

out = open(sys.argv[1], "w")
out.write("contacts %d\n" % len(rib_contacts))
for i, (a, b, contact) in enumerate(rib_contacts):
    tie = OuterRibConnectorElement.from_contact(contact, name=f"outer_rib_connector_{i}")
    cantilever.add_element(tie)
    o = Point(0, 0, 0).transformed(tie.transformation)
    y = Vector(0, 1, 0).transformed(tie.transformation)
    out.write("tie %d %s %s area %.6f origin %.6f %.6f %.6f y %.6f %.6f %.6f\n" % (i, a.name, b.name, contact.polygon.area, o[0], o[1], o[2], y[0], y[1], y[2]))
    for rib in (a, b):
        cutter = tie.cutter_for(rib.modelgeometry.centroid())
        rib.add_cutters([cutter.transformed(rib.modeltransformation.inverted())], name=f"outer_rib_connector_{i}_cut")
    body = tie.modelgeometry
    c = body.aabb().frame.point
    out.write("outer_rib_connector_%d %.6f %.6f %.6f %.6f\n" % (i, abs(body.volume()), c[0], c[1], c[2]))

for element in cantilever.elements():
    if isinstance(element, PlateElement) and (element.name or "").startswith("outer_ribs_"):
        element._modelgeometry = None
        mesh = element.modelgeometry
        c = mesh.aabb().frame.point
        out.write("%s %.6f %.6f %.6f %.6f\n" % (element.name, abs(mesh.volume()), c[0], c[1], c[2]))
print("written", len(rib_contacts), "seam contacts")
