#pragma once

#include "wood_session.h"
#include "wood_profile.h"

namespace wood_grid {

using session_cpp::Line;
using session_cpp::Plane;
using session_cpp::Point;
using session_cpp::Polyline;
using session_cpp::Xform;

// ═══════════════════════════════════════════════════════════════════════════
// Framing
// ═══════════════════════════════════════════════════════════════════════════

/// Structural framing family on a planar grid.
enum class System {
    Slab,          ///< Point-supported CLT slab directly on columns or heads.
    PostAndBeam,   ///< Primary girders on `span` axes and secondary beams (`span = -1`: all primary).
    GirderPurlin,  ///< Primary girders with flush subdivided purlins trimmed to girder side faces.
    StackedPurlin, ///< Continuous subdivided purlins stacked on top of lowered primary girders.
};

/// Column-to-frame connection detail at standing grid nodes.
enum class Node {
    Flush,   ///< Columns stop at the framing soffit.
    Through, ///< Continuous columns to the datum plane; beams and decks butt to column faces.
    Head,    ///< Chamfered timber head: in-line with beams (`[-beam_depth, 0]`), or under a `Slab`.
};

/// In-plane timber bracing inside selected quadrilateral bays.
enum class Brace {
    None,
    Diagonal,
    Cross,
    Chevron,
};

/// Timber cross-sections by member role.
struct Profiles {
    std::vector<Polyline> column = wood_session::profile_rectangle(260.0, 260.0);
    std::vector<Polyline> girder = wood_session::profile_rectangle(260.0, 520.0);
    std::vector<Polyline> beam; ///< Optional secondary/edge beam; empty reuses `girder`.
    std::vector<Polyline> purlin = wood_session::profile_rectangle(160.0, 340.0);
    std::vector<Polyline> brace = wood_session::profile_rectangle(160.0, 160.0);
};

/// Timber framing specification for a planar `Plan` or multi-storey `Stack`.
struct Framing {
    System system = System::PostAndBeam;
    Node node = Node::Flush;
    Brace brace = Brace::None;
    std::vector<size_t> braced_bays; ///< Empty braces every quad bay when `brace != Brace::None`.
    int span = 0;                    ///< Primary axis family (`0` or `1`), or `-1` for all axes.
    double height = 3600.0;          ///< Column length along `-frame.z_axis()` (`0` omits posts).
    double spacing = 2000.0;         ///< Target purlin spacing in `GirderPurlin` / `StackedPurlin`.
    double deck = 0.0;               ///< CLT deck or infill thickness (`0` omits panels).
    bool deck_flush = false;         ///< `false`: `[0, +deck]`; `true`: flush infill in `[-deck, 0]`.
    double panel = 0.0;              ///< Max CLT strip width (`0` keeps one plate per bay).
    double wall = 0.0;               ///< CLT shear wall thickness around `Plan::cores`.
    double head = 240.0;             ///< Under-slab chamfered head height (`Slab` + `Node::Head`).
    double reach = 180.0;            ///< Outward chamfer projection of `Node::Head`.
    Profiles profiles;
};

// ═══════════════════════════════════════════════════════════════════════════
// Pattern
// ═══════════════════════════════════════════════════════════════════════════

/// Divides `length` into equal bays closest to `target`.
std::vector<double> compute_bays(double length, double target);

/// 2D axis lines and optional default boundary rings in local `(u, v)` coordinates.
struct Pattern {
    std::vector<Line> lines;
    std::vector<int> families;
    std::vector<Polyline> boundary;

    static Pattern orthogonal(
        const std::vector<double>& u_bays,
        const std::vector<double>& v_bays,
        double skew_degrees = 0.0
    );
    static Pattern radial(const std::vector<double>& radii, int sectors, bool inner_hole = true);
    static Pattern triangular(double side, int nu, int nv);
    static Pattern hexagonal(double side, int nu, int nv);
    static Pattern from_lines(
        const std::vector<Line>& lines, const std::vector<int>& families = {}
    );
    Pattern transformed(const Xform& xform) const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Plan
// ═══════════════════════════════════════════════════════════════════════════

/// Planar cell-edge-node grid embedded on any 3D `Plane` (floor, isolated bay, roof, or facade).
struct Plan {
    struct Edge {
        size_t a = 0;
        size_t b = 0;
        int family = -1;
        bool boundary = false;
        bool core = false;
    };

    struct Cell {
        std::vector<size_t> outer;
        std::vector<std::vector<size_t>> holes;
    };

    Plane frame = Plane::xy_plane();
    std::vector<Point> nodes;
    std::vector<bool> standing;
    std::vector<std::optional<Point>> column_feet;
    std::vector<Edge> edges;
    std::vector<Cell> cells;
    std::vector<Polyline> cores;
    std::vector<Polyline> boundary;

    Point to_world(double u, double v, double w = 0.0) const;
    size_t bay_count() const { return cells.size(); }
    Plan bay(size_t index) const;
    Plan bays(const std::vector<size_t>& indices) const;
    Plan transformed(const Xform& xform) const;

    static Plan from_pattern(const Pattern& pattern, const Plane& frame = Plane::xy_plane());
    static Plan from_footprint(
        const std::vector<Polyline>& footprint,
        const Pattern& pattern,
        const std::vector<Polyline>& cores = {},
        const Plane& frame = Plane::xy_plane()
    );
    static Plan facade(
        const Line& baseline,
        double height,
        const std::vector<double>& u_bays,
        const std::vector<double>& v_levels,
        const std::vector<Polyline>& openings_uv = {},
        const Polyline& silhouette_uv = {}
    );
};

// ═══════════════════════════════════════════════════════════════════════════
// Stack & Grid
// ═══════════════════════════════════════════════════════════════════════════

/// Multi-storey composition of horizontal floor `Plan`s and optional vertical facade `Plan`s.
struct Stack {
    struct Level {
        Plan plan;
        Framing framing = {};
    };

    std::vector<Level> levels;
    std::vector<Level> facades;

    /// Extrudes one `footprint` across `elevations` (`elevations[0]` is ground base; `1..n` are floors).
    static Stack from_footprint(
        const std::vector<Polyline>& footprint,
        const std::vector<double>& elevations,
        const Pattern& pattern,
        const Framing& framing = {},
        const std::vector<Polyline>& cores = {}
    );

    /// Composes arbitrary per-storey floor `Level`s (setbacks, shifted cantilevers, custom framing).
    static Stack from_levels(
        std::vector<Level> levels, double base_z = 0.0, bool plumb_columns = true
    );

    /// Morphs `footprint` and `pattern` by `xform_at(t)` (`t` in `[0, 1]`) with inclined/helicoidal columns.
    static Stack morph(
        const std::vector<Polyline>& footprint,
        const std::vector<double>& elevations,
        const Pattern& pattern,
        const std::function<Xform(double)>& xform_at,
        const Framing& framing = {},
        const std::vector<Polyline>& cores = {}
    );

    /// Appends planar infill facades around clear perimeter bays of every floor level.
    void add_facades(double target_bay, const Framing& framing, double base_z = 0.0);
};

/// Timber framing generator: builds elements and face-to-face contacts for a `Plan` or `Stack`.
class Grid : public wood_session::WoodSession {
public:
    explicit Grid(const Plan& plan, const Framing& framing = {}, std::string name = "grid");
    explicit Grid(const Stack& stack, std::string name = "grid");
};

} // namespace wood_grid
