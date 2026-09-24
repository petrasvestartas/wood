#include "pch.h"
#include "src/templates/grid_joints.h"

using namespace session_cpp;

namespace wood_grid::joints {

using namespace wood_grid::plan;

// ═══════════════════════════════════════════════════════════════════════════
// Roles
// ═══════════════════════════════════════════════════════════════════════════

int compute_rank(int role) {

    switch (role) {
        case 1: return 6;
        case 2: return 5;
        case 3: return 4;
        case 4: return 8;
        case 5: return 7;
        case 6: return 1;
        default: return 0;
    }
}

std::string compute_name(int role) {

    switch (role) {
        case 1: return "girder";
        case 2: return "beam";
        case 3: return "purlin";
        case 4: return "edge_girder";
        case 5: return "edge_beam";
        case 6: return "brace";
        default: return "member";
    }
}

std::vector<Polyline> compute_role_profile(int role, const Framing& framing) {

    const Profiles& profiles = framing.profiles;
    const std::vector<Polyline>& beam = profiles.beam.empty() ? profiles.girder : profiles.beam;
    const std::vector<Polyline>& purlin = profiles.purlin.empty() ? beam : profiles.purlin;

    switch (role) {
        case 1: return profiles.girder;
        case 2: return beam;
        case 3: return purlin;
        case 4: return profiles.edge_girder.empty() ? profiles.girder : profiles.edge_girder;
        case 5: return profiles.edge_beam.empty() ? (framing.system == 2 ? purlin : beam) : profiles.edge_beam;
        case 6: return profiles.brace.empty() ? beam : profiles.brace;
        default: return {};
    }
}

std::vector<Polyline> compute_edge_profile(const Mesh& plan, std::pair<size_t, size_t> edge, const Framing& framing) {
    return wood_session::compute_scaled(compute_role_profile(static_cast<int>(plan.edge_attribute(edge, "role").value_or(0.0)), framing), plan.edge_attribute(edge, "width").value_or(0.0), plan.edge_attribute(edge, "depth").value_or(0.0));
}

/// Top and bottom of a member of role relative to its datum: girders and edge girders [-drop, -drop - depth], every other member [0, -depth].
std::pair<double, double> compute_range(int role, double depth, double drop) {

    if (role == 1 || role == 4)
        return {-drop, -drop - depth};

    return {0.0, -depth};
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════

Member compute_member(const Context& context, size_t vertex, size_t other) {

    const std::pair<size_t, size_t> edge(vertex, other);
    const std::pair<double, double> size = wood_session::compute_size(compute_edge_profile(context.plan, edge, context.framing));
    const int role = static_cast<int>(context.plan.edge_attribute(edge, "role").value_or(0.0));
    const std::pair<double, double> range = compute_range(role, size.second, context.plan.edge_attribute(edge, "drop").value_or(context.framing.drop));

    return {other, compute_direction(*context.plan.vertex_point(vertex), *context.plan.vertex_point(other)), role, compute_rank(role), size.first, range.first, range.second};
}

/// True when a turns before b counter-clockwise from x.
bool is_earlier(const Member& a, const Member& b) {
    return std::atan2(a.direction[1], a.direction[0]) < std::atan2(b.direction[1], b.direction[0]);
}

std::vector<Member> compute_members(const Context& context, size_t vertex) {

    std::vector<Member> members;
    for (const size_t other : context.plan.vertex_neighbors(vertex).value_or(std::vector<size_t>()))
        if (context.plan.edge_attribute({vertex, other}, "role").value_or(0.0) > 0.0)
            members.push_back(compute_member(context, vertex, other));

    std::sort(members.begin(), members.end(), is_earlier);

    return members;
}

/// Angle of a plan direction from x as a line, in [0, pi).
double compute_angle(const Vector& direction) {

    const double angle = std::atan2(direction[1], direction[0]);

    return angle < 0.0 ? angle + Tolerance::PI : angle;
}

/// True when two plan directions lie on one line.
bool is_parallel(const Vector& a, const Vector& b) {
    return std::abs(a.cross(b)[2]) < 1e-6;
}

/// True when two members share more than tolerance of height.
bool is_sharing_height(const Member& a, const Member& b, double tolerance) {
    return std::min(a.top, b.top) - std::max(a.bottom, b.bottom) > tolerance;
}

/// The member of index that continues straight on within 45 degrees, the straightest; at the same rank only when equal is true; none otherwise.
std::optional<size_t> compute_continuation(const std::vector<Member>& members, size_t index, bool equal = true) {

    std::optional<size_t> straight;
    double best = std::cos(45.0 * Tolerance::TO_RADIANS);
    for (size_t k = 0; k < members.size(); k++) {
        const double dot = -members[k].direction.dot(members[index].direction);
        if (k != index && (!equal || members[k].rank == members[index].rank) && dot > best) {
            best = dot;
            straight = k;
        }
    }

    return straight;
}

/// Order of a member for the through choice: the highest rank, then one with a straight continuation of its rank, then one with any straight continuation, then the smallest angle from x.
std::tuple<int, int, int, double> compute_priority(const std::vector<Member>& members, size_t index) {
    return {-members[index].rank, compute_continuation(members, index) ? 0 : 1, compute_continuation(members, index, false) ? 0 : 1, compute_angle(members[index].direction)};
}

/// The member that runs through a vertex and its continuation of its rank: the highest rank, then one with a straight continuation within 45 degrees, then the smallest angle from x; the vertex attribute through overrides the first.
std::pair<std::optional<size_t>, std::optional<size_t>> compute_through(const std::vector<Member>& members, std::optional<double> forced) {

    std::optional<size_t> through;
    if (forced && *forced >= 0.0 && *forced < static_cast<double>(members.size()))
        through = static_cast<size_t>(*forced);

    for (size_t k = 0; k < members.size() && !forced; k++)
        if (!through || compute_priority(members, k) < compute_priority(members, *through))
            through = k;

    if (!through)
        return {std::nullopt, std::nullopt};

    return {through, compute_continuation(members, *through)};
}

/// True when index is the through member or its continuation.
bool is_through(const std::pair<std::optional<size_t>, std::optional<size_t>>& through, size_t index) {
    return (through.first && index == *through.first) || (through.second && index == *through.second);
}

/// True when member k takes the end of member j at a node: j butts, k of a higher rank or the through member; a through member butts nowhere unless the top rank mitres.
bool is_over(const std::vector<Member>& members, size_t k, size_t j, const std::pair<std::optional<size_t>, std::optional<size_t>>& through, bool mitre) {

    if (is_through(through, j) && !mitre)
        return false;

    return members[k].rank > members[j].rank || (!mitre && is_through(through, k));
}

// ═══════════════════════════════════════════════════════════════════════════
// Cuts
// ═══════════════════════════════════════════════════════════════════════════

void add_plane(std::vector<Plane>& planes, const Plane& plane) {

    for (const Plane& other : planes)
        if (other.z_axis().dot(plane.z_axis()) > 1.0 - 1e-9 && std::abs((plane.origin() - other.origin()).dot(other.z_axis())) < 1e-6)
            return;

    planes.push_back(plane);
}

void add_exit(std::vector<Plane>& planes, const std::vector<Point>& polygon, const Point& origin, const Vector& direction) {

    const std::optional<Plane> plane = compute_exit(polygon, origin, direction);
    if (plane)
        add_plane(planes, *plane);
}

/// Adds the face a member along direction from origin butts into on a column section: the exit face of a convex section, the support plane at the farthest reach of a concave one (a W, a T), so no flange is run through.
void add_column_cut(std::vector<Plane>& planes, const std::vector<Point>& polygon, const Point& origin, const Vector& direction) {

    if (is_convex(polygon))
        add_exit(planes, polygon, origin, direction);
    else
        add_plane(planes, compute_bound(polygon, origin, direction));
}

/// How far the open end of the through member index runs past its vertex, negative outwards: to the farthest end-face corner of the members butting into it, the column's far face under node 0 when nothing butts, the vertex itself when a lower member continues it collinearly.
double compute_open(const Context& context, size_t vertex, const std::vector<Member>& members, size_t index) {

    const Vector direction = members[index].direction;
    const Vector side = direction.cross(Vector(0.0, 0.0, 1.0));
    bool continued = false;
    for (const Member& member : members)
        if (member.direction.dot(direction) < -0.999 && is_sharing_height(members[index], member, context.tolerance))
            continued = true;

    double open = 0.0;
    bool butted = false;
    for (size_t k = 0; k < members.size(); k++) {
        const double speed = members[k].direction.dot(side);
        if (k == index || std::abs(speed) < 1e-6 || !is_sharing_height(members[index], members[k], context.tolerance))
            continue;

        const Vector across = members[k].direction.cross(Vector(0.0, 0.0, 1.0));
        butted = true;
        for (const double sign : {-1.0, 1.0}) {
            const double t = ((speed > 0.0 ? 1.0 : -1.0) * members[index].width / 2.0 - sign * members[k].width / 2.0 * across.dot(side)) / speed;
            open = std::min(open, (members[k].direction * t + across * (sign * members[k].width / 2.0)).dot(direction));
        }
    }

    const Point origin = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    const double column = context.feet.count(vertex) ? compute_reach(context.feet.at(vertex), origin, -direction) : 0.0;

    return context.framing.node == 0 && !continued && !butted ? -column : open;
}

/// The two angular neighbours of member index among the members of its rank, the through pair left out unless every member of the top rank mitres.
std::vector<size_t> compute_neighbours(const std::vector<Member>& members, size_t index, const std::pair<std::optional<size_t>, std::optional<size_t>>& through, bool mitre) {

    std::vector<size_t> equal;
    for (size_t j = 0; j < members.size(); j++)
        if (members[j].rank == members[index].rank && (mitre || !is_through(through, j)))
            equal.push_back(j);

    const size_t at = std::find(equal.begin(), equal.end(), index) - equal.begin();

    return {equal[(at + equal.size() - 1) % equal.size()], equal[(at + 1) % equal.size()]};
}

/// The end of the through member me: the bisector with its partner when it continues, else the perpendicular plane at its open end.
End compute_through_end(const Context& context, size_t vertex, const std::vector<Member>& members, size_t me, const Point& origin, const std::pair<std::optional<size_t>, std::optional<size_t>>& through) {

    End end;
    const std::optional<size_t> partner = me == *through.first ? through.second : through.first;
    if (partner) {
        add_plane(end.planes, compute_bisector(origin, members[me].direction, members[*partner].direction));
        end.overrun = context.framing.reach + 4.0 * members[me].width;
        return end;
    }

    const double open = compute_open(context, vertex, members, me);
    add_plane(end.planes, Plane::from_point_normal(origin + members[me].direction * open, members[me].direction));
    end.overrun = std::max(0.0, -open) + context.framing.reach + members[me].width;

    return end;
}

/// The planes that cut a member at vertex before any other member does: the column face there (nodes 1 and 2) and the outer face of every core wall meeting the vertex that the member crosses within the wall's height; a wall that starts at the face of a column there cuts only a member that reaches past that face.
std::vector<Plane> compute_support_cuts(const Context& context, size_t vertex, const Member& member) {

    const Point origin = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    std::vector<Plane> planes;
    if (context.columns.count(vertex))
        add_column_cut(planes, context.columns.at(vertex), origin, member.direction);

    for (const size_t next : context.plan.vertex_neighbors(vertex).value_or(std::vector<size_t>())) {
        if (context.plan.edge_attribute({vertex, next}, "wall").value_or(0.0) != 2.0)
            continue;

        const Vector along = compute_direction(origin, *context.plan.vertex_point(next));
        const double top = context.plan.edge_attribute({vertex, next}, "role").value_or(0.0) > 0.0 ? compute_member(context, vertex, next).bottom : 0.0;
        const bool short_of = context.feet.count(vertex) && member.direction.dot(along) < 1e-6 && member.width / 2.0 * std::abs(member.direction.cross(along)[2]) <= compute_reach(context.feet.at(vertex), origin, along) + context.tolerance;
        if (!is_parallel(along, member.direction) && !short_of && std::min(member.top, top) - member.bottom > context.tolerance)
            add_exit(planes, compute_strip(origin, along, context.framing.wall / 2.0), origin, member.direction);
    }

    return planes;
}

/// Distance from the vertex along a member's direction to where it begins after compute_support_cuts: the farthest plane crossed, minus infinity when nothing cuts it.
double compute_run(const Context& context, size_t vertex, const Member& member) {

    const Point origin = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    double run = -std::numeric_limits<double>::max();
    for (const Plane& plane : compute_support_cuts(context, vertex, member)) {
        const double speed = plane.z_axis().dot(member.direction);
        if (speed > 1e-9)
            run = std::max(run, (plane.origin() - origin).dot(plane.z_axis()) / speed);
    }

    return run;
}

/// True when member k still covers the whole end of member me once the column and the core walls at vertex have cut k back: the end face lands wholly where k runs past its cuts, on k's side or its continuation's; always when nothing cuts k.
bool is_covered(const Context& context, size_t vertex, const std::vector<Member>& members, size_t me, size_t k, const Point& origin) {

    const std::optional<Plane> exit = compute_exit(compute_strip(origin, members[k].direction, members[k].width / 2.0), origin, members[me].direction);
    if (!exit)
        return true;

    const double at = (exit->origin() - origin).dot(members[k].direction);
    const double half = members[me].width / 2.0 * std::abs(members[me].direction.cross(Vector(0.0, 0.0, 1.0)).dot(members[k].direction));
    if (at - half >= compute_run(context, vertex, members[k]) - context.tolerance)
        return true;

    const std::optional<size_t> partner = compute_continuation(members, k);

    return partner && at + half <= -compute_run(context, vertex, members[*partner]) + context.tolerance;
}

Point compute_meet(const Point& p, const Vector& d, const Point& q, const Vector& e) {

    const double denominator = d.cross(e)[2];
    if (std::abs(denominator) < 1e-9)
        return p;

    return p + d * ((q - p).cross(e)[2] / denominator);
}

/// The plane that mitres the corner where members me and k both overhang their supports past each other's end, through the meeting of their end faces and of their far sides, keeping me's side; none unless both meetings fall where a near-square corner puts them.
std::optional<Plane> compute_overhang(const Context& context, size_t vertex, const std::vector<Member>& members, size_t me, size_t k, const Point& origin) {

    const Vector up(0.0, 0.0, 1.0);
    const double run_me = compute_run(context, vertex, members[me]);
    const double run_k = compute_run(context, vertex, members[k]);
    const std::optional<Plane> side_k = compute_exit(compute_strip(origin, members[k].direction, members[k].width / 2.0), origin, members[me].direction);
    const std::optional<Plane> side_me = compute_exit(compute_strip(origin, members[me].direction, members[me].width / 2.0), origin, members[k].direction);
    if (!side_k || !side_me || run_me < -1e300 || run_k < -1e300)
        return std::nullopt;

    const Point ends = compute_meet(origin + members[me].direction * run_me, up.cross(members[me].direction), origin + members[k].direction * run_k, up.cross(members[k].direction));
    const Point sides = compute_meet(side_me->origin(), members[me].direction, side_k->origin(), members[k].direction);
    const double tolerance = context.tolerance;
    if (std::abs((ends - origin).dot(up.cross(members[me].direction))) > members[me].width / 2.0 + tolerance || std::abs((ends - origin).dot(up.cross(members[k].direction))) > members[k].width / 2.0 + tolerance)
        return std::nullopt;
    if ((sides - origin).dot(members[me].direction) <= run_me + tolerance || (sides - origin).dot(members[k].direction) <= run_k + tolerance)
        return std::nullopt;

    Vector normal = up.cross(sides - ends).normalized();
    if (normal.dot(members[me].direction) < 0.0)
        normal = -normal;

    return Plane::from_point_normal(ends, normal);
}

/// The end of a butting member me: the exit faces of every member over it that it shares height with where they still cover it, the overhang mitre where a support keeps them from covering it, the open end of a collinear through member, then the bisectors with its angular neighbours of equal rank.
void compute_butt_end(End& end, const Context& context, size_t vertex, const std::vector<Member>& members, size_t me, const Point& origin, const std::pair<std::optional<size_t>, std::optional<size_t>>& through, bool mitre) {

    const Vector direction = members[me].direction;
    for (size_t k = 0; k < members.size(); k++) {
        if (k == me || !is_sharing_height(members[me], members[k], context.tolerance) || !is_over(members, k, me, through, mitre))
            continue;

        if (is_parallel(members[k].direction, direction) && members[k].direction.dot(direction) < 0.0 && is_through(through, k))
            add_plane(end.planes, Plane::from_point_normal(origin + members[k].direction * compute_open(context, vertex, members, k), direction));
        else if (!is_parallel(members[k].direction, direction) && is_covered(context, vertex, members, me, k, origin))
            add_exit(end.planes, compute_strip(origin, members[k].direction, members[k].width / 2.0), origin, direction);
        else if (!is_parallel(members[k].direction, direction)) {
            const std::optional<Plane> corner = compute_overhang(context, vertex, members, me, k, origin);
            if (corner)
                add_plane(end.planes, *corner);
        }
    }

    for (const size_t k : compute_neighbours(members, me, through, mitre))
        if (k != me && !is_parallel(members[k].direction, direction) && is_sharing_height(members[me], members[k], context.tolerance))
            add_plane(end.planes, compute_bisector(origin, direction, members[k].direction));

    end.overrun = end.planes.empty() ? 0.0 : context.framing.reach + 4.0 * members[me].width;
}

End compute_cuts(const Context& context, size_t vertex, size_t other) {

    const Point origin = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    const std::vector<Member> members = compute_members(context, vertex);
    size_t me = 0;
    for (size_t k = 0; k < members.size(); k++)
        if (members[k].other == other)
            me = k;
    const Vector direction = members[me].direction;

    End end;
    end.planes = compute_support_cuts(context, vertex, members[me]);
    end.bearing = !end.planes.empty();

    const std::pair<std::optional<size_t>, std::optional<size_t>> through = compute_through(members, context.plan.vertex_attribute(vertex, "through"));
    int top_count = 0;
    for (const Member& member : members)
        if (through.first && member.rank == members[*through.first].rank)
            top_count++;
    const bool mitre = through.first && top_count > 1 && !compute_continuation(members, *through.first, false);

    for (size_t k = 0; k < members.size(); k++) {
        if (k == me || is_parallel(members[k].direction, direction) || !is_sharing_height(members[me], members[k], context.tolerance) || !is_over(members, me, k, through, mitre) || is_covered(context, vertex, members, k, me, origin))
            continue;

        const std::optional<Plane> corner = compute_overhang(context, vertex, members, me, k, origin);
        if (corner)
            add_plane(end.planes, *corner);
        else
            add_exit(end.planes, compute_strip(origin, members[k].direction, members[k].width / 2.0), origin, direction);
    }

    if (is_through(through, me) && !mitre) {
        const End own = compute_through_end(context, vertex, members, me, origin, through);
        for (const Plane& plane : own.planes)
            add_plane(end.planes, plane);
        end.overrun = own.overrun;
        return end;
    }

    compute_butt_end(end, context, vertex, members, me, origin, through, mitre);

    return end;
}

double compute_head_top(const Context& context, size_t vertex) {

    double top = 0.0;
    for (const Member& member : compute_members(context, vertex))
        top = std::min(top, member.bottom);

    return top;
}

// ═══════════════════════════════════════════════════════════════════════════
// Outlines
// ═══════════════════════════════════════════════════════════════════════════

std::vector<size_t> compute_loop(const Mesh& plan, size_t face) {

    std::vector<size_t> loop = *plan.face_vertices(face);
    if (compute_area(compute_flat(plan, loop)) < 0.0)
        std::reverse(loop.begin(), loop.end());

    return loop;
}

/// True when loop a encloses more area than b.
bool is_larger(const Polyline& a, const Polyline& b) {
    return compute_area(to_loop(a)) > compute_area(to_loop(b));
}

/// Outward plan normal of the plan edge from a to b.
Vector compute_outward(const Mesh& plan, std::pair<size_t, size_t> edge) {
    return compute_direction(*plan.vertex_point(edge.first), *plan.vertex_point(edge.second)).cross(Vector(0.0, 0.0, 1.0));
}

Side compute_side(const Context& context, std::pair<size_t, size_t> edge) {

    const Vector outward = compute_outward(context.plan, edge);
    size_t floors = 0;
    for (const size_t face : context.plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>())) {
        if (context.plan.face_attribute(face, "core").value_or(0.0) == 1.0)
            return {outward, -context.framing.wall / 2.0};
        if (context.plan.face_attribute(face, "floor").value_or(0.0) == 1.0)
            floors++;
    }
    if (floors > 1)
        return {outward, 0.0};

    double distance = wood_session::compute_size(compute_edge_profile(context.plan, edge, context.framing)).first / 2.0;
    for (const size_t key : {edge.first, edge.second})
        if (context.feet.count(key))
            distance = std::max(distance, compute_reach(context.feet.at(key), *context.plan.vertex_point(key), outward));

    return {outward, distance};
}

/// The pushed deck side of another floor face at a vertex: a push across the given normal when crossing is true, else one along it; none when no other deck pushes so.
std::optional<Side> compute_partner(const Context& context, size_t vertex, size_t face, const Vector& normal, bool crossing) {

    for (const size_t other : context.plan.vertex_faces(vertex).value_or(std::vector<size_t>())) {
        if (other == face || context.plan.face_attribute(other, "floor").value_or(0.0) != 1.0)
            continue;

        const std::vector<size_t> loop = compute_loop(context.plan, other);
        const size_t count = loop.size();
        const size_t at = std::find(loop.begin(), loop.end(), vertex) - loop.begin();
        for (const std::pair<size_t, size_t>& edge : {std::make_pair(loop[(at + count - 1) % count], vertex), std::make_pair(vertex, loop[(at + 1) % count])}) {
            const Side side = compute_side(context, edge);
            if (side.distance > 0.0 && is_parallel(side.outward, normal) != crossing)
                return side;
        }
    }

    return std::nullopt;
}

/// True when a core face meets a plan vertex: a pinwheel wall end stands in the quadrant across from the core there.
bool is_core_corner(const Mesh& plan, size_t vertex) {

    for (const size_t face : plan.vertex_faces(vertex).value_or(std::vector<size_t>()))
        if (plan.face_attribute(face, "core").value_or(0.0) == 1.0)
            return true;

    return false;
}

/// The corner of a deck outline at a loop vertex between its sides before and after: a pinwheel notch at a core corner, a mitre when both or neither side move, else the corner of the overlap with the deck pushing across it, or a step between the two side lines when they are parallel or another deck pushes along one.
std::vector<Point> compute_deck_corner(const Context& context, size_t face, size_t vertex, const Side& before, const Side& after) {

    const Point point = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    const double half = context.framing.wall / 2.0;
    if (before.distance == 0.0 && after.distance == 0.0 && is_core_corner(context.plan, vertex))
        return {point - after.outward * half, point - after.outward * half - before.outward * half, point - before.outward * half};
    if (before.distance == after.distance || (before.distance > 0.0 && after.distance > 0.0))
        return {compute_corner(point, before.outward, before.distance, after.outward, after.distance)};

    const Vector up(0.0, 0.0, 1.0);
    if (before.distance < 0.0 && after.distance == 0.0 && is_parallel(before.outward, after.outward))
        return {point + before.outward * before.distance - up.cross(after.outward) * before.distance, point - up.cross(after.outward) * before.distance};
    if (before.distance == 0.0 && after.distance < 0.0 && is_parallel(before.outward, after.outward))
        return {point + up.cross(before.outward) * after.distance, point + up.cross(before.outward) * after.distance + after.outward * after.distance};

    const bool first = before.distance > after.distance;
    const Side& pushed = first ? before : after;
    const Side& other = first ? after : before;
    const std::optional<Side> partner = compute_partner(context, vertex, face, pushed.outward, true);
    if (partner) {
        const Point far = compute_corner(point, pushed.outward, pushed.distance, partner->outward, partner->distance);
        const Point near = point + other.outward * other.distance;
        return first ? std::vector<Point>{far, near} : std::vector<Point>{near, far};
    }

    if (is_parallel(before.outward, after.outward) || compute_partner(context, vertex, face, pushed.outward, false))
        return {point + before.outward * before.distance, point + after.outward * after.distance};

    return {compute_corner(point, before.outward, before.distance, after.outward, after.distance)};
}

/// The deck holes of a face at z 0, clockwise: a core ring moved out onto its walls' outer faces, an open hole ring as drawn.
std::vector<Polyline> compute_deck_holes(const Mesh& plan, size_t face, const Framing& framing) {

    std::vector<Polyline> holes;
    if (!plan.get_face_holes().count(face))
        return holes;

    for (const std::vector<size_t>& ring : plan.get_face_holes().at(face)) {
        std::vector<Point> hole = compute_flat(plan, ring);
        if (compute_area(hole) < 0.0)
            std::reverse(hole.begin(), hole.end());
        if (plan.vertex_attribute(ring[0], "wall").value_or(0.0) == 2.0)
            hole = compute_offset(hole, std::vector<double>(hole.size(), framing.wall / 2.0));
        std::reverse(hole.begin(), hole.end());
        holes.push_back(to_polyline(hole));
    }

    return holes;
}

/// Deck loops minus the column notches: the largest outer ring of the difference with the holes inside it; a fragment a notch cuts off at a corner (the sliver past a column at a mitred deck corner) is dropped, and the loops stay as they were when nothing is left.
std::vector<Polyline> compute_notched(const std::vector<Polyline>& loops, const std::vector<Polyline>& notches) {

    if (notches.empty())
        return loops;

    std::vector<Polyline> cut = compute_regions(loops, notches, 2);
    std::stable_sort(cut.begin(), cut.end(), is_larger);
    if (cut.empty() || compute_area(to_loop(cut[0])) <= 0.0)
        return loops;

    std::vector<Polyline> kept = {cut[0]};
    for (size_t k = 1; k < cut.size(); k++)
        if (compute_area(to_loop(cut[k])) < 0.0 && cut[0].point_in_polygon_2d(to_loop(cut[k])[0]))
            kept.push_back(cut[k]);

    return kept;
}

std::vector<Polyline> compute_outline(const Context& context, size_t face) {

    const std::vector<size_t> loop = compute_loop(context.plan, face);
    const size_t count = loop.size();
    std::vector<Side> sides;
    for (size_t i = 0; i < count; i++)
        sides.push_back(compute_side(context, {loop[i], loop[(i + 1) % count]}));

    std::vector<Point> outline;
    for (size_t i = 0; i < count; i++)
        for (const Point& corner : compute_deck_corner(context, face, loop[i], sides[(i + count - 1) % count], sides[i]))
            outline.push_back(corner);

    std::vector<Polyline> loops = {to_polyline(outline)};
    for (const Polyline& hole : compute_deck_holes(context.plan, face, context.framing))
        loops.push_back(hole);

    std::vector<Polyline> notches;
    for (const size_t key : loop)
        if (context.through.count(key))
            notches.push_back(to_polyline(context.through.at(key)));
    loops = compute_notched(loops, notches);

    std::stable_sort(loops.begin(), loops.end(), is_larger);

    return loops;
}

std::vector<std::vector<Polyline>> compute_panels(const std::vector<Polyline>& loops, const Vector& span, double panel) {

    if (panel <= 0.0 || loops.empty())
        return {loops};

    const Vector across = Vector(0.0, 0.0, 1.0).cross(span);
    const Point origin(0.0, 0.0, 0.0);
    double low = std::numeric_limits<double>::max();
    double high = -low;
    for (const Point& point : to_loop(loops[0])) {
        low = std::min(low, (point - origin).dot(across));
        high = std::max(high, (point - origin).dot(across));
    }

    const int strips = std::max(1, static_cast<int>(compute_bays(high - low, panel).size()));
    const double width = (high - low) / strips;
    std::vector<std::vector<Polyline>> panels;
    for (int k = 0; k < strips; k++) {
        const Point centre = origin + across * (low + (k + 0.5) * width);
        const std::vector<Point> band = {centre - span * 1e7 - across * (width / 2.0), centre + span * 1e7 - across * (width / 2.0), centre + span * 1e7 + across * (width / 2.0), centre - span * 1e7 + across * (width / 2.0)};
        std::vector<Polyline> strip = compute_regions(loops, {to_polyline(band)}, 0);
        std::stable_sort(strip.begin(), strip.end(), is_larger);
        if (!strip.empty())
            panels.push_back(strip);
    }

    return panels;
}

// ═══════════════════════════════════════════════════════════════════════════
// Purlin ends
// ═══════════════════════════════════════════════════════════════════════════

/// Half the width of what stands on a plan edge for a station end: the wall on a core edge, the member when the heights overlap, nothing otherwise.
double compute_station_half(const Context& context, std::pair<size_t, size_t> edge, const Member& purlin) {

    if (context.plan.edge_attribute(edge, "wall").value_or(0.0) == 2.0)
        return context.framing.wall / 2.0;
    if (context.plan.edge_attribute(edge, "role").value_or(0.0) <= 0.0)
        return 0.0;

    return is_sharing_height(purlin, compute_member(context, edge.first, edge.second), context.tolerance) ? compute_member(context, edge.first, edge.second).width / 2.0 : 0.0;
}

/// True when a core wall edge meets a plan vertex.
bool is_core_vertex(const Mesh& plan, size_t vertex) {

    for (const size_t other : plan.vertex_neighbors(vertex).value_or(std::vector<size_t>()))
        if (plan.edge_attribute({vertex, other}, "wall").value_or(0.0) == 2.0)
            return true;

    return false;
}

/// The plan vertex of a support edge a station end sits on within tolerance, none mid-edge.
std::optional<size_t> compute_corner_vertex(const Context& context, const Support& support, const Point& at) {

    for (const size_t vertex : {support.edge.first, support.edge.second})
        if (compute_distance(compute_lift(*context.plan.vertex_point(vertex), 0.0), compute_lift(at, 0.0)) <= context.tolerance)
            return vertex;

    return std::nullopt;
}

std::vector<Plane> compute_station_cuts(const Context& context, const Support& support, const Point& at, const Vector& inward, const Member& purlin) {

    double half = support.kind == 2 ? context.framing.wall / 2.0 : 0.0;
    const std::optional<size_t> vertex = support.kind == 1 ? compute_corner_vertex(context, support, at) : std::nullopt;
    if (support.kind == 1 && !vertex)
        half = compute_station_half(context, support.edge, purlin);

    if (vertex) {
        const bool core = is_core_vertex(context.plan, *vertex);
        if (core)
            half = context.framing.wall / 2.0;
        if (!core || purlin.width / 2.0 > context.framing.wall / 2.0 + context.tolerance)
            for (const size_t other : context.plan.vertex_neighbors(*vertex).value_or(std::vector<size_t>()))
                if (std::abs(compute_direction(*context.plan.vertex_point(*vertex), *context.plan.vertex_point(other)).dot(support.along)) > 0.999)
                    half = std::max(half, compute_station_half(context, {*vertex, other}, purlin));
    }

    std::vector<Plane> planes;
    if (half > 0.0)
        add_exit(planes, compute_strip(at, support.along, half), at, inward);

    return planes;
}

} // namespace wood_grid::joints
