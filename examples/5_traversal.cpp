#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_contacts();
    wood_session.compute_features();

    const InteractionFeaturePlate joint = wood_session.get_plate_features().front();
    const std::shared_ptr<Plate> male = wood_session.get_element<Plate>(joint.element_a);
    const std::shared_ptr<Plate> female = wood_session.get_element<Plate>(joint.element_b);
    std::cout << "joint " << joint.name << " cuts " << male->name << " and " << female->name << "\n";

    const std::vector<std::shared_ptr<Interaction>> interactions = wood_session.get_interaction(female, male);
    std::cout << "their edge holds " << interactions.size() << " interactions\n";

    for (const std::shared_ptr<Interaction>& interaction : interactions)
        if (interaction->guid() == joint.contact_guid)
            std::cout << "the joint was solved from " << *interaction << "\n";

    const Edge& edge = wood_session.graph.edges.at(male->guid()).at(female->guid());
    std::cout << "the edge joins " << wood_session.get_element<Element>(edge.v0)->name << " and " << wood_session.get_element<Element>(edge.v1)->name << "\n";

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
from a joint to its two plates by guid, to the interactions on their edge in either order, to the contact it names by guid, to the edge's elements.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 5_traversal --parallel 4 && ./build/5_traversal && ../bash/publish-scene.sh --target 5_traversal

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
