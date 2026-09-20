#include "pch.h"
#include "wood_assignment.h"
using namespace session_cpp;

namespace wood_session {

static int nearest_slot(const Plate& plate, const Point& point, double threshold, bool faces) {

    if (plate.polylines.size() < 2)
        return -1;

    size_t segment_top = 0;
    size_t segment_bottom = 0;
    Point closest;
    const double distance_top = plate.polylines[1].closest_distance_and_point(point, segment_top, closest);
    const double distance_bottom = plate.polylines[0].closest_distance_and_point(point, segment_bottom, closest);
    const bool top = distance_top * distance_top <= distance_bottom * distance_bottom;
    const double nearest = top ? distance_top : distance_bottom;
    if (nearest * nearest >= threshold)
        return -1;

    if (faces)
        return top ? 1 : 0;

    return static_cast<int>(2 + (top ? segment_top : segment_bottom));
}

static SpatialRTree<int, double, 3> plate_rtree(const std::vector<std::shared_ptr<Plate>>& plates, double radius) {

    SpatialRTree<int, double, 3> rtree;
    for (size_t i = 0; i < plates.size(); i++) {

        if (plates[i]->polylines.empty())
            continue;

        const AABB box = plates[i]->aabb(radius);
        const Point lo = box.min_point();
        const Point hi = box.max_point();
        const double low[3] = {lo[0], lo[1], lo[2]};
        const double high[3] = {hi[0], hi[1], hi[2]};
        rtree.insert(low, high, static_cast<int>(i));
    }

    return rtree;
}

/// A slot per face: bottom, top, then one per side of the top outline.
static size_t slot_count(const Plate& plate) {
    const size_t n = plate.polylines.size() > 1 ? plate.polylines[1].point_count() : 0;
    return 2 + (n > 0 ? n - 1 : 0);
}

void assign_feature_types(const std::vector<std::shared_ptr<Plate>>& plates, const Settings& settings, const std::vector<Point>& points, const std::vector<int>& types) {

    const double threshold = settings.distance_squared * 100.0;
    const double radius = std::max(settings.distance, std::sqrt(threshold));
    for (const std::shared_ptr<Plate>& plate : plates)
        plate->feature_types.assign(slot_count(*plate), -1);

    if (points.empty() || types.size() < points.size())
        return;

    const SpatialRTree<int, double, 3> rtree = plate_rtree(plates, radius);
    for (size_t i = 0; i < points.size(); i++) {

        const Point& point = points[i];
        const int type = types[i];
        const double low[3] = {point[0] - radius, point[1] - radius, point[2] - radius};
        const double high[3] = {point[0] + radius, point[1] + radius, point[2] + radius};
        rtree.search(low, high, [&](const int index) {
            const int slot = nearest_slot(*plates[index], point, threshold, type < 0);
            if (slot >= 0 && slot < static_cast<int>(plates[index]->feature_types.size()))
                plates[index]->feature_types[slot] = std::abs(type);
            return true;
        });
    }
}

void assign_insertion_vectors(const std::vector<std::shared_ptr<Plate>>& plates, const Settings& settings, const std::vector<Line>& lines) {

    const double threshold = settings.distance_squared * 100.0;
    const double radius = std::max(settings.distance, std::sqrt(threshold));
    for (const std::shared_ptr<Plate>& plate : plates)
        plate->insertion_vectors().assign(slot_count(*plate), Vector(0.0, 0.0, 0.0));

    if (lines.empty())
        return;

    const SpatialRTree<int, double, 3> rtree = plate_rtree(plates, radius);
    for (const Line& line : lines) {

        const Point point = line.start();
        const Vector direction = line.to_vector();
        const double low[3] = {point[0] - radius, point[1] - radius, point[2] - radius};
        const double high[3] = {point[0] + radius, point[1] + radius, point[2] + radius};
        rtree.search(low, high, [&](const int index) {
            const int slot = nearest_slot(*plates[index], point, threshold, false);
            if (slot >= 0 && slot < static_cast<int>(plates[index]->insertion_vectors().size()))
                plates[index]->insertion_vectors()[slot] = direction;
            return true;
        });
    }
}

} // namespace wood_session
