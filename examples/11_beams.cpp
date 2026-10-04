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

    for (const std::shared_ptr<JointBeam>& joint : wood_session.get_elements<JointBeam>()) {
        const std::shared_ptr<Beam> a = wood_session.get_element<Beam>(joint->targets[0]);
        const std::shared_ptr<Beam> b = wood_session.get_element<Beam>(joint->targets[1]);
        const int end = joint->feature.end_type;
        std::cout << fmt::format("{} with {}: end type {} ({}), {} volume rectangles\n", a->name, b->name, end, end == 0 ? "crossing" : end == 1 ? "side to end" : "end to end", joint->feature.volumes.size());
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
cmake --build build --target 11_beams --parallel 4 && ./build/11_beams && ../bash/publish-scene.sh --target 11_beams

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
