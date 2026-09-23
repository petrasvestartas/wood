#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    // Create session and elements
    WoodSession wood_session("elements");

    const std::shared_ptr<Plate> plate = Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 400, 300, Vector(0, 0, 40));
    const std::shared_ptr<Beam> beam = std::make_shared<Beam>(Polyline({Point(0, 0, 100), Point(800, 0, 100)}), 60.0);
    const std::shared_ptr<Column> column = std::make_shared<Column>(Line::from_points(Point(950, 50, 0), Point(950, 50, 600)), Polyline::rectangle(Point(900, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 100, 100));
    const std::shared_ptr<Block> block = std::make_shared<Block>(std::vector<Polyline>{Polyline::rectangle(Point(1200, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 200, 400), Polyline::rectangle(Point(1170, 0, 250), Vector(1, 0, 0), Vector(0, 1, 0), 260, 400)});

    wood_session.add(plate);
    wood_session.add(beam);
    wood_session.add(column);
    wood_session.add(block);

    // Every element lofts itself on the first read; nothing has to be synced by hand
    for (const std::shared_ptr<Element>& element : wood_session.elements())
        std::cout << element->str() << "\n";

    // // The same solid as a boundary representation, per element, cached beside the mesh
    // std::cout << std::get<BRep>(plate->element_geometry(false)).face_count() << " faces on the plate brep\n";

    // Serialize and push for the viewer: https://petrasvestartas.github.io/session/
    std::cout << wood_session;
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the four element kinds built in code and added to a wood session: a plate from a rectangle, a beam from an axis, a column from an axis and a section, a voussoir as a block lofted between two rectangles; every one a closed solid lofted on the first read of its geometry, element_geometry(false) the same solid as faces, its outlines, axis and sections features the viewer draws while they are visible.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 1_elements --parallel 4 && ./build/1_elements && ../bash/publish-scene.sh --target 1_elements

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
