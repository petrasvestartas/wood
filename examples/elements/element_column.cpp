#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The column element: from its axis and section, square with a notch, with a glued head, and the floor's carved column.
int main() {

    WoodSession scene("element_column");
    const double height = 3500.0;

    // 1. an axis and a closed section at its base
    const Polyline section = Polyline::rectangle(Point(-100.0, -150.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), 200.0, 300.0);
    scene.add(std::make_shared<Column>(Line::from_points(Point(0.0, 0.0, 0.0), Point(0.0, 0.0, height)), section, "section"));

    // 2. a square column, a block taking a notch out of it through a subtract feature
    const std::shared_ptr<Column> square = Column::square(Line::from_points(Point(1000.0, 0.0, 0.0), Point(1000.0, 0.0, height)), Plane::from_point_normal(Point(1000.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0)), 220.0, "square");
    const Polyline notch = Polyline::rectangle(Point(1100.0, -50.0, 2000.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), 200.0, 320.0);
    const std::shared_ptr<Block> cutter = std::make_shared<Block>(std::vector<Polyline>{notch, notch.translated(Vector(0.0, 0.0, 300.0))}, "notch");
    cutter->is_visible = false;
    scene.add(square);
    scene.add(cutter);
    scene.add_interaction(cutter, square, std::make_shared<InteractionFeatureSolid>(cutter->element_geometry_mesh(), SolidOperation::subtract));

    // 3. a session: the 220 shaft and two blocks glued on for a 340 head over its top 730, each added through an add feature
    scene.graft(wood_floor::ColumnSession::glued_head(Line::from_points(Point(2000.0, 0.0, 0.0), Point(2000.0, 0.0, height)), Plane::from_point_normal(Point(2000.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0)), 220.0, 340.0, 730.0, "glued_head"), nullptr);

    // 4. the floor's column at corner 0: the glued head on its support, six cutter plates taking away the faces the ribs and the column blocks bear on
    const wood_floor::FloorGuide guide({Point(3000.0, 0.0, 0.0), Point(9000.0, 0.0, 0.0), Point(9000.0, 6000.0, 0.0), Point(3000.0, 6000.0, 0.0)});
    scene.graft(wood_floor::ColumnSession(guide, 0), nullptr);

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The column element, four ways, 3500 high and 1000 apart along x. 1: an axis and a 200 x 300 section at its base. 2: Column::square, a 220 square from a corner frame, a hidden block taking a notch out of it through add_interaction(block, column, InteractionFeatureSolid subtract). 3: ColumnSession::glued_head, a session of the 220 shaft and two blocks glued on with add features, so the top 730 is a 340 square; the blocks are hidden, the column draws them as part of its solid. 4: the floor's column at corner 0 of a 6000 square bay: the glued head on its support, the support joint's seat and screws, and six hidden cutter plates of the floor guide each taking an inclined face away with a subtract feature, the faces the ribs and the column blocks bear on.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_column --parallel 6 && ./build/element_column && ../bash/publish-scene.sh --target element_column

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
