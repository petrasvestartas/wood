#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The support element: a steel column base on a plane, and the column it carries.
int main() {

    WoodSession scene("element_support");

    const std::shared_ptr<Support> support = std::make_shared<Support>(Plane::xy_plane(), "support");
    const Point foot = support->column_foot();
    const std::shared_ptr<Column> column = Column::square(Line::from_points(foot, Point(foot[0], foot[1], 400.0)), Plane::from_point_normal(Point(-110.0, -110.0, 0.0), Vector(0.0, 0.0, 1.0)), 220.0);
    scene.add(support);
    scene.add(column);

    const std::shared_ptr<Joint> joint = Joint::support(*support, *column);
    scene.add(joint);
    scene.add_joint(joint);

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The support element: a steel column base on the xy plane with the manufacturer's dimensions, and a 220 square column 400 high on its head plate; the support joint lets the head plate into the column end and drills the three column screws, a subtract feature from the joint.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_support --parallel 6 && ./build/element_support && ../bash/publish-scene.sh --target element_support

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
