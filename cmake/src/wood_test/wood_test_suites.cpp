#include "stdafx.h"
#include "wood_globals.h"
#include "wood_xml.h"
#include "wood_test.h"
#include "shapes.h"
#include "session.h"
#include "wood_test_runner.h"

namespace closest_points_joints { void run_benchmark(session_cpp::Session& session); }

namespace wood_test {

void run_suites() {

    // ─── shapes_cpp ───────────────────────────────────────────────────────────
    TEST_CPP("shapes_cpp", "annen_surfaces_load",
        "auto surfaces = session_cpp::annen_surfaces();\n"
        "CHECK(surfaces.size() > 0);",
        {
            auto surfaces = session_cpp::annen_surfaces();
            CHECK(surfaces.size() > 0);
        }
    );

    TEST_CPP("shapes_cpp", "chevron_mesh_basic",
        "auto surfaces = session_cpp::annen_surfaces();\n"
        "auto m = session_cpp::chevron_mesh(surfaces[0], 4, 900.0, 0.5, 0.05799);\n"
        "CHECK(m.vertex.size() > 0);\n"
        "CHECK(m.face.size() > 0);",
        {
            auto surfaces = session_cpp::annen_surfaces();
            if (!surfaces.empty()) {
                auto m = session_cpp::chevron_mesh(surfaces[0], 4, 900.0, 0.5, 0.05799);
                CHECK(m.vertex.size() > 0);
                CHECK(m.face.size() > 0);
            }
        }
    );

    // ─── folded_plates_cpp ────────────────────────────────────────────────────
    TEST_CPP("folded_plates_cpp", "barrel_vault_basic",
        "const double r = 100.0, s = r * 0.70710678;\n"
        "std::vector<session_cpp::Point> pts = { {-r,0,0},{-s,0,s},{0,0,r},{s,0,s},{r,0,0},\n"
        "    {-r,300,0},{-s,300,s},{0,300,r},{s,300,s},{r,300,0} };\n"
        "auto barrel = session_cpp::NurbsSurface::create(false,false,1,3,2,5,pts);\n"
        "session_cpp::FoldedPlates fp(barrel, 5, 2, 1.4, 6.0);\n"
        "CHECK(fp.mesh.face.size() > 0);\n"
        "CHECK(fp.mesh.vertex.size() > 0);",
        {
            const double r = 100.0, s = r * 0.70710678;
            std::vector<session_cpp::Point> pts = {
                {-r, 0, 0}, {-s, 0, s}, {0, 0, r}, {s, 0, s}, {r, 0, 0},
                {-r, 300, 0}, {-s, 300, s}, {0, 300, r}, {s, 300, s}, {r, 300, 0},
            };
            auto barrel = session_cpp::NurbsSurface::create(false, false, 1, 3, 2, 5, pts);
            session_cpp::FoldedPlates fp(barrel, 5, 2, 1.4, 6.0);
            CHECK(fp.mesh.face.size() > 0);
            CHECK(fp.mesh.vertex.size() > 0);
        }
    );

    TEST_CPP("folded_plates_cpp", "folded_plates_has_polylines",
        "// (same barrel vault construction)\n"
        "session_cpp::FoldedPlates fp(barrel, 5, 2, 1.4, 6.0);\n"
        "CHECK(!fp.polylines.empty());\n"
        "CHECK(!fp.insertion_lines.empty());",
        {
            const double r = 100.0, s = r * 0.70710678;
            std::vector<session_cpp::Point> pts = {
                {-r, 0, 0}, {-s, 0, s}, {0, 0, r}, {s, 0, s}, {r, 0, 0},
                {-r, 300, 0}, {-s, 300, s}, {0, 300, r}, {s, 300, s}, {r, 300, 0},
            };
            auto barrel = session_cpp::NurbsSurface::create(false, false, 1, 3, 2, 5, pts);
            session_cpp::FoldedPlates fp(barrel, 5, 2, 1.4, 6.0);
            CHECK(!fp.polylines.empty());
            CHECK(!fp.insertion_lines.empty());
        }
    );

    // ─── cross_connectors_cpp ─────────────────────────────────────────────────
    TEST_CPP("cross_connectors_cpp", "hexshell_connectors_smoke",
        "// Load hexshell XML polylines\n"
        "wood::xml::read_xml_polylines(flat_plines);\n"
        "// Build mesh, weld, unify winding\n"
        "auto mesh = session_cpp::Mesh::from_polylines(cc_polys, 0.01);\n"
        "mesh = mesh.weld(1.0);\n"
        "mesh.unify_winding();\n"
        "session_cpp::CrossConnectors cc(mesh, 2.0, {0.0}, 2, 100.0, 100.0, 2.0, 0.0);\n"
        "CHECK(cc.face_polylines.size() > 0);\n"
        "CHECK(cc.edge_polylines.size() > 0);",
        {
            auto saved_path = wood::xml::path_and_file_for_input_polylines;
            wood::xml::path_and_file_for_input_polylines =
                wood::GLOBALS::DATA_SET_INPUT_FOLDER
                + "type_plates_name_side_to_side_edge_inplane_hexshell.xml";

            std::vector<Polyline> flat_plines;
            wood::xml::read_xml_polylines(flat_plines);
            wood::xml::path_and_file_for_input_polylines = saved_path;

            std::vector<std::vector<session_cpp::Point>> cc_polys;
            for (size_t i = 0; i < flat_plines.size(); i += 2) {
                auto& face = flat_plines[i];
                std::vector<session_cpp::Point> poly;
                for (size_t j = 0; j < face.size(); j++) {
                    if (j == face.size() - 1 && face[j] == face[0]) break;
                    poly.push_back({face[j][0], face[j][1], face[j][2]});
                }
                if (poly.size() >= 3) cc_polys.push_back(poly);
            }

            CHECK(!cc_polys.empty());

            if (!cc_polys.empty()) {
                auto mesh = session_cpp::Mesh::from_polylines(cc_polys, 0.01);
                mesh = mesh.weld(1.0);
                mesh.unify_winding();
                session_cpp::CrossConnectors cc(mesh, 2.0, {0.0}, 2, 100.0, 100.0, 2.0, 0.0);
                CHECK(cc.face_polylines.size() > 0);
                CHECK(cc.edge_polylines.size() > 0);
            }
        }
    );

    // ─── closest_points_cpp ──────────────────────────────────────────────────
    TEST_CPP("closest_points_cpp", "benchmark_runs_without_crash",
        "session_cpp::Session session;\n"
        "closest_points_joints::run_benchmark(session);\n"
        "CHECK(true); // verifies no exception thrown",
        {
            session_cpp::Session session;
            closest_points_joints::run_benchmark(session);
            CHECK(true);
        }
    );

    TEST_CPP("closest_points_cpp", "benchmark_adds_geometry",
        "session_cpp::Session session;\n"
        "closest_points_joints::run_benchmark(session);\n"
        "// session should have points, polylines, or meshes after benchmark\n"
        "CHECK(session.objects.points->size() > 0\n"
        "   || session.objects.polylines->size() > 0\n"
        "   || session.objects.meshes->size() > 0);",
        {
            session_cpp::Session session;
            closest_points_joints::run_benchmark(session);
            bool has_geometry = session.objects.points->size() > 0
                             || session.objects.polylines->size() > 0
                             || session.objects.meshes->size() > 0;
            CHECK(has_geometry);
        }
    );

    // ─── wood_joints_cpp ─────────────────────────────────────────────────────
    TEST_CPP("wood_joints_cpp", "xml_roundtrip",
        "// Load hilti XML with pugixml reader\n"
        "wood::xml::read_xml_polylines(polylines);\n"
        "CHECK(polylines.size() == 24); // hilti dataset has 24 polylines\n"
        "// first point of first polyline: x=-200, y=900\n"
        "CHECK(std::abs(polylines[0][0].x() - (-200.0)) < 1e-6);\n"
        "CHECK(std::abs(polylines[0][0].y() - 900.0) < 1e-6);",
        {
            auto saved_path = wood::xml::path_and_file_for_input_polylines;
            wood::xml::path_and_file_for_input_polylines =
                wood::GLOBALS::DATA_SET_INPUT_FOLDER
                + "type_plates_name_side_to_side_edge_inplane_hilti.xml";

            std::vector<Polyline> polylines;
            wood::xml::read_xml_polylines(polylines);
            wood::xml::path_and_file_for_input_polylines = saved_path;

            CHECK(polylines.size() == 24);
            if (!polylines.empty() && !polylines[0].empty()) {
                CHECK(std::abs(polylines[0][0][0] - (-200.0)) < 1e-6);
                CHECK(std::abs(polylines[0][0][1] - 900.0) < 1e-6);
            }
        }
    );

    TEST_CPP("wood_joints_cpp", "type_plates_hilti_runs",
        "wood::GLOBALS::OUTPUT_GEOMETRY_TYPE = 3;\n"
        "wood::test::type_plates_name_side_to_side_edge_inplane_hilti();\n"
        "CHECK(true); // verifies no exception thrown",
        {
            wood::GLOBALS::OUTPUT_GEOMETRY_TYPE = 3;
            wood::test::type_plates_name_side_to_side_edge_inplane_hilti();
            CHECK(true);
        }
    );

} // run_suites

} // namespace wood_test
