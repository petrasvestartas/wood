#include "wood_session.h"
#include "src/templates/timber_floor/floor_product.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// Floors whose sections follow their spans by each producer's own equations: BauBuche joists, Kerto ribs and open boxes, CLT BOX, Lignatur, Kielsteg, VMG beams and joists.
int main() {

    std::vector<std::pair<std::string, FloorProduct>> products;

    for (const double span : {4000.0, 5000.0, 6000.0, 7000.0}) {
        const size_t metres = static_cast<size_t>(span / 1000.0);
        products.push_back({fmt::format("baubuche_{}", metres), FloorProduct::baubuche(span)});
        products.push_back({fmt::format("kerto_rib_{}", metres), FloorProduct::kerto_ripa(span, false)});
        products.push_back({fmt::format("kerto_box_{}", metres), FloorProduct::kerto_ripa(span, true)});
        products.push_back({fmt::format("clt_box_{}", metres), FloorProduct::clt_box_spanning(span)});
        products.push_back({fmt::format("lignatur_{}", metres), FloorProduct::lignatur(span)});
        products.push_back({fmt::format("kielsteg_{}", metres), FloorProduct::kielsteg_spanning(span)});
        products.push_back({fmt::format("vmg_beam_{}", metres), FloorProduct::vmg_beams(0.75 * span)});
        products.push_back({fmt::format("vmg_joist_{}", metres), FloorProduct::vmg_joists(span, 300.0)});
    }

    FloorProducts floors(products, "timber_floor_spans", 8);
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 10 of the timber floors: floors whose sections follow their spans, each by its producer's own equations (floor_sizing.h): BauBuche GL75 joists 120 every 850 under 40 (8 Hz under the permanent load), Kerto-Ripa ribs every 612 under 25 and the same with strips 300 x 43 (8 Hz), Schneider CLT BOX (span / 300 of the lasting deflection), Lignatur LFE (span / 475 under the whole load), Kielsteg (8 Hz with three fifths of the imposed load), VMG Lignum beams a metre apart over three quarters of those spans (bearing, span / 250, 1 kN, 8 Hz) and VMG Lignum joists every 300 (span / 250, 8 Hz with the deck), over 4, 5, 6 and 7 m.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_10_spans --parallel 6 && ./build/templates_timber_floor_10_spans && ../bash/publish-scene.sh --target templates_timber_floor_10_spans

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
