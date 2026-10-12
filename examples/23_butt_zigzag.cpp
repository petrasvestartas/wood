#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Drawing
// ═══════════════════════════════════════════════════════════════════════════

/// A plate's mid-plane polygon, the outline the user drew it on, red over the computed solids.
static void draw_plate(WoodSession& scene, const Plate& plate) {

    std::vector<Point> points;
    for (size_t i = 0; i < plate.polylines[0].point_count(); i++)
        points.push_back(plate.polylines[0][i] + (plate.polylines[1][i] - plate.polylines[0][i]) * 0.5);
    std::shared_ptr<Polyline> drawn = std::make_shared<Polyline>(points);
    drawn->linecolor = Color(0.85f, 0.1f, 0.1f);
    drawn->width = 4.0;
    scene.add_polyline(drawn, scene.group_named("input"));
}

/// The butt joined and passed to both plates, running first.
static void join(WoodSession& scene, const std::shared_ptr<Plate>& running, const std::shared_ptr<Plate>& butting) {

    const std::shared_ptr<JointBeam> butt = JointBeam::butt(*running, *butting);
    butt->name = fmt::format("butt_{}_{}", running->name, butting->name);
    scene.add(butt);
    scene.add_interaction(butt, running, butt->interaction(0));
    scene.add_interaction(butt, butting, butt->interaction(1));
}

// ═══════════════════════════════════════════════════════════════════════════
// Cases
// ═══════════════════════════════════════════════════════════════════════════

/// Walls on a zigzag polyline in plan, each segment a 140 thick plate 2400 high, on a floor: every fold an oblique corner closed flush.
static void zigzag_walls() {

    WoodSession scene("butt_zigzag_walls");
    const std::vector<Point> zigzag = {{0.0, 0.0, 0.0}, {1500.0, 900.0, 0.0}, {3000.0, 0.0, 0.0}, {4500.0, 900.0, 0.0}, {6000.0, 0.0, 0.0}};
    const std::shared_ptr<Plate> floor = Plate::from_rectangle({-300.0, -600.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6600.0, 2100.0, 200.0, "zigzag_floor");
    scene.add(floor);
    draw_plate(scene, *floor);
    std::vector<std::shared_ptr<Plate>> walls;
    for (size_t i = 0; i + 1 < zigzag.size(); i++) {
        walls.push_back(Plate::from_line(Line::from_points(zigzag[i], zigzag[i + 1]), {0.0, 0.0, 1.0}, 2400.0, 140.0, fmt::format("zigzag_wall_{}", i)));
        scene.add(walls.back());
        draw_plate(scene, *walls.back());
    }

    // the walls on the floor, then each fold, the earlier wall running on to the later one's outer face
    for (const std::shared_ptr<Plate>& wall : walls)
        join(scene, floor, wall);
    for (size_t i = 0; i + 1 < walls.size(); i++)
        join(scene, walls[i], walls[i + 1]);

    scene.pb_dump(pb_path("butt_zigzag_walls"));
}

/// A folded roof on a zigzag polyline in section, each segment a 160 thick plate 4000 deep, over two gable walls: the folds closed flush,
/// each gable cut along the zigzag under every roof plate, valleys and ridges alike.
static void folded_roof() {

    WoodSession scene("butt_folded_roof");
    const std::vector<Point> zigzag = {{0.0, 0.0, 2400.0}, {1500.0, 0.0, 3200.0}, {3000.0, 0.0, 2400.0}, {4500.0, 0.0, 3200.0}, {6000.0, 0.0, 2400.0}};
    std::vector<std::shared_ptr<Plate>> roof;
    for (size_t i = 0; i + 1 < zigzag.size(); i++) {
        const Vector along = (zigzag[i + 1] - zigzag[i]).normalized();
        const Vector across = along.cross(Vector(0.0, 1.0, 0.0));
        roof.push_back(Plate::from_rectangle(zigzag[i] - across * 80.0, along, {0.0, 1.0, 0.0}, Point::distance(zigzag[i], zigzag[i + 1]), 4000.0, 160.0, fmt::format("roof_{}", i)));
        scene.add(roof.back());
        draw_plate(scene, *roof.back());
    }
    std::array<std::shared_ptr<Plate>, 2> gables;
    for (size_t g = 0; g < 2; g++) {
        const double y = g == 0 ? 0.0 : 4000.0;
        gables[g] = Plate::from_line(Line::from_points({0.0, y, 0.0}, {6000.0, y, 0.0}), {0.0, 0.0, 1.0}, 3600.0, 140.0, fmt::format("gable_{}", g));
        scene.add(gables[g]);
        draw_plate(scene, *gables[g]);
    }

    // each gable under every roof plate, the roof running on to the gables' outer faces, then each fold, the earlier plate running on
    for (const std::shared_ptr<Plate>& plate : roof)
        for (const std::shared_ptr<Plate>& gable : gables)
            join(scene, plate, gable);
    for (size_t i = 0; i + 1 < roof.size(); i++)
        join(scene, roof[i], roof[i + 1]);

    scene.pb_dump(pb_path("butt_folded_roof"));
}

/// Plates drawn on zigzag polylines, walls in plan and a roof in section, butted flat at every fold.
int main() {

    // each case its own scene, written to data/output/pb/butt_<case>.pb
    zigzag_walls();
    folded_roof();
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Plates drawn on zigzag polylines, red: walls on a zigzag in plan, each segment a 140 thick plate on a floor, and a folded roof on a zigzag in section over two gable walls. Every fold is an oblique corner closed flush, the earlier plate running on to the later one's outer face; the walls stand flat on the floor, and each gable is cut along the zigzag under every roof plate, valleys as well as ridges.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 23_butt_zigzag --parallel 6 && ./build/23_butt_zigzag && ../bash/publish-scene.sh --target 23_butt_zigzag

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
