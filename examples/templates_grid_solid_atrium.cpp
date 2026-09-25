#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const double SIDE = 36000.0;
const double HOLE = 12000.0; // the atrium, centred, through every level
const double HEIGHT = 20000.0;
const std::vector<double> ELEVATIONS = {0.0, 4000.0, 8000.0, 12000.0, 16000.0, 20000.0};
const wood_grid::Pattern PATTERN = wood_grid::Pattern::orthogonal(wood_grid::compute_bays(SIDE, 6000.0), wood_grid::compute_bays(SIDE, 6000.0));
const wood_grid::Framing FRAMING{.system = 2, .span = 0, .spacing = 3000.0, .node = 2};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    WoodSession wood_session("templates_grid_solid_atrium");
    const std::vector<Polyline> bottom = {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, SIDE, SIDE), Polyline::rectangle(Point((SIDE - HOLE) / 2.0, (SIDE - HOLE) / 2.0, 0.0), X, Y, HOLE, HOLE)};
    const std::vector<Polyline> top = {bottom[0].transformed(Xform::translation(0.0, 0.0, HEIGHT)), bottom[1].transformed(Xform::translation(0.0, 0.0, HEIGHT))};
    wood_grid::Building::from_solid(Mesh::loft(bottom, top), ELEVATIONS, PATTERN).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Workflow A on a massing with a through hole: a square block with a centred atrium lofted between an outer and an inner ring, sliced at five storeys; every section comes back as an outer ring with a hole, the pattern crossing at the centre falls in the hole and gets no column, the atrium ring joins the arrangement so edge members and columns surround it, girders on the x lines, purlin stations across the bays, columns through the levels with every deck notched round them. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_solid_atrium --parallel 4 && ./build/templates_grid_solid_atrium && ../bash/publish-scene.sh --target templates_grid_solid_atrium

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
