#pragma once

#include "wood_pch.h"

#include "wood_joint.h"

/// membership[element][face] = [(joint index, is_male)].
using JointMembership = std::vector<std::vector<std::vector<std::pair<int, bool>>>>;

namespace wood_session {

/// Stitches the oriented joint cut outlines of one plate into its top and bottom outlines.
class MergeModifier {
public:
    /// [hole0_top, hole0_bot, ..., merged_top, merged_bot] for the plate; a joint's outlines are swapped top/bottom in place when they arrive reversed.
    static std::vector<session_cpp::Polyline> apply(
        const Plate& plate,
        const std::vector<std::vector<std::pair<int, bool>>>& membership,
        std::vector<WoodJoint>& joints,
        int plate_index
    );

private:
    /// Joint point runs keyed by plate edge; a multimap, so joints sharing a key on one edge all survive.
    using SortedRuns = std::multimap<size_t, std::pair<std::pair<double, double>, std::vector<session_cpp::Point>>>;

    /// A joint's top and bottom outline lists.
    using JointOutlines = std::array<std::vector<session_cpp::Polyline>, 2>;

    /// membership[face] = [(joint index, is_male)] of one plate.
    using PlateMembership = std::vector<std::vector<std::pair<int, bool>>>;

    /// Relocated plate corners at a line joint's two ends, top and bottom.
    struct RelocatedCorners {
        session_cpp::Point top_at_previous; // Top corner at the previous edge.
        session_cpp::Point top_at_next; // Top corner at the next edge.
        session_cpp::Point bottom_at_previous; // Bottom corner at the previous edge.
        session_cpp::Point bottom_at_next; // Bottom corner at the next edge.
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
    std::vector<session_cpp::Point> top_points; // Top outline vertices, relocated by the line joints.
    std::vector<session_cpp::Point> bottom_points; // Bottom outline vertices, relocated by the line joints.
    std::vector<session_cpp::Plane> joint_planes; // The plate planes; side planes are replaced by the joint planes as the joints are visited.
    session_cpp::Point top_original_front; // First top point before relocation: the closing duplicate is never relocated, so closure is tested against this.
    session_cpp::Point bottom_original_front; // First bottom point before relocation.
    double distance_squared = 0.0; // The global DISTANCE_SQUARED tolerance.
    SortedRuns top_runs; // Joint point runs on the top outline, keyed by edge.
    SortedRuns bottom_runs; // Joint point runs on the bottom outline, keyed by edge.
    int last_id = -1; // Face index of the last line joint visited.

    /// First joint line on the top outline: the closing corner moves to its intersection with the last.
    std::array<session_cpp::Point, 2> first_top_segment{{session_cpp::Point(0, 0, 0), session_cpp::Point(0, 0, 0)}};

    /// First joint line on the bottom outline.
    std::array<session_cpp::Point, 2> first_bottom_segment{{session_cpp::Point(0, 0, 0), session_cpp::Point(0, 0, 0)}};

    /// Last joint line on the top outline.
    std::array<session_cpp::Point, 2> last_top_segment{{session_cpp::Point(0, 0, 0), session_cpp::Point(0, 0, 0)}};

    /// Last joint line on the bottom outline.
    std::array<session_cpp::Point, 2> last_bottom_segment{{session_cpp::Point(0, 0, 0), session_cpp::Point(0, 0, 0)}};

    /// Copies the plate outlines and planes and opens the log.
    MergeModifier(const Plate& plate, int plate_index);

    /// Squared perpendicular distance from p to the infinite line through line_a and line_b.
    static double perpendicular_distance_squared(const session_cpp::Point& p, const session_cpp::Point& line_a, const session_cpp::Point& line_b);

    /// Writes every point of the polyline to the log.
    static void log_points(std::ofstream& log, const session_cpp::Polyline& polyline);

    /// Writes the plate header (planes and both outlines) to the log.
    void log_plate() const;

    /// Writes the merged top/bottom outlines to the log.
    void log_result(const session_cpp::Polyline& merged_top, const session_cpp::Polyline& merged_bottom) const;

    /// Selects the joint's male/female outlines, checks the endpoint markers and swaps top/bottom when reversed; null means skip.
    JointOutlines* joint_outlines(WoodJoint& joint, size_t face, int joint_id, bool male_or_female) const;

    /// Clips the rectangle joint against both outlines and inserts the clipped runs.
    void insert_rectangle_cut(const JointOutlines& outlines);

    /// Intersects the joint plane with its neighbours and the top/bottom planes; a degenerate joint plane leaves the corners in place.
    RelocatedCorners corner_intersections(size_t face, int previous, int next, bool z_axis_valid) const;

    /// Snaps the previous-edge corners to the previous joint's plane when that joint line is offset from the plate edge.
    void relocate_previous_corners(size_t face, const session_cpp::Point& top_start, const session_cpp::Point& bottom_start, RelocatedCorners& corners) const;

    /// Updates the joint plane, relocates the plate vertices at both joint ends and tracks the segment; false means skip.
    bool relocate_edge_vertices(JointOutlines& outlines, size_t face);

    /// Reverses the joint outlines when they run against the plate walk, then inserts them keyed by (id + 0.1, id + 0.9).
    void flip_and_insert_cut(const WoodJoint& joint, JointOutlines& outlines, size_t face, int joint_id, bool male_or_female);

    /// Runs the side-joint passes for every joint on faces 2..N: 2-point markers are line joints, 5-point markers rectangles.
    void insert_side_joints(const PlateMembership& membership, std::vector<WoodJoint>& joints);

    /// Builds one merged outline from the relocated vertices and the sorted joint runs.
    static session_cpp::Polyline build_merged_outline(const std::vector<session_cpp::Point>& points, SortedRuns& runs, const session_cpp::Point& original_front);

    /// Moves the closing corner to the first/last joint-line intersection when both edges carry a joint.
    void close_corner(session_cpp::Polyline& merged_top, session_cpp::Polyline& merged_bottom) const;

    /// Appends the hole outlines of the top/bottom face joints (faces 0, 1) to result: every outline but the last, the bounding rectangle.
    void cut_holes_top_bottom(const PlateMembership& membership, std::vector<WoodJoint>& joints, std::vector<session_cpp::Polyline>& result) const;

    /// Appends the hole outlines of the side joints (faces 2..N) to result: the outlines tagged wood_cut::hole, shadow joints included.
    void cut_holes_side(const PlateMembership& membership, std::vector<WoodJoint>& joints, std::vector<session_cpp::Polyline>& result) const;
};

} // namespace wood_session
