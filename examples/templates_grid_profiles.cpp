#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const double BAY = 6000.0;
const double GAP = 2000.0; // clear distance between the bays
const std::vector<double> ELEVATIONS = {0.0, 4500.0};
const std::vector<wood_grid::Profiles> PROFILES = {
    {.column = profile_rectangle(315.0, 342.0), .girder = profile_rectangle(265.0, 608.0)},
    {.column = profile_round(360.0), .girder = profile_round(300.0)},
    {.column = profile_w(206.0, 210.0, 14.2, 10.2), .girder = profile_w(250.0, 250.0, 15.0, 10.0)}, // W8X31 columns
    {.column = profile_hss(178.0, 178.0, 12.7), .girder = profile_hss(250.0, 250.0, 10.0)},
    {.column = profile_rectangle(315.0, 342.0), .girder = profile_double(120.0, 600.0, 60.0)},
    {.column = profile_rectangle(315.0, 342.0), .girder = profile_slab_band(1200.0, 300.0)},
    {.column = profile_rectangle(315.0, 342.0), .girder = profile_t(300.0, 500.0, 100.0, 80.0)}
}; // one purlin bay per girder and column profile of the library
const wood_grid::Framing FRAMING{.system = 2, .span = 0, .spacing = 2000.0, .node = 1, .deck = 87.0, .profiles = {.purlin = profile_rectangle(215.0, 456.0)}};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_profiles");
    const std::shared_ptr<TreeNode> group = wood_session.add_group("storey_0");

    for (size_t bay = 0; bay < PROFILES.size(); bay++) {
        wood_grid::Framing framing = FRAMING;
        framing.profiles.column = PROFILES[bay].column;
        framing.profiles.girder = PROFILES[bay].girder;

        const wood_grid::Pattern pattern = wood_grid::Pattern::orthogonal({BAY}, {BAY}).transformed(Xform::translation((BAY + GAP) * bay, 0.0, 0.0));
        for (const std::shared_ptr<Element>& element : wood_grid::Building::from_footprint({}, ELEVATIONS, pattern).to_elements(framing, 0))
            wood_session.add(element, group);
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The profile library on the grid template: seven purlin-on-girder bays in a row, one per girder profile (rectangle, round, W, HSS with its hole, double as two members with the same cuts, slab band, T) with a matching column (rectangle, round, W8X31, HSS), flush columns the girders and edge purlins frame into, purlin stations at 2000 hung flush with the girders, a deck per bay; every solid closed with its volume the profile area times its length, no clash, contacts over the whole scene. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_profiles --parallel 4 && ./build/templates_grid_profiles && ../bash/publish-scene.sh --target templates_grid_profiles

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
