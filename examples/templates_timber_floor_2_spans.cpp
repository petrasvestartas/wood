#include "wood_session.h"
#include "src/templates/timber_floor/timber_floor.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// Floors of producers whose sections follow their spans by each producer's own equations.
int main() {

    std::vector<std::pair<std::string, TimberFloorGuide>> guides;

    for (const double span : {4000.0, 6000.0}) {
        const size_t metres = static_cast<size_t>(span / 1000.0);
        guides.push_back({fmt::format("baubuche_{}", metres), TimberFloorGuide::baubuche(span)});
        guides.push_back({fmt::format("vmg_{}", metres), TimberFloorGuide::vmg_joists(span)});
        guides.push_back({fmt::format("kerto_ripa_{}", metres), TimberFloorGuide::kerto_ripa(span, false)});
        guides.push_back({fmt::format("kerto_box_{}", metres), TimberFloorGuide::kerto_ripa(span, true)});
        guides.push_back({fmt::format("clt_ribs_{}", metres), TimberFloorGuide::clt_ribs(span)});
        guides.push_back({fmt::format("clt_box_{}", metres), TimberFloorGuide::clt_box(span)});
        guides.push_back({fmt::format("novatop_{}", metres), TimberFloorGuide::novatop(span)});
        guides.push_back({fmt::format("lignatur_{}", metres), TimberFloorGuide::lignatur(span)});
    }

    for (std::pair<std::string, TimberFloorGuide>& guide : guides)
        guide.second.layers = LAYERS_HEAVY;

    TimberFloors floors(guides, "timber_floor_spans");
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 2 of the timber floors: the floors of eight producers over 4000 and 6000, each section the least its producer's own equations allow (floor_sizing.h), all the layers on them as one of 110: BauBuche GL75 joists every 850 under 40, VMG Lignum I-joists every 600 under 22, Kerto-Ripa ribs every 612 under 25 and with strips under them, Hasslacher glulam ribs every 625 under cross-laminated 120, Schneider CLT BOX, Novatop Element and Lignatur surface elements.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_2_spans --parallel 6 && ./build/templates_timber_floor_2_spans && ../bash/publish-scene.sh --target templates_timber_floor_2_spans

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
