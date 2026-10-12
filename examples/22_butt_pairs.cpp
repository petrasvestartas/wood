#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Drawing
// ═══════════════════════════════════════════════════════════════════════════

/// The user's line, as drawn: red over the computed solids, in the input group.
static void draw_line(WoodSession& scene, const Polyline& line) {

    std::shared_ptr<Polyline> drawn = std::make_shared<Polyline>(line);
    drawn->linecolor = Color(0.85f, 0.1f, 0.1f);
    drawn->width = 4.0;
    scene.add_polyline(drawn, scene.group_named("input"));
}

/// A member added with the line or the polygon it was drawn on: a beam's axis, a column's axis, a plate's mid-plane polygon.
static void add_drawn(WoodSession& scene, const std::shared_ptr<Element>& member) {

    scene.add(member);
    if (const std::shared_ptr<Beam> beam = std::dynamic_pointer_cast<Beam>(member))
        draw_line(scene, beam->axis);
    if (const std::shared_ptr<Column> column = std::dynamic_pointer_cast<Column>(member))
        draw_line(scene, Polyline({column->axis.start(), column->axis.end()}));
    if (const std::shared_ptr<Plate> plate = std::dynamic_pointer_cast<Plate>(member)) {
        std::vector<Point> points;
        for (size_t i = 0; i < plate->polylines[0].point_count(); i++)
            points.push_back(plate->polylines[0][i] + (plate->polylines[1][i] - plate->polylines[0][i]) * 0.5);
        draw_line(scene, Polyline(points));
    }
}

/// A pair, its butt or side_side butt passed to both, written to data/output/pb/butt_pair_<name>.pb.
static void pair(const std::string& name, const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b, bool side_side = false) {

    WoodSession scene("butt_pair_" + name);
    add_drawn(scene, a);
    add_drawn(scene, b);
    const std::shared_ptr<JointBeam> butt = side_side ? JointBeam::butt_side_side(*a, *b) : JointBeam::butt(*a, *b);
    butt->name = fmt::format("{}_{}_{}", butt->name, a->name, b->name);
    scene.add(butt);
    scene.add_interaction(butt, a, butt->interaction(0));
    scene.add_interaction(butt, b, butt->interaction(1));
    scene.pb_dump(pb_path("butt_pair_" + name));
}

/// A beam of 150 square on a line.
static std::shared_ptr<Beam> beam(const Point& start, const Point& end, const std::string& name) {

    return std::make_shared<Beam>(Polyline({start, end}), 75.0, name);
}

/// A wall 1500 high and 120 thick standing on a plan line.
static std::shared_ptr<Plate> wall(const Point& start, const Point& end, const std::string& name) {

    return Plate::from_line(Line::from_points(start, end), Vector(0.0, 0.0, 1.0), 1500.0, 120.0, name);
}

/// The plan point at angle_degrees from x, length away.
static Point polar(double angle_degrees, double length) {

    return Point(length * std::cos(angle_degrees * Tolerance::TO_RADIANS), length * std::sin(angle_degrees * Tolerance::TO_RADIANS), 0.0);
}

/// Butt pairs, beams, plates and both, one scene each: top_side, at an end drawn meeting, past and short, side_side out of plane and in plane, and crosses.
int main() {

    // beams: top_side square and at 60 degrees, top_side at an end drawn meeting, past and short, side_side_outplane at 120 and 90 degrees, side_side_inplane, a cross
    pair("beam_top_side", beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}, "through"), beam({0.0, 0.0, 0.0}, {0.0, 0.0, 1500.0}, "post"));
    pair("beam_top_side_oblique", beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}, "through"), beam({0.0, 0.0, 0.0}, {750.0, 0.0, 1300.0}, "strut"));
    pair("beam_top_side_end", beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "running"), beam({0.0, 0.0, 0.0}, {0.0, 0.0, 1500.0}, "butting"));
    pair("beam_top_side_end_past", beam({-150.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "running"), beam({0.0, 0.0, -150.0}, {0.0, 0.0, 1500.0}, "butting"));
    pair("beam_top_side_end_short", beam({100.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "running"), beam({0.0, 0.0, 100.0}, {0.0, 0.0, 1500.0}, "butting"));
    pair("beam_side_side_outplane", beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "left"), beam({0.0, 0.0, 0.0}, {-750.0, 0.0, 1300.0}, "right"));
    pair("beam_side_side_outplane_square", beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "left"), beam({0.0, 0.0, 0.0}, {0.0, 0.0, 1500.0}, "right"), true);
    pair("beam_side_side_inplane", beam({-1500.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, "left"), beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "right"));
    pair("beam_cross", beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}, "first"), beam({0.0, -1000.0, 0.0}, {0.0, 1000.0, 0.0}, "second"));

    // plates: top_side, top_side at an end, side_side_outplane at 120 degrees, side_side_inplane, a wall top_side on a floor, a cross
    pair("plate_top_side", wall({-1500.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "through"), wall({0.0, 0.0, 0.0}, {0.0, 2000.0, 0.0}, "butting"));
    pair("plate_top_side_end", wall({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0}, "running"), wall({0.0, 0.0, 0.0}, {0.0, 2000.0, 0.0}, "butting"));
    pair("plate_side_side_outplane", wall({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0}, "left"), wall({0.0, 0.0, 0.0}, polar(120.0, 2000.0), "right"));
    pair("plate_side_side_inplane", wall({-2000.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, "left"), wall({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0}, "right"));
    pair("plate_top_side_floor", Plate::from_rectangle({-1500.0, -1500.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 3000.0, 3000.0, 200.0, "floor"), wall({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}, "wall"));
    pair("plate_cross", wall({-1500.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "first"), wall({0.0, -1500.0, 0.0}, {0.0, 1500.0, 0.0}, "second"));

    // beams and plates, all top_side: a column under a slab, a wall on a beam, a beam into a wall at 60 degrees in plan
    pair("column_top_side_plate", Plate::from_rectangle({-1500.0, -1500.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 3000.0, 3000.0, 200.0, "slab"), Column::square(Line::from_points({0.0, 0.0, -2000.0}, {0.0, 0.0, 0.0}), Plane({-100.0, -100.0, -2000.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}), 200.0, "column"));
    pair("plate_top_side_beam", std::make_shared<Beam>(Polyline({{-1500.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}}), 100.0, "beam"), wall({-1200.0, 0.0, 0.0}, {1200.0, 0.0, 0.0}, "wall"));
    pair("beam_top_side_plate", wall({-1500.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "wall"), beam({0.0, 0.0, 800.0}, Point(0.0, 0.0, 800.0) + Vector(polar(-60.0, 1500.0)[0], polar(-60.0, 1500.0)[1], 0.0), "beam"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
JointBeam::butt and JointBeam::butt_side_side on pairs, the cases every assembly is made of, each drawn as the user draws it, beams and columns on lines, plates on mid-plane polygons (red): beams top_side square and at 60 degrees, top_side at an end drawn meeting, drawn past each other and drawn short, side_side_outplane at 120 and at 90 degrees, side_side_inplane and a cross; walls top_side, top_side at an end, side_side_outplane at 120 degrees, side_side_inplane, a wall top_side on a floor and a cross; a column under a slab, a wall on a beam and a beam into a wall at 60 degrees, all top_side. Each pair is its own scene, data/output/pb/butt_pair_<case>.pb, its joint named by its configuration.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 22_butt_pairs --parallel 6 && ./build/22_butt_pairs && ../bash/publish-scene.sh --target 22_butt_pairs

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
