#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

const bool DUMP = true; // write every guide coordinate to data/output/pb/floor_1_floorguide.txt, the parity record against compas_tf

// ═══════════════════════════════════════════════════════════════════════════
// Dump
// ═══════════════════════════════════════════════════════════════════════════

/// One dump line: the name and every coordinate.
void dump(std::ofstream& file, const std::string& name, const std::vector<Point>& points) {

    file << name;

    for (const Point& point : points)
        file << fmt::format(" {:.9f} {:.9f} {:.9f}", point[0], point[1], point[2]);

    file << "\n";
}

/// One dump line for a plane: its origin and normal.
void dump(std::ofstream& file, const std::string& name, const Plane& plane) {

    const Vector& normal = plane.z_axis();
    dump(file, name, {plane.origin(), Point(normal[0], normal[1], normal[2])});
}

/// The plane pairs of one family.
void dump(std::ofstream& file, const std::string& key, const std::vector<std::array<Plane, 2>>& pairs) {

    for (size_t i = 0; i < pairs.size(); i++)
        for (size_t j = 0; j < 2; j++)
            dump(file, fmt::format("planes/{}/{}/{}", key, i, j), pairs[i][j]);
}

/// The quads of one family.
void dump(std::ofstream& file, const std::string& key, const std::vector<Polyline>& quads) {

    for (size_t i = 0; i < quads.size(); i++)
        dump(file, fmt::format("quads/{}/{}", key, i), quads[i].get_points());
}

/// The outline pairs of one member group.
void dump(std::ofstream& file, const std::string& group, const std::vector<wood_floor::Outline>& outlines) {

    for (size_t i = 0; i < outlines.size(); i++) {
        dump(file, fmt::format("{}/{}/top", group, i), outlines[i].top.get_points());
        dump(file, fmt::format("{}/{}/bottom", group, i), outlines[i].bottom.get_points());
    }
}

/// The outline pairs of the bed rows, numbered through, each with its row.
void dump(std::ofstream& file, const std::string& group, const std::vector<std::vector<wood_floor::Outline>>& rows) {

    size_t i = 0;

    for (size_t row = 0; row < rows.size(); row++)
        for (const wood_floor::Outline& outline : rows[row]) {
            dump(file, fmt::format("{}/{}/top", group, i), outline.top.get_points());
            dump(file, fmt::format("{}/{}/bottom", group, i), outline.bottom.get_points());
            file << fmt::format("{}/{}/row {:.9f}\n", group, i, static_cast<double>(row));
            i++;
        }
}

/// Every coordinate of quarter 0 in the order compas_tf's dump writes them.
void dump_guide(const wood_floor::Floor& floor, const std::string& path) {

    const wood_floor::Quarter quarter = floor.quarter(0);
    const wood_floor::QuarterGeometry& geometry = quarter.geometry();
    std::ofstream file(path);
    dump(file, "quarter_polygon", geometry.polygon);
    dump(file, "quarter_column_polygon", quarter.column().head);
    dump(file, "oculus_points", std::vector<Point>(floor.oculus_corners.begin(), floor.oculus_corners.end()));

    const wood_floor::ConstructionPlanes& cp = geometry.planes;
    dump(file, "outer_ribs", cp.outer_ribs);
    dump(file, "inner_beams", cp.inner_beams);
    dump(file, "inner_ribs", cp.inner_ribs);
    dump(file, "wedges", cp.wedges);
    dump(file, "t_sections", cp.t_sections);

    const wood_floor::ConstructionQuads& quads = geometry.quads;
    dump(file, "outer_ribs", quads.outer_ribs);
    dump(file, "inner_beams", quads.inner_beams);
    dump(file, "inner_ribs", quads.inner_ribs);
    dump(file, "wedges", quads.wedges);
    dump(file, "t_sections", quads.t_sections);

    const std::vector<std::array<Polyline, 3>>& parabolas = geometry.parabolas;

    for (size_t i = 0; i < parabolas.size(); i++)
        for (size_t j = 0; j < 3; j++)
            dump(file, fmt::format("parabolas/{}/{}", i, j), parabolas[i][j].get_points());

    file << fmt::format("block_level_bottom {:.9f}\n", geometry.block_level_bottom);
    file << fmt::format("block_level_top {:.9f}\n", geometry.block_level_top);

    const std::vector<Plane>& beds = geometry.bed_top_planes;

    for (size_t i = 0; i < beds.size(); i++)
        dump(file, fmt::format("bed_top_planes/{}", i), beds[i]);

    dump(file, "outer_ribs", quarter.outer_ribs());
    dump(file, "inner_ribs", quarter.inner_ribs());
    dump(file, "inner_beams", quarter.inner_beams());
    dump(file, "wedges_inner_beams", quarter.wedges_inner_beams());
    dump(file, "tsections", quarter.tsections());
    dump(file, "beds", quarter.beds());
    dump(file, "oculus", floor.oculus());
    dump(file, "column_cutters", quarter.column_cutters());
}

// ═══════════════════════════════════════════════════════════════════════════
// Scene
// ═══════════════════════════════════════════════════════════════════════════

/// The points as a closed polyline.
Polyline loop(std::vector<Point> points) {

    points.push_back(points.front());

    return Polyline(points);
}

/// A named polyline under a group.
void add(WoodSession& session, const Polyline& polyline, const std::string& name, const std::shared_ptr<TreeNode>& group) {

    std::shared_ptr<Polyline> copy = std::make_shared<Polyline>(polyline);
    copy->name = name;
    session.add_polyline(copy, group);
}

/// The quads of one family under a group.
void add(WoodSession& session, const std::vector<Polyline>& quads, const std::string& key, const std::shared_ptr<TreeNode>& group) {

    for (size_t i = 0; i < quads.size(); i++)
        add(session, loop(quads[i].get_points()), fmt::format("{}_{}", key, i), group);
}

/// The square floor with the model's definitions, or in compas_tf's parity mode with --compas.
int main(int argc, char** argv) {

    const wood_floor::FloorPlan plan = wood_floor::FloorPlan::rectangle(3000.0, 3000.0);
    const wood_floor::Floor floor = argc > 1 && std::string(argv[1]) == "--compas" ? wood_floor::Floor::compas_parity(plan, wood_floor::FloorSizes{}) : wood_floor::Floor(plan, wood_floor::FloorSizes{});
    std::cout << floor.check().str() << std::endl;
    WoodSession session("templates_floor_1_floorguide");

    const wood_floor::QuarterGeometry& geometry = floor.geometry[0];
    const std::shared_ptr<TreeNode> plan_group = session.add_group("plan");
    add(session, loop(geometry.polygon), "quarter_polygon", plan_group);
    add(session, loop(floor.columns[0].head), "quarter_column_polygon", plan_group);

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

    if constexpr (DUMP)
        dump_guide(floor, std::filesystem::path(pb_path("floor_1_floorguide")).replace_extension(".txt").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 1 of the timber floor, port of compas_tf example_model_1_floorguide: the square bay Floor(FloorPlan::rectangle(3000, 3000), FloorSizes{}) and quarter 0 of it, its plan polygons (quarter, column head, oculus corners), the plan quad of every member at the floor datum and the four rib parabolas with their two t-section offsets; prints the floor's report. DUMP writes every construction plane, quad, parabola and member outline of quarter 0 to floor_1_floorguide.txt in the format of the compas_tf reference dump: with --compas (Floor::compas_parity) it equals compas_tf's coordinate by coordinate, without it the model's central layers and cutter level move the central row and the cutters as data/reference/floor/README.md records.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_1_floorguide --parallel 6 && ./build/templates_floor_1_floorguide && ../bash/publish-scene.sh --target templates_floor_1_floorguide

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
