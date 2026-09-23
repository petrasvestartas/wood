#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    WoodSession wood_session("beams");
    wood_session.add(std::make_shared<Beam>(Polyline({Point(-1000, 0, 0), Point(1000, 0, 0)}), 60.0));
    wood_session.add(std::make_shared<Beam>(Polyline({Point(0, -1000, 0), Point(0, 1000, 0)}), 60.0));
    wood_session.add(std::make_shared<Beam>(Polyline({Point(0, 0, 0), Point(0, 0, 1000)}), 60.0));

    wood_session.compute_axis_contacts(5.0);
    wood_session.compute_beam_features(400.0, 0.9, 1);

    for (const auto& [guid, interaction] : wood_session.interactions) {
        const auto [a, b] = wood_session.edge_of(interaction);
        for (const InteractionFeature& feature : interaction.features)
            if (const FeatureBeam* beam = feature.beam())
                std::cout << fmt::format("{} with {}: end type {} ({}), {} volume rectangles\n", wood_session.get_element<Beam>(a)->name, wood_session.get_element<Beam>(b)->name, beam->end_type, beam->end_type == 0 ? "crossing" : beam->end_type == 1 ? "side to end" : "end to end", beam->volumes.size());
    }

    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
beams built in code, the closest axis segments as axis contacts, and one beam feature per pair: four volume rectangles trimmed for a crossing, a side-to-end or an end-to-end meeting.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 9_beams --parallel 4 && ./build/9_beams && ../bash/publish-scene.sh --target 9_beams

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
