#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::annen_box};

int main() {

    WoodSession wood_session = WoodSession::obj_load(DATASET);

    Settings& s = wood_session.settings;
    s.joint_parameters[1 * 3 + 0] = 200;   // ss_e_op family: division length
    s.joint_parameters[1 * 3 + 2] = 10;    // ss_e_op family: joint id
    s.joint_parameters[2 * 3 + 2] = 20;    // ts_e_p family: joint id
    s.joint_volume_extension = {0, 0, 10};
    s.joint_scale = {1.0, 1.0, 1.0};
    s.distance = 0.1;
    s.angle = 0.11;

    wood_session.compute_features(face_to_face);
    for (const FeaturePlate& joint : wood_session.get_plate_features())
        std::cout << fmt::format("{} divisions {} length {:.0f}\n", joint.name, joint.divisions, joint.length);

    const WoodSession back = WoodSession::pb_loads(wood_session.pb_dumps());
    std::cout << "the file carries the settings: division length " << back.settings.joint_parameters[3] << "\n";

    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the solver settings as a value on the wood session: joint ids and division lengths per family, volume extension, scale and tolerances set in code, the solve, and the settings read back from the file.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 6_settings --parallel 4 && ./build/6_settings && ../bash/publish-scene.sh --target 6_settings

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
