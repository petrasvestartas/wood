#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Polyline PODIUM = Polyline::rectangle(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), 40000.0, 30000.0);
const Polyline TOWER = Polyline::rectangle(Point(10000.0, 5000.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), 20000.0, 20000.0); // inside the podium, its sides on the pattern lines
const double PODIUM_TOP = 8000.0;
const double TOWER_TOP = 22400.0;
const std::vector<double> ELEVATIONS = {0.0, 4000.0, 8000.0, 11600.0, 15200.0, 18800.0, 22400.0};
const wood_grid::Pattern PATTERN = wood_grid::Pattern::orthogonal(wood_grid::compute_bays(40000.0, 5000.0), wood_grid::compute_bays(30000.0, 5000.0));
const wood_grid::Framing FRAMING{.system = 1, .span = 1, .node = 1};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

/// The podium and the tower as one closed shell: the faces of the podium, podium roof and tower lofts, and the two caps.
Mesh compute_massing() {

    const Polyline roof = PODIUM.transformed(Xform::translation(0.0, 0.0, PODIUM_TOP));
    const Polyline foot = TOWER.transformed(Xform::translation(0.0, 0.0, PODIUM_TOP));
    const Polyline top = TOWER.transformed(Xform::translation(0.0, 0.0, TOWER_TOP));

    std::vector<Polyline> faces = {PODIUM, top};
    for (const Mesh& part : {Mesh::loft({PODIUM}, {roof}, false), Mesh::loft({roof}, {foot}, false), Mesh::loft({foot}, {top}, false)})
        for (const Polyline& face : part.face_outlines())
            faces.push_back(face);

    return Mesh::from_polylines(faces, 1.0);
}

int main() {

    WoodSession wood_session("templates_grid_solid_setback");
    wood_grid::Building::from_solid(compute_massing(), ELEVATIONS, PATTERN).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Workflow A on a setback massing: a two storey podium with a tower of four storeys standing on it, one closed shell sliced at every elevation; at the podium roof the section just below is the podium and just above the tower, the plan fills their union so the terrace deck covers the podium ring, and the tower ring is added as lines to the roof plan so every tower column stands on a vertex of it with a podium column or girder below, no transfer. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_solid_setback --parallel 4 && ./build/templates_grid_solid_setback && ../bash/publish-scene.sh --target templates_grid_solid_setback

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
