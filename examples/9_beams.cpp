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

    for (const std::tuple<std::string, std::string>& pair : wood_session.graph.get_edges()) {

        const Edge& edge = wood_session.graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair));
        const std::shared_ptr<Beam> a = wood_session.get_element<Beam>(edge.v0);
        const std::shared_ptr<Beam> b = wood_session.get_element<Beam>(edge.v1);

        for (const std::shared_ptr<Interaction>& interaction : wood_session.get_interaction(a, b))
            if (const InteractionFeatureBeam* joint = dynamic_cast<const InteractionFeatureBeam*>(interaction.get()))
                std::cout << fmt::format("{} with {}: end type {} ({}), {} volume rectangles\n", a->name, b->name, joint->end_type, joint->end_type == 0 ? "crossing" : joint->end_type == 1 ? "side to end" : "end to end", joint->volumes.size());
    }

    wood_session.pb_dump(pb_path("live"));

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
