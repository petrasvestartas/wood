#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_contacts();
    wood_session.compute_features();

    const std::string bytes = wood_session.pb_dumps();
    const WoodSession back = WoodSession::pb_loads(bytes);
    std::cout << back.interactions.size() << " interactions and " << back.get_features().size() << " features read back, consistent " << back.consistent() << "\n";

    const Session kernel = Session::pb_loads(bytes);
    std::cout << "the kernel opens the same bytes: " << kernel.objects.elements->size() << " elements, " << kernel.graph.number_of_edges() << " edges\n";

    const InteractionFeaturePlate joint = wood_session.get_plate_features().front();
    const nlohmann::ordered_json json = joint.jsondump();
    std::cout << "one joint as JSON: type " << json["interaction_type"] << ", name " << json["name"] << ", " << json.size() << " keys\n";
    std::cout << "and back: " << *Interaction::file_json_loads(joint.file_json_dumps()) << "\n";

    std::cout << "one contact as protobuf: " << wood_session.get_contacts().front()->pb_dumps().size() << " bytes\n";
    std::cout << "the settings as JSON: distance " << wood_session.settings.jsondump()["distance"] << "\n";

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the wood session as bytes and back with every record intact, the same bytes opened by the plain kernel Session, one record as the kernel's JSON and back through the registry as its wood type, one record as protobuf bytes.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 10_serialization --parallel 4 && ./build/10_serialization && ../bash/publish-scene.sh --target 10_serialization

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
