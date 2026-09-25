#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    WoodSession wood_session("elements");

    const std::shared_ptr<Plate> plate = Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 400, 300, Vector(0, 0, 40));
    const std::shared_ptr<Beam> beam = std::make_shared<Beam>(Polyline({Point(0, 0, 100), Point(800, 0, 100)}), 60.0);
    const std::shared_ptr<Column> column = std::make_shared<Column>(Line::from_points(Point(950, 50, 0), Point(950, 50, 600)), Polyline::rectangle(Point(900, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 100, 100));
    const std::shared_ptr<Block> block = std::make_shared<Block>(std::vector<Polyline>{Polyline::rectangle(Point(1200, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 200, 400), Polyline::rectangle(Point(1170, 0, 250), Vector(1, 0, 0), Vector(0, 1, 0), 260, 400)});

    wood_session.add(plate);
    wood_session.add(beam);
    wood_session.add(column);
    wood_session.add(block);

    std::cout << wood_session << "\n";

    std::shared_ptr<InteractionFeatureBeam> joint = std::make_shared<InteractionFeatureBeam>();
    joint->end_type = 1;
    joint->volumes = {beam->sections().back().translated(Vector(-60, 0, 0)), beam->sections().back(),
                      column->section.translated(Vector(0, 0, 100)), column->section.translated(Vector(0, 0, 160))};

    const std::shared_ptr<Interaction> contact = wood_session.add_interaction(beam, column, std::make_shared<InteractionContactAxis>(Line::from_points(Point(800, 0, 100), Point(950, 50, 100)), 1.0, 1.0 / 6.0, 0, 0, 0, 0));
    joint->contact_guid = contact->guid();
    wood_session.add_interaction(beam, column, joint);

    for (const std::shared_ptr<Interaction>& interaction : wood_session.get_interaction(column, beam))
        std::cout << *interaction << "\n";

    std::cout << "Beam-column interaction: " << wood_session.has_interaction(column, beam) << "\n";

    std::shared_ptr<InteractionContactFace> glue = std::make_shared<InteractionContactFace>(0, 4, ContactType::top_top, Polyline::rectangle(Point(1200, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 200, 300));
    glue->name = "glue";
    wood_session.add_interaction(plate, block, glue);
    wood_session.remove_interaction(block, plate);
    std::cout << "Plate-block interaction after removal: " << wood_session.has_interaction(plate, block) << "\n";

    for (const std::shared_ptr<Element>& element : wood_session.elements()) {
        std::cout << *element << "\n";
        std::cout << element->model_geometry_mesh() << "\n";
        std::cout << element->model_geometry_brep() << "\n\n";
    }

    std::cout << plate->element_geometry_mesh() << "\n";
    std::cout << plate->element_geometry_brep() << "\n\n";

    std::string path = pb_path("live");
    wood_session.pb_dump(path);
    std::cout << path << "\n";

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the four element kinds built in code and added to a wood session: a plate from a rectangle, a beam from an axis, a column from an axis and a section, a voussoir as a block lofted between two rectangles; every one a closed solid lofted on the first read of its geometry, element_geometry_brep() the same solid as faces, its outlines, axis and sections features the viewer draws while they are visible; a beam and a column joined by an axis contact and a beam joint that names it by guid, a plate and a block joined by a face contact named "glue" and then parted.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 1_elements --parallel 4 && ./build/1_elements && ../bash/publish-scene.sh --target 1_elements

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
