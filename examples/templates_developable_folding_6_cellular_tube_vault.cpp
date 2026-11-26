#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_tube.h"

using namespace session_cpp;
using namespace wood_session;

/// Composite flat-foldable cellular curved-crease vault assembled from six space-curve parallelogram tubes.
int main() {

    CurvedCreaseTube vault(
        8000.0,
        0.30,
        45.0 * Tolerance::TO_RADIANS,
        480.0,
        360.0,
        6,
        24.0
    );
    std::cout << vault << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<FaceCurvedElement>> plates =
        vault.get_elements_placed<FaceCurvedElement>("tube");
    const std::vector<std::shared_ptr<FaceCurvedElement>> flat_pieces =
        vault.get_elements_placed<FaceCurvedElement>("flat");

    for (const std::shared_ptr<FaceCurvedElement>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    size_t flat_solids = 0;
    for (const std::shared_ptr<FaceCurvedElement>& piece : flat_pieces)
        flat_solids += piece->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "cellular tube vault: {} plates ({} solids), {} unfolded flat pieces ({} solids), {} contacts, "
        "distortion {:.2e}, min speed {:.3f}, midspan taper {:.3f}\n",
        plates.size(),
        solids,
        flat_pieces.size(),
        flat_solids,
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
Step 6 of the developable folding templates: composite flat-foldable curved-crease tubular vault (Tachi 2013). Six parallelogram tubes (24 curved timber plates of 24 mm thickness) chained along non-planar space curves with constant curvature and harmonic torsion (a = 0.30, T(0.5 * s_0) . T(0) = 0 for coplanar horizontal supports over an 8 m span at 2 * opening = 90 degrees, initial cross-section widths 480 x 360 mm tapering by 1.35x at mid-arch while preserving constant parallelogram area). Every plate unfolds isometrically into its beveled 3D flat piece in group flat. Groups: plates, creases, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_6_cellular_tube_vault --parallel 6 && ./build/templates_developable_folding_6_cellular_tube_vault && ../bash/publish-scene.sh --target templates_developable_folding_6_cellular_tube_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
