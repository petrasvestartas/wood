#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include "file_step.h"

using namespace session_cpp;
using namespace wood_session;

/// Throws with the message when the condition fails.
void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// A 100 x 100 x 50 block with its corner at the origin.
Mesh block() {

    const Polyline bottom({Point(0, 0, 0), Point(100, 0, 0), Point(100, 100, 0), Point(0, 100, 0), Point(0, 0, 0)});

    return Mesh::loft({bottom}, {bottom.translated(Vector(0, 0, 50))}, true);
}

/// The same block as six faces with its top in two coplanar halves meeting at x = 50, so the front and back faces carry a T-vertex on their top sides.
Mesh split_block() {

    Mesh mesh;
    const std::vector<size_t> v = {
        mesh.add_vertex(Point(0, 0, 0)), mesh.add_vertex(Point(100, 0, 0)), mesh.add_vertex(Point(100, 100, 0)), mesh.add_vertex(Point(0, 100, 0)),
        mesh.add_vertex(Point(0, 0, 50)), mesh.add_vertex(Point(50, 0, 50)), mesh.add_vertex(Point(100, 0, 50)),
        mesh.add_vertex(Point(100, 100, 50)), mesh.add_vertex(Point(50, 100, 50)), mesh.add_vertex(Point(0, 100, 50))};

    mesh.add_face({v[0], v[3], v[2], v[1]});
    mesh.add_face({v[4], v[5], v[8], v[9]});
    mesh.add_face({v[5], v[6], v[7], v[8]});
    mesh.add_face({v[0], v[1], v[6], v[4]});
    mesh.add_face({v[3], v[9], v[7], v[2]});
    mesh.add_face({v[0], v[4], v[9], v[3]});
    mesh.add_face({v[1], v[2], v[7], v[6]});

    return mesh;
}

/// Every edge of the solid is used by exactly two edge uses over all its faces, so no face runs an edge another splits.
void check_shared_edges(const BRep& brep, const std::string& name) {

    std::map<int, int> uses;

    for (const BRepFace& face : brep.m_faces)
        for (const BRepRef& wire : face.wires)
            for (const BRepRef& edge : brep.m_wires[wire.index].edges)
                uses[edge.index]++;

    check(uses.size() == brep.m_edges.size(), name + ": " + std::to_string(brep.m_edges.size() - uses.size()) + " edges unused");

    for (const std::pair<const int, int>& use : uses)
        check(use.second == 2, name + ": edge " + std::to_string(use.first) + " used " + std::to_string(use.second) + " times");
}

/// The drilled block: a closed solid with faces planar faces plus cylinders cylindrical ones, every edge shared, at the volume of the block less the holes.
void check_drilled(const Mesh& solid, const std::vector<Drill>& drills, size_t faces, size_t cylinders, double removed, const std::string& name) {

    const std::optional<BRep> brep = drilled_brep(solid, drills);
    check(brep.has_value(), name + ": no exact solid");
    check(brep->is_solid(), name + ": not solid");
    check(static_cast<size_t>(brep->face_count()) == faces, name + ": faces " + std::to_string(brep->face_count()));
    check_shared_edges(*brep, name);

    size_t round = 0;

    for (const NurbsSurface& surface : brep->m_surfaces)
        if (surface.is_rational())
            round++;

    check(round == cylinders, name + ": cylinders " + std::to_string(round));

    const double expected = 500000.0 - removed;
    check(std::abs(brep->volume() - expected) <= 1e-3 * expected, name + ": volume " + std::to_string(brep->volume()) + " not " + std::to_string(expected));
    check(file_step::write_file_step_brep(*brep, pb_path(name).replace(pb_path(name).size() - 3, 3, ".stp")), name + ": step");
}

int main() {

    const double r = 5.0;
    const double area = M_PI * r * r;

    check_drilled(block(), {{Line::from_points(Point(50, 50, -10), Point(50, 50, 60)), r}}, 7, 1, area * 50.0, "drill_through");

    const double tilt = 20.0 * M_PI / 180.0;
    const Vector d(std::sin(tilt), 0.0, std::cos(tilt));
    check_drilled(block(), {{Line::from_points(Point(50, 50, 25) - d * 40.0, Point(50, 50, 25) + d * 40.0), r}}, 7, 1, area * 50.0 / std::cos(tilt), "drill_tilted");

    check_drilled(block(), {{Line::from_points(Point(30, 30, 60), Point(30, 30, 30)), r}}, 8, 1, area * 20.0, "drill_blind");

    check_drilled(block(), {{Line::from_points(Point(30, 30, -10), Point(30, 30, 60)), r}, {Line::from_points(Point(70, 70, 60), Point(70, 70, 20)), 4.0}}, 9, 2, area * 50.0 + M_PI * 16.0 * 30.0, "drill_two");

    check(!drilled_brep(block(), {{Line::from_points(Point(3, 50, -10), Point(3, 50, 60)), r}}).has_value(), "a drill across an edge must fall back");
    check(!drilled_brep(block(), {{Line::from_points(Point(50, 50, -10), Point(50, 50, 60)), r}, {Line::from_points(Point(56, 50, -10), Point(56, 50, 60)), r}}).has_value(), "touching drills must fall back");

    check_drilled(split_block(), {}, 6, 0, 0.0, "split_planar");
    check_drilled(split_block(), {{Line::from_points(Point(25, 50, -10), Point(25, 50, 60)), r}}, 7, 1, area * 50.0, "split_drilled");

    check_drilled(block(), {{Line::from_points(Point(50, 50, 60), Point(50, 50, 25)), r}, {Line::from_points(Point(50, 50, -10), Point(50, 50, 25)), r}}, 7, 1, area * 50.0, "drill_meeting");
    check(merged_drills({{Line::from_points(Point(0, 0, 0), Point(0, 0, 10)), r}, {Line::from_points(Point(0, 0, 10), Point(0, 0, 30)), r}, {Line::from_points(Point(3, 0, 10), Point(3, 0, 30)), r}}).size() == 2, "two drills meeting on one axis merge, an offset one stays");

    const std::vector<std::array<double, 2>> through = inside_stretches(block(), Line::from_points(Point(50, 50, -10), Point(50, 50, 60)));
    check(through.size() == 1 && std::abs(through[0][0] - 10.0) < 1e-9 && std::abs(through[0][1] - 60.0) < 1e-9, "a line through the block runs inside it from 10 to 60");

    const std::vector<std::array<double, 2>> from_inside = inside_stretches(block(), Line::from_points(Point(50, 50, 25), Point(50, 50, 100)));
    check(from_inside.size() == 1 && std::abs(from_inside[0][0] + 25.0) < 1e-9 && std::abs(from_inside[0][1] - 25.0) < 1e-9, "a line from inside the block carries the stretch past its own start");

    check(inside_stretches(block(), Line::from_points(Point(150, 50, -10), Point(150, 50, 60))).empty(), "a line beside the block runs inside nothing");

    std::cout << "brep_drill: through, tilted, blind and two holes exact and solid; edge and touching drills fall back; T-vertices of merged neighbours split so every edge is shared; two blind holes meeting one through bore; inside stretches through, from inside and beside" << std::endl;

    return 0;
}
