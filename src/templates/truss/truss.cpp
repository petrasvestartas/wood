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

    // webs: on the chords, over end posts, mitered at shared nodes
    add_web_butts();
}

void Truss::add_input() {

    std::vector<Polyline> lines = guide.chords;
    for (const Line& web : guide.webs)
        lines.push_back(Polyline({web.start(), web.end()}));
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
    const std::vector<std::shared_ptr<Beam>> webs = get_elements_numbered<Beam>("web");

    // each web's ends: top_side on a run they lie on or end at with others; at a run's lone end, an end post, the chord runs on over it
    for (size_t w = 0; w < webs.size(); w++)
        for (const Point& end : {guide.webs[w].start(), guide.webs[w].end()})
            for (const std::pair<std::array<Point, 2>, std::shared_ptr<Beam>>& run : runs) {
                const bool on = inside(end, run.first);
                const bool at_end = at(end, run.first[0]) || at(end, run.first[1]);
                if (on || (at_end && ending(end) > 1))
                    add_butt(JointBeam::butt_top_side(*run.second, *webs[w]), run.second, webs[w]);
                else if (at_end)
                    add_butt(JointBeam::butt(*run.second, *webs[w]), run.second, webs[w]);
            }

    // webs ending at the same node, on the plane halfway
    for (size_t v = 0; v < webs.size(); v++)
        for (size_t w = v + 1; w < webs.size(); w++) {
            bool shared = false;
            for (const Point& a : {guide.webs[v].start(), guide.webs[v].end()})
                for (const Point& b : {guide.webs[w].start(), guide.webs[w].end()})
                    shared = shared || at(a, b);
            if (shared)
                add_butt(JointBeam::butt_side_side(*webs[v], *webs[w]), webs[v], webs[w]);
        }
}

} // namespace wood_truss
