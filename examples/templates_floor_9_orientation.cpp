#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The square bay with its columns, every connector and the assembly pins.
int main() {

    const wood_floor::FloorGuide guide({
        Point(-3000.0, -3000.0, 0.0),
        Point(3000.0, -3000.0, 0.0),
        Point(3000.0, 3000.0, 0.0),
        Point(-3000.0, 3000.0, 0.0),
    });
    wood_floor::Floor floor(guide);

    const std::string column_name = "column_0";
    WoodSession fabrication_session = floor.get_branch(column_name);
    const std::shared_ptr<Column> column =
        fabrication_session.get_element_by_name<Column>(column_name);
    const std::shared_ptr<NodeSupportElement> support =
        fabrication_session.get_element_by_name<NodeSupportElement>("support_0");

    const Plane from = Plane(column->section.get_point(0), Vector::x_axis(), Vector::y_axis());
    const Plane on_ground = Plane(Point(0.0, 0.0, 0.0), Vector::y_axis(), Vector::z_axis());

    const Xform flat = Xform::plane_to_plane(from, on_ground);

    // the column carries its blocks, cuts and seat under its own node; the support stands beside it in the branch
    fabrication_session.set_xform(column->guid(), flat);
    fabrication_session.set_xform(support->guid(), flat);

    fabrication_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The fabrication orientation of one column of the square bay: the floor built from the guide, column_0 read back as its own branch with its glued blocks, support seat and head cuts, and turned by the transform from its upright section plane to the ground plane, so the column lies flat on the ground as it is machined; that branch alone is written.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_9_orientation --parallel 6 && ./build/templates_floor_9_orientation && ../bash/publish-scene.sh --target templates_floor_9_orientation

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
