#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    WoodSession scene("beams");
    scene.add(std::make_shared<Beam>(Polyline({Point(-1000, 0, 0), Point(1000, 0, 0)}), 60.0));
    scene.add(std::make_shared<Beam>(Polyline({Point(0, -1000, 0), Point(0, 1000, 0)}), 60.0));
    scene.add(std::make_shared<Beam>(Polyline({Point(0, 0, 0), Point(0, 0, 1000)}), 60.0));

    scene.compute_axis_contacts(5.0);
    scene.compute_beam_features(400.0, 0.9, 1);

    for (const auto& [guid, interaction] : scene.interactions) {
        const auto [a, b] = scene.edge_of(interaction);
        for (const InteractionFeature& feature : interaction.features)
            if (const FeatureBeam* beam = feature.beam())
                std::cout << fmt::format("{} with {}: end type {} ({}), {} volume rectangles\n", scene.get_element<Beam>(a)->name, scene.get_element<Beam>(b)->name, beam->end_type, beam->end_type == 0 ? "crossing" : beam->end_type == 1 ? "side to end" : "end to end", beam->volumes.size());
    }

    scene.add_to_tree();
    scene.pb_dump(pb_path("live").string());

    return 0;
}

/*
description: beams built in code, the closest axis segments as axis contacts, and one beam feature per pair: four volume rectangles trimmed for a crossing, a side-to-end or an end-to-end meeting.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target 9_beams --parallel 4 && ./build/9_beams
cloudflare: ../bash/publish-scene.sh --target 9_beams
view: https://petrasvestartas.github.io/session/
*/
