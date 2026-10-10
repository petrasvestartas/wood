#include "oracle_polyline.h"
#include "session.pb.h"
#include <chrono>
#include <iomanip>
#include <set>
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
static const double OVERCUT_REL = 2e-3; // relative to a member, the overlap of a design 2024 over-cut on purpose (overcut_by_design): measured 0 to 4044 mm3 of 3.03e6 on the fixture, 72000 with the rings left open
static const double UNFILLED_REL = 0.1; // relative to the stock a member lost, what the other leaves unfilled in a design 2024 over-cut: measured 4.5 % to 7.5 % for cr_c_ip_2 to cr_c_ip_4 (7488 to 12499 of 165133 to 167658 mm3)
static const double UNFILLED_BITS_REL = 0.4; // the same for cr_c_ip_5, whose bits nothing fills: measured 28 % to 31 % (52236 of 186118, 63518 of 202471 mm3)
static const double HOLE_REL = 1e-2; // relative, the polygonal drill mesh against pi r^2 L
static const double GOLDEN_TOL = 1e-6; // mm, a golden coordinate
static const double CONTACT_GRID = 0.01; // mm, the Clipper grid of a face contact: the 2024 solver clipped the face quads at two decimals in the face's own frame, so a joint moved rigidly lands on another grid and its outlines move by up to this
static const std::string GOLDEN_DIR = std::string(WOOD_SOURCE_DIR) + "/tests/golden/joint_library";
static const std::string PASSING_FILE = GOLDEN_DIR + "/passing.txt"; // The variants that passed when the list was last accepted: one of them failing fails the run.

/// Every design of the library with its default and a non-default parameter set: "family/library/parameters...". The top-top rings and lattices keep their offset above the pin radius: a hole tangent to a side face is a BRep boolean no kernel takes. ss_e_op_4 keeps its female outline modified: without it its mortises lie outside the mitred face, whole only as the linked joint of ss_e_op_5. ss_e_op_4 to ss_e_op_6 fail C4 and C6 on the mitred fixture by 2024's construction, the floor's edge a thickness past the mitre and the tenons out of the floor's far face: the port matches the 2025 reference point for point there, the dataset out_of_plane_mitre. ts_e_p_3 stays off the shifts 0 and 1: there its tenon sides lean by a whole point spacing and the mortise rectangles fold onto themselves, in 2024 as here. cr_c_ip_2 to cr_c_ip_5 stay below the shift 0.85: there the 0.6 extension of the bottom sides' slanted segments crosses their upper ends over and the ring folds onto itself, in 2024 as here.
static const std::vector<std::string> VARIANTS = {
    "ip/ss_e_ip_0", "ip/ss_e_ip_1", "ip/ss_e_ip_1/8/0.5", "ip/ss_e_ip_1/4/0.0", "ip/ss_e_ip_1/16/1.0", "ip/ss_e_ip_2", "ip/ss_e_ip_2/4",
    "ip/ss_e_ip_2/2", "ip/ss_e_ip_3", "ip/ss_e_ip_4", "ip/ss_e_ip_5", "ip/ss_e_ip_5/4", "ip/ss_e_ip_5/6", "ip/ss_e_ip_custom",
    "ip/side_removal/0/0.5",
    "op/ss_e_op_0", "op/ss_e_op_1", "op/ss_e_op_1/8/0.5", "op/ss_e_op_1/6/0.0", "op/ss_e_op_2", "op/ss_e_op_2/8/0.5", "op/ss_e_op_2/12/1.0",
    "op/ss_e_op_3", "op/ss_e_op_4", "op/ss_e_op_4/8/0/0/1", "op/ss_e_op_4/8/0.1/1/1", "op/ss_e_op_4/8/0.5/0/1", "op/ss_e_op_5", "op/ss_e_op_5/8/0",
    "op/ss_e_op_5/8/1", "op/ss_e_op_6", "op/ss_e_op_6/8", "op/ss_e_op_17/4", "op/ss_e_op_tutorial", "op/ss_e_op_custom", "op/side_removal/1/0.5",
    "ts/ts_e_p_0", "ts/ts_e_p_1", "ts/ts_e_p_2", "ts/ts_e_p_2/8/0.5", "ts/ts_e_p_2/16/0.25", "ts/ts_e_p_3", "ts/ts_e_p_3/8/0.5",
    "ts/ts_e_p_3/16/0.25", "ts/ts_e_p_3/24/0.75", "ts/ts_e_p_4", "ts/ts_e_p_custom", "ts/side_removal/0/0.5",
    "r/ss_e_r_0", "r/ss_e_r_2", "r/ss_e_r_2/4/0.5", "r/ss_e_r_2/2/0.25", "r/ss_e_r_3", "r/ss_e_r_3/4/0.5", "r/ss_e_r_3/6/1.0",
    "r/ss_e_r_custom", "r/side_removal/0/0.5", "r/side_removal/1/0.5", "r/side_removal_ss_e_r_1/0/0.5", "r/side_removal_ss_e_r_1/1/0.5",
    "cr/cr_c_ip_0", "cr/cr_c_ip_1/0.5", "cr/cr_c_ip_1/0.25", "cr/cr_c_ip_2", "cr/cr_c_ip_2/0.0", "cr/cr_c_ip_3", "cr/cr_c_ip_3/0.75", "cr/cr_c_ip_4",
    "cr/cr_c_ip_4/0.25", "cr/cr_c_ip_5", "cr/cr_c_ip_5/0.75", "cr/cr_c_ip_custom",
    "tt/tt_e_p_0/8", "tt/tt_e_p_1/8", "tt/tt_e_p_2/6/0.95/8", "tt/tt_e_p_2/5/0.5/8", "tt/tt_e_p_3/60/12/8", "tt/tt_e_p_3/30/12/8", "tt/tt_e_p_4/60/12/8",
    "tt/tt_e_p_5/60/0.95/8", "tt/tt_e_p_5/-60/0.95/8", "tt/tt_e_p_custom",
};

/// Designs no fixture can orient or no pair check can hold: the boundary type 60 joins one plate on its border contact, not a pair; ts_e_p_5's 2024 literals put its mortises 3.65e-6 units inside the base's faces and run its snap-fit hook 3.4 units past the base's top, through the base and out below it, so its loops leave the faces and its material leaves the stock by design, and its copies overlap on a 250 mm joint line from four divisions up; the datasets top_to_side_box and top_to_side_snap_fit prove it against the 2025 reference.
static const std::vector<std::string> SKIPPED = {
    "b/b_0: a border joint on one plate, no pair to check (tests/joint_border checks it against the 2025 reference)",
    "b/b_custom: a border joint on one plate, no pair to check, no 2025 reference reaches it",
    "ts/ts_e_p_5: the 2024 literals leave the faces by 1.5e-4 mm and the hook leaves the stock by design (dataset only)",
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
    double own = 0.0; // The volume of the joint's own solid: a key, else zero.
    size_t drills = 0;
    double ms = 0.0;
    std::vector<std::vector<double>> keys; // Each loose key's shape: its volume, then its loops' edge lengths sorted.
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

/// The angle fixtures per family, each running every design of the family on its defaults (a variant id of two parts): the floor and wall at
/// 120 and 150 degrees beside the right angle, the rotated pair folded 90, 120 and 150 degrees, the upright skewed in plan at 60 and 75 degrees
/// and leaning at 80, the crossing at 60 and 45 degrees, and the in-plane pair on a slanted seam and on a seam shorter than both plates.
static const std::vector<std::pair<std::string, std::string>> ANGLE_FIXTURES = {
    {"op", "120"}, {"op", "150"},
    {"r", "90"}, {"r", "120"}, {"r", "150"},
    {"ts", "skew60"}, {"ts", "skew75"}, {"ts", "lean80"},
    {"cr", "60"}, {"cr", "45"},
    {"ip", "trapezoid"}, {"ip", "short"},
};

/// The loose-key designs, whose keys must keep their shape on every fixture: never sheared or scaled by the joint volume's change of basis.
static const std::vector<std::string> RIGID_KEYS = {"ss_e_ip_2", "ss_e_ip_5", "ss_e_r_2", "ss_e_r_3"};

// ═══════════════════════════════════════════════════════════════════════════
// Fixtures - the pairs of the element examples, kept joined, moved by xform before the contact is found
// ═══════════════════════════════════════════════════════════════════════════

/// Two 40 thick plates edge to edge in one plane: ss_e_ip, ss_e_r (with the scene set to treat every side-side joint as rotated) and
/// side removal. The shape is "" for two 300 x 400 plates, "trapezoid" for the seam slanted to 75 degrees from the bottom edges, "short" for a 300 x 200
/// right plate in the middle of the left one's 400 edge, a seam shorter than both plates.
static Fixture pair_in_plane(WoodSession& scene, const Xform& xform, const std::string& shape = "") {

    // the slanted seam's top end, the seam at 75 degrees to the bottom edges
    const double slant = 300.0 - 400.0 / std::tan(75.0 * std::numbers::pi / 180.0);
    std::array<std::vector<Point>, 2> outlines;
    if (shape == "trapezoid")
        outlines = {{{{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {slant, 400.0, 0.0}, {0.0, 400.0, 0.0}}, {{300.0, 0.0, 0.0}, {600.0, 0.0, 0.0}, {600.0, 400.0, 0.0}, {slant, 400.0, 0.0}}}};
    else if (shape == "short")
        outlines = {{{{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {0.0, 400.0, 0.0}}, {{300.0, 100.0, 0.0}, {600.0, 100.0, 0.0}, {600.0, 300.0, 0.0}, {300.0, 300.0, 0.0}}}};

    Fixture f;
    if (shape.empty()) {
        f.a = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 300.0, 400.0, 40.0, "left");
        f.b = Plate::from_rectangle({300.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 300.0, 400.0, 40.0, "right");
    } else {
        const Polyline left = Polyline(outlines[0]).closed();
        const Polyline right = Polyline(outlines[1]).closed();
        f.a = std::make_shared<Plate>(left, left.transformed(Xform::translation(0.0, 0.0, 40.0)), "left");
        f.b = std::make_shared<Plate>(right, right.transformed(Xform::translation(0.0, 0.0, 40.0)), "right");
    }
    f.a->place(xform);
    f.b->place(xform);
    scene.add(f.a);
    scene.add(f.b);
    f.face = scene.compute_face_contact(f.a, f.b);
    f.target0 = f.a;
    f.target1 = f.b;

    return f;
}

/// A 300 x 400 floor and a 300 high wall, both 40 thick, meeting at the dihedral angle (degrees) on the 400 seam, their side faces mitred on the
/// bisector through the outer and the inner corner: ss_e_op, and ss_e_r folded; at 90 the right-angle corner of the examples. The joint's first
/// side is the wall, the male of an out-of-plane pair being the second plate of the contact.
static Fixture pair_out_of_plane(WoodSession& scene, const Xform& xform, double dihedral = 90.0) {

    // the wall leaves the outer corner at the dihedral from the floor's -x, its inner face 40 toward the floor, the mitre through the inner corner
    const double angle = dihedral * std::numbers::pi / 180.0;
    const Vector up(-std::cos(angle), 0.0, std::sin(angle));
    const Vector inward(-std::sin(angle), 0.0, -std::cos(angle));
    const Vector along(0.0, 400.0, 0.0);
    const Point outer(300.0, 0.0, 0.0);
    const double inset = 40.0 * (1.0 + std::cos(angle)) / std::sin(angle);
    const Point inner(300.0 - inset, 0.0, 40.0);
    const Point wall_end = outer + up * 300.0;

    const Polyline floor_bottom({{0.0, 0.0, 0.0}, outer, outer + along, {0.0, 400.0, 0.0}, {0.0, 0.0, 0.0}});
    const Polyline floor_top({{0.0, 0.0, 40.0}, inner, inner + along, {0.0, 400.0, 40.0}, {0.0, 0.0, 40.0}});
    const Polyline wall_bottom({outer, outer + along, wall_end + along, wall_end, outer});
    const Polyline wall_top({inner, inner + along, wall_end + inward * 40.0 + along, wall_end + inward * 40.0, inner});

    Fixture f;
    f.a = std::make_shared<Plate>(floor_bottom, floor_top, "floor");
    f.b = std::make_shared<Plate>(wall_bottom, wall_top, "wall");
    f.a->place(xform);
    f.b->place(xform);
    scene.add(f.a);
    scene.add(f.b);
    f.face = scene.compute_face_contact(f.a, f.b);
    f.target0 = f.b;
    f.target1 = f.a;

    return f;
}

/// An upright 250 x 250 x 40 plate standing on the middle of a 400 x 400 base: ts_e_p; the joint's first side is the upright. Its foot runs at
/// the plan angle (degrees) to the base's x axis, 90 along y, and it leans toward its normal to the lean angle from the base, 90 upright, its
/// foot kept flat on the base and its height 250.
static Fixture pair_top_side(WoodSession& scene, const Xform& xform, double plan = 90.0, double lean = 90.0) {

    const double plan_angle = plan * std::numbers::pi / 180.0;
    const double tilt = (90.0 - lean) * std::numbers::pi / 180.0;
    const Vector foot(std::cos(plan_angle), std::sin(plan_angle), 0.0);
    const Vector normal = foot.cross(Vector(0.0, 0.0, 1.0));
    const Vector rise = Vector(0.0, 0.0, 250.0) + normal * (250.0 * std::tan(tilt));
    const Point corner = Point(200.0, 200.0, 40.0) - foot * 125.0 - normal * 20.0;
    const Polyline face({corner, corner + foot * 250.0, corner + foot * 250.0 + rise, corner + rise, corner});

    Fixture f;
    f.a = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 400.0, 40.0, "base");
    if (plan == 90.0 && lean == 90.0)
        f.b = Plate::from_rectangle({180.0, 75.0, 40.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 250.0, 250.0, 40.0, "upright");
    else
        f.b = std::make_shared<Plate>(face, face.transformed(Xform::translation(normal[0] * 40.0, normal[1] * 40.0, 0.0)), "upright");
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

/// Two upright 400 x 200 x 40 plates crossing at their middles, the second at the angle (degrees) to the first: cr_c_ip.
static Fixture pair_cross(WoodSession& scene, const Xform& xform, double angle = 90.0) {

    const double radians = angle * std::numbers::pi / 180.0;
    const Vector along(std::cos(radians), std::sin(radians), 0.0);
    const Vector normal = along.cross(Vector(0.0, 0.0, 1.0));
    const Point origin = Point(200.0, 0.0, 0.0) - along * 200.0 - normal * 20.0;

    Fixture f;
    f.a = Plate::from_rectangle({0.0, 20.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "first");
    f.b = Plate::from_rectangle(origin, along, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "second");
    f.a->place(xform);
    f.b->place(xform);
    scene.add(f.a);
    scene.add(f.b);
    f.cross = scene.compute_cross_contact(f.a, f.b);
    f.target0 = f.a;
    f.target1 = f.b;

    return f;
}

/// The fixture of a family token: the family prefix, then after an @ the angle or shape of an angle fixture (op@120, r@150, ts@skew60, ts@lean80, cr@45, ip@trapezoid).
static Fixture make_fixture(const std::string& token, WoodSession& scene, const Xform& xform) {

    const size_t at = token.find('@');
    const std::string family = token.substr(0, at);
    const std::string shape = at == std::string::npos ? "" : token.substr(at + 1);

    if (family == "ip")
        return pair_in_plane(scene, xform, shape);
    if (family == "r") {
        scene.settings.all_treated_as_rotated = true;
        scene.settings.rotated_joint_as_average = true;
        if (shape.empty())
            return pair_in_plane(scene, xform);
        // a rotated design keeps the contact's order
        Fixture f = pair_out_of_plane(scene, xform, std::stod(shape));
        f.target0 = f.a;
        f.target1 = f.b;
        return f;
    }
    if (family == "op")
        return pair_out_of_plane(scene, xform, shape.empty() ? 90.0 : std::stod(shape));
    if (family == "ts") {
        if (shape.starts_with("skew"))
            return pair_top_side(scene, xform, std::stod(shape.substr(4)), 90.0);
        if (shape.starts_with("lean"))
            return pair_top_side(scene, xform, 90.0, std::stod(shape.substr(4)));
        return pair_top_side(scene, xform);
    }
    if (family == "tt")
        return pair_top_top(scene, xform);
    if (family == "cr")
        return pair_cross(scene, xform, shape.empty() ? 90.0 : std::stod(shape));

    throw std::invalid_argument("no fixture for family " + token);
}

// ═══════════════════════════════════════════════════════════════════════════
// Variants - a library factory per id, the custom designs on unit outlines
// ═══════════════════════════════════════════════════════════════════════════

/// A closed rectangle of five points on one face of the unit box: `fixed_axis` held at `fixed`, spanning -1 to 1 on `swing_axis` and z0 to z1 along z, starting at `first_swing`, the end the merge's clip walks in from, so it lies outside the plate; what 2024 merged of a custom pair, clipped into the plate as a notch.
static Polyline rectangle(int fixed_axis, double fixed, int swing_axis, double z0, double z1, double first_swing = -1.0) {

    const std::array<double, 5> swing = {first_swing, -first_swing, -first_swing, first_swing, first_swing};
    const std::array<double, 5> z = {z0, z0, z1, z1, z0};
    std::vector<Point> points;

    for (size_t i = 0; i < 5; i++) {
        Point p(0.0, 0.0, z[i]);
        p[fixed_axis] = fixed;
        p[swing_axis] = swing[i];
        points.push_back(p);
    }

    return Polyline(points);
}

/// The male and female unit outlines of a custom design, face 0 then face 1 of each: the side and top-side families keep theirs pair by pair as 2024 did, so they get the rectangles 2024 merged, a notch into each member over its own stretch of the joint line (the top-side female lies on the base's face, where 2024 merged nothing, so only the upright is notched); the cross family stitches its pairs as edge insertions.
static std::array<std::vector<Polyline>, 2> custom_outlines(const std::string& family) {

    if (family == "op")
        return {std::vector<Polyline>{rectangle(1, 0.5, 0, 0.4, 0.1), rectangle(1, -0.5, 0, 0.4, 0.1)}, std::vector<Polyline>{rectangle(0, 0.5, 1, -0.1, -0.4), rectangle(0, -0.5, 1, -0.1, -0.4)}};
    if (family == "ts")
        return {std::vector<Polyline>{rectangle(0, 0.5, 1, 0.4, 0.1, 1.0), rectangle(0, -0.5, 1, 0.4, 0.1, 1.0)}, std::vector<Polyline>{rectangle(1, -0.5, 0, -0.1, -0.4), rectangle(1, 0.5, 0, -0.1, -0.4)}};
    if (family == "cr") {
        const Polyline male0({{0.5, 0.5, -1.0}, {-0.5, 0.5, -1.0}, {-0.5, 0.5, 0.0}, {0.5, 0.5, 0.0}, {0.5, 0.5, -1.0}});
        const Polyline male1({{0.5, -0.5, -1.0}, {-0.5, -0.5, -1.0}, {-0.5, -0.5, 0.0}, {0.5, -0.5, 0.0}, {0.5, -0.5, -1.0}});
        const Polyline female0({{-0.5, 0.5, 1.0}, {-0.5, -0.5, 1.0}, {-0.5, -0.5, 0.0}, {-0.5, 0.5, 0.0}, {-0.5, 0.5, 1.0}});
        const Polyline female1({{0.5, 0.5, 1.0}, {0.5, -0.5, 1.0}, {0.5, -0.5, 0.0}, {0.5, 0.5, 0.0}, {0.5, 0.5, 1.0}});
        return {std::vector<Polyline>{male0, male1}, std::vector<Polyline>{female0, female1}};
    }

    // in plane and rotated: a notch into the male's edge and one into the female's, over their own stretches
    return {std::vector<Polyline>{rectangle(1, -0.5, 0, 0.4, 0.1), rectangle(1, 0.5, 0, 0.4, 0.1)}, std::vector<Polyline>{rectangle(1, -0.5, 0, -0.1, -0.4), rectangle(1, 0.5, 0, -0.1, -0.4)}};
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
    if (library == "ss_e_ip_1") return JointPlate::ss_e_ip_1(integer(parts, 2, 0), number(parts, 3, 0.5));
    if (library == "ss_e_ip_2") return JointPlate::ss_e_ip_2(integer(parts, 2, 0));
    if (library == "ss_e_ip_3") return JointPlate::ss_e_ip_3();
    if (library == "ss_e_ip_4") return JointPlate::ss_e_ip_4();
    if (library == "ss_e_ip_5") return JointPlate::ss_e_ip_5(integer(parts, 2, 0));
    if (library == "ss_e_ip_custom") return JointPlate::ss_e_ip_custom(custom[0], custom[1]);

    if (library == "ss_e_op_0") return JointPlate::ss_e_op_0();
    if (library == "ss_e_op_1") return JointPlate::ss_e_op_1(integer(parts, 2, 0), number(parts, 3, 0.64));
    if (library == "ss_e_op_2") return JointPlate::ss_e_op_2(integer(parts, 2, 0), number(parts, 3, 0.64));
    if (library == "ss_e_op_3") return JointPlate::ss_e_op_3();
    if (library == "ss_e_op_4") return JointPlate::ss_e_op_4(integer(parts, 2, 0), number(parts, 3, 0.0), integer(parts, 4, 1) != 0, integer(parts, 5, 1) != 0);
    if (library == "ss_e_op_5") return JointPlate::ss_e_op_5(integer(parts, 2, 0), integer(parts, 3, 0) != 0);
    if (library == "ss_e_op_6") return JointPlate::ss_e_op_6(integer(parts, 2, 0));
    if (library == "ss_e_op_17") return JointPlate::ss_e_op_17(integer(parts, 2, 4));
    if (library == "ss_e_op_tutorial") return JointPlate::ss_e_op_tutorial();
    if (library == "ss_e_op_custom") return JointPlate::ss_e_op_custom(custom[0], custom[1]);

    if (library == "ts_e_p_0") return JointPlate::ts_e_p_0();
    if (library == "ts_e_p_1") return JointPlate::ts_e_p_1();
    if (library == "ts_e_p_2") return JointPlate::ts_e_p_2(integer(parts, 2, 0), number(parts, 3, 0.5));
    if (library == "ts_e_p_3") return JointPlate::ts_e_p_3(integer(parts, 2, 0), number(parts, 3, 0.5));
    if (library == "ts_e_p_4") return JointPlate::ts_e_p_4();
    if (library == "ts_e_p_5") return JointPlate::ts_e_p_5(integer(parts, 2, 0));
    if (library == "ts_e_p_custom") return JointPlate::ts_e_p_custom(custom[0], custom[1]);

    if (library == "ss_e_r_0") return JointPlate::ss_e_r_0();
    if (library == "ss_e_r_2") return JointPlate::ss_e_r_2(integer(parts, 2, 0), number(parts, 3, 0.5));
    if (library == "ss_e_r_3") return JointPlate::ss_e_r_3(integer(parts, 2, 0), number(parts, 3, 0.5));
    if (library == "ss_e_r_custom") return JointPlate::ss_e_r_custom(custom[0], custom[1]);

    if (library == "cr_c_ip_0") return JointPlate::cr_c_ip_0();
    if (library == "cr_c_ip_1") return JointPlate::cr_c_ip_1(number(parts, 2, 0.5));
    if (library == "cr_c_ip_2") return JointPlate::cr_c_ip_2(number(parts, 2, 0.5));
    if (library == "cr_c_ip_3") return JointPlate::cr_c_ip_3(number(parts, 2, 0.5));
    if (library == "cr_c_ip_4") return JointPlate::cr_c_ip_4(number(parts, 2, 0.5));
    if (library == "cr_c_ip_5") return JointPlate::cr_c_ip_5(number(parts, 2, 0.5));
    if (library == "cr_c_ip_custom") return JointPlate::cr_c_ip_custom(custom[0], custom[1]);

    if (library == "tt_e_p_0") return JointPlate::tt_e_p_0(number(parts, 2, 1.0));
    if (library == "tt_e_p_1") return JointPlate::tt_e_p_1(number(parts, 2, 1.0));
    if (library == "tt_e_p_2") return JointPlate::tt_e_p_2(integer(parts, 2, 6), number(parts, 3, 0.95), number(parts, 4, 1.0));
    if (library == "tt_e_p_3") return JointPlate::tt_e_p_3(number(parts, 2, 6.0), number(parts, 3, 0.95), number(parts, 4, 1.0));
    if (library == "tt_e_p_4") return JointPlate::tt_e_p_4(number(parts, 2, 6.0), number(parts, 3, 0.95), number(parts, 4, 1.0));
    if (library == "tt_e_p_5") return JointPlate::tt_e_p_5(number(parts, 2, 6.0), number(parts, 3, 0.95), number(parts, 4, 1.0));
    if (library == "tt_e_p_custom") return JointPlate::tt_e_p_custom(custom[0], custom[1]);

    if (library == "side_removal") return JointPlate::side_removal(integer(parts, 2, 0) != 0, number(parts, 3, 0.5));
    if (library == "side_removal_ss_e_r_1") return JointPlate::side_removal_ss_e_r_1(integer(parts, 2, 0) != 0, number(parts, 3, 0.5));

    throw std::invalid_argument("no factory for " + library);
}

/// The variant joined on its fixture, the pair moved by xform first: the joint oriented on the contact, added, and passed to each target.
static Built build_variant(const std::string& id, const Xform& xform) {

    const std::vector<std::string> parts = split(id, '/');
    Built built;
    built.family = parts[0].substr(0, parts[0].find('@'));
    built.library = parts[1];
    built.scene = std::make_shared<WoodSession>(id);
    built.fixture = make_fixture(parts[0], *built.scene, xform);
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

/// C2: both members are closed single solids as mesh and as BRep, the two agreeing in volume; the joint's own solid, when it has one (a key, its pins), is closed too, and a design whose every piece belongs to a member has none.
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
    if (joint_mesh.number_of_faces() == 0)
        return;
    if (!joint_mesh.is_closed())
        fail(row, "C2", fmt::format("the joint's own mesh is open, {} naked edges", joint_mesh.naked_edges().size()));
    const BRep& joint_brep = built.joint->element_geometry_brep();
    if (!joint_brep.is_valid() || !joint_brep.is_solid())
        fail(row, "C2", fmt::format("the joint's own BRep valid {} solid {}", joint_brep.is_valid(), joint_brep.is_solid()));

    const Mesh body = built.joint->body_mesh();
    if (body.number_of_faces() == 0)
        return;
    row.own = compute_volume(body);
    if (!body.is_closed() || !(row.own > 0.0))
        fail(row, "C2", fmt::format("the joint's own solid is closed {} with volume {:.6g}", body.is_closed(), row.own));
}

/// cr_c_ip_2 to cr_c_ip_5 as 2024 wrote them: the bottom sides of the half-lap are extended 0.15 along the plate and 0.6 along their slant "to compensate for irregularities", so the wedge each plate loses runs past the other plate's faces below the lap's middle, a margin the other does not fill, and stops 0.075 short of them at the middle, a sliver both plates keep; cr_c_ip_5 bores its bits besides, which nothing fills. The 2025 reference solver builds the same rings on this fixture, to 6e-6 mm, so the misfit is the design's: the oracle bounds it by what it measured (OVERCUT_REL, UNFILLED_REL, UNFILLED_BITS_REL) instead of expecting none.
static bool overcut_by_design(const std::string& library) {

    return library == "cr_c_ip_2" || library == "cr_c_ip_3" || library == "cr_c_ip_4" || library == "cr_c_ip_5";
}

/// What 2024's clip grid can leave of a crossing: the cross slots are merged into the outlines clipped at two decimals (CONTACT_GRID) in each
/// plate's own frame, so a slot wall oblique to that frame stands up to half a grid step off, over the walls the two stocks share: half the
/// grid times the surface of their crossing block. Zero for every other family and for a crossing at a right angle within a millionth.
static double clip_grid_allowance(const Built& built) {

    if (built.family != "cr")
        return 0.0;

    const Mesh block = solid_boolean(built.fixture.a->element_geometry_mesh(), built.fixture.b->element_geometry_mesh(), SolidOperation::intersect);
    return block.number_of_faces() == 0 ? 0.0 : 0.5 * CONTACT_GRID * block.area();
}

/// C3: the members do not overlap after the joint, within OVERCUT_REL for a design 2024 over-cut; a cross pair overlapped before it, so the check is not empty.
static void check_overlap(const Built& built, Row& row) {

    const Plate& a = *built.fixture.a;
    const Plate& b = *built.fixture.b;
    row.volume_a = compute_volume(a.model_geometry_mesh());
    row.volume_b = compute_volume(b.model_geometry_mesh());
    row.overlap = boolean_volume(a.model_geometry_mesh(), b.model_geometry_mesh(), SolidOperation::intersect);

    const double allowed = (overcut_by_design(built.library) ? OVERCUT_REL : ZERO_REL) * std::min(row.volume_a, row.volume_b) + clip_grid_allowance(built);
    if (row.overlap > allowed)
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

/// C6: the stock one member lost to the joint is what the other now fills inside that stock, or the joint's own key when neither reaches into the other; a side removal and a custom pair of a side or top-side family, which only take away, fill nothing, and so does ts_e_p_4, whose pockets take a loose wedge 2024 never modelled and whose male pieces lie in the base's thickness, under an upright standing on it; a pin joint is measured by C7 instead.
static void check_fit(const Built& built, Row& row) {

    if (built.family == "tt")
        return;

    const Plate& a = *built.fixture.a;
    const Plate& b = *built.fixture.b;
    const double taken_a = boolean_volume(a.element_geometry_mesh(), a.model_geometry_mesh(), SolidOperation::subtract);
    const double taken_b = boolean_volume(b.element_geometry_mesh(), b.model_geometry_mesh(), SolidOperation::subtract);
    const double filled_by_b = boolean_volume(b.model_geometry_mesh(), a.element_geometry_mesh(), SolidOperation::intersect);
    const double filled_by_a = boolean_volume(a.model_geometry_mesh(), b.element_geometry_mesh(), SolidOperation::intersect);
    const double grid = clip_grid_allowance(built);
    const double tolerance_a = ZERO_REL * compute_volume(a.element_geometry_mesh()) + grid;
    const double tolerance_b = ZERO_REL * compute_volume(b.element_geometry_mesh()) + grid;

    const bool removal_only = built.library.starts_with("side_removal") || built.library == "ss_e_ip_custom" || built.library == "ss_e_op_custom" || built.library == "ss_e_r_custom"
                              || built.library == "ts_e_p_custom" || built.library == "ts_e_p_4";
    if (removal_only) {
        if (filled_by_a > tolerance_b || filled_by_b > tolerance_a)
            fail(row, "C6", fmt::format("a removal fills {:.6g} and {:.6g} mm3 of the other member", filled_by_a, filled_by_b));
        if (!(taken_a + taken_b > 0.0))
            fail(row, "C6", "a removal took nothing away");
        return;
    }

    // a design 2024 over-cut: each member fills no more than the other lost, and leaves no more of it unfilled than the design measured
    if (overcut_by_design(built.library)) {
        const double unfilled = built.library == "cr_c_ip_5" ? UNFILLED_BITS_REL : UNFILLED_REL;
        if (filled_by_b > taken_a + tolerance_a || taken_a - filled_by_b > unfilled * taken_a)
            fail(row, "C6", fmt::format("{} lost {:.6g} mm3, {} fills {:.6g} of it", a.name, taken_a, b.name, filled_by_b));
        if (filled_by_a > taken_b + tolerance_b || taken_b - filled_by_a > unfilled * taken_b)
            fail(row, "C6", fmt::format("{} lost {:.6g} mm3, {} fills {:.6g} of it", b.name, taken_b, a.name, filled_by_a));
        return;
    }

    // a key design: neither member reaches into the other, the joint's own solid fills both pockets; its pins' ends out of the plates fill none
    if (filled_by_a <= tolerance_b && filled_by_b <= tolerance_a && taken_a > tolerance_a && taken_b > tolerance_b) {
        const Mesh stock = solid_boolean(a.element_geometry_mesh(), b.element_geometry_mesh(), SolidOperation::add);
        const double key = boolean_volume(built.joint->element_geometry_mesh(), stock, SolidOperation::intersect);
        if (std::abs(key - taken_a - taken_b) > tolerance_a + tolerance_b)
            fail(row, "C6", fmt::format("the members lost {:.6g} and {:.6g} mm3 to pockets, the joint's own solid is {:.6g}", taken_a, taken_b, key));
        return;
    }

    // a member reaches into the other: what one lost the other fills, and no loose piece is left for the joint to own
    if (std::abs(taken_a - filled_by_b) > tolerance_a)
        fail(row, "C6", fmt::format("{} lost {:.6g} mm3, {} fills {:.6g} of it", a.name, taken_a, b.name, filled_by_b));
    if (std::abs(taken_b - filled_by_a) > tolerance_b)
        fail(row, "C6", fmt::format("{} lost {:.6g} mm3, {} fills {:.6g} of it", b.name, taken_b, a.name, filled_by_a));
    if (row.own > tolerance_a + tolerance_b)
        fail(row, "C6", fmt::format("the members fill each other, yet the joint owns a solid of {:.6g} mm3", row.own));
}

/// C7: a pin joint declares axes, and every design with drills bores each member along the axes inside it: a drill feature and an exact cylinder per axis; on a pin joint the hole volume is pi r^2 L.
static void check_drills(const Built& built, Row& row) {

    // a custom pair carries the user's outlines with no type and no drills, as 2024 kept it; only a pin joint must declare drills
    const std::vector<Line> axes = built.joint->drill_axes();
    row.drills = axes.size();
    const bool pins = built.family == "tt" && built.library != "tt_e_p_custom";
    if (axes.empty()) {
        if (pins)
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
        // what a hole takes is measurable alone on a pin joint; a design that also mills or slices shares that volume
        if (!pins)
            continue;
        const double expected = std::numbers::pi * radius * radius * length;
        const double removed = compute_volume(plate->element_geometry_mesh()) - compute_volume(plate->model_geometry_mesh());
        if (std::abs(removed - expected) > HOLE_REL * std::max(expected, 1.0))
            fail(row, "C7", fmt::format("{} lost {:.6g} mm3 to its holes, pi r^2 L gives {:.6g}", who, removed, expected));
    }
}

/// C13: the geometry the file carries for each plate, as the viewer reads it from the raw session message, is the cut model: a BRep with one exact cylinder per bore the model has, of the model's volume.
static void check_written(const Built& built, Row& row) {

    session_proto::Session proto;
    if (!proto.ParseFromString(built.scene->pb_dumps())) {
        fail(row, "C13", "the written session does not parse");
        return;
    }

    for (const std::shared_ptr<Plate>& plate : {built.fixture.a, built.fixture.b}) {
        const session_proto::Element* written = nullptr;
        for (const session_proto::Element& element : proto.objects().elements())
            if (element.guid() == plate->guid())
                written = &element;
        if (!written) {
            fail(row, "C13", plate->name + " is not in the written file");
            continue;
        }

        const double model = compute_volume(plate->model_geometry_mesh());
        const size_t bores = cylinders(plate->model_geometry_brep());
        double volume = 0.0;
        size_t written_bores = 0;
        if (written->geometry_type() == "BRep") {
            const BRep brep = BRep::pb_loads(written->geometry_data());
            volume = brep.volume();
            written_bores = cylinders(brep);
        } else {
            volume = compute_volume(Mesh::pb_loads(written->geometry_data()));
        }

        if (bores > 0 && written->geometry_type() != "BRep")
            fail(row, "C13", fmt::format("{} has {} bores but is written as a {}, its holes faceted", plate->name, bores, written->geometry_type()));
        if (written_bores != bores)
            fail(row, "C13", fmt::format("{} is written with {} exact cylinders, its model has {}", plate->name, written_bores, bores));
        const double tolerance = bores > 0 ? ROUND_REL : VOLUME_REL;
        if (std::abs(volume - model) > tolerance * model)
            fail(row, "C13", fmt::format("{} is written with {:.9g} mm3, its cut model has {:.9g}", plate->name, volume, model));
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

/// C10b: the same variant built on the pair moved by a rotation and a translation gives the same volumes and the moved outlines within the contact grid, the volumes within the grid swept over the member's surface; a mirror is refused.
static void check_rigid_motion(const Built& built, Row& row) {

    const Xform motion = Xform::translation(1234.0, -567.0, 89.0) * Xform::rotation_z(37.0, true);
    const Built moved = build_variant(row.id, motion);

    const std::array<std::shared_ptr<Plate>, 2> plates = {built.fixture.a, built.fixture.b};
    const std::array<std::shared_ptr<Plate>, 2> moved_plates = {moved.fixture.a, moved.fixture.b};
    for (size_t k = 0; k < 2; k++) {
        const Mesh& mesh = plates[k]->model_geometry_mesh();
        const double volume = compute_volume(mesh);
        const double moved_volume = compute_volume(moved_plates[k]->model_geometry_mesh());
        if (std::abs(volume - moved_volume) > std::max(VOLUME_REL * volume, CONTACT_GRID * mesh.area()))
            fail(row, "C10", fmt::format("{} volume {:.9g} becomes {:.9g} under a rigid motion", plates[k]->name, volume, moved_volume));
        const std::vector<Polyline> loops = oracle::merged_loops(*plates[k]);
        const std::vector<Polyline> moved_loops = oracle::merged_loops(*moved_plates[k]);
        if (loops.size() != moved_loops.size()) {
            fail(row, "C10", fmt::format("{} has {} loops, {} under a rigid motion", plates[k]->name, loops.size(), moved_loops.size()));
            continue;
        }
        for (size_t i = 0; i < loops.size(); i++) {
            const double deviation = oracle::point_set_distance(loops[i].transformed(motion), moved_loops[i]);
            if (deviation > CONTACT_GRID)
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

/// C14, measured: each loose key's shape on this fixture, its volume and its loops' edge lengths sorted, compared across the fixtures in main.
static void measure_keys(const Built& built, Row& row) {

    if (std::find(RIGID_KEYS.begin(), RIGID_KEYS.end(), built.library) == RIGID_KEYS.end())
        return;

    const Joint& joint = *built.joint;
    for (const std::array<Polyline, 2>& body : joint.bodies()) {
        std::vector<double> shape = {compute_volume(Mesh::loft({body[0]}, {body[1]}, true))};
        std::vector<double> edges;
        for (const Polyline& loop : body)
            for (size_t i = 0; i + 1 < loop.point_count(); i++)
                edges.push_back(loop[i].distance(loop[i + 1]));
        std::sort(edges.begin(), edges.end());
        shape.insert(shape.end(), edges.begin(), edges.end());
        row.keys.push_back(shape);
    }
}

/// C14: every loose key on an angle fixture has the shape of a key on the design's own fixture, within VOLUME_REL of its volume and CLOSE of
/// each edge: the keys are rigid, whatever the angle or the seam.
static void check_rigid_keys(std::vector<Row>& rows) {

    for (Row& row : rows) {
        const size_t at = row.id.find('@');
        if (at == std::string::npos || row.keys.empty())
            continue;
        const std::string reference_id = row.id.substr(0, at) + row.id.substr(row.id.find('/'));
        const Row* reference = nullptr;
        for (const Row& other : rows)
            if (other.id == reference_id)
                reference = &other;
        if (!reference || reference->keys.empty()) {
            fail(row, "C14", "no keys on the design's own fixture to compare with");
            continue;
        }
        for (size_t k = 0; k < row.keys.size(); k++) {
            bool matched = false;
            for (const std::vector<double>& shape : reference->keys) {
                if (shape.size() != row.keys[k].size())
                    continue;
                bool same = std::abs(shape[0] - row.keys[k][0]) <= VOLUME_REL * std::max(shape[0], 1.0) + CLOSE;
                for (size_t i = 1; same && i < shape.size(); i++)
                    same = std::abs(shape[i] - row.keys[k][i]) <= 1e-6;
                matched = matched || same;
            }
            if (!matched)
                fail(row, "C14", fmt::format("key {} of {:.6g} mm3 has no counterpart among the {} keys of {}", k, row.keys[k][0], reference->keys.size(), reference_id));
        }
    }
}

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
    {"C13", check_written},
    {"C10", check_round_trip},
    {"C10", check_rigid_motion},
    {"C11", check_golden},
    {"C14", measure_keys},
};

/// The checks of an angle fixture: outlines, solids, overlap, conservation, fit, the goldens and the keys' shapes.
static const Check ANGLE_CHECKS[] = {
    {"C1", check_outlines},
    {"C2", check_solids},
    {"C3", check_overlap},
    {"C4", check_conservation},
    {"C6", check_fit},
    {"C11", check_golden},
    {"C14", measure_keys},
};

/// One variant built and put through every check.
static Row run_variant(const std::string& id) {

    Row row;
    row.id = id;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    try {
        const Built built = build_variant(id, Xform::identity());
        const bool angle = id.find('@') != std::string::npos;
        const Check* checks = angle ? ANGLE_CHECKS : CHECKS;
        const size_t count = angle ? std::size(ANGLE_CHECKS) : std::size(CHECKS);
        for (size_t c = 0; c < count; c++) {
            try {
                checks[c].run(built, row);
            } catch (const std::exception& e) {
                fail(row, checks[c].name, std::string("throws: ") + e.what());
            }
        }
    } catch (const std::exception& e) {
        fail(row, "build", std::string("throws: ") + e.what());
    }

    row.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return row;
}

/// The variants PASSING_FILE lists, one id per line.
static std::set<std::string> accepted_passes() {

    std::set<std::string> ids;
    std::ifstream file(PASSING_FILE);
    std::string line;
    while (std::getline(file, line))
        if (!line.empty())
            ids.insert(line);

    return ids;
}

int main(int argc, char** argv) {

    std::vector<std::string> ids;
    for (int i = 1; i < argc; i++)
        ids.emplace_back(argv[i]);
    if (ids.empty()) {
        ids = VARIANTS;
        // every design on its defaults on each angle fixture of its family
        for (const std::pair<std::string, std::string>& fixture : ANGLE_FIXTURES)
            for (const std::string& id : VARIANTS) {
                const std::vector<std::string> parts = split(id, '/');
                if (parts.size() == 2 && parts[0] == fixture.first)
                    ids.push_back(fixture.first + "@" + fixture.second + "/" + parts[1]);
            }
    }

    std::vector<Row> rows;
    for (const std::string& id : ids)
        rows.push_back(run_variant(id));
    check_rigid_keys(rows);

    // the table
    std::cout << std::left << std::setw(34) << "variant" << std::right << std::setw(12) << "V_a" << std::setw(12) << "V_b" << std::setw(12) << "lost"
              << std::setw(11) << "overlap" << std::setw(12) << "own" << std::setw(7) << "drills" << std::setw(7) << "ms" << "  status\n";
    size_t failed = 0;
    for (const Row& row : rows) {
        failed += !row.failures.empty();
        std::cout << std::left << std::setw(34) << row.id << std::right << std::fixed << std::setprecision(0) << std::setw(12) << row.volume_a << std::setw(12) << row.volume_b
                  << std::setprecision(1) << std::setw(12) << row.lost << std::setprecision(3) << std::setw(11) << row.overlap << std::setprecision(0) << std::setw(12) << row.own << std::setw(7) << row.drills
                  << std::setprecision(0) << std::setw(7) << row.ms << "  " << (row.failures.empty() ? "ok" : fmt::format("{} failures", row.failures.size())) << "\n";
    }
    for (const std::string& skipped : SKIPPED)
        std::cout << "skipped " << skipped << "\n";

    // the failures
    for (const Row& row : rows)
        for (const std::string& failure : row.failures)
            std::cerr << row.id << " | " << failure << "\n";
    std::cout << fmt::format("\njoint library: {} / {} variants pass, {} skipped\n", rows.size() - failed, rows.size(), SKIPPED.size());

    // the ratchet: the library is not complete yet, so a variant fails the run only when it passed when the list was accepted
    const std::set<std::string> accepted = accepted_passes();
    size_t regressed = 0;
    for (const Row& row : rows) {
        if (accepted.count(row.id) && !row.failures.empty()) {
            regressed++;
            std::cerr << fmt::format("REGRESSION {}: passed before, now {}\n", row.id, row.failures.front());
        }
        if (row.failures.empty() && !accepted.count(row.id))
            std::cout << fmt::format("new pass {}: add it with WOOD_UPDATE_PASSING=1\n", row.id);
    }
    if (std::getenv("WOOD_UPDATE_PASSING") != nullptr && argc == 1) {
        std::ofstream file(PASSING_FILE);
        for (const Row& row : rows)
            if (row.failures.empty())
                file << row.id << "\n";
        std::cout << "passing list rewritten: " << PASSING_FILE << "\n";
    }

    return regressed > 0 ? 1 : 0;
}
