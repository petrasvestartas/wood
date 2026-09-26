#include "pch.h"
#include "src/templates/grid/grid_plan.h"

using namespace session_cpp;

namespace wood_grid::build {

using namespace wood_grid::plan;

// ═══════════════════════════════════════════════════════════════════════════
// Columns
// ═══════════════════════════════════════════════════════════════════════════

/// The edges at a plan vertex.
std::vector<std::pair<size_t, size_t>> compute_star(const Mesh& plan, size_t vertex) {

    std::vector<std::pair<size_t, size_t>> edges;
    for (const size_t other : plan.vertex_neighbors(vertex).value_or(std::vector<size_t>()))
        edges.emplace_back(vertex, other);

    return edges;
}

/// The turn of the column profile at a plan vertex: the mean direction of the first pattern family's edges there, x where none passes.
double compute_turn(const Mesh& plan, size_t vertex) {

    const std::optional<Vector> axis = compute_family_direction(plan, compute_star(plan, vertex), 0);

    return axis ? std::atan2((*axis)[1], (*axis)[0]) : 0.0;
}

/// The plan section of the column at a vertex, counter-clockwise at z 0: the column profile centred there, its x axis along the first pattern family.
std::vector<Point> compute_column_polygon(const Mesh& plan, size_t vertex, const Framing& framing) {

    const double turn = compute_turn(plan, vertex);
    const Point centre = compute_lift(*plan.vertex_point(vertex), 0.0);

    std::vector<Point> points;
    for (const Point& point : to_loop(framing.profiles.column[0]))
        points.push_back(centre + Vector(point[0] * std::cos(turn) - point[1] * std::sin(turn), point[0] * std::sin(turn) + point[1] * std::cos(turn), 0.0));

    return points;
}

/// Column 0 on every vertex whose column section would stand in a core wall: the wall carries the deck there.
void compute_clearance(Level& level, const Framing& framing) {

    for (const size_t vertex : level.plan.vertices()) {
        if (level.plan.vertex_attribute(vertex, "column").value_or(0.0) != 1.0)
            continue;

        const Point centre = compute_lift(*level.plan.vertex_point(vertex), 0.0);
        double radius = 0.0;
        for (const Point& corner : compute_column_polygon(level.plan, vertex, framing))
            radius = std::max(radius, compute_distance(corner, centre));

        for (const Polyline& core : level.cores) {
            const std::vector<Point> corners = to_loop(core);
            for (size_t i = 0; i < corners.size(); i++)
                if (Line::from_points(corners[i], corners[(i + 1) % corners.size()]).closest_point(centre).second.distance(centre) < framing.wall / 2.0 + radius)
                    level.plan.set_vertex_attribute(vertex, "column", 0.0);
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

    const bool line = key.first >= 0.0 && (own.first == key.first || own.second == key.second || own.first == key.second || own.second == key.first);
    const bool section = upper.vertex_attribute(vertex, "boundary").value_or(0.0) == 1.0 && lower.vertex_attribute(other, "boundary").value_or(0.0) == 1.0;
    if ((line || section) && distance <= lean)
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

        if (match)
            stacks.push_back({*match, vertex, compute_lift(*lower.vertex_point(*match), 0.0), compute_lift(*upper.vertex_point(vertex), 0.0)});
        else
            upper.set_vertex_attribute(vertex, "column", 2.0);
    }

    return stacks;
}

// ═══════════════════════════════════════════════════════════════════════════
// Heads
// ═══════════════════════════════════════════════════════════════════════════

/// The side of a direction polygon that vanishes or turns back between its neighbours, none when every side stands: the diagonal side at the corner of a square column.
std::optional<size_t> compute_degenerate(const std::vector<Vector>& directions, const Point& centre, const std::vector<double>& distances) {

    const std::vector<Point> corners = to_loop(compute_polygon(directions, centre, distances));
    for (size_t j = 0; j < directions.size(); j++)
        if ((corners[j] - corners[(j + directions.size() - 1) % directions.size()]).dot(Vector(0.0, 0.0, 1.0).cross(directions[j])) <= 1e-6)
            return j;

    return std::nullopt;
}

/// The polygons of a head that carries the deck at a vertex, bottom, middle, top: the direction polygon circumscribing the column, halfway to reach and at reach; a direction whose side would not stand is left out.
std::vector<std::vector<Point>> compute_head(const Mesh& plan, size_t vertex, const Framing& framing) {

    const double turn = compute_turn(plan, vertex);
    const Point centre = compute_lift(*plan.vertex_point(vertex), 0.0);
    std::vector<Vector> directions = compute_directions(plan, vertex);
    std::vector<double> distances;
    for (const Vector& direction : directions) {
        const double angle = std::atan2(direction[1], direction[0]) - turn;
        distances.push_back(wood_session::compute_support(framing.profiles.column, Vector(std::cos(angle), std::sin(angle), 0.0)));
    }

    for (size_t round = 0; round < distances.size() && directions.size() > 3; round++) {
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

/// The outer face of every core wall the head top at a vertex would reach into, keeping the head's side.
std::vector<Plane> compute_head_cuts(const Level& level, size_t vertex, const std::vector<Point>& top, const Framing& framing) {

    const Point centre = compute_lift(*level.plan.vertex_point(vertex), 0.0);

    std::vector<Plane> cuts;
    for (const Polyline& core : level.cores) {
        const std::vector<Point> corners = to_loop(core);
        for (size_t i = 0; i < corners.size(); i++) {
            const Line side = Line::from_points(corners[i], corners[(i + 1) % corners.size()]);
            const Vector normal = side.to_direction().cross(Vector(0.0, 0.0, 1.0));
            const double gap = (centre - corners[i]).dot(normal) - framing.wall / 2.0;
            const double along = (centre - corners[i]).dot(side.to_direction());
            if (gap > 0.0 && gap < compute_reach(top, centre, -normal) && along > -framing.reach && along < side.length() + framing.reach)
                add_plane(cuts, Plane::from_point_normal(corners[i] + normal * (framing.wall / 2.0), normal));
        }
    }

    return cuts;
}

/// Where a head stops at the deck edge on the perimeter: the side of every perimeter edge of a floor face at the vertex as a vertical plane keeping the inside; at a re-entrant corner one plane through the corner of the two sides, square to their bisector.
std::vector<Plane> compute_edge_cuts(const Context& context, size_t vertex) {

    const Point centre = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    std::vector<std::pair<Vector, double>> sides;
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
            if (context.plan.edge_attribute(edge, "boundary").value_or(0.0) == 1.0)
                sides.emplace_back(compute_direction(*context.plan.vertex_point(edge.first), *context.plan.vertex_point(edge.second)).cross(Vector(0.0, 0.0, 1.0)), compute_side(context, edge));
        }
    }

    std::vector<Plane> cuts;
    if (turn > Tolerance::PI + 1e-6 && sides.size() == 2) {
        const Point corner = compute_corner(centre, sides[0].first, sides[0].second, sides[1].first, sides[1].second);
        add_plane(cuts, Plane::from_point_normal(corner, compute_direction(corner, centre)));
    }
    for (const std::pair<Vector, double>& side : sides)
        if (turn <= Tolerance::PI + 1e-6)
            add_plane(cuts, Plane::from_point_normal(centre + side.first * side.second, -side.first));

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
        return {to_block(context.standing.at(vertex), context.standing.at(vertex), z_bottom, z_top, {}, "head")};

    const std::vector<std::vector<Point>> polygons = compute_head(level.plan, vertex, context.framing);
    std::vector<Plane> cuts = compute_head_cuts(level, vertex, polygons[2], context.framing);
    for (const Plane& plane : compute_edge_cuts(context, vertex))
        add_plane(cuts, plane);
    if (context.framing.capital == 0)
        return {to_block(polygons[0], polygons[2], z_bottom, z_top, cuts, "head")};

    const double z_middle = (z_bottom + z_top) / 2.0;

    return {to_block(polygons[0], polygons[1], z_bottom, z_middle, cuts, "head"), to_block(polygons[2], polygons[2], z_middle, z_top, cuts, "drop_panel")};
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
        if (std::abs(speed) < 1e-9 && offset > 0.0)
            return -1.0;
        if (speed > 1e-9)
            low = std::max(low, offset / speed);
        else if (speed < -1e-9)
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

/// The member on a plan edge of the level at z: its role's profile, its top at the datum less its drop, its ends from compute_cuts at both vertices, split where it crosses a core and cut at the wall faces; a stub shorter than its width is dropped unless it bears at both ends.
std::vector<std::shared_ptr<Element>> to_beam(const Context& context, std::pair<size_t, size_t> edge, double z, const std::vector<Polyline>& outer) {

    const Member member = compute_member(context, edge.first, edge.second);
    const End first = compute_cuts(context, edge.first, edge.second);
    const End second = compute_cuts(context, edge.second, edge.first);
    const Point a = compute_lift(*context.plan.vertex_point(edge.first), 0.0) - member.direction * first.overrun;
    const Point b = compute_lift(*context.plan.vertex_point(edge.second), 0.0) + member.direction * second.overrun;
    std::vector<Plane> cuts = first.planes;
    cuts.insert(cuts.end(), second.planes.begin(), second.planes.end());

    std::vector<std::shared_ptr<Element>> beams;
    for (const Piece& piece : compute_pieces(Line::from_points(a, b), outer, false, context.tolerance)) {
        std::vector<Plane> own = cuts;
        for (const size_t e : {0, 1})
            if (piece.ring[e] >= 0)
                add_plane(own, compute_wall_face(outer, piece.ring[e], piece.side[e], e == 0 ? piece.line.start() : piece.line.end()));

        const bool bearing = (first.bearing || piece.ring[0] >= 0) && (second.bearing || piece.ring[1] >= 0);
        for (const std::shared_ptr<Element>& beam : to_member(piece.line.start(), piece.line.end(), z + (member.top + member.bottom) / 2.0, compute_profile(member.role, context.framing), own, compute_name(member.role), bearing ? context.tolerance : member.width))
            beams.push_back(beam);
    }

    return beams;
}

/// The column of a stack: the polygon at its head vertex swept from the foot at z_foot to z_top.
std::shared_ptr<Element> to_column(const Stack& stack, const std::vector<Point>& polygon, double z_foot, double z_top) {

    std::vector<Point> section;
    for (const Point& point : polygon)
        section.push_back(compute_lift(point + (stack.foot - stack.head), z_foot));

    return std::make_shared<wood_session::Column>(Line::from_points(compute_lift(stack.foot, z_foot), compute_lift(stack.head, z_top)), to_polyline(section), "column");
}

/// The deck plates of a face at z: loop 0 of the outline, holes and notches as features, one plate per panel strip.
std::vector<std::shared_ptr<Element>> to_deck(const std::vector<Polyline>& outline, double z, double thickness, const Vector& span, double panel) {

    std::vector<std::shared_ptr<Element>> decks;
    for (const std::vector<Polyline>& loops : compute_panels(outline, span, panel)) {
        std::shared_ptr<wood_session::Plate> deck = std::make_shared<wood_session::Plate>(compute_lifted(loops[0], z), compute_lifted(loops[0], z + thickness), "deck");
        for (const Polyline& ring : loops) {
            deck->features.bottom.push_back(compute_lifted(ring, z));
            deck->features.top.push_back(compute_lifted(ring, z + thickness));
        }
        deck->invalidate_geometry();
        decks.push_back(deck);
    }

    return decks;
}

/// The side of a facade wall at a vertex, bottom up: on the column face from foot to top, under the faces of a head that carries the deck; inward is +1 at the start of the wall line and -1 at its end.
std::vector<Point> compute_wall_side(const Context& context, size_t vertex, const Vector& along, double inward, double foot, double datum, double top) {

    const Point base = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    const double support = context.standing.count(vertex) ? compute_reach(context.standing.at(vertex), base, along * inward) : 0.0;
    if (context.framing.node != 0 || !context.standing.count(vertex) || is_resting(context, vertex))
        return {compute_lift(base + along * (support * inward), foot), compute_lift(base + along * (support * inward), top)};

    const double reach = context.framing.reach;
    const double head_top = datum + compute_head_top(context, vertex);
    const double middle = head_top - context.framing.head / 2.0;
    std::vector<Point> points = {compute_lift(base + along * (support * inward), foot), compute_lift(base + along * (support * inward), head_top - context.framing.head)};
    if (context.framing.capital == 1) {
        points.push_back(compute_lift(base + along * ((support + reach) / 2.0 * inward), middle));
        points.push_back(compute_lift(base + along * (reach * inward), middle));
    }
    points.push_back(compute_lift(base + along * (reach * inward), head_top));
    if (top > head_top + context.tolerance)
        points.push_back(compute_lift(base + along * (reach * inward), top));

    return points;
}

/// The facade wall under a perimeter edge over a storey: between the column faces, chamfered along the head faces under node 0, from foot to the member bottom or the datum, thickness centred on the line.
std::shared_ptr<Element> to_wall(const Context& context, std::pair<size_t, size_t> edge, double foot, double datum, const std::string& name) {

    const Vector along = compute_direction(*context.plan.vertex_point(edge.first), *context.plan.vertex_point(edge.second));
    const Vector normal = along.cross(Vector(0.0, 0.0, 1.0));
    const Member member = compute_member(context, edge.first, edge.second);
    const double top = datum + (member.role > 0 ? member.bottom : 0.0);

    std::vector<Point> outline = compute_wall_side(context, edge.first, along, 1.0, foot, datum, top);
    const std::vector<Point> back = compute_wall_side(context, edge.second, along, -1.0, foot, datum, top);
    outline.insert(outline.end(), back.rbegin(), back.rend());
    const Polyline middle = to_polyline(outline);

    return std::make_shared<wood_session::Plate>(middle.translated(normal * (-context.framing.wall / 2.0)), middle.translated(normal * (context.framing.wall / 2.0)), name);
}

/// Deck thickness at a level vertex: the framing's deck under a floor face there, 0 without a floor.
double compute_floor(const Mesh& plan, size_t vertex, const Framing& framing) {

    for (const size_t face : plan.vertex_faces(vertex).value_or(std::vector<size_t>()))
        if (plan.face_attribute(face, "floor").value_or(0.0) == 1.0)
            return framing.deck;

    return 0.0;
}

/// A brace of a storey: the brace profile along the line, cut by the column faces at both ends, the deck top at the foot and the members or the deck at the head.
std::vector<std::shared_ptr<Element>> to_brace(const Line& line, const Context& lower, const Context& upper, double z_lower, double z_upper) {

    const Point foot = line.start()[2] < line.end()[2] ? line.start() : line.end();
    const Point head = line.start()[2] < line.end()[2] ? line.end() : line.start();
    const Vector direction = compute_direction(foot, head);
    std::optional<size_t> under;
    std::optional<size_t> over;
    for (const size_t vertex : lower.plan.vertices())
        if (compute_distance(*lower.plan.vertex_point(vertex), foot) <= lower.tolerance)
            under = vertex;
    for (const size_t vertex : upper.plan.vertices())
        if (compute_distance(*upper.plan.vertex_point(vertex), head) <= upper.tolerance)
            over = vertex;

    const double z_foot = z_lower + (under && upper.framing.node != 2 ? compute_floor(lower.plan, *under, upper.framing) : 0.0);
    std::vector<Plane> cuts = {Plane::from_point_normal(Point(0.0, 0.0, z_foot), Vector(0.0, 0.0, 1.0))};
    cuts.push_back(Plane::from_point_normal(Point(0.0, 0.0, z_upper + (over ? compute_head_top(upper, *over) : 0.0)), Vector(0.0, 0.0, -1.0)));
    if (over && upper.framing.node != 0 && upper.standing.count(*over))
        add_exit(cuts, upper.standing.at(*over), compute_lift(head, 0.0), -direction);
    if (under && upper.framing.node != 0 && lower.rising.count(*under))
        add_exit(cuts, lower.rising.at(*under), compute_lift(foot, 0.0), direction);

    const Vector along = (head - foot).normalized();
    const Vector up = along.cross(direction.cross(Vector(0.0, 0.0, 1.0))).normalized();
    std::shared_ptr<wood_session::Beam> beam = std::make_shared<wood_session::Beam>(Polyline({foot - along * upper.framing.reach, head + along * upper.framing.reach}), compute_profile(4, upper.framing), std::vector<Vector>{up}, "brace");
    beam->cuts = cuts;

    return {beam};
}

// ═══════════════════════════════════════════════════════════════════════════
// Storeys
// ═══════════════════════════════════════════════════════════════════════════

/// Direction the deck of a face spans: across the girders under system 1, along them otherwise.
Vector compute_span(const Mesh& plan, size_t face, const Framing& framing) {

    const Vector girder = compute_family_direction(plan, compute_sides(compute_loop(plan, face)), static_cast<int>(plan.face_attribute(face, "span").value_or(framing.span))).value_or(Vector(1.0, 0.0, 0.0));

    return static_cast<int>(plan.face_attribute(face, "system").value_or(framing.system)) == 1 ? Vector(0.0, 0.0, 1.0).cross(girder) : girder;
}

/// The column sections of stacks at their head vertices on a plan, or moved to their feet on the lower plan when feet is true.
std::map<size_t, std::vector<Point>> compute_sections(const Mesh& plan, const std::vector<Stack>& stacks, const Framing& framing, bool feet) {

    std::map<size_t, std::vector<Point>> sections;
    for (const Stack& stack : stacks)
        for (const Point& point : compute_column_polygon(plan, stack.upper, framing))
            sections[feet ? stack.lower : stack.upper].push_back(feet ? point + (stack.foot - stack.head) : point);

    return sections;
}

/// The columns, core walls, facade walls and heads of a storey, in that order: columns from the deck top below (the datum under node 2) to the head bottom or the datum.
std::vector<std::shared_ptr<Element>> to_uprights(const Building& building, size_t storey, const std::vector<Stack>& stacks, const Context& upper) {

    const Level& lower = building.levels[storey];
    const Level& level = building.levels[storey + 1];
    const Framing& framing = upper.framing;

    std::vector<std::shared_ptr<Element>> elements;
    for (const Stack& stack : stacks) {
        const double foot = lower.z + (framing.node == 2 ? 0.0 : compute_floor(lower.plan, stack.lower, framing));
        const double top = level.z + (framing.node == 0 ? compute_head_top(upper, stack.upper) - framing.head : 0.0);
        elements.push_back(to_column(stack, upper.standing.at(stack.upper), foot, top));
    }

    for (const Polyline& core : level.cores) {
        const double bottom = lower.z + (storey == 0 ? 0.0 : framing.deck);
        for (const std::vector<Point>& quad : compute_core_quads(core, framing.wall))
            elements.push_back(std::make_shared<wood_session::Plate>(compute_lifted(to_polyline(quad), bottom), compute_lifted(to_polyline(quad), level.z + framing.deck), "core_wall"));
    }

    for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
        const double wall = level.plan.edge_attribute(edge, "wall").value_or(0.0);
        if (wall == 0.0)
            continue;

        const double bottom = lower.z + std::max(compute_floor(lower.plan, edge.first, framing), compute_floor(lower.plan, edge.second, framing));
        elements.push_back(to_wall(upper, edge, bottom, level.z, wall == 2.0 ? "core_wall" : "wall"));
    }

    for (const Stack& stack : stacks) {
        const double top = level.z + compute_head_top(upper, stack.upper);
        for (const std::shared_ptr<Element>& head : framing.node == 0 ? to_head(upper, level, stack.upper, top - framing.head, top) : std::vector<std::shared_ptr<Element>>())
            elements.push_back(head);
    }

    return elements;
}

/// The members, purlin stations and decks of a level; the decks alone when members is false.
std::vector<std::shared_ptr<Element>> to_floor(const Level& level, const Context& context, const std::map<size_t, std::vector<Polyline>>& outlines, bool members) {

    std::vector<Polyline> outer;
    for (const Polyline& core : level.cores)
        outer.push_back(compute_wall_ring(core, context.framing.wall));

    std::vector<std::shared_ptr<Element>> elements;
    for (const std::pair<size_t, size_t>& edge : level.plan.edges())
        if (members && level.plan.edge_attribute(edge, "role").value_or(0.0) > 0.0)
            for (const std::shared_ptr<Element>& beam : to_beam(context, edge, level.z, outer))
                elements.push_back(beam);

    for (const size_t face : level.plan.faces()) {
        if (!members || !outlines.count(face) || static_cast<int>(level.plan.face_attribute(face, "system").value_or(context.framing.system)) != 2)
            continue;

        const std::pair<double, double> size = wood_session::compute_size(compute_profile(3, context.framing));
        for (const Station& station : compute_stations(context, face, level.cores))
            for (const std::shared_ptr<Element>& purlin : to_member(station.line.start(), station.line.end(), level.z - size.second / 2.0, compute_profile(3, context.framing), station.cuts, "purlin", size.first))
                elements.push_back(purlin);
    }

    for (const std::pair<const size_t, std::vector<Polyline>>& outline : outlines)
        for (const std::shared_ptr<Element>& deck : to_deck(outline.second, level.z, context.framing.deck, compute_span(level.plan, outline.first, context.framing), context.framing.panel))
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
        build::compute_roles(level.plan, framing);
        build::compute_clearance(level, framing);
    }

    const std::vector<build::Stack> stacks = build::compute_columns(working, storey, framing);
    const std::vector<build::Stack> next = storey + 2 < working.levels.size() ? build::compute_columns(working, storey + 1, framing) : std::vector<build::Stack>();
    const Level& lower = working.levels[storey];
    const Level& level = working.levels[storey + 1];
    const std::map<size_t, std::vector<Point>> standing = build::compute_sections(level.plan, stacks, framing, false);
    const std::map<size_t, std::vector<Point>> rising = next.empty() ? std::map<size_t, std::vector<Point>>() : build::compute_sections(working.levels[storey + 2].plan, next, framing, true);
    const std::map<size_t, std::vector<Point>> feet = build::compute_sections(level.plan, stacks, framing, true);
    const std::map<size_t, std::vector<Point>> none;
    const build::Context upper{level.plan, framing, tolerance, standing, rising};
    const build::Context ground{lower.plan, framing, tolerance, none, feet};
    const std::map<size_t, std::vector<Polyline>> outlines = build::compute_outlines(upper, level.cores);

    std::vector<std::shared_ptr<Element>> elements = build::to_uprights(working, storey, stacks, upper);
    for (const std::shared_ptr<Element>& element : build::to_floor(level, upper, outlines, true))
        elements.push_back(element);
    if (storey == 0)
        for (const std::shared_ptr<Element>& element : build::to_floor(lower, ground, build::compute_outlines(ground, lower.cores), false))
            elements.push_back(element);

    for (const Line& line : braces)
        if (std::min(line.start()[2], line.end()[2]) >= lower.z - tolerance && std::max(line.start()[2], line.end()[2]) <= level.z + tolerance)
            for (const std::shared_ptr<Element>& brace : build::to_brace(line, ground, upper, lower.z, level.z))
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
