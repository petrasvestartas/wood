window.TEST_DATA = {
  "closest_points_cpp": [
    {
      "test_name": "benchmark_runs_without_crash",
      "passed": true,
      "time_ms": 0.7419,
      "language": "cpp",
      "code": "session_cpp::Session session;\nclosest_points_joints::run_benchmark(session);\nCHECK(true); // verifies no exception thrown",
      "checks": [{"line": 135, "code_line": "true", "passed": true}]
    },
    {
      "test_name": "benchmark_adds_geometry",
      "passed": true,
      "time_ms": 0.6469,
      "language": "cpp",
      "code": "session_cpp::Session session;\nclosest_points_joints::run_benchmark(session);\n// session should have points, polylines, or meshes after benchmark\nCHECK(session.objects.points->size() > 0\n   || session.objects.polylines->size() > 0\n   || session.objects.meshes->size() > 0);",
      "checks": [{"line": 152, "code_line": "has_geometry", "passed": true}]
    }
  ],
  "cross_connectors_cpp": [
    {
      "test_name": "hexshell_connectors_smoke",
      "passed": true,
      "time_ms": 6.5972,
      "language": "cpp",
      "code": "// Load hexshell XML polylines\nwood::xml::read_xml_polylines(flat_plines);\n// Build mesh, weld, unify winding\nauto mesh = session_cpp::Mesh::from_polylines(cc_polys, 0.01);\nmesh = mesh.weld(1.0);\nmesh.unify_winding();\nsession_cpp::CrossConnectors cc(mesh, 2.0, {0.0}, 2, 100.0, 100.0, 2.0, 0.0);\nCHECK(cc.face_polylines.size() > 0);\nCHECK(cc.edge_polylines.size() > 0);",
      "checks": [{"line": 123, "code_line": "!cc_polys.empty()", "passed": true}, {"line": 123, "code_line": "cc.face_polylines.size() > 0", "passed": true}, {"line": 123, "code_line": "cc.edge_polylines.size() > 0", "passed": true}]
    }
  ],
  "folded_plates_cpp": [
    {
      "test_name": "barrel_vault_basic",
      "passed": true,
      "time_ms": 3.743,
      "language": "cpp",
      "code": "const double r = 100.0, s = r * 0.70710678;\nstd::vector<session_cpp::Point> pts = { {-r,0,0},{-s,0,s},{0,0,r},{s,0,s},{r,0,0},\n    {-r,300,0},{-s,300,s},{0,300,r},{s,300,s},{r,300,0} };\nauto barrel = session_cpp::NurbsSurface::create(false,false,1,3,2,5,pts);\nsession_cpp::FoldedPlates fp(barrel, 5, 2, 1.4, 6.0);\nCHECK(fp.mesh.face.size() > 0);\nCHECK(fp.mesh.vertex.size() > 0);",
      "checks": [{"line": 60, "code_line": "fp.mesh.face.size() > 0", "passed": true}, {"line": 60, "code_line": "fp.mesh.vertex.size() > 0", "passed": true}]
    },
    {
      "test_name": "folded_plates_has_polylines",
      "passed": true,
      "time_ms": 3.5276,
      "language": "cpp",
      "code": "// (same barrel vault construction)\nsession_cpp::FoldedPlates fp(barrel, 5, 2, 1.4, 6.0);\nCHECK(!fp.polylines.empty());\nCHECK(!fp.insertion_lines.empty());",
      "checks": [{"line": 78, "code_line": "!fp.polylines.empty()", "passed": true}, {"line": 78, "code_line": "!fp.insertion_lines.empty()", "passed": true}]
    }
  ],
  "shapes_cpp": [
    {
      "test_name": "annen_surfaces_load",
      "passed": true,
      "time_ms": 25.6561,
      "language": "cpp",
      "code": "auto surfaces = session_cpp::annen_surfaces();\nCHECK(surfaces.size() > 0);",
      "checks": [{"line": 23, "code_line": "surfaces.size() > 0", "passed": true}]
    },
    {
      "test_name": "chevron_mesh_basic",
      "passed": true,
      "time_ms": 27.7872,
      "language": "cpp",
      "code": "auto surfaces = session_cpp::annen_surfaces();\nauto m = session_cpp::chevron_mesh(surfaces[0], 4, 900.0, 0.5, 0.05799);\nCHECK(m.vertex.size() > 0);\nCHECK(m.face.size() > 0);",
      "checks": [{"line": 38, "code_line": "m.vertex.size() > 0", "passed": true}, {"line": 38, "code_line": "m.face.size() > 0", "passed": true}]
    }
  ],
  "wood_joints_cpp": [
    {
      "test_name": "xml_roundtrip",
      "passed": true,
      "time_ms": 0.2225,
      "language": "cpp",
      "code": "// Load hilti XML with pugixml reader\nwood::xml::read_xml_polylines(polylines);\nCHECK(polylines.size() == 24); // hilti dataset has 24 polylines\n// first point of first polyline: x=-200, y=900\nCHECK(std::abs(polylines[0][0].x() - (-200.0)) < 1e-6);\nCHECK(std::abs(polylines[0][0].y() - 900.0) < 1e-6);",
      "checks": [{"line": 178, "code_line": "polylines.size() == 24", "passed": true}, {"line": 178, "code_line": "std::abs(polylines[0][0][0] - (-200.0)) < 1e-6", "passed": true}, {"line": 178, "code_line": "std::abs(polylines[0][0][1] - 900.0) < 1e-6", "passed": true}]
    },
    {
      "test_name": "type_plates_hilti_runs",
      "passed": true,
      "time_ms": 3.2848,
      "language": "cpp",
      "code": "wood::GLOBALS::OUTPUT_GEOMETRY_TYPE = 3;\nwood::test::type_plates_name_side_to_side_edge_inplane_hilti();\nCHECK(true); // verifies no exception thrown",
      "checks": [{"line": 189, "code_line": "true", "passed": true}]
    }
  ]
};
