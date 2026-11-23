#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A node plate: a post and a diagonal ending on a chord at one node, each stopping on the members before it, one plate let
/// into all three and pinned.
int main() {

    WoodSession scene("element_joint_beam_node_plate");

    const std::shared_ptr<EdgeStraightElement> chord =
        EdgeStraightElement::square(Polyline({Point(-800.0, 0.0, 0.0), Point(800.0, 0.0, 0.0)}), 80.0, "chord");
    const std::shared_ptr<EdgeStraightElement> post =
        EdgeStraightElement::square(Polyline({Point(0.0, 0.0, 0.0), Point(0.0, 0.0, 900.0)}), 80.0, "post");
    const std::shared_ptr<EdgeStraightElement> diagonal =
        EdgeStraightElement::square(Polyline({Point(0.0, 0.0, 0.0), Point(700.0, 0.0, 700.0)}), 80.0, "diagonal");
    scene.add(chord);
    scene.add(post);
    scene.add(diagonal);

    // the ends stop on the chord, the diagonal on the post too
    scene.add_interaction(chord, post, std::make_shared<InteractionFeatureSolid>(chord->element_geometry_mesh(), SolidOperation::subtract));
    scene.add_interaction(chord, diagonal, std::make_shared<InteractionFeatureSolid>(chord->element_geometry_mesh(), SolidOperation::subtract));
    scene.add_interaction(post, diagonal, std::make_shared<InteractionFeatureSolid>(post->element_geometry_mesh(), SolidOperation::subtract));

    // the plate a third of their thickness, the pins past where each member is clear of the others, dropped in from below
    const std::vector<const EdgeStraightElement*> members = {chord.get(), post.get(), diagonal.get()};
    const std::vector<double> clear = {0.0, 80.0, 194.0};
    const std::shared_ptr<FacePlanarElement> plate = JointBeamElement::node_plate(members, Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), {48.0, 128.0, 242.0}, 160.0 / 3.0);
    const std::shared_ptr<JointBeamElement> pins = JointBeamElement::slotted_plate(members, *plate, Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, -1.0), clear);
    scene.add(plate);
    scene.add(pins);
    scene.add_interaction(pins, chord, pins->interaction(0));
    scene.add_interaction(pins, post, pins->interaction(1));
    scene.add_interaction(pins, diagonal, pins->interaction(2));
    scene.add_interaction(pins, plate, pins->interaction(3));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A node plate: a post and a diagonal ending on a chord at one node, each stopping on the members before it, one plate let into all three (the union of their strips, its convex hull) and pinned, one row of two pins in each member; the plate drops in last from below along its slot.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_node_plate --parallel 6 && ./build/element_joint_beam_node_plate && ../bash/publish-scene.sh --target element_joint_beam_node_plate

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
