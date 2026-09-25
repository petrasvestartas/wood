#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Polyline FOOTPRINT({Point(0.0, 0.0, 0.0), Point(13716.0, 13716.0, 0.0), Point(36576.0, 13716.0, 0.0), Point(36576.0, 27432.0, 0.0), Point(0.0, 27432.0, 0.0), Point(0.0, 0.0, 0.0)}); // a pentagon with one diagonal side
const double HEIGHT = 10972.8;
const std::vector<double> ELEVATIONS = {0.0, 3657.6, 7315.2, 10972.8};
const wood_grid::Pattern PATTERN = wood_grid::Pattern::orthogonal(wood_grid::compute_bays(36576.0, 9144.0), wood_grid::compute_bays(27432.0, 9144.0));
const wood_grid::Framing FRAMING{.system = 2, .span = 0, .spacing = 3048.0, .node = 0};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_solid_prism");
    const Mesh massing = Mesh::loft({FOOTPRINT}, {FOOTPRINT.transformed(Xform::translation(0.0, 0.0, HEIGHT))});
    wood_grid::Building::from_solid(massing, ELEVATIONS, PATTERN).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Workflow A on a non-orthogonal prism: a pentagon with one diagonal side lofted straight up and sliced at three storeys, the orthogonal pattern clipped by every section, girders on the x lines and purlin stations across the bays, heads under the members; the edge member on the diagonal runs through with every girder, purlin and deck meeting it on an oblique cut. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_solid_prism --parallel 4 && ./build/templates_grid_solid_prism && ../bash/publish-scene.sh --target templates_grid_solid_prism

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
