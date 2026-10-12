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

/// A plate's mid-plane polygon, the outline the user drew it on.
static Polyline mid_polygon(const Plate& plate) {

    std::vector<Point> points;
    for (size_t i = 0; i < plate.polylines[0].point_count(); i++)
        points.push_back(plate.polylines[0][i] + (plate.polylines[1][i] - plate.polylines[0][i]) * 0.5);

    return Polyline(points);
}

/// The butt joined and passed to both members, running first.
static void join(WoodSession& scene, const std::shared_ptr<Element>& running, const std::shared_ptr<Element>& butting) {

    const std::shared_ptr<JointBeam> butt = JointBeam::butt(*running, *butting);
    butt->name = fmt::format("butt_{}_{}", running->name, butting->name);
    scene.add(butt);
    scene.add_interaction(butt, running, butt->interaction(0));
    scene.add_interaction(butt, butting, butt->interaction(1));
}

// ═══════════════════════════════════════════════════════════════════════════
// Cases
// ═══════════════════════════════════════════════════════════════════════════

/// A post on a sill and a brace from the post down to the sill at 55 degrees.
static void brace() {

    WoodSession scene("butt_brace");
    const std::shared_ptr<Beam> sill = std::make_shared<Beam>(Polyline({{-800.0, 0.0, 0.0}, {1400.0, 0.0, 0.0}}), 70.0, "brace_sill");
    const std::shared_ptr<Beam> post = std::make_shared<Beam>(Polyline({{0.0, 0.0, 0.0}, {0.0, 0.0, 2400.0}}), 70.0, "brace_post");
    const std::shared_ptr<Beam> strut = std::make_shared<Beam>(Polyline({{0.0, 0.0, 1500.0}, {1050.0, 0.0, 0.0}}), 60.0, "brace_strut");
    for (const std::shared_ptr<Beam>& beam : {sill, post, strut}) {
        scene.add(beam);
        draw_line(scene, beam->axis);
    }
    join(scene, sill, post);
    join(scene, post, strut);
    join(scene, sill, strut);

    scene.pb_dump(pb_path("butt_brace"));
}

/// A rafter at 30 degrees resting on a wall plate, its seat cut flat on the plate, and plumb cut against the ridge beam.
static void rafter() {

    WoodSession scene("butt_rafter");
    const double slope = std::tan(30.0 * Tolerance::TO_RADIANS);
    const std::shared_ptr<Beam> plate = std::make_shared<Beam>(Polyline({{0.0, -600.0, 0.0}, {0.0, 600.0, 0.0}}), 70.0, "rafter_wall_plate");
    const std::shared_ptr<Beam> ridge = std::make_shared<Beam>(Polyline({{1600.0, -600.0, 1600.0 * slope}, {1600.0, 600.0, 1600.0 * slope}}), 90.0, "rafter_ridge");
    const std::shared_ptr<Beam> spar = std::make_shared<Beam>(Polyline({{-500.0, 0.0, -500.0 * slope}, {1600.0, 0.0, 1600.0 * slope}}), 50.0, "rafter_spar");
    for (const std::shared_ptr<Beam>& beam : {plate, ridge, spar}) {
        scene.add(beam);
        draw_line(scene, beam->axis);
    }
    join(scene, plate, spar);
    join(scene, ridge, spar);

    scene.pb_dump(pb_path("butt_rafter"));
}

/// A beam resting on a column: the column cut flat on the beam's underside.
static void beam_on_column() {

    WoodSession scene("butt_beam_on_column");
    const std::shared_ptr<Column> column = Column::square(Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 2400.0}), Plane({-100.0, -100.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}), 200.0, "column_under_beam");
    const std::shared_ptr<Beam> beam = std::make_shared<Beam>(Polyline({{-1000.0, 0.0, 2400.0}, {1000.0, 0.0, 2400.0}}), 100.0, "beam_on_column");
    scene.add(column);
    scene.add(beam);
    draw_line(scene, Polyline({column->axis.start(), column->axis.end()}));
    draw_line(scene, beam->axis);
    join(scene, beam, column);

    scene.pb_dump(pb_path("butt_beam_on_column"));
}

/// A beam framing into a column at 60 degrees in plan: the beam's end cut flat on the column's face, obliquely.
static void beam_into_column() {

    WoodSession scene("butt_beam_into_column");
    const std::shared_ptr<Column> column = Column::square(Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 2400.0}), Plane({-100.0, -100.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}), 200.0, "column_beside_beam");
    const Vector along(std::cos(30.0 * Tolerance::TO_RADIANS), std::sin(30.0 * Tolerance::TO_RADIANS), 0.0);
    const std::shared_ptr<Beam> beam = std::make_shared<Beam>(Polyline({{0.0, 0.0, 1600.0}, Point(0.0, 0.0, 1600.0) + along * 1400.0}), 80.0, "beam_into_column");
    scene.add(column);
    scene.add(beam);
    draw_line(scene, Polyline({column->axis.start(), column->axis.end()}));
    draw_line(scene, beam->axis);
    join(scene, column, beam);

    scene.pb_dump(pb_path("butt_beam_into_column"));
}

/// Joists into a wall, one square and one at 30 degrees in plan: each joist's end cut flat on the wall's face.
static void joists_into_wall() {

    WoodSession scene("butt_joists_into_wall");
    const std::shared_ptr<Plate> wall = Plate::from_line(Line::from_points({0.0, -800.0, 0.0}, {0.0, 800.0, 0.0}), {0.0, 0.0, 1.0}, 2400.0, 160.0, "wall_with_joists");
    const std::shared_ptr<Beam> square = std::make_shared<Beam>(Polyline({{0.0, -400.0, 1800.0}, {1500.0, -400.0, 1800.0}}), 60.0, "joist_square");
    const std::shared_ptr<Beam> skew = std::make_shared<Beam>(Polyline({{0.0, 300.0, 1800.0}, {1300.0, 1050.0, 1800.0}}), 60.0, "joist_skew");
    scene.add(wall);
    draw_line(scene, mid_polygon(*wall));
    for (const std::shared_ptr<Beam>& joist : {square, skew}) {
        scene.add(joist);
        draw_line(scene, joist->axis);
        join(scene, wall, joist);
    }

    scene.pb_dump(pb_path("butt_joists_into_wall"));
}

/// A wall standing on a beam: the wall cut flat on the beam's top.
static void wall_on_beam() {

    WoodSession scene("butt_wall_on_beam");
    const std::shared_ptr<Beam> beam = std::make_shared<Beam>(Polyline({{-1200.0, 0.0, 0.0}, {1200.0, 0.0, 0.0}}), 120.0, "beam_under_wall");
    const std::shared_ptr<Plate> wall = Plate::from_line(Line::from_points({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}), {0.0, 0.0, 1.0}, 2000.0, 120.0, "wall_on_beam");
    scene.add(beam);
    scene.add(wall);
    draw_line(scene, beam->axis);
    draw_line(scene, mid_polygon(*wall));
    join(scene, beam, wall);

    scene.pb_dump(pb_path("butt_wall_on_beam"));
}

/// A column above and a column below a slab: each cut flat on the slab's face.
static void columns_on_slab() {

    WoodSession scene("butt_columns_on_slab");
    const std::shared_ptr<Plate> slab = Plate::from_rectangle({-1000.0, -1000.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 2000.0, 2000.0, 200.0, "slab");
    const std::shared_ptr<Column> above = Column::square(Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 1800.0}), Plane({-100.0, -100.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}), 200.0, "column_above_slab");
    const std::shared_ptr<Column> below = Column::square(Line::from_points({0.0, 0.0, -1800.0}, {0.0, 0.0, 0.0}), Plane({-100.0, -100.0, -1800.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}), 200.0, "column_below_slab");
    scene.add(slab);
    draw_line(scene, mid_polygon(*slab));
    for (const std::shared_ptr<Column>& column : {above, below}) {
        scene.add(column);
        draw_line(scene, Polyline({column->axis.start(), column->axis.end()}));
        join(scene, slab, column);
    }

    scene.pb_dump(pb_path("butt_columns_on_slab"));
}

/// Two walls meeting at 120 degrees in plan: the butting wall cut on the running wall's inner face, the running wall run on to the other's outer face.
static void walls_at_angle() {

    WoodSession scene("butt_walls_at_120");
    const Point corner(0.0, 0.0, 0.0);
    const Vector turned(std::cos(120.0 * Tolerance::TO_RADIANS), std::sin(120.0 * Tolerance::TO_RADIANS), 0.0);
    const std::shared_ptr<Plate> running = Plate::from_line(Line::from_points(corner + Vector(1600.0, 0.0, 0.0), corner), {0.0, 0.0, 1.0}, 2400.0, 160.0, "wall_running");
    const std::shared_ptr<Plate> butting = Plate::from_line(Line::from_points(corner, corner + turned * 1600.0), {0.0, 0.0, 1.0}, 2400.0, 160.0, "wall_butting");
    for (const std::shared_ptr<Plate>& wall : {running, butting}) {
        scene.add(wall);
        draw_line(scene, mid_polygon(*wall));
    }
    join(scene, running, butting);

    scene.pb_dump(pb_path("butt_walls_at_120"));
}

/// A wall under a roof sloped at 25 degrees: the wall's top cut flat on the roof's underside.
static void wall_under_roof() {

    WoodSession scene("butt_wall_under_roof");
    const double angle = 25.0 * Tolerance::TO_RADIANS;
    const Vector up_slope(std::cos(angle), 0.0, std::sin(angle));
    const Vector across = up_slope.cross(Vector(0.0, 1.0, 0.0));
    const Point eave = Point(0.0, -1000.0, 2400.0) - up_slope * 1000.0 + across * 100.0;
    const std::shared_ptr<Plate> roof = Plate::from_rectangle(eave, up_slope, {0.0, 1.0, 0.0}, 2400.0, 2000.0, 200.0, "roof_sloped");
    const std::shared_ptr<Plate> wall = Plate::from_line(Line::from_points({0.0, -800.0, 0.0}, {0.0, 800.0, 0.0}), {0.0, 0.0, 1.0}, 2400.0, 160.0, "wall_under_roof");
    for (const std::shared_ptr<Plate>& plate : {roof, wall}) {
        scene.add(plate);
        draw_line(scene, mid_polygon(*plate));
    }
    join(scene, roof, wall);

    scene.pb_dump(pb_path("butt_wall_under_roof"));
}

/// A joist drawn 200 short of a wall: run on to the wall's face.
static void short_joist() {

    WoodSession scene("butt_short_joist");
    const std::shared_ptr<Plate> wall = Plate::from_line(Line::from_points({0.0, -800.0, 0.0}, {0.0, 800.0, 0.0}), {0.0, 0.0, 1.0}, 2400.0, 160.0, "wall_short_joist");
    const std::shared_ptr<Beam> joist = std::make_shared<Beam>(Polyline({{280.0, 0.0, 1800.0}, {1600.0, 0.0, 1800.0}}), 60.0, "joist_short");
    scene.add(wall);
    scene.add(joist);
    draw_line(scene, mid_polygon(*wall));
    draw_line(scene, joist->axis);
    join(scene, wall, joist);

    scene.pb_dump(pb_path("butt_short_joist"));
}

/// Two posts on a curved sill, its axis a quarter circle of radius 2000 in eight segments: each post cut flat on the segment it stands on.
static void curved_sill() {

    WoodSession scene("butt_curved_sill");
    std::vector<Point> arc;
    for (int i = 0; i <= 8; i++) {
        const double angle = 0.5 * Tolerance::PI * i / 8.0;
        arc.emplace_back(2000.0 * std::cos(angle), 2000.0 * std::sin(angle), 0.0);
    }
    const std::shared_ptr<Beam> sill = std::make_shared<Beam>(Polyline(arc), 70.0, "curved_sill");
    scene.add(sill);
    draw_line(scene, sill->axis);
    for (const int i : {2, 6}) {
        const Point foot = arc[i] + (arc[i + 1] - arc[i]) * 0.5;
        const std::shared_ptr<Beam> post = std::make_shared<Beam>(Polyline({foot, foot + Vector(0.0, 0.0, 1800.0)}), 60.0, fmt::format("post_on_curve_{}", i));
        scene.add(post);
        draw_line(scene, post->axis);
        join(scene, sill, post);
    }

    scene.pb_dump(pb_path("butt_curved_sill"));
}

/// A pentagon floor and walls standing on two of its edges, meeting at 108 degrees, and a wall with a sloped top on a third edge.
static void polygon_floor() {

    WoodSession scene("butt_polygon_floor");
    std::vector<Point> pentagon;
    for (int i = 0; i <= 5; i++) {
        const double angle = 2.0 * Tolerance::PI * (i % 5) / 5.0 - 0.5 * Tolerance::PI;
        pentagon.emplace_back(2000.0 * std::cos(angle), 2000.0 * std::sin(angle), 0.0);
    }
    const Polyline middle(pentagon);
    const std::shared_ptr<Plate> floor = std::make_shared<Plate>(middle.translated({0.0, 0.0, -100.0}), middle.translated({0.0, 0.0, 100.0}), "pentagon_floor");
    const std::shared_ptr<Plate> first = Plate::from_line(Line::from_points(pentagon[0], pentagon[1]), {0.0, 0.0, 1.0}, 2400.0, 140.0, "pentagon_wall_0");
    const std::shared_ptr<Plate> second = Plate::from_line(Line::from_points(pentagon[1], pentagon[2]), {0.0, 0.0, 1.0}, 2400.0, 140.0, "pentagon_wall_1");

    // the sloped wall on the edge before the first: its outline rises from 1800 to 2600 along the edge, thickness across it, centred
    const Vector along = (pentagon[0] - pentagon[4]).normalized();
    const Vector across = along.cross(Vector(0.0, 0.0, 1.0)) * 70.0;
    const Polyline sloped({pentagon[4], pentagon[0], pentagon[0] + Vector(0.0, 0.0, 2600.0), pentagon[4] + Vector(0.0, 0.0, 1800.0), pentagon[4]});
    const std::shared_ptr<Plate> third = std::make_shared<Plate>(sloped.translated(across * -1.0), sloped.translated(across), "pentagon_wall_sloped");
    for (const std::shared_ptr<Plate>& plate : {floor, first, second, third}) {
        scene.add(plate);
        draw_line(scene, mid_polygon(*plate));
    }
    for (const std::shared_ptr<Plate>& wall : {first, second, third})
        join(scene, floor, wall);
    join(scene, first, second);
    join(scene, first, third);

    scene.pb_dump(pb_path("butt_polygon_floor"));
}

/// Butts on the cases of a timber building, beams, columns and plates mixed, square and oblique, each a scene with the user's axes and mid-plane polygons in red.
int main() {

    // each case its own scene, written to data/output/pb/butt_<case>.pb
    brace();
    rafter();
    beam_on_column();
    beam_into_column();
    joists_into_wall();
    wall_on_beam();
    columns_on_slab();
    walls_at_angle();
    wall_under_roof();
    short_joist();
    curved_sill();
    polygon_floor();
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
JointBeam::butt on the cases of a timber building, each drawn as the user draws it, beams and columns on lines, plates on mid-plane polygons (red), and joined into flat butts: a brace at 55 degrees between a post and a sill, a rafter at 30 degrees seated on a wall plate and plumb cut on the ridge, a beam on a column, a beam into a column at 60 degrees in plan, joists into a wall square and at 30 degrees, a wall on a beam, columns above and below a slab, two walls at 120 degrees, and a wall cut under a roof sloped at 25 degrees, a joist drawn short of a wall and run on to it, two posts on a curved sill, and a pentagon floor with walls at 108 degrees and a wall with a sloped top.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 22_butt_gallery --parallel 6 && ./build/22_butt_gallery && ../bash/publish-scene.sh --target 22_butt_gallery

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
