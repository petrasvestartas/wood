#include "pch.h"
#include "src/templates/truss/truss.h"
#include "wood_element_geometry.h"

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// Truss
// ═══════════════════════════════════════════════════════════════════════════

Truss::Truss(const TrussGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // input: the guide's lines as drawn
    add_input();

    // members: a beam per chord run and per web
    add_chords();
    add_webs();

    // chords: the runs mitered at kinks, butted at heels
    add_chord_butts();

    // webs and posts: on the chords, lapped where they cross, mitered with their neighbours at every node
    add_web_butts();
}

void Truss::add_input() {

    std::vector<Polyline> lines = guide.chords;
    for (const std::vector<Line>* members : {&guide.webs, &guide.posts})
        for (const Line& member : *members)
            lines.push_back(Polyline({member.start(), member.end()}));
    for (const Polyline& line : lines) {
        std::shared_ptr<Polyline> drawn = std::make_shared<Polyline>(line);
        drawn->linecolor = Color(0.85f, 0.1f, 0.1f);
        drawn->width = 4.0;
        add_polyline(drawn, group_named("input"));
    }
}

void Truss::add_chords() {

    for (size_t c = 0; c < guide.chords.size(); c++) {
        const std::vector<std::array<Point, 2>> runs = guide.runs(c);
        for (size_t s = 0; s < runs.size(); s++)
            add(std::make_shared<Beam>(Polyline({runs[s][0], runs[s][1]}), guide.chord_radius, fmt::format("chord_{}_{}", c, s)), group_named("chords"));
    }
}

void Truss::add_webs() {

    for (size_t w = 0; w < guide.webs.size(); w++)
        add(std::make_shared<Beam>(Polyline({guide.webs[w].start(), guide.webs[w].end()}), guide.web_radius, fmt::format("web_{}", w)), group_named("webs"));
    for (size_t p = 0; p < guide.posts.size(); p++)
        add(std::make_shared<Beam>(Polyline({guide.posts[p].start(), guide.posts[p].end()}), guide.chord_radius, fmt::format("post_{}", p)), group_named("webs"));
}

void Truss::add_butt(const std::shared_ptr<JointBeam>& butt, const std::shared_ptr<Beam>& a, const std::shared_ptr<Beam>& b) {

    if (!butt)
        return;
    butt->name = fmt::format("{}_{}_{}", butt->name, a->name, b->name);
    add(butt, group_named("butts"));
    add_interaction(butt, a, butt->interaction(0));
    add_interaction(butt, b, butt->interaction(1));
}

/// Whether a point lies at a node: within the tolerance of another.
static bool at(const Point& point, const Point& node) {
    return Point::distance(point, node) < Truss::NODE_TOLERANCE;
}

/// Whether a point lies on a run between its ends, clear of both.
static bool inside(const Point& point, const std::array<Point, 2>& run) {

    const Vector along = run[1] - run[0];
    const double t = (point - run[0]).dot(along) / along.dot(along);
    const Point foot = run[0] + along * t;

    return Point::distance(point, foot) < Truss::NODE_TOLERANCE && !at(point, run[0]) && !at(point, run[1]);
}

void Truss::add_chord_butts() {

    // every run with its chord and its ends, and its beam read back by name
    struct Run {
        size_t chord;
        std::array<Point, 2> ends;
        std::shared_ptr<Beam> beam;
    };
    std::vector<Run> runs;
    for (size_t c = 0; c < guide.chords.size(); c++) {
        const std::vector<std::array<Point, 2>> ends = guide.runs(c);
        for (size_t s = 0; s < ends.size(); s++)
            runs.push_back({c, ends[s], get_element_by_name<Beam>(fmt::format("chord_{}_{}", c, s))});
    }

    // kinks: consecutive runs of one chord on the miter
    for (size_t i = 0; i + 1 < runs.size(); i++)
        if (runs[i].chord == runs[i + 1].chord)
            add_butt(JointBeam::butt_side_side(*runs[i].beam, *runs[i + 1].beam), runs[i].beam, runs[i + 1].beam);

    // heels: runs of two chords meeting at their ends, the case read from them; a run's end on another chord's run, top_side on it
    for (size_t i = 0; i < runs.size(); i++)
        for (size_t j = i + 1; j < runs.size(); j++) {
            if (runs[i].chord == runs[j].chord)
                continue;
            bool heel = false;
            for (const Point& a : runs[i].ends)
                for (const Point& b : runs[j].ends)
                    heel = heel || at(a, b);
            if (heel)
                add_butt(JointBeam::butt(*runs[i].beam, *runs[j].beam), runs[i].beam, runs[j].beam);
            for (const Point& end : runs[j].ends)
                if (inside(end, runs[i].ends))
                    add_butt(JointBeam::butt_top_side(*runs[i].beam, *runs[j].beam), runs[i].beam, runs[j].beam);
            for (const Point& end : runs[i].ends)
                if (inside(end, runs[j].ends))
                    add_butt(JointBeam::butt_top_side(*runs[j].beam, *runs[i].beam), runs[j].beam, runs[i].beam);
        }
}

void Truss::add_web_butts() {

    // every run with its ends and beam, and how many runs end at a node
    std::vector<std::pair<std::array<Point, 2>, std::shared_ptr<Beam>>> runs;
    for (size_t c = 0; c < guide.chords.size(); c++) {
        const std::vector<std::array<Point, 2>> ends = guide.runs(c);
        for (size_t s = 0; s < ends.size(); s++)
            runs.emplace_back(ends[s], get_element_by_name<Beam>(fmt::format("chord_{}_{}", c, s)));
    }
    const auto ending = [&runs](const Point& node) {
        size_t count = 0;
        for (const std::pair<std::array<Point, 2>, std::shared_ptr<Beam>>& run : runs)
            count += at(node, run.first[0]) || at(node, run.first[1]);
        return count;
    };

    // the webs and the posts with their lines
    std::vector<std::pair<Line, std::shared_ptr<Beam>>> members;
    for (size_t w = 0; w < guide.webs.size(); w++)
        members.emplace_back(guide.webs[w], get_element_by_name<Beam>(fmt::format("web_{}", w)));
    for (size_t p = 0; p < guide.posts.size(); p++)
        members.emplace_back(guide.posts[p], get_element_by_name<Beam>(fmt::format("post_{}", p)));

    // the truss's plane, for the angles of the members around a node: through its two nodes farthest apart and the node farthest off the
    // line between them
    std::vector<Point> points;
    for (const Polyline& chord : guide.chords)
        for (const Point& point : chord.get_points())
            points.push_back(point);
    std::array<Point, 2> far = {points[0], points[0]};
    for (const Point& a : points)
        for (const Point& b : points)
            if (Point::distance(a, b) > Point::distance(far[0], far[1]))
                far = {a, b};
    const Vector along = (far[1] - far[0]).normalized();
    Point off = far[0];
    for (const Point& point : points)
        if ((point - far[0]).cross(along).magnitude() > (off - far[0]).cross(along).magnitude())
            off = point;
    const Plane plane = Plane::from_point_normal(far[0], along.cross(off - far[0]).normalized());

    // each end on the runs: top_side on a run it lies on or where several runs end; at a run's lone end the chord runs on over it; where two
    // runs end at a kink or a heel and the member stands outside the bend, one cut, what lies beyond both faces, so the end fits over the
    // ridge the runs make instead of losing a V to each face
    const auto flat = [&plane](const Vector& vector) { return std::array<double, 2>{vector.dot(plane.x_axis()), vector.dot(plane.y_axis())}; };
    const auto cross = [](const std::array<double, 2>& a, const std::array<double, 2>& b) { return a[0] * b[1] - a[1] * b[0]; };
    for (const std::pair<Line, std::shared_ptr<Beam>>& member : members)
        for (const Point& end : {member.first.start(), member.first.end()}) {
            std::vector<std::pair<std::shared_ptr<JointBeam>, std::shared_ptr<Beam>>> stops;
            std::vector<Vector> along;
            for (const std::pair<std::array<Point, 2>, std::shared_ptr<Beam>>& run : runs) {
                const bool at_start = at(end, run.first[0]);
                const bool at_end = at_start || at(end, run.first[1]);
                if (inside(end, run.first) || (at_end && ending(end) > 1)) {
                    stops.emplace_back(JointBeam::butt_top_side(*run.second, *member.second), run.second);
                    along.push_back(at_end ? (at_start ? run.first[1] - run.first[0] : run.first[0] - run.first[1]) : Vector(0.0, 0.0, 0.0));
                } else if (at_end) {
                    add_butt(JointBeam::butt_top_side_end(*run.second, *member.second), run.second, member.second);
                }
            }
            const Vector away = at(end, member.first.start()) ? member.first.end() - member.first.start() : member.first.start() - member.first.end();
            const bool bend = stops.size() == 2 && along[0].magnitude() > 0.0 && along[1].magnitude() > 0.0 && stops[0].first && stops[1].first
                && stops[0].first->butt_planes[1] && stops[1].first->butt_planes[1];
            const double turn = bend ? cross(flat(along[0]), flat(along[1])) : 0.0;
            const bool outside = bend && !(cross(flat(along[0]), flat(away)) * turn > 0.0 && cross(flat(away), flat(along[1])) * turn > 0.0);
            if (outside) {
                // each run's outer face, away from the other run, half a chord off its line, and the member losing what lies beyond both, kept on the
                // run nearer its line
                if (along[0].normalized().dot(away.normalized()) < along[1].normalized().dot(away.normalized())) {
                    std::swap(stops[0], stops[1]);
                    std::swap(along[0], along[1]);
                }
                const double size = 4.0 * away.magnitude();
                Mesh wedge;
                for (size_t r = 0; r < 2; r++) {
                    const Vector line = along[r].normalized();
                    Vector side = plane.z_axis().cross(line).normalized();
                    if (side.dot(along[1 - r]) > 0.0)
                        side = side * -1.0;
                    const Plane face = Plane::from_point_normal(end + side * guide.chord_radius, side);
                    const Polyline base = Polyline::rectangle(face.origin() - face.x_axis() * size - face.y_axis() * size, face.x_axis(), face.y_axis(), 2.0 * size, 2.0 * size);
                    const Mesh half = Mesh::loft({base}, {base.translated(side * -size)}, true);
                    wedge = wedge.number_of_faces() == 0 ? half : solid_boolean(wedge, half, SolidOperation::intersect);
                }
                stops[0].first->butt_solids[1].mesh = wedge;
                stops.pop_back();
            }
            for (const std::pair<std::shared_ptr<JointBeam>, std::shared_ptr<Beam>>& stop : stops)
                add_butt(stop.first, stop.second, member.second);
        }

    // each end on another web's or post's middle, as a K's on its vertical, top_side on it
    for (const std::pair<Line, std::shared_ptr<Beam>>& member : members)
        for (const Point& end : {member.first.start(), member.first.end()})
            for (const std::pair<Line, std::shared_ptr<Beam>>& other : members)
                if (inside(end, {other.first.start(), other.first.end()}))
                    add_butt(JointBeam::butt_top_side(*other.second, *member.second), other.second, member.second);

    // each two webs or posts whose lines cross inside both, a half lap at the crossing
    for (size_t i = 0; i < members.size(); i++)
        for (size_t j = i + 1; j < members.size(); j++) {
            const Line& a = members[i].first;
            const Line& b = members[j].first;
            double t_a = 0.0;
            double t_b = 0.0;
            if (!Intersection::line_line_parameters(a, b, t_a, t_b, NODE_TOLERANCE, false))
                continue;
            const Point crossing = a.point_at(t_a);
            if (t_a <= 0.0 || t_a >= 1.0 || t_b <= 0.0 || t_b >= 1.0 || !inside(crossing, {a.start(), a.end()}) || !inside(crossing, {b.start(), b.end()}))
                continue;
            add_butt(JointBeam::butt(*members[i].second, *members[j].second), members[i].second, members[j].second);
        }

    // every node a web or post ends at, once
    std::vector<Point> nodes;
    for (const std::pair<Line, std::shared_ptr<Beam>>& member : members)
        for (const Point& end : {member.first.start(), member.first.end()}) {
            bool known = false;
            for (const Point& node : nodes)
                known = known || at(end, node);
            if (!known)
                nodes.push_back(end);
        }

    // at each node, the members ending there sorted by the angle they leave at, each mitered with the next; on a chord the widest gap, the
    // chord's side, left open, at a node clear of the chords all the way round
    for (const Point& node : nodes) {
        std::vector<std::pair<double, std::shared_ptr<Beam>>> around;
        for (const std::pair<Line, std::shared_ptr<Beam>>& member : members) {
            const bool from_start = at(member.first.start(), node);
            if (!from_start && !at(member.first.end(), node))
                continue;
            const Vector away = from_start ? member.first.end() - member.first.start() : member.first.start() - member.first.end();
            around.emplace_back(std::atan2(away.dot(plane.y_axis()), away.dot(plane.x_axis())), member.second);
        }
        if (around.size() < 2)
            continue;
        std::sort(around.begin(), around.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        size_t widest = 0;
        double gap = -1.0;
        for (size_t k = 0; k < around.size(); k++) {
            const double next = k + 1 < around.size() ? around[k + 1].first : around[0].first + 2.0 * Tolerance::PI;
            if (next - around[k].first > gap) {
                gap = next - around[k].first;
                widest = k;
            }
        }
        bool on_chord = false;
        for (const std::pair<std::array<Point, 2>, std::shared_ptr<Beam>>& run : runs)
            on_chord = on_chord || at(node, run.first[0]) || at(node, run.first[1]) || inside(node, run.first);
        for (size_t k = 0; k < around.size(); k++)
            if (k != widest || (!on_chord && around.size() > 2))
                add_butt(JointBeam::butt_side_side(*around[k].second, *around[(k + 1) % around.size()].second), around[k].second, around[(k + 1) % around.size()].second);
    }
}

} // namespace wood_truss
