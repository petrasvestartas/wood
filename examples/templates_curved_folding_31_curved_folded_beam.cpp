#include "wood_session.h"
#include "src/templates/curved_folding/curved_folded_beam.h"

using namespace session_cpp;
using namespace wood_session;

/// Six closed curved-folded beams side by side: boxes 800 wide and 400 high at the supports, the top and bottom bulging out, the
/// sides sagging in, five webs across the span.
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
Step 31 of the curved folding templates: a closed curved-folded beam. A box spans 12 m along x, 800 mm wide and 400 mm high at the supports; its four plates are cylinders bent along the span with their rulings straight across: the top and bottom bulge 250 mm outward toward mid-span, the sides sag 80 mm inward, so the flat section at the supports becomes 640 mm wide and 900 mm deep at mid-span. Every corner is a curved crease, the two plates mitred on the ruled face between the crease and the inner crease. Five flat webs across the span divide the box, each filling its inner section, its four edges on the plates' inner faces. Six beams of 27 mm plates side by side; the contacts are the four mitres and the four edges of every web. Groups: beam_<b>.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_31_curved_folded_beam --parallel 6 && ./build/templates_curved_folding_31_curved_folded_beam && ../bash/publish-scene.sh --target templates_curved_folding_31_curved_folded_beam

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
