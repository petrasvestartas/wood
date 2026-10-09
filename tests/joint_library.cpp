#include "oracle_polyline.h"
#include <chrono>
#include <iomanip>
#include <numbers>

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// The joint library oracle: every JointPlate design on a pair that stays joined, measured, never looked at
// ═══════════════════════════════════════════════════════════════════════════

static const double CLOSE = 1e-6; // mm, the closure and the duplicate points of a merged outline
static const double PLANAR = 1e-6; // mm, a loop's distance from its plane
static const double VOLUME_REL = 1e-9; // relative, planar solids against each other and a round trip
static const double ROUND_REL = 1e-3; // relative, a BRep with cylinders against its polygonal mesh
static const double ZERO_REL = 1e-6; // relative to a member, an overlap, material outside the stock, a fit
static const double HOLE_REL = 1e-2; // relative, the polygonal drill mesh against pi r^2 L
static const double GOLDEN_TOL = 1e-6; // mm, a golden coordinate
static const std::string GOLDEN_DIR = std::string(WOOD_SOURCE_DIR) + "/tests/golden/joint_library";

/// Every design of the library with its default and a non-default parameter set: "family/library/parameters...".
static const std::vector<std::string> VARIANTS = {
    "ip/ss_e_ip_0", "ip/ss_e_ip_1/8/0.5", "ip/ss_e_ip_1/4/0.0", "ip/ss_e_ip_1/16/1.0", "ip/ss_e_ip_2/4", "ip/ss_e_ip_2/2",
    "ip/ss_e_ip_3", "ip/ss_e_ip_4", "ip/ss_e_ip_5/4", "ip/ss_e_ip_5/6", "ip/ss_e_ip_custom", "ip/side_removal/0/0.5",
    "op/ss_e_op_0", "op/ss_e_op_1/8/0.5", "op/ss_e_op_1/6/0.0", "op/ss_e_op_2/8/0.5", "op/ss_e_op_2/12/1.0", "op/ss_e_op_3",
    "op/ss_e_op_4/8/0/0/1", "op/ss_e_op_4/8/0.1/1/1", "op/ss_e_op_4/8/0/0/0", "op/ss_e_op_5/8/0", "op/ss_e_op_5/8/1",
    "op/ss_e_op_17/4", "op/ss_e_op_tutorial", "op/ss_e_op_custom", "op/side_removal/1/0.5",
    "ts/ts_e_p_0", "ts/ts_e_p_1", "ts/ts_e_p_2/8/0.5", "ts/ts_e_p_2/16/0.25", "ts/ts_e_p_3/8/0.5", "ts/ts_e_p_3/16/0.0",
    "ts/ts_e_p_3/24/1.0", "ts/ts_e_p_5/4", "ts/ts_e_p_5/8", "ts/ts_e_p_custom", "ts/side_removal/0/0.5",
    "r/ss_e_r_0", "r/ss_e_r_1", "r/ss_e_r_2/4/0.5", "r/ss_e_r_2/2/0.25", "r/ss_e_r_3/4/0.5", "r/ss_e_r_3/6/1.0", "r/ss_e_r_custom",
    "r/side_removal_ss_e_r_1_port/0.5",
    "cr/cr_c_ip_0", "cr/cr_c_ip_1/0.5", "cr/cr_c_ip_1/0.25", "cr/cr_c_ip_2", "cr/cr_c_ip_3", "cr/cr_c_ip_4", "cr/cr_c_ip_5", "cr/cr_c_ip_custom",
    "tt/tt_e_p_0/8", "tt/tt_e_p_1/8", "tt/tt_e_p_2/6/60/8", "tt/tt_e_p_3/60/8", "tt/tt_e_p_4/60/8", "tt/tt_e_p_5/60/8", "tt/tt_e_p_3/30/4",
};

/// Designs no fixture can orient: plate_contact_family has no family for the boundary type 60.
static const std::vector<std::string> SKIPPED = {
    "b/b_0: no contact family for joint type 60, orient throws (dataset only)",
    "b/b_custom: no contact family for joint type 60, orient throws (dataset only)",
};

/// The pair a variant is joined on: the plates in contact order, their contact, and the target of each interaction side.
struct Fixture {
    std::shared_ptr<Plate> a;
    std::shared_ptr<Plate> b;
    std::shared_ptr<InteractionContactFace> face;
    std::shared_ptr<InteractionContactCross> cross;
    std::shared_ptr<Plate> target0; // interaction(0)
    std::shared_ptr<Plate> target1; // interaction(1)
};

/// A variant built on its fixture in a scene of its own.
struct Built {
    std::shared_ptr<WoodSession> scene;
    Fixture fixture;
    std::shared_ptr<JointPlate> joint;
    std::string family;
    std::string library;
};

/// Everything measured on one variant: the row of the table and its failures.
struct Row {
    std::string id;
    double volume_a = 0.0;
    double volume_b = 0.0;
    double lost = 0.0;
    double overlap = 0.0;
    size_t pieces = 0;
    size_t drills = 0;
    double ms = 0.0;
    std::vector<std::string> failures;
};

static void fail(Row& row, const std::string& check, const std::string& message) {

    row.failures.push_back(check + ": " + message);
}

static std::vector<std::string> split(const std::string& text, char separator) {

    std::vector<std::string> parts;
    std::string part;
    std::istringstream stream(text);
    while (std::getline(stream, part, separator))
        parts.push_back(part);

    return parts;
}

/// The id as a file name: "ip/ss_e_ip_1/8/0.5" -> "ip_ss_e_ip_1_8_0.5".
static std::string file_name(const std::string& id) {

    std::string name = id;
    std::replace(name.begin(), name.end(), '/', '_');

    return name;
}

// ═══════════════════════════════════════════════════════════════════════════
// Fixtures - the pairs of the element examples, kept joined, moved by xform before the contact is found
// ═══════════════════════════════════════════════════════════════════════════

/// Two 300 x 400 x 40 plates edge to edge in one plane: ss_e_ip, ss_e_r (with the scene set to treat every side-side joint as rotated) and side removal.
static Fixture pair_in_plane(WoodSession& scene, const Xform& xform) {

    Fixture f;
    f.a = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 300.0, 400.0, 40.0, "left");
    f.b = Plate::from_rectangle({300.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 300.0, 400.0, 40.0, "right");
    f.a->place(xform);
    f.b->place(xform);
    scene.add(f.a);
    scene.add(f.b);
    f.face = scene.compute_face_contact(f.a, f.b);
    f.target0 = f.a;
    f.target1 = f.b;

    return f;
}

/// A floor and a wall meeting at a right angle, their side faces mitred on one plane: ss_e_op.
static Fixture pair_out_of_plane(WoodSession& scene, const Xform& xform) {

    const Polyline floor_bottom({{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {0.0, 400.0, 0.0}, {0.0, 0.0, 0.0}});
    const Polyline floor_top({{0.0, 0.0, 40.0}, {260.0, 0.0, 40.0}, {260.0, 400.0, 40.0}, {0.0, 400.0, 40.0}, {0.0, 0.0, 40.0}});
    const Polyline wall_bottom({{300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {300.0, 400.0, 300.0}, {300.0, 0.0, 300.0}, {300.0, 0.0, 0.0}});
    const Polyline wall_top({{260.0, 0.0, 40.0}, {260.0, 400.0, 40.0}, {260.0, 400.0, 300.0}, {260.0, 0.0, 300.0}, {260.0, 0.0, 40.0}});

    Fixture f;
    f.a = std::make_shared<Plate>(floor_bottom, floor_top, "floor");
    f.b = std::make_shared<Plate>(wall_bottom, wall_top, "wall");
    f.a->place(xform);
    f.b->place(xform);
    scene.add(f.a);
    scene.add(f.b);
    f.face = scene.compute_face_contact(f.a, f.b);
    f.target0 = f.a;
    f.target1 = f.b;

    return f;
}

/// An upright 250 x 250 plate standing in the middle of a 400 x 400 base: ts_e_p; the joint's first side is the upright.
static Fixture pair_top_side(WoodSession& scene, const Xform& xform) {

    Fixture f;
    f.a = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 400.0, 40.0, "base");
    f.b = Plate::from_rectangle({180.0, 75.0, 40.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 250.0, 250.0, 40.0, "upright");
    f.a->place(xform);
    f.b->place(xform);
    scene.add(f.a);
    scene.add(f.b);
    f.face = scene.compute_face_contact(f.a, f.b);
    f.target0 = f.b;
    f.target1 = f.a;

    return f;
}

/// Two 400 x 300 x 40 plates stacked, the upper one 150 along: tt_e_p.
static Fixture pair_top_top(WoodSession& scene, const Xform& xform) {

    Fixture f;
    f.a = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 300.0, 40.0, "lower");
    f.b = Plate::from_rectangle({150.0, 0.0, 40.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 300.0, 40.0, "upper");
    f.a->place(xform);
    f.b->place(xform);
    scene.add(f.a);
    scene.add(f.b);
    f.face = scene.compute_face_contact(f.a, f.b);
    f.target0 = f.a;
    f.target1 = f.b;

    return f;
}

/// Two upright 400 x 200 x 40 plates crossing at their middles: cr_c_ip.
static Fixture pair_cross(WoodSession& scene, const Xform& xform) {

    Fixture f;
    f.a = Plate::from_rectangle({0.0, 20.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "first");
    f.b = Plate::from_rectangle({180.0, -200.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "second");
    f.a->place(xform);
    f.b->place(xform);
    scene.add(f.a);
    scene.add(f.b);
    f.cross = scene.compute_cross_contact(f.a, f.b);
    f.target0 = f.a;
    f.target1 = f.b;

    return f;
}

/// The fixture of a family prefix.
static Fixture make_fixture(const std::string& family, WoodSession& scene, const Xform& xform) {

    if (family == "ip")
        return pair_in_plane(scene, xform);
    if (family == "r") {
        scene.settings.all_treated_as_rotated = true;
        scene.settings.rotated_joint_as_average = true;
        return pair_in_plane(scene, xform);
    }
    if (family == "op")
        return pair_out_of_plane(scene, xform);
    if (family == "ts")
        return pair_top_side(scene, xform);
    if (family == "tt")
        return pair_top_top(scene, xform);
    if (family == "cr")
        return pair_cross(scene, xform);

    throw std::invalid_argument("no fixture for family " + family);
}

// ═══════════════════════════════════════════════════════════════════════════
// Variants - a library factory per id, the custom designs on unit outlines
// ═══════════════════════════════════════════════════════════════════════════

/// A three-finger zigzag across the unit box on one face: `fixed_axis` held at `fixed`, the fingers swinging between -0.5 and 0.5 on `swing_axis`, stepping along z upwards or downwards.
static Polyline zigzag(int fixed_axis, double fixed, int swing_axis, bool z_up) {

    const double a = 0.357142857142857;
    const double b = 0.214285714285714;
    const double c = 0.0714285714285715;
    const std::array<double, 12> z = {-a, -a, -b, -b, -c, -c, c, c, b, b, a, a};
    const std::array<double, 12> swing = {0.5, -0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5};
    std::vector<Point> points;

    for (size_t i = 0; i < 12; i++) {
        Point p(0.0, 0.0, z_up ? z[i] : -z[i]);
        p[fixed_axis] = fixed;
        p[swing_axis] = swing[i];
        points.push_back(p);
    }

    return Polyline(points);
}

/// The male and female unit outlines of a custom design, face 0 then face 1 of each.
static std::array<std::vector<Polyline>, 2> custom_outlines(const std::string& family) {

    if (family == "op")
        return {std::vector<Polyline>{zigzag(1, 0.5, 0, false), zigzag(1, -0.5, 0, false)}, std::vector<Polyline>{zigzag(0, 0.5, 1, true), zigzag(0, -0.5, 1, true)}};
    if (family == "ts")
        return {std::vector<Polyline>{zigzag(0, 0.5, 1, true), zigzag(0, -0.5, 1, true)}, std::vector<Polyline>{zigzag(0, 0.5, 1, true), zigzag(0, -0.5, 1, true)}};
    if (family == "cr") {
        const Polyline male0({{0.5, 0.5, -1.0}, {-0.5, 0.5, -1.0}, {-0.5, 0.5, 0.0}, {0.5, 0.5, 0.0}, {0.5, 0.5, -1.0}});
        const Polyline male1({{0.5, -0.5, -1.0}, {-0.5, -0.5, -1.0}, {-0.5, -0.5, 0.0}, {0.5, -0.5, 0.0}, {0.5, -0.5, -1.0}});
        const Polyline female0({{-0.5, 0.5, 1.0}, {-0.5, -0.5, 1.0}, {-0.5, -0.5, 0.0}, {-0.5, 0.5, 0.0}, {-0.5, 0.5, 1.0}});
        const Polyline female1({{0.5, 0.5, 1.0}, {0.5, -0.5, 1.0}, {0.5, -0.5, 0.0}, {0.5, 0.5, 0.0}, {0.5, 0.5, 1.0}});
        return {std::vector<Polyline>{male0, male1}, std::vector<Polyline>{female0, female1}};
    }

    // in plane and rotated: the same zigzag on both members, as ss_e_ip_0
    return {std::vector<Polyline>{zigzag(1, -0.5, 0, true), zigzag(1, 0.5, 0, true)}, std::vector<Polyline>{zigzag(1, -0.5, 0, true), zigzag(1, 0.5, 0, true)}};
}

static int integer(const std::vector<std::string>& parts, size_t index, int fallback) {

    return index < parts.size() ? std::stoi(parts[index]) : fallback;
}

static double number(const std::vector<std::string>& parts, size_t index, double fallback) {

    return index < parts.size() ? std::stod(parts[index]) : fallback;
}

/// The library joint of an id split at '/': the family, the library name, then its parameters in declaration order.
static std::shared_ptr<JointPlate> make_variant(const std::vector<std::string>& parts) {

    const std::string& family = parts[0];
    const std::string& library = parts[1];
    const std::array<std::vector<Polyline>, 2> custom = custom_outlines(family);

    if (library == "ss_e_ip_0") return JointPlate::ss_e_ip_0();
    if (library == "ss_e_ip_1") return JointPlate::ss_e_ip_1(integer(parts, 2, 8), number(parts, 3, 0.5));
    if (library == "ss_e_ip_2") return JointPlate::ss_e_ip_2(integer(parts, 2, 4));
    if (library == "ss_e_ip_3") return JointPlate::ss_e_ip_3();
    if (library == "ss_e_ip_4") return JointPlate::ss_e_ip_4();
    if (library == "ss_e_ip_5") return JointPlate::ss_e_ip_5(integer(parts, 2, 4));
    if (library == "ss_e_ip_custom") return JointPlate::ss_e_ip_custom(custom[0], custom[1], integer(parts, 2, 4));

    if (library == "ss_e_op_0") return JointPlate::ss_e_op_0();
    if (library == "ss_e_op_1") return JointPlate::ss_e_op_1(integer(parts, 2, 8), number(parts, 3, 0.5));
    if (library == "ss_e_op_2") return JointPlate::ss_e_op_2(integer(parts, 2, 8), number(parts, 3, 0.5));
    if (library == "ss_e_op_3") return JointPlate::ss_e_op_3();
    if (library == "ss_e_op_4") return JointPlate::ss_e_op_4(integer(parts, 2, 8), number(parts, 3, 0.0), integer(parts, 4, 0) != 0, integer(parts, 5, 1) != 0);
    if (library == "ss_e_op_5") return JointPlate::ss_e_op_5(integer(parts, 2, 8), integer(parts, 3, 0) != 0);
    if (library == "ss_e_op_17") return JointPlate::ss_e_op_17(integer(parts, 2, 4));
    if (library == "ss_e_op_tutorial") return JointPlate::ss_e_op_tutorial();
    if (library == "ss_e_op_custom") return JointPlate::ss_e_op_custom(custom[0], custom[1]);

    if (library == "ts_e_p_0") return JointPlate::ts_e_p_0();
    if (library == "ts_e_p_1") return JointPlate::ts_e_p_1();
    if (library == "ts_e_p_2") return JointPlate::ts_e_p_2(integer(parts, 2, 8), number(parts, 3, 0.5));
    if (library == "ts_e_p_3") return JointPlate::ts_e_p_3(integer(parts, 2, 8), number(parts, 3, 0.5));
    if (library == "ts_e_p_5") return JointPlate::ts_e_p_5(integer(parts, 2, 4));
    if (library == "ts_e_p_custom") return JointPlate::ts_e_p_custom(custom[0], custom[1]);

    if (library == "ss_e_r_0") return JointPlate::ss_e_r_0();
    if (library == "ss_e_r_1") return JointPlate::ss_e_r_1();
    if (library == "ss_e_r_2") return JointPlate::ss_e_r_2(integer(parts, 2, 4), number(parts, 3, 0.5));
    if (library == "ss_e_r_3") return JointPlate::ss_e_r_3(integer(parts, 2, 4), number(parts, 3, 0.5));
    if (library == "ss_e_r_custom") return JointPlate::ss_e_r_custom(custom[0], custom[1]);

    if (library == "cr_c_ip_0") return JointPlate::cr_c_ip_0();
    if (library == "cr_c_ip_1") return JointPlate::cr_c_ip_1(number(parts, 2, 0.5));
    if (library == "cr_c_ip_2") return JointPlate::cr_c_ip_2();
    if (library == "cr_c_ip_3") return JointPlate::cr_c_ip_3();
    if (library == "cr_c_ip_4") return JointPlate::cr_c_ip_4();
    if (library == "cr_c_ip_5") return JointPlate::cr_c_ip_5();
    if (library == "cr_c_ip_custom") return JointPlate::cr_c_ip_custom(custom[0], custom[1]);

    if (library == "tt_e_p_0") return JointPlate::tt_e_p_0(number(parts, 2, 1.0));
    if (library == "tt_e_p_1") return JointPlate::tt_e_p_1(number(parts, 2, 1.0));
    if (library == "tt_e_p_2") return JointPlate::tt_e_p_2(integer(parts, 2, 6), number(parts, 3, 20.0), number(parts, 4, 1.0));
    if (library == "tt_e_p_3") return JointPlate::tt_e_p_3(number(parts, 2, 30.0), number(parts, 3, 1.0));
    if (library == "tt_e_p_4") return JointPlate::tt_e_p_4(number(parts, 2, 30.0), number(parts, 3, 1.0));
    if (library == "tt_e_p_5") return JointPlate::tt_e_p_5(number(parts, 2, 30.0), number(parts, 3, 1.0));

    if (library == "side_removal") return JointPlate::side_removal(integer(parts, 2, 0) != 0, number(parts, 3, 0.5));
    if (library == "side_removal_ss_e_r_1_port") return JointPlate::side_removal_ss_e_r_1_port(number(parts, 2, 0.5));

    throw std::invalid_argument("no factory for " + library);
}

/// The variant joined on its fixture, the pair moved by xform first: the joint oriented on the contact, added, and passed to each target.
static Built build_variant(const std::string& id, const Xform& xform) {

    const std::vector<std::string> parts = split(id, '/');
    Built built;
    built.family = parts[0];
    built.library = parts[1];
    built.scene = std::make_shared<WoodSession>(id);
    built.fixture = make_fixture(built.family, *built.scene, xform);
    built.joint = make_variant(parts);

    const Fixture& f = built.fixture;
    if (f.cross)
        built.joint->orient(f.cross, {f.a, f.b}, built.scene->settings);
    else {
        if (!f.face)
            throw std::runtime_error("the fixture plates do not touch");
        built.joint->orient(f.face, {f.a, f.b}, built.scene->settings);
    }
    built.scene->add(built.joint);
    built.scene->add_interaction(built.joint, f.target0, built.joint->interaction(0));
    built.scene->add_interaction(built.joint, f.target1, built.joint->interaction(1));

    return built;
}

// ═══════════════════════════════════════════════════════════════════════════
// Measurements shared by the checks
// ═══════════════════════════════════════════════════════════════════════════

/// The solid pieces a joint makes: every male and female outline pair with three or more points lofted, as JointPlate::bodies does.
static std::vector<Mesh> joint_pieces(const JointPlate& joint) {

    std::vector<Mesh> pieces;
    for (const InteractionFeaturePlate& connection : joint.connections) {
        for (int side = 0; side < 2; side++) {
            const std::array<std::vector<Polyline>, 2>& outlines = side == 0 ? connection.male_outlines : connection.female_outlines;
            for (size_t i = 0; i < std::min(outlines[0].size(), outlines[1].size()); i++) {
                const Polyline& a = outlines[0][i];
                const Polyline& b = outlines[1][i];
                if (a.point_count() >= 3 && b.point_count() == a.point_count())
                    pieces.push_back(Mesh::loft({a.closed()}, {b.closed()}, true));
            }
        }
    }

    return pieces;
}

/// The faces of a BRep on a rational surface: its cylinders.
static size_t cylinders(const BRep& brep) {

    size_t count = 0;
    for (const BRepFace& face : brep.m_faces)
        count += brep.m_surfaces[face.surface_index].is_rational();

    return count;
}

/// The volume of a boolean of two members, zero for an empty result.
static double boolean_volume(const Mesh& a, const Mesh& b, SolidOperation operation) {

    const Mesh result = solid_boolean(a, b, operation);
    return result.number_of_faces() == 0 ? 0.0 : compute_volume(result);
}

/// True when the point lies between the plate's outer planes and inside the box of its outlines: where a drill axis counts as the plate's.
static bool inside_plate(const Plate& plate, const Point& point) {

    const bool between = plate.planes[0].signed_distance(point) <= CLOSE && plate.planes[1].signed_distance(point) <= CLOSE;
    return between && plate.aabb(CLOSE).contains(point);
}

/// The drill features of the element whose guid starts with the joint's.
static size_t drills_of(const Element& element, const std::string& joint) {

    size_t count = 0;
    for (const ElementFeature& feature : element.features())
        if (feature.feature_type == "drill" && feature.guid().starts_with(joint + "/"))
            count++;

    return count;
}

// ═══════════════════════════════════════════════════════════════════════════
// Checks - each appends what it finds wrong to the row, none throws on a finding
// ═══════════════════════════════════════════════════════════════════════════

/// The largest distance of a loop's points from a plane.
static double off_face(const Plane& plane, const Polyline& loop) {

    double worst = 0.0;
    for (const Point& point : loop.get_points())
        worst = std::max(worst, std::abs(plane.signed_distance(point)));

    return worst;
}

/// The outer face of the plate the loop lies on within PLANAR: 0 the plane of polylines[0], 1 of polylines[1], -1 neither.
static int face_of(const Plate& plate, const Polyline& loop) {

    for (int face = 0; face < 2; face++)
        if (off_face(plate.planes[face], loop) <= PLANAR)
            return face;

    return -1;
}

/// C1: every merged loop of both plates is closed, planar on the plate's face, free of repeated points and self crossings, with area, holes inside the outer ring, top and bottom paired.
static void check_outlines(const Built& built, Row& row) {

    for (const std::shared_ptr<Plate>& plate : {built.fixture.a, built.fixture.b}) {
        const Features& features = plate->features;
        const std::string who = plate->name;

        if (features.top.empty()) {
            if (built.family != "tt")
                fail(row, "C1", who + " has no merged outlines after the joint");
            continue;
        }
        if (features.top.size() != features.bottom.size()) {
            fail(row, "C1", fmt::format("{}: {} top loops against {} bottom loops", who, features.top.size(), features.bottom.size()));
            continue;
        }

        for (int face = 0; face < 2; face++) {
            const std::vector<Polyline>& loops = face == 0 ? features.bottom : features.top;
            for (size_t i = 0; i < loops.size(); i++) {
                const Polyline& loop = loops[i];
                const std::string where = fmt::format("{} {} loop {}", who, face == 0 ? "bottom" : "top", i);
                if (loop.point_count() < 4) {
                    fail(row, "C1", where + fmt::format(" has {} points", loop.point_count()));
                    continue;
                }
                if (oracle::closing_gap(loop) > CLOSE)
                    fail(row, "C1", where + fmt::format(" is open by {:.3g} mm", oracle::closing_gap(loop)));
                if (oracle::planarity(loop) > PLANAR)
                    fail(row, "C1", where + fmt::format(" leaves its plane by {:.3g} mm", oracle::planarity(loop)));
                // the face the loop lies on: either outer plane of the plate, the top and bottom loops of a pair on different ones
                const int on = face_of(*plate, loop);
                const Plane& plane = plate->planes[on < 0 ? face : on];
                if (on < 0)
                    fail(row, "C1", where + fmt::format(" leaves both plate faces, by {:.3g} and {:.3g} mm", off_face(plate->planes[0], loop), off_face(plate->planes[1], loop)));
                else if (face == 1 && on == face_of(*plate, features.bottom[i]))
                    fail(row, "C1", where + " and its bottom loop lie on the same face");
                if (oracle::consecutive_duplicates(loop, CLOSE) > 1)
                    fail(row, "C1", where + fmt::format(" repeats {} consecutive points", oracle::consecutive_duplicates(loop, CLOSE) - 1));
                if (loop.area() <= CLOSE)
                    fail(row, "C1", where + " has no area");
                if (!oracle::is_simple(loop, CLOSE))
                    fail(row, "C1", where + " crosses itself");
                if (i > 0) {
                    const std::vector<Polyline> inside = Polyline::boolean_op(loops[0], loop, plane, 0);
                    const double kept = inside.size() == 1 ? inside[0].area() : 0.0;
                    if (std::abs(kept - loop.area()) > CLOSE * std::max(1.0, loop.area()))
                        fail(row, "C1", where + " is a hole outside the outer ring");
                }
                if (face == 1 && features.bottom[i].point_count() != loop.point_count())
                    fail(row, "C1", where + fmt::format(" has {} points, its bottom loop {}", loop.point_count(), features.bottom[i].point_count()));
            }
        }
    }
}

/// C2: both members and the joint are closed single solids as mesh and as BRep, the two agreeing in volume; every piece the joint makes is a closed solid.
static void check_solids(const Built& built, Row& row) {

    for (const std::shared_ptr<Plate>& plate : {built.fixture.a, built.fixture.b}) {
        const std::string who = plate->name;
        const Mesh& mesh = plate->model_geometry_mesh();
        if (mesh.number_of_faces() == 0) {
            fail(row, "C2", who + " has no model mesh");
            continue;
        }
        if (!mesh.is_closed())
            fail(row, "C2", who + fmt::format(" model mesh is open, {} naked edges", mesh.naked_edges().size()));
        const double volume = compute_volume(mesh);
        if (!(volume > 0.0))
            fail(row, "C2", who + fmt::format(" model volume {:.6g}", volume));

        const BRep& brep = plate->model_geometry_brep();
        if (!brep.is_valid() || !brep.is_solid() || brep.solid_count() != 1)
            fail(row, "C2", who + fmt::format(" model BRep valid {} solid {} solids {}", brep.is_valid(), brep.is_solid(), brep.solid_count()));
        const double tolerance = cylinders(brep) > 0 ? ROUND_REL : VOLUME_REL;
        if (std::abs(brep.volume() - volume) > tolerance * std::max(volume, 1.0))
            fail(row, "C2", who + fmt::format(" BRep volume {:.9g} against mesh {:.9g}", brep.volume(), volume));
    }

    const Mesh& joint_mesh = built.joint->element_geometry_mesh();
    if (joint_mesh.number_of_faces() == 0 || !joint_mesh.is_closed())
        fail(row, "C2", fmt::format("the joint's own mesh has {} faces, closed {}", joint_mesh.number_of_faces(), joint_mesh.is_closed()));
    const BRep& joint_brep = built.joint->element_geometry_brep();
    if (!joint_brep.is_valid() || !joint_brep.is_solid())
        fail(row, "C2", fmt::format("the joint's own BRep valid {} solid {}", joint_brep.is_valid(), joint_brep.is_solid()));

    const std::vector<Mesh> pieces = joint_pieces(*built.joint);
    row.pieces = pieces.size();
    for (size_t i = 0; i < pieces.size(); i++) {
        if (!pieces[i].is_closed())
            fail(row, "C2", fmt::format("piece {} is not closed", i));
        else if (!(compute_volume(pieces[i]) > 0.0))
            fail(row, "C2", fmt::format("piece {} has volume {:.6g}", i, compute_volume(pieces[i])));
    }
}

/// C3: the members do not overlap after the joint; a cross pair overlapped before it, so the check is not empty.
static void check_overlap(const Built& built, Row& row) {

    const Plate& a = *built.fixture.a;
    const Plate& b = *built.fixture.b;
    row.volume_a = compute_volume(a.model_geometry_mesh());
    row.volume_b = compute_volume(b.model_geometry_mesh());
    row.overlap = boolean_volume(a.model_geometry_mesh(), b.model_geometry_mesh(), SolidOperation::intersect);

    if (row.overlap > ZERO_REL * std::min(row.volume_a, row.volume_b))
        fail(row, "C3", fmt::format("the members overlap by {:.6g} mm3", row.overlap));
    if (built.family == "cr") {
        const double before = boolean_volume(a.element_geometry_mesh(), b.element_geometry_mesh(), SolidOperation::intersect);
        if (!(before > 0.0))
            fail(row, "C3", "the crossing plates did not overlap before the joint");
    }
}

/// C4: nothing is made outside the pair's stock, nothing lost but by design, and the two models add up to their union.
static void check_conservation(const Built& built, Row& row) {

    const Plate& a = *built.fixture.a;
    const Plate& b = *built.fixture.b;
    const Mesh stock = solid_boolean(a.element_geometry_mesh(), b.element_geometry_mesh(), SolidOperation::add);
    const Mesh after = solid_boolean(a.model_geometry_mesh(), b.model_geometry_mesh(), SolidOperation::add);
    const double stock_volume = compute_volume(stock);
    const double after_volume = compute_volume(after);
    row.lost = stock_volume - after_volume;

    const double outside = boolean_volume(after, stock, SolidOperation::subtract);
    if (outside > ZERO_REL * stock_volume)
        fail(row, "C4", fmt::format("{:.6g} mm3 of material outside the stock", outside));
    if (row.lost < -VOLUME_REL * stock_volume)
        fail(row, "C4", fmt::format("the pair gained {:.6g} mm3", -row.lost));
    const double sum = compute_volume(a.model_geometry_mesh()) + compute_volume(b.model_geometry_mesh());
    if (std::abs(sum - after_volume - row.overlap) > ZERO_REL * after_volume)
        fail(row, "C4", fmt::format("the models sum to {:.9g} mm3, their union is {:.9g}", sum, after_volume));
}

/// C6: the stock one member lost to the joint is what the other now fills inside that stock, or the joint's own key when neither reaches into the other; a side removal fills nothing; a pin joint is measured by C7 instead.
static void check_fit(const Built& built, Row& row) {

    if (built.family == "tt")
        return;

    const Plate& a = *built.fixture.a;
    const Plate& b = *built.fixture.b;
    const double taken_a = boolean_volume(a.element_geometry_mesh(), a.model_geometry_mesh(), SolidOperation::subtract);
    const double taken_b = boolean_volume(b.element_geometry_mesh(), b.model_geometry_mesh(), SolidOperation::subtract);
    const double filled_by_b = boolean_volume(b.model_geometry_mesh(), a.element_geometry_mesh(), SolidOperation::intersect);
    const double filled_by_a = boolean_volume(a.model_geometry_mesh(), b.element_geometry_mesh(), SolidOperation::intersect);
    const double tolerance_a = ZERO_REL * compute_volume(a.element_geometry_mesh());
    const double tolerance_b = ZERO_REL * compute_volume(b.element_geometry_mesh());

    if (built.library.starts_with("side_removal")) {
        if (filled_by_a > tolerance_b || filled_by_b > tolerance_a)
            fail(row, "C6", fmt::format("a side removal fills {:.6g} and {:.6g} mm3 of the other member", filled_by_a, filled_by_b));
        if (!(taken_a + taken_b > 0.0))
            fail(row, "C6", "a side removal took nothing away");
        return;
    }

    // a key design: neither member reaches into the other, the joint's own solid fills both pockets
    if (filled_by_a <= tolerance_b && filled_by_b <= tolerance_a && taken_a > tolerance_a && taken_b > tolerance_b) {
        const double key = compute_volume(built.joint->element_geometry_mesh());
        if (std::abs(key - taken_a - taken_b) > tolerance_a + tolerance_b)
            fail(row, "C6", fmt::format("the members lost {:.6g} and {:.6g} mm3 to pockets, the joint's own solid is {:.6g}", taken_a, taken_b, key));
        return;
    }

    if (std::abs(taken_a - filled_by_b) > tolerance_a)
        fail(row, "C6", fmt::format("{} lost {:.6g} mm3, {} fills {:.6g} of it", a.name, taken_a, b.name, filled_by_b));
    if (std::abs(taken_b - filled_by_a) > tolerance_b)
        fail(row, "C6", fmt::format("{} lost {:.6g} mm3, {} fills {:.6g} of it", b.name, taken_b, a.name, filled_by_a));
}

/// C7: a pin joint declares axes, and each member is bored along the axes inside it: a drill feature and a cylinder per axis, the hole volume pi r^2 L.
static void check_drills(const Built& built, Row& row) {

    if (built.family != "tt")
        return;

    const std::vector<Line> axes = built.joint->drill_axes();
    row.drills = axes.size();
    if (axes.empty()) {
        fail(row, "C7", "the pin joint declares no drill axes");
        return;
    }

    const double radius = built.joint->line_radius;
    for (const std::shared_ptr<Plate>& plate : {built.fixture.a, built.fixture.b}) {
        size_t inside = 0;
        double length = 0.0;
        for (const Line& axis : axes) {
            if (!inside_plate(*plate, axis.point_at(0.5)))
                continue;
            inside++;
            length += axis.length();
        }
        const std::string who = plate->name;
        const size_t features = drills_of(*plate, built.joint->guid());
        if (features != inside)
            fail(row, "C7", fmt::format("{} carries {} drill features for {} axes inside it", who, features, inside));
        const size_t bores = cylinders(plate->model_geometry_brep());
        if (bores != inside)
            fail(row, "C7", fmt::format("{} BRep has {} cylinders for {} axes inside it", who, bores, inside));
        const double expected = std::numbers::pi * radius * radius * length;
        const double removed = compute_volume(plate->element_geometry_mesh()) - compute_volume(plate->model_geometry_mesh());
        if (std::abs(removed - expected) > HOLE_REL * std::max(expected, 1.0))
            fail(row, "C7", fmt::format("{} lost {:.6g} mm3 to its holes, pi r^2 L gives {:.6g}", who, removed, expected));
    }
}

/// C10a: the scene through the protobuf keeps every plate's model volume and merged outlines.
static void check_round_trip(const Built& built, Row& row) {

    const WoodSession restored = WoodSession::pb_loads(built.scene->pb_dumps());
    for (const std::shared_ptr<Plate>& plate : {built.fixture.a, built.fixture.b}) {
        const std::shared_ptr<Plate> twin = restored.get_element<Plate>(plate->guid());
        if (!twin) {
            fail(row, "C10", plate->name + " is missing after the round trip");
            continue;
        }
        const double volume = compute_volume(plate->model_geometry_mesh());
        const double twin_volume = compute_volume(twin->model_geometry_mesh());
        if (std::abs(volume - twin_volume) > VOLUME_REL * volume)
            fail(row, "C10", fmt::format("{} volume {:.9g} comes back as {:.9g}", plate->name, volume, twin_volume));
        const std::vector<Polyline> loops = oracle::merged_loops(*plate);
        const std::vector<Polyline> twin_loops = oracle::merged_loops(*twin);
        if (loops.size() != twin_loops.size()) {
            fail(row, "C10", fmt::format("{} has {} loops, {} after the round trip", plate->name, loops.size(), twin_loops.size()));
            continue;
        }
        for (size_t i = 0; i < loops.size(); i++)
            if (oracle::point_set_distance(loops[i], twin_loops[i]) > 1e-9)
                fail(row, "C10", fmt::format("{} loop {} moves {:.3g} mm through the round trip", plate->name, i, oracle::point_set_distance(loops[i], twin_loops[i])));
    }
    const std::shared_ptr<JointPlate> joint = restored.get_element<JointPlate>(built.joint->guid());
    if (!joint || joint->connections.size() != built.joint->connections.size() || !restored.consistent())
        fail(row, "C10", "the joint does not come back whole");
}

/// C10b: the same variant built on the pair moved by a rotation and a translation gives the same volumes and the moved outlines; a mirror is refused.
static void check_rigid_motion(const Built& built, Row& row) {

    const Xform motion = Xform::translation(1234.0, -567.0, 89.0) * Xform::rotation_z(37.0, true);
    const Built moved = build_variant(row.id, motion);

    const std::array<std::shared_ptr<Plate>, 2> plates = {built.fixture.a, built.fixture.b};
    const std::array<std::shared_ptr<Plate>, 2> moved_plates = {moved.fixture.a, moved.fixture.b};
    for (size_t k = 0; k < 2; k++) {
        const double volume = compute_volume(plates[k]->model_geometry_mesh());
        const double moved_volume = compute_volume(moved_plates[k]->model_geometry_mesh());
        if (std::abs(volume - moved_volume) > VOLUME_REL * volume)
            fail(row, "C10", fmt::format("{} volume {:.9g} becomes {:.9g} under a rigid motion", plates[k]->name, volume, moved_volume));
        const std::vector<Polyline> loops = oracle::merged_loops(*plates[k]);
        const std::vector<Polyline> moved_loops = oracle::merged_loops(*moved_plates[k]);
        if (loops.size() != moved_loops.size()) {
            fail(row, "C10", fmt::format("{} has {} loops, {} under a rigid motion", plates[k]->name, loops.size(), moved_loops.size()));
            continue;
        }
        for (size_t i = 0; i < loops.size(); i++) {
            const double deviation = oracle::point_set_distance(loops[i].transformed(motion), moved_loops[i]);
            if (deviation > 1e-6)
                fail(row, "C10", fmt::format("{} loop {} deviates {:.3g} mm under a rigid motion", plates[k]->name, i, deviation));
        }
    }

    if (built.fixture.a->transformed(Xform::scale_xyz(-1.0, 1.0, 1.0)) != nullptr)
        fail(row, "C10", "a mirrored plate is not refused");
}

// ═══════════════════════════════════════════════════════════════════════════
// C11: the golden snapshot of a variant - merged outlines, pieces and drill axes as numbers
// ═══════════════════════════════════════════════════════════════════════════

/// The numbers of a variant, one per token: the loops of both plates, every piece's two loops, every drill axis.
static std::vector<double> snapshot(const Built& built, std::vector<std::string>& labels) {

    std::vector<double> numbers;
    for (const std::shared_ptr<Plate>& plate : {built.fixture.a, built.fixture.b}) {
        const std::vector<Polyline> loops = oracle::merged_loops(*plate);
        labels.push_back(fmt::format("plate {} loops {}", plate->name, loops.size()));
        numbers.push_back(static_cast<double>(loops.size()));
        for (const Polyline& loop : loops) {
            numbers.push_back(static_cast<double>(loop.point_count()));
            for (const Point& point : loop.get_points())
                numbers.insert(numbers.end(), {point[0], point[1], point[2]});
        }
    }
    for (const InteractionFeaturePlate& connection : built.joint->connections) {
        for (int side = 0; side < 2; side++) {
            const std::array<std::vector<Polyline>, 2>& outlines = side == 0 ? connection.male_outlines : connection.female_outlines;
            for (int face = 0; face < 2; face++) {
                labels.push_back(fmt::format("{} face {} outlines {}", side == 0 ? "male" : "female", face, outlines[face].size()));
                numbers.push_back(static_cast<double>(outlines[face].size()));
                for (const Polyline& outline : outlines[face]) {
                    numbers.push_back(static_cast<double>(outline.point_count()));
                    for (const Point& point : outline.get_points())
                        numbers.insert(numbers.end(), {point[0], point[1], point[2]});
                }
            }
        }
    }
    const std::vector<Line> axes = built.joint->drill_axes();
    labels.push_back(fmt::format("drill axes {}", axes.size()));
    numbers.push_back(static_cast<double>(axes.size()));
    for (const Line& axis : axes)
        numbers.insert(numbers.end(), {axis.start()[0], axis.start()[1], axis.start()[2], axis.end()[0], axis.end()[1], axis.end()[2]});

    return numbers;
}

/// Writes the golden file on the first run or when WOOD_UPDATE_GOLDEN is set, else compares with it at GOLDEN_TOL and reports the largest deviation.
static void check_golden(const Built& built, Row& row) {

    std::vector<std::string> labels;
    const std::vector<double> numbers = snapshot(built, labels);
    const std::filesystem::path path = std::filesystem::path(GOLDEN_DIR) / (file_name(row.id) + ".txt");
    const bool update = std::getenv("WOOD_UPDATE_GOLDEN") != nullptr;

    if (update || !std::filesystem::exists(path)) {
        std::filesystem::create_directories(GOLDEN_DIR);
        std::ofstream file(path);
        file << std::setprecision(12);
        file << "variant " << row.id << "\n";
        for (const std::string& label : labels)
            file << "# " << label << "\n";
        for (size_t i = 0; i < numbers.size(); i++)
            file << numbers[i] << (i + 1 < numbers.size() ? " " : "\n");
        std::cout << (update ? "golden rewritten " : "golden written ") << path.filename().string() << "\n";
        return;
    }

    std::ifstream file(path);
    std::string line;
    std::vector<double> golden;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line.starts_with("variant"))
            continue;
        std::istringstream stream(line);
        double value = 0.0;
        while (stream >> value)
            golden.push_back(value);
    }

    if (golden.size() != numbers.size()) {
        fail(row, "C11", fmt::format("{} numbers against {} in {}", numbers.size(), golden.size(), path.filename().string()));
        return;
    }
    double worst = 0.0;
    for (size_t i = 0; i < numbers.size(); i++)
        worst = std::max(worst, std::abs(numbers[i] - golden[i]));
    if (worst > GOLDEN_TOL)
        fail(row, "C11", fmt::format("deviates {:.6g} from {}", worst, path.filename().string()));
}

// ═══════════════════════════════════════════════════════════════════════════
// The run
// ═══════════════════════════════════════════════════════════════════════════

/// A check by name; each runs under its own guard, so one that throws is a failure of that variant and the next still runs.
struct Check {
    const char* name;
    void (*run)(const Built&, Row&);
};

static const Check CHECKS[] = {
    {"C1", check_outlines},
    {"C2", check_solids},
    {"C3", check_overlap},
    {"C4", check_conservation},
    {"C6", check_fit},
    {"C7", check_drills},
    {"C10", check_round_trip},
    {"C10", check_rigid_motion},
    {"C11", check_golden},
};

/// One variant built and put through every check.
static Row run_variant(const std::string& id) {

    Row row;
    row.id = id;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    try {
        const Built built = build_variant(id, Xform::identity());
        for (const Check& check : CHECKS) {
            try {
                check.run(built, row);
            } catch (const std::exception& e) {
                fail(row, check.name, std::string("throws: ") + e.what());
            }
        }
    } catch (const std::exception& e) {
        fail(row, "build", std::string("throws: ") + e.what());
    }

    row.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return row;
}

int main(int argc, char** argv) {

    std::vector<std::string> ids;
    for (int i = 1; i < argc; i++)
        ids.emplace_back(argv[i]);
    if (ids.empty())
        ids = VARIANTS;

    std::vector<Row> rows;
    for (const std::string& id : ids)
        rows.push_back(run_variant(id));

    // the table
    std::cout << std::left << std::setw(34) << "variant" << std::right << std::setw(12) << "V_a" << std::setw(12) << "V_b" << std::setw(12) << "lost"
              << std::setw(11) << "overlap" << std::setw(7) << "pieces" << std::setw(7) << "drills" << std::setw(7) << "ms" << "  status\n";
    size_t failed = 0;
    for (const Row& row : rows) {
        failed += !row.failures.empty();
        std::cout << std::left << std::setw(34) << row.id << std::right << std::fixed << std::setprecision(0) << std::setw(12) << row.volume_a << std::setw(12) << row.volume_b
                  << std::setprecision(1) << std::setw(12) << row.lost << std::setprecision(3) << std::setw(11) << row.overlap << std::setw(7) << row.pieces << std::setw(7) << row.drills
                  << std::setprecision(0) << std::setw(7) << row.ms << "  " << (row.failures.empty() ? "ok" : fmt::format("{} failures", row.failures.size())) << "\n";
    }
    for (const std::string& skipped : SKIPPED)
        std::cout << "skipped " << skipped << "\n";

    // the failures
    for (const Row& row : rows)
        for (const std::string& failure : row.failures)
            std::cerr << row.id << " | " << failure << "\n";
    std::cout << fmt::format("\njoint library: {} / {} variants pass, {} skipped\n", rows.size() - failed, rows.size(), SKIPPED.size());

    return failed > 0 ? 1 : 0;
}
