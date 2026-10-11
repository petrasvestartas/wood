#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A steel plate let into a column and a rib, pinned through all three.
int main() {

    WoodSession scene("element_joint_beam_rectangle_plate");

    const std::shared_ptr<Column> column = std::make_shared<Column>(
        Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 1100.0}),
        Polyline::rectangle({-110.0, -110.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 220.0, 220.0),
        "column"
    );
    const std::shared_ptr<BeamVariable> rib = BeamVariable::between(
        Polyline::rectangle({110.0, -60.0, 700.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 120.0, 400.0),
        Polyline::rectangle({1400.0, -60.0, 700.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 120.0, 400.0),
        "rib"
    );
    scene.add(column);
    scene.add(rib);

    // the plate and its connector from their contact, added, and the connector passed to the column, the rib and the plate
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(column, rib);
    const std::shared_ptr<Plate> plate = JointBeam::let_in_plate(
        *rib,
        *contact,
        30.0,  // width
        150.0, // back: into the column, short of its far face
        200.0, // front: into the rib
        250.0  // height: down from the tops, flush
    );
    const std::shared_ptr<JointBeam> pins = JointBeam::rectangle_plate(
        *column,
        *rib,
        *plate,
        *contact,
        25.0, // pin_radius
        3.0,  // margin_x: a pin diameter and a half in from the plate's ends
        3.0   // margin_z: a pin diameter and a half in from its top and bottom
    );
    plate->name = "plate";
    pins->name = "plate_pins";
    scene.add(plate);
    scene.add(pins);
    scene.add_interaction(pins, column, pins->interaction(0));
    scene.add_interaction(pins, rib, pins->interaction(1));
    scene.add_interaction(pins, plate, pins->interaction(2));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A steel plate let into a column and a rib, pinned through all three.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_rectangle_plate --parallel 6 && ./build/element_joint_beam_rectangle_plate && ../bash/publish-scene.sh --target element_joint_beam_rectangle_plate

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
