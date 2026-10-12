#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// Every truss of the TrussGuide library built by the Truss template, each its own scene truss_<name>.
int main() {

    const std::vector<std::pair<std::string, wood_truss::TrussGuide>> guides = {
        {"pratt", wood_truss::TrussGuide::pratt()},
        {"king_post", wood_truss::TrussGuide::king_post()},
        {"queen_post", wood_truss::TrussGuide::queen_post()},
        {"howe", wood_truss::TrussGuide::howe()},
        {"fink", wood_truss::TrussGuide::fink()},
        {"belgian", wood_truss::TrussGuide::belgian()},
        {"scissors", wood_truss::TrussGuide::scissors()},
        {"warren", wood_truss::TrussGuide::warren()},
        {"warren_verticals", wood_truss::TrussGuide::warren_verticals()},
        {"pratt_bridge", wood_truss::TrussGuide::pratt_bridge()},
        {"parker", wood_truss::TrussGuide::parker()},
        {"k_truss", wood_truss::TrussGuide::k_truss()},
        {"cambered_verticals", wood_truss::TrussGuide::cambered_verticals()},
        {"funicular_arch", wood_truss::TrussGuide::funicular_arch()},
        {"lens", wood_truss::TrussGuide::lens()},
        {"triangulated_arch", wood_truss::TrussGuide::triangulated_arch()},
        {"sloped", wood_truss::TrussGuide::sloped()},
        {"pratt_flat", wood_truss::TrussGuide::pratt_flat()},
        {"flat_peaked", wood_truss::TrussGuide::flat_peaked()},
        {"bowstring", wood_truss::TrussGuide::bowstring()},
        {"pitched_posts", wood_truss::TrussGuide::pitched_posts()},
        {"roof_cambered_tie", wood_truss::TrussGuide::roof_cambered_tie()},
        {"quarter_fan", wood_truss::TrussGuide::quarter_fan()},
        {"cantilever", wood_truss::TrussGuide::cantilever()},
        {"cantilever_design_1", wood_truss::TrussGuide::cantilever_design_1()},
        {"cantilever_design_2", wood_truss::TrussGuide::cantilever_design_2()},
        {"cantilever_design_3", wood_truss::TrussGuide::cantilever_design_3()},
        {"fish_belly", wood_truss::TrussGuide::fish_belly()},
        {"sawtooth", wood_truss::TrussGuide::sawtooth()},
        {"howe_flat", wood_truss::TrussGuide::howe_flat()},
        {"three_hinged_arch", wood_truss::TrussGuide::three_hinged_arch()},
        {"burr_arch", wood_truss::TrussGuide::burr_arch()},
        {"town_lattice", wood_truss::TrussGuide::town_lattice()},
        {"bowstring_arch", wood_truss::TrussGuide::bowstring_arch()},
        {"waddell_a", wood_truss::TrussGuide::waddell_a()},
        {"wichert", wood_truss::TrussGuide::wichert()},
        {"truss_leg_bedstead", wood_truss::TrussGuide::truss_leg_bedstead()},
        {"kellogg", wood_truss::TrussGuide::kellogg()},
        {"baltimore_a", wood_truss::TrussGuide::baltimore_a()},
        {"pennsylvania_a", wood_truss::TrussGuide::pennsylvania_a()},
        {"greiner", wood_truss::TrussGuide::greiner()},
        {"double_intersection_pratt", wood_truss::TrussGuide::double_intersection_pratt()},
        {"schwedler", wood_truss::TrussGuide::schwedler()},
        {"double_intersection_warren", wood_truss::TrussGuide::double_intersection_warren()},
        {"pegram", wood_truss::TrussGuide::pegram()},
        {"post", wood_truss::TrussGuide::post()},
        {"bollman", wood_truss::TrussGuide::bollman()},
        {"fink_bridge", wood_truss::TrussGuide::fink_bridge()},
        {"stearns", wood_truss::TrussGuide::stearns()},
        {"crescent_crossed", wood_truss::TrussGuide::crescent_crossed()},
        {"braced_arch_rib", wood_truss::TrussGuide::braced_arch_rib()},
        {"belgian_verticals", wood_truss::TrussGuide::belgian_verticals()},
        {"raised_tie_king_post", wood_truss::TrussGuide::raised_tie_king_post()},
        {"trapezoid_crossed", wood_truss::TrussGuide::trapezoid_crossed()},
        {"fink_cambered", wood_truss::TrussGuide::fink_cambered()},
        {"crescent_verticals", wood_truss::TrussGuide::crescent_verticals()},
        {"raised_tie_posts", wood_truss::TrussGuide::raised_tie_posts()},
        {"pitched_warren_columns", wood_truss::TrussGuide::pitched_warren_columns()},
        {"fink_compound", wood_truss::TrussGuide::fink_compound()},
        {"fink_bent_tie", wood_truss::TrussGuide::fink_bent_tie()},
        {"french_steep", wood_truss::TrussGuide::french_steep()},
        {"fink_lantern", wood_truss::TrussGuide::fink_lantern()},
        {"arch_polygonal_tie", wood_truss::TrussGuide::arch_polygonal_tie()},
        {"pitched_crossed_posts", wood_truss::TrussGuide::pitched_crossed_posts()},
        {"cambered_crossed_lantern", wood_truss::TrussGuide::cambered_crossed_lantern()},
        {"arched_crossed_columns", wood_truss::TrussGuide::arched_crossed_columns()},
        {"trapezoid_king_crossed", wood_truss::TrussGuide::trapezoid_king_crossed()},
    };
    for (const std::pair<std::string, wood_truss::TrussGuide>& guide : guides) {
        wood_truss::Truss truss(guide.second, "truss_" + guide.first);
        truss.pb_dump(pb_path("truss_" + guide.first));
    }
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The TrussGuide library, all 80 guides built by the Truss template with their default parameters, each its own scene data/output/pb/truss_<name>.pb: the king post, queen post, Pratt, Howe, Fink, Belgian and scissors roof trusses; the Warren, Warren with verticals, Pratt, Parker and K bridge trusses; the user's graphic-statics form diagrams (cambered verticals, funicular arch, lens, triangulated arch, sloped, flat Pratt, flat peaked, bowstring, pitched on end posts, three roof trusses with a straight, a raised and a cambered tie, a quarter fan, six cantilevers, three cantilever designs from their dimensions, a fish belly, a crescent with crossed webs and a braced arch rib); every other HAER truss (sawtooth, parallel-chord Howe, three-hinged arch, Burr arch, Town lattice, bowstring arch, Waddell A, Wichert, Pratt half-hip, truss leg bedstead, camelback, camelback with subdivided panels, Kellogg, Baltimore and Pennsylvania with sub-struts and sub-ties, lenticular, Greiner, double intersection Pratt, Schwedler, double intersection Warren, Pegram, Post, Bollman, Fink bridge, Stearns); and sixteen roof trusses from a catalogue, lanterns among them. Each guide is only lines, parameterized by span, heights, divisions and the two half sections: chords as polylines through their nodes, webs and end posts as lines (red). The template makes a beam per chord run, web and post and every joint from how the lines meet: runs mitered at kinks, heels by JointBeam::butt, web and post ends top_side on the runs and on other webs, a chord running on over a lone end, the webs at a node mitered with their neighbours, and a cross half lap where two webs cross.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_truss_2_library --parallel 6 && ./build/templates_truss_2_library && ../bash/publish-scene.sh --target templates_truss_2_library

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
