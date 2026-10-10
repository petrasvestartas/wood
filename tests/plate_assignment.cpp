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

int main() {

    for (const std::string name : {"annen_box", "annen_box_pair", "annen_corner", "annen_grid_small", "annen_grid_full_arch", "inplane_differentdirections", "vidy_corner", "vidy_folding", "vidy_full", "vidy_one_axis_two_layers"})
        check_dataset(name);
    return 0;
}
