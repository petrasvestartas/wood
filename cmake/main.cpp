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
	wood::globals::DISTANCE = 0.1;
	wood::globals::DISTANCE_SQUARED = 0.01;
	wood::globals::ANGLE = 0.11;
	wood::globals::OUTPUT_GEOMETRY_TYPE = 3;

	wood::globals::DATA_SET_INPUT_FOLDER = std::filesystem::current_path().parent_path().string() + "/src/wood/dataset/";
	wood::globals::DATA_SET_OUTPUT_FILE = wood::globals::DATA_SET_INPUT_FOLDER + "out.xml";
	wood::globals::DATA_SET_OUTPUT_DATABASE = wood::globals::DATA_SET_INPUT_FOLDER + "out.db";

	wood::globals::DATA_SET_OUTPUT_DATABASE = std::filesystem::current_path().parent_path().parent_path().parent_path().string() +"/database_viewer/cmake/src/viewer/database/database_viewer.db";


	wood::globals::OUTPUT_GEOMETRY_TYPE = 3;
	wood::test::type_plates_name_side_to_side_edge_inplane_hilti();
	//wood::test::type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths();

	return 0;
}
