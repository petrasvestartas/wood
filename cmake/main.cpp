#include "stdafx.h"
#include "wood_test.h" // test

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






	std::string filepath = "C:/Users/petras/Desktop/dev_wood/wood_log.ply";  // icosahedron_ascii
	std::vector <double> v;
	std::vector <int> f;
	tinyply::read(filepath, v, f, false);


	std::vector<CGAL_Polyline> output_polylines;
	// CGAL_Polyline output_polyline;
	// CGAL::Polyhedron_3<CK> output_mesh;
	// std::vector<double> output_distances;

	// Run Skeleton > equally space points > get distances > extend skeleton
	cgal::skeleton::mesh_skeleton(v, f, output_polylines);
	// cgal::skeleton::divide_polyline(output_polylines, 10, output_polyline);
	// cgal::skeleton::find_nearest_mesh_distances(output_mesh, output_polyline, 10, output_distances);
	// cgal::skeleton::extend_polyline_to_mesh(output_mesh, output_polyline, output_distances);

	std::cout << output_polylines.size() << std::endl;

	// for(auto output_polyline : output_polylines)
	// {
	// 	for(auto p : output_polyline)
	// 	{
	// 		std::cout << p << std::endl;
	// 	}
	// }
	// for (auto p : output_polyline)
	// {
	// 	std::cout << p << std::endl;
	// }

	// for (auto d : output_distances)
	// {
	// 	std::cout << d << std::endl;
	// }


	return 0;
}
