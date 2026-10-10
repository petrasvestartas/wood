// The scenes of docs/joint_library.md, two session files per design for the viewer, never overlapping: <id>_unit.pb, the design's male and
// female outlines in its unit box, polylines since a plate joint is outlines merged into the plates'; and <id>.pb, the oracle's pair joined
// by it, the plates with the merged outlines and any solid the joint owns as BReps.
//   ./build/joint_tiles <out_dir> [family/library/parameters ...]     every oracle variant when no id is given
#define main joint_library_main
#include "../../tests/joint_library.cpp"
#undef main

namespace {
using namespace wood_session;
#include "wood_interaction_feature_plate_joints.h"
}

static const double BOX = 200.0; // mm, the unit box
static const double APART = 250.0; // mm, the second plate moved off the first
static const Color MALE = Color(0.86f, 0.43f, 0.16f);
static const Color FEMALE = Color(0.16f, 0.43f, 0.78f);
static const Color EDGE = Color(0.6f, 0.6f, 0.6f);

// ═══════════════════════════════════════════════════════════════════════════
// The unit box
// ═══════════════════════════════════════════════════════════════════════════

/// The design's outlines in its unit box, the library function run on a joint with the built one's parameters; false for a design that is
/// built on the plates in their own space (top to top, side removal, custom), which has no unit box to draw.
static bool unit_outlines(const Built& built, InteractionFeaturePlate& unit) {

    const InteractionFeaturePlate& built_connection = built.joint->connections.at(0);
    const JointPlateParameters& p = built.joint->parameters;
    unit.divisions = built_connection.divisions;
    unit.shift = built_connection.shift;
    const std::vector<std::shared_ptr<Plate>> plates = {built.fixture.a, built.fixture.b};
    std::vector<InteractionFeaturePlate> no_joints;

    const std::map<std::string, std::function<void()>> designs = {
        {"ss_e_ip_0", [&] { ss_e_ip_0(unit); }}, {"ss_e_ip_1", [&] { ss_e_ip_1(unit); }}, {"ss_e_ip_2", [&] { ss_e_ip_2(unit, plates); }},
        {"ss_e_ip_3", [&] { ss_e_ip_3(unit); }}, {"ss_e_ip_4", [&] { ss_e_ip_4(unit); }}, {"ss_e_ip_5", [&] { ss_e_ip_5(unit, plates); }},
        {"ss_e_op_0", [&] { ss_e_op_0(unit); }}, {"ss_e_op_1", [&] { ss_e_op_1(unit); }}, {"ss_e_op_2", [&] { ss_e_op_2(unit); }},
        {"ss_e_op_3", [&] { ss_e_op_3(unit); }},
        {"ss_e_op_4", [&] { ss_e_op_4(unit, p.taper, p.chamfer, p.modify_outline, p.x[0], p.x[1], p.y[0], p.y[1], p.z[0], p.z[1]); }},
        {"ss_e_op_5", [&] { ss_e_op_5(unit, no_joints, p.disable_divisions); }}, {"ss_e_op_6", [&] { ss_e_op_6(unit, no_joints); }},
        {"ss_e_op_17", [&] { ss_e_op_17(unit); }}, {"ss_e_op_tutorial", [&] { ss_e_op_tutorial(unit); }},
        {"ts_e_p_0", [&] { ts_e_p_0(unit); }}, {"ts_e_p_1", [&] { ts_e_p_1(unit); }}, {"ts_e_p_2", [&] { ts_e_p_2(unit); }},
        {"ts_e_p_3", [&] { ts_e_p_3(unit); }}, {"ts_e_p_4", [&] { ts_e_p_4(unit); }}, {"ts_e_p_5", [&] { ts_e_p_5(unit); }},
        {"cr_c_ip_0", [&] { cr_c_ip_0(unit); }}, {"cr_c_ip_1", [&] { cr_c_ip_1(unit); }}, {"cr_c_ip_2", [&] { cr_c_ip_2(unit); }},
        {"cr_c_ip_3", [&] { cr_c_ip_3(unit); }}, {"cr_c_ip_4", [&] { cr_c_ip_4(unit); }}, {"cr_c_ip_5", [&] { cr_c_ip_5(unit); }},
        {"ss_e_r_2", [&] { ss_e_r_2(unit, plates); }}, {"ss_e_r_3", [&] { ss_e_r_3(unit, plates); }},
    };

    const auto design = designs.find(built.library);
    if (design == designs.end())
        return false;
    design->second();
    return true;
}

/// The unit box and the outlines in it, scaled to BOX.
static void add_unit_box(WoodSession& scene, const InteractionFeaturePlate& unit) {

    const std::shared_ptr<TreeNode> group = scene.group_named("unit_box");
    const Xform place = Xform::scale_uniform(Point(0.0, 0.0, 0.0), BOX);

    // the box's twelve edges
    const double h = 0.5;
    for (int axis = 0; axis < 3; axis++)
        for (double u : {-h, h})
            for (double v : {-h, h}) {
                std::array<double, 3> start = {u, v, -h};
                std::array<double, 3> end = {u, v, h};
                std::rotate(start.begin(), start.begin() + 2 - axis, start.end());
                std::rotate(end.begin(), end.begin() + 2 - axis, end.end());
                auto edge = std::make_shared<Polyline>(std::vector<Point>{Point(start[0], start[1], start[2]), Point(end[0], end[1], end[2])});
                *edge = edge->transformed(place);
                edge->linecolor = EDGE;
                edge->width = 1.0;
                scene.add_polyline(edge, group);
            }

    // every male and female outline of both faces, the 2-point seam markers left out
    for (int side = 0; side < 2; side++)
        for (int face = 0; face < 2; face++)
            for (const Polyline& outline : side == 0 ? unit.male_outlines[face] : unit.female_outlines[face]) {
                if (outline.point_count() < 3)
                    continue;
                auto drawn = std::make_shared<Polyline>(outline.transformed(place));
                drawn->linecolor = side == 0 ? MALE : FEMALE;
                drawn->width = 3.0;
                drawn->name = fmt::format("{}_face_{}", side == 0 ? "male" : "female", face);
                scene.add_polyline(drawn, group);
            }
}

// ═══════════════════════════════════════════════════════════════════════════
// Scenes
// ═══════════════════════════════════════════════════════════════════════════

/// One design's scenes: its unit box alone when it has one, and its pair as the oracle joins it.
static void write_scene(const std::string& id, const std::string& dir) {

    Built built = build_variant(id, Xform::identity());
    std::string name = id;
    std::replace(name.begin(), name.end(), '/', '_');

    // the unit box, a scene of its own
    InteractionFeaturePlate unit;
    if (unit_outlines(built, unit)) {
        WoodSession unit_scene(name + "_unit");
        add_unit_box(unit_scene, unit);
        unit_scene.pb_dump(dir + "/" + name + "_unit.pb");
    }

    // the pair drawn apart, the second plate moved along the contact from the first so the merged outlines read, a key half way and shown
    const Fixture& fixture = built.fixture;
    Vector apart = fixture.cross
        ? fixture.a->planes[0].z_axis().cross(fixture.b->planes[0].z_axis()).normalized()
        : compute_newell(fixture.face->polygon.get_points()).normalized();
    if ((fixture.b->element_geometry_mesh().centroid() - fixture.a->element_geometry_mesh().centroid()).dot(apart) < 0.0)
        apart = apart * -1.0;
    fixture.b->place(Xform::translation(apart[0] * APART, apart[1] * APART, apart[2] * APART));
    built.joint->place(Xform::translation(apart[0] * 0.5 * APART, apart[1] * 0.5 * APART, apart[2] * 0.5 * APART));
    built.joint->is_visible = built.joint->key_mesh().number_of_faces() > 0;
    built.scene->pb_dump(dir + "/" + name + ".pb");
}

int main(int argc, char** argv) {

    if (argc < 2)
        throw std::invalid_argument("joint_tiles <out_dir> [ids...]");

    std::vector<std::string> ids(argv + 2, argv + argc);
    if (ids.empty())
        ids = VARIANTS;

    for (const std::string& id : ids) {
        try {
            write_scene(id, argv[1]);
        } catch (const std::exception& e) {
            std::cout << id << ": " << e.what() << std::endl;
        }
    }

    return 0;
}
