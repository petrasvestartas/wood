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

/// The turn of the column profile at a plan vertex.
double compute_turn(const Mesh& plan, size_t vertex) {

    const std::optional<Vector> axis = compute_family_direction(plan, compute_star(plan, vertex), 0);

    return axis ? std::atan2((*axis)[1], (*axis)[0]) : 0.0;
}

/// The plan section of the column at a vertex, counter-clockwise at z 0.
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
            for (size_t i = 0; i < corners.size(); i++) {
                const Line side = Line::from_points(corners[i], corners[(i + 1) % corners.size()]);
                const Point closest = side.closest_point(centre).second;
                if (closest.distance(centre) < framing.wall / 2.0 + radius)
                    level.plan.set_vertex_attribute(vertex, "column", 0.0);
            }
        }
    }
}

/// The two arrangement line ids that made a plan vertex, the identity used across levels.
std::pair<double, double> compute_identity(const Mesh& plan, size_t vertex) {
    return {plan.vertex_attribute(vertex, "line_a").value_or(-1.0), plan.vertex_attribute(vertex, "line_b").value_or(-1.0)};
}

/// How well a lower vertex carries an upper one, the lower the better.
double compute_carry(
    const Mesh& lower,
    size_t other,
    const Mesh& upper,
    size_t vertex,
    double lean,
    double tolerance
) {

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

/// Columns between levels k and k + 1.
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
            const double score = lower.vertex_attribute(other, "column").value_or(0.0) == 1.0 ? compute_carry(
                lower,
                other,
                upper,
                vertex,
                lean,
                building.tolerance
            ) : std::numeric_limits<double>::max();
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

/// The side of a direction polygon that vanishes or turns back between its neighbours, none when every side stands.
std::optional<size_t> compute_degenerate(const std::vector<Vector>& directions, const Point& centre, const std::vector<double>& distances) {

    const std::vector<Point> corners = to_loop(compute_polygon(directions, centre, distances));
    for (size_t j = 0; j < directions.size(); j++)
        if ((corners[j] - corners[(j + directions.size() - 1) % directions.size()]).dot(Vector(0.0, 0.0, 1.0).cross(directions[j])) <= 1e-6)
            return j;

    return std::nullopt;
}

/// The polygons of a head that carries the deck at a vertex, bottom, middle, top.
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

    const std::vector<double> reach(directions.size(), framing.reach);
    const Polyline bottom = compute_polygon(directions, centre, distances);
    const Polyline half = compute_polygon(directions, centre, middle);
    const Polyline top = compute_polygon(directions, centre, reach);

    return {to_loop(bottom), to_loop(half), to_loop(top)};
}

/// The outer face of every core wall the head top at a vertex would reach into, keeping the head's side.
std::vector<Plane> compute_head_cuts(
    const Level& level,
    size_t vertex,
    const std::vector<Point>& top,
    const Framing& framing
) {

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

/// Where a head stops at the deck edge on the perimeter.
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
            if (context.plan.edge_attribute(edge, "boundary").value_or(0.0) == 1.0) {
                const Vector along = compute_direction(*context.plan.vertex_point(edge.first), *context.plan.vertex_point(edge.second));
                sides.emplace_back(along.cross(Vector(0.0, 0.0, 1.0)), compute_side(context, edge));
            }
        }
    }

    std::vector<Plane> cuts;
    if (turn > Tolerance::PI + 1e-6 && sides.size() == 2) {
        const Point corner = compute_corner(
            centre,
            sides[0].first,
            sides[0].second,
            sides[1].first,
            sides[1].second
        );
        add_plane(cuts, Plane::from_point_normal(corner, compute_direction(corner, centre)));
    }
    for (const std::pair<Vector, double>& side : sides)
        if (turn <= Tolerance::PI + 1e-6)
            add_plane(cuts, Plane::from_point_normal(centre + side.first * side.second, -side.first));

    return cuts;
}

/// A closed mesh between a bottom and a top loop paired corner by corner, each face without the corners its pairs share.
Mesh compute_loft(const std::vector<Point>& bottom, const std::vector<Point>& top, double tolerance) {

    std::vector<std::vector<Point>> faces = {std::vector<Point>(bottom.rbegin(), bottom.rend()), top};
    for (size_t i = 0; i < bottom.size(); i++) {
        const size_t j = (i + 1) % bottom.size();
        faces.push_back({bottom[i], bottom[j], top[j], top[i]});
    }

    std::vector<std::vector<Point>> kept;
    for (const std::vector<Point>& face : faces) {
        std::vector<Point> corners;
        for (size_t i = 0; i < face.size(); i++)
            if (face[i].distance(face[(i + 1) % face.size()]) > tolerance)
                corners.push_back(face[i]);
        if (corners.size() >= 3)
            kept.push_back(corners);
    }

    return Mesh::from_polylines(kept, tolerance);
}

/// The head at a vertex where members arrive as a closed convex mesh between z_bottom and z_top, the datum at z.
Mesh compute_pyramid(
    const Context& context,
    size_t vertex,
    double z_bottom,
    double z_top,
    double z
) {

    const Point centre = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    const std::vector<Member> members = compute_members(context, vertex);
    double far = context.framing.reach;
    for (const Point& corner : context.standing.at(vertex))
        far = std::max(far, compute_distance(compute_lift(corner, 0.0), centre));
    for (const Member& member : members)
        far = std::max(far, compute_slope_start(context, vertex, member));

    // a box round the vertex, cut down to the pyramid one side at a time
    std::vector<Point> bottom;
    std::vector<Point> top;
    for (const Vector& corner : {Vector(-1.0, -1.0, 0.0), Vector(1.0, -1.0, 0.0), Vector(1.0, 1.0, 0.0), Vector(-1.0, 1.0, 0.0)}) {
        bottom.push_back(compute_lift(centre + corner * (4.0 * far), z_bottom));
        top.push_back(compute_lift(centre + corner * (4.0 * far), z_top));
    }
    Mesh solid = compute_loft(bottom, top, context.tolerance);
    for (const Vector& direction : compute_directions(context.plan, vertex)) {
        const double reach = compute_reach(context.standing.at(vertex), centre, direction);
        Plane side = Plane::from_point_normal(centre + direction * reach, direction);
        for (const Member& member : members)
            if (member.direction.dot(direction) > 1.0 - 1e-9)
                side = compute_slope_face(context, vertex, member);
        const Plane lifted_side = Plane::from_point_normal(side.origin() + Vector(0.0, 0.0, z), -side.z_axis());
        solid = solid.cut_by_plane(lifted_side);
    }

    // a chamfer between every two neighbouring members, as compas_grid
    for (size_t k = 0; k < members.size() && members.size() > 1; k++) {
        const Member& member = members[k];
        const Member& next = members[(k + 1) % members.size()];
        if (member.direction.cross(next.direction)[2] <= 1e-6)
            continue;

        const Vector left = Vector(0.0, 0.0, 1.0).cross(member.direction) * (member.width / 2.0);
        const Vector right = Vector(0.0, 0.0, 1.0).cross(next.direction) * (-next.width / 2.0);
        const double member_start = std::max(context.framing.reach, compute_slope_start(context, vertex, member));
        const Point a = compute_lift(centre + left + member.direction * member_start, z_bottom);
        const double next_start = std::max(context.framing.reach, compute_slope_start(context, vertex, next));
        const Point b = compute_lift(centre + right + next.direction * next_start, z_bottom);
        const Point c = compute_lift(
            compute_meet(
                centre + left,
                member.direction,
                centre + right,
                next.direction
            ),
            z_top
        );
        Vector normal = (b - a).cross(c - a).normalized();
        if (normal.dot(compute_lift(centre, z_bottom) - a) > 0.0)
            normal = -normal;
        solid = solid.cut_by_plane(Plane::from_point_normal(a, -normal));
    }

    return solid;
}

/// A head block between two plan polygons at z_bottom and z_top with cuts.
std::shared_ptr<Element> to_block(
    const std::vector<Point>& bottom,
    const std::vector<Point>& top,
    double z_bottom,
    double z_top,
    const std::vector<Plane>& cuts,
    const std::string& name
) {

    const Polyline lower = compute_lifted(to_polyline(bottom), z_bottom);
    const Polyline upper = compute_lifted(to_polyline(top), z_top);
    std::shared_ptr<wood_session::Block> block = std::make_shared<wood_session::Block>(std::vector<Polyline>{lower, upper}, name);
    block->cuts = cuts;
    block->invalidate_geometry();

    return block;
}

/// The head at a vertex under node 0.
std::vector<std::shared_ptr<Element>> to_head(
    const Context& context,
    const Level& level,
    size_t vertex,
    double z_bottom,
    double z_top
) {

    const std::vector<Member> members = compute_members(context, vertex);
    if (!members.empty()) {
        // the reach under every member says how far the core walls cut the head back
        std::vector<Point> extent = context.standing.at(vertex);
        for (const Member& member : members)
            extent.push_back(compute_lift(*context.plan.vertex_point(vertex), 0.0) + member.direction * context.framing.reach);

        std::shared_ptr<wood_session::Block> head = std::make_shared<wood_session::Block>(
            compute_pyramid(
                context,
                vertex,
                z_bottom,
                z_top,
                level.z
            ),
            "head"
        );
        head->cuts = compute_head_cuts(
            level,
            vertex,
            extent,
            context.framing
        );
        head->invalidate_geometry();
        return {head};
    }

    const std::vector<std::vector<Point>> polygons = compute_head(level.plan, vertex, context.framing);
    std::vector<Plane> cuts = compute_head_cuts(
        level,
        vertex,
        polygons[2],
        context.framing
    );
    for (const Plane& plane : compute_edge_cuts(context, vertex))
        add_plane(cuts, plane);
    if (context.framing.capital == 0)
        return {
            to_block(
                polygons[0],
                polygons[2],
                z_bottom,
                z_top,
                cuts,
                "head"
            )
        };

    const double z_middle = (z_bottom + z_top) / 2.0;

    return {
        to_block(
            polygons[0],
            polygons[1],
            z_bottom,
            z_middle,
            cuts,
            "head"
        ),
        to_block(
            polygons[2],
            polygons[2],
            z_middle,
            z_top,
            cuts,
            "drop_panel"
        )
    };
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

/// Beams of a profile along an axis at height z with cuts.
std::vector<std::shared_ptr<Element>> to_member(
    const Point& start,
    const Point& end,
    double z,
    const std::vector<Polyline>& profile,
    const std::vector<Plane>& cuts,
    const std::string& name,
    double least
) {

    std::vector<std::shared_ptr<Element>> beams;
    if (compute_kept(compute_lift(start, z), compute_lift(end, z), cuts) < least)
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

        const Polyline axis({compute_lift(start, z) + side * centre, compute_lift(end, z) + side * centre});
        std::shared_ptr<wood_session::Beam> beam = std::make_shared<wood_session::Beam>(
            axis, centred, std::vector<Vector>{Vector(0.0, 0.0, 1.0)}, name);
        beam->cuts = cuts;
        beams.push_back(beam);
    }

    return beams;
}

/// The member on a plan edge of the level at z.
std::vector<std::shared_ptr<Element>> to_beam(
    const Context& context,
    std::pair<size_t, size_t> edge,
    double z,
    const std::vector<Polyline>& outer
) {

    const Member member = compute_member(context, edge.first, edge.second);
    const End first = compute_cuts(context, edge.first, edge.second);
    const End second = compute_cuts(context, edge.second, edge.first);
    const Point a = compute_lift(*context.plan.vertex_point(edge.first), 0.0) - member.direction * first.overrun;
    const Point b = compute_lift(*context.plan.vertex_point(edge.second), 0.0) + member.direction * second.overrun;
    std::vector<Plane> cuts;
    for (const End* end : {&first, &second})
        for (const Plane& plane : end->planes)
            cuts.push_back(Plane::from_point_normal(plane.origin() + Vector(0.0, 0.0, z), plane.z_axis()));

    std::vector<std::shared_ptr<Element>> beams;
    for (const Piece& piece : compute_pieces(
        Line::from_points(a, b),
        outer,
        false,
        context.tolerance
    )) {
        std::vector<Plane> own = cuts;
        for (const size_t e : {0, 1})
            if (piece.ring[e] >= 0)
                add_plane(
                    own,
                    compute_wall_face(
                        outer,
                        piece.ring[e],
                        piece.side[e],
                        e == 0 ? piece.line.start() : piece.line.end()
                    )
                );

        const bool bearing = (first.bearing || piece.ring[0] >= 0) && (second.bearing || piece.ring[1] >= 0);
        const double axis_z = z + (member.top + member.bottom) / 2.0;
        const double least = bearing ? context.tolerance : member.width;
        for (const std::shared_ptr<Element>& beam : to_member(piece.line.start(), piece.line.end(),
            axis_z, compute_profile(member.role, context.framing), own, compute_name(member.role),
            least
        ))
            beams.push_back(beam);
    }

    return beams;
}

/// The column of a stack: the polygon at its head vertex swept from the foot at z_foot to z_top.
std::shared_ptr<Element> to_column(
    const Stack& stack,
    const std::vector<Point>& polygon,
    double z_foot,
    double z_top
) {

    std::vector<Point> section;
    for (const Point& point : polygon)
        section.push_back(compute_lift(point + (stack.foot - stack.head), z_foot));

    const Line axis = Line::from_points(compute_lift(stack.foot, z_foot), compute_lift(stack.head, z_top));

    return std::make_shared<wood_session::Column>(axis, to_polyline(section), "column");
}

/// The deck plates of a face at z.
std::vector<std::shared_ptr<Element>> to_deck(
    const std::vector<Polyline>& outline,
    double z,
    double thickness,
    const Vector& span,
    double panel
) {

    std::vector<std::shared_ptr<Element>> decks;
    for (const std::vector<Polyline>& loops : compute_panels(outline, span, panel)) {
        const Polyline bottom = compute_lifted(loops[0], z);
        const Polyline top = compute_lifted(loops[0], z + thickness);
        std::shared_ptr<wood_session::Plate> deck = std::make_shared<wood_session::Plate>(bottom, top, "deck");
        for (const Polyline& ring : loops) {
            deck->features.bottom.push_back(compute_lifted(ring, z));
            deck->features.top.push_back(compute_lifted(ring, z + thickness));
        }
        deck->invalidate_geometry();
        decks.push_back(deck);
    }

    return decks;
}

/// The side of a facade wall at a vertex, bottom up.
std::vector<Point> compute_wall_side(
    const Context& context,
    size_t vertex,
    const Vector& along,
    double inward,
    double foot,
    double datum,
    double top
) {

    const Point base = compute_lift(*context.plan.vertex_point(vertex), 0.0);
    const double support = context.standing.count(vertex) ? compute_reach(context.standing.at(vertex), base, along * inward) : 0.0;
    const std::vector<Point> plain = {compute_lift(base + along * (support * inward), foot), compute_lift(base + along * (support * inward), top)};
    if (context.framing.node != 0 || !context.standing.count(vertex))
        return plain;

    const double head_top = datum + compute_head_top(context, vertex);
    const double head_bottom = datum + compute_head_bottom(context, vertex);
    const std::vector<Member> members = compute_members(context, vertex);
    if (!members.empty()) {
        std::optional<Member> along_wall;
        for (const Member& member : members)
            if (member.direction.dot(along * inward) > 0.999)
                along_wall = member;
        if (!along_wall)
            return plain;

        // under the sloped side: out along its bottom, then up its slope to the member bottom
        const double start = compute_slope_start(context, vertex, *along_wall);
        const double reach = std::max(context.framing.reach, start);
        std::vector<Point> points = {plain[0], compute_lift(base + along * (support * inward), head_bottom), compute_lift(base + along * (reach * inward), head_bottom)};
        if (top > head_bottom + context.tolerance)
            points.push_back(compute_lift(base + along * ((reach - (reach - start) * (top - head_bottom) / (head_top - head_bottom)) * inward), top));
        return points;
    }

    const double reach = context.framing.reach;
    const double middle = head_top - context.framing.head / 2.0;
    std::vector<Point> points = {compute_lift(base + along * (support * inward), foot), compute_lift(base + along * (support * inward), head_bottom)};
    if (context.framing.capital == 1) {
        points.push_back(compute_lift(base + along * ((support + reach) / 2.0 * inward), middle));
        points.push_back(compute_lift(base + along * (reach * inward), middle));
    }
    points.push_back(compute_lift(base + along * (reach * inward), head_top));
    if (top > head_top + context.tolerance)
        points.push_back(compute_lift(base + along * (reach * inward), top));

    return points;
}

/// The facade wall under a perimeter edge over a storey.
std::shared_ptr<Element> to_wall(
    const Context& context,
    std::pair<size_t, size_t> edge,
    double foot,
    double datum,
    const std::string& name
) {

    const Vector along = compute_direction(*context.plan.vertex_point(edge.first), *context.plan.vertex_point(edge.second));
    const Vector normal = along.cross(Vector(0.0, 0.0, 1.0));
    const Member member = compute_member(context, edge.first, edge.second);
    const double top = datum + (member.role > 0 ? member.bottom : 0.0);

    std::vector<Point> outline = compute_wall_side(
        context,
        edge.first,
        along,
        1.0,
        foot,
        datum,
        top
    );
    const std::vector<Point> back = compute_wall_side(
        context,
        edge.second,
        along,
        -1.0,
        foot,
        datum,
        top
    );
    outline.insert(outline.end(), back.rbegin(), back.rend());
    const Polyline middle = to_polyline(outline);
    const Polyline back_face = middle.translated(normal * (-context.framing.wall / 2.0));
    const Polyline front_face = middle.translated(normal * (context.framing.wall / 2.0));

    return std::make_shared<wood_session::Plate>(back_face, front_face, name);
}

/// Deck thickness over the datum at a level vertex.
double compute_floor(const Mesh& plan, size_t vertex, const Framing& framing) {

    for (const size_t face : plan.vertex_faces(vertex).value_or(std::vector<size_t>()))
        if (plan.face_attribute(face, "floor").value_or(0.0) == 1.0 && !is_flush(plan, face, framing))
            return framing.deck;

    return 0.0;
}

/// A brace of a storey.
std::vector<std::shared_ptr<Element>> to_brace(
    const Line& line,
    const Context& lower,
    const Context& upper,
    double z_lower,
    double z_upper
) {

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
    const double z_head = z_upper + (over ? compute_under(upper, *over) : 0.0);
    cuts.push_back(Plane::from_point_normal(Point(0.0, 0.0, z_head), Vector(0.0, 0.0, -1.0)));
    if (over && upper.framing.node != 0 && upper.standing.count(*over))
        add_exit(
            cuts,
            upper.standing.at(*over),
            compute_lift(head, 0.0),
            -direction
        );
    if (under && upper.framing.node != 0 && lower.rising.count(*under))
        add_exit(
            cuts,
            lower.rising.at(*under),
            compute_lift(foot, 0.0),
            direction
        );

    const Vector along = (head - foot).normalized();
    const Vector up = along.cross(direction.cross(Vector(0.0, 0.0, 1.0))).normalized();
    const Polyline axis({foot - along * upper.framing.reach, head + along * upper.framing.reach});
    std::shared_ptr<wood_session::Beam> beam = std::make_shared<wood_session::Beam>(
        axis, compute_profile(4, upper.framing), std::vector<Vector>{up}, "brace");
    beam->cuts = cuts;

    return {beam};
}

// ═══════════════════════════════════════════════════════════════════════════
// Storeys
// ═══════════════════════════════════════════════════════════════════════════

/// Direction the deck of a face spans: across the girders under system 1, along them otherwise.
Vector compute_span(const Mesh& plan, size_t face, const Framing& framing) {

    const std::vector<std::pair<size_t, size_t>> sides = compute_sides(compute_loop(plan, face));
    const int span = static_cast<int>(plan.face_attribute(face, "span").value_or(framing.span));
    const Vector girder = compute_family_direction(plan, sides, span).value_or(Vector(1.0, 0.0, 0.0));
    const int face_system = static_cast<int>(plan.face_attribute(face, "system").value_or(framing.system));

    return face_system == 1 ? Vector(0.0, 0.0, 1.0).cross(girder) : girder;
}

/// The column sections of stacks at their head vertices on a plan, or moved to their feet on the lower plan when feet is true.
std::map<size_t, std::vector<Point>> compute_sections(
    const Mesh& plan,
    const std::vector<Stack>& stacks,
    const Framing& framing,
    bool feet
) {

    std::map<size_t, std::vector<Point>> sections;
    for (const Stack& stack : stacks)
        for (const Point& point : compute_column_polygon(plan, stack.upper, framing))
            sections[feet ? stack.lower : stack.upper].push_back(feet ? point + (stack.foot - stack.head) : point);

    return sections;
}

} // namespace wood_grid::build

namespace wood_grid {

using namespace wood_grid::plan;

// ═══════════════════════════════════════════════════════════════════════════
// Grid
// ═══════════════════════════════════════════════════════════════════════════

Grid::Grid(
    const Building& guide,
    const Framing& framing,
    const std::string& name
)
    : WoodSession(name),
      framing(compute_framing(framing)),
      guide(compute_guide(guide, this->framing)) {

    // plans: every level's member lines and column points at its datum
    add_plans();

    // storeys: the columns, walls, heads, members, purlins, decks and braces of every storey, under the level it caps
    for (size_t storey = 0; storey + 1 < this->guide.levels.size(); storey++)
        add_storey(storey);

    // contacts: an interaction between every two elements that touch, across the levels
    compute_face_contacts(0);
}

Framing Grid::compute_framing(const Framing& framing) {

    Framing built = framing;
    if (built.system == 0 && built.node == 2)
        built.node = 1;

    return built;
}

Building Grid::compute_guide(const Building& guide, const Framing& framing) {

    Building filled = guide;
    for (Level& level : filled.levels) {
        build::compute_roles(level.plan, framing);
        build::compute_clearance(level, framing);
    }

    return filled;
}

// ═══════════════════════════════════════════════════════════════════════════
// Plans
// ═══════════════════════════════════════════════════════════════════════════

void Grid::add_plans() {

    for (size_t l = 0; l < guide.levels.size(); l++) {
        const Level& level = guide.levels[l];
        const std::shared_ptr<TreeNode> plan = add_group(fmt::format("plan_{}", l), level_group(l));

        for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
            const Point start = compute_lift(*level.plan.vertex_point(edge.first), level.z);
            const Point end = compute_lift(*level.plan.vertex_point(edge.second), level.z);
            add_line(Line::from_points(start, end), plan);
        }

        for (const size_t vertex : level.plan.vertices())
            if (level.plan.vertex_attribute(vertex, "column").value_or(0.0) > 0.0)
                add_point(compute_lift(*level.plan.vertex_point(vertex), level.z), plan);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Storeys
// ═══════════════════════════════════════════════════════════════════════════

void Grid::add_storey(size_t storey) {

    // stacks: the columns of this storey and of the next, matched on a working copy as the joint rules read them
    Building working = guide;
    const std::vector<build::Stack> stacks = build::compute_columns(working, storey, framing);
    std::vector<build::Stack> next;
    if (storey + 2 < working.levels.size())
        next = build::compute_columns(working, storey + 1, framing);

    // sections: the columns standing under the level, rising from it and their feet on the level below
    const Level& lower = working.levels[storey];
    const Level& level = working.levels[storey + 1];
    const std::map<size_t, std::vector<Point>> standing = build::compute_sections(
        level.plan,
        stacks,
        framing,
        false
    );
    std::map<size_t, std::vector<Point>> rising;
    if (!next.empty())
        rising = build::compute_sections(
            working.levels[storey + 2].plan,
            next,
            framing,
            true
        );
    const std::map<size_t, std::vector<Point>> feet = build::compute_sections(
        level.plan,
        stacks,
        framing,
        true
    );
    const std::map<size_t, std::vector<Point>> none;
    const build::Context upper{level.plan, framing, guide.tolerance, standing, rising};
    const build::Context ground{lower.plan, framing, guide.tolerance, none, feet};

    // outlines: the deck loops of every floor face of the level, largest first
    const std::map<size_t, std::vector<Polyline>> outlines = build::compute_outlines(upper, level.cores);

    // columns: column_<i>_<level>, from the deck top below to the head bottom or the datum
    add_columns(
        working,
        storey,
        stacks,
        upper
    );

    // walls: core_wall_<i>_<level> in a pinwheel round every core, wall_<i>_<level> under the facade members
    add_walls(working, storey, upper);

    // heads: head_<i>_<level> on every column under node 0, a drop_panel over a stepped capital
    const std::map<size_t, std::shared_ptr<Element>> heads = add_heads(
        level,
        stacks,
        upper,
        storey + 1
    );

    // members: girder_, beam_ and purlin_<i>_<level> at the datum, their ends cut by the joint rules
    add_members(
        level,
        upper,
        outlines,
        storey + 1
    );

    // decks: deck_<i>_<level>, a flush deck cut by the heads at its corners; the ground decks with the first storey
    add_decks(
        level,
        upper,
        outlines,
        heads,
        storey + 1
    );
    if (storey == 0)
        add_decks(
            lower,
            ground,
            build::compute_outlines(ground, lower.cores),
            {},
            0
        );

    // braces: brace_<i>_<level>, every drawn brace standing in the storey
    add_braces(storey, ground, upper);
}

void Grid::add_columns(
    const Building& working,
    size_t storey,
    const std::vector<build::Stack>& stacks,
    const build::Context& upper
) {

    const Level& lower = working.levels[storey];
    const Level& level = working.levels[storey + 1];

    std::vector<std::shared_ptr<Element>> columns;
    for (const build::Stack& stack : stacks) {
        const double foot = lower.z + (framing.node == 2 ? 0.0 : build::compute_floor(lower.plan, stack.lower, framing));
        const double top = level.z + (framing.node == 0 ? build::compute_head_bottom(upper, stack.upper) : 0.0);
        const std::vector<Point>& section = upper.standing.at(stack.upper);
        columns.push_back(
            build::to_column(
                stack,
                section,
                foot,
                top
            )
        );
    }

    add_numbered(columns, storey + 1);
}

void Grid::add_walls(
    const Building& working,
    size_t storey,
    const build::Context& upper
) {

    const Level& lower = working.levels[storey];
    const Level& level = working.levels[storey + 1];

    std::vector<std::shared_ptr<Element>> walls;
    for (const Polyline& core : level.cores) {
        const double bottom = lower.z + (storey == 0 ? 0.0 : framing.deck);
        for (const std::vector<Point>& quad : build::compute_core_quads(core, framing.wall)) {
            const Polyline outline = to_polyline(quad);
            const Polyline base = compute_lifted(outline, bottom);
            const Polyline cap = compute_lifted(outline, level.z + framing.deck);
            walls.push_back(std::make_shared<wood_session::Plate>(base, cap, "core_wall"));
        }
    }

    for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
        const double wall = level.plan.edge_attribute(edge, "wall").value_or(0.0);
        if (wall == 0.0)
            continue;

        const double first_floor = build::compute_floor(lower.plan, edge.first, framing);
        const double second_floor = build::compute_floor(lower.plan, edge.second, framing);
        const double bottom = lower.z + std::max(first_floor, second_floor);
        walls.push_back(
            build::to_wall(
                upper,
                edge,
                bottom,
                level.z,
                wall == 2.0 ? "core_wall" : "wall"
            )
        );
    }

    add_numbered(walls, storey + 1);
}

std::map<size_t, std::shared_ptr<Element>> Grid::add_heads(
    const Level& level,
    const std::vector<build::Stack>& stacks,
    const build::Context& upper,
    size_t place
) {

    std::map<size_t, std::shared_ptr<Element>> heads;
    if (framing.node != 0)
        return heads;

    std::vector<std::shared_ptr<Element>> blocks;
    for (const build::Stack& stack : stacks) {
        const double bottom = level.z + build::compute_head_bottom(upper, stack.upper);
        const double top = level.z + build::compute_head_top(upper, stack.upper);
        const std::vector<std::shared_ptr<Element>> head = build::to_head(
            upper,
            level,
            stack.upper,
            bottom,
            top
        );
        heads[stack.upper] = head[0];
        blocks.insert(blocks.end(), head.begin(), head.end());
    }

    add_numbered(blocks, place);

    return heads;
}

void Grid::add_members(
    const Level& level,
    const build::Context& upper,
    const std::map<size_t, std::vector<Polyline>>& outlines,
    size_t place
) {

    std::vector<Polyline> outer;
    for (const Polyline& core : level.cores)
        outer.push_back(compute_wall_ring(core, framing.wall));

    // girders and beams on every plan edge with a role
    std::vector<std::shared_ptr<Element>> members;
    for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
        if (level.plan.edge_attribute(edge, "role").value_or(0.0) <= 0.0)
            continue;

        const std::vector<std::shared_ptr<Element>> beams = build::to_beam(
            upper,
            edge,
            level.z,
            outer
        );
        members.insert(members.end(), beams.begin(), beams.end());
    }

    // purlins at every station of a system 2 bay, their tops at the datum
    const std::vector<Polyline> profile = build::compute_profile(3, framing);
    const std::pair<double, double> size = wood_session::compute_size(profile);
    for (const size_t face : level.plan.faces()) {
        const int system = static_cast<int>(level.plan.face_attribute(face, "system").value_or(framing.system));
        if (!outlines.count(face) || system != 2)
            continue;

        for (const build::Station& station : build::compute_stations(upper, face, level.cores)) {
            const std::vector<std::shared_ptr<Element>> purlins = build::to_member(
                station.line.start(),
                station.line.end(),
                level.z - size.second / 2.0,
                profile,
                station.cuts,
                "purlin",
                size.first
            );
            members.insert(members.end(), purlins.begin(), purlins.end());
        }
    }

    add_numbered(members, place);
}

void Grid::add_decks(
    const Level& level,
    const build::Context& context,
    const std::map<size_t, std::vector<Polyline>>& outlines,
    const std::map<size_t, std::shared_ptr<Element>>& heads,
    size_t place
) {

    for (const std::pair<const size_t, std::vector<Polyline>>& outline : outlines) {
        const bool flush = build::is_flush(level.plan, outline.first, framing);
        const double deck_z = flush ? level.z - framing.deck : level.z;
        const Vector span = build::compute_span(level.plan, outline.first, framing);
        const std::vector<std::shared_ptr<Element>> decks = build::to_deck(
            outline.second,
            deck_z,
            framing.deck,
            span,
            framing.panel
        );
        add_numbered(decks, place);

        // a flush deck hangs between the members, the head at each corner cut out of it
        if (!flush)
            continue;

        for (const size_t vertex : build::compute_loop(level.plan, outline.first)) {
            if (!heads.count(vertex))
                continue;

            const double bottom = level.z + build::compute_head_bottom(context, vertex);
            const double top = level.z + build::compute_head_top(context, vertex);
            const Mesh pyramid = build::compute_pyramid(
                context,
                vertex,
                bottom,
                top,
                level.z
            );
            for (const std::shared_ptr<Element>& deck : decks) {
                const std::shared_ptr<wood_session::InteractionFeatureSolid> cut = std::make_shared<wood_session::InteractionFeatureSolid>(pyramid, wood_session::SolidOperation::subtract);
                add_interaction(heads.at(vertex), deck, cut);
            }
        }
    }
}

void Grid::add_braces(
    size_t storey,
    const build::Context& ground,
    const build::Context& upper
) {

    const double z_lower = guide.levels[storey].z;
    const double z_upper = guide.levels[storey + 1].z;

    std::vector<std::shared_ptr<Element>> braces;
    for (const Line& line : guide.braces) {
        const double low = std::min(line.start()[2], line.end()[2]);
        const double high = std::max(line.start()[2], line.end()[2]);
        if (low < z_lower - guide.tolerance || high > z_upper + guide.tolerance)
            continue;

        const std::vector<std::shared_ptr<Element>> brace = build::to_brace(
            line,
            ground,
            upper,
            z_lower,
            z_upper
        );
        braces.insert(braces.end(), brace.begin(), brace.end());
    }

    add_numbered(braces, storey + 1);
}

// ═══════════════════════════════════════════════════════════════════════════
// Groups
// ═══════════════════════════════════════════════════════════════════════════

void Grid::add_numbered(const std::vector<std::shared_ptr<Element>>& elements, size_t place) {

    for (const std::shared_ptr<Element>& element : elements) {
        const std::string kind = element->name;
        const std::shared_ptr<TreeNode> family = group_named(fmt::format("{}s_{}", kind, place), level_group(place));
        const size_t count = family->children().size();
        element->name = fmt::format("{}_{}_{}", kind, count, place);
        add(element, family);
    }
}

std::shared_ptr<TreeNode> Grid::level_group(size_t place) {
    return group_named(fmt::format("level_{}", place));
}

} // namespace wood_grid
