#include "cutting_gallery.h"
using namespace cutting_gallery;

int main() {
    WoodSession scene("11_drill_solids");
    int row = 0;
    for (double tolerance : {2.0, 0.2, 0.02}) {
        const Line axis = Line::from_points({0, double(row * 180), 0}, {0, double(row * 180), 100});
        const std::shared_ptr<Joint> drill = Joint::drill(axis, 35, tolerance);
        drill->is_visible = true;
        drill->name = fmt::format(
            "Mesh: tolerance {}, {} segments", tolerance, circle_segments(35, tolerance)
        );
        drill->compute_geometry_mesh();
        Mesh drill_mesh = drill->geometry_mesh();
        drill_mesh.set_objectcolor(CUTTER_COLOR);
        drill->set_geometry(drill_mesh);
        scene.add(drill);
        BRep exact =
            drill->element_geometry_brep().transformed(Xform::translation(180, 0, 0));
        exact.surfacecolor = RESULT_COLOR;
        scene.add(std::make_shared<Element>(exact, "Exact cylindrical BRep"));
        ++row;
    }
    const std::shared_ptr<Block> stock = std::make_shared<Block>(
        std::vector<Polyline>{rectangle(0, 0, 0, 200, 160), rectangle(0, 0, 80, 200, 160)}
    );
    cut_case(
        scene,
        "Tilted drill through block",
        stock,
        Joint::drill(Line::from_points({60, 80, -30}, {145, 80, 130}), 28, 0.2),
        640
    );
    finish(scene, "11_drill_solids");
}

/*
directory: cd /home/petras/code/code_cpp/wood_research/wood
run: buildslot ~/.local/bin/cmake --build build --target 11_drill_solids --parallel 4 && tools/run_guarded.sh -t 10 -m 4 -- build/11_drill_solids
cloudflare: ../bash/publish-scene.sh "$PWD/data/output/pb/11_drill_solids.pb" --no-notify
view: https://petrasvestartas.github.io/session/
*/
