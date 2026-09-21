#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession scene = WoodSession::yaml_load(DATASET);
    scene.compute_contacts();
    scene.compute_features();

    const std::string bytes = scene.pb_dumps();
    const WoodSession back = WoodSession::pb_loads(bytes);
    std::cout << back.interactions.size() << " interactions and " << back.get_features().size() << " features read back, consistent " << back.consistent() << "\n";

    const Session kernel = Session::pb_loads(bytes);
    std::cout << "the kernel opens the same bytes: " << kernel.objects.elements->size() << " elements, " << kernel.graph.number_of_edges() << " edges\n";

    const FeaturePlate joint = scene.get_plate_features().front();
    const nlohmann::ordered_json json = joint.jsondump();
    std::cout << "one joint as JSON: type " << json["type"] << ", name " << json["name"] << ", " << json.size() << " keys\n";
    std::cout << "and back: " << FeaturePlate::jsonload(json).name << "\n";

    std::cout << "one contact as protobuf: " << scene.get_contacts().front().pb_dumps().size() << " bytes\n";
    std::cout << "the settings as JSON: distance " << scene.settings.jsondump()["distance"] << "\n";

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the scene as bytes and back with every record intact, the same bytes opened by the plain kernel Session, one record as JSON derived from its proto message and back, one record as protobuf bytes.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 10_serialization --parallel 4 && ./build/10_serialization && ../bash/publish-scene.sh --target 10_serialization

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
