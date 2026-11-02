#include "wood_session.h"
#include "src/templates/chevron/chevron_shifted.h"

using namespace session_cpp;
using namespace wood_session;

/// The shifted chevron shell on the first Annen surface: the surface, the cell mesh, its edge planes, a box of plates per cell with its joints.
int main() {

    const std::filesystem::path path = config::session_data_dir() / "annen_surfaces.json";
    const std::vector<NurbsSurface> surfaces = wood_chevron::annen_surfaces(path.string());
    ChevronShifted shifted(surfaces[0]);
    const std::vector<std::shared_ptr<Plate>> plates =
        shifted.get_elements_numbered<Plate>("plate");
    std::cout << shifted << std::endl;
    std::cout << fmt::format(
        "chevron shifted: {} cells, {} plates\n", shifted.cells().masks.size(), plates.size()
    );
    shifted.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The shifted chevron template on the first of the 23 Annen surfaces in annen_surfaces.json, the 2018 Annen pattern: 54 iso curves across the arch at equal arc length, 9 points on each, the interior points shifted by 0.4 of a row towards the next row in even columns and the previous one in odd columns, so the cells are hexagons over three rows in staggered columns, 27 per column, with a half-cell quad at one end of every column; a plane through every edge, the even columns' long sides merged and turned by 0.05 rad, the neighbours' cross planes inserted; then per cell a top and a bottom plate 300 mm from the surface, stepped in by the 40 mm wall where the cell's own walls stand, and its walls running 80 mm past the plates, the two halves of a merged long side one wall, every wall mitred against the next wall of its box and stopped short of a neighbour's wall standing inside the cell, the odd plates' steps between two long walls cut along the insertion vector of whatever passes them, with the insertion vectors, feature axes, joint types, divisions, adjacency and three-valence groups the wood solver reads to cut the definition's own through-tenons (top_side_annen_shifted_tenons) and their slots and Christopher's Annen fingers (side_side_outplane_annen_fingers) into the 224 box corners: the 431 jointed plates of the 2020 Annen2.3dm ground truth within 0.011 mm but for the 16 on the naked long side of column 0, within 1.1 mm, and every tenon they carry within 1 mm, and the one top plate it lacks; each step in its own group (surface, mesh, edge_planes, plates with a box_<cell> group per cell, insertion, joints).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_chevron_2_shifted --parallel 6 && ./build/templates_chevron_2_shifted && ../bash/publish-scene.sh --target templates_chevron_2_shifted

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
