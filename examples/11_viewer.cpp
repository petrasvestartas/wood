#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::annen_corner};

int main() {

    WoodSession scene = WoodSession::yaml_load(DATASET);
    scene.compute_contacts();
    scene.compute_features();

    // One group per element: the element, then `attributes`, `contacts` and `joints` child groups, each flag adding that part.
    scene.add_to_tree(true, true, true, true);

    std::cout << scene.tree.root()->descendants().size() << " tree nodes, " << scene.objects.polylines->size() << " polylines, " << scene.objects.meshes->size() << " meshes for the viewer\n";
    std::cout << "a joint of type 12 draws in " << joint_color(12).name << ", a side_side contact in " << contact_color(ContactType::side_side).name << "\n";

    // "live" is the file session_viewer watches; a named file lands beside it, here without the attributes.
    scene.pb_dump(pb_path("live").string());
    scene.show_attributes(false);
    std::cout << scene.tree.root()->descendants().size() << " tree nodes without the attributes\n";
    scene.pb_dump(pb_path(scene.name).string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the scene arranged for the viewer: a group per element with its attributes, contacts and joints as children, the colour tables, show_attributes(false) taking the attributes out again, and the two files pb_dump writes.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 11_viewer --parallel 4 && ./build/11_viewer && ../bash/publish-scene.sh --target 11_viewer

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
