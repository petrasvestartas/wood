#pragma once
// Included by grid.h after its structs: the joint rules read Framing.

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Roles
// ═══════════════════════════════════════════════════════════════════════════

/// Rank of a role at a node, highest through: core wall 10, column 9, edge girder 8, edge beam 7, girder 6, beam 5, purlin 4, deck 3, facade wall 2, brace 1, none 0.
inline int compute_rank(int role) {

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

/// Element name of a role.
inline std::string compute_name(int role) {

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

/// Profile of a role from the framing, the fallbacks of Profiles applied.
inline std::vector<session_cpp::Polyline> compute_profile(int role, const Framing& framing) {

    const Profiles& profiles = framing.profiles;
    switch (role) {
        case 1: return profiles.girder;
        case 2: return profiles.beam.empty() ? profiles.girder : profiles.beam;
        case 3: return profiles.purlin.empty() ? compute_profile(2, framing) : profiles.purlin;
        case 4: return profiles.edge_girder.empty() ? profiles.girder : profiles.edge_girder;
        case 5: return profiles.edge_beam.empty() ? compute_profile(framing.system == 2 ? 3 : 2, framing) : profiles.edge_beam;
        case 6: return profiles.brace.empty() ? compute_profile(2, framing) : profiles.brace;
        default: return {};
    }
}

/// Profile of the member on a plan edge: its role's, scaled by the edge's width and depth.
inline std::vector<session_cpp::Polyline> compute_profile(const session_cpp::Mesh& plan, std::pair<size_t, size_t> edge, const Framing& framing) {
    return wood_session::compute_scaled(compute_profile(static_cast<int>(plan.edge_attribute(edge, "role").value_or(0.0)), framing), plan.edge_attribute(edge, "width").value_or(0.0), plan.edge_attribute(edge, "depth").value_or(0.0));
}

/// Top and bottom of a member of role relative to its datum: girders and edge girders [-drop, -drop - depth], every other member [0, -depth].
inline std::pair<double, double> compute_range(int role, double depth, double drop) {

    if (role == 1 || role == 4)
        return {-drop, -drop - depth};

    return {0.0, -depth};
}

/// A member end at a plan vertex: what the joint rules read.
struct Member {
    size_t other = 0; // The vertex at its far end.
    session_cpp::Vector direction; // Unit plan direction away from the vertex.
    int role = 0; // Edge role.
    int rank = 0; // compute_rank of the role.
    double width = 0.0; // Profile width.
    double top = 0.0; // Top relative to the datum.
    double bottom = 0.0; // Bottom relative to the datum.
};

/// The member on the edge from vertex to other.
inline Member compute_member(const session_cpp::Mesh& plan, size_t vertex, size_t other, const Framing& framing) {

    const std::pair<size_t, size_t> edge(vertex, other);
    const std::pair<double, double> size = wood_session::compute_size(compute_profile(plan, edge, framing));
    const int role = static_cast<int>(plan.edge_attribute(edge, "role").value_or(0.0));
    const std::pair<double, double> range = compute_range(role, size.second, plan.edge_attribute(edge, "drop").value_or(framing.drop));

    return {other, compute_direction(*plan.vertex_point(vertex), *plan.vertex_point(other)), role, compute_rank(role), size.first, range.first, range.second};
}

/// Every member at a vertex, counter-clockwise by direction.
inline std::vector<Member> compute_members(const session_cpp::Mesh& plan, size_t vertex, const Framing& framing) {

    std::vector<Member> members;
    for (const size_t other : plan.vertex_neighbors(vertex).value_or(std::vector<size_t>()))
        if (plan.edge_attribute({vertex, other}, "role").value_or(0.0) > 0.0)
            members.push_back(compute_member(plan, vertex, other, framing));

    std::sort(members.begin(), members.end(), [](const Member& a, const Member& b) { return std::atan2(a.direction[1], a.direction[0]) < std::atan2(b.direction[1], b.direction[0]); });

    return members;
}

/// Angle of a plan direction from x as a line, in [0, pi).
inline double compute_angle(const session_cpp::Vector& direction) {

    const double angle = std::atan2(direction[1], direction[0]);

    return angle < 0.0 ? angle + session_cpp::Tolerance::PI : angle;
}

/// The member of index that continues straight on within 45 degrees at the same rank, the straightest; none otherwise.
inline std::optional<size_t> compute_continuation(const std::vector<Member>& members, size_t index) {

    std::optional<size_t> straight;
    double best = std::cos(45.0 * session_cpp::Tolerance::TO_RADIANS);
    for (size_t k = 0; k < members.size(); k++) {
        const double dot = -members[k].direction.dot(members[index].direction);
        if (k != index && members[k].rank == members[index].rank && dot > best) {
            best = dot;
            straight = k;
        }
    }

    return straight;
}

/// The member that runs through a vertex and its continuation: the highest rank, then one with a straight continuation within 45 degrees, then the smallest angle from x; the vertex attribute through overrides the first.
inline std::pair<std::optional<size_t>, std::optional<size_t>> compute_through(const std::vector<Member>& members, std::optional<double> forced) {

    std::optional<size_t> through;
    if (forced && *forced >= 0.0 && *forced < members.size())
        through = static_cast<size_t>(*forced);

    for (size_t k = 0; k < members.size() && !forced; k++) {
        const std::tuple<int, int, double> score(-members[k].rank, compute_continuation(members, k) ? 0 : 1, compute_angle(members[k].direction));
        if (!through || score < std::tuple<int, int, double>(-members[*through].rank, compute_continuation(members, *through) ? 0 : 1, compute_angle(members[*through].direction)))
            through = k;
    }

    if (!through)
        return {std::nullopt, std::nullopt};

    return {through, compute_continuation(members, *through)};
}

// ═══════════════════════════════════════════════════════════════════════════
// Cuts
// ═══════════════════════════════════════════════════════════════════════════

/// Adds a plane unless an equal one is there.
inline void add_plane(std::vector<session_cpp::Plane>& planes, const session_cpp::Plane& plane) {

    for (const session_cpp::Plane& other : planes)
        if (other.z_axis().dot(plane.z_axis()) > 1.0 - 1e-9 && std::abs((plane.origin() - other.origin()).dot(other.z_axis())) < 1e-6)
            return;

    planes.push_back(plane);
}

/// True when two members share more than tolerance of height.
inline bool compute_overlap(const Member& a, const Member& b, double tolerance) {
    return std::min(a.top, b.top) - std::max(a.bottom, b.bottom) > tolerance;
}

/// The end of a member at its vertex: how far its axis runs past the vertex before the cuts, and the cut planes, all at z 0.
struct End {
    double overrun = 0.0; // Length past the vertex the axis is built with.
    std::vector<session_cpp::Plane> planes; // Cut planes, each keeping the side its normal points to.
};

/// Cut planes of the member on the edge from vertex to other at its vertex end: exit faces of the column there (nodes 1 and 2), of every core wall, of the through member and of every higher member whose height it shares; the bisector with its partner when it is through or continues; the perpendicular plane at the farthest butting corner, or reach under node 0, when it is through and open; equal open members mitre pairwise with their angular neighbours.
inline End compute_cuts(const session_cpp::Mesh& plan, size_t vertex, size_t other, const Framing& framing, const std::map<size_t, std::vector<session_cpp::Point>>& columns) {

    const session_cpp::Point origin = compute_lift(*plan.vertex_point(vertex), 0.0);
    const std::vector<Member> members = compute_members(plan, vertex, framing);
    const size_t me = std::find_if(members.begin(), members.end(), [&](const Member& member) { return member.other == other; }) - members.begin();
    const session_cpp::Vector direction = members[me].direction;
    const auto parallel = [&](const session_cpp::Vector& along) { return std::abs(along.cross(direction)[2]) < 1e-6; };
    const auto exit = [&](const std::vector<session_cpp::Point>& polygon, std::vector<session_cpp::Plane>& planes) {
        if (const std::optional<session_cpp::Plane> plane = compute_exit(polygon, origin, direction))
            add_plane(planes, *plane);
    };

    End end;
    if (framing.node != 0 && columns.count(vertex))
        exit(columns.at(vertex), end.planes);

    for (const size_t next : plan.vertex_neighbors(vertex).value_or(std::vector<size_t>())) {
        const session_cpp::Vector along = compute_direction(origin, *plan.vertex_point(next));
        if (plan.edge_attribute({vertex, next}, "wall").value_or(0.0) == 2.0 && !parallel(along))
            exit(compute_strip(origin, along, framing.wall / 2.0), end.planes);
    }

    const std::pair<std::optional<size_t>, std::optional<size_t>> through = compute_through(members, plan.vertex_attribute(vertex, "through"));
    const size_t top = through.first ? members[*through.first].rank : 0;
    const bool mitre = through.first && !through.second && std::count_if(members.begin(), members.end(), [&](const Member& member) { return member.rank == top; }) > 1;
    const auto is_through = [&](size_t k) { return (through.first && k == *through.first) || (through.second && k == *through.second); };
    const auto neighbours = [&](size_t k) {
        std::vector<size_t> equal;
        for (size_t j = 0; j < members.size(); j++)
            if (members[j].rank == members[k].rank && (mitre || !is_through(j)))
                equal.push_back(j);
        const size_t at = std::find(equal.begin(), equal.end(), k) - equal.begin();
        return std::vector<size_t>{equal[(at + equal.size() - 1) % equal.size()], equal[(at + 1) % equal.size()]};
    };

    if (is_through(me) && !mitre) {
        const std::optional<size_t> partner = me == *through.first ? through.second : through.first;
        if (partner) {
            add_plane(end.planes, compute_bisector(origin, direction, members[*partner].direction));
            end.overrun = framing.reach + 4.0 * members[me].width;
            return end;
        }

        const session_cpp::Vector side = direction.cross(session_cpp::Vector(0.0, 0.0, 1.0));
        double open = framing.node == 0 ? -framing.reach : 0.0;
        for (size_t k = 0; k < members.size(); k++) {
            const double speed = members[k].direction.dot(side);
            if (k == me || std::abs(speed) < 1e-6 || !compute_overlap(members[me], members[k], framing.tolerance))
                continue;

            const session_cpp::Vector across = members[k].direction.cross(session_cpp::Vector(0.0, 0.0, 1.0));
            for (const double sign : {-1.0, 1.0}) {
                const double t = ((speed > 0.0 ? 1.0 : -1.0) * members[me].width / 2.0 - sign * members[k].width / 2.0 * across.dot(side)) / speed;
                open = std::min(open, (members[k].direction * t + across * (sign * members[k].width / 2.0)).dot(direction));
            }
        }

        add_plane(end.planes, session_cpp::Plane::from_point_normal(origin + direction * open, direction));
        end.overrun = std::max(0.0, -open) + framing.reach + members[me].width;
        return end;
    }

    for (size_t k = 0; k < members.size(); k++) {
        if (k == me || parallel(members[k].direction) || !compute_overlap(members[me], members[k], framing.tolerance))
            continue;

        if (members[k].rank > members[me].rank || (!mitre && is_through(k)))
            exit(compute_strip(origin, members[k].direction, members[k].width / 2.0), end.planes);
    }

    for (const size_t k : neighbours(me))
        if (k != me && !parallel(members[k].direction) && compute_overlap(members[me], members[k], framing.tolerance))
            add_plane(end.planes, compute_bisector(origin, direction, members[k].direction));

    end.overrun = end.planes.empty() ? 0.0 : framing.reach + 4.0 * members[me].width;

    return end;
}

/// Lowest member bottom at a vertex relative to the datum, 0 when only the deck arrives: the head top under node 0.
inline double compute_head_top(const session_cpp::Mesh& plan, size_t vertex, const Framing& framing) {

    double top = 0.0;
    for (const Member& member : compute_members(plan, vertex, framing))
        top = std::min(top, member.bottom);

    return top;
}

// ═══════════════════════════════════════════════════════════════════════════
// Outlines
// ═══════════════════════════════════════════════════════════════════════════

/// Counter-clockwise corners of a plan face.
inline std::vector<size_t> compute_loop(const session_cpp::Mesh& plan, size_t face) {

    std::vector<size_t> loop = *plan.face_vertices(face);
    std::vector<session_cpp::Point> points;
    for (const size_t key : loop)
        points.push_back(*plan.vertex_point(key));
    if (compute_area(points) < 0.0)
        std::reverse(loop.begin(), loop.end());

    return loop;
}

/// Reach of a column polygon past a vertex along a plan direction.
inline double compute_reach(const std::vector<session_cpp::Point>& polygon, const session_cpp::Point& vertex, const session_cpp::Vector& direction) {

    double reach = 0.0;
    for (const session_cpp::Point& point : polygon)
        reach = std::max(reach, (compute_lift(point, 0.0) - compute_lift(vertex, 0.0)).dot(direction));

    return reach;
}

/// Deck loops of a face at z 0: boundary sides on the outer face of the boundary member or the column there, whichever is further out; interior sides on the line; core sides on the core wall's outer face; a re-entrant corner mitred against the neighbouring deck; minus every through column section (node 2); face_holes as core outer rings or open hole rings.
inline std::vector<session_cpp::Polyline> compute_outline(const session_cpp::Mesh& plan, size_t face, const Framing& framing, const std::map<size_t, std::vector<session_cpp::Point>>& columns) {

    const std::vector<size_t> loop = compute_loop(plan, face);
    const size_t count = loop.size();
    std::vector<session_cpp::Point> points;
    for (const size_t key : loop)
        points.push_back(compute_lift(*plan.vertex_point(key), 0.0));

    std::vector<std::vector<size_t>> sides;
    std::vector<double> distances;
    for (size_t i = 0; i < count; i++) {
        const std::pair<size_t, size_t> edge(loop[i], loop[(i + 1) % count]);
        sides.push_back(plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>()));
        const session_cpp::Vector outward = compute_direction(points[i], points[(i + 1) % count]).cross(session_cpp::Vector(0.0, 0.0, 1.0));

        double distance = 0.0;
        if (plan.edge_attribute(edge, "wall").value_or(0.0) == 2.0)
            distance = -framing.wall / 2.0;
        else if (sides[i].size() < 2) {
            distance = wood_session::compute_size(compute_profile(plan, edge, framing)).first / 2.0;
            for (const size_t key : {edge.first, edge.second})
                if (columns.count(key))
                    distance = std::max(distance, compute_reach(columns.at(key), *plan.vertex_point(key), outward));
        }
        distances.push_back(distance);
    }

    const std::vector<session_cpp::Point> offset = compute_offset(points, distances);
    std::vector<session_cpp::Point> outline;
    for (size_t i = 0; i < count; i++) {
        const size_t before = (i + count - 1) % count;
        const std::vector<size_t> around = plan.vertex_faces(loop[i]).value_or(std::vector<size_t>());
        const bool reentrant = distances[before] != distances[i] && std::any_of(around.begin(), around.end(), [&](size_t other) {
            return plan.face_attribute(other, "floor").value_or(0.0) == 1.0 && std::count(sides[before].begin(), sides[before].end(), other) + std::count(sides[i].begin(), sides[i].end(), other) == 0;
        });

        if (!reentrant) {
            outline.push_back(offset[i]);
            continue;
        }

        const session_cpp::Vector in = compute_direction(points[before], points[i]).cross(session_cpp::Vector(0.0, 0.0, 1.0));
        const session_cpp::Vector out = compute_direction(points[i], points[(i + 1) % count]).cross(session_cpp::Vector(0.0, 0.0, 1.0));
        const session_cpp::Vector pushed = distances[i] > distances[before] ? out : in;
        const session_cpp::Vector held = distances[i] > distances[before] ? in : out;
        const session_cpp::Point mitre = points[i] + (pushed - held) * (std::max(distances[before], distances[i]) / (1.0 - pushed.dot(held)));
        outline.push_back(distances[i] > distances[before] ? points[i] : mitre);
        outline.push_back(distances[i] > distances[before] ? mitre : points[i]);
    }

    std::vector<session_cpp::Polyline> loops = {to_polyline(outline)};
    if (plan.get_face_holes().count(face))
        for (const std::vector<size_t>& ring : plan.get_face_holes().at(face)) {
            std::vector<session_cpp::Point> hole;
            for (const size_t key : ring)
                hole.push_back(compute_lift(*plan.vertex_point(key), 0.0));
            if (compute_area(hole) < 0.0)
                std::reverse(hole.begin(), hole.end());
            if (plan.vertex_attribute(ring[0], "wall").value_or(0.0) == 2.0)
                hole = compute_offset(hole, std::vector<double>(hole.size(), framing.wall / 2.0));
            std::reverse(hole.begin(), hole.end());
            loops.push_back(to_polyline(hole));
        }

    std::vector<session_cpp::Polyline> notches;
    for (const size_t key : loop)
        if (framing.node == 2 && columns.count(key))
            notches.push_back(to_polyline(columns.at(key)));
    if (!notches.empty())
        loops = compute_regions(loops, notches, 2);

    std::stable_sort(loops.begin(), loops.end(), [](const session_cpp::Polyline& a, const session_cpp::Polyline& b) { return compute_area(to_loop(a)) > compute_area(to_loop(b)); });

    return loops;
}

/// Loops of the deck of a face cut into ceil(width / panel) equal strips across the deck span; one loop set when panel is 0.
inline std::vector<std::vector<session_cpp::Polyline>> compute_panels(const std::vector<session_cpp::Polyline>& loops, const session_cpp::Vector& span, double panel) {

    if (panel <= 0.0 || loops.empty())
        return {loops};

    const session_cpp::Vector across = session_cpp::Vector(0.0, 0.0, 1.0).cross(span);
    double low = std::numeric_limits<double>::max();
    double high = -low;
    for (const session_cpp::Point& point : to_loop(loops[0])) {
        low = std::min(low, (point - session_cpp::Point(0.0, 0.0, 0.0)).dot(across));
        high = std::max(high, (point - session_cpp::Point(0.0, 0.0, 0.0)).dot(across));
    }

    const int strips = std::max(1, static_cast<int>(std::ceil((high - low) / panel - 1e-9)));
    const double width = (high - low) / strips;
    std::vector<std::vector<session_cpp::Polyline>> panels;
    for (int k = 0; k < strips; k++) {
        const session_cpp::Point centre = session_cpp::Point(0.0, 0.0, 0.0) + across * (low + (k + 0.5) * width);
        const std::vector<session_cpp::Point> band = {centre - span * 1e7 - across * (width / 2.0), centre + span * 1e7 - across * (width / 2.0), centre + span * 1e7 + across * (width / 2.0), centre - span * 1e7 + across * (width / 2.0)};
        std::vector<session_cpp::Polyline> strip = compute_regions(loops, {to_polyline(band)}, 0);
        std::stable_sort(strip.begin(), strip.end(), [](const session_cpp::Polyline& a, const session_cpp::Polyline& b) { return compute_area(to_loop(a)) > compute_area(to_loop(b)); });
        if (!strip.empty())
            panels.push_back(strip);
    }

    return panels;
}

} // namespace wood_grid
