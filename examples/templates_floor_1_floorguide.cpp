#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// A named polyline under a group.
void add(WoodSession& session, const Polyline& polyline, const std::string& name, const std::shared_ptr<TreeNode>& group) {

    std::shared_ptr<Polyline> copy = std::make_shared<Polyline>(polyline);
    copy->name = name;
    session.add_polyline(copy, group);
}

/// The quads of one family under a group.
void add(WoodSession& session, const std::vector<Polyline>& quads, const std::string& key, const std::shared_ptr<TreeNode>& group) {

    for (size_t i = 0; i < quads.size(); i++)
        add(session, quads[i].closed(), fmt::format("{}_{}", key, i), group);
}

/// The square floor.
int main() {

    const wood_floor::FloorPlan plan = wood_floor::FloorPlan::rectangle(3000.0, 3000.0);
    const wood_floor::Floor floor(plan, wood_floor::FloorSizes{});
    std::cout << floor.check().str() << std::endl;
    WoodSession session("templates_floor_1_floorguide");

    const wood_floor::QuarterGeometry& geometry = floor.geometry[0];
    const std::shared_ptr<TreeNode> plan_group = session.add_group("plan");
    add(session, Polyline(geometry.polygon).closed(), "quarter_polygon", plan_group);
    add(session, Polyline(floor.columns[0].head).closed(), "quarter_column_polygon", plan_group);

    for (const Point& point : floor.oculus_corners)
        session.add_point(std::make_shared<Point>(point), plan_group);

    const wood_floor::ConstructionQuads& quads = geometry.quads;
    const std::shared_ptr<TreeNode> quad_group = session.add_group("construction_quads");
    add(session, quads.outer_ribs, "outer_ribs", quad_group);
    add(session, quads.inner_beams, "inner_beams", quad_group);
    add(session, quads.inner_ribs, "inner_ribs", quad_group);
    add(session, quads.wedges, "wedges", quad_group);
    add(session, quads.t_sections, "t_sections", quad_group);

    const std::vector<std::array<Polyline, 3>>& parabolas = geometry.parabolas;
    const std::shared_ptr<TreeNode> parabola_group = session.add_group("parabolas");

    for (size_t i = 0; i < parabolas.size(); i++)
        for (size_t j = 0; j < 3; j++)
            add(session, parabolas[i][j], fmt::format("parabola_{}_offset_{}", i, j), parabola_group);

    session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 1 of the timber floor: the square bay Floor(FloorPlan::rectangle(3000, 3000), FloorSizes{}) and the construction of its quarter 0: the plan polygons (quarter, column head, oculus corners), the plan quad of every member at the floor datum and the four rib parabolas with their two t-section offsets. Prints the floor's report.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_1_floorguide --parallel 6 && ./build/templates_floor_1_floorguide && ../bash/publish-scene.sh --target templates_floor_1_floorguide

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
