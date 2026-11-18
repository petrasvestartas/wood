#pragma once

#include "wood_session.h"
#include "wood_profile.h"

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Framing
// ═══════════════════════════════════════════════════════════════════════════

/// Structural framing family on a planar grid.
enum class System {
    Slab,         ///< Point-supported CLT slab directly on columns or chamfered heads.
    PostAndBeam,  ///< Beams along grid axes (`span` selects primary family or `-1` for all axes).
    GirderPurlin, ///< Primary girders on `span` axes with subdivided secondary purlins/joists.
};

/// Column-to-frame connection detail at standing grid nodes.
enum class NodeMode {
    Flush,   ///< Columns stop at the framing soffit; beams meet cleanly above the column.
    Through, ///< Columns run continuously to the plane datum; beams and decks butt to column faces.
    Head,    ///< Chamfered timber head block: in-line with the beams (`[-beam_depth, 0]`), or beneath a flat `Slab`.
};

/// Vertical relation between secondary purlins and primary girders in `System::GirderPurlin`.
enum class PurlinMode {
    Flush,   ///< Purlins frame flush with the girder top (`w = 0`), trimmed to girder side faces.
    Stacked, ///< Purlins sit continuously on top of lowered primary girders.
};

/// In-plane timber bracing pattern inside selected quadrilateral bays.
enum class BraceMode {
    None,     ///< Unbraced open bay.
    Diagonal, ///< Single diagonal timber brace per braced bay.
    Cross,    ///< Co-planar X-brace: continuous primary diagonal with split secondary diagonal.
    Chevron,  ///< Inverted-V chevron braces meeting at the midpoint of the top member.
};

/// Timber cross-sections (rectangular glulam/CLT billets or round turned columns) by member role.
struct Profiles {
    std::vector<session_cpp::Polyline> column = wood_session::profile_rectangle(260.0, 260.0);
    std::vector<session_cpp::Polyline> girder = wood_session::profile_rectangle(260.0, 520.0);
    std::vector<session_cpp::Polyline> beam; ///< Optional secondary/perimeter beam in `PostAndBeam`; empty reuses `girder`.
    std::vector<session_cpp::Polyline> purlin = wood_session::profile_rectangle(160.0, 340.0);
    std::vector<session_cpp::Polyline> brace = wood_session::profile_rectangle(160.0, 160.0);
};

/// Timber framing specification for a single planar `Plan` or multi-storey `Stack`.
struct Framing {
    System system = System::PostAndBeam;
    NodeMode node = NodeMode::Flush;
    PurlinMode purlin = PurlinMode::Flush;
    BraceMode brace = BraceMode::None;
    std::vector<size_t> braced_bays; ///< Bay indices receiving braces (`empty` braces all quad bays when `brace != None`).
    int span = 0;                    ///< Primary axis family (`0` or `1`), or `-1` to make all grid axes primary.
    double post_height = 3600.0;     ///< Post/column length along `-frame.z_axis()` (`0.0` omits normal posts, e.g. facades).
    double spacing = 2000.0;         ///< Target centre-to-centre purlin spacing in `System::GirderPurlin`.
    double deck = 0.0;               ///< CLT deck or infill panel thickness (`0.0` omits panels).
    bool deck_flush = false;         ///< `false`: deck sits on `[0, +deck]`; `true`: flush infill in `[-deck, 0]`.
    double panel_width = 0.0;        ///< Maximum CLT transport strip width (`0.0` keeps one plate per bay).
    double wall = 0.0;               ///< CLT shear wall thickness around `Plan::cores` (`0.0` omits core walls).
    double head_height = 240.0;      ///< Height of the single chamfered timber head block (`NodeMode::Head`).
    double head_reach = 180.0;       ///< Outward corbel projection of the chamfered head beyond interior column faces.
    Profiles profiles;
};

// ═══════════════════════════════════════════════════════════════════════════
// Pattern
// ═══════════════════════════════════════════════════════════════════════════

/// Divides `length` into equal bays closest to `target`.
std::vector<double> compute_bays(double length, double target);

/// 2D axis lines and optional intrinsic boundary rings in local `(u, v)` coordinates of a `Plane`.
struct Pattern {
    std::vector<session_cpp::Line> lines;
    std::vector<int> families;
    std::vector<session_cpp::Polyline> boundary; ///< Optional default boundary + hole loops (e.g. radial oculus).

    /// Two axis families spaced by `u_bays` and `v_bays`, with family 1 skewed by `skew_degrees` from +v.
    static Pattern orthogonal(
        const std::vector<double>& u_bays,
        const std::vector<double>& v_bays,
        double skew_degrees = 0.0
    );

    /// Concentric polygon rings at `radii` and `sectors` radial rays (`inner_hole` leaves the inner ring open as an oculus).
    static Pattern
    radial(const std::vector<double>& radii, int sectors, bool inner_hole = true);

    /// Three axis families at 0, 60 and 120 degrees forming equilateral triangles of `side`.
    static Pattern triangular(double side, int nu, int nv);

    /// Regular honeycomb of hexagons of `side`, `nu` columns by `nv` rows.
    static Pattern hexagonal(double side, int nu, int nv);

    /// Explicit 2D lines with optional family tags (`0`, `1`, ...).
    static Pattern
    from_lines(const std::vector<session_cpp::Line>& lines, const std::vector<int>& families = {});

    /// Copy transformed in 2D/3D by `xform`.
    Pattern transformed(const session_cpp::Xform& xform) const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Plan (Single Floor, Single Bay, or Single Facade on Any Plane)
// ═══════════════════════════════════════════════════════════════════════════

/// One topological segment between two nodes of a `Plan`.
struct PlanEdge {
    size_t a = 0;
    size_t b = 0;
    int family = -1;       ///< Pattern axis family (`0`, `1`, `2`), or `-1` for a pure boundary segment.
    bool boundary = false; ///< True when the edge bounds the outer perimeter or an opening/courtyard hole.
    bool core = false;     ///< True when the edge lies on a core ring.
};

/// One bounded polygonal bay of a `Plan` in local `(u, v)` vertex indices (no triangulation chords).
struct PlanCell {
    std::vector<size_t> outer;              ///< Counter-clockwise outer boundary loop in `Plan::nodes`.
    std::vector<std::vector<size_t>> holes; ///< Optional inner hole loops in `Plan::nodes`.
};

/// Clean 2D cell-edge-node subdivision embedded on an arbitrary 3D `Plane` (floor, roof, or facade).
struct Plan {
    session_cpp::Plane frame = session_cpp::Plane::xy_plane();
    std::vector<session_cpp::Point> nodes;       ///< Local `(u, v, 0)` coordinates on `frame`.
    std::vector<bool> standing;                  ///< True at structural column/post nodes.
    std::vector<std::optional<session_cpp::Point>> column_feet; ///< Optional 3D world column foot per node (for tapered/twisted stacks).
    std::vector<PlanEdge> edges;                 ///< Unique undirected structural/boundary edges.
    std::vector<PlanCell> cells;                 ///< Polygonal bays without mesh triangulation diagonals.
    std::vector<session_cpp::Polyline> cores;    ///< Closed core loops in local `(u, v, 0)`.
    std::vector<session_cpp::Polyline> boundary; ///< Outer + hole boundary loops in local `(u, v, 0)`.

    /// Maps local `(u, v, w)` coordinates on `frame` into 3D world space.
    session_cpp::Point to_world(double u, double v, double w = 0.0) const;

    /// Maps a 3D world point into local `(u, v, w)` coordinates of `frame`.
    session_cpp::Point to_local(const session_cpp::Point& world) const;

    /// Lifts a local `(u, v, 0)` polyline to world space at normal offset `w`.
    session_cpp::Polyline lift_polyline(const session_cpp::Polyline& local_loop, double w = 0.0) const;

    /// Number of bounded polygonal bays in this plan.
    size_t bay_count() const {
        return cells.size();
    }

    /// Isolates a single bay `index` as its own standalone `Plan` on the same `Plane`.
    Plan bay(size_t index) const;

    /// Isolates a subset of bays `indices` as a standalone `Plan` on the same `Plane`.
    Plan bays(const std::vector<size_t>& indices) const;

    /// Copy with `frame` moved by `xform` (keeping local `(u, v)` topology intact).
    Plan transformed(const session_cpp::Xform& xform) const;

    /// Every bounded cell of `pattern` on `frame` (uses `pattern.boundary` when present).
    static Plan from_pattern(
        const Pattern& pattern, const session_cpp::Plane& frame = session_cpp::Plane::xy_plane()
    );

    /// Clips `pattern` to world-space `footprint` rings (outer + holes) and `cores` projected onto `frame`.
    static Plan from_footprint(
        const std::vector<session_cpp::Polyline>& footprint,
        const Pattern& pattern,
        const std::vector<session_cpp::Polyline>& cores = {},
        const session_cpp::Plane& frame = session_cpp::Plane::xy_plane()
    );

    /// Vertical (or custom-plane) facade grid over `baseline` and `height`, with optional `(u, v)` window `openings` and `silhouette`.
    static Plan facade(
        const session_cpp::Line& baseline,
        double height,
        const std::vector<double>& u_bays,
        const std::vector<double>& v_levels,
        const std::vector<session_cpp::Polyline>& openings_uv = {},
        const session_cpp::Polyline& silhouette_uv = {}
    );
};

// ═══════════════════════════════════════════════════════════════════════════
// Stack (Thin Multi-Storey Composition of Floor & Facade Plans)
// ═══════════════════════════════════════════════════════════════════════════

/// One elevated floor level in a multi-storey `Stack`.
struct LevelSpec {
    Plan plan;
    Framing framing;
    std::string name;
};

/// Multi-storey timber building composed of horizontal floor `Plan`s and optional vertical facade `Plan`s.
struct Stack {
    std::vector<LevelSpec> levels;
    std::vector<LevelSpec> facades;

    /// Extrudes one `footprint` + `pattern` + `cores` across `elevations` (`elevations[0]` is ground base; `1..n` are framed levels).
    static Stack from_footprint(
        const std::vector<session_cpp::Polyline>& footprint,
        const std::vector<double>& elevations,
        const Pattern& pattern,
        const Framing& framing = {},
        const std::vector<session_cpp::Polyline>& cores = {}
    );

    /// Builds a multi-storey stack where each storey supplies its own `(elevation, footprint, pattern, framing, cores)`.
    static Stack from_levels(
        double base_elevation,
        const std::vector<std::tuple<
            double,
            std::vector<session_cpp::Polyline>,
            Pattern,
            Framing,
            std::vector<session_cpp::Polyline>>>& storeys
    );

    /// Multi-storey building whose floor footprints shift horizontally by `shifts_xy` over a shared plumb column grid.
    static Stack shifted(
        const std::vector<session_cpp::Polyline>& base_footprint,
        const std::vector<double>& elevations,
        const std::vector<std::pair<double, double>>& shifts_xy,
        const Pattern& shared_pattern,
        const Framing& framing = {},
        const std::vector<session_cpp::Polyline>& cores = {}
    );

    /// Multi-storey tapered building scaling smoothly from `1.0` at ground to `top_scale` at roof, with inclined columns.
    static Stack tapered(
        const std::vector<session_cpp::Polyline>& base_footprint,
        const std::vector<double>& elevations,
        const Pattern& base_pattern,
        double top_scale,
        const Framing& framing = {},
        const std::vector<session_cpp::Polyline>& cores = {}
    );

    /// Multi-storey twisting tower rotating progressively up to `total_twist_degrees` at roof, with helicoidal columns.
    static Stack twisted(
        const std::vector<session_cpp::Polyline>& base_footprint,
        const std::vector<double>& elevations,
        const Pattern& base_pattern,
        double total_twist_degrees,
        const Framing& framing = {},
        const std::vector<session_cpp::Polyline>& cores = {}
    );

    /// Appends vertical facade `Plan`s around the outer ring of `footprint` spanning `[z_bottom, z_top]`.
    void add_perimeter_facades(
        const session_cpp::Polyline& footprint_outer,
        double z_bottom,
        double z_top,
        double target_bay,
        const std::vector<double>& storey_heights,
        const Framing& facade_framing
    );
};

// ═══════════════════════════════════════════════════════════════════════════
// Grid Session
// ═══════════════════════════════════════════════════════════════════════════

/// Timber framing generator: builds elements and face-to-face contacts for one `Plan` or a multi-storey `Stack`.
class Grid : public wood_session::WoodSession {
public:
    /// Frames a single floor, single isolated bay, sloped roof, or vertical facade `Plan`.
    Grid(const Plan& plan, const Framing& framing = {}, std::string name = "grid");

    /// Frames a multi-storey `Stack` of floor plans and optional facade plans.
    Grid(const Stack& stack, std::string name = "building");
};

} // namespace wood_grid
