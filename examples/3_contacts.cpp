#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_contacts();
    wood_session.compute_cross_contacts();
    wood_session.compute_line_contacts();

    for (const auto& [guid, interaction] : wood_session.interactions) {
        const auto [a, b] = wood_session.edge_of(interaction);
        for (const InteractionContact& contact : interaction.contacts) {
            if (const ContactFace* face = contact.face())
                std::cout << fmt::format("face   {} f{} with {} f{} class {} points {}\n", a.substr(0, 8), face->face_a, b.substr(0, 8), face->face_b, contact_type_name(face->type), face->polygon.point_count());
            if (const ContactAxis* axis = contact.axis())
                std::cout << fmt::format("axis   {} s{} with {} s{} gap {:.3f}\n", a.substr(0, 8), axis->segment_a, b.substr(0, 8), axis->segment_b, axis->segment.length());
            if (const ContactCross* cross = contact.cross())
                std::cout << fmt::format("cross  {} f{},{} with {} f{},{}\n", a.substr(0, 8), cross->faces_a[0], cross->faces_a[1], b.substr(0, 8), cross->faces_b[0], cross->faces_b[1]);
        }
    }

    std::cout << wood_session.get_contacts().size() << " contacts on " << wood_session.interactions.size() << " interactions\n";

    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the three contact kinds on one dataset: face overlaps, boundary crossings as axis contacts, plates passing through each other as cross contacts; each read through the interaction of its edge.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 3_contacts --parallel 4 && ./build/3_contacts && ../bash/publish-scene.sh --target 3_contacts

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
