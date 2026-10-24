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

/// A member added with the line or the polygon it was drawn on: a beam's axis, a plate's mid-plane polygon.
static void add_drawn(WoodSession& scene, const std::shared_ptr<Element>& member) {

    scene.add(member);
    if (const std::shared_ptr<Beam> beam = std::dynamic_pointer_cast<Beam>(member))
        draw_line(scene, beam->axis);
    if (const std::shared_ptr<Plate> plate = std::dynamic_pointer_cast<Plate>(member)) {
        std::vector<Point> points;
        for (size_t i = 0; i < plate->polylines[0].point_count(); i++)
            points.push_back(
                plate->polylines[0][i] + (plate->polylines[1][i] - plate->polylines[0][i]) * 0.5
            );
        draw_line(scene, Polyline(points));
    }
}

/// The butt of a pair, named by its configuration and its members, passed to both.
static void
join(WoodSession& scene, const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b) {

    const std::shared_ptr<JointBeam> butt = JointBeam::butt(*a, *b);
    butt->name = fmt::format("{}_{}_{}", butt->name, a->name, b->name);
    scene.add(butt);
    scene.add_interaction(butt, a, butt->interaction(0));
    scene.add_interaction(butt, b, butt->interaction(1));
}

// ═══════════════════════════════════════════════════════════════════════════
// Assemblies
// ═══════════════════════════════════════════════════════════════════════════

/// A post on a sill.
static void brace_frame() {

    WoodSession scene("butt_brace_frame");
    const std::shared_ptr<Beam> sill =
        std::make_shared<Beam>(Polyline({{-800.0, 0.0, 0.0}, {1400.0, 0.0, 0.0}}), 70.0, "sill");
    const std::shared_ptr<Beam> post =
        std::make_shared<Beam>(Polyline({{0.0, 0.0, 0.0}, {0.0, 0.0, 2400.0}}), 70.0, "post");
    const std::shared_ptr<Beam> brace =
        std::make_shared<Beam>(Polyline({{0.0, 0.0, 1500.0}, {1050.0, 0.0, 0.0}}), 60.0, "brace");
    for (const std::shared_ptr<Beam>& beam : {sill, post, brace})
        add_drawn(scene, beam);
    join(scene, sill, post);
    join(scene, post, brace);
    join(scene, sill, brace);

    scene.pb_dump(pb_path("butt_brace_frame"));
}

/// Two posts on a curved sill.
static void curved_sill() {

    WoodSession scene("butt_curved_sill");
    std::vector<Point> arc;
    for (int i = 0; i <= 8; i++) {
        const double angle = 0.5 * Tolerance::PI * i / 8.0;
        arc.emplace_back(2000.0 * std::cos(angle), 2000.0 * std::sin(angle), 0.0);
    }
    const std::shared_ptr<Beam> sill = std::make_shared<Beam>(Polyline(arc), 70.0, "curved_sill");
    add_drawn(scene, sill);
    for (const int i : {2, 6}) {
        const Point foot = arc[i] + (arc[i + 1] - arc[i]) * 0.5;
        const std::shared_ptr<Beam> post = std::make_shared<Beam>(
            Polyline({foot, foot + Vector(0.0, 0.0, 1800.0)}), 60.0, fmt::format("post_{}", i)
        );
        add_drawn(scene, post);
        join(scene, sill, post);
    }

    scene.pb_dump(pb_path("butt_curved_sill"));
}

/// A pentagon floor, walls on three of its edges.
static void polygon_floor() {

    WoodSession scene("butt_polygon_floor");
    std::vector<Point> pentagon;
    for (int i = 0; i <= 5; i++) {
        const double angle = 2.0 * Tolerance::PI * (i % 5) / 5.0 - 0.5 * Tolerance::PI;
        pentagon.emplace_back(2000.0 * std::cos(angle), 2000.0 * std::sin(angle), 0.0);
    }
    const Polyline middle(pentagon);
    const std::shared_ptr<Plate> floor = std::make_shared<Plate>(
        middle.translated({0.0, 0.0, -100.0}), middle.translated({0.0, 0.0, 100.0}), "floor"
    );
    const std::shared_ptr<Plate> first = Plate::from_line(
        Line::from_points(pentagon[0], pentagon[1]), {0.0, 0.0, 1.0}, 2400.0, 140.0, "wall_0"
    );
    const std::shared_ptr<Plate> second = Plate::from_line(
        Line::from_points(pentagon[1], pentagon[2]), {0.0, 0.0, 1.0}, 2400.0, 140.0, "wall_1"
    );

    // the sloped wall on the edge before the first: its outline rises from 1800 to 2400 along the edge, thickness across it, centred
    const Vector along = (pentagon[0] - pentagon[4]).normalized();
    const Vector across = along.cross(Vector(0.0, 0.0, 1.0)) * 70.0;
    const Polyline sloped(
        {pentagon[4],
         pentagon[0],
         pentagon[0] + Vector(0.0, 0.0, 2400.0),
         pentagon[4] + Vector(0.0, 0.0, 1800.0),
         pentagon[4]}
    );
    const std::shared_ptr<Plate> third = std::make_shared<Plate>(
        sloped.translated(across * -1.0), sloped.translated(across), "wall_sloped"
    );
    for (const std::shared_ptr<Plate>& plate : {floor, first, second, third})
        add_drawn(scene, plate);
    for (const std::shared_ptr<Plate>& wall : {first, second, third})
        join(scene, floor, wall);
    join(scene, first, second);
    join(scene, third, first);

    scene.pb_dump(pb_path("butt_polygon_floor"));
}

/// Walls on a zigzag in plan.
static void zigzag_walls() {

    WoodSession scene("butt_zigzag_walls");
    const std::vector<Point> zigzag = {
        {0.0, 0.0, 0.0},
        {1500.0, 900.0, 0.0},
        {3000.0, 0.0, 0.0},
        {4500.0, 900.0, 0.0},
        {6000.0, 0.0, 0.0}
    };
    const std::shared_ptr<Plate> floor = Plate::from_rectangle(
        {-300.0, -600.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6600.0, 2100.0, 200.0, "floor"
    );
    add_drawn(scene, floor);
    std::vector<std::shared_ptr<Plate>> walls;
    for (size_t i = 0; i + 1 < zigzag.size(); i++) {
        walls.push_back(Plate::from_line(
            Line::from_points(zigzag[i], zigzag[i + 1]),
            {0.0, 0.0, 1.0},
            2400.0,
            140.0,
            fmt::format("wall_{}", i)
        ));
        add_drawn(scene, walls.back());
    }
    for (const std::shared_ptr<Plate>& wall : walls)
        join(scene, floor, wall);
    for (size_t i = 0; i + 1 < walls.size(); i++)
        join(scene, walls[i], walls[i + 1]);

    scene.pb_dump(pb_path("butt_zigzag_walls"));
}

/// A folded roof on a zigzag in section.
static void folded_roof() {

    WoodSession scene("butt_folded_roof");
    const std::vector<Point> zigzag = {
        {0.0, 0.0, 2400.0},
        {1500.0, 0.0, 3200.0},
        {3000.0, 0.0, 2400.0},
        {4500.0, 0.0, 3200.0},
        {6000.0, 0.0, 2400.0}
    };
    std::vector<std::shared_ptr<Plate>> roof;
    for (size_t i = 0; i + 1 < zigzag.size(); i++) {
        const Vector along = (zigzag[i + 1] - zigzag[i]).normalized();
        const Vector across = along.cross(Vector(0.0, 1.0, 0.0));

        const double before = i == 0 ? 300.0 / along[0] : 0.0;
        const double after = i + 2 == zigzag.size() ? 300.0 / along[0] : 0.0;
        roof.push_back(Plate::from_rectangle(
            zigzag[i] - along * before - across * 80.0,
            along,
            {0.0, 1.0, 0.0},
            Point::distance(zigzag[i], zigzag[i + 1]) + before + after,
            4000.0,
            160.0,
            fmt::format("roof_{}", i)
        ));
        add_drawn(scene, roof.back());
    }

    std::vector<Point> outline = {{6000.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    outline.insert(outline.end(), zigzag.begin(), zigzag.end());
    outline.push_back(outline.front());
    std::array<std::shared_ptr<Plate>, 2> gables;
    for (size_t g = 0; g < 2; g++) {
        const Polyline drawn = Polyline(outline).translated({0.0, g == 0 ? 0.0 : 4000.0, 0.0});
        gables[g] = std::make_shared<Plate>(
            drawn.translated({0.0, -70.0, 0.0}),
            drawn.translated({0.0, 70.0, 0.0}),
            fmt::format("gable_{}", g)
        );
        add_drawn(scene, gables[g]);
    }
    for (const std::shared_ptr<Plate>& plate : roof)
        for (const std::shared_ptr<Plate>& gable : gables)
            join(scene, plate, gable);
    for (size_t i = 0; i + 1 < roof.size(); i++)
        join(scene, roof[i], roof[i + 1]);

    scene.pb_dump(pb_path("butt_folded_roof"));
}

/// Butt pairs combined into assemblies, beams and plates drawn on lines and polygons, each its own scene.
int main() {

    brace_frame();
    curved_sill();
    polygon_floor();
    zigzag_walls();
    folded_roof();
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Butt pairs combined into assemblies, each drawn as the user draws it, beams on lines and plates on mid-plane polygons (red), and every pair joined by JointBeam::butt, which reads its configuration from the drawn members: a braced frame of three top_side butts, two posts on a curved sill of eight segments, a pentagon floor with walls side_side_outplane at 108 degrees and one with a sloped top, walls on a zigzag in plan side_side_outplane at every fold, and a folded roof on a zigzag in section, side_side_outplane at every fold and top_side on two gable walls. Each assembly is its own scene, data/output/pb/butt_<assembly>.pb.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 23_butt_assemblies --parallel 6 && ./build/23_butt_assemblies && ../bash/publish-scene.sh --target 23_butt_assemblies

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
