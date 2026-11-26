#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_tube.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// Composite flat-foldable cellular curved-crease vault of six parallelogram tubes along a space curve.
int main() {

    CurvedCreaseTube vault(CurvedCreaseTubeGuide::cellular_vault());
    std::cout << vault << std::endl;
    std::cout << fmt::format(
        "cellular tube vault: {} plates and flat pieces, {} contacts, distortion {:.2e}, min speed {:.3f}, midspan taper {:.3f}\n",
        vault.get_elements<FaceCurvedElement>().size(),
        vault.get_contacts().size(),
        vault.distortion(),
        vault.min_speed(),
        vault.midspan_taper()
    );
    vault.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 6 of the developable folding templates: composite flat-foldable curved-crease tubular vault (Tachi 2013), CurvedCreaseTubeGuide::cellular_vault. Six parallelogram tubes chained along a space curve of constant curvature and harmonic torsion (a = 0.30, T(0.5 * s_0) . T(0) = 0 for coplanar horizontal supports over an 8 m span at 2 * opening = 90 degrees, cross-section widths 480 x 360 mm tapering by 1.35x at mid-arch). The 24 walls are first surfaces without thickness, in walls, with their spine, ridge and keel creases and their crease pattern; the thickness comes last: every wall a 24 mm plate centred on it and mitered where the walls meet, and its flat piece for cutting in flat. Groups: walls, creases, pattern, plates, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_6_cellular_tube_vault --parallel 6 && ./build/templates_developable_folding_6_cellular_tube_vault && ../bash/publish-scene.sh --target templates_developable_folding_6_cellular_tube_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
