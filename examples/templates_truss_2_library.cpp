#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// Every design of the TrussGuide library, its overload with the default parameters, built by the Truss template, each
/// its own scene truss_<name>.
int main() {

    const std::vector<std::pair<std::string, wood_truss::TrussGuide>> guides = {
        {"pratt", wood_truss::Pratt{}},
        {"king_post", wood_truss::KingPost{}},
        {"queen_post", wood_truss::QueenPost{}},
        {"howe", wood_truss::Howe{}},
        {"fink", wood_truss::Fink{}},
        {"belgian", wood_truss::Belgian{}},
        {"scissors", wood_truss::Scissors{}},
        {"warren", wood_truss::Warren{}},
        {"warren_verticals", wood_truss::WarrenVerticals{}},
        {"pratt_bridge", wood_truss::PrattBridge{}},
        {"parker", wood_truss::Parker{}},
        {"k_truss", wood_truss::KTruss{}},
        {"cambered_verticals", wood_truss::CamberedVerticals{}},
        {"funicular_arch", wood_truss::FunicularArch{}},
        {"lens", wood_truss::Lens{}},
        {"triangulated_arch", wood_truss::TriangulatedArch{}},
        {"sloped", wood_truss::Sloped{}},
        {"pratt_flat", wood_truss::PrattFlat{}},
        {"flat_peaked", wood_truss::FlatPeaked{}},
        {"bowstring", wood_truss::Bowstring{}},
        {"pitched_posts", wood_truss::PitchedPosts{}},
        {"roof_cambered_tie", wood_truss::RoofCamberedTie{}},
        {"quarter_fan", wood_truss::QuarterFan{}},
        {"cantilever", wood_truss::Cantilever{}},
        {"cantilever_design_1", wood_truss::CantileverDesign1{}},
        {"cantilever_design_2", wood_truss::CantileverDesign2{}},
        {"cantilever_design_3", wood_truss::CantileverDesign3{}},
        {"fish_belly", wood_truss::FishBelly{}},
        {"sawtooth", wood_truss::Sawtooth{}},
        {"howe_flat", wood_truss::HoweFlat{}},
        {"three_hinged_arch", wood_truss::ThreeHingedArch{}},
        {"burr_arch", wood_truss::BurrArch{}},
        {"town_lattice", wood_truss::TownLattice{}},
        {"bowstring_arch", wood_truss::BowstringArch{}},
        {"waddell_a", wood_truss::WaddellA{}},
        {"wichert", wood_truss::Wichert{}},
        {"truss_leg_bedstead", wood_truss::TrussLegBedstead{}},
        {"kellogg", wood_truss::Kellogg{}},
        {"baltimore_a", wood_truss::BaltimoreA{}},
        {"pennsylvania_a", wood_truss::PennsylvaniaA{}},
        {"greiner", wood_truss::Greiner{}},
        {"double_intersection_pratt", wood_truss::DoubleIntersectionPratt{}},
        {"schwedler", wood_truss::Schwedler{}},
        {"double_intersection_warren", wood_truss::DoubleIntersectionWarren{}},
        {"pegram", wood_truss::Pegram{}},
        {"post", wood_truss::Post{}},
        {"bollman", wood_truss::Bollman{}},
        {"fink_bridge", wood_truss::FinkBridge{}},
        {"stearns", wood_truss::Stearns{}},
        {"crescent_crossed", wood_truss::CrescentCrossed{}},
        {"braced_arch_rib", wood_truss::BracedArchRib{}},
        {"belgian_verticals", wood_truss::BelgianVerticals{}},
        {"raised_tie_king_post", wood_truss::RaisedTieKingPost{}},
        {"trapezoid_crossed", wood_truss::TrapezoidCrossed{}},
        {"fink_cambered", wood_truss::FinkCambered{}},
        {"crescent_verticals", wood_truss::CrescentVerticals{}},
        {"raised_tie_posts", wood_truss::RaisedTiePosts{}},
        {"pitched_warren_columns", wood_truss::PitchedWarrenColumns{}},
        {"fink_compound", wood_truss::FinkCompound{}},
        {"fink_bent_tie", wood_truss::FinkBentTie{}},
        {"french_steep", wood_truss::FrenchSteep{}},
        {"fink_lantern", wood_truss::FinkLantern{}},
        {"arch_polygonal_tie", wood_truss::ArchPolygonalTie{}},
        {"pitched_crossed_posts", wood_truss::PitchedCrossedPosts{}},
        {"cambered_crossed_lantern", wood_truss::CamberedCrossedLantern{}},
        {"arched_crossed_columns", wood_truss::ArchedCrossedColumns{}},
        {"trapezoid_king_crossed", wood_truss::TrapezoidKingCrossed{}},
        {"topology_optimised", wood_truss::TopologyOptimised{}},
    };
    for (const std::pair<std::string, wood_truss::TrussGuide>& guide : guides) {
        wood_truss::Truss truss(guide.second, "truss_" + guide.first);
        truss.pb_dump(pb_path("truss_" + guide.first));
    }
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The TrussGuide library, its 68 designs built by the Truss template with their default parameters, each its own scene data/output/pb/truss_<name>.pb and each a TrussGuide overload taking the design's parameters (TrussGuide(Pratt{}), TrussGuide(Warren{.panels = 10}), ...): the king post, queen post, Pratt, Howe, Fink, Belgian and scissors roof trusses; the Warren, Warren with verticals, Pratt, Parker and K bridge trusses; graphic-statics form diagrams (cambered verticals, funicular arch, lens, triangulated arch, sloped, flat Pratt, flat peaked, bowstring, pitched on end posts, a roof truss with a cambered tie, a quarter fan, a cantilever and three cantilever designs from their dimensions, a fish belly, a crescent with crossed webs and a braced arch rib); HAER trusses (sawtooth, parallel-chord Howe, three-hinged arch, Burr arch, Town lattice, bowstring arch, Waddell A, Wichert, truss leg bedstead, Kellogg, Baltimore and Pennsylvania with sub-struts, Greiner, double intersection Pratt, Schwedler, double intersection Warren, Pegram, Post, Bollman, Fink bridge, Stearns); sixteen roof trusses from a catalogue, lanterns among them; and a topology-optimised beam. Each design draws its chords through their nodes and its webs and end posts between theirs (red), parameterized by span, heights, divisions and the two half sections. The template makes a beam per chord run, web and post and every joint from how the lines meet: runs mitered at kinks, heels by JointBeam::butt, web and post ends top_side on the runs and on other webs, a chord running on over a lone end, the webs at a node mitered with their neighbours, and a cross half lap where two webs cross.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_truss_2_library --parallel 6 && ./build/templates_truss_2_library && ../bash/publish-scene.sh --target templates_truss_2_library

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
