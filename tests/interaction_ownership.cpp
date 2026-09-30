#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

static void check(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

static const ElementFeature* feature(const Element& element, const std::string& guid) {
    for (const ElementFeature& item : element.features())
        if (item.guid() == guid)
            return &item;

    return nullptr;
}

int main() {
    WoodSession scene("interaction ownership");
    const auto beam = std::make_shared<Beam>(Polyline({{0, 0, 0}, {100, 0, 0}}), 10.0);
    const auto column = std::make_shared<Beam>(Polyline({{50, 0, -50}, {50, 0, 50}}), 10.0);
    scene.add(beam);
    scene.add(column);

    const auto contact = std::make_shared<InteractionContactFace>(
        2, 4, ContactType::side_side,
        Polyline::rectangle({45, -5, 0}, {1, 0, 0}, {0, 1, 0}, 10, 10));
    scene.add_interaction(beam, column, contact);
    check(feature(*beam, contact->guid()), "first contact belongs to source");

    scene.add_interaction(column, beam, contact->flipped());
    check(!feature(*beam, contact->guid()), "old contact host must be cleared");
    const ElementFeature* moved_contact = feature(*column, contact->guid());
    check(moved_contact && moved_contact->face_index == 4, "contact face is relative to its new host");
    check(scene.get_interaction(beam, column).size() == 1, "contact is not duplicated");

    const auto joint = std::make_shared<InteractionFeatureBeam>();
    for (int i = 0; i < 4; ++i)
        joint->volumes[i] = Polyline::rectangle({double(i), 0, 0}, {1, 0, 0}, {0, 1, 0}, 2, 3);

    const auto stored = std::dynamic_pointer_cast<InteractionFeatureBeam>(scene.add_interaction(column, beam, joint));
    check(!feature(*beam, joint->guid()), "existing edge's first endpoint must not own the joint");
    check(feature(*column, joint->guid()), "joint belongs to the argument source");
    check(joint->volumes[0][0][0] == 0, "adding a reversed joint must not mutate caller geometry");
    scene.add_interaction(beam, column, joint);
    check(feature(*beam, joint->guid()), "reversed call transfers the joint");
    check(!feature(*column, joint->guid()), "joint has exactly one host");
    check(scene.get_interaction(beam, column).size() == 2, "reusing a joint must not duplicate the record");
    check(scene.graph.edges.at(beam->guid()).at(column->guid()).v0 == beam->guid(), "graph ordering stays stable");
    check(stored->volumes[0][0][0] == 2, "stored geometry stays in edge order after ownership changes");

    const WoodSession restored = WoodSession::pb_loads(scene.pb_dumps());
    check(feature(*restored.get_element<Element>(beam->guid()), joint->guid()), "protobuf preserves the joint host");
    check(!feature(*restored.get_element<Element>(column->guid()), joint->guid()), "protobuf keeps one joint host");
    scene.add_interaction(column, beam, joint);
    scene.compute_beam_features(20, 0.5, 0);
    check(!feature(*beam, joint->guid()) && !feature(*column, joint->guid()), "recomputation removes old features from either host");
    scene.remove_interaction(column, beam);
    check(!feature(*beam, joint->guid()) && !feature(*column, contact->guid()), "removing the edge clears its features");
}
