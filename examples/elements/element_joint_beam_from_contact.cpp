#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Beam-to-beam joints from the axis contact of two beams: a crossing and a side-to-end.
int main() {

    WoodSession scene("element_joint_beam_from_contact");

    const std::shared_ptr<Beam> crossing_a = std::make_shared<Beam>(Polyline({{-500.0, 0.0, 0.0}, {500.0, 0.0, 0.0}}), 60.0, "crossing_a");
    const std::shared_ptr<Beam> crossing_b = std::make_shared<Beam>(Polyline({{0.0, -500.0, 0.0}, {0.0, 500.0, 0.0}}), 60.0, "crossing_b");
    const std::shared_ptr<Beam> tee_a = std::make_shared<Beam>(Polyline({{900.0, 0.0, 0.0}, {1900.0, 0.0, 0.0}}), 60.0, "tee_a");
    const std::shared_ptr<Beam> tee_b = std::make_shared<Beam>(Polyline({{1400.0, 0.0, 0.0}, {1400.0, 600.0, 0.0}}), 60.0, "tee_b");
    scene.add(crossing_a);
    scene.add(crossing_b);
    scene.add(tee_a);
    scene.add(tee_b);

    // a crossing: the axes meet halfway along both
    const InteractionContactAxis crossing(
        Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}),
        0.5,
        0.5,
        0,
        0,
        0,
        0
    );
    const std::shared_ptr<JointBeam> lap = JointBeam::from_contact(
        *crossing_a,
        *crossing_b,
        crossing,
        400.0,
        0.9
    );
    lap->name = "crossing";
    scene.add(lap);
    scene.add_interaction(lap, crossing_a, lap->interaction(0));
    scene.add_interaction(lap, crossing_b, lap->interaction(1));

    // a side-to-end: the second axis ends halfway along the first
    const InteractionContactAxis tee(
        Line::from_points({1400.0, 0.0, 0.0}, {1400.0, 0.0, 0.0}),
        0.5,
        0.0,
        0,
        0,
        0,
        0
    );
    const std::shared_ptr<JointBeam> butt = JointBeam::from_contact(
        *tee_a,
        *tee_b,
        tee,
        400.0,
        0.9
    );
    butt->name = "tee";
    scene.add(butt);
    scene.add_interaction(butt, tee_a, butt->interaction(0));
    scene.add_interaction(butt, tee_b, butt->interaction(1));

    // drawn apart: each pair's second beam moved 250 along the way it comes off the first, so the cuts read
    const Vector crossing_off = lap->insertion(1) * 250.0;
    const Vector tee_off = butt->insertion(1) * 250.0;
    crossing_b->place(Xform::translation(crossing_off[0], crossing_off[1], crossing_off[2]));
    tee_b->place(Xform::translation(tee_off[0], tee_off[1], tee_off[2]));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Beam-to-beam joints from the axis contact of two beams: a crossing and a side-to-end.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_from_contact --parallel 6 && ./build/element_joint_beam_from_contact && ../bash/publish-scene.sh --target element_joint_beam_from_contact

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
