#pragma once
#include "src/templates/grid.h"
#include "src/templates/grid_plan.h"

namespace wood_grid::joints {

// ═══════════════════════════════════════════════════════════════════════════
// Joint records
// ═══════════════════════════════════════════════════════════════════════════

/// What the joint rules read at one level: its plan, the framing, the weld tolerance, the column sections in plan at its vertices the members butt into (nodes 1 and 2), the sections of the columns that rise through its deck, and every column section the deck runs out to.
struct Context {
    const Mesh& plan; // The level plan with its roles filled.
    const Framing& framing; // The framing the level is built with.
    double tolerance = 1.0; // Weld distance and coplanarity tolerance of the building.
    const std::map<size_t, std::vector<Point>>& columns; // Column section at a vertex, counter-clockwise at z 0: members butt into it; empty under node 0.
    const std::map<size_t, std::vector<Point>>& through; // Sections of the columns rising through the deck: the notches.
    const std::map<size_t, std::vector<Point>>& feet; // Every column section at a vertex, the columns standing in the storey and the feet of the storey above: a boundary deck side runs out to them.
};

/// A member end at a plan vertex: what the joint rules read.
struct Member {
    size_t other = 0; // The vertex at its far end.
    Vector direction; // Unit plan direction away from the vertex.
    int role = 0; // Edge role.
    int rank = 0; // compute_rank of the role.
    double width = 0.0; // Profile width.
    double top = 0.0; // Top relative to the datum.
    double bottom = 0.0; // Bottom relative to the datum.
};

/// The end of a member at its vertex: how far its axis runs past the vertex before the cuts, and the cut planes, all at z 0.
struct End {
    double overrun = 0.0; // Length past the vertex the axis is built with.
    std::vector<Plane> planes; // Cut planes, each keeping the side its normal points to.
    bool bearing = false; // True when a column or core wall cuts the end: the member bears there, however short it is.
};

/// A deck side on a plan edge: its outward normal and how far the side moves out from the line.
struct Side {
    Vector outward; // Outward plan normal of the edge.
    double distance = 0.0; // Move out from the line: negative onto a core wall's outer face, positive onto the outer face of the boundary member or column, 0 on an interior edge.
};

/// What a purlin station ends on, and where along the station: a plan edge, a held-back core ring side, or nothing.
struct Support {
    int kind = 0; // 0 nothing, 1 a plan edge, 2 a core ring side.
    std::pair<size_t, size_t> edge; // The plan edge under kind 1.
    Vector along; // Unit direction of the supporting line.
    double t = 0.0; // Distance along the station where it is met.
};

// ═══════════════════════════════════════════════════════════════════════════
// Joint rules
// ═══════════════════════════════════════════════════════════════════════════

/// Rank of a role at a node, highest through: edge girder 8, edge beam 7, girder 6, beam 5, purlin 4, brace 1, none 0; core walls and columns stand above every member.
int compute_rank(int role);

/// Element name of a role.
std::string compute_name(int role);

/// Profile of a role from the framing, the fallbacks of Profiles applied.
std::vector<Polyline> compute_role_profile(int role, const Framing& framing);

/// Profile of the member on a plan edge: its role's, scaled by the edge's width and depth.
std::vector<Polyline> compute_edge_profile(const Mesh& plan, std::pair<size_t, size_t> edge, const Framing& framing);

/// The member on the edge from vertex to other.
Member compute_member(const Context& context, size_t vertex, size_t other);

/// Every member at a vertex, counter-clockwise by direction.
std::vector<Member> compute_members(const Context& context, size_t vertex);

/// Adds a plane unless an equal one is there.
void add_plane(std::vector<Plane>& planes, const Plane& plane);

/// Adds the exit face of a convex plan polygon for the ray from origin along direction, when the ray leaves it.
void add_exit(std::vector<Plane>& planes, const std::vector<Point>& polygon, const Point& origin, const Vector& direction);

/// Plan intersection of two lines given by a point and a direction.
Point compute_meet(const Point& p, const Vector& d, const Point& q, const Vector& e);

/// Cut planes of the member on the edge from vertex to other at its vertex end: its supports, the overhang mitre or far side of every member it covers, then its through or butt end.
End compute_cuts(const Context& context, size_t vertex, size_t other);

/// Lowest member bottom at a vertex relative to the datum, 0 when only the deck arrives: the head top under node 0.
double compute_head_top(const Context& context, size_t vertex);

/// Counter-clockwise corners of a plan face.
std::vector<size_t> compute_loop(const Mesh& plan, size_t face);

/// The deck side on a plan edge: inwards onto the core wall's outer face beside a core face, out onto the outer face of the boundary member or the farthest corner of a column standing at either end on an edge with no floor across it, on the line between two floors.
Side compute_side(const Context& context, std::pair<size_t, size_t> edge);

/// Deck loops of a face at z 0, largest first: every side moved by compute_side, corners by compute_deck_corner, the holes of compute_deck_holes, minus the section of every column rising through the deck.
std::vector<Polyline> compute_outline(const Context& context, size_t face);

/// Loops of the deck of a face cut into equal strips across the deck span, as many as compute_bays lays over its width at panel, so a rest under a foot widens the last strip instead of adding one; one loop set when panel is 0.
std::vector<std::vector<Polyline>> compute_panels(const std::vector<Polyline>& loops, const Vector& span, double panel);

/// The cut plane of a station end on its support: the far side of the widest core wall or member carrying it there, at a core corner the wall; none on an open hole or a stacked girder.
std::vector<Plane> compute_station_cuts(const Context& context, const Support& support, const Point& at, const Vector& inward, const Member& purlin);

} // namespace wood_grid::joints
