#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const int VARIANT = 0; // FAST+EPP Timber Bay Tool v1 to v4, 0 to 3
const std::vector<double> XS = {9000.0, 6000.0, 6000.0, 8000.0}; // the short side, the girders run along it
const std::vector<double> YS = {9000.0, 12000.0, 9000.0, 12000.0}; // the long side, the purlins run along it
const std::vector<double> ELEVATIONS = {0.0, 4500.0};
const std::vector<wood_grid::Framing> FRAMINGS = {
    wood_grid::Framing{.system = 2, .span = 0, .spacing = 2250.0, .node = 1, .deck = 87.0, .panel = 3114.0, .profiles = {.column = profile_rectangle(315.0, 342.0), .girder = profile_rectangle(265.0, 608.0), .purlin = profile_rectangle(215.0, 456.0)}},
    wood_grid::Framing{.system = 2, .span = 0, .spacing = 1500.0, .node = 1, .deck = 87.0, .panel = 3095.0, .profiles = {.column = profile_rectangle(265.0, 380.0), .girder = profile_rectangle(265.0, 532.0), .purlin = profile_rectangle(215.0, 532.0)}},
    wood_grid::Framing{.system = 1, .span = 1, .node = 1, .deck = 243.0, .panel = 3126.67, .profiles = {.column = profile_rectangle(265.0, 380.0), .girder = profile_rectangle(265.0, 836.0), .edge_girder = profile_rectangle(215.0, 836.0)}},
    wood_grid::Framing{.system = 2, .span = 0, .spacing = 8000.0 / 6.0, .node = 1, .deck = 87.0, .panel = 3104.5, .profiles = {.column = profile_rectangle(365.0, 418.0), .girder = profile_rectangle(265.0, 684.0), .purlin = profile_rectangle(215.0, 494.0)}},
}; // v3 spans its 243 CLT the whole bay between two beams on the column lines, the others hang purlins from the girders under 87 CLT
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_fastepp");
    wood_grid::Building building = wood_grid::Building::from_footprint({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, XS[VARIANT], YS[VARIANT])}, ELEVATIONS, wood_grid::Pattern::orthogonal({XS[VARIANT]}, {YS[VARIANT]}));

    if (VARIANT == 2)
        for (const std::pair<size_t, size_t>& edge : building.levels[1].plan.edges())
            if (building.levels[1].plan.edge_attribute(edge, "family").value_or(-1.0) == 0.0)
                building.levels[1].plan.set_edge_attribute(edge, "role", 0.0); // FAST+EPP draws no beam on the short sides of v3

    building.to_session(wood_session, FRAMINGS[VARIANT]);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The four FAST+EPP timber bay variants on the grid template, one storey each, the datum at the framing top so the members overlay the reference to the millimetre: columns flush with the datum, girders along the short side cut by the column faces, edge purlins on the column lines, interior purlins at the tool's spacing cut by the girder sides, CLT strips over the outer column faces; v3 has no purlins and no beam on its short sides, two overrides on the plan edges. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_fastepp --parallel 4 && ./build/templates_grid_fastepp && ../bash/publish-scene.sh --target templates_grid_fastepp

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
