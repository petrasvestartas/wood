#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession scene = WoodSession::yaml_load(DATASET);
    scene.compute_contacts();
    scene.compute_features();

    const FeaturePlate joint = scene.get_plate_features().front();
    std::cout << "stored: " << joint.has_session() << ", scene " << joint.session().name << "\n";

    WoodSession& owner = joint.session();
    const std::shared_ptr<Plate> male = owner.get_element<Plate>(joint.element_a);
    const std::shared_ptr<Plate> female = owner.get_element<Plate>(joint.element_b);
    std::cout << "joint " << joint.name << " cuts " << male->name << " and " << female->name << "\n";

    const Interaction& interaction = *owner.get_interaction(joint.element_a, joint.element_b);
    const InteractionContact& contact = interaction.contacts.front();
    std::cout << "its interaction has " << interaction.contacts.size() << " contacts, the first is a " << contact.kind() << " in scene " << contact.session().name << "\n";

    const auto [a, b] = owner.edge_of(interaction);
    std::cout << "the edge joins " << owner.get_element<Element>(a)->name << " and " << owner.get_element<Element>(b)->name << "\n";

    FeaturePlate loose;
    std::cout << "a joint built by hand is stored: " << loose.has_session() << "\n";

    return 0;
}

/*
description: every stored record answers session() with its scene: from a joint back to the scene, to its two plates, to the interaction and the contacts on the same edge, to the edge's elements; a record built by hand has no scene.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target 5_traversal --parallel 4 && ./build/5_traversal
*/
