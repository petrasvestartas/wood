#pragma once

#include "pch.h"

#include "wood_feature_construction.h"

using namespace session_cpp;

namespace wood_session {

/// Stitches the oriented joint cut outlines of one plate into its top and bottom outlines.
class MergeModifier {
private:
    /// Relocated plate corners at a line joint's two ends, top and bottom.
    struct RelocatedCorners {
        Point top_at_previous; // Top corner at the previous edge.
        Point top_at_next; // Top corner at the next edge.
        Point bottom_at_previous; // Bottom corner at the previous edge.
        Point bottom_at_next; // Bottom corner at the next edge.
        bool has_top_at_previous = false; // Whether the three planes met at the top previous corner.
        bool has_top_at_next = false; // Whether the three planes met at the top next corner.
        bool has_bottom_at_previous = false; // Whether the three planes met at the bottom previous corner.
        bool has_bottom_at_next = false; // Whether the three planes met at the bottom next corner.
    };

    static constexpr double EDGE_SCALE = 1000000.0; // Sort keys pack the edge id scaled by this.
    static constexpr double FRACTION_SCALE = 1000.0; // Sort keys pack the sub-edge fraction scaled by this.
    const Plate& plate; // The plate being merged.
    int plate_index = -1; // Position of the plate in the element list, for the log.
    std::ofstream log_file; // The diagnostic log file, open only when tracing.
    std::ofstream* log = nullptr; // The open log, or null when tracing is off.
    std::vector<Point> top_points; // Top outline vertices, relocated by the line joints.
    std::vector<Point> bottom_points; // Bottom outline vertices, relocated by the line joints.
    std::vector<Plane> joint_planes; // The plate planes; side planes are replaced by the joint planes as the joints are visited.
    Point top_original_front; // First top point before relocation: the closing duplicate is never relocated, so closure is tested against this.
    Point bottom_original_front; // First bottom point before relocation.
    double distance_squared = 0.0; // The global DISTANCE_SQUARED tolerance.
    std::multimap<size_t, std::pair<std::pair<double, double>, std::vector<Point>>> top_runs; // Joint point runs on the top outline, keyed by edge.
    std::multimap<size_t, std::pair<std::pair<double, double>, std::vector<Point>>> bottom_runs; // Joint point runs on the bottom outline, keyed by edge.
    int last_id = -1; // Face index of the last line joint visited.

    /// First joint line on the top outline: the closing corner moves to its intersection with the last.
    std::array<Point, 2> first_top_segment{{Point(0, 0, 0), Point(0, 0, 0)}};

    /// First joint line on the bottom outline.
    std::array<Point, 2> first_bottom_segment{{Point(0, 0, 0), Point(0, 0, 0)}};

    /// Last joint line on the top outline.
    std::array<Point, 2> last_top_segment{{Point(0, 0, 0), Point(0, 0, 0)}};

    /// Last joint line on the bottom outline.
    std::array<Point, 2> last_bottom_segment{{Point(0, 0, 0), Point(0, 0, 0)}};

    /// Copies the plate outlines and planes and opens the log; `distance_squared` is the offset tolerance.
    MergeModifier(const Plate& plate, int plate_index, double distance_squared);

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// [hole0_top, hole0_bot, ..., merged_top, merged_bot] for the plate; a joint's outlines are swapped top/bottom in place when they arrive reversed.
    static std::vector<Polyline> apply(
        const Plate& plate,
        const std::vector<std::vector<std::pair<int, bool>>>& membership,
        std::vector<InteractionFeaturePlate>& joints,
        int plate_index,
        double distance_squared
    );

private:
    /// Squared perpendicular distance from p to the infinite line through line_a and line_b.
    static double perpendicular_distance_squared(const Point& p, const Point& line_a, const Point& line_b);

    /// Writes every point of the polyline to the log.
    static void log_points(std::ofstream& log, const Polyline& polyline);

    /// Writes the plate header (planes and both outlines) to the log.
    void log_plate() const;

    /// Writes the merged top/bottom outlines to the log.
    void log_result(const Polyline& merged_top, const Polyline& merged_bottom) const;

    /// Selects the joint's male/female outlines, checks the endpoint markers and swaps top/bottom when reversed; null means skip.
    std::array<std::vector<Polyline>, 2>* joint_outlines(InteractionFeaturePlate& joint, size_t face, int joint_id, bool male_or_female) const;

    /// Clips the rectangle joint against both outlines and inserts the clipped runs.
    void insert_rectangle_cut(const std::array<std::vector<Polyline>, 2>& outlines);

    /// Intersects the joint plane with its neighbours and the top/bottom planes; a degenerate joint plane leaves the corners in place.
    RelocatedCorners corner_intersections(size_t face, int previous, int next, bool z_axis_valid) const;

    /// Snaps the previous-edge corners to the previous joint's plane when that joint line is offset from the plate edge.
    void relocate_previous_corners(size_t face, const Point& top_start, const Point& bottom_start, RelocatedCorners& corners) const;

    /// Updates the joint plane, relocates the plate vertices at both joint ends and tracks the segment; false means skip.
    bool relocate_edge_vertices(std::array<std::vector<Polyline>, 2>& outlines, size_t face);

    /// Reverses the joint outlines when they run against the plate walk, then inserts them keyed by (id + 0.1, id + 0.9).
    void flip_and_insert_cut(const InteractionFeaturePlate& joint, std::array<std::vector<Polyline>, 2>& outlines, size_t face, int joint_id, bool male_or_female);

    /// Runs the side-joint passes for every joint on faces 2..N: 2-point markers are line joints, 5-point markers rectangles.
    void insert_side_joints(const std::vector<std::vector<std::pair<int, bool>>>& membership, std::vector<InteractionFeaturePlate>& joints);

    /// Builds one merged outline from the relocated vertices and the sorted joint runs.
    static Polyline build_merged_outline(const std::vector<Point>& points, std::multimap<size_t, std::pair<std::pair<double, double>, std::vector<Point>>>& runs, const Point& original_front);

    /// Moves the closing corner to the first/last joint-line intersection when both edges carry a joint.
    void close_corner(Polyline& merged_top, Polyline& merged_bottom) const;

    /// Appends the hole outlines of the top/bottom face joints (faces 0, 1) to result: every outline but the last, the bounding rectangle.
    void cut_holes_top_bottom(const std::vector<std::vector<std::pair<int, bool>>>& membership, std::vector<InteractionFeaturePlate>& joints, std::vector<Polyline>& result) const;

    /// Appends the hole outlines of the side joints (faces 2..N) to result: the outlines tagged FabricationType::hole, shadow joints included.
    void cut_holes_side(const std::vector<std::vector<std::pair<int, bool>>>& membership, std::vector<InteractionFeaturePlate>& joints, std::vector<Polyline>& result) const;
};

} // namespace wood_session
