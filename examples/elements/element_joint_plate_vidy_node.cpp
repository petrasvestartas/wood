#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Double-layer Théâtre de Vidy folded crease node: assembled cutaway and exploded 4-plate view.
int main() {

    WoodSession scene("element_joint_plate_vidy_node");

    // assembled: 110-degree double-layer node (45 mm CLT + 210 mm cavity + 45 mm CLT = 300 mm)
    const std::shared_ptr<TreeNode> assembled = scene.add_group("assembled");
    const VidyDoubleLayerNode node =
        JointVidyElement::folded_double_layer_node(110.0, 1650.0, 950.0, 45.0, 300.0, 500.0, true);
    scene.add(node.inner_a, assembled);
    scene.add(node.outer_a, assembled);
    scene.add(node.inner_b, assembled);
    scene.add(node.outer_b, assembled);

    for (const std::shared_ptr<FacePlanarElement>& border : node.borders)
        scene.add(border, assembled);

    // exploded: the four jointed CLT plates drawn apart along each plate's single insertion vector
    const std::shared_ptr<TreeNode> exploded = scene.add_group("exploded");
    const VidyDoubleLayerNode exp =
        JointVidyElement::folded_double_layer_node(110.0, 1650.0, 950.0, 45.0, 300.0, 500.0, true);
    const Vector shift_y(0.0, 2800.0, 0.0);
    const std::array<std::pair<std::shared_ptr<FacePlanarElement>, double>, 4> parts = {{
        {exp.inner_a, 0.0},
        {exp.inner_b, 380.0},
        {exp.outer_b, 300.0},
        {exp.outer_a, 320.0},
    }};

    for (const std::pair<std::shared_ptr<FacePlanarElement>, double>& item : parts) {
        const Vector v_ins = item.first->insertion_vectors().front();
        const Vector delta = shift_y - v_ins * item.second;
        item.first->name = item.first->name + "_exploded";
        item.first->place(Xform::translation(delta[0], delta[1], delta[2]));
        scene.add(item.first, exploded);
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Double-layer Vidy node: 4 x 45 mm CLT plates across 300 mm depth (210 mm cavity + perpendicular border boards), stepped Double_Pin (270/180/140 mm), Extend_Pins + Single_Pin (180/140 mm), Extend lip and through-mortises, assembled and exploded along single insertion vectors.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_vidy_node --parallel 6 && ./build/element_joint_plate_vidy_node && ../bash/publish-scene.sh --target element_joint_plate_vidy_node

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
