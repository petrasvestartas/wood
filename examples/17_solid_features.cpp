#include "cutting_gallery.h"
using namespace cutting_gallery;

int main() {
    WoodSession scene("17_solid_features");
    int row = 0;
    for (const std::pair<SolidOperation, std::string>& design : std::vector<std::pair<SolidOperation, std::string>>{
             {SolidOperation::subtract, "Difference"}, {SolidOperation::intersect, "Intersection"}, {SolidOperation::add, "Union"}}) {
        const std::shared_ptr<Block> stock = std::make_shared<Block>(std::vector<Polyline>{rectangle(0, 0, 0, 200, 160), rectangle(30, 15, 100, 140, 130)});
        const std::shared_ptr<Joint> cutter = std::make_shared<Joint>(std::vector<Polyline>{rectangle(90, -20, 20, 130, 190), rectangle(60, -5, 130, 130, 190)});
        cutter->operation = design.first;
        cut_case(scene, design.second + " / sloped solids", stock, cutter, row++ * 280);
    }
    const Mesh tetrahedron = Mesh::from_vertices_and_faces(
        {{40, 30, -10}, {190, 30, -10}, {115, 150, -10}, {115, 80, 130}},
        {{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}});
    cut_case(scene, "Custom mesh cutter / tetrahedron", std::make_shared<Block>(
                 Mesh::loft({rectangle(0, 0, 0, 200, 160)}, {rectangle(0, 0, 80, 200, 160)})),
             std::make_shared<Joint>(tetrahedron), row * 280);
    finish(scene, "17_solid_features");
}

/*
directory: cd /home/petras/code/code_cpp/wood_research/wood
run: buildslot ~/.local/bin/cmake --build build --target 17_solid_features --parallel 4 && tools/run_guarded.sh -t 10 -m 4 -- build/17_solid_features
cloudflare: ../bash/publish-scene.sh "$PWD/data/output/pb/17_solid_features.pb" --no-notify
view: https://petrasvestartas.github.io/session/
*/
