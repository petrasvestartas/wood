#include "pch.h"
#include "src/templates/grid/grid_plan.h"

using namespace session_cpp;

namespace wood_grid::build {

using namespace wood_grid::plan;

// ═══════════════════════════════════════════════════════════════════════════
// Roles
// ═══════════════════════════════════════════════════════════════════════════

/// The role of a plan edge from the framing and its family: girders on the span family, beams on free lines and under span -1, purlins on the cross lines under system 2, beams on perimeter cross lines under system 1, nothing under system 0.
int compute_role(const Mesh& plan, std::pair<size_t, size_t> edge, const Framing& framing) {

    const int family = static_cast<int>(plan.edge_attribute(edge, "family").value_or(-1.0));
    int system = framing.system;
    int span = framing.span;
    for (const size_t face : plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>())) {
        system = static_cast<int>(plan.face_attribute(face, "system").value_or(system));
        span = static_cast<int>(plan.face_attribute(face, "span").value_or(span));
    }

    if (system == 0)
        return 0;
    if (span < 0 || family < 0)
        return 2;
    if (family == span)
        return 1;
    if (system == 2)
        return 3;

    return plan.edge_attribute(edge, "boundary").value_or(0.0) == 1.0 ? 2 : 0;
}

void compute_roles(Mesh& plan, const Framing& framing) {

    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        if (framing.facade && plan.edge_attribute(edge, "boundary").value_or(0.0) == 1.0 && !plan.edge_attribute(edge, "wall"))
            plan.set_edge_attribute(edge, "wall", 1.0);
        if (!plan.edge_attribute(edge, "role"))
            plan.set_edge_attribute(edge, "role", compute_role(plan, edge, framing));
    }
}

std::string compute_name(int role) {

    switch (role) {
        case 1: return "girder";
        case 2: return "beam";
        case 3: return "purlin";
        default: return "brace";
    }
}

std::vector<Polyline> compute_profile(int role, const Framing& framing) {

    const Profiles& profiles = framing.profiles;
    const std::vector<Polyline>& beam = profiles.beam.empty() ? profiles.girder : profiles.beam;

    switch (role) {
        case 1: return profiles.girder;
        case 2: return beam;
        case 3: return profiles.purlin.empty() ? beam : profiles.purlin;
        default: return profiles.brace.empty() ? beam : profiles.brace;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════

Member compute_member(const Context& context, size_t vertex, size_t other) {

    const std::pair<size_t, size_t> edge(vertex, other);
    const int role = static_cast<int>(context.plan.edge_attribute(edge, "role").value_or(0.0));
    const std::pair<double, double> size = wood_session::compute_size(compute_profile(role, context.framing));
    const double drop = role == 1 ? context.framing.drop : 0.0;
    const int rank = context.plan.edge_attribute(edge, "boundary").value_or(0.0) == 1.0 ? 7 : role == 1 ? 6 : role == 2 ? 5 : role == 3 ? 4 : 1;

    return {other, compute_direction(*context.plan.vertex_point(vertex), *context.plan.vertex_point(other)), role, rank, size.first, -drop, -drop - size.second};
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

/// True when two plan directions lie on one line.
bool is_parallel(const Vector& a, const Vector& b) {
    return std::abs(a.cross(b)[2]) < 1e-6;
}

/// True when two members share more than tolerance of height.
bool is_sharing_height(const Member& a, const Member& b, double tolerance) {
    return std::min(a.top, b.top) - std::max(a.bottom, b.bottom) > tolerance;
}

/// The member that continues index straight on within 45 degrees, the straightest; of the same rank only when equal is true; none otherwise.
std::optional<size_t> compute_continuation(const std::vector<Member>& members, size_t index, bool equal) {

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

/// Order of a member for the through choice: the highest rank, then one with a straight continuation of its rank, then one with any straight continuation, then the smallest angle from x as a line.
std::tuple<int, int, int, double> compute_priority(const std::vector<Member>& members, size_t index) {

    const double angle = std::atan2(members[index].direction[1], members[index].direction[0]);

    return {-members[index].rank, compute_continuation(members, index, true) ? 0 : 1, compute_continuation(members, index, false) ? 0 : 1, angle < 0.0 ? angle + Tolerance::PI : angle};
}

/// The member that runs through a vertex and its continuation of its rank.
std::pair<std::optional<size_t>, std::optional<size_t>> compute_through(const std::vector<Member>& members) {

    std::optional<size_t> through;
    for (size_t k = 0; k < members.size(); k++)
        if (!through || compute_priority(members, k) < compute_priority(members, *through))
            through = k;
    if (!through)
        return {std::nullopt, std::nullopt};

    return {through, compute_continuation(members, *through, true)};
}

/// True when index is the through member or its continuation.
bool is_through(const std::pair<std::optional<size_t>, std::optional<size_t>>& through, size_t index) {
    return (through.first && index == *through.first) || (through.second && index == *through.second);
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

/// How far the open end of the through member index runs past its vertex, negative outwards: to the farthest end-face corner of the members butting into it, the column's far face under node 0 when nothing butts, the vertex itself when a lower member continues it collinearly.
double compute_open(const Context& context, size_t vertex, const std::vector<Member>& members, size_t index) {

    const Vector direction = members[index].direction;
    const Vector side = direction.cross(Vector(0.0, 0.0, 1.0));
    bool continued = false;
    bool butted = false;
    double open = 0.0;
    for (size_t k = 0; k < members.size(); k++) {
        const double speed = members[k].direction.dot(side);
        if (k == index || !is_sharing_height(members[index], members[k], context.tolerance))
            continue;
        if (members[k].direction.dot(direction) < -0.999)
            continued = true;
        if (std::abs(speed) < 1e-6)
            continue;

        const Vector across = members[k].direction.cross(Vector(0.0, 0.0, 1.0));
        butted = true;
        for (const double sign : {-1.0, 1.0}) {
            const double t = ((speed > 0.0 ? 1.0 : -1.0) * members[index].width / 2.0 - sign * members[k].width / 2.0 * across.dot(side)) / speed;
            open = std::min(open, (members[k].direction * t + across * (sign * members[k].width / 2.0)).dot(direction));
        }
    }

    const double column = compute_column_reach(context, vertex, -direction);

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

End compute_cuts(const Context& context, size_t vertex, size_t other) {

    const Point origin = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    const std::vector<Member> members = compute_members(context, vertex);
    size_t me = 0;
    for (size_t k = 0; k < members.size(); k++)
        if (members[k].other == other)
            me = k;
    const Vector direction = members[me].direction;

    End end;
    if (context.framing.node != 0 && context.standing.count(vertex))
        add_exit(end.planes, context.standing.at(vertex), origin, direction);
    end.bearing = !end.planes.empty();

    const std::pair<std::optional<size_t>, std::optional<size_t>> through = compute_through(members);
    int top = 0;
    for (const Member& member : members)
        top += member.rank == members[*through.first].rank ? 1 : 0;
    const bool mitre = top > 1 && !compute_continuation(members, *through.first, false);

    if (is_through(through, me) && !mitre) {
        const std::optional<size_t> partner = me == *through.first ? through.second : through.first;
        const double open = partner ? 0.0 : compute_open(context, vertex, members, me);
        add_plane(end.planes, partner ? compute_bisector(origin, direction, members[*partner].direction) : Plane::from_point_normal(origin + direction * open, direction));
        end.overrun = std::max(0.0, -open) + context.framing.reach + 4.0 * members[me].width;
        return end;
    }

    for (size_t k = 0; k < members.size(); k++) {
        const bool over = members[k].rank > members[me].rank || (!mitre && is_through(through, k));
        if (k == me || !over || !is_sharing_height(members[me], members[k], context.tolerance))
            continue;

        if (is_parallel(members[k].direction, direction) && members[k].direction.dot(direction) < 0.0 && is_through(through, k))
            add_plane(end.planes, Plane::from_point_normal(origin + members[k].direction * compute_open(context, vertex, members, k), direction));
        else if (!is_parallel(members[k].direction, direction))
            add_exit(end.planes, compute_strip(origin, members[k].direction, members[k].width / 2.0), origin, direction);
    }

    for (const size_t k : compute_neighbours(members, me, through, mitre))
        if (k != me && !is_parallel(members[k].direction, direction) && is_sharing_height(members[me], members[k], context.tolerance))
            add_plane(end.planes, compute_bisector(origin, direction, members[k].direction));
    end.overrun = end.planes.empty() ? 0.0 : context.framing.reach + 4.0 * members[me].width;

    return end;
}

double compute_head_top(const Context& context, size_t vertex) {

    double top = 0.0;
    for (const Member& member : compute_members(context, vertex))
        top = std::min(top, member.bottom);

    return top;
}

double compute_column_reach(const Context& context, size_t vertex, const Vector& direction) {

    double reach = 0.0;
    for (const std::map<size_t, std::vector<Point>>* columns : {&context.standing, &context.rising})
        if (columns->count(vertex))
            reach = std::max(reach, compute_reach(columns->at(vertex), *context.plan.vertex_point(vertex), direction));

    return reach;
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

std::vector<std::pair<size_t, size_t>> compute_sides(const std::vector<size_t>& loop) {

    std::vector<std::pair<size_t, size_t>> sides;
    for (size_t i = 0; i < loop.size(); i++)
        sides.emplace_back(loop[i], loop[(i + 1) % loop.size()]);

    return sides;
}

double compute_side(const Context& context, std::pair<size_t, size_t> edge) {

    if (context.plan.edge_attribute(edge, "boundary").value_or(0.0) != 1.0)
        return 0.0;

    const Vector outward = compute_direction(*context.plan.vertex_point(edge.first), *context.plan.vertex_point(edge.second)).cross(Vector(0.0, 0.0, 1.0));
    const int role = static_cast<int>(context.plan.edge_attribute(edge, "role").value_or(0.0));
    double distance = role > 0 ? wood_session::compute_size(compute_profile(role, context.framing)).first / 2.0 : 0.0;
    for (const size_t vertex : {edge.first, edge.second})
        distance = std::max(distance, compute_column_reach(context, vertex, outward));

    return distance;
}

/// True when loop a encloses more area than b.
bool is_larger(const Polyline& a, const Polyline& b) {
    return compute_area(to_loop(a)) > compute_area(to_loop(b));
}

/// Plan intersection of two lines given by a point and a direction.
Point compute_meet(const Point& p, const Vector& d, const Point& q, const Vector& e) {

    const double denominator = d.cross(e)[2];

    return std::abs(denominator) < 1e-9 ? p : p + d * ((q - p).cross(e)[2] / denominator);
}

std::vector<std::vector<Point>> compute_core_quads(const Polyline& ring, double wall) {

    const std::vector<Point> corners = to_loop(ring);
    const size_t count = corners.size();
    std::vector<Vector> normals;
    for (size_t i = 0; i < count; i++)
        normals.push_back(compute_direction(corners[i], corners[(i + 1) % count]).cross(Vector(0.0, 0.0, 1.0)));

    std::vector<std::vector<Point>> quads;
    for (size_t i = 0; i < count; i++) {
        const size_t before = (i + count - 1) % count;
        const size_t after = (i + 1) % count;
        const Vector direction = compute_direction(corners[i], corners[after]);
        const Vector previous = compute_direction(corners[before], corners[i]);
        const Vector next = compute_direction(corners[after], corners[(i + 2) % count]);
        const Point inner = corners[i] - normals[before] * (wall / 2.0);
        const Point outer = corners[after] + normals[after] * (wall / 2.0);
        quads.push_back({
            compute_meet(corners[i] + normals[i] * (wall / 2.0), direction, inner, previous),
            compute_meet(corners[i] + normals[i] * (wall / 2.0), direction, outer, next),
            compute_meet(corners[i] - normals[i] * (wall / 2.0), direction, outer, next),
            compute_meet(corners[i] - normals[i] * (wall / 2.0), direction, inner, previous)
        });
    }

    return quads;
}

/// The largest counter-clockwise ring of a boolean result with the clockwise rings inside it; empty when nothing is left.
std::vector<Polyline> compute_largest(std::vector<Polyline> rings) {

    std::stable_sort(rings.begin(), rings.end(), is_larger);
    if (rings.empty() || compute_area(to_loop(rings[0])) <= 0.0)
        return {};

    std::vector<Polyline> kept = {rings[0]};
    for (size_t k = 1; k < rings.size(); k++)
        if (compute_area(to_loop(rings[k])) < 0.0 && rings[0].point_in_polygon_2d(to_loop(rings[k])[0]))
            kept.push_back(rings[k]);

    return kept;
}

std::map<size_t, std::vector<Polyline>> compute_outlines(const Context& context, const std::vector<Polyline>& cores) {

    std::vector<Polyline> cutters;
    for (const Polyline& core : cores)
        cutters.push_back(compute_wall_ring(core, context.framing.wall));

    std::map<size_t, std::vector<Polyline>> outlines;
    for (const size_t face : context.plan.faces()) {
        if (context.plan.face_attribute(face, "floor").value_or(0.0) != 1.0)
            continue;

        const std::vector<size_t> loop = compute_loop(context.plan, face);
        std::vector<double> distances;
        std::vector<Polyline> notches = cutters;
        for (const std::pair<size_t, size_t>& side : compute_sides(loop)) {
            distances.push_back(compute_side(context, side));
            if (context.framing.node == 2 && context.rising.count(side.first))
                notches.push_back(to_polyline(context.rising.at(side.first)));
        }

        const std::vector<Polyline> loops = compute_largest(BooleanPolyline::compute_regions({to_polyline(compute_flat(context.plan, loop)).offset_sides(distances)}, notches, 2));
        if (loops.empty())
            continue;

        cutters.insert(cutters.end(), loops.begin(), loops.end());
        outlines[face] = loops;
    }

    return outlines;
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
        const std::vector<Polyline> strip = compute_largest(BooleanPolyline::compute_regions(loops, {to_polyline(band)}, 0));
        if (!strip.empty())
            panels.push_back(strip);
    }

    return panels;
}

// ═══════════════════════════════════════════════════════════════════════════
// Stations
// ═══════════════════════════════════════════════════════════════════════════

std::optional<Vector> compute_family_direction(const Mesh& plan, const std::vector<std::pair<size_t, size_t>>& edges, int family) {

    Vector sum(0.0, 0.0, 0.0);
    for (const std::pair<size_t, size_t>& edge : edges) {
        if (static_cast<int>(plan.edge_attribute(edge, "family").value_or(-1.0)) != family)
            continue;

        const Vector direction = compute_direction(*plan.vertex_point(edge.first), *plan.vertex_point(edge.second));
        const double doubled = 2.0 * std::atan2(direction[1], direction[0]);
        sum += Vector(std::cos(doubled), std::sin(doubled), 0.0);
    }
    if (sum.magnitude() < 1e-9)
        return std::nullopt;

    const double angle = std::atan2(sum[1], sum[0]) / 2.0;

    return Vector(std::cos(angle), std::sin(angle), 0.0);
}

Plane compute_wall_face(const std::vector<Polyline>& outer, int ring, int side, const Point& at) {

    const std::vector<Point> corners = to_loop(outer[ring]);

    return Plane::from_point_normal(at, compute_direction(corners[side], corners[(side + 1) % corners.size()]).cross(Vector(0.0, 0.0, 1.0)));
}

/// The cut plane of a station end at the side of a face loop it lands on: the face of the member there when the heights overlap, none otherwise.
void add_station_cut(std::vector<Plane>& cuts, const Context& context, const std::vector<size_t>& loop, int side, const Point& at, const Vector& inward, const Member& purlin) {

    const std::pair<size_t, size_t> edge(loop[side], loop[(side + 1) % loop.size()]);
    if (context.plan.edge_attribute(edge, "role").value_or(0.0) <= 0.0)
        return;

    const Member member = compute_member(context, edge.first, edge.second);
    if (is_sharing_height(purlin, member, context.tolerance))
        add_exit(cuts, compute_strip(at, member.direction, member.width / 2.0), at, inward);
}

std::vector<Station> compute_stations(const Context& context, size_t face, const std::vector<Polyline>& cores) {

    const Mesh& plan = context.plan;
    const std::vector<size_t> loop = compute_loop(plan, face);
    const int span = static_cast<int>(plan.face_attribute(face, "span").value_or(context.framing.span));
    const std::optional<Vector> girder = compute_family_direction(plan, compute_sides(loop), span);
    if (!girder)
        return {};

    std::optional<Vector> station;
    for (int family = 0; family < 3 && !station; family++)
        if (family != span)
            station = compute_family_direction(plan, compute_sides(loop), family);
    const Vector along = station.value_or(Vector(0.0, 0.0, 1.0).cross(*girder));
    const Vector across = along.cross(Vector(0.0, 0.0, 1.0));
    double low = std::numeric_limits<double>::max();
    double high = -low;
    double first = low;
    double last = -low;
    for (const Point& corner : compute_flat(plan, loop)) {
        low = std::min(low, (corner - Point(0.0, 0.0, 0.0)).dot(across));
        high = std::max(high, (corner - Point(0.0, 0.0, 0.0)).dot(across));
        first = std::min(first, (corner - Point(0.0, 0.0, 0.0)).dot(along));
        last = std::max(last, (corner - Point(0.0, 0.0, 0.0)).dot(along));
    }

    std::vector<Polyline> outer;
    for (const Polyline& core : cores)
        outer.push_back(compute_wall_ring(core, context.framing.wall));
    const std::vector<Polyline> bay = {to_polyline(compute_flat(plan, loop))};
    const std::pair<double, double> size = wood_session::compute_size(compute_profile(3, context.framing));
    const Member purlin{0, along, 3, 4, size.first, 0.0, -size.second};
    const int intervals = std::max(1, static_cast<int>(std::ceil((high - low) / context.framing.spacing - 1e-9)));

    std::vector<Station> stations;
    for (int k = 1; k < intervals; k++) {
        const Point base = Point(0.0, 0.0, 0.0) + across * (low + (high - low) * k / intervals);
        for (const Piece& piece : compute_pieces(Line::from_points(base + along * (first - 1.0), base + along * (last + 1.0)), bay, true, context.tolerance)) {
            std::vector<Plane> cuts;
            for (const size_t e : {0, 1})
                if (piece.side[e] >= 0)
                    add_station_cut(cuts, context, loop, piece.side[e], e == 0 ? piece.line.start() : piece.line.end(), e == 0 ? along : -along, purlin);

            for (const Piece& part : compute_pieces(piece.line, outer, false, context.tolerance)) {
                Station station{part.line, cuts};
                for (const size_t e : {0, 1})
                    if (part.ring[e] >= 0)
                        add_plane(station.cuts, compute_wall_face(outer, part.ring[e], part.side[e], e == 0 ? part.line.start() : part.line.end()));
                if (part.line.length() > size.first)
                    stations.push_back(station);
            }
        }
    }

    return stations;
}

} // namespace wood_grid::build
