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

	return 0;
}
