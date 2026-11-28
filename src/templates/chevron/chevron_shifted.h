#pragma once
#include "src/templates/chevron/chevron.h"
#include "wood_element_geometry.h"
#include "wood_solid_body.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_chevron {

/// A side of a cell's outline: its plane and the cell edge it stands on, -1 for a neighbour's cross plane.
struct ShiftedSide {
    Plane plane;
    int edge;
};

/// A wall of a cell: the first and last side of its outline it stands on, and whether it stands outward of them.
struct ShiftedWallRun {
    int first;
    int last;
    bool outward;
};

/// The cells of the shifted pattern: hexagons over three rows in staggered columns, a half-cell quad at one end of every column.
struct ShiftedCells {
    ChevronFaces faces; // Per cell its vertex keys into the point grid and the cells on every edge.
    std::vector<int>
        masks; // Per cell its kind: 20 and 2 even-column hexagons, -2 odd-column hexagons, 3 and -3 last quads, -1 first quads.
    std::vector<int> columns; // Per cell its column.
};

/// The plates of the shifted pattern, per cell its top and bottom plate and then its walls.
struct ShiftedPlates {
    std::vector<std::shared_ptr<FacePlanarElement>> plates; // In the order they are added, plate_<i>.
    std::vector<std::vector<int>>
        edges; // Per plate the cell edge under every side of its first outline, -1 for none.
    std::vector<std::array<int, 2>> boxes; // Per cell its top and bottom plate.
    std::vector<std::array<int, 2>> walls; // Per wall its plate and its cell.
    std::set<int> outward; // The walls standing outward of their cell, on the sides it gives to the neighbouring column.
    std::vector<std::vector<std::pair<size_t, size_t>>>
        wall_edges; // Per wall the mesh edges it stands on, both halves of a merged long side.
    std::vector<std::array<int, 3>>
        corners; // Per box corner the two walls mitred there, the one ending on the mitre then the one starting on it, and their cell.
};

}

// ═══════════════════════════════════════════════════════════════════════════
// ChevronShifted
// ═══════════════════════════════════════════════════════════════════════════

/// The shifted chevron shell: hexagonal cells in staggered columns on a surface, a box of plates per cell, the 2018 Annen pattern.
class ChevronShifted : public WoodSession {
public:
    static constexpr double OPEN_REACH = 160.0; // mm of every box's way in its slots are opened over.
    static constexpr double OPEN_STEP = 2.0; // mm between two looks at what it shares there.
    const ChevronShiftedGuide guide;

    /// The shifted chevron shell of the guide as the session named name.
    explicit ChevronShifted(const ChevronShiftedGuide& guide, const std::string& name = "chevron_shifted");

    /// Not copied, as a session is not.
    ChevronShifted(const ChevronShifted&) = delete;

    /// Not assigned, as it is not copied.
    ChevronShifted& operator=(const ChevronShifted&) = delete;

    /// The cell mesh, a hexagon or quad face per cell.
    const Mesh& mesh() const;

    /// The cells: vertex keys, kinds and columns.
    const wood_chevron::ShiftedCells& cells() const;

    /// Per plate one insertion vector per face: the direction of its edge on the top and bottom plates' tenon sides, zero elsewhere.
    const std::vector<std::vector<Vector>>& insertion_vectors() const;

    /// Per plate one axis per face: the mean surface normal at the insertion corner of the cell that owns its edge on the top and bottom plates' tenon sides, the definition's cut plane axis, zero elsewhere.
    const std::vector<std::vector<Vector>>& feature_axes() const;

    /// Per plate one joint type per face: 23, the definition's tenon, on the top and bottom plates' sides its outer loop moved out by a wall; 17 and 10, the Annen fingers, on the two mitre faces of every box corner; 0 elsewhere.
    const std::vector<std::vector<int>>& joints_per_face() const;

    /// Per plate one division count per face: divisions on the top and bottom plates' sides, finger_divisions on the walls' mitre faces, 0 elsewhere.
    const std::vector<std::vector<int>>& divisions_per_face() const;

    /// Rows {t0, w, t1, w}: the top (or bottom) plates t0 and t1 of the two cells on a wall w both join it, so their tenons are aligned.
    const std::vector<std::array<int, 4>>& three_valence() const;

    /// The plate pairs that share a joint: every wall with the top and bottom plates of the cells on its edges, once per edge, and the two walls of every box corner.
    const std::vector<std::pair<int, int>>& adjacency() const;

private:
    NurbsSurface _across;
    std::vector<std::vector<Point>> _points;
    wood_chevron::ShiftedCells _cells;
    Mesh _mesh;
    std::map<std::pair<size_t, size_t>, Plane> _edge_planes;
    std::vector<Plane> _face_planes;
    std::vector<std::vector<wood_chevron::ShiftedSide>> _sides;
    std::vector<std::vector<Vector>> _insertion_vectors;
    std::vector<std::vector<Vector>> _feature_axes;
    std::vector<std::vector<int>> _joints_per_face;
    std::vector<std::vector<int>> _divisions_per_face;
    std::vector<std::array<int, 4>> _three_valence;
    std::vector<std::pair<int, int>> _adjacency;

    /// The point grid: rows at equal arc length along the middle iso curve, the ends and then half a step in, each divided at equal arc length across, the interior points shifted.
    std::vector<std::vector<Point>> compute_points() const;

    /// The cells column by column, the odd columns from the far end, and the cells on every edge.
    wood_chevron::ShiftedCells compute_cells() const;

    /// The cells as one mesh.
    Mesh compute_mesh() const;

    /// A plane through every edge along the surface normal; on a naked edge along the curvature of the nearest long side curve.
    std::map<std::pair<size_t, size_t>, Plane> compute_edge_planes() const;

    /// The edge planes with both halves of every even cell's long sides merged into one plane through their middle, turned by edge_rotation; not turned on a naked side.
    std::map<std::pair<size_t, size_t>, Plane>
    compute_merged_edge_planes(std::map<std::pair<size_t, size_t>, Plane> planes) const;

    /// A plane at the surface point nearest every cell's centre, x along the rows, z into the surface.
    std::vector<Plane> compute_face_planes() const;

    /// Per cell its edge planes in order, with the neighbour's cross plane inserted after edges 1 and 4 of a hexagon; an odd hexagon's steps, where it moves from one even neighbour's long wall to the next, are cut along the insertion vector of whatever passes them on its way in, the lower even cell's wall after edge 1, the odd cell itself after edge 4, where the steps would otherwise stand in the way.
    std::vector<std::vector<wood_chevron::ShiftedSide>> compute_sides() const;

    /// Per cell a top and a bottom plate, stepped in by a wall where its own walls stand, then its walls between the outer box faces: the halves of a merged long side one wall, every wall mitred against the next wall of its box and stopped short of a side where a neighbour's wall stands inside the cell.
    wood_chevron::ShiftedPlates compute_plates() const;

    /// The insertion vectors, feature axes, joint types, divisions, adjacency and three-valence groups of the plates, and an insertion line per cell.
    std::vector<Line> compute_joinery(const wood_chevron::ShiftedPlates& plates);

    /// Opens the walls where the boxes, going in cell by cell along their own vectors, share wood with the ones before: the tenons lean out of their plate's plane by the turn between two cells.
    void open_slots(const wood_chevron::ShiftedPlates& plates, const std::vector<Line>& insertion_lines);

    /// The grid point of a vertex key.
    const Point& point(size_t key) const;

    /// The vertex key of row i and column j in the point grid.
    size_t vertex_key(int i, int j) const;

    /// Appends a cell with vertex keys, kind mask and column to cells.
    static void add_cell(
        wood_chevron::ShiftedCells& cells,
        std::vector<size_t> keys,
        int mask,
        int column
    );

    /// Appends a hexagon cell starting at row i and column j with kind mask to cells.
    void add_hexagon(
        wood_chevron::ShiftedCells& cells,
        int i,
        int j,
        int mask
    ) const;

    /// Appends a quad cell starting at row i and column j with kind mask to cells.
    void add_quad(
        wood_chevron::ShiftedCells& cells,
        int i,
        int j,
        int mask
    ) const;

    /// The sorted vertex pair of a cell's edge.
    std::pair<size_t, size_t> edge_key(int cell, int edge) const;

    /// Per side of a cell's outline whether the top and bottom plates step in by a wall (-1) or stay (0); a first quad steps in on its left side too, under the even neighbour's wall, which the definition left unstepped.
    static std::vector<int> step_flags(int mask);

    /// The walls of a cell as (side, outward) pairs in the order they are made.
    static std::vector<std::pair<int, bool>> wall_sides(int mask);

    /// The two cell edges whose outline segments give the insertion direction.
    static std::array<int, 2> insertion_edges(int mask);

    /// The insertion direction of a cell with the side loop sides and the face plane face: the bisector of its two insertion edges' outline segments.
    static Vector
    direction_of(const std::vector<wood_chevron::ShiftedSide>& sides, const Plane& face, int mask);

    /// The cell edges whose walls the cell slides into: its direction goes to them.
    static std::vector<int> owned_edges(int mask);

    /// Whether the definition's outer loop moved the plane of a cell edge out by a wall: every edge of a hexagon and of a last quad, the long side and the top of a first quad.
    static bool tenon_side(int mask, int edge);

    /// The plane turned about its y axis.
    static Plane turned_about_y(const Plane& plane, double angle);

    /// The mitre between two walls' outer faces a and b: through their seam, its normal halfway between theirs turned away from centre; none when they are parallel.
    static std::optional<Plane> mitre(const Plane& a, const Plane& b, const Point& centre);

    /// The plane moved by distance along its normal to the side nearer centre (inward) or farther from it.
    static Plane stepped(const Plane& plane, double distance, const Point& centre, bool inward);

    /// Both closed outlines without their coincident consecutive corners; returns the edge under every remaining side.
    static std::vector<int>
    compact(Polyline& first, Polyline& second, const std::vector<int>& edges);
};
