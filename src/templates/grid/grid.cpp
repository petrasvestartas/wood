#include "pch.h"
#include "src/templates/grid/grid_joints.h"

using namespace session_cpp;

namespace wood_grid::build {

using namespace wood_grid::plan;
using namespace wood_grid::joints;

// ═══════════════════════════════════════════════════════════════════════════
// Roles and columns
// ═══════════════════════════════════════════════════════════════════════════

/// True when a plan edge lies between core faces alone, or on a core ring with a core face beside it; a drawn core wall between floors carries its beam.
bool is_core_edge(const Mesh& plan, std::pair<size_t, size_t> edge) {

    bool cores = true;
    bool floors = true;
    for (const size_t face : plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>())) {
        cores = cores && plan.face_attribute(face, "core").value_or(0.0) == 1.0;
        floors = floors && plan.face_attribute(face, "floor").value_or(0.0) == 1.0;
    }

    return cores || (plan.edge_attribute(edge, "wall").value_or(0.0) == 2.0 && !floors);
}

/// The role of a plan edge from the framing and the pattern: perimeter edges edge girders on span-family lines and edge beams elsewhere, girders on the span family, purlins on the cross lines of system 2, beams on free lines and under span -1, nothing inside cores or under system 0.
int compute_role(const Mesh& plan, std::pair<size_t, size_t> edge, const Framing& framing) {

    const int family = static_cast<int>(plan.edge_attribute(edge, "family").value_or(-1.0));
    int system = framing.system;
    int span = framing.span;
    for (const size_t face : plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>())) {
        system = static_cast<int>(plan.face_attribute(face, "system").value_or(system));
        span = static_cast<int>(plan.face_attribute(face, "span").value_or(span));
    }

    if (is_core_edge(plan, edge))
        return 0;
    if (plan.edge_attribute(edge, "boundary").value_or(0.0) == 1.0)
        return !framing.edge ? 0 : span >= 0 && family == span ? 4 : 5;
    if (system == 0)
        return 0;
    if (span < 0 || family < 0)
        return 2;
    if (family == span)
        return 1;

    return system == 2 ? 3 : 0;
}

/// The role of every edge without one from compute_role; wall 1 on the perimeter when the framing asks for a facade.
void compute_roles(Mesh& plan, const Framing& framing) {

    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        if (plan.edge_attribute(edge, "boundary").value_or(0.0) == 1.0 && framing.facade && plan.edge_attribute(edge, "wall").value_or(0.0) == 0.0)
            plan.set_edge_attribute(edge, "wall", 1.0);
        if (!plan.edge_attribute(edge, "role"))
            plan.set_edge_attribute(edge, "role", compute_role(plan, edge, framing));
    }
}

/// The sides of a loop, side i from corner i to i + 1.
std::vector<std::pair<size_t, size_t>> compute_sides(const std::vector<size_t>& loop) {

    std::vector<std::pair<size_t, size_t>> sides;
    for (size_t i = 0; i < loop.size(); i++)
        sides.emplace_back(loop[i], loop[(i + 1) % loop.size()]);

    return sides;
}

/// Mean line direction of the plan edges in a family, opposite directions alike; none when no edge is in it.
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

/// The turn of the column profile at a plan vertex: the mean direction of the first pattern family's edges there, x where none passes.
double compute_turn(const Mesh& plan, size_t vertex) {

    std::vector<std::pair<size_t, size_t>> edges;
    for (const size_t other : plan.vertex_neighbors(vertex).value_or(std::vector<size_t>()))
        edges.emplace_back(vertex, other);

    const std::optional<Vector> axis = compute_family_direction(plan, edges, 0);

    return axis ? std::atan2((*axis)[1], (*axis)[0]) : 0.0;
}

/// The plan section of the column at a vertex, counter-clockwise at z 0: the column profile centred there, its x axis along the first pattern family, so profile_rectangle(265, 380) is 265 across x on an orthogonal grid and squared to the rays on a radial one.
std::vector<Point> compute_column_polygon(const Mesh& plan, size_t vertex, const Framing& framing) {

    const double turn = compute_turn(plan, vertex);
    const Point centre = compute_lift(*plan.vertex_point(vertex), 0.0);

    std::vector<Point> points;
    for (const Point& point : to_loop(framing.profiles.column[0]))
        points.push_back(centre + Vector(point[0] * std::cos(turn) - point[1] * std::sin(turn), point[0] * std::sin(turn) + point[1] * std::cos(turn), 0.0));

    return points;
}

/// The side of a direction polygon that vanishes or turns back between its neighbours, none when every side stands: the diagonal side at the corner of a square column.
std::optional<size_t> compute_degenerate(const std::vector<Vector>& directions, const Point& centre, const std::vector<double>& distances) {

    const std::vector<Point> corners = to_loop(compute_polygon(directions, centre, distances));
    for (size_t j = 0; j < directions.size(); j++)
        if ((corners[j] - corners[(j + directions.size() - 1) % directions.size()]).dot(Vector(0.0, 0.0, 1.0).cross(directions[j])) <= 1e-6)
            return j;

    return std::nullopt;
}

/// Unit plan direction along the perimeter edge that opens the floor at a vertex counter-clockwise, none inside the floor: where a corner head's loops start, so its rotated copies key alike.
std::optional<Vector> compute_start(const Mesh& plan, size_t vertex) {

    for (const size_t face : plan.vertex_faces(vertex).value_or(std::vector<size_t>())) {
        const std::vector<size_t> loop = compute_loop(plan, face);
        const size_t next = loop[(std::find(loop.begin(), loop.end(), vertex) - loop.begin() + 1) % loop.size()];
        if (plan.face_attribute(face, "floor").value_or(0.0) == 1.0 && plan.edge_attribute({vertex, next}, "boundary").value_or(0.0) == 1.0)
            return compute_direction(*plan.vertex_point(vertex), *plan.vertex_point(next));
    }

    return std::nullopt;
}

/// The directions at a vertex turned to begin at the first one counter-clockwise from start.
void compute_started(std::vector<Vector>& directions, const Vector& start) {

    size_t first = 0;
    double best = Tolerance::TWO_PI;
    for (size_t j = 0; j < directions.size(); j++) {
        double angle = std::atan2(start.cross(directions[j])[2], start.dot(directions[j]));
        angle = angle < -1e-9 ? angle + Tolerance::TWO_PI : angle;
        if (angle < best) {
            best = angle;
            first = j;
        }
    }

    std::rotate(directions.begin(), directions.begin() + first, directions.end());
}

/// The polygons of a head that carries the deck at a vertex, bottom, middle, top: the direction polygon circumscribing the column, halfway to reach and at reach, all from compute_start; a direction whose side would not stand is left out.
std::vector<std::vector<Point>> compute_head(const Mesh& plan, size_t vertex, const Framing& framing) {

    const double turn = compute_turn(plan, vertex);
    const Point centre = compute_lift(*plan.vertex_point(vertex), 0.0);
    std::vector<Vector> directions = compute_directions(plan, vertex);
    const std::optional<Vector> start = compute_start(plan, vertex);
    if (start)
        compute_started(directions, *start);
    std::vector<double> distances;
    for (const Vector& direction : directions) {
        const double angle = std::atan2(direction[1], direction[0]) - turn;
        distances.push_back(wood_session::compute_support(framing.profiles.column, Vector(std::cos(angle), std::sin(angle), 0.0)));
    }

    const size_t count = directions.size();
    for (size_t round = 0; round < count && directions.size() > 3; round++) {
        const std::optional<size_t> bad = compute_degenerate(directions, centre, distances);
        if (!bad)
            break;

        directions.erase(directions.begin() + *bad);
        distances.erase(distances.begin() + *bad);
    }

    std::vector<double> middle;
    for (const double distance : distances)
        middle.push_back((distance + framing.reach) / 2.0);

    return {to_loop(compute_polygon(directions, centre, distances)), to_loop(compute_polygon(directions, centre, middle)), to_loop(compute_polygon(directions, centre, std::vector<double>(directions.size(), framing.reach)))};
}

/// True when a plan polygon comes within clearance of the wall band about a core side, wall wide and centred on it.
bool is_against(const std::vector<Point>& polygon, const Point& centre, const Point& a, const Point& b, double wall, double clearance) {

    const Vector along = compute_direction(a, b);
    const Vector normal = along.cross(Vector(0.0, 0.0, 1.0));
    const double across = (compute_lift(centre, 0.0) - compute_lift(a, 0.0)).dot(normal);
    const Vector towards = across > 0.0 ? -normal : normal;
    if (std::abs(across) >= wall / 2.0 + compute_reach(polygon, centre, towards) + clearance)
        return false;

    const double at = (compute_lift(centre, 0.0) - compute_lift(a, 0.0)).dot(along);

    return at + compute_reach(polygon, centre, along) > -clearance && at - compute_reach(polygon, centre, -along) < compute_distance(a, b) + clearance;
}

/// Column 0 on every vertex whose column section would stand in a core wall: the wall carries the deck there, as it does on the ring.
void compute_clearance(Level& level, const Framing& framing) {

    for (const size_t vertex : level.plan.vertices()) {
        if (level.plan.vertex_attribute(vertex, "column").value_or(0.0) != 1.0)
            continue;

        const std::vector<Point> polygon = compute_column_polygon(level.plan, vertex, framing);
        const Point centre = compute_lift(*level.plan.vertex_point(vertex), 0.0);
        for (const Polyline& core : level.cores) {
            const std::vector<Point> corners = to_loop(core);
            for (size_t i = 0; i < corners.size(); i++)
                if (is_against(polygon, centre, corners[i], corners[(i + 1) % corners.size()], framing.wall, 0.0))
                    level.plan.set_vertex_attribute(vertex, "column", 0.0);
        }
    }
}

/// The loop of face b joined onto the loop of face a across the run of edges they share, both counter-clockwise, every vertex kept; empty when they share no edge or more than one run.
std::vector<size_t> compute_joined(const std::vector<size_t>& a, const std::vector<size_t>& b) {

    std::set<std::pair<size_t, size_t>> shared;
    for (size_t j = 0; j < b.size(); j++)
        shared.insert(std::minmax(b[j], b[(j + 1) % b.size()]));

    const size_t count = a.size();
    std::vector<bool> on(count, false);
    for (size_t i = 0; i < count; i++)
        on[i] = shared.count(std::minmax(a[i], a[(i + 1) % count])) > 0;

    size_t runs = 0;
    size_t start = 0;
    for (size_t i = 0; i < count; i++)
        if (on[i] && !on[(i + count - 1) % count]) {
            runs++;
            start = i;
        }
    if (runs != 1)
        return {};

    size_t length = 0;
    for (size_t i = 0; i < count && on[(start + i) % count]; i++)
        length++;

    std::vector<size_t> loop;
    for (size_t k = 0; k <= count - length; k++)
        loop.push_back(a[(start + length + k) % count]);

    const size_t at = std::find(b.begin(), b.end(), a[start]) - b.begin();
    for (size_t k = 1; k < b.size() && b[(at + k) % b.size()] != a[(start + length) % count]; k++)
        loop.push_back(b[(at + k) % b.size()]);

    return loop;
}

/// The floor face across the edge of a face's loop farthest from the line through a and b, none when there is no such face or it is a core.
std::optional<size_t> compute_far_face(const Mesh& plan, size_t face, const std::vector<size_t>& loop, const Point& a, const Vector& normal, double depth, double tolerance) {

    for (size_t j = 0; j < loop.size(); j++) {
        const size_t next = loop[(j + 1) % loop.size()];
        if ((compute_lift(*plan.vertex_point(loop[j]), 0.0) - a).dot(normal) < depth - tolerance || (compute_lift(*plan.vertex_point(next), 0.0) - a).dot(normal) < depth - tolerance)
            continue;

        for (const size_t other : plan.edge_faces(loop[j], next).value_or(std::vector<size_t>()))
            if (other != face && plan.face_attribute(other, "floor").value_or(0.0) == 1.0 && plan.face_attribute(other, "core").value_or(0.0) != 1.0)
                return other;
    }

    return std::nullopt;
}

/// Two faces replaced by one over the joined loop, with the attributes and the holes of the second.
void compute_merged(Mesh& plan, size_t face, size_t other, const std::vector<size_t>& loop) {

    std::vector<std::vector<size_t>> holes;
    for (const size_t key : {face, other})
        if (plan.get_face_holes().count(key))
            for (const std::vector<size_t>& hole : plan.get_face_holes().at(key))
                holes.push_back(hole);

    std::map<std::string, double> attributes;
    for (const char* name : {"floor", "core", "system", "span", "spacing", "thickness"})
        if (plan.face_attribute(other, name))
            attributes[name] = *plan.face_attribute(other, name);

    plan.remove_face(face);
    plan.remove_face(other);
    const std::optional<size_t> joined = plan.add_face(loop);
    if (!joined)
        return;

    for (const std::pair<const std::string, double>& attribute : attributes)
        plan.set_face_attribute(*joined, attribute.first, attribute.second);
    if (!holes.empty())
        plan.set_face_holes(*joined, holes);
}

/// Every floor face beside a core wall whose deck would be narrower than the wall once moved onto the wall's outer face joined into the floor face across its far edge, so a pattern line hugging a core leaves no sliver plate.
void compute_slivers(Mesh& plan, double wall, double tolerance) {

    for (const size_t face : plan.faces()) {
        if (!plan.face_vertices(face) || plan.face_attribute(face, "floor").value_or(0.0) != 1.0 || plan.face_attribute(face, "core").value_or(0.0) == 1.0)
            continue;

        const std::vector<size_t> loop = compute_loop(plan, face);
        const size_t count = loop.size();
        for (size_t i = 0; i < count; i++) {
            if (plan.edge_attribute({loop[i], loop[(i + 1) % count]}, "wall").value_or(0.0) != 2.0)
                continue;

            const Point a = compute_lift(*plan.vertex_point(loop[i]), 0.0);
            const Vector normal = Vector(0.0, 0.0, 1.0).cross(compute_direction(a, *plan.vertex_point(loop[(i + 1) % count])));
            double depth = 0.0;
            for (const size_t key : loop)
                depth = std::max(depth, (compute_lift(*plan.vertex_point(key), 0.0) - a).dot(normal));
            if (depth - wall / 2.0 >= wall)
                continue;

            const std::optional<size_t> other = compute_far_face(plan, face, loop, a, normal, depth, tolerance);
            const std::vector<size_t> joined = other ? compute_joined(loop, compute_loop(plan, *other)) : std::vector<size_t>();
            if (joined.empty())
                continue;

            compute_merged(plan, face, *other, joined);
            break;
        }
    }
}

/// A column of a storey: its foot on the lower level and its head on the upper one.
struct Stack {
    size_t lower = 0; // Vertex in the lower plan.
    size_t upper = 0; // Vertex in the upper plan.
    Point foot; // Plan point of the foot at z 0.
    Point head; // Plan point of the head at z 0.
};

/// The two arrangement line ids that made a plan vertex, the identity used across levels.
std::pair<double, double> compute_identity(const Mesh& plan, size_t vertex) {
    return {plan.vertex_attribute(vertex, "line_a").value_or(-1.0), plan.vertex_attribute(vertex, "line_b").value_or(-1.0)};
}

/// How well a lower vertex carries an upper one, the lower the better: its plan distance by identity within lean, plus 1 at the same point within tolerance, plus lean + 2 on the same line or the section within lean; infinite otherwise.
double compute_carry(const Mesh& lower, size_t other, const Mesh& upper, size_t vertex, double lean, double tolerance) {

    const double distance = compute_distance(*lower.vertex_point(other), *upper.vertex_point(vertex));
    const std::pair<double, double> key = compute_identity(upper, vertex);
    const std::pair<double, double> own = compute_identity(lower, other);
    if (own == key && distance <= lean)
        return distance;
    if (distance <= tolerance)
        return distance + 1.0;

    const bool same_line = key.first >= 0.0 && (own.first == key.first || own.second == key.second || own.first == key.second || own.second == key.first);
    const bool section = upper.vertex_attribute(vertex, "boundary").value_or(0.0) == 1.0 && lower.vertex_attribute(other, "boundary").value_or(0.0) == 1.0;
    if ((same_line || section) && distance <= lean)
        return distance + lean + 2.0;

    return std::numeric_limits<double>::max();
}

/// Columns between levels k and k + 1: every upper vertex with column 1 matched to the lower column vertex with the best compute_carry; an upper vertex with no match is written column 2.
std::vector<Stack> compute_columns(Building& building, size_t k, const Framing& framing) {

    const Mesh& lower = building.levels[k].plan;
    Mesh& upper = building.levels[k + 1].plan;
    const double lean = (building.levels[k + 1].z - building.levels[k].z) * std::tan(framing.taper * Tolerance::TO_RADIANS);

    std::vector<Stack> stacks;
    for (const size_t vertex : upper.vertices()) {
        if (upper.vertex_attribute(vertex, "column").value_or(0.0) != 1.0)
            continue;

        std::optional<size_t> match;
        double best = std::numeric_limits<double>::max();
        for (const size_t other : lower.vertices()) {
            const double score = lower.vertex_attribute(other, "column").value_or(0.0) == 1.0 ? compute_carry(lower, other, upper, vertex, lean, building.tolerance) : std::numeric_limits<double>::max();
            if (score < best) {
                best = score;
                match = other;
            }
        }

        if (!match) {
            upper.set_vertex_attribute(vertex, "column", 2.0);
            continue;
        }

        stacks.push_back({*match, vertex, compute_lift(*lower.vertex_point(*match), 0.0), compute_lift(*upper.vertex_point(vertex), 0.0)});
    }

    return stacks;
}

// ═══════════════════════════════════════════════════════════════════════════
// Stations
// ═══════════════════════════════════════════════════════════════════════════

/// A purlin station: its line at z 0 and what each end lands on.
struct Station {
    Line line; // From the first support to the second.
    Support first; // Support at the start.
    Support second; // Support at the end.
};

/// True when support a comes first along the station.
bool is_before(const Support& a, const Support& b) {
    return a.t < b.t;
}

/// Plan offset of a point across the stations.
double compute_projection(const Point& point, const Vector& across) {
    return (compute_lift(point, 0.0) - Point(0.0, 0.0, 0.0)).dot(across);
}

/// The extent of a face across the stations, and of the unclipped pattern cell round it: the nearest cross lines of the pattern outside the face, the face itself when there are none.
std::array<double, 4> compute_cell(const Mesh& plan, const std::vector<size_t>& loop, const Pattern& pattern, int span, const Vector& along, const Vector& across, double tolerance) {

    double low = std::numeric_limits<double>::max();
    double high = -low;
    for (const size_t key : loop) {
        low = std::min(low, compute_projection(*plan.vertex_point(key), across));
        high = std::max(high, compute_projection(*plan.vertex_point(key), across));
    }

    double cell_low = -std::numeric_limits<double>::max();
    double cell_high = std::numeric_limits<double>::max();
    for (size_t i = 0; i < pattern.lines.size(); i++) {
        if (pattern.families[i] == span || std::abs(pattern.lines[i].to_direction().dot(along)) < 0.999)
            continue;

        const double at = compute_projection(pattern.lines[i].start(), across);
        if (at <= low + tolerance)
            cell_low = std::max(cell_low, at);
        if (at >= high - tolerance)
            cell_high = std::min(cell_high, at);
    }

    return {low, high, cell_low == -std::numeric_limits<double>::max() ? low : cell_low, cell_high == std::numeric_limits<double>::max() ? high : cell_high};
}

/// What a station end lands on at side i of ring r, met at t along the station: a plan edge of the face loop, a held-back core ring side, or nothing.
Support compute_landing(const Mesh& plan, const std::vector<size_t>& ring, size_t r, size_t i, double t) {

    const size_t next = (i + 1) % ring.size();
    Support support;
    support.kind = r == 0 ? 1 : plan.vertex_attribute(ring[i], "wall").value_or(0.0) == 2.0 ? 2 : 0;
    support.edge = {ring[i], ring[next]};
    support.along = compute_direction(compute_lift(*plan.vertex_point(ring[i]), 0.0), compute_lift(*plan.vertex_point(ring[next]), 0.0));
    support.t = t;

    return support;
}

/// The station pieces at offset at across a face and its holes: the station cut where it crosses a ring side and at both ends of a side it runs along, the pieces a purlin fits inside kept, each end remembering the side it landed on; pieces shorter than width dropped.
std::vector<Station> compute_station(const Mesh& plan, const std::vector<std::vector<size_t>>& rings, const Vector& along, const Vector& across, double at, double width) {

    const Point base = Point(0.0, 0.0, 0.0) + across * at;
    std::vector<Polyline> loops;
    std::vector<Support> supports;
    for (size_t r = 0; r < rings.size(); r++) {
        loops.push_back(to_polyline(compute_flat(plan, rings[r])));
        const size_t count = rings[r].size();
        for (size_t i = 0; i < count; i++) {
            const Point a = compute_lift(*plan.vertex_point(rings[r][i]), 0.0);
            const Point b = compute_lift(*plan.vertex_point(rings[r][(i + 1) % count]), 0.0);
            const double da = compute_projection(a, across) - at;
            const double db = compute_projection(b, across) - at;
            if (std::abs(da) < 1e-3 && std::abs(db) < 1e-3) {
                supports.push_back(compute_landing(plan, rings[r], r, (i + count - 1) % count, (a - base).dot(along)));
                supports.push_back(compute_landing(plan, rings[r], r, (i + 1) % count, (b - base).dot(along)));
            } else if ((da > 0.0) != (db > 0.0)) {
                supports.push_back(compute_landing(plan, rings[r], r, i, (a + (b - a) * (da / (da - db)) - base).dot(along)));
            }
        }
    }
    std::sort(supports.begin(), supports.end(), is_before);

    std::vector<Station> stations;
    for (size_t c = 0; c + 1 < supports.size(); c++) {
        const Point middle = base + along * ((supports[c].t + supports[c + 1].t) / 2.0);
        if (supports[c + 1].t - supports[c].t > width && is_inside(loops, middle + across * (width / 2.0)) && is_inside(loops, middle - across * (width / 2.0)))
            stations.push_back({Line::from_points(base + along * supports[c].t, base + along * supports[c + 1].t), supports[c], supports[c + 1]});
    }

    return stations;
}

/// Purlin stations of a system 2 face: stations parallel to its cross-family edges (else perpendicular to its girders) at ceil(cell / spacing) intervals over the unclipped cell between the bounding cross lines of the pattern, clipped to the face and its holes.
std::vector<Station> compute_stations(const Context& context, size_t face, const Pattern& pattern) {

    const Mesh& plan = context.plan;
    const std::vector<size_t> loop = compute_loop(plan, face);
    const int span = static_cast<int>(plan.face_attribute(face, "span").value_or(context.framing.span));
    const double spacing = plan.face_attribute(face, "spacing").value_or(context.framing.spacing);
    const std::optional<Vector> girder = compute_family_direction(plan, compute_sides(loop), span);
    if (!girder)
        return {};

    std::optional<Vector> station;
    for (int family = 0; family < 3 && !station; family++)
        if (family != span)
            station = compute_family_direction(plan, compute_sides(loop), family);
    const Vector along = station.value_or(Vector(0.0, 0.0, 1.0).cross(*girder));
    const Vector across = along.cross(Vector(0.0, 0.0, 1.0));
    const std::array<double, 4> cell = compute_cell(plan, loop, pattern, span, along, across, context.tolerance);

    std::vector<std::vector<size_t>> rings = {loop};
    if (plan.get_face_holes().count(face))
        for (const std::vector<size_t>& hole : plan.get_face_holes().at(face))
            rings.push_back(hole);
    const double width = wood_session::compute_size(compute_role_profile(3, context.framing)).first;

    std::vector<Station> stations;
    const int intervals = std::max(1, static_cast<int>(std::ceil((cell[3] - cell[2]) / spacing - 1e-9)));
    for (int k = 1; k < intervals; k++) {
        const double at = cell[2] + (cell[3] - cell[2]) * k / intervals;
        if (at <= cell[0] + context.tolerance || at >= cell[1] - context.tolerance)
            continue;

        for (const Station& piece : compute_station(plan, rings, along, across, at, width))
            stations.push_back(piece);
    }

    return stations;
}

// ═══════════════════════════════════════════════════════════════════════════
// Builders
// ═══════════════════════════════════════════════════════════════════════════

/// The kept length of an axis under cut planes, negative when nothing is left.
double compute_kept(const Point& start, const Point& end, const std::vector<Plane>& planes) {

    const Vector direction = (end - start).normalized();
    double low = 0.0;
    double high = start.distance(end);
    for (const Plane& plane : planes) {
        const double speed = direction.dot(plane.z_axis());
        const double offset = (plane.origin() - start).dot(plane.z_axis());
        if (std::abs(speed) < 1e-9) {
            if (offset > 0.0)
                return -1.0;
            continue;
        }

        if (speed > 0.0)
            low = std::max(low, offset / speed);
        else
            high = std::min(high, offset / speed);
    }

    return high - low;
}

/// Beams of a profile along an axis at height z with cuts: one, or two side by side for a double profile; none when the cuts leave less than least.
std::vector<std::shared_ptr<Element>> to_member(const Point& start, const Point& end, double z, const std::vector<Polyline>& profile, const std::vector<Plane>& cuts, const std::string& name, double least) {

    std::vector<std::shared_ptr<Element>> beams;
    if (compute_kept(start, end, cuts) < least)
        return beams;

    std::vector<std::vector<Polyline>> parts = {profile};
    if (profile.size() > 1 && compute_area(to_loop(profile[1])) > 0.0)
        parts = {{profile[0]}, {profile[1]}};

    const Vector side = Vector(0.0, 0.0, 1.0).cross((end - start).normalized());
    for (const std::vector<Polyline>& part : parts) {
        const double centre = Point::centroid(to_loop(part[0]))[0];
        std::vector<Polyline> centred;
        for (const Polyline& ring : part)
            centred.push_back(ring.translated(Vector(-centre, 0.0, 0.0)));

        std::shared_ptr<wood_session::Beam> beam = std::make_shared<wood_session::Beam>(Polyline({compute_lift(start, z) + side * centre, compute_lift(end, z) + side * centre}), centred, std::vector<Vector>{Vector(0.0, 0.0, 1.0)}, name);
        beam->cuts = cuts;
        beams.push_back(beam);
    }

    return beams;
}

/// The member on a plan edge of the level at z: its role's profile, its top at the datum less its drop, its ends from compute_cuts at both vertices; a stub shorter than its width is dropped unless it bears at both ends.
std::vector<std::shared_ptr<Element>> to_beam(const Context& context, std::pair<size_t, size_t> edge, double z) {

    const Member member = compute_member(context, edge.first, edge.second);
    const End first = compute_cuts(context, edge.first, edge.second);
    const End second = compute_cuts(context, edge.second, edge.first);
    const Point a = compute_lift(*context.plan.vertex_point(edge.first), 0.0);
    const Point b = compute_lift(*context.plan.vertex_point(edge.second), 0.0);
    std::vector<Plane> cuts = first.planes;
    cuts.insert(cuts.end(), second.planes.begin(), second.planes.end());
    const double least = first.bearing && second.bearing ? context.tolerance : member.width;

    return to_member(a - member.direction * first.overrun, b + member.direction * second.overrun, z + (member.top + member.bottom) / 2.0, compute_edge_profile(context.plan, edge, context.framing), cuts, compute_name(member.role), least);
}

/// The purlin on a station of a face at z: the purlin profile, top at the datum, each end cut on its support.
std::vector<std::shared_ptr<Element>> to_purlin(const Context& context, const Station& station, double z) {

    const std::vector<Polyline> profile = compute_role_profile(3, context.framing);
    const std::pair<double, double> size = wood_session::compute_size(profile);
    const Member purlin{0, station.line.to_direction(), 3, compute_rank(3), size.first, 0.0, -size.second};
    std::vector<Plane> cuts = compute_station_cuts(context, station.first, station.line.start(), station.line.to_direction(), purlin);
    for (const Plane& plane : compute_station_cuts(context, station.second, station.line.end(), -station.line.to_direction(), purlin))
        cuts.push_back(plane);

    return to_member(station.line.start(), station.line.end(), z - size.second / 2.0, profile, cuts, "purlin", size.first);
}

/// The column of a stack: the polygon at its head vertex swept from the foot at z_foot to z_top.
std::shared_ptr<Element> to_column(const Stack& stack, const std::vector<Point>& polygon, double z_foot, double z_top) {

    const Vector shift = compute_lift(stack.foot, 0.0) - compute_lift(stack.head, 0.0);
    std::vector<Point> section;
    for (const Point& point : polygon)
        section.push_back(compute_lift(point + shift, z_foot));

    return std::make_shared<wood_session::Column>(Line::from_points(compute_lift(stack.foot, z_foot), compute_lift(stack.head, z_top)), to_polyline(section), "column");
}

/// The near face of every core wall the head top at a vertex would reach into: a vertical plane on the wall's outer face, keeping the head's side.
std::vector<Plane> compute_head_cuts(const Level& level, size_t vertex, const std::vector<Point>& top, const Framing& framing) {

    const Point centre = compute_lift(*level.plan.vertex_point(vertex), 0.0);

    std::vector<Plane> cuts;
    for (const Polyline& core : level.cores) {
        const std::vector<Point> corners = to_loop(core);
        for (size_t i = 0; i < corners.size(); i++) {
            if (!is_against(top, centre, corners[i], corners[(i + 1) % corners.size()], framing.wall, 0.0))
                continue;

            const Vector normal = compute_direction(corners[i], corners[(i + 1) % corners.size()]).cross(Vector(0.0, 0.0, 1.0));
            const Vector towards = (centre - compute_lift(corners[i], 0.0)).dot(normal) > 0.0 ? normal : -normal;
            add_plane(cuts, Plane::from_point_normal(compute_lift(corners[i], 0.0) + towards * (framing.wall / 2.0), towards));
        }
    }

    return cuts;
}

/// Where a head stops at the deck edge on the perimeter: the side of every perimeter edge of a floor face at the vertex as a vertical plane keeping the inside, the side opening the floor first; at a re-entrant corner one plane through the corner of the two sides, square to their bisector.
std::vector<Plane> compute_edge_cuts(const Context& context, size_t vertex) {

    const Point centre = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    std::vector<Side> opens;
    std::vector<Side> closes;
    double turn = 0.0;
    for (const size_t face : context.plan.vertex_faces(vertex).value_or(std::vector<size_t>())) {
        if (context.plan.face_attribute(face, "floor").value_or(0.0) != 1.0)
            continue;

        const std::vector<size_t> loop = compute_loop(context.plan, face);
        const size_t count = loop.size();
        const size_t at = std::find(loop.begin(), loop.end(), vertex) - loop.begin();
        const Vector next = compute_direction(centre, *context.plan.vertex_point(loop[(at + 1) % count]));
        const Vector previous = compute_direction(centre, *context.plan.vertex_point(loop[(at + count - 1) % count]));
        const double angle = std::atan2(next.cross(previous)[2], next.dot(previous));
        turn += angle < 0.0 ? angle + Tolerance::TWO_PI : angle;

        for (const std::pair<size_t, size_t>& edge : {std::make_pair(vertex, loop[(at + 1) % count]), std::make_pair(loop[(at + count - 1) % count], vertex)}) {
            const Side side = compute_side(context, edge);
            if (context.plan.edge_attribute(edge, "boundary").value_or(0.0) == 1.0 && side.distance > 0.0)
                (edge.first == vertex ? opens : closes).push_back(side);
        }
    }

    std::vector<Plane> cuts;
    if (turn > Tolerance::PI + 1e-6 && opens.size() == 1 && closes.size() == 1) {
        const Point corner = compute_corner(centre, closes[0].outward, closes[0].distance, opens[0].outward, opens[0].distance);
        add_plane(cuts, Plane::from_point_normal(corner, compute_direction(corner, centre)));
    }
    if (turn > Tolerance::PI + 1e-6)
        return cuts;

    opens.insert(opens.end(), closes.begin(), closes.end());
    for (const Side& side : opens)
        add_plane(cuts, Plane::from_point_normal(centre + side.outward * side.distance, -side.outward));

    return cuts;
}

/// True when the members at a vertex only rest on its head: one member, or two running straight on, none cut against another there.
bool is_resting(const Context& context, size_t vertex) {

    const std::vector<Member> members = compute_members(context, vertex);

    return members.size() == 1 || (members.size() == 2 && members[0].direction.dot(members[1].direction) < -0.999);
}

/// A head block between two plan polygons at z_bottom and z_top with cuts.
std::shared_ptr<Element> to_block(const std::vector<Point>& bottom, const std::vector<Point>& top, double z_bottom, double z_top, const std::vector<Plane>& cuts, const std::string& name) {

    std::shared_ptr<wood_session::Block> block = std::make_shared<wood_session::Block>(std::vector<Polyline>{compute_lifted(to_polyline(bottom), z_bottom), compute_lifted(to_polyline(top), z_top)}, name);
    block->cuts = cuts;
    block->invalidate_geometry();

    return block;
}

/// The head at a vertex under node 0: the column section extruded where members only rest on it; where it carries cut member ends or the deck itself a frustum from the column to reach (capital 0) or a capital to halfway under a drop panel at reach (capital 1), cut back at the core walls and the deck edge.
std::vector<std::shared_ptr<Element>> to_head(const Context& context, const Level& level, size_t vertex, double z_bottom, double z_top) {

    if (is_resting(context, vertex))
        return {to_block(context.columns.at(vertex), context.columns.at(vertex), z_bottom, z_top, {}, "head")};

    const std::vector<std::vector<Point>> polygons = compute_head(level.plan, vertex, context.framing);
    std::vector<Plane> cuts = compute_head_cuts(level, vertex, polygons[2], context.framing);
    for (const Plane& plane : compute_edge_cuts(context, vertex))
        add_plane(cuts, plane);

    if (context.framing.capital == 0)
        return {to_block(polygons[0], polygons[2], z_bottom, z_top, cuts, "head")};

    const double z_middle = (z_bottom + z_top) / 2.0;

    return {to_block(polygons[0], polygons[1], z_bottom, z_middle, cuts, "head"), to_block(polygons[2], polygons[2], z_middle, z_top, cuts, "drop_panel")};
}

/// The deck plates of a face at z: loop 0 of the outline, holes and notches as features, one plate per panel strip.
std::vector<std::shared_ptr<Element>> to_deck(const Context& context, size_t face, double z, const Vector& span) {

    const double thickness = context.plan.face_attribute(face, "thickness").value_or(context.framing.deck);
    std::vector<std::shared_ptr<Element>> decks;
    for (const std::vector<Polyline>& loops : compute_panels(compute_outline(context, face), span, context.framing.panel)) {
        if (loops.empty() || compute_area(to_loop(loops[0])) <= 0.0)
            continue;

        std::shared_ptr<wood_session::Plate> deck = std::make_shared<wood_session::Plate>(compute_lifted(loops[0], z), compute_lifted(loops[0], z + thickness), "deck");
        if (loops.size() > 1) {
            for (const Polyline& ring : loops) {
                deck->features.bottom.push_back(compute_lifted(ring, z));
                deck->features.top.push_back(compute_lifted(ring, z + thickness));
            }
            deck->invalidate_geometry();
        }
        decks.push_back(deck);
    }

    return decks;
}

/// The heights a wall stands between.
struct Rise {
    double foot = 0.0; // Wall bottom: the deck top below, or the ground.
    double datum = 0.0; // The level datum the wall stands under.
    double top = 0.0; // Wall top: the member bottom, or the datum when nothing runs there.
};

/// The side of a facade wall at a vertex, bottom up: on the column face from foot to top, under the faces of a head that carries the deck; inward is +1 at the start of the wall line and -1 at its end.
std::vector<Point> compute_wall_side(const Context& context, size_t vertex, const Vector& along, double inward, const Rise& rise, const std::set<size_t>& heads) {

    const Point base = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    const double support = context.columns.count(vertex) ? compute_reach(context.columns.at(vertex), base, along * inward) : 0.0;
    if (!heads.count(vertex) || is_resting(context, vertex))
        return {compute_lift(base + along * (support * inward), rise.foot), compute_lift(base + along * (support * inward), rise.top)};

    const double reach = context.framing.reach;
    const double head_top = rise.datum + compute_head_top(context, vertex);
    const double middle = head_top - context.framing.head / 2.0;
    std::vector<Point> points = {compute_lift(base + along * (support * inward), rise.foot), compute_lift(base + along * (support * inward), head_top - context.framing.head)};
    if (context.framing.capital == 1) {
        points.push_back(compute_lift(base + along * ((support + reach) / 2.0 * inward), middle));
        points.push_back(compute_lift(base + along * (reach * inward), middle));
    }
    points.push_back(compute_lift(base + along * (reach * inward), head_top));
    if (rise.top > head_top + context.tolerance)
        points.push_back(compute_lift(base + along * (reach * inward), rise.top));

    return points;
}

/// The facade wall under a perimeter edge over a storey: between the column faces, chamfered along the head faces under node 0, from foot to the member bottom or the deck, thickness centred on the line.
std::shared_ptr<Element> to_wall(const Context& context, std::pair<size_t, size_t> edge, double foot, double datum, const std::set<size_t>& heads) {

    const Vector along = compute_direction(*context.plan.vertex_point(edge.first), *context.plan.vertex_point(edge.second));
    const Vector normal = along.cross(Vector(0.0, 0.0, 1.0));
    const Member member = compute_member(context, edge.first, edge.second);
    const Rise rise{foot, datum, datum + (member.role > 0 ? member.bottom : 0.0)};

    std::vector<Point> outline = compute_wall_side(context, edge.first, along, 1.0, rise, heads);
    const std::vector<Point> back = compute_wall_side(context, edge.second, along, -1.0, rise, heads);
    outline.insert(outline.end(), back.rbegin(), back.rend());
    const Polyline middle = to_polyline(outline);

    return std::make_shared<wood_session::Plate>(middle.translated(normal * (-context.framing.wall / 2.0)), middle.translated(normal * (context.framing.wall / 2.0)), "wall");
}

/// The core walls of a ring over a storey, pinwheel: each wall runs from the inner face of the wall before it to the outer face of the wall after it, from z_bottom to z_top.
std::vector<std::shared_ptr<Element>> to_core(const Polyline& ring, const Framing& framing, double z_bottom, double z_top) {

    const std::vector<Point> corners = to_loop(ring);
    const std::vector<Vector> normals = compute_normals(corners);
    const size_t count = corners.size();
    const double half = framing.wall / 2.0;

    std::vector<std::shared_ptr<Element>> walls;
    for (size_t i = 0; i < count; i++) {
        const size_t before = (i + count - 1) % count;
        const size_t after = (i + 1) % count;
        const Vector direction = compute_direction(corners[i], corners[after]);
        const Vector previous = compute_direction(corners[before], corners[i]);
        const Vector next = compute_direction(corners[after], corners[(i + 2) % count]);
        const Point start_inner = corners[i] - normals[before] * half;
        const Point end_outer = corners[after] + normals[after] * half;
        const std::vector<Point> quad = {
            compute_meet(corners[i] + normals[i] * half, direction, start_inner, previous),
            compute_meet(corners[i] + normals[i] * half, direction, end_outer, next),
            compute_meet(corners[i] - normals[i] * half, direction, end_outer, next),
            compute_meet(corners[i] - normals[i] * half, direction, start_inner, previous)
        };
        walls.push_back(std::make_shared<wood_session::Plate>(compute_lifted(to_polyline(quad), z_bottom), compute_lifted(to_polyline(quad), z_top), "core_wall"));
    }

    return walls;
}

/// Deck thickness at a level vertex: the thickest floor face there, 0 without a floor.
double compute_floor(const Mesh& plan, size_t vertex, const Framing& framing) {

    double thickness = 0.0;
    for (const size_t face : plan.vertex_faces(vertex).value_or(std::vector<size_t>()))
        if (plan.face_attribute(face, "floor").value_or(0.0) == 1.0)
            thickness = std::max(thickness, plan.face_attribute(face, "thickness").value_or(framing.deck));

    return thickness;
}

/// The plan vertex of a level within tolerance of a point, none otherwise.
std::optional<size_t> compute_vertex(const Mesh& plan, const Point& point, double tolerance) {

    for (const size_t vertex : plan.vertices())
        if (compute_distance(*plan.vertex_point(vertex), point) <= tolerance)
            return vertex;

    return std::nullopt;
}

/// A brace of a storey: the brace profile along the line, cut by the column faces at both ends, the deck top at the foot and the members or the deck at the head; lower holds the column sections at the feet, upper the level context at the head.
std::vector<std::shared_ptr<Element>> to_brace(const Line& line, const Level& lower, const Context& upper, double z_upper, double tolerance, const std::map<size_t, std::vector<Point>>& feet) {

    const Point foot = line.start()[2] < line.end()[2] ? line.start() : line.end();
    const Point head = line.start()[2] < line.end()[2] ? line.end() : line.start();
    const Vector direction = compute_direction(foot, head);
    const std::optional<size_t> under = compute_vertex(lower.plan, foot, tolerance);
    const double z_foot = lower.z + (under && upper.framing.node != 2 ? compute_floor(lower.plan, *under, upper.framing) : 0.0);

    std::vector<Plane> cuts = {Plane::from_point_normal(Point(0.0, 0.0, z_foot), Vector(0.0, 0.0, 1.0))};
    double top = z_upper;
    const std::optional<size_t> over = compute_vertex(upper.plan, head, tolerance);
    if (over) {
        top = z_upper + compute_head_top(upper, *over);
        if (upper.columns.count(*over))
            add_exit(cuts, upper.columns.at(*over), compute_lift(head, 0.0), -direction);
    }
    cuts.push_back(Plane::from_point_normal(Point(0.0, 0.0, top), Vector(0.0, 0.0, -1.0)));
    if (under && feet.count(*under))
        add_exit(cuts, feet.at(*under), compute_lift(foot, 0.0), direction);

    const Vector along = (head - foot).normalized();
    const Vector up = along.cross(direction.cross(Vector(0.0, 0.0, 1.0))).normalized();
    std::shared_ptr<wood_session::Beam> beam = std::make_shared<wood_session::Beam>(Polyline({foot - along * upper.framing.reach, head + along * upper.framing.reach}), compute_role_profile(6, upper.framing), std::vector<Vector>{up}, "brace");
    beam->cuts = cuts;

    return {beam};
}

// ═══════════════════════════════════════════════════════════════════════════
// Storeys
// ═══════════════════════════════════════════════════════════════════════════

/// Direction the deck of a face spans: across the girders under system 1, along them otherwise.
Vector compute_span(const Mesh& plan, size_t face, const Framing& framing) {

    const int span = static_cast<int>(plan.face_attribute(face, "span").value_or(framing.span));
    const int system = static_cast<int>(plan.face_attribute(face, "system").value_or(framing.system));
    const Vector girder = compute_family_direction(plan, compute_sides(compute_loop(plan, face)), span).value_or(Vector(1.0, 0.0, 0.0));

    return system == 1 ? Vector(0.0, 0.0, 1.0).cross(girder) : girder;
}

/// The column sections of stacks at their head vertices on a plan.
std::map<size_t, std::vector<Point>> compute_sections(const Mesh& plan, const std::vector<Stack>& stacks, const Framing& framing) {

    std::map<size_t, std::vector<Point>> columns;
    for (const Stack& stack : stacks)
        columns[stack.upper] = compute_column_polygon(plan, stack.upper, framing);

    return columns;
}

/// The column sections of stacks moved to their feet on the lower plan.
std::map<size_t, std::vector<Point>> compute_feet(const std::vector<Stack>& stacks, const std::map<size_t, std::vector<Point>>& columns) {

    std::map<size_t, std::vector<Point>> below;
    for (const Stack& stack : stacks)
        for (const Point& point : columns.at(stack.upper))
            below[stack.lower].push_back(point + (stack.foot - stack.head));

    return below;
}

/// The columns, core walls, facade walls and heads of a storey, in that order: columns from the deck top below (the datum under node 2) to the head bottom, the datum or the level.
std::vector<std::shared_ptr<Element>> to_uprights(const Building& building, size_t storey, const std::vector<Stack>& stacks, const Context& upper, const std::map<size_t, std::vector<Point>>& sections) {

    const Level& lower = building.levels[storey];
    const Level& level = building.levels[storey + 1];
    const Framing& framing = upper.framing;
    std::set<size_t> heads;
    for (const Stack& stack : stacks)
        if (framing.node == 0)
            heads.insert(stack.upper);

    std::vector<std::shared_ptr<Element>> elements;
    for (const Stack& stack : stacks) {
        const double foot = lower.z + (framing.node == 2 ? 0.0 : compute_floor(lower.plan, stack.lower, framing));
        const double top = level.z + (framing.node == 0 ? compute_head_top(upper, stack.upper) - framing.head : 0.0);
        elements.push_back(to_column(stack, sections.at(stack.upper), foot, top));
    }

    for (const Polyline& core : level.cores) {
        const double bottom = lower.z + (storey == 0 || lower.plan.faces().empty() ? 0.0 : framing.deck);
        for (const std::shared_ptr<Element>& wall : to_core(core, framing, bottom, level.z + framing.deck))
            elements.push_back(wall);
    }

    for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
        const double wall = level.plan.edge_attribute(edge, "wall").value_or(0.0);
        if (wall == 0.0 || (wall == 2.0 && !level.cores.empty()))
            continue;

        const double bottom = lower.z + std::max(compute_floor(lower.plan, edge.first, framing), compute_floor(lower.plan, edge.second, framing));
        elements.push_back(to_wall(upper, edge, bottom, level.z, heads));
        if (wall == 2.0)
            elements.back()->name = "core_wall";
    }

    if (framing.node == 0)
        for (const Stack& stack : stacks) {
            const double top = level.z + compute_head_top(upper, stack.upper);
            for (const std::shared_ptr<Element>& head : to_head(upper, level, stack.upper, top - framing.head, top))
                elements.push_back(head);
        }

    return elements;
}

/// The members, purlin stations and decks of a level; the ground decks too when asked.
std::vector<std::shared_ptr<Element>> to_floor(const Level& level, const Context& context, const Pattern& pattern, bool members) {

    std::vector<std::shared_ptr<Element>> elements;
    for (const std::pair<size_t, size_t>& edge : level.plan.edges())
        if (members && level.plan.edge_attribute(edge, "role").value_or(0.0) > 0.0)
            for (const std::shared_ptr<Element>& beam : to_beam(context, edge, level.z))
                elements.push_back(beam);

    for (const size_t face : level.plan.faces()) {
        if (!members || level.plan.face_attribute(face, "floor").value_or(0.0) != 1.0 || static_cast<int>(level.plan.face_attribute(face, "system").value_or(context.framing.system)) != 2)
            continue;

        for (const Station& station : compute_stations(context, face, pattern))
            for (const std::shared_ptr<Element>& purlin : to_purlin(context, station, level.z))
                elements.push_back(purlin);
    }

    for (const size_t face : level.plan.faces())
        if (level.plan.face_attribute(face, "floor").value_or(0.0) == 1.0)
            for (const std::shared_ptr<Element>& deck : to_deck(context, face, level.z, compute_span(level.plan, face, context.framing)))
                elements.push_back(deck);

    return elements;
}

} // namespace wood_grid::build

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Elements
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::shared_ptr<Element>> Building::to_elements(const Framing& given, size_t storey) const {

    Framing framing = given;
    if (framing.system == 0 && framing.node == 2)
        framing.node = 1;

    Building working = *this;
    for (Level& level : working.levels) {
        build::compute_slivers(level.plan, framing.wall, tolerance);
        build::compute_roles(level.plan, framing);
        build::compute_clearance(level, framing);
    }

    const std::vector<build::Stack> stacks = build::compute_columns(working, storey, framing);
    const std::vector<build::Stack> next = storey + 2 < working.levels.size() ? build::compute_columns(working, storey + 1, framing) : std::vector<build::Stack>();
    const Level& lower = working.levels[storey];
    const Level& level = working.levels[storey + 1];
    const std::map<size_t, std::vector<Point>> sections = build::compute_sections(level.plan, stacks, framing);
    const std::map<size_t, std::vector<Point>> rising = next.empty() ? std::map<size_t, std::vector<Point>>() : build::compute_feet(next, build::compute_sections(working.levels[storey + 2].plan, next, framing));
    std::map<size_t, std::vector<Point>> columns = sections;
    columns.insert(rising.begin(), rising.end());
    const std::map<size_t, std::vector<Point>> none;
    const std::map<size_t, std::vector<Point>> feet = build::compute_feet(stacks, sections);
    const joints::Context upper{level.plan, framing, tolerance, framing.node == 0 ? none : columns, framing.node == 2 ? rising : none, columns};
    const joints::Context standing{level.plan, framing, tolerance, columns, none, columns};
    const joints::Context ground{lower.plan, framing, tolerance, none, none, feet};

    std::vector<std::shared_ptr<Element>> elements = build::to_uprights(working, storey, stacks, standing, sections);
    for (const std::shared_ptr<Element>& element : build::to_floor(level, upper, pattern, true))
        elements.push_back(element);
    if (storey == 0)
        for (const std::shared_ptr<Element>& element : build::to_floor(lower, ground, pattern, false))
            elements.push_back(element);

    for (const Line& line : braces)
        if (std::min(line.start()[2], line.end()[2]) >= lower.z - tolerance && std::max(line.start()[2], line.end()[2]) <= level.z + tolerance)
            for (const std::shared_ptr<Element>& brace : build::to_brace(line, lower, upper, level.z, tolerance, framing.node == 0 ? none : feet))
                elements.push_back(brace);

    return elements;
}

void Building::to_session(wood_session::WoodSession& session, const Framing& framing) const {

    for (size_t storey = 0; storey + 1 < levels.size(); storey++) {
        const std::shared_ptr<TreeNode> group = session.add_group(fmt::format("storey_{}", storey));
        for (const std::shared_ptr<Element>& element : to_elements(framing, storey))
            session.add(element, group);
    }
}

} // namespace wood_grid
