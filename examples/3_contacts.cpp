#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_contacts();
    wood_session.compute_cross_contacts();
    wood_session.compute_line_contacts();

    for (const std::tuple<std::string, std::string>& pair : wood_session.graph.get_edges()) {

        const Edge& edge = wood_session.graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair));
        const std::shared_ptr<Element> first = wood_session.get_element<Element>(edge.v0);
        const std::shared_ptr<Element> second = wood_session.get_element<Element>(edge.v1);
        const std::string a = edge.v0.substr(0, 8);
        const std::string b = edge.v1.substr(0, 8);

        for (const std::shared_ptr<Interaction>& interaction : wood_session.get_interaction(first, second)) {

            if (const InteractionContactFace* face = dynamic_cast<const InteractionContactFace*>(interaction.get()))
                std::cout << fmt::format("face   {} f{} with {} f{} class {} points {}\n", a, face->face_a, b, face->face_b, contact_type_name(face->type), face->polygon.point_count());

            if (const InteractionContactAxis* axis = dynamic_cast<const InteractionContactAxis*>(interaction.get()))
                std::cout << fmt::format("axis   {} s{} with {} s{} gap {:.3f}\n", a, axis->segment_a, b, axis->segment_b, axis->segment.length());

            if (const InteractionContactCross* cross = dynamic_cast<const InteractionContactCross*>(interaction.get()))
                std::cout << fmt::format("cross  {} f{},{} with {} f{},{}\n", a, cross->faces_a[0], cross->faces_a[1], b, cross->faces_b[0], cross->faces_b[1]);
        }
    }

    std::cout << wood_session.get_contacts().size() << " contacts on " << wood_session.interactions.size() << " interactions\n";

    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the three contact kinds on one dataset: face overlaps, boundary crossings as axis contacts, plates passing through each other as cross contacts; each read from the interactions of its edge, oriented to the edge's first element.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 3_contacts --parallel 4 && ./build/3_contacts && ../bash/publish-scene.sh --target 3_contacts

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
