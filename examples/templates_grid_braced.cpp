#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const double BAY = 6000.0;
const double STOREY = 3800.0;
const int BAYS = 2;
const int STOREYS = 2;
const wood_grid::Framing FRAMING{.span = -1, .node = 1, .profiles = {.column = profile_rectangle(300.0, 300.0), .girder = profile_rectangle(200.0, 500.0), .brace = profile_rectangle(150.0, 150.0)}};
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

int main() {

    std::vector<Line> lines;
    std::vector<Polyline> surfaces;
    for (int k = 0; k <= STOREYS; k++)
        for (int i = 0; i <= BAYS; i++)
            for (int j = 0; j <= BAYS; j++) {
                const Point node(i * BAY, j * BAY, k * STOREY);
                if (k > 0 && i < BAYS)
                    lines.push_back(Line::from_points(node, node + X * BAY));
                if (k > 0 && j < BAYS)
                    lines.push_back(Line::from_points(node, node + Y * BAY));
                if (k < STOREYS)
                    lines.push_back(Line::from_points(node, node + Vector(0.0, 0.0, STOREY)));
                if (k > 0 && i < BAYS && j < BAYS)
                    surfaces.push_back(Polyline::rectangle(node, X, Y, BAY, BAY));
                if (k < STOREYS && j < BAYS && (i == 0 || i == BAYS))
                    lines.push_back(Line::from_points(node, node + Y * BAY + Vector(0.0, 0.0, STOREY)));
            }

    WoodSession wood_session("templates_grid_braced");
    wood_grid::Building::from_lines(lines, surfaces).to_session(wood_session, FRAMING);

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(0);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Workflow C, line by line: columns, beams and diagonal braces drawn as 3D lines and the floors as quads, two by two bays over two storeys with a brace in every end bay; the horizontal lines make each level's plan, the vertical lines its column points, the floor quads its decks, the tilted lines its braces; every beam is cut by the column faces, every brace by the column faces, the deck top at its foot and the beam bottom at its head. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_braced --parallel 4 && ./build/templates_grid_braced && ../bash/publish-scene.sh --target templates_grid_braced

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
