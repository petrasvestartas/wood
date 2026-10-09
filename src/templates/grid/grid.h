#pragma once
#include "wood_session.h"
#include "wood_profile.h"

namespace wood_grid {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Pattern
// ═══════════════════════════════════════════════════════════════════════════

/// Plan lines a building is drawn on, at z 0, each in a parallel family.
struct Pattern {
    std::vector<Line> lines; // Segments in plan, as long as the pattern extends; a footprint beyond them gets no members there.
    std::vector<int> families; // One per line.

    /// Lines along x at the sums of ys and along y at the sums of xs, the y lines leaning skew degrees.
    static Pattern orthogonal(const std::vector<double>& xs, const std::vector<double>& ys, double skew = 0.0);

    /// Rays from the first radius to the last and ring chords at every radius over sweep degrees.
    static Pattern radial(const std::vector<double>& radii, int sectors, double sweep = 360.0);

    /// Three families of lines at 0, 60 and 120 degrees, side apart, over nx by ny rhombi.
    static Pattern triangular(double side, int nx, int ny);

    /// Edges of pointy-top hexagons of side in nx columns and ny rows, every edge a free line (family -1).
    static Pattern hexagonal(double side, int nx, int ny);

    /// Lines as drawn with a family per line, all free when families is empty.
    static Pattern from_lines(const std::vector<Line>& lines, const std::vector<int>& families = {});

    /// A copy moved by xform: the pattern's origin and rotation under the building.
    Pattern transformed(const Xform& xform) const;
};

/// Bay widths over length at spacing, Branch's rule.
std::vector<double> compute_bays(double length, double spacing, double remainder = 304.8);

// ═══════════════════════════════════════════════════════════════════════════
// Framing
// ═══════════════════════════════════════════════════════════════════════════

/// Section per role, each a profile centred on the member axis, x width and y depth up.
struct Profiles {
    std::vector<Polyline> column = wood_session::profile_rectangle(300.0, 300.0);
    std::vector<Polyline> girder = wood_session::profile_rectangle(200.0, 600.0);
    std::vector<Polyline> beam = {}; // Members on free lines and under span -1; empty takes girder.
    std::vector<Polyline> purlin = {}; // Purlin rows under system 2; empty takes beam.
    std::vector<Polyline> brace = {}; // Workflow C braces; empty takes beam.
};

/// How every level is framed and jointed.
struct Framing {
    int system = 1; // 0 point supported, 1 post and beam, 2 purlin on girder.
    int span = 0; // Pattern family the girders run on; -1 every line carries a beam.
    double spacing = 3000.0; // Largest purlin spacing under system 2.
    int node = 0; // Column joint: 0 head, 1 flush, 2 through.
    double drop = 0.0; // Girder top below the datum: 0 flush with the purlins, 203.2 hung as Branch, the purlin depth stacked.
    double deck = 200.0; // Deck thickness, above the datum or flush in the bay under post and beam with heads.
    double wall = 200.0; // Facade and core wall thickness, centred on the line.
    double head = 300.0; // Height of a head that carries the deck alone.
    double reach = 400.0; // How far a head reaches from the column centre.
    int capital = 0; // Head under the deck alone: 0 conical, 1 stepped under a drop panel.
    double panel = 0.0; // Largest deck strip width across the deck span; 0 one deck per bay.
    double taper = 30.0; // Largest lean in degrees of a column following a moving section; beyond it the vertex is a transfer.
    bool facade = false; // A wall under every perimeter member.
    Profiles profiles = {}; // Sections per role.
};

// ═══════════════════════════════════════════════════════════════════════════
// Building
// ═══════════════════════════════════════════════════════════════════════════

/// One level: its datum, its cores and its plan, whose double attributes a user may set before building a Grid.
struct Level {
    double z = 0.0; // Datum: the framing top, the deck underside.
    std::vector<Polyline> cores; // Core rings on the wall centre line, counter-clockwise at z 0.
    Mesh plan; // Arrangement of the pattern and the section rings: faces are bays, edges member lines, vertices column points.
};

/// A building as its levels: the same pipeline from a massing, a footprint or drawn lines; a Grid frames it.
struct Building {
    std::vector<Level> levels; // Ascending; storey k spans levels[k] to levels[k + 1].
    std::vector<Line> braces; // Tilted lines from from_lines, built as beams cut by what they meet.
    double tolerance = 1.0; // Weld distance and coplanarity tolerance the plans were built with.

    /// A. A closed massing sliced at elevations, the pattern filling the sections, cores on every level.
    static Building from_solid(
        const Mesh& massing,
        const std::vector<double>& elevations,
        const Pattern& pattern,
        const std::vector<Polyline>& cores = {},
        double tolerance = 1.0,
        double merge = 1000.0
    );

    /// B. Footprint rings filled with the pattern at every elevation, empty for every bounded cell of the pattern.
    static Building from_footprint(
        const std::vector<Polyline>& footprint,
        const std::vector<double>& elevations,
        const Pattern& pattern,
        const std::vector<Polyline>& cores = {},
        double tolerance = 1.0,
        double merge = 1000.0
    );

    /// C. Members and surfaces as drawn: horizontal lines and floors make the plans, vertical lines the columns, tilted lines braces.
    static Building from_lines(
        const std::vector<Line>& lines,
        const std::vector<Polyline>& surfaces,
        double tolerance = 1.0,
        double angle = 10.0
    );
};

namespace build {
struct Context;
struct Stack;
}

// ═══════════════════════════════════════════════════════════════════════════
// Grid
// ═══════════════════════════════════════════════════════════════════════════

/// The timber building, a session built level by level from a Building guide and a Framing.
///
/// Fields: `framing` and `guide`, its plans with the roles filled.
/// Every element is found in the session by name, `<kind>_<i>_<level>` under `level_<level>` > `<kind>s_<level>`.
class Grid : public wood_session::WoodSession {
public:
    const Framing framing; // How every level is framed and jointed.
    const Building guide; // The levels, their plans with the roles filled.

    /// The building of the guide framed by framing, as the session named name.
    explicit Grid(
        const Building& guide,
        const Framing& framing = {},
        const std::string& name = "grid"
    );

    /// Not copied, as a session is not.
    Grid(const Grid&) = delete;

    /// Not assigned, as it is not copied.
    Grid& operator=(const Grid&) = delete;

private:
    /// Adds every level's plan at its datum: the member lines and the column points.
    void add_plans();

    /// Adds the storey's columns, walls, heads, members, purlins, decks and braces under the level it caps.
    void add_storey(size_t storey);

    /// Adds the columns of the storey from the deck top below to the head bottom or the datum.
    void add_columns(
        const Building& working,
        size_t storey,
        const std::vector<build::Stack>& stacks,
        const build::Context& upper
    );

    /// Adds the core walls in a pinwheel and the facade walls under the perimeter members.
    void add_walls(
        const Building& working,
        size_t storey,
        const build::Context& upper
    );

    /// Adds the head on every column under node 0 and returns the heads by plan vertex.
    std::map<size_t, std::shared_ptr<Element>> add_heads(
        const Level& level,
        const std::vector<build::Stack>& stacks,
        const build::Context& upper,
        size_t place
    );

    /// Adds the girders, beams and purlins of a level, their ends cut by the joint rules.
    void add_members(
        const Level& level,
        const build::Context& upper,
        const std::map<size_t, std::vector<Polyline>>& outlines,
        size_t place
    );

    /// Adds the decks of a level, a flush deck cut by the heads at its corners.
    void add_decks(
        const Level& level,
        const build::Context& context,
        const std::map<size_t, std::vector<Polyline>>& outlines,
        const std::map<size_t, std::shared_ptr<Element>>& heads,
        size_t place
    );

    /// Adds the drawn braces standing in the storey.
    void add_braces(
        size_t storey,
        const build::Context& ground,
        const build::Context& upper
    );

    /// Adds each element under its family group of the level, named by its kind, count and level.
    void add_numbered(const std::vector<std::shared_ptr<Element>>& elements, size_t place);

    /// The group of a level, made the first time.
    std::shared_ptr<TreeNode> level_group(size_t place);

    /// The framing as built: point supported with node 2 framed as node 1.
    static Framing compute_framing(const Framing& framing);

    /// The guide with every plan's roles filled and the columns inside core walls removed.
    static Building compute_guide(const Building& guide, const Framing& framing);
};

} // namespace wood_grid
