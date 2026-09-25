#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const std::string INPUT = "crea_4x4"; // crea_4x4, crea_4x4_ground, crea_4x4_beams_without_column, crea_full: compas_grid's crea datasets as drawn lines and surfaces
const double GAP = 8000.0; // clear distance between the crea building and the braced frame
const double BAY = 6000.0;
const double STOREY = 3800.0;
const int BAYS = 2;
const int STOREYS = 2;
const wood_grid::Framing CREA{.system = 1, .span = -1, .node = 0, .deck = 200.0, .wall = 200.0, .head = 300.0, .reach = 360.0, .profiles = {.column = profile_rectangle(300.0, 300.0), .girder = profile_rectangle(300.0, 300.0)}};
const wood_grid::Framing BRACED{.span = -1, .node = 1, .profiles = {.column = profile_rectangle(300.0, 300.0), .girder = profile_rectangle(200.0, 500.0), .brace = profile_rectangle(150.0, 150.0)}};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

/// Every element of a building under one group.
void add_building(WoodSession& session, const wood_grid::Building& building, const wood_grid::Framing& framing, const std::string& name) {

    const std::shared_ptr<TreeNode> group = session.add_group(name);
    for (size_t storey = 0; storey + 1 < building.levels.size(); storey++)
        for (const std::shared_ptr<Element>& element : building.to_elements(framing, storey))
            session.add(element, group);
}

int main() {

    const Session input = Session::pb_load(config::dataset_path("crea/" + INPUT + "_input", ".pb").string());
    std::vector<Line> lines;
    std::vector<Polyline> surfaces;
    double right = -std::numeric_limits<double>::max();
    for (const std::vector<Polyline>& group : input.select_by_type<Polyline>())
        for (const Polyline& polyline : group) {
            lines.push_back(Line::from_points(polyline.get_points().front(), polyline.get_points().back()));
            right = std::max({right, lines.back().start()[0], lines.back().end()[0]});
        }
    for (const std::vector<Mesh>& group : input.select_by_type<Mesh>())
        for (const Mesh& mesh : group)
            for (const size_t face : mesh.faces()) {
                surfaces.push_back(*mesh.face_polygon(face));
                surfaces.back().name = mesh.name.find("Core") == std::string::npos ? "surface" : "core";
            }

    WoodSession wood_session("templates_grid_lines");
    add_building(wood_session, wood_grid::Building::from_lines(lines, surfaces), CREA, INPUT);

    std::vector<Line> frame;
    std::vector<Polyline> floors;
    for (int k = 0; k <= STOREYS; k++)
        for (int i = 0; i <= BAYS; i++)
            for (int j = 0; j <= BAYS; j++) {
                const Point node(right + GAP + i * BAY, j * BAY, k * STOREY);
                if (k > 0 && i < BAYS)
                    frame.push_back(Line::from_points(node, node + X * BAY));
                if (k > 0 && j < BAYS)
                    frame.push_back(Line::from_points(node, node + Y * BAY));
                if (k < STOREYS)
                    frame.push_back(Line::from_points(node, node + Vector(0.0, 0.0, STOREY)));
                if (k > 0 && i < BAYS && j < BAYS)
                    floors.push_back(Polyline::rectangle(node, X, Y, BAY, BAY));
                if (k < STOREYS && j < BAYS && (i == 0 || i == BAYS))
                    frame.push_back(Line::from_points(node, node + Y * BAY + Vector(0.0, 0.0, STOREY)));
            }
    add_building(wood_session, wood_grid::Building::from_lines(frame, floors), BRACED, "braced");

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(1);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Workflow C, line by line, two buildings side by side, one group each: the crea dataset compas_grid ships, read from data/crea/<INPUT>_input.pb, every column, beam, floor, facade and core quad as drawn, the vertical lines as column points, the horizontal lines and floor edges as the plan of their level, the vertical quads as the facade and core walls compas_grid drops, every line a beam ending on the head tops and its neighbours' sides; and a braced frame of two by two bays over two storeys drawn as lines and floor quads, every beam cut by the column faces and every brace by the column faces, the deck top at its foot and the beam bottom at its head. compute_contacts(1) pairs elements inside each building only. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_lines --parallel 4 && ./build/templates_grid_lines && ../bash/publish-scene.sh --target templates_grid_lines

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
