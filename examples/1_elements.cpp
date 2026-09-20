#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    WoodSession scene("elements");

    const std::shared_ptr<Plate> plate = Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 400, 300, Vector(0, 0, 40));
    const std::shared_ptr<Beam> beam = std::make_shared<Beam>(Polyline({Point(0, 0, 100), Point(800, 0, 100)}), 60.0);
    const std::shared_ptr<Column> column = std::make_shared<Column>(Mesh::from_polylines({Polyline::rectangle(Point(900, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 100, 100)}), Line::from_points(Point(950, 50, 0), Point(950, 50, 600)), Polyline::rectangle(Point(900, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 100, 100));
    const std::shared_ptr<Block> block = std::make_shared<Block>(std::vector<Polyline>{Polyline::rectangle(Point(1200, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 200, 200), Polyline::rectangle(Point(1200, 0, 200), Vector(1, 0, 0), Vector(0, 1, 0), 200, 200)});

    scene.add(plate);
    scene.add(beam);
    scene.add(column);
    scene.add(block);

    std::cout << scene << "\n";
    std::cout << *plate << " thickness " << plate->thickness << " faces " << plate->polylines.size() << "\n";
    std::cout << *beam << " radius " << beam->radius(0) << "\n";
    std::cout << *scene.get_element<Column>(column->guid()) << "\n";
    std::cout << scene.plates().size() << " plates, " << scene.beams().size() << " beams, " << scene.blocks().size() << " blocks\n";

    scene.add_to_tree();
    scene.pb_dump(pb_path("live").string());

    return 0;
}

/*
description: the four element kinds built in code and added to a scene; a plate from a rectangle, a beam from an axis, a column from a solid, a block from loops; get_element and the typed lists.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target 1_elements --parallel 4 && ./build/1_elements
cloudflare: ../bash/publish-scene.sh --target 1_elements
view: https://petrasvestartas.github.io/session/
*/
