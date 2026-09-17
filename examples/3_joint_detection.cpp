#include "wood_session.h"

using namespace wood_session;

const std::string DATASET = globals::Dataset::inplane_hexshell;   // globals::Dataset::<name>

/// Loads the plates, detects the joints, and writes the scene; no plate is lofted before the file is written.
int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_joints();

    /// element_geometry(): the plate alone, the loft of its two outlines, never cut; compas_model's elementgeometry.
    /// model_geometry(): the plate with its joints cut in, the loft of the merged outlines; compas_model's modelgeometry, the one to inspect.
    /// Both loft on first call and stay cached until the plate changes; asking for neither keeps the plates as outlines only.
    const std::shared_ptr<Plate> plate = wood_session.plates().front();
    fmt::print("{}: {} faces alone, {} faces with joints\n", plate->name, plate->element_geometry().number_of_faces(), plate->model_geometry().number_of_faces());

    /// pb_dump lofts every plate not yet lofted (model geometry) so the viewer sees the cut plates; the tree decides what else is drawn.
    wood_session.add_to_tree(true, true, false, false);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Compute joints between wood elements in a dataset.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/3_joint_detection && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 3_joint_detection

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
