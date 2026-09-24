#include "wood_session.h"
#include "src/templates/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::string INPUT = "crea_4x4"; // crea_4x4, crea_4x4_ground, crea_4x4_beams_without_column, crea_full: compas_grid's crea datasets as drawn lines and surfaces
const wood_grid::Framing FRAMING{.system = 1, .span = -1, .node = 0, .deck = 200.0, .wall = 200.0, .head = 300.0, .reach = 360.0, .profiles = {.column = profile_rectangle(300.0, 300.0), .girder = profile_rectangle(300.0, 300.0)}};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    const Session input = Session::pb_load(config::dataset_path("crea/" + INPUT + "_input", ".pb").string());

    std::vector<Line> lines;
    for (const std::vector<Polyline>& group : input.select_by_type<Polyline>())
        for (const Polyline& polyline : group)
            lines.push_back(Line::from_points(polyline.get_points().front(), polyline.get_points().back()));

    std::vector<Polyline> surfaces;
    for (const std::vector<Mesh>& group : input.select_by_type<Mesh>())
        for (const Mesh& mesh : group)
            for (const size_t face : mesh.faces()) {
                surfaces.push_back(*mesh.face_polygon(face));
                surfaces.back().name = mesh.name.find("Core") == std::string::npos ? "surface" : "core";
            }

    WoodSession wood_session("templates_grid_crea");
    wood_grid::Building::from_lines(lines, surfaces).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Workflow C on the crea dataset compas_grid ships: every column, beam, floor, facade and core quad as drawn, read from data/crea/<INPUT>_input.pb, the vertical lines as column points, the horizontal lines and floor edges as the plan of their level, the vertical quads as facade and core walls that compas_grid drops; every line a beam ending on the head tops and on its neighbours' sides, decks on the beams, walls between the column faces. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_crea --parallel 4 && ./build/templates_grid_crea && ../bash/publish-scene.sh --target templates_grid_crea

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
