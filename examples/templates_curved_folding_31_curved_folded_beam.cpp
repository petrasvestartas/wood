#include "wood_session.h"
#include "src/templates/curved_folding/curved_folded_beam.h"

using namespace session_cpp;
using namespace wood_session;

/// Six closed curved-folded beams side by side: boxes of four cylinder plates spanning 12 m, the top and bottom bulging out and the
/// sides sagging in toward mid-span.
int main() {

    CurvedFoldedBeam beams;
    std::cout << beams << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> plates = beams.get_elements_placed<PlateCurved>("beam");

    for (const std::shared_ptr<PlateCurved>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("curved folded beam: {} plates, {} closed solids, {} contacts, development distortion {:.2e}\n", plates.size(), solids, beams.get_contacts().size(), beams.distortion());
    beams.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 31 of the curved folding templates: a closed curved-folded beam of four plates. A box spans 12 m along x, 800 mm square at the supports; each of its four plates is a cylinder bent along the span with its rulings straight across, and the two pairs fold opposite ways: the top and bottom plates bulge 250 mm outward toward mid-span, the sides sag 220 mm inward, so the beam is 1300 mm deep and 360 mm wide at mid-span. The corners are the curved creases; every plate is mitred there, its inner face ending where it meets its neighbour's a thickness inside both, the mitre the ruled face between the crease and that inner crease. The ends are cut square at the supports. Six beams 27 mm thick lie side by side, touching at the supports, lens-shaped gaps opening between them toward mid-span; the session's contact detection finds the four mitres of every beam. Groups: beam_<b> (beam_<b>_<k>).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_31_curved_folded_beam --parallel 6 && ./build/templates_curved_folding_31_curved_folded_beam && ../bash/publish-scene.sh --target templates_curved_folding_31_curved_folded_beam

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
