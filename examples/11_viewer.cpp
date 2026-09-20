#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::annen_corner};

int main() {

    WoodSession scene = WoodSession::yaml_load(DATASET);
    scene.compute_contacts();
    scene.compute_features();

    // One group per element: the element, then `outlines`, `contacts` and `joints` child groups, each flag adding that part.
    scene.add_to_tree(true, true, true, true);

    std::cout << scene.tree.root()->descendants().size() << " tree nodes, " << scene.objects.polylines->size() << " polylines, " << scene.objects.meshes->size() << " meshes for the viewer\n";
    std::cout << "a joint of type 12 draws in " << joint_color(12).name << ", a side_side contact in " << contact_color(ContactType::side_side).name << "\n";

    // "live" is the file session_viewer watches; a named file lands beside it.
    scene.pb_dump(pb_path("live").string());
    scene.pb_dump(pb_path(scene.name).string());

    return 0;
}

/*
description: the scene arranged for the viewer: a group per element with its outlines, contacts and joints as children, the colour tables, and the two files pb_dump writes.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target 11_viewer --parallel 4 && ./build/11_viewer
cloudflare: ../bash/publish-scene.sh --target 11_viewer
view: https://petrasvestartas.github.io/session/
*/
