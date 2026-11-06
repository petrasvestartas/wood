#include "wood_session.h"
#include "src/templates/cylindrical_folding/reflection_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Rulings per curve: enough for a smooth barrel, the curves within 0.05 mm of the default 24, the scene a fraction of the size.
const size_t SAMPLES = 6;

/// The Mendrisio pavilion.
int main() {

    const Vector along(0.1719, 0.0, 0.9851);
    ReflectionFold fold(
        {Primitives::arc(
            Point(6000.0, -2250.0, 0.0), Point(6400.0, 0.0, 0.0), Point(6000.0, 2250.0, 0.0)
        )},
        along,
        Vector(0.0, -1.0, 0.0),
        12000.0,
        {{2923.0, 89.9 * Tolerance::TO_RADIANS, true}, {3623.0, 10.0 * Tolerance::TO_RADIANS, true}
        },
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        77.0,
        "reflection_fold",
        SAMPLES
    );
    std::cout << fold << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<FaceCurvedElement>> panels =
        fold.get_elements_placed<FaceCurvedElement>("lamella");

    for (const std::shared_ptr<FaceCurvedElement>& panel : panels)
        solids += panel->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "mendrisio pavilion: {} lamellas, {} closed solids, {} contacts, development distortion "
        "{:.2e}, ruling twist {:.2e}\n",
        panels.size(),
        solids,
        fold.get_contacts().size(),
        fold.distortion(),
        fold.twist()
    );
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the cylindrical folding templates: the Mendrisio pavilion of IBOIS, 13 m long at the top, 4.5 m wide and 3.4 m high, 77 mm curved cross-laminated timber. One shallow barrel, an arc 4.5 m wide and 400 mm high, stands as the end wall leaning out, is reflected at the eave onto the roof rising 10 degrees, a trough, and 700 mm further at the ridge crease onto the level middle, a vault whose rulings cross the plane of symmetry at mid-length square, so it is one plate from ridge to ridge. Five curved plates, wall, trough, middle, trough, wall, each an exact cylinder, mitred on the four creases. Groups: creases, lamellas, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cylindrical_folding_3_mendrisio_pavilion --parallel 6 && ./build/templates_cylindrical_folding_3_mendrisio_pavilion && ../bash/publish-scene.sh --target templates_cylindrical_folding_3_mendrisio_pavilion

|||||||| TIME ||||||||
1.32 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
