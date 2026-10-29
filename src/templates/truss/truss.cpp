#include "pch.h"
#include "src/templates/truss/truss.h"
#include "wood_element_geometry.h"

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// Truss
// ═══════════════════════════════════════════════════════════════════════════
Truss::Truss(const TrussGuide& guide, const std::string& name) : WoodSession(name), guide(guide) {

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
            add(std::make_shared<Beam>(
                    Polyline({runs[s][0], runs[s][1]}),
                    guide.chord_radii.empty() ? guide.chord_radius : guide.chord_radii[c],
                    fmt::format("chord_{}_{}", c, s)
                ),
                group_named("chords"));
    }
}

void Truss::add_webs() {

    for (size_t w = 0; w < guide.webs.size(); w++)
        add(std::make_shared<Beam>(
                Polyline({guide.webs[w].start(), guide.webs[w].end()}),
                guide.web_radii.empty() ? guide.web_radius : guide.web_radii[w],
                fmt::format("web_{}", w)
            ),
            group_named("webs"));
    for (size_t p = 0; p < guide.posts.size(); p++)
        add(std::make_shared<Beam>(
                Polyline({guide.posts[p].start(), guide.posts[p].end()}),
                guide.post_radii.empty() ? guide.chord_radius : guide.post_radii[p],
                fmt::format("post_{}", p)
            ),
            group_named("webs"));
}

void Truss::add_butt(
    const std::shared_ptr<JointBeam>& butt,
    const std::shared_ptr<Beam>& a,
    const std::shared_ptr<Beam>& b
) {

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

    return t > 0.0 && t < 1.0 && Point::distance(point, foot) < Truss::NODE_TOLERANCE &&
           !at(point, run[0]) && !at(point, run[1]);
}

void Truss::add_chord_butts() {

    struct Run {
        size_t chord;
        std::array<Point, 2> ends;
        std::shared_ptr<Beam> beam;
    };

    std::vector<Run> runs;
    for (size_t c = 0; c < guide.chords.size(); c++) {
        const std::vector<std::array<Point, 2>> ends = guide.runs(c);
        for (size_t s = 0; s < ends.size(); s++)
            runs.push_back({c, ends[s], get_element_by_name<Beam>(fmt::format("chord_{}_{}", c, s))}
            );
    }

    // kinks: consecutive runs of one chord on the miter
    for (size_t i = 0; i + 1 < runs.size(); i++)
        if (runs[i].chord == runs[i + 1].chord)
            add_butt(
                JointBeam::butt_side_side(*runs[i].beam, *runs[i + 1].beam),
                runs[i].beam,
                runs[i + 1].beam
            );

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
                    add_butt(
                        JointBeam::butt_top_side(*runs[i].beam, *runs[j].beam),
                        runs[i].beam,
                        runs[j].beam
                    );
            for (const Point& end : runs[i].ends)
                if (inside(end, runs[j].ends))
                    add_butt(
                        JointBeam::butt_top_side(*runs[j].beam, *runs[i].beam),
                        runs[j].beam,
                        runs[i].beam
                    );
        }
}

void Truss::add_web_butts() {

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

    std::vector<std::pair<Line, std::shared_ptr<Beam>>> members;
    for (size_t w = 0; w < guide.webs.size(); w++)
        members.emplace_back(guide.webs[w], get_element_by_name<Beam>(fmt::format("web_{}", w)));
    for (size_t p = 0; p < guide.posts.size(); p++)
        members.emplace_back(guide.posts[p], get_element_by_name<Beam>(fmt::format("post_{}", p)));

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

    const auto flat = [&plane](const Vector& vector) {
        return std::array<double, 2>{vector.dot(plane.x_axis()), vector.dot(plane.y_axis())};
    };
    const auto cross = [](const std::array<double, 2>& a, const std::array<double, 2>& b) {
        return a[0] * b[1] - a[1] * b[0];
    };

    std::vector<std::pair<Line, std::shared_ptr<Beam>>> lines;
    for (const std::pair<std::array<Point, 2>, std::shared_ptr<Beam>>& run : runs)
        lines.emplace_back(Line::from_points(run.first[0], run.first[1]), run.second);
    lines.insert(lines.end(), members.begin(), members.end());

    // the graph the lines draw: every end and crossing a vertex, each line split at the vertices on it into edges
    std::vector<Point> vertices;
    const auto vertex_of = [&vertices](const Point& point) {
        for (size_t n = 0; n < vertices.size(); n++)
            if (at(point, vertices[n]))
                return n;
        vertices.push_back(point);
        return vertices.size() - 1;
    };
    for (const std::pair<Line, std::shared_ptr<Beam>>& line : lines) {
        vertex_of(line.first.start());
        vertex_of(line.first.end());
    }
    for (size_t i = 0; i < lines.size(); i++)
        for (size_t j = i + 1; j < lines.size(); j++) {
            double t_a = 0.0;
            double t_b = 0.0;
            if (Intersection::line_line_parameters(
                    lines[i].first, lines[j].first, t_a, t_b, NODE_TOLERANCE, false
                ) &&
                t_a > 0.0 && t_a < 1.0 && t_b > 0.0 && t_b < 1.0)
                vertex_of(lines[i].first.point_at(t_a));
        }

    struct Edge {
        size_t from;
        size_t to;
        size_t line;
    };

    std::vector<Edge> edges;
    for (size_t l = 0; l < lines.size(); l++) {
        const Line& line = lines[l].first;
        std::vector<std::pair<double, size_t>> on = {
            {0.0, vertex_of(line.start())}, {1.0, vertex_of(line.end())}
        };
        for (size_t n = 0; n < vertices.size(); n++)
            if (inside(vertices[n], {line.start(), line.end()}))
                on.emplace_back(
                    (vertices[n] - line.start()).dot(line.to_vector()) /
                        line.to_vector().dot(line.to_vector()),
                    n
                );
        std::sort(on.begin(), on.end());
        for (size_t k = 0; k + 1 < on.size(); k++)
            edges.push_back({on[k].second, on[k + 1].second, l});
    }

    std::vector<int> rank(lines.size(), std::numeric_limits<int>::max());
    std::vector<bool> left(edges.size(), true);
    for (int layer = 0; std::find(left.begin(), left.end(), true) != left.end(); layer++) {
        std::vector<bool> walked(vertices.size(), false);
        std::vector<size_t> boundary;
        while (true) {
            std::optional<size_t> start;
            for (size_t e = 0; e < edges.size(); e++)
                if (left[e])
                    for (const size_t n : {edges[e].from, edges[e].to})
                        if (!walked[n] && (!start ||
                                           flat(vertices[n] - vertices[0])[0] <
                                               flat(vertices[*start] - vertices[0])[0] - 1e-9 ||
                                           (std::abs(
                                                flat(vertices[n] - vertices[0])[0] -
                                                flat(vertices[*start] - vertices[0])[0]
                                            ) <= 1e-9 &&
                                            flat(vertices[n] - vertices[0])[1] <
                                                flat(vertices[*start] - vertices[0])[1])))
                            start = n;
            if (!start)
                break;
            size_t current = *start;
            double back = Tolerance::PI;
            std::optional<size_t> first;
            for (size_t step = 0; step < 4 * edges.size(); step++) {
                std::optional<size_t> next;
                double turn = 0.0;
                for (size_t e = 0; e < edges.size(); e++) {
                    if (!left[e] || (edges[e].from != current && edges[e].to != current))
                        continue;
                    const std::array<double, 2> d = flat(
                        vertices[edges[e].from == current ? edges[e].to : edges[e].from] -
                        vertices[current]
                    );
                    double clockwise = std::fmod(
                        back - std::atan2(d[1], d[0]) + 4.0 * Tolerance::PI, 2.0 * Tolerance::PI
                    );
                    clockwise = clockwise < 1e-9 ? 2.0 * Tolerance::PI : clockwise;
                    if (!next || clockwise < turn) {
                        next = e;
                        turn = clockwise;
                    }
                }
                if (!next || (first && *next == *first && current == *start))
                    break;
                if (!first)
                    first = next;
                boundary.push_back(*next);
                const size_t other =
                    edges[*next].from == current ? edges[*next].to : edges[*next].from;
                const std::array<double, 2> d = flat(vertices[current] - vertices[other]);
                back = std::atan2(d[1], d[0]);
                current = other;
            }

            std::vector<size_t> stack = {*start};
            walked[*start] = true;
            while (!stack.empty()) {
                const size_t n = stack.back();
                stack.pop_back();
                for (size_t e = 0; e < edges.size(); e++)
                    if (left[e] && (edges[e].from == n || edges[e].to == n))
                        for (const size_t m : {edges[e].from, edges[e].to})
                            if (!walked[m]) {
                                walked[m] = true;
                                stack.push_back(m);
                            }
            }
        }
        for (const size_t e : boundary) {
            rank[edges[e].line] = std::min(rank[edges[e].line], layer);
            left[e] = false;
        }
        if (boundary.empty())
            break;
    }
    std::map<const Beam*, int> ranks;
    for (size_t l = 0; l < lines.size(); l++)
        ranks[lines[l].second.get()] = rank[l];

    std::vector<std::pair<std::shared_ptr<Beam>, std::shared_ptr<Beam>>> lets;
    const auto let_in =
        [&lets](const std::shared_ptr<Beam>& outer, const std::shared_ptr<Beam>& inner) {
            lets.emplace_back(outer, inner);
        };

    // each end on the runs: let into a run it lies on or into each run ending there; at a run's lone end the chord runs on over it unless the
    for (const std::pair<Line, std::shared_ptr<Beam>>& member : members)
        for (const Point& end : {member.first.start(), member.first.end()}) {
            std::vector<std::shared_ptr<Beam>> stops;
            std::vector<Vector> along;
            for (const std::pair<std::array<Point, 2>, std::shared_ptr<Beam>>& run : runs) {
                const bool at_start = at(end, run.first[0]);
                const bool at_end = at_start || at(end, run.first[1]);
                if (inside(end, run.first) || (at_end && ending(end) > 1)) {
                    stops.push_back(run.second);
                    along.push_back(
                        at_end
                            ? (at_start ? run.first[1] - run.first[0] : run.first[0] - run.first[1])
                            : Vector(0.0, 0.0, 0.0)
                    );
                } else if (at_end && ranks[member.second.get()] > ranks[run.second.get()]) {
                    let_in(run.second, member.second);
                } else if (at_end) {
                    add_butt(
                        JointBeam::butt_top_side_end(*run.second, *member.second),
                        run.second,
                        member.second
                    );
                }
            }
            const Vector away = at(end, member.first.start())
                                    ? member.first.end() - member.first.start()
                                    : member.first.start() - member.first.end();

            for (size_t k = 0; k < stops.size(); k++)
                if (along[k].magnitude() == 0.0) {
                    const std::array<double, 2> through =
                        flat(stops[k]->axis.get_lines()[0].to_vector().normalized());
                    const double side = cross(through, flat(away.normalized()));
                    for (size_t j = stops.size(); j-- > 0;)
                        if (along[j].magnitude() > 0.0 &&
                            cross(through, flat(along[j].normalized())) * side < -0.1) {
                            stops.erase(stops.begin() + j);
                            along.erase(along.begin() + j);
                            k -= j < k ? 1 : 0;
                        }
                }
            for (const std::shared_ptr<Beam>& stop : stops)
                let_in(stop, member.second);
        }

    for (const std::pair<Line, std::shared_ptr<Beam>>& member : members)
        for (const Point& end : {member.first.start(), member.first.end()})
            for (const std::pair<Line, std::shared_ptr<Beam>>& other : members)
                if (inside(end, {other.first.start(), other.first.end()}))
                    let_in(other.second, member.second);

    for (const std::pair<std::array<Point, 2>, std::shared_ptr<Beam>>& run : runs)
        for (const Point& end : run.first)
            for (const std::pair<Line, std::shared_ptr<Beam>>& member : members)
                if (inside(end, {member.first.start(), member.first.end()}))
                    let_in(member.second, run.second);

    std::vector<std::pair<Point, std::vector<size_t>>> crossings;
    for (size_t i = 0; i < lines.size(); i++)
        for (size_t j = i + 1; j < lines.size(); j++) {
            const Line& a = lines[i].first;
            const Line& b = lines[j].first;
            double t_a = 0.0;
            double t_b = 0.0;
            if (!Intersection::line_line_parameters(a, b, t_a, t_b, NODE_TOLERANCE, false))
                continue;
            const Point crossing = a.point_at(t_a);
            if (!inside(crossing, {a.start(), a.end()}) || !inside(crossing, {b.start(), b.end()}))
                continue;
            size_t c = 0;
            while (c < crossings.size() && !at(crossings[c].first, crossing))
                c++;
            if (c == crossings.size())
                crossings.push_back({crossing, {}});
            for (const size_t k : {i, j})
                if (std::find(crossings[c].second.begin(), crossings[c].second.end(), k) ==
                    crossings[c].second.end())
                    crossings[c].second.push_back(k);
        }

    // at each crossing every two members lapped, each keeping its own layer of the depth they share, a half each where two cross: one member
    for (const std::pair<Point, std::vector<size_t>>& crossing : crossings) {
        std::vector<size_t> crossed = crossing.second;
        std::sort(crossed.begin(), crossed.end());
        double depth = 2.0 * guide.chord_radius;
        for (const size_t k : crossed)
            depth = std::min(depth, 2.0 * lines[k].second->radius(0));
        for (size_t i = 0; i < crossed.size(); i++)
            for (size_t j = i + 1; j < crossed.size(); j++) {
                const double layer = -0.5 * depth + depth * static_cast<double>(i + 1) /
                                                        static_cast<double>(crossed.size());
                const Plane lap = Plane::from_point_normal(
                    crossing.first + plane.z_axis() * layer, plane.z_axis()
                );
                add_butt(
                    JointBeam::butt_cross(
                        *lines[crossed[i]].second, *lines[crossed[j]].second, lap
                    ),
                    lines[crossed[i]].second,
                    lines[crossed[j]].second
                );
            }
    }

    std::vector<Point> nodes;
    for (const std::pair<Line, std::shared_ptr<Beam>>& member : members)
        for (const Point& end : {member.first.start(), member.first.end()}) {
            bool known = false;
            for (const Point& node : nodes)
                known = known || at(end, node);
            if (!known)
                nodes.push_back(end);
        }

    // at each node, the members ending there sorted by the angle they leave at: each inner one stopping on every outer one, those of one rank
    for (const Point& node : nodes) {
        std::vector<std::pair<double, std::shared_ptr<Beam>>> around;
        for (const std::pair<Line, std::shared_ptr<Beam>>& member : members) {
            const bool from_start = at(member.first.start(), node);
            if (!from_start && !at(member.first.end(), node))
                continue;
            const Vector away = from_start ? member.first.end() - member.first.start()
                                           : member.first.start() - member.first.end();
            around.emplace_back(
                std::atan2(away.dot(plane.y_axis()), away.dot(plane.x_axis())), member.second
            );
        }
        std::sort(around.begin(), around.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });
        bool on_chord = false;
        for (const std::pair<std::array<Point, 2>, std::shared_ptr<Beam>>& run : runs)
            on_chord = on_chord || at(node, run.first[0]) || at(node, run.first[1]) ||
                       inside(node, run.first);

        std::vector<std::array<double, 2>> passing;
        for (const std::pair<Line, std::shared_ptr<Beam>>& line : lines)
            if (inside(node, {line.first.start(), line.first.end()}))
                passing.push_back(flat(line.first.to_vector().normalized()));
        const auto apart = [&passing, &cross](double a, double b) {
            for (const std::array<double, 2>& through : passing)
                if (cross(through, {std::cos(a), std::sin(a)}) *
                        cross(through, {std::cos(b), std::sin(b)}) <
                    0.0)
                    return true;
            return false;
        };

        for (const std::pair<double, std::shared_ptr<Beam>>& outer : around)
            for (const std::pair<double, std::shared_ptr<Beam>>& inner : around)
                if (ranks[outer.second.get()] < ranks[inner.second.get()])
                    let_in(outer.second, inner.second);

        std::set<int> layers;
        for (const std::pair<double, std::shared_ptr<Beam>>& member : around)
            layers.insert(ranks[member.second.get()]);
        for (const int layer : layers) {
            std::vector<std::pair<double, std::shared_ptr<Beam>>> same;
            for (const std::pair<double, std::shared_ptr<Beam>>& member : around)
                if (ranks[member.second.get()] == layer)
                    same.push_back(member);
            if (same.size() < 2)
                continue;
            size_t widest = 0;
            double gap = -1.0;
            for (size_t k = 0; k < same.size(); k++) {
                const double next =
                    k + 1 < same.size() ? same[k + 1].first : same[0].first + 2.0 * Tolerance::PI;
                if (next - same[k].first > gap) {
                    gap = next - same[k].first;
                    widest = k;
                }
            }
            for (size_t k = 0; k < same.size(); k++) {
                if (k == widest && (on_chord || same.size() <= 2))
                    continue;
                const std::pair<double, std::shared_ptr<Beam>>& a = same[k];
                const std::pair<double, std::shared_ptr<Beam>>& b = same[(k + 1) % same.size()];
                if (!apart(a.first, b.first))
                    add_butt(JointBeam::butt_side_side(*a.second, *b.second), a.second, b.second);
            }
        }
    }

    // the lets, inner rank by inner rank: the inner, ending inside the outer and never run on, loses the outer as already cut, so the boundary
    const Xform thicker =
        Xform::frame_to_world(plane.origin(), plane.x_axis(), plane.y_axis(), plane.z_axis()) *
        Xform::scale_xyz(1.0, 1.0, 1.02) *
        Xform::world_to_frame(plane.origin(), plane.x_axis(), plane.y_axis(), plane.z_axis());
    std::stable_sort(lets.begin(), lets.end(), [&ranks](const auto& a, const auto& b) {
        return ranks[a.second.get()] < ranks[b.second.get()];
    });
    while (!lets.empty()) {

        size_t next = 0;
        for (size_t k = lets.size(); k-- > 0;) {
            bool waiting = false;
            for (const std::pair<std::shared_ptr<Beam>, std::shared_ptr<Beam>>& other : lets)
                waiting = waiting || other.second == lets[k].first;
            if (!waiting)
                next = k;
        }
        const std::pair<std::shared_ptr<Beam>, std::shared_ptr<Beam>> let = lets[next];
        lets.erase(lets.begin() + next);
        const std::shared_ptr<JointBeam> joint = JointBeam::butt_top_side(*let.first, *let.second);
        if (joint) {
            joint->butt_extensions[1] = InteractionFeatureSolid();
            joint->butt_solids[1] = InteractionFeatureSolid(
                let.first->model_geometry_mesh().transformed(thicker), SolidOperation::subtract
            );
        }
        add_butt(joint, let.first, let.second);
    }
}

}
