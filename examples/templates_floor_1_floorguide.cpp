#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

const bool DUMP = true; // write every guide coordinate to data/output/pb/floor_1_floorguide.txt, the parity record against compas_tf

const wood_floor::FloorGuide GUIDE{
    .size_grid_x = 3000.0,
    .size_grid_y = 3000.0,
    .size_column_head = 220.0,
    .size_column_head_chamfer = 120.0,
    .size_outer_ribs = 100.0,
    .size_inner_ribs = 60.0,
    .size_inner_beams = 60.0,
    .size_wedge = 240.0,
    .height = 650.0,
    .rise = 453.0,
    .size_oculus = 1000.0,
};

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

/// The outline pairs of one member group, and the bed row where there is one.
void dump(std::ofstream& file, const std::string& group, const std::vector<wood_floor::Outline>& outlines) {

    for (size_t i = 0; i < outlines.size(); i++) {
        dump(file, fmt::format("{}/{}/top", group, i), outlines[i].top.get_points());
        dump(file, fmt::format("{}/{}/bottom", group, i), outlines[i].bottom.get_points());

        if (outlines[i].row >= 0)
            file << fmt::format("{}/{}/row {:.9f}\n", group, i, static_cast<double>(outlines[i].row));
    }
}

/// Every guide coordinate in the order compas_tf's dump writes them.
void dump_guide(const wood_floor::FloorGuide& guide, const std::string& path) {

    std::ofstream file(path);
    dump(file, "quarter_polygon", guide.quarter_polygon());
    dump(file, "quarter_column_polygon", guide.quarter_column_polygon());
    dump(file, "oculus_points", guide.oculus_points());

    const wood_floor::ConstructionPlanes cp = guide.construction_planes();
    dump(file, "outer_ribs", cp.outer_ribs);
    dump(file, "inner_beams", cp.inner_beams);
    dump(file, "inner_ribs", cp.inner_ribs);
    dump(file, "wedges", cp.wedges);
    dump(file, "t_sections", cp.t_sections);

    const wood_floor::ConstructionQuads quads = guide.construction_quads();
    dump(file, "outer_ribs", quads.outer_ribs);
    dump(file, "inner_beams", quads.inner_beams);
    dump(file, "inner_ribs", quads.inner_ribs);
    dump(file, "wedges", quads.wedges);
    dump(file, "t_sections", quads.t_sections);

    const std::vector<std::array<Polyline, 3>> parabolas = guide.boundary_parabolas();

    for (size_t i = 0; i < parabolas.size(); i++)
        for (size_t j = 0; j < 3; j++)
            dump(file, fmt::format("parabolas/{}/{}", i, j), parabolas[i][j].get_points());

    file << fmt::format("block_level_bottom {:.9f}\n", guide.block_level_bottom());
    file << fmt::format("block_level_top {:.9f}\n", guide.block_level_top());

    const std::vector<Plane> beds = guide.bed_top_planes();

    for (size_t i = 0; i < beds.size(); i++)
        dump(file, fmt::format("bed_top_planes/{}", i), beds[i]);

    dump(file, "outer_ribs", guide.outer_ribs());
    dump(file, "inner_ribs", guide.inner_ribs());
    dump(file, "inner_beams", guide.inner_beams());
    dump(file, "wedges_inner_beams", guide.wedges_inner_beams());
    dump(file, "tsections", guide.tsections());
    dump(file, "beds", guide.beds());
    dump(file, "oculus", guide.oculus());
    dump(file, "column_cutters", guide.column_cutters());
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

int main() {

    WoodSession session("templates_floor_1_floorguide");

    const std::shared_ptr<TreeNode> plan = session.add_group("plan");
    add(session, loop(GUIDE.quarter_polygon()), "quarter_polygon", plan);
    add(session, loop(GUIDE.quarter_column_polygon()), "quarter_column_polygon", plan);

    for (const Point& point : GUIDE.oculus_points())
        session.add_point(std::make_shared<Point>(point), plan);

    const wood_floor::ConstructionQuads quads = GUIDE.construction_quads();
    const std::shared_ptr<TreeNode> quad_group = session.add_group("construction_quads");
    add(session, quads.outer_ribs, "outer_ribs", quad_group);
    add(session, quads.inner_beams, "inner_beams", quad_group);
    add(session, quads.inner_ribs, "inner_ribs", quad_group);
    add(session, quads.wedges, "wedges", quad_group);
    add(session, quads.t_sections, "t_sections", quad_group);

    const std::vector<std::array<Polyline, 3>> parabolas = GUIDE.boundary_parabolas();
    const std::shared_ptr<TreeNode> parabola_group = session.add_group("parabolas");

    for (size_t i = 0; i < parabolas.size(); i++)
        for (size_t j = 0; j < 3; j++)
            add(session, parabolas[i][j], fmt::format("parabola_{}_offset_{}", i, j), parabola_group);

    session.pb_dump(pb_path("live"));

    if constexpr (DUMP)
        dump_guide(GUIDE, std::filesystem::path(pb_path("floor_1_floorguide")).replace_extension(".txt").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 1 of the timber floor, port of compas_tf example_model_1_floorguide: the FloorGuide of one quarter bay with the same parameters, its plan polygons (quarter, column head, oculus points), the plan quad of every member at the floor datum and the four rib parabolas with their two t-section offsets. DUMP writes every construction plane, quad, parabola and member outline to floor_1_floorguide.txt in the format of the compas_tf reference dump, the record the C++ port is compared against coordinate by coordinate.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_1_floorguide --parallel 6 && ./build/templates_floor_1_floorguide && ../bash/publish-scene.sh --target templates_floor_1_floorguide

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
