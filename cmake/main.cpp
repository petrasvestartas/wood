#include "stdafx.h"
#include "wood_test.h" // test
#include "shapes.h"
#include "step_reader.h"
#include "session.h"
namespace closest_points_joints { void run_benchmark(); }

int main(int argc, char **argv)
{

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GoogleTest
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// wood::test::run_all_tests();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Display
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	wood::GLOBALS::DISTANCE = 0.1;
	wood::GLOBALS::DISTANCE_SQUARED = 0.01;
	wood::GLOBALS::ANGLE = 0.11;
	wood::GLOBALS::OUTPUT_GEOMETRY_TYPE = 3;

	wood::GLOBALS::DATA_SET_INPUT_FOLDER = std::filesystem::current_path().parent_path().string() + "/src/wood/dataset/";
    ///wood::GLOBALS::DATA_SET_INPUT_FOLDER = std::filesystem::path(argv[0]).parent_path().string() + "/src/wood/dataset/";

	wood::GLOBALS::DATA_SET_OUTPUT_FILE = wood::GLOBALS::DATA_SET_INPUT_FOLDER + "out.xml";
	wood::GLOBALS::DATA_SET_OUTPUT_DATABASE = wood::GLOBALS::DATA_SET_INPUT_FOLDER + "out.db";

	wood::GLOBALS::DATA_SET_OUTPUT_DATABASE = std::filesystem::current_path().parent_path().parent_path().parent_path().string() +"/database_viewer/cmake/src/viewer/database/database_viewer.db";
	// wood::GLOBALS::DATA_SET_OUTPUT_DATABASE = std::filesystem::current_path().parent_path().parent_path().parent_path().parent_path().parent_path().string() +"/database_viewer/cmake/src/viewer/database/database_viewer.db";

	wood::GLOBALS::OUTPUT_GEOMETRY_TYPE = 3;
	wood::test::type_plates_name_side_to_side_edge_inplane_hilti();
	//wood::test::type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Session_cpp examples — serialize outputs to data/
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{
		using namespace session_cpp;
		std::string data_dir = std::filesystem::current_path().parent_path().string() + "/data/";
		std::filesystem::create_directories(data_dir);

		auto surfaces = annen_surfaces();
		std::cout << "[session] data_dir=" << data_dir << "  surfaces=" << surfaces.size() << std::endl;
		if (!surfaces.empty()) {
			Session session;

			// Chevron mesh — parameters match Python plugin
			auto chevron = chevron_mesh(surfaces[0], 4, 900.0, 0.5, 0.05799);
			session.add_mesh(std::make_shared<session_cpp::Mesh>(chevron));

			// Folded plates — barrel vault surface (semicircle r=100 in XZ extruded 300 in Y)
			{
				const double r = 100.0, s = r * 0.70710678;
				std::vector<session_cpp::Point> barrel_pts = {
					{-r, 0, 0}, {-s, 0, s}, {0, 0, r}, {s, 0, s}, {r, 0, 0},
					{-r, 300, 0}, {-s, 300, s}, {0, 300, r}, {s, 300, s}, {r, 300, 0},
				};
				auto barrel = session_cpp::NurbsSurface::create(false, false, 1, 3, 2, 5, barrel_pts);
				FoldedPlates fp(barrel, 5, 2, 1.4, 6.0);
				session.add_mesh(std::make_shared<session_cpp::Mesh>(fp.mesh));
			}

			// Cross connectors — annen corner element faces from hexshell XML
			{
				wood::xml::path_and_file_for_input_polylines = wood::GLOBALS::DATA_SET_INPUT_FOLDER
					+ "type_plates_name_side_to_side_edge_inplane_hexshell.xml";
				std::vector<CGAL_Polyline> flat_plines;
				wood::xml::read_xml_polylines(flat_plines);

				std::vector<std::vector<session_cpp::Point>> cc_polys;
				for (size_t i = 0; i < flat_plines.size(); i += 2) {
					auto& face = flat_plines[i];
					std::vector<session_cpp::Point> poly;
					for (size_t j = 0; j < face.size(); j++) {
						if (j == face.size() - 1 && face[j] == face[0]) break;
						poly.push_back({face[j].x(), face[j].y(), face[j].z()});
					}
					if (poly.size() >= 3) cc_polys.push_back(poly);
				}
				auto cc_base = session_cpp::Mesh::from_polylines(cc_polys, 0.01);
				cc_base = cc_base.weld(1.0);   // merge ~0.93-unit vertex cracks → creates shared edges between quads and hexes
				cc_base.unify_winding();       // BFS now reaches all faces via the new shared edges
				CrossConnectors cc(cc_base, 2.0, {0.0}, 2, 100.0, 100.0, 2.0, 0.0);

				// face plate outlines (one polyline per face × position)
				for (auto& face_pls : cc.face_polylines)
					for (auto& pl : face_pls)
						session.add_polyline(std::make_shared<session_cpp::Polyline>(pl));

				// edge connector rectangles
				for (auto& edge_pls : cc.edge_polylines)
					for (auto& pl : edge_pls)
						session.add_polyline(std::make_shared<session_cpp::Polyline>(pl));
			}

			// All Annen surfaces
			for (auto& srf : surfaces)
				session.add_nurbssurface(std::make_shared<session_cpp::NurbsSurface>(srf));

			session.pb_dump(data_dir + "session.pb");
		}

		// STEP read
		StepReader::read(data_dir + "annen.stp");

		// Closest-points joints benchmark — RTree vs AABBTree vs BVH
		closest_points_joints::run_benchmark();
	}

	return 0;
}
