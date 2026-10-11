#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// in_plane_splayed_scarf on beams: an end-to-end splice of two extended beams, drawn apart.
int main() {

    WoodSession scene("element_joint_beam_in_plane_splayed_scarf");
    scene.settings.joint_parameters[2] = JointPlate::library_id("in_plane_splayed_scarf");

    // two 150 x 150 beams end to end at the origin, each solid running 150 past its axis ends for the joint to cut back
    const std::array<std::shared_ptr<Beam>, 2> pair = Beam::pair_end_to_end(0.0);
    const std::shared_ptr<Beam> first = pair[0];
    const std::shared_ptr<Beam> second = pair[1];
    scene.add(first);
    scene.add(second);

    // the joint from their axis contact at the shared end, added and passed to each beam
    const InteractionContactAxis contact(Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}), 1.0, 0.0, 0, 0, 0, 0);
    const std::shared_ptr<JointBeam> joint = JointBeam::from_contact(*first, *second, contact, 600.0, 0.91, 0, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    // drawn apart: the second beam moved along its axis
    second->place(Xform::translation(300.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
in_plane_splayed_scarf on beams: an end-to-end splice of two extended beams, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_in_plane_splayed_scarf --parallel 6 && ./build/element_joint_beam_in_plane_splayed_scarf && ../bash/publish-scene.sh --target element_joint_beam_in_plane_splayed_scarf

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
