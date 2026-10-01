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

/// The drilled block: a closed solid with faces planar faces plus cylinders cylindrical ones, at the volume of the block less the holes.
void check_drilled(const std::vector<Drill>& drills, size_t faces, size_t cylinders, double removed, const std::string& name) {

    const std::optional<BRep> brep = drilled_brep(block(), drills);
    check(brep.has_value(), name + ": no exact solid");
    check(brep->is_solid(), name + ": not solid");
    check(static_cast<size_t>(brep->face_count()) == faces, name + ": faces " + std::to_string(brep->face_count()));

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

    check_drilled({{Line::from_points(Point(50, 50, -10), Point(50, 50, 60)), r}}, 7, 1, area * 50.0, "drill_through");

    const double tilt = 20.0 * M_PI / 180.0;
    const Vector d(std::sin(tilt), 0.0, std::cos(tilt));
    check_drilled({{Line::from_points(Point(50, 50, 25) - d * 40.0, Point(50, 50, 25) + d * 40.0), r}}, 7, 1, area * 50.0 / std::cos(tilt), "drill_tilted");

    check_drilled({{Line::from_points(Point(30, 30, 60), Point(30, 30, 30)), r}}, 8, 1, area * 20.0, "drill_blind");

    check_drilled({{Line::from_points(Point(30, 30, -10), Point(30, 30, 60)), r}, {Line::from_points(Point(70, 70, 60), Point(70, 70, 20)), 4.0}}, 9, 2, area * 50.0 + M_PI * 16.0 * 30.0, "drill_two");

    check(!drilled_brep(block(), {{Line::from_points(Point(3, 50, -10), Point(3, 50, 60)), r}}).has_value(), "a drill across an edge must fall back");
    check(!drilled_brep(block(), {{Line::from_points(Point(50, 50, -10), Point(50, 50, 60)), r}, {Line::from_points(Point(56, 50, -10), Point(56, 50, 60)), r}}).has_value(), "touching drills must fall back");

    std::cout << "brep_drill: through, tilted, blind and two holes exact and solid; edge and touching drills fall back" << std::endl;

    return 0;
}
