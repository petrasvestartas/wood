#include "cutting_gallery.h"
using namespace cutting_gallery;

int main() {
    WoodSession scene("12_solid_features");
    int row = 0;
    for (const std::pair<SolidOperation, std::string>& design :
         std::vector<std::pair<SolidOperation, std::string>>{
             {SolidOperation::subtract, "Difference"},
             {SolidOperation::intersect, "Intersection"},
             {SolidOperation::add, "Union"}
         }) {
        const std::shared_ptr<NodeBlockElement> stock = std::make_shared<NodeBlockElement>(
            std::vector<Polyline>{rectangle(0, 0, 0, 200, 160), rectangle(30, 15, 100, 140, 130)}
        );
        const std::shared_ptr<JointElement> cutter = std::make_shared<JointElement>(std::vector<Polyline>{
            rectangle(90, -20, 20, 130, 190), rectangle(60, -5, 130, 130, 190)
        });
        cutter->operation = design.first;
        cut_case(scene, design.second + " / sloped solids", stock, cutter, row++ * 280);
    }
    const Mesh tetrahedron = Mesh::from_vertices_and_faces(
        {{40, 30, -10}, {190, 30, -10}, {115, 150, -10}, {115, 80, 130}},
        {{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}}
    );
    const Polyline block_bottom = rectangle(0, 0, 0, 200, 160);
    const Polyline block_top = rectangle(0, 0, 80, 200, 160);
    const std::shared_ptr<NodeBlockElement> block =
        std::make_shared<NodeBlockElement>(Mesh::loft({block_bottom}, {block_top}));
    cut_case(
        scene,
        "Custom mesh cutter / tetrahedron",
        block,
        std::make_shared<JointElement>(tetrahedron),
        row * 280
    );
    finish(scene, "12_solid_features");
}

/*
|||||||| DESCRIPTION ||||||||
A plate under the three solid operations of a joint: difference, intersection and union.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 12_solid_features --parallel 6 && ./build/12_solid_features && ../bash/publish-scene.sh --target 12_solid_features

|||||||| TIME ||||||||
2.34 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
