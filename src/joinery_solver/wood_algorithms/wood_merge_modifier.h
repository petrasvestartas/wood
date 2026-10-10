#pragma once

#include "pch.h"

#include "wood_feature_construction.h"

using namespace session_cpp;

namespace wood_session {

/// Stitches the oriented joint cut outlines of one plate into its bottom and top outlines: the edge insertions into the outer loops, the holes as inner loops. The outlines of the solid types (mill, slice, cut, conic, drill) are not merged; WoodSession cuts those as solid features.
class MergeModifier {
private:
    /// Relocated plate corners at a line joint's two ends, bottom and top.
    struct RelocatedCorners {
        Point bottom_at_previous; // Bottom corner at the previous edge.
        Point bottom_at_next; // Bottom corner at the next edge.
        Point top_at_previous; // Top corner at the previous edge.
        Point top_at_next; // Top corner at the next edge.
        bool has_bottom_at_previous = false; // Whether the three planes met at the bottom previous corner.
        bool has_bottom_at_next = false; // Whether the three planes met at the bottom next corner.
        bool has_top_at_previous = false; // Whether the three planes met at the top previous corner.
        bool has_top_at_next = false; // Whether the three planes met at the top next corner.
    };

    /// The 2D frame a face is clipped in.
    struct ClipFrame {
        Point origin; // The face outline's first point.
        Vector x_axis; // CGAL's base1 of the face plane.
        Vector y_axis; // The normal's cross with it.
    };

    static constexpr double EDGE_SCALE = 1000000.0; // Sort keys pack the edge id scaled by this.
    static constexpr double FRACTION_SCALE = 1000.0; // Sort keys pack the sub-edge fraction scaled by this.
    static constexpr int CLIP_DECIMALS = 2; // The decimals 2024 gave Clipper2's ClipperD when it clipped the open joint outline against the face outline, in the frame of the face's first point and CGAL's bases of its plane.
    static constexpr double CLIP_SCALE = 128.0; // ClipperD's scale at those decimals, the power of two above 10^2 (its radix rule, #25): what a coordinate is multiplied by before it is rounded to an integer.
    static constexpr double CLIP_GRID = 1.0 / CLIP_SCALE; // mm, the grid that clip left every slot on, 1/128, where the 2025 reference holds them.
    const Plate& plate; // The plate being merged.
    int plate_index = -1; // Position of the plate in the element list, for the log.
    std::ofstream log_file; // The diagnostic log file, open only when tracing.
    std::ofstream* log = nullptr; // The open log, or null when tracing is off.
    std::vector<Point> bottom_points; // Bottom outline vertices, relocated by the line joints.
    std::vector<Point> top_points; // Top outline vertices, relocated by the line joints.
    std::vector<Plane> joint_planes; // The plate planes; side planes are replaced by the joint planes as the joints are visited.
    Point bottom_original_front; // First bottom point before relocation: the closing duplicate is never relocated, so closure is tested against this.
    Point top_original_front; // First top point before relocation.
    double distance_squared = 0.0; // The global DISTANCE_SQUARED tolerance.
    std::multimap<size_t, std::pair<std::pair<double, double>, std::vector<Point>>> bottom_runs; // Joint point runs on the bottom outline, keyed by edge.
    std::multimap<size_t, std::pair<std::pair<double, double>, std::vector<Point>>> top_runs; // Joint point runs on the top outline, keyed by edge.
    int last_id = -1; // Face index of the last line joint visited.

    /// First joint line on the bottom outline: the closing corner moves to its intersection with the last.
    std::array<Point, 2> first_bottom_segment{{Point(0, 0, 0), Point(0, 0, 0)}};

    /// First joint line on the top outline.
    std::array<Point, 2> first_top_segment{{Point(0, 0, 0), Point(0, 0, 0)}};

    /// Last joint line on the bottom outline.
    std::array<Point, 2> last_bottom_segment{{Point(0, 0, 0), Point(0, 0, 0)}};

    /// Last joint line on the top outline.
    std::array<Point, 2> last_top_segment{{Point(0, 0, 0), Point(0, 0, 0)}};

    /// Copies the plate outlines and planes and opens the log; `distance_squared` is the offset tolerance.
    MergeModifier(const Plate& plate, int plate_index, double distance_squared);

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// [hole0_bottom, hole0_top, ..., merged_bottom, merged_top] for the plate, the bottom of every pair on the side of polylines[0] as Plate names its faces; a joint's outlines and fabrication types are swapped bottom/top in place when they arrive reversed.
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

    /// Writes the merged bottom/top outlines to the log.
    void log_result(const Polyline& merged_bottom, const Polyline& merged_top) const;

    /// Selects the joint's male/female outlines, checks the endpoint markers and swaps bottom/top, outlines and fabrication types together, when reversed; null means skip.
    std::array<std::vector<Polyline>, 2>* joint_outlines(
        InteractionFeaturePlate& joint,
        size_t face,
        int joint_id,
        bool male_or_female
    ) const;

    /// The frame 2024 clipped a face in: the face outline's first point, CGAL's base1 of the face plane and the normal's cross with it.
    static ClipFrame clip_frame(const Polyline& face, const Plane& plane);

    /// The outline on 2024's clip grid: each point's two coordinates in the frame rounded to CLIP_GRID, as Clipper2 rounds every vertex it takes at two decimals, and the point put back on the face, so the kernel's clip sees the outlines Clipper2 saw.
    static Polyline on_clip_grid(const Polyline& outline, const ClipFrame& frame);

    /// The run with each point moved onto the nearest point, within a grid step, of Clipper2's own clip of the joint outline against the face outline at two decimals, the clip 2024 ran: Clipper2 keeps a vertex on the grid, truncates the intersection of two slanted edges toward zero and rounds one on a horizontal edge to the nearest, and the 2025 reference holds every slot where it put them. A point no result point is near keeps its grid position.
    static Polyline on_clipper_points(const Polyline& run, const Polyline& face, const Polyline& joint, const ClipFrame& frame);

    /// Clips the rectangle joint against both outlines on the 2024 clip grid and inserts the clipped runs.
    void insert_rectangle_cut(const std::array<std::vector<Polyline>, 2>& outlines);

    /// Intersects the joint plane with its neighbours and the bottom/top planes; a degenerate joint plane leaves the corners in place.
    RelocatedCorners corner_intersections(
        size_t face,
        int previous,
        int next,
        bool z_axis_valid
    ) const;

    /// Snaps the previous-edge corners to the previous joint's plane when that joint line is offset from the plate edge.
    void relocate_previous_corners(
        size_t face,
        const Point& bottom_start,
        const Point& top_start,
        RelocatedCorners& corners
    ) const;

    /// Updates the joint plane, relocates the plate vertices at both joint ends and tracks the segment; false means skip.
    bool relocate_edge_vertices(std::array<std::vector<Polyline>, 2>& outlines, size_t face);

    /// Reverses the joint outlines when they run against the plate walk, then inserts them keyed by (id + 0.1, id + 0.9).
    void flip_and_insert_cut(
        const InteractionFeaturePlate& joint,
        std::array<std::vector<Polyline>, 2>& outlines,
        size_t face,
        int joint_id,
        bool male_or_female
    );

    /// True when the side's first outline is an edge insertion (edge_insertion, insert_between_multiple_edges, or untyped), what the outline passes stitch in, or a custom pair of no type whose second copy has two points or five, the line and the rectangle 2024 merged whatever the type; a hole, a mill, a slice, a cut, a conic or a drill is not.
    static bool merges_into_outline(const InteractionFeaturePlate& joint, bool male_or_female);

    /// The merged pair without what the stitching leaves meaningless, both loops kept in step vertex for vertex: a point repeated on a face, or a corner a run folds back over on a face, where the loop reverses along its own edge, as a male outline rising from the mitre corner does on a right-angle pair, goes from both loops when the other face loses no shape by it, its vertex there repeated, folded or on the edge between its neighbours; all within the merge's distance, the contact grid having moved the oriented outlines off the corners by microns. 2024 kept them all and its right-angle loops crossed themselves. The closing points stay in step.
    void drop_folded_corners(Polyline& merged_bottom, Polyline& merged_top) const;

    /// Runs the side-joint passes for every edge-insertion joint on faces 2..N: 2-point markers are line joints, 5-point markers rectangles.
    void insert_side_joints(const std::vector<std::vector<std::pair<int, bool>>>& membership, std::vector<InteractionFeaturePlate>& joints);

    /// Builds one merged outline from the relocated vertices and the sorted joint runs.
    static Polyline build_merged_outline(const std::vector<Point>& points, std::multimap<size_t, std::pair<std::pair<double, double>, std::vector<Point>>>& runs, const Point& original_front);

    /// Moves the closing corner to the first/last joint-line intersection when both edges carry a joint.
    void close_corner(Polyline& merged_bottom, Polyline& merged_top) const;

    /// Appends the hole outlines of the bottom/top face joints (faces 0, 1) to result: every outline tagged FabricationType::hole but the last, the bounding rectangle.
    void cut_holes_top_bottom(const std::vector<std::vector<std::pair<int, bool>>>& membership, std::vector<InteractionFeaturePlate>& joints, std::vector<Polyline>& result) const;

    /// Appends the hole outlines of the side joints (faces 2..N) to result: the outlines tagged FabricationType::hole, shadow joints included.
    void cut_holes_side(const std::vector<std::vector<std::pair<int, bool>>>& membership, std::vector<InteractionFeaturePlate>& joints, std::vector<Polyline>& result) const;
};

} // namespace wood_session
