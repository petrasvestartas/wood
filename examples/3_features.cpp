#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);

    // fingers a quarter as deep as the dataset's, as wide and as many, and no shrinking of the volume, which would part the plates at the shell's folds
    wood_session.settings.joint_volume_extension = {0.0, 0.0, -10.0};
    wood_session.settings.joint_depth_scale = 0.2;

    wood_session.compute_features();

    for (const InteractionFeaturePlate& joint : wood_session.get_plate_features())
        std::cout << fmt::format(
            "{} type {} between {} f{} and {} f{}: {} male, {} female outlines, {} divisions\n",
            joint.name,
            joint.joint_type,
            joint.element_a.substr(0, 8),
            joint.contact.face_a,
            joint.element_b.substr(0, 8),
            joint.contact.face_b,
            joint.male_outlines[0].size(),
            joint.female_outlines[0].size(),
            joint.divisions
        );

    const std::shared_ptr<FacePlanarElement> plate = wood_session.get_elements<FacePlanarElement>().front();
    std::cout << fmt::format(
        "{}: {} faces alone, {} faces with joints, {} joint features\n",
        plate->name,
        plate->element_geometry_mesh().number_of_faces(),
        plate->model_geometry_mesh().number_of_faces(),
        plate->Element::features().size()
    );

    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the joinery pipeline on one dataset: every plate joint with its type, faces and cut outlines; a plate's geometry alone and with its joints cut in; the joint features the hosts carry.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 3_features --parallel 6 && ./build/3_features && ../bash/publish-scene.sh --target 3_features

|||||||| TIME ||||||||
1.33 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
