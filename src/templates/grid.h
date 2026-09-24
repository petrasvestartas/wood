#pragma once
#include "wood_session.h"
#include "wood_profile.h"
#include "wood_element_geometry.h"

namespace wood_grid {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Pattern
// ═══════════════════════════════════════════════════════════════════════════

/// Plan lines a building is drawn on, at z 0, each in a parallel family: 0 and 1 the two directions a rectangular system names, 2 a third family, -1 a free line.
struct Pattern {
    std::vector<Line> lines; // Segments in plan, as long as the pattern extends; a footprint beyond them gets no members there.
    std::vector<int> families; // One per line.

    /// Lines along x at the running sums of ys (family 0) and along y at the running sums of xs (family 1), the y axis leaning skew degrees towards x.
    static Pattern orthogonal(const std::vector<double>& xs, const std::vector<double>& ys, double skew = 0.0);

    /// Rays from the first radius to the last (family 0) and ring chords at every radius (family 1) over sweep degrees; a first radius of 0 gives a centre point.
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

/// Bay widths over length at spacing, Branch's rule: whole bays from the start while the rest is at least remainder long, the rest as the last bay.
std::vector<double> compute_bays(double length, double spacing, double remainder = 304.8);

// ═══════════════════════════════════════════════════════════════════════════
// Framing
// ═══════════════════════════════════════════════════════════════════════════

/// Section per role, each a profile in its own frame (loop 0 outer counter-clockwise centred on the axis, x width, y depth up; loops 1.. holes); an empty role falls back as noted.
struct Profiles {
    std::vector<Polyline> column = wood_session::profile_rectangle(300.0, 300.0);
    std::vector<Polyline> girder = wood_session::profile_rectangle(200.0, 600.0);
    std::vector<Polyline> beam = {}; // Members on free lines and under span -1; empty takes girder.
    std::vector<Polyline> purlin = {}; // Purlin rows and stations; empty takes beam.
    std::vector<Polyline> edge_girder = {}; // Perimeter members on girder-family lines; empty takes girder.
    std::vector<Polyline> edge_beam = {}; // Perimeter members on any other line or ring edge; empty takes purlin under system 2, else beam.
    std::vector<Polyline> brace = {}; // Workflow C braces; empty takes beam.
};

/// How every level is framed and jointed: the structural method, the joint choices and the sizes; per-bay and per-member changes are attributes on the level plans.
struct Framing {
    int system = 1; // 0 point supported (deck on columns or heads, no members), 1 post and beam (girders on the span family, the deck spans between them), 2 purlin on girder (girders plus purlin rows at spacing).
    int span = 0; // Pattern family the girders run on, and under system 0 the family the deck strips run along; -1 every line carries a beam (two-way, hexagonal, irregular).
    double spacing = 3000.0; // Largest purlin spacing under system 2; ceil(cell / spacing) intervals per unclipped cell, one row on every interior cross line.
    bool edge = true; // Perimeter members on the section rings and hole rings.
    int node = 0; // Column joint: 0 head (under the members, or under the deck where none arrive), 1 flush (column top at the datum, members into its faces, deck over all), 2 through (column datum to datum, deck notched, members into its faces; as 1 under system 0, so the deck has a bearing).
    double drop = 0.0; // Girder top below the datum: 0 flush with the purlins, 203.2 hung as Branch, the purlin depth stacked.
    double deck = 200.0; // Deck thickness above the datum.
    double wall = 200.0; // Facade and core wall thickness, centred on the line.
    double head = 300.0; // Head height under node 0.
    double reach = 400.0; // Top half-width of a head that carries cut member ends or the deck; a head members only rest on is the column section extruded.
    int capital = 0; // Shape of a head that carries cut member ends or the deck: 0 conical, a frustum from the column section up to reach; 1 stepped, a capital to halfway under a drop panel at reach.
    double panel = 0.0; // Largest deck strip width across the deck span; 0 one deck per bay.
    double taper = 30.0; // Largest lean in degrees of a perimeter column following a moving section; beyond it the vertex is a transfer.
    bool facade = false; // A wall under every perimeter member.
    Profiles profiles = {}; // Sections per role.
};

// ═══════════════════════════════════════════════════════════════════════════
// Building
// ═══════════════════════════════════════════════════════════════════════════

/// One level: its datum, its section rings and cores, and the plan the pattern fills into the section, whose double attributes (docs/templates.md) a user may set before to_elements.
struct Level {
    double z = 0.0; // Datum: the framing top, the deck underside.
    std::vector<Polyline> rings; // Section at z, the union of the slices just below and just above: outer rings counter-clockwise seen from above, holes clockwise, all at z 0.
    std::vector<Polyline> cores; // Core rings on the wall centre line, counter-clockwise: a void face, a wall per side, a deck hole, a support for the members that reach them.
    Mesh plan; // Arrangement of the pattern and the rings inside the section; a hole or core ring that meets no line is a face hole of its bay.
};

/// A building as its levels: the same pipeline from a massing, a footprint or drawn lines; elements per storey from a Framing.
struct Building {
    std::vector<Level> levels; // Ascending; storey k spans levels[k] to levels[k + 1]; levels[0] is the ground and carries the column feet.
    std::vector<Line> braces; // Tilted lines from from_lines, built as beams cut by what they meet.
    Pattern pattern; // The lines the plans were drawn on: the unclipped cells purlin stations are spaced over.
    double tolerance = 1.0; // Weld distance, coplanarity and clash tolerance the plans were built with.

    /// A. A closed massing sliced at elevations: sections just below and just above each, their union filled with pattern, cores as rings through every level; columns follow the sections within taper; a BRep goes through to_mesh first.
    static Building from_solid(const Mesh& massing, const std::vector<double>& elevations, const Pattern& pattern, const std::vector<Polyline>& cores = {}, double tolerance = 1.0, double merge = 1000.0);

    /// B. Footprint rings (outer counter-clockwise, holes clockwise; empty means every bounded cell of the pattern that more than one family bounds) at the level elevations, the first the ground, the same section on every level, cores on every level.
    static Building from_footprint(const std::vector<Polyline>& footprint, const std::vector<double>& elevations, const Pattern& pattern, const std::vector<Polyline>& cores = {}, double tolerance = 1.0, double merge = 1000.0);

    /// C. Members and surfaces as drawn: horizontal lines and floors make the plan of their level, vertical lines its column points, vertical surfaces its walls (core walls when named core), tilted lines braces, each within angle degrees; lines split at every node and crossing.
    static Building from_lines(const std::vector<Line>& lines, const std::vector<Polyline>& surfaces, double tolerance = 1.0, double angle = 10.0);

    /// Every element of storey k with its joints resolved, world space, in plan order so instance_by_key() dedups them: columns and walls standing in the storey, then the heads, members, stations and decks of the level that caps it, then its braces.
    std::vector<std::shared_ptr<Element>> to_elements(const Framing& framing, size_t storey) const;

    /// Every storey's elements added to session under a group per storey named storey_k.
    void to_session(wood_session::WoodSession& session, const Framing& framing) const;
};

/// A BRep massing as the closed mesh Building::from_solid slices, facet degrees per curved face; the sections take their corners from the pattern, so any facet serves.
Mesh to_mesh(const BRep& massing, double facet = 15.0, double tolerance = 1.0);

} // namespace wood_grid
