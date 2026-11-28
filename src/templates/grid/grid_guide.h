#pragma once

#include "wood_session.h"
#include "wood_profile.h"

namespace wood_grid {

using laika_cpp::Line;
using laika_cpp::Plane;
using laika_cpp::Point;
using laika_cpp::Polyline;
using laika_cpp::Xform;

// ═══════════════════════════════════════════════════════════════════════════
// Profiles
// ═══════════════════════════════════════════════════════════════════════════

/// Timber cross-sections by member role.
struct Profiles {
    std::vector<Polyline> column =
        wood_session::profile_rectangle(260.0, 260.0); // Section of a column.
    std::vector<Polyline> girder =
        wood_session::profile_rectangle(260.0, 520.0); // Section of a primary girder.
    std::vector<Polyline> beam; // Section of a secondary or edge beam, empty for the girder's.
    std::vector<Polyline> purlin =
        wood_session::profile_rectangle(160.0, 340.0); // Section of a purlin.
    std::vector<Polyline> brace =
        wood_session::profile_rectangle(160.0, 160.0); // Section of a brace.

    /// Heavier glulam: columns 280, girders 280 x 600, purlins 220 x 440.
    static Profiles heavy();

    /// Facade members: mullions 160 x 280, transoms 140 x 280, braces 140 x 180.
    static Profiles mullions();

    /// Built-up section-active timber: glulam columns 260, girders 260 x 560, I-beam purlins 180 x 380, hollow box edge beams 240 x 460.
    static Profiles built_up();
};

// ═══════════════════════════════════════════════════════════════════════════
// Framing
// ═══════════════════════════════════════════════════════════════════════════

/// Timber framing of a planar `Plan` or of every storey of a `Stack`.
struct Framing {
    int system =
        1; // 0 a CLT slab on the columns, 1 girders and beams, 2 girders with flush purlins, 3 purlins on lowered girders, 4 a two-way waffle slab.
    int node =
        0; // Columns: 0 stop under the framing, 1 run through to the datum, 2 carry a chamfered head.
    int brace = 0; // Braces in the bays: 0 none, 1 a diagonal, 2 a cross, 3 a chevron.
    std::vector<size_t> braced_bays; // Bays braced, empty for every quad bay.
    int span = 0; // Axis family of the girders, 0 or 1; -1 makes every axis equal, -2 gradates by family 0 > 1 > 2.
    double height = 3600.0; // Column length below the plan, 0 for none.
    double spacing = 2000.0; // Target purlin spacing.
    double deck = 0.0; // CLT deck thickness, 0 for none.
    bool deck_flush = false; // Deck on the framing in [0, deck], or flush within it in [-deck, 0].
    double panel = 0.0; // Largest CLT strip width, 0 for one plate per bay.
    double wall = 0.0; // CLT shear wall thickness around the plan's cores.
    double head = 240.0; // Height of a chamfered head under a slab.
    double reach = 180.0; // Outward projection of a head's chamfer.
    double haunch = 0.0; // Extra girder depth over standing columns, tapering into spans and along cantilevers.
    double belly = 0.0; // Parabolic extra girder depth at the midspan of simple spans.
    double gerber_lap = 0.0; // Stepped half-lap length at zero-moment Gerber girder hinges.
    double drop_width = 0.0; // Square solid shear drop-panel side over standing columns under a slab or waffle grid.
    double drop_depth = 0.0; // Drop-panel thickness, 0 for head height under slabs or rib depth in waffle grids.
    double foot_scale = 1.0; // Scale of a standing column's foot section, below 1 for tapered column shafts.
    bool pinned_feet = false; // Base support shoe under each standing column foot.
    bool open_perimeter = false; // Suppresses unsupported perimeter fascia beams so slabs and ribs cantilever with open ends.
    Profiles profiles; // Member sections.

    /// Girders and beams of 240 columns under a 160 deck, for the L-shaped floor.
    static Framing l_shaped_floor();

    /// A CLT slab 240 in 3000 strips on chamfered column heads.
    static Framing slab_on_heads();

    /// Girders on every axis between chamfered column heads.
    static Framing beams_on_heads();

    /// Heavy girders with flush purlins every 2000 under a 140 deck.
    static Framing girders_and_purlins();

    /// Heavy girders with flush purlins, the columns running through.
    static Framing through_columns();

    /// Heavy purlins every 2000 stacked on lowered girders.
    static Framing stacked_purlins();

    /// Heavy girders with flush purlins on round columns.
    static Framing round_columns();

    /// Girders with purlins every 2500 under a 160 deck, for the large L-shaped floor.
    static Framing large_l_shaped_floor();

    /// Girders with purlins every 2000, for the skewed floor.
    static Framing skewed_floor();

    /// Ring girders and radial beams on round columns, for the ring floor.
    static Framing ring_floor();

    /// Girders on every axis on round columns, for the triangular floor.
    static Framing triangular_floor();

    /// Girders on every axis on round columns, for the hexagonal floor.
    static Framing hexagonal_floor();

    /// Continuous girders with cross beams, for the irregular floor.
    static Framing irregular_floor();

    /// Girders with purlins, the columns running through, for the courtyard floor.
    static Framing courtyard_floor();

    /// Continuous mullions and transoms, for the curtain wall.
    static Framing curtain_wall();

    /// Mullions and transoms with flush CLT spandrels 120, for the punched wall.
    static Framing punched_wall();

    /// Heavy mullions and transoms with a cross in the outer bays, for the braced wall.
    static Framing braced_wall();

    /// Mullions and transoms with chevron braces, for the gable wall.
    static Framing gable_wall();

    /// Spandrel bands 760 x 180 behind vertical fins 95 x 320, for the fin screen.
    static Framing fin_screen();

    /// Rectangular girders carrying built-up timber I-beam purlins and hollow box edge beams.
    static Framing built_up_profiles();

    /// Simple-span girders with parabolic midspan belly depth under stacked purlins.
    static Framing belly_beams();

    /// Continuous girders with haunches over inset columns and tapered end cantilevers.
    static Framing haunched_cantilever();

    /// Articulated five-span Gerber girders with stepped half-lap hinges carrying two parabolic drop-in spans.
    static Framing gerber_hall();

    /// Herringbone skew-coupled zigzag girders meeting at symmetric V-nodes on both edges.
    static Framing zigzag_floor();

    /// Commensurate 45-degree diagonal square grillage with symmetric perimeter K-nodes and cantilevered corners.
    static Framing diagonal_floor();

    /// Four-fold symmetric gradated concentric beam grid on eight perimeter fin columns.
    static Framing concentric_floor();

    /// Branching three-way ramified beam layout on six perimeter round columns.
    static Framing ramified_floor();

    /// One-way continuous CLT slab cantilevering on all four sides over two parallel haunched girders.
    static Framing one_way_girder_slab();

    /// Three-sided U-beam supported CLT slab with one free cantilevered open edge.
    static Framing three_sided_slab();

    /// Integral T-beam ribbed floor (`profile_t`) cantilevering over inset primary girders under a thin CLT flange.
    static Framing t_beam_slab();

    /// Point-supported CLT mushroom slab on two-stage flared capitals and square drop-panels.
    static Framing mushroom_drop_slab();

    /// Closely spaced one-way ribs cantilevering with open ends over inset primary girders under a thin top deck.
    static Framing ribbed_cantilever_slab();

    /// Orthogonal two-way coffered waffle slab with solid column shear drop-panels under a thin top deck.
    static Framing waffle_coffered_slab();

    /// Diagonal 45-degree coffered waffle slab with diamond column shear drop-panels under a thin top deck.
    static Framing diagonal_waffle_slab();
};

// ═══════════════════════════════════════════════════════════════════════════
// Pattern
// ═══════════════════════════════════════════════════════════════════════════

/// Divides length into equal bays closest to target.
std::vector<double> compute_bays(double length, double target);

/// Divides length into count symmetric bays scaled by end_ratio at both ends relative to midspan.
std::vector<double> compute_gradated_bays(double length, int count, double end_ratio = 0.55);

/// Axis lines in a plan's (u, v) and the boundary rings they come with.
struct Pattern {
    std::vector<Line> lines; // Axis lines.
    std::vector<int> families; // Family of every line.
    std::vector<Polyline> boundary; // Default boundary rings.

    /// Two families of parallel axes at the bays, the second skewed by degrees.
    static Pattern orthogonal(
        const std::vector<double>& u_bays,
        const std::vector<double>& v_bays,
        double skew_degrees = 0.0
    );

    /// Rays and polygonal rings of sectors around the origin, open in the middle when inner_hole.
    static Pattern radial(const std::vector<double>& radii, int sectors, bool inner_hole = true);

    /// Three families at 60 degrees, triangles of side, nu by nv.
    static Pattern triangular(double side, int nu, int nv);

    /// Hexagons of side, nu by nv.
    static Pattern hexagonal(double side, int nu, int nv);

    /// Two commensurate diagonal axis families inside width by depth, half-module shifted unless corner_anchored.
    static Pattern diagonal(double width, double depth, double pitch, bool corner_anchored = false);

    /// Four-fold symmetric gradated concentric squares and 45-degree corner girders inside width by depth.
    static Pattern concentric(double width, double depth, int rings = 3, double corner_ratio = 0.292893);

    /// Herringbone skew-coupled zigzag axes across strips longitudinal bays and waves full V-cycles.
    static Pattern zigzag(double width, double depth, int strips = 2, int waves = 2);

    /// Branching primary trunks from the perimeter forking at three-way Y-nodes into a central hexagonal spine.
    static Pattern ramified(double width, double depth, int bays = 2);

    /// Any lines, their families from their direction when not given.
    static Pattern
    from_lines(const std::vector<Line>& lines, const std::vector<int>& families = {});

    /// The pattern moved by xform.
    Pattern transformed(const Xform& xform) const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Plan
// ═══════════════════════════════════════════════════════════════════════════

/// Planar cell-edge-node grid on any plane: a floor, an isolated bay, a roof or a facade.
struct Plan {

    /// An axis between two nodes.
    struct Edge {
        size_t a = 0; // First node.
        size_t b = 0; // Second node.
        int family = -1; // Axis family, -1 for a boundary not on an axis.
        bool boundary = false; // On the plan's outline.
        bool core = false; // On a core, framed by its walls.
    };

    /// A bay: its outer loop of nodes and its holes.
    struct Cell {
        std::vector<size_t> outer; // Nodes around the bay.
        std::vector<std::vector<size_t>> holes; // Nodes around each hole.
    };

    Plane frame = Plane::xy_plane(); // The plane of the plan, (u, v) along its axes.
    std::vector<Point> nodes; // Nodes in (u, v).
    std::vector<bool> standing; // Whether a column stands under each node.
    std::vector<std::optional<Point>> column_feet; // Foot of each column when it leans.
    std::vector<Edge> edges; // Axes between nodes.
    std::vector<Cell> cells; // Bays.
    std::vector<Polyline> cores; // Core rings in (u, v).
    std::vector<Polyline> boundary; // Boundary rings in (u, v).

    /// The world point at u, v on the plan and w along its normal.
    Point to_world(double u, double v, double w = 0.0) const;

    /// Number of bays.
    size_t bay_count() const {
        return cells.size();
    }

    /// One bay as its own plan.
    Plan bay(size_t index) const;

    /// Some bays as their own plan.
    Plan bays(const std::vector<size_t>& indices) const;

    /// The plan moved by xform.
    Plan transformed(const Xform& xform) const;

    /// The plan with columns suppressed within overhang_u of the u-ends and overhang_v of the v-ends.
    Plan with_cantilever(double overhang_u, double overhang_v = -1.0) const;

    /// The plan keeping standing columns only every stride_u and stride_v standing grid lines.
    Plan with_column_stride(int stride_u, int stride_v = 0) const;

    /// The plan keeping standing columns only on the exterior boundary.
    Plan perimeter_columns_only() const;

    /// The pattern's bays within its own boundary, on frame.
    static Plan from_pattern(const Pattern& pattern, const Plane& frame = Plane::xy_plane());

    /// The pattern's bays within footprint, around cores, on frame.
    static Plan from_footprint(
        const std::vector<Polyline>& footprint,
        const Pattern& pattern,
        const std::vector<Polyline>& cores = {},
        const Plane& frame = Plane::xy_plane()
    );

    /// A vertical wall on baseline, height tall, its bays and levels, openings and outline in (u, v).
    static Plan facade(
        const Line& baseline,
        double height,
        const std::vector<double>& u_bays,
        const std::vector<double>& v_levels,
        const std::vector<Polyline>& openings_uv = {},
        const Polyline& silhouette_uv = {}
    );

    /// Five bays of 4000 in an L, 12000 x 8000, at 3600.
    static Plan l_shaped_floor();

    /// One bay of 6000 x 6000 at 4200.
    static Plan square_bay();

    /// Two bays of 4500 x 6000 at 4200.
    static Plan double_bay();

    /// Five bays of 8000 x 7500 in an L, 24000 x 15000, at 4000.
    static Plan large_l_shaped_floor();

    /// Four by three bays of 6000 skewed by 30 degrees, at 4000.
    static Plan skewed_floor();

    /// Twelve sectors in rings of 4000, 8000 and 12000 around an oculus, at 4000.
    static Plan ring_floor();

    /// Triangles of 6000, four by three, at 4000.
    static Plan triangular_floor();

    /// Hexagons of 4200, three by two, at 4000.
    static Plan hexagonal_floor();

    /// A five-sided footprint on five free axes, at 4000.
    static Plan irregular_floor();

    /// A 32000 x 24000 floor around a 16000 x 8000 courtyard in bays of 8000, at 4000.
    static Plan courtyard_floor();

    /// A curtain wall 16000 x 10800 in four bays and three storeys.
    static Plan curtain_wall();

    /// A wall 15000 x 8000 in five bays with a door and three windows.
    static Plan punched_wall();

    /// A wall 18000 x 11400 in four bays and three storeys.
    static Plan braced_wall();

    /// A gable wall 16000 wide, 6400 to the eaves and 10800 to the ridge.
    static Plan gable_wall();

    /// A screen 39080 x 14400 in nine bays of five fins and four storeys, from 3600.
    static Plan fin_screen();

    /// Five equal spans of 4800 x 6000 at 4000, for simple-span belly girders.
    static Plan five_span_beams();

    /// Three interior spans of 6000 with 2100 cantilevered ends across 6000 depth, at 4200.
    static Plan cantilever_hall();

    /// Five column bays carrying two suspended drop-in Gerber spans on four stepped cantilever hinges, at 4200.
    static Plan gerber_hall();

    /// A 12000 x 16000 hall of herringbone skew-coupled zigzag girders meeting on both edges and spine, at 4200.
    static Plan zigzag_floor();

    /// A 16000 x 16000 hall on a commensurate 45-degree diagonal grillage with symmetric perimeter K-nodes, at 4200.
    static Plan diagonal_floor();

    /// An 18000 x 18000 four-fold symmetric gradated concentric hall on eight perimeter fin columns, at 4400.
    static Plan concentric_floor();

    /// An 18000 x 12000 hall of three-way branching Y-trunks and an open central hexagonal ring, at 4200.
    static Plan ramified_floor();

    /// A 9600 x 10000 one-way slab cantilevered past two parallel haunched girders on four inset columns, at 4200.
    static Plan one_way_girder_floor();

    /// A 9600 x 8400 three-sided U-beam supported slab with one free cantilevered open edge, at 4200.
    static Plan three_sided_floor();

    /// A 14400 x 10800 integral T-beam floor cantilevering 1800 past two inset girder lines, at 4200.
    static Plan t_beam_floor();

    /// A 16000 x 16000 mushroom slab cantilevered 2000 on all sides over 3 x 3 inset columns, at 4200.
    static Plan mushroom_floor();

    /// A 14400 x 10800 one-way closely ribbed floor cantilevering 1800 past two inset girder lines, at 4200.
    static Plan ribbed_floor();

    /// A 12000 x 12000 orthogonal coffered waffle grid of 1200 pitch cantilevered over four inset columns, at 4200.
    static Plan waffle_floor();

    /// A 14400 x 14400 diagonal 45-degree coffered waffle grid cantilevered over four inset columns, at 4200.
    static Plan diagonal_waffle_floor();
};

// ═══════════════════════════════════════════════════════════════════════════
// Stack
// ═══════════════════════════════════════════════════════════════════════════

/// Multi-storey composition of horizontal floor plans and vertical facade plans.
struct Stack {

    /// A plan and its framing.
    struct Level {
        Plan plan; // The plan.
        Framing framing = {}; // Its framing.
    };

    std::vector<Level> levels; // Floors from the bottom up.
    std::vector<Level> facades; // Facades.

    /// One footprint across elevations, the first the ground and the rest the floors.
    static Stack from_footprint(
        const std::vector<Polyline>& footprint,
        const std::vector<double>& elevations,
        const Pattern& pattern,
        const Framing& framing = {},
        const std::vector<Polyline>& cores = {}
    );

    /// Any floors, the columns plumb on the floor below when plumb_columns.
    static Stack
    from_levels(std::vector<Level> levels, double base_z = 0.0, bool plumb_columns = true);

    /// The footprint and pattern moved by xform_at(t), t from 0 at the bottom to 1 at the top, the columns leaning between.
    static Stack morph(
        const std::vector<Polyline>& footprint,
        const std::vector<double>& elevations,
        const Pattern& pattern,
        const std::function<Xform(double)>& xform_at,
        const Framing& framing = {},
        const std::vector<Polyline>& cores = {}
    );

    /// Adds planar infill facades in the clear perimeter bays of every floor.
    void add_facades(double target_bay, const Framing& framing, double base_z = 0.0);

    /// Two storeys of five bays in an L, chamfered heads in line with the girders and flush decks, starting at x.
    static Stack l_shaped_two_storey(double x = 0.0);

    /// Five storeys of an 18000 x 12000 box shifted by 3000 over one plumb column grid, starting at x.
    static Stack shifted_floors(double x = 0.0);

    /// Six storeys of 21000 square narrowing to 55 percent, the columns running through, starting at x.
    static Stack tapered_tower(double x = 0.0);

    /// Seven storeys of 18000 square turning 42 degrees, on round columns running through, starting at x.
    static Stack twisted_tower(double x = 0.0);

    /// Six storeys stepping back in three terraces around a 10000 atrium, starting at x.
    static Stack setback_tower(double x = 0.0);

    /// Four storeys of sixteen sectors around an oculus, on round columns running through, starting at x.
    static Stack rotunda(double x = 0.0);

    /// Four storeys of a five-sided floor cut at 45 degrees, starting at x.
    static Stack flatiron(double x = 0.0);

    /// Three storeys around a CLT core, the facades braced at the corners, starting at x.
    static Stack core_and_facades(double x = 0.0);

    /// Five storeys of 36000 x 10800 on a recessed colonnade, the upper floors cantilevered 900, the roof 1850, starting at x.
    static Stack colonnade_office(double x = 0.0);

    /// The colonnade office with a storefront in the colonnade and ribbon windows above, starting at x.
    static Stack glazed_office(double x = 0.0);

    /// The glazed office with fin screens on its four sides, starting at x.
    static Stack screened_office(double x = 0.0);

    /// Four staggered single-column cantilevered umbrella canopies at alternating heights, starting at x.
    static Stack umbrella_canopies(double x = 0.0);
};

// ═══════════════════════════════════════════════════════════════════════════
// Design
// ═══════════════════════════════════════════════════════════════════════════

/// One design of a row of grids: a plan and its framing, or a stack.
struct Design {
    std::string name; // Group and grid name.
    Plan plan; // The plan, moved to its place in the row.
    Framing framing; // Its framing.
    std::function<Stack(double)> stack; // The stack made from x along x, empty for the plan.
    double width = 0.0; // Room taken along x, 0 for the plan's own width.

    /// A plan and its framing.
    Design(const std::string& name, const Plan& plan, const Framing& framing, double width = 0.0);

    /// A stack, made where the row places it.
    Design(const std::string& name, const std::function<Stack(double)>& stack, double width);
};

} // namespace wood_grid
