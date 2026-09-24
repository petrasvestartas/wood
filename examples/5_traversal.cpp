#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_contacts();
    wood_session.compute_features();

    const FeaturePlate joint = wood_session.get_plate_features().front();
    std::cout << "stored: " << joint.has_session() << ", wood session " << joint.session().name << "\n";

    WoodSession& owner = joint.session();
    const std::shared_ptr<Plate> male = owner.get_element<Plate>(joint.element_a);
    const std::shared_ptr<Plate> female = owner.get_element<Plate>(joint.element_b);
    std::cout << "joint " << joint.name << " cuts " << male->name << " and " << female->name << "\n";

    const Interaction& interaction = *owner.get_interaction(male, female);
    const InteractionContact& contact = interaction.contacts.front();
    std::cout << "its interaction has " << interaction.contacts.size() << " contacts, the first is a " << contact.kind() << " in wood session " << contact.session().name << "\n";

    const auto [a, b] = owner.edge_of(interaction);
    std::cout << "the edge joins " << owner.get_element<Element>(a)->name << " and " << owner.get_element<Element>(b)->name << "\n";

    FeaturePlate loose;
    std::cout << "a joint built by hand is stored: " << loose.has_session() << "\n";

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
every stored record answers session() with its wood session: from a joint back to the wood session, to its two plates, to the interaction and the contacts on the same edge, to the edge's elements; a record built by hand has no wood session.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 5_traversal --parallel 4 && ./build/5_traversal && ../bash/publish-scene.sh --target 5_traversal

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
