#include "wood_session.h"
#include "src/templates/curved_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Buri, Stotz and Weinand's curved folded plate (IBOIS, 2011): an undulating corrugation profile folded at the reverse folds of an arch of five segments spanning 13.5 m, every half wave of every segment a curved plate.
int main() {

    const double climb = 60.0 * Tolerance::TO_RADIANS;
    const Vector along(0.0, std::cos(climb), std::sin(climb));
    const Vector up(0.0, -std::sin(climb), std::cos(climb));
    ReflectionFold fold(
        ReflectionFold::waves(Point(-4500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), up, 6, 1500.0, 400.0),
        along,
        Vector(1.0, 0.0, 0.0),
        18100.0,
        {{3620.0, -30.0 * Tolerance::TO_RADIANS}, {7240.0, -30.0 * Tolerance::TO_RADIANS}, {10860.0, -30.0 * Tolerance::TO_RADIANS}, {14480.0, -30.0 * Tolerance::TO_RADIANS}}
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> panels = fold.get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("curved folded plate: {} lamellas, {} closed solids, development distortion {:.2e}, ruling twist {:.2e}\n", panels.size(), solids, fold.distortion(), fold.twist());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the curved folding templates: the curved folded plate of Buri, Stotz and Weinand (IBOIS EPFL, Curved Folded Plate Timber Structures, IABSE-IASS 2011; built as the Mendrisio pavilion of 2013, 13.5 m span, 77 mm curved cross-laminated timber), made by Mitani's reflection. Buri's doubly corrugated surface has a corrugation profile across the span and a cross-section profile along it; with an undulating corrugation profile the surface is a cylinder, and every kink of the cross-section profile, a reverse fold, is a reflection of the sheet in the plane bisecting the kink. Here the corrugation is three sine waves of 3 m wavelength and 400 mm amplitude across a 9 m wide sheet, swept along a polygonal arch of five segments of 3.62 m turning 30 degrees at each of four reverse folds, from 60 degrees up at one foot to 60 degrees down at the other, spanning 13.5 m and rising 4.9 m; the stations are measured along the rulings from the sheet's first corner. Every crease is the bisecting plane's section of the corrugation, a planar undulating curve, and every half wave of every segment is one lamella. Every part of the sheet between two creases and two arcs is a lamella, a PlateCurved: the exact cylinder between its two creases, 120 mm thick on the sheet's physical underside, its side faces the mitre where it meets the next plate across the crease, the crease plane at a mirror fold and the bisector surface at a reflection, its caps the straight rulings at the arcs' cusps and the sheet's edges; every lamella unfolds flat, its development drawn where it lies on the unrolled sheet in the group flat. Groups: creases, lamellas (lamella_<segment>_<row>), flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_3_curved_folded_plate --parallel 6 && ./build/templates_curved_folding_3_curved_folded_plate && ../bash/publish-scene.sh --target templates_curved_folding_3_curved_folded_plate

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
