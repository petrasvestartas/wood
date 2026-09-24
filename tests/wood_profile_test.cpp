#include "wood_session.h"
#include "wood_profile.h"
#include "wood_element_geometry.h"
#include "wood_instance.h"
#include "src/templates/grid_plan.h"
using namespace session_cpp;
using namespace wood_session;

static int failures = 0;

static void check(bool condition, const std::string& name) {

    if (condition)
        return;

    std::cerr << "FAIL " << name << "\n";
    ++failures;
}

/// True when two volumes agree to one part in a million of the expected one.
static bool is_near(double volume, double expected) {
    return std::abs(std::abs(volume) - expected) <= 1e-6 * expected;
}

/// A volume check that reports both numbers when it fails.
static void check_volume(double volume, double expected, const std::string& name) {
    check(is_near(volume, expected), fmt::format("{}: {:.0f} vs {:.0f}", name, std::abs(volume), expected));
}

/// The BRep volume check, skipped for the W whose thin flanges the kernel tessellation still opens (BRep::mesh(), reported to the kernel); any other open tessellation fails.
static void check_brep(const BRep& brep, double expected, const std::string& name) {

    if (brep.mesh().is_closed())
        check_volume(brep.volume(), expected, name);
    else
        check(name.rfind("w ", 0) == 0, name + " tessellation open");
}

/// Area of a profile: every loop's signed area summed, so a clockwise hole subtracts and a second outline adds.
static double compute_profile_area(const std::vector<Polyline>& profile) {

    double area = 0.0;
    for (const Polyline& ring : profile)
        area += wood_grid::plan::compute_area(wood_grid::plan::to_loop(ring));

    return area;
}

/// True when the mesh holds a vertex at point.
static bool has_vertex(const Mesh& mesh, const Point& point) {

    for (const size_t vertex : mesh.vertices())
        if (mesh.vertex_point(vertex)->distance(point) < 1e-6)
            return true;

    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// Cases
// ═══════════════════════════════════════════════════════════════════════════

const double LENGTH = 3000.0;
const std::vector<std::pair<std::string, std::vector<Polyline>>> PROFILES = {
    {"rectangle", profile_rectangle(265.0, 608.0)},
    {"round", profile_round(300.0)},
    {"w", profile_w(250.0, 250.0, 15.0, 10.0)},
    {"hss", profile_hss(250.0, 250.0, 10.0)},
    {"double_left", {profile_double(120.0, 600.0, 60.0)[0]}},
    {"double_right", {profile_double(120.0, 600.0, 60.0)[1]}},
    {"slab_band", profile_slab_band(1200.0, 300.0)},
    {"t", profile_t(300.0, 500.0, 100.0, 80.0)}
};

/// A beam of the profile along x: closed, mesh and BRep volumes area times length, the same after two square end cuts and still closed after an oblique one.
static void beam_test(const std::string& name, const std::vector<Polyline>& profile) {

    const double area = compute_profile_area(profile);
    Beam beam(Polyline({Point(0.0, 0.0, 0.0), Point(LENGTH, 0.0, 0.0)}), profile);
    check(beam.element_geometry_mesh().is_closed(), name + " beam closed");
    check_volume(compute_volume(beam.element_geometry_mesh()), area * LENGTH, name + " beam mesh volume");
    check_brep(beam.element_geometry_brep(), area * LENGTH, name + " beam brep volume");

    beam.cuts = {Plane::from_point_normal(Point(500.0, 0.0, 0.0), Vector::x_axis()), Plane::from_point_normal(Point(2500.0, 0.0, 0.0), -Vector::x_axis())};
    beam.invalidate_geometry();
    check(beam.model_geometry_mesh().is_closed(), name + " cut beam closed");
    check_volume(compute_volume(beam.model_geometry_mesh()), area * 2000.0, name + " cut beam mesh volume");
    check_brep(beam.model_geometry_brep(), area * 2000.0, name + " cut beam brep volume");

    beam.cuts.push_back(Plane::from_point_normal(Point(1500.0, 0.0, 0.0), Vector(-1.0, 0.0, 0.5)));
    beam.invalidate_geometry();
    check(beam.model_geometry_mesh().is_closed(), name + " oblique cut beam closed");
    check(std::abs(compute_volume(beam.model_geometry_mesh())) < area * 2000.0, name + " oblique cut beam smaller");
    check_brep(beam.model_geometry_brep(), std::abs(compute_volume(beam.model_geometry_mesh())), name + " oblique cut mesh and brep agree");
}

/// A column of the profile turned 30 degrees: closed, both volumes area times length, one square end cut, and after a turn about z the solid still passes through the moved section.
static void column_test(const std::string& name, const std::vector<Polyline>& profile) {

    const double area = compute_profile_area(profile);
    Column column(Line::from_points(Point(0.0, 0.0, 0.0), Point(0.0, 0.0, LENGTH)), profile, 30.0);
    check(column.element_geometry_mesh().is_closed(), name + " column closed");
    check_volume(compute_volume(column.element_geometry_mesh()), area * LENGTH, name + " column mesh volume");
    check_brep(column.element_geometry_brep(), area * LENGTH, name + " column brep volume");

    column.cuts = {Plane::from_point_normal(Point(0.0, 0.0, 1000.0), Vector::z_axis())};
    column.invalidate_geometry();
    check(column.model_geometry_mesh().is_closed(), name + " cut column closed");
    check_volume(compute_volume(column.model_geometry_mesh()), area * 2000.0, name + " cut column mesh volume");
    check_brep(column.model_geometry_brep(), area * 2000.0, name + " cut column brep volume");

    column.place(Xform::rotation_z(45.0, true));
    check(has_vertex(column.element_geometry_mesh(), column.section.get_point(0)), name + " placed column follows its section");
    check_volume(compute_volume(column.model_geometry_mesh()), area * 2000.0, name + " placed column volume");

    const std::shared_ptr<Column> moved = column.transformed(Xform::translation(1000.0, 0.0, 0.0) * Xform::rotation_z(-45.0, true));
    check(has_vertex(moved->element_geometry_mesh(), moved->section.get_point(0)), name + " transformed column follows its section");
}

/// The profile survives the protobuf payload of both elements, geometry equal on reload.
static void round_trip_test(const std::string& name, const std::vector<Polyline>& profile) {

    const Beam beam(Polyline({Point(0.0, 0.0, 0.0), Point(LENGTH, 0.0, 0.0)}), profile);
    const std::shared_ptr<Beam> beam_copy = Beam::from_element(beam);
    check(beam_copy->profile.size() == profile.size(), name + " beam profile reloaded");
    check_volume(compute_volume(beam_copy->element_geometry_mesh()), std::abs(compute_volume(beam.element_geometry_mesh())), name + " beam volume reloaded");

    const Column column(Line::from_points(Point(0.0, 0.0, 0.0), Point(0.0, 0.0, LENGTH)), profile, 30.0);
    const std::shared_ptr<Column> column_copy = Column::from_element(column);
    check(column_copy->profile.size() == profile.size() && column_copy->rotation == 30.0, name + " column profile reloaded");
    check_volume(compute_volume(column_copy->element_geometry_mesh()), std::abs(compute_volume(column.element_geometry_mesh())), name + " column volume reloaded");
}

/// Two beams of one profile at different places share an instancing key; another profile keys apart.
static void key_test() {

    const Beam a(Polyline({Point(0.0, 0.0, 0.0), Point(LENGTH, 0.0, 0.0)}), PROFILES[0].second);
    const Beam b(Polyline({Point(0.0, 5000.0, 0.0), Point(LENGTH, 5000.0, 0.0)}), PROFILES[0].second);
    const Beam c(Polyline({Point(0.0, 0.0, 0.0), Point(LENGTH, 0.0, 0.0)}), PROFILES[2].second);
    check(element_key(a)->first == element_key(b)->first, "same profile same key");
    check(element_key(a)->first != element_key(c)->first, "other profile other key");

    const Column d(Line::from_points(Point(0.0, 0.0, 0.0), Point(0.0, 0.0, LENGTH)), PROFILES[3].second, 30.0);
    const Column e(Line::from_points(Point(5000.0, 0.0, 0.0), Point(5000.0, 0.0, LENGTH)), PROFILES[3].second, 30.0);
    check(element_key(d)->first == element_key(e)->first, "same column profile same key");
}

int main() {

    for (const std::pair<std::string, std::vector<Polyline>>& entry : PROFILES) {
        beam_test(entry.first, entry.second);
        column_test(entry.first, entry.second);
        round_trip_test(entry.first, entry.second);
    }
    key_test();

    return failures == 0 ? 0 : 1;
}

/*
|||||||| DESCRIPTION ||||||||
profile cases: every profile of the library swept as a Beam and as a Column is a closed solid whose mesh and BRep volumes equal the profile area times the length with holes subtracted, stays closed and exact under square end cuts and closed under an oblique one, keeps its volume through place and transformed, survives the protobuf payload, and keys the same for instancing wherever it stands.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE BUILD && RUN ||||||||
cmake --build build --target wood_profile_test --parallel 4 && tools/run_guarded.sh -t 3 -m 3 -- build/wood_profile_test
*/
