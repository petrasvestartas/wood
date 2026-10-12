#include "pch.h"
#include "src/templates/truss/truss.h"

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

    // webs and posts: on the chords, mitered with their neighbours at every node
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

    // each end on the runs: top_side on a run it lies on or where several runs end; at a run's lone end the chord runs on over it
    for (const std::pair<Line, std::shared_ptr<Beam>>& member : members)
        for (const Point& end : {member.first.start(), member.first.end()})
            for (const std::pair<std::array<Point, 2>, std::shared_ptr<Beam>>& run : runs) {
                const bool at_end = at(end, run.first[0]) || at(end, run.first[1]);
                if (inside(end, run.first) || (at_end && ending(end) > 1))
                    add_butt(JointBeam::butt_top_side(*run.second, *member.second), run.second, member.second);
                else if (at_end)
                    add_butt(JointBeam::butt_top_side_end(*run.second, *member.second), run.second, member.second);
            }

    // the truss's plane, for the angles of the members around a node
    std::vector<Point> points;
    for (const Polyline& chord : guide.chords)
        for (const Point& point : chord.get_points())
            points.push_back(point);
    const Plane plane = Plane::from_points(points);

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

    // at each node, the members ending there sorted by the angle they leave at, each mitered with the next, the widest gap left open
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
        for (size_t k = 0; k < around.size(); k++)
            if (k != widest)
                add_butt(JointBeam::butt_side_side(*around[k].second, *around[(k + 1) % around.size()].second), around[k].second, around[(k + 1) % around.size()].second);
    }
}

} // namespace wood_truss
