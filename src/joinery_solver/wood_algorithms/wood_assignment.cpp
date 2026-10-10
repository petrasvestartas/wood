#include "pch.h"
#include "wood_session.h"
using namespace session_cpp;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Faces by points and lines
// ═══════════════════════════════════════════════════════════════════════════

/// A slot per face: bottom, top, then one per side of the top outline.
static size_t slot_count(const Plate& plate) {

    const size_t n = plate.polylines.size() > 1 ? plate.polylines[1].point_count() : 0;
    return 2 + (n > 0 ? n - 1 : 0);
}

/// The face of a plate a point snaps to, -1 when none lies within snap_radius, as the plate's given tables index it: with faces the bottom or
/// top face whose outline is nearer, else the side face whose middle line, between its bottom and top edges, is nearest, so a point on a
/// side face snaps to that face and not to the plate stacked on the same edge.
static int snapped_face(const Plate& plate, const Point& point, double snap_radius, bool faces) {

    if (plate.polylines.size() < 2)
        return -1;

    if (faces) {
        size_t edge = 0;
        Point closest;
        const double distance_top = plate.polylines[1].closest_distance_and_point(point, edge, closest);
        const double distance_bottom = plate.polylines[0].closest_distance_and_point(point, edge, closest);
        if (std::min(distance_top, distance_bottom) > snap_radius)
            return -1;
        return plate.given_face(distance_top <= distance_bottom ? 1 : 0);
    }

    int nearest = -1;
    double nearest_distance = snap_radius;
    for (size_t i = 0; i + 1 < plate.polylines[1].point_count(); i++) {
        Line middle;
        Line::get_middle_line(Line::from_points(plate.polylines[0][i], plate.polylines[0][i + 1]), Line::from_points(plate.polylines[1][i], plate.polylines[1][i + 1]), middle);
        const double distance = (middle.closest_point(point).second - point).magnitude();
        if (distance <= nearest_distance) {
            nearest = static_cast<int>(2 + i);
            nearest_distance = distance;
        }
    }

    return nearest < 0 ? -1 : plate.given_face(nearest);
}

void WoodSession::assign_joint_types_by_points(const std::vector<Point>& points, const std::vector<int>& types, double snap_radius) {

    if (types.size() < points.size())
        throw std::invalid_argument(fmt::format("assign_joint_types_by_points: {} types for {} points", types.size(), points.size()));

    if (points.empty())
        return;

    for (const std::shared_ptr<Plate>& plate : plates()) {

        // every plate a table, as a sidecar gives every plate a row, -1 on the faces it did not have
        std::vector<bool> reached(slot_count(*plate), false);
        if (plate->feature_types.size() != reached.size())
            plate->feature_types.resize(reached.size(), -1);

        // a face several points reach keeps the largest of their types, as a joint takes the larger type of its two faces
        for (size_t i = 0; i < points.size(); i++) {
            const int face = snapped_face(*plate, points[i], snap_radius, types[i] < 0);
            if (face < 0)
                continue;
            if (reached[face] && plate->feature_types[face] >= std::abs(types[i]))
                continue;
            plate->feature_types[face] = std::abs(types[i]);
            reached[face] = true;
        }
    }
}

void WoodSession::assign_joint_types_by_points(const std::vector<Point>& points, const std::vector<std::string>& names, double snap_radius) {

    if (names.size() < points.size())
        throw std::invalid_argument(fmt::format("assign_joint_types_by_points: {} names for {} points", names.size(), points.size()));

    std::vector<int> types;
    types.reserve(points.size());
    for (size_t i = 0; i < points.size(); i++) {

        // the nearest bottom or top outline of any plate against the nearest side face's middle line
        double outline = std::numeric_limits<double>::max();
        double side = std::numeric_limits<double>::max();
        for (const std::shared_ptr<Plate>& plate : plates()) {
            if (plate->polylines.size() < 2)
                continue;
            size_t edge = 0;
            Point closest;
            outline = std::min({outline, plate->polylines[0].closest_distance_and_point(points[i], edge, closest), plate->polylines[1].closest_distance_and_point(points[i], edge, closest)});
            for (size_t k = 0; k + 1 < plate->polylines[1].point_count(); k++) {
                Line middle;
                Line::get_middle_line(Line::from_points(plate->polylines[0][k], plate->polylines[0][k + 1]), Line::from_points(plate->polylines[1][k], plate->polylines[1][k + 1]), middle);
                side = std::min(side, (middle.closest_point(points[i]).second - points[i]).magnitude());
            }
        }

        // a type negative for the bottom or top face, as a sidecar's point on an outline
        const int id = JointPlate::library_id(names[i]);
        types.push_back(outline < side ? -id : id);
    }

    assign_joint_types_by_points(points, types, snap_radius);
}

void WoodSession::assign_insertion_vectors_by_lines(const std::vector<Line>& lines, double snap_radius) {

    if (lines.empty())
        return;

    for (const std::shared_ptr<Plate>& plate : plates()) {

        // every plate a table, as a sidecar gives every plate a row, zero on the faces it did not have
        std::vector<Vector>& vectors = plate->insertion_vectors();
        if (vectors.size() != slot_count(*plate))
            vectors.resize(slot_count(*plate), Vector(0.0, 0.0, 0.0));

        for (const Line& line : lines) {
            const int face = snapped_face(*plate, line.start(), snap_radius, false);
            if (face >= 0)
                vectors[face] = line.to_vector();
        }
    }
}

} // namespace wood_session
