#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ts_e_p_4 on beams: the milled wedge, drawn apart.
int main() {

    WoodSession scene("element_joint_beam_ts_e_p_4");
    scene.settings.joint_parameters[8] = JointPlate::library_id("ts_e_p_4");

    // two 150 x 150 beams in a tee, the second ending on the middle of the first
    const std::array<std::shared_ptr<Beam>, 2> pair = Beam::pair_tee(90.0);
    const std::shared_ptr<Beam> first = pair[0];
    const std::shared_ptr<Beam> second = pair[1];
    scene.add(first);
    scene.add(second);

    // the joint from their axis contact, the second axis ending halfway along the first, added and passed to each beam
    const InteractionContactAxis contact(Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}), 0.5, 0.0, 0, 0, 0, 0);
    const std::shared_ptr<JointBeam> joint = JointBeam::from_contact(*first, *second, contact, 500.0, 0.91, 1, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    // drawn apart: the second beam lifted
    second->place(Xform::translation(0.0, 0.0, 300.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ts_e_p_4 on two beams in a tee: the milled wedge, cut into both beams as solids; the second beam is lifted 300.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_ts_e_p_4 --parallel 6 && ./build/element_joint_beam_ts_e_p_4 && ../bash/publish-scene.sh --target element_joint_beam_ts_e_p_4

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
