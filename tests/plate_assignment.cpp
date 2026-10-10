#include "wood_session.h"
#include "oracle_polyline.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Joint types and insertion vectors by points and lines, against the sidecars
// ═══════════════════════════════════════════════════════════════════════════

static const double SNAP = 1.0; // mm, how far a point may lie from the edge it names
static const double MATCH = 1e-6; // mm, an outline of the points' run against the sidecars' run: a line gives its vector back to rounding

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// The dataset solved twice: once with its *_joints_types.txt and *_insertion_vectors.txt sidecars, once with the tables cleared and
/// refilled by assign_joint_types_by_points and assign_insertion_vectors_by_lines, a point or a line at the centre of every side face the
/// sidecars name, one point where two plates' faces meet (a point on the outline for a bottom or top face, its type negative; a 0 there, which no negative type writes, is left out); both runs give the same joints and the same outlines.
static void check_dataset(const std::string& name) {

    config::reset_defaults();
    WoodSession sidecars = WoodSession::yaml_load(name);
    const std::vector<InteractionFeaturePlate> expected = sidecars.compute_features();

    config::reset_defaults();
    WoodSession scene = WoodSession::yaml_load(name);
    std::vector<Point> points;
    std::vector<int> types;
    std::vector<Line> lines;
    for (const std::shared_ptr<Plate>& plate : scene.plates()) {
        const std::vector<int> given_types = plate->feature_types;
        const std::vector<Vector> given_vectors = plate->insertion_vectors();
        plate->feature_types.clear();
        plate->insertion_vectors().clear();

        // the face each given index names on the plate's outlines, the mapping its own inverse
        for (size_t given = 0; given < std::max(given_types.size(), given_vectors.size()); given++) {
            const int face = plate->given_face(static_cast<int>(given));
            // a point on the outline for a bottom or top face, at the centre of a side face
            const Point at = face < 2 ? plate->polylines[face][0]
                                      : Point::mid_point(Point::mid_point(plate->polylines[0][face - 2], plate->polylines[0][face - 1]), Point::mid_point(plate->polylines[1][face - 2], plate->polylines[1][face - 1]));
            if (given < given_types.size() && given_types[given] != -1 && !(face < 2 && given_types[given] == 0)) {
                const int type = face < 2 ? -std::abs(given_types[given]) : std::abs(given_types[given]);
                // one point per joint, as a user places them: two faces on one edge share it, and it carries the larger type, the joint's
                size_t same = 0;
                while (same < points.size() && (points[same] - at).magnitude() > SNAP)
                    same++;
                if (same == points.size()) {
                    points.push_back(at);
                    types.push_back(type);
                } else if (std::abs(type) > std::abs(types[same])) {
                    types[same] = type;
                }
            }
            if (given < given_vectors.size() && given_vectors[given].magnitude() > 0.0) {
                check(face >= 2, fmt::format("{} gives an insertion vector on its face {}, not a side", plate->name, face));
                lines.push_back(Line::from_points(at, at + given_vectors[given]));
            }
        }
    }
    scene.assign_joint_types_by_points(points, types, SNAP);
    scene.assign_insertion_vectors_by_lines(lines, SNAP);
    const std::vector<InteractionFeaturePlate> joints = scene.compute_features();

    check(joints.size() == expected.size(), fmt::format("{}: {} joints by points and lines, {} by the sidecars", name, joints.size(), expected.size()));
    for (size_t i = 0; i < joints.size(); i++)
        check(joints[i].name == expected[i].name, fmt::format("{}: joint {} is {} by points, {} by the sidecars", name, i, joints[i].name, expected[i].name));

    const std::vector<std::shared_ptr<Plate>> plates = scene.plates();
    const std::vector<std::shared_ptr<Plate>> twins = sidecars.plates();
    for (size_t i = 0; i < plates.size(); i++) {
        const std::vector<Polyline>& outlines = plates[i]->features.top;
        const std::vector<Polyline>& reference = twins[i]->features.top;
        check(outlines.size() == reference.size(), fmt::format("{}: plate {} has {} outlines by points, {} by the sidecars", name, i, outlines.size(), reference.size()));
        for (size_t k = 0; k < outlines.size(); k++) {
            const double distance = oracle::point_set_distance(outlines[k], reference[k]);
            check(distance <= MATCH, fmt::format("{}: plate {} outline {} lies {:.3g} mm from the sidecars' run", name, i, k, distance));
        }
    }

    std::cout << fmt::format("plate_assignment: {} by {} points and {} lines gives the sidecars' {} joints", name, points.size(), lines.size(), joints.size()) << std::endl;
}

/// annen_box_pair joined twice from the same points, once by the ids the sidecar holds and once by the designs' names as the user interface
/// writes them: both give the same joints and outlines, the named points taking the side face each lies on.
static void check_names() {

    const std::vector<Point> points = {
        {-632.383, -853.908, -197.922}, {-79.806, -853.908, 185.917}, {-99.086, 475.200, 78.237}, {194.781, -408.746, -622.197},
        {202.789, 221.365, 532.068}, {256.424, 48.863, -452.615}, {324.694, 83.236, -484.489}, {48.696, -873.908, -326.382},
        {578.235, -185.452, -2.611}, {656.267, -151.400, -42.119}, {674.422, -526.549, -238.183}, {77.030, -39.981, 52.933},
        {842.265, -82.062, -560.511},
    };
    const std::vector<int> types = {20, 20, 20, 20, 20, 20, 20, 10, 20, 20, 20, 10, 0};
    std::vector<std::string> names;
    for (const int type : types)
        names.push_back(type == 20 ? "ts_e_p_3" : type == 10 ? "ss_e_op_1" : "");

    std::array<std::vector<InteractionFeaturePlate>, 2> runs;
    std::array<std::vector<std::vector<Polyline>>, 2> outlines;
    for (size_t run = 0; run < 2; run++) {
        config::reset_defaults();
        WoodSession scene = WoodSession::obj_load("annen_box_pair");
        scene.settings = config::load_yaml("annen_box_pair");
        if (run == 0)
            scene.assign_joint_types_by_points(points, types, SNAP);
        else
            scene.assign_joint_types_by_points(points, names, SNAP);
        runs[run] = scene.compute_features();
        for (const std::shared_ptr<Plate>& plate : scene.plates())
            outlines[run].push_back(plate->features.top);
    }

    check(runs[0].size() == runs[1].size() && !runs[0].empty(), fmt::format("annen_box_pair: {} joints by names, {} by ids", runs[1].size(), runs[0].size()));
    for (size_t i = 0; i < runs[0].size(); i++)
        check(runs[0][i].name == runs[1][i].name, fmt::format("annen_box_pair: joint {} is {} by names, {} by ids", i, runs[1][i].name, runs[0][i].name));
    for (size_t i = 0; i < outlines[0].size(); i++)
        for (size_t k = 0; k < std::min(outlines[0][i].size(), outlines[1][i].size()); k++)
            check(oracle::point_set_distance(outlines[0][i][k], outlines[1][i][k]) <= MATCH, fmt::format("annen_box_pair: plate {} outline {} differs by names", i, k));

    bool refused = false;
    try {
        JointPlate::library_id("ts_e_p_9");
    } catch (const std::invalid_argument&) {
        refused = true;
    }
    check(refused, "an unknown design name is not refused");

    std::cout << fmt::format("plate_assignment: annen_box_pair by {} named points gives the {} joints of their ids", points.size(), runs[1].size()) << std::endl;
}

int main() {

    check_names();

    for (const std::string name : {"annen_box", "annen_box_pair", "annen_corner", "annen_grid_small", "annen_grid_full_arch", "inplane_differentdirections", "vidy_corner", "vidy_folding", "vidy_full", "vidy_one_axis_two_layers"})
        check_dataset(name);
    return 0;
}
