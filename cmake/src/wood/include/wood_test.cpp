
#include "wood_test.h"

#include "../../../stdafx.h" //go up to the folder where the CMakeLists.txt is
#include "database_writer.h"

namespace wood
{
namespace test
{

//! \cond NO_DOXYGEN
namespace internal
{
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// plate and beam - helper methods (display, etc.)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
set_file_path_for_input_xml_and_screenshot (const std::string &function_name)
{
    // input data-set
    wood::xml::path_and_file_for_input_polylines = wood::GLOBALS::DATA_SET_INPUT_FOLDER + function_name + ".xml";
}

void
set_file_path_for_input_xml_and_screenshot (std::vector<std::vector<IK::Point_3> > &input_polyline_pairs, const std::string &function_name, bool remove_duplicate_points)
{
    // input data-set h
    wood::xml::path_and_file_for_input_polylines = wood::GLOBALS::DATA_SET_INPUT_FOLDER + function_name + ".xml";

    // read the xml file
    wood::xml::read_xml_polylines (input_polyline_pairs, false, remove_duplicate_points);
}

void
set_file_path_for_input_xml_and_screenshot (std::vector<std::vector<IK::Point_3> > &input_polyline_pairs, std::vector<std::vector<IK::Vector_3> > &input_insertion_vectors,
                                            std::vector<std::vector<int> > &input_JOINTS_TYPES, std::vector<std::vector<int> > &input_three_valence_element_indices_and_instruction, std::vector<int> &input_adjacency,
                                            const std::string &function_name, bool remove_duplicate_points)
{
    // input data-set h
    wood::xml::path_and_file_for_input_polylines = wood::GLOBALS::DATA_SET_INPUT_FOLDER + function_name + ".xml";

    // read the xml file
    wood::xml::read_xml_polylines_and_properties (input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency, false, remove_duplicate_points);
}

void
set_plate_display (std::vector<std::vector<IK::Point_3> > &input_polyline_pairs, std::vector<std::vector<CGAL_Polyline> > &output_plines, bool add_loft)
{
    switch (wood::GLOBALS::OUTPUT_GEOMETRY_TYPE)
        {
        case (5):
            database_writer::LINE_THICKNESS = 2;
            database_writer::COLOR = "#E0E0E0";
            database_writer::add_mesh_boolean_difference (input_polyline_pairs,
                                                          output_plines); // grey
            database_writer::COLOR = "#000000";
            database_writer::add_polylines (input_polyline_pairs.begin (),
                                            input_polyline_pairs.end ()); // grey
            database_writer::add_polylines (output_plines.begin (),
                                            output_plines.end ()); // grey
            break;
        case (0):
            database_writer::LINE_THICKNESS = 2;
            database_writer::COLOR = "#000000";
            database_writer::add_polylines (input_polyline_pairs.begin (), input_polyline_pairs.end ());

            for (auto &plines : output_plines)
                for (auto &pline : plines)
                    {
                        database_writer::COLOR = "#000000";
                        std::vector<CGAL_Polyline> plines{ pline };
                        database_writer::add_polylines (plines.begin (), plines.end ());
                        database_writer::COLOR = "#E0E0E0";
                        database_writer::add_polygon_mesh (plines);
                    };

            break;
        case (2):
            database_writer::LINE_THICKNESS = 2;
            database_writer::COLOR = "#000000";
            database_writer::add_polylines (input_polyline_pairs.begin (),
                                            input_polyline_pairs.end ()); // grey

            for (auto &plines : output_plines)
                for (auto &pline : plines)
                    {
                        database_writer::COLOR = "#000000";
                        std::vector<CGAL_Polyline> plines{ pline };
                        database_writer::add_polylines (plines.begin (), plines.end ());
                        database_writer::COLOR = "#E0E0E0";
                        database_writer::add_polygon_mesh (plines);
                    };
            break;
        case (1):
        case (3):
            database_writer::LINE_THICKNESS = 2;
            database_writer::LINE_THICKNESS = 2;
            database_writer::COLOR = "#000000";
            database_writer::add_polylines (input_polyline_pairs.begin (), input_polyline_pairs.end ());
            database_writer::COLOR = "#0767ff";
            database_writer::add_polylines (output_plines.begin (), output_plines.end ());
            break;
        case (4):
            database_writer::LINE_THICKNESS = 2;
            database_writer::LINE_THICKNESS = 2;
            database_writer::COLOR = "#000000";
            database_writer::add_polylines (output_plines.begin (), output_plines.end ());
            database_writer::COLOR = "#0767ff";
            database_writer::add_loft (output_plines);
            break;
        }
    database_writer::write_to_database ();
}

} // namespace internal
//! \endcond

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// methods for the demonstration of a single joint
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
joint_demonstration ()
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create two simple elements
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // std::vector<std::vector<WPoint>> input_polyline_pairs;

    // /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // // The filename of the xml file and the screenshot directory
    // /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // std::vector<std::vector<IK::Point_3>> input_polyline_pairs;
    // internal::set_file_path_for_input_xml_and_screenshot(input_polyline_pairs,
    // "joint_demonstration");

    // // Global Parameters and output joint selection and orientation
    // double division_length = 300;
    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 1 + 0] = 450;
    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 1 + 1] = 0.64;
    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 1 + 2] = 10;
    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 2 + 0] = 450;
    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 2 + 1] = 0.5;
    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 2 + 2] = 20;

    // bool compute_joints = true;
    // int search_type = 0;
    // std::vector<double> scale = {1, 1, 1};
    // std::vector<std::vector<IK::Vector_3>> input_insertion_vectors;
    // std::vector<std::vector<int>> input_JOINTS_TYPES;
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction;
    // std::vector<int> input_adjacency;

    // /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // // Main Method of Wood
    // /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // // output
    // std::vector<std::vector<CGAL_Polyline>> output_plines;
    // std::vector<std::vector<wood::cut::cut_type>> output_types;
    // std::vector<std::vector<int>> top_face_triangulation;

    // wood::main::get_connection_zones(
    //     // input
    //     input_polyline_pairs,
    //     input_insertion_vectors,
    //     input_JOINTS_TYPES,
    //     input_three_valence_element_indices_and_instruction,
    //     input_adjacency,
    //     // output
    //     output_plines,
    //     output_types,
    //     top_face_triangulation,
    //     // Global Parameters
    //     wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES,
    //     scale,
    //     search_type,
    //     wood::GLOBALS::OUTPUT_GEOMETRY_TYPE);

    // /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // // Export
    // /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // wood::xml::path_and_file_for_output_polylines =
    // wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    // wood::xml::write_xml_polylines_and_types(output_plines,
    // output_types);

    // /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // // Render
    // /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // internal::set_plate_display(input_polyline_pairs, output_plines);
    // return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// plate methods
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// side-to-side in-plane and out-of-plane
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool
type_plates_name_hexbox_and_corner ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_hexbox_and_corner");

    // Global Parameters and output joint selection and orientation
    double division_length = 300;

    wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 1 + 0] = 450;
    wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 1 + 1] = 0.64;
    wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 1 + 2] = 10;
    wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 2 + 0] = 450;
    wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 2 + 1] = 0.5;
    wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 2 + 2] = 20;

    bool compute_joints = true;
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;
    std::vector<std::vector<int> > input_JOINTS_TYPES;
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction;
    std::vector<int> input_adjacency;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // output
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    // std::cout << __func__ << "\n";
    wood::main::get_connection_zones (

        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,

        // output
        output_plines, output_types, top_face_triangulation,

        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Render         // std::cout << "wood_test.cpp ->"<< __func__ <<
    // "\n";
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_joint_linking_vidychapel_corner ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_joint_linking_vidychapel_corner");

    wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 150; // division_length
    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -10;

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{
        { -1, -1, -1, -1, 15, -1 }, { -1, -1, 15, -1, -1, -1 }, { -1, -1, -1, -1, 15, -1 }, { -1, -1, 15, -1, -1, -1 }, { -1, -1, -1, -1, 15, -1 }, { -1, -1, 15, -1, -1, -1 }, { -1, -1, -1, -1, 15, -1 },
        { -1, -1, -1, 15, -1, -1 }, { -1, -1, -1, -1, -1, 15 }, { -1, -1, -1, 15, -1, -1 }, { -1, -1, -1, -1, 15, -1 }, { -1, -1, -1, 15, -1, -1 }, { -1, -1, -1, -1, -1, 15 }, { -1, -1, -1, 15, -1, -1 },
        { -1, -1, -1, -1, 15, -1 }, { -1, -1, 15, -1, -1, -1 }, { -1, -1, -1, -1, -1, 15 }, { -1, -1, -1, 15, -1, -1 }, { -1, -1, -1, -1, 16, -1 }, { -1, -1, 16, -1, -1, -1 }, { -1, -1, -1, -1, -1, 16 },
        { -1, -1, -1, 16, -1, -1 }, { -1, -1, -1, -1, 16, -1 }, { -1, -1, 16, -1, -1, -1 }, { -1, -1, -1, -1, -1, 16 }, { -1, -1, -1, 16, -1, -1 }, { -1, -1, -1, -1, 16, -1 }, { -1, -1, 16, -1, -1, -1 },
        { -1, -1, -1, -1, -1, 16 }, { -1, -1, -1, 16, -1, -1 }, { -1, -1, -1, -1, 15, -1 }, { -1, -1, 15, -1, -1, -1 }, { -1, -1, -1, -1, -1, 15 }, { -1, -1, -1, 15, -1, -1 }, { -1, -1, -1, -1, 15, -1 },
        { -1, -1, 15, -1, -1, -1 }, { -1, -1, -1, -1, -1, 15 }, { -1, -1, -1, 15, -1, -1 }, { -1, -1, -1, -1, 15, -1 }, { -1, -1, 15, -1, -1, -1 }, { -1, -1, -1, -1, -1, 15 }, { -1, -1, -1, 15, -1, -1 }

    };
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {
        { 1 },
        // column1 - joint 15
        { 6, 7, 9, 8 },
        { 10, 11, 13, 12 },
        { 14, 15, 17, 16 },
        // column2 - joint 16
        { 18, 19, 21, 20 },
        { 22, 23, 25, 24 },
        { 26, 27, 29, 28 },
        // column3 - joint one direction
        { 30, 31, 32, 32 }, // 33,32
        { 34, 35, 36, 36 }, // 37,36
        { 38, 39, 40, 40 }, // 41,40

    };

    std::vector<int> input_adjacency = {
        // column 0
        0,
        1,
        -1,
        -1,
        2,
        3,
        -1,
        -1,
        4,
        5,
        -1,
        -1,
        // column 1
        6,
        7,
        -1,
        -1,
        8,
        9,
        -1,
        -1,
        10,
        11,
        -1,
        -1,
        12,
        13,
        -1,
        -1,
        14,
        15,
        -1,
        -1,
        16,
        17,
        -1,
        -1,
        // column 2
        18,
        19,
        -1,
        -1,
        20,
        21,
        -1,
        -1,
        22,
        23,
        -1,
        -1,
        24,
        25,
        -1,
        -1,
        26,
        27,
        -1,
        -1,
        28,
        29,
        -1,
        -1,
        // column 3
        30,
        31,
        -1,
        -1,
        32,
        33,
        -1,
        -1,
        34,
        35,
        -1,
        -1,
        36,
        37,
        -1,
        -1,
        38,
        39,
        -1,
        -1,
        40,
        41,
        -1,
        -1,

    };
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_joint_linking_vidychapel_one_layer ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_joint_linking_vidychapel_one_layer", true);

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -15;
    wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 50; // division_length
    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10; //
    // division_length

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    // xml_parser::write_xml_polylines(output_plines);

    std::vector<std::vector<CGAL_Polyline> > input_polyline_pairs_2;
    for (int i = 0; i < input_polyline_pairs.size (); i += 2)
        input_polyline_pairs_2.emplace_back (std::vector<CGAL_Polyline>{ input_polyline_pairs[i + 0], input_polyline_pairs[i + 1] });
    // wood::xml::write_xml_polylines_and_types(input_polyline_pairs_2,
    // output_types);
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_joint_linking_vidychapel_one_axis_two_layers ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_joint_linking_vidychapel_one_axis_two_layers");

    // wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -10;
    wood::GLOBALS::JOINT_VOLUME_EXTENSION = { 0, 0, -200, 0, 0, -200, 0, 0, -20, 0, 0, -20 };
    std::vector<double> JOINTS_TYPES = wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES;
    JOINTS_TYPES[1 * 3 + 0] = 50;
    int search_type = 0;
    int output_type = wood::GLOBALS::OUTPUT_GEOMETRY_TYPE; // 0 - Plate outlines 1 - joint lines 2 -
                                                           // joint volumes 3 - joint geometry 4 - merge
    // std::cout << "\n output_type " << output_type << "\n";
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{ { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }

    };
    std::vector<std::vector<int> > input_JOINTS_TYPES{ { -1, -1, -1, 16, -1, -1, -1 },
                                                       { -1, -1, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 16, -1, -1, -1, 16, -1 },
                                                       { -1, -1, 16, -1, -1, -1, 16, -1 },
                                                       { -1, -1, -1, -1, 16, -1, -1 },
                                                       { -1, -1, -1, -1, 16, -1, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 }

    };
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {
        { 1 },
        { 9, 15, 140, 8 },  // long edge
        { 16, 10, 11, 17 }, // long edge
        { 6, 12, 13, 7 },   // long edge
        { 15, 17, 16, 14 }, // wall
        { 9, 11, 10, 8 },   // wall
        { 15, 13, 12, 14 }, // wall
        { 9, 7, 6, 8 },     // wall
        { 3, 5, 4, 2 },     // wall
        { 3, 1, 0, 2 }      // wall
    };

    std::vector<int> input_adjacency = { // long edge
                                         9, 15, -1, -1, 14, 8, -1, -1,
                                         // long edge
                                         16, 10, -1, -1, 11, 17, -1, -1,
                                         // long edge
                                         6, 12, -1, -1, 13, 7, -1, -1,
                                         // wall
                                         15, 17, -1, -1, 14, 16, -1, -1,
                                         // wall
                                         9, 11, -1, -1, 8, 10, -1, -1,
                                         // wall
                                         15, 13, -1, -1, 14, 12, -1, -1,
                                         // wall
                                         9, 7, -1, -1, 8, 6, -1, -1,
                                         // wall
                                         3, 5, -1, -1, 2, 4, -1, -1,
                                         // wall
                                         3, 1, -1, -1, 2, 0, -1, -1

    };
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        JOINTS_TYPES, scale, search_type, output_type, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    // xml_parser::write_xml_polylines(output_plines);
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_joint_linking_vidychapel_full ()
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_joint_linking_vidychapel_full");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -10;
    wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 50;
    int search_type = 0;
    // std::cout << "\n output_type " << output_type << "\n";
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{ { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
                                                                     { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 133.378166, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } }

    };
    std::vector<std::vector<int> > input_JOINTS_TYPES{ { -1, -1, -1, 16, -1, -1, -1 },
                                                       { -1, -1, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 16, -1, -1, -1, 16, -1 },
                                                       { -1, -1, 16, -1, -1, -1, 16, -1 },
                                                       { -1, -1, -1, -1, 16, -1, -1 },
                                                       { -1, -1, -1, -1, 16, -1, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, -1, 15, -1, 16, -1, -1, -1, 16 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, 15, -1, 16, -1, -1, -1 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, -1, 16, -1, 15, -1, 16 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, 15, -1 },
                                                       { -1, -1, -1, -1, 16, -1, -1 },
                                                       { -1, -1, -1, -1, 16, -1, -1 },
                                                       { -1, -1, 16, -1, -1, -1, 16, -1 },
                                                       { -1, -1, 16, -1, -1, -1, 16, -1 },
                                                       { -1, -1, -1, 16, -1, -1, -1 },
                                                       { -1, -1, -1, 16, -1, -1, -1 }

    };
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = { { 1 },
                                                                                           { 3, 1, 0, 2 },
                                                                                           { 3, 5, 4, 2 },
                                                                                           { 9, 7, 6, 8 },
                                                                                           { 9, 11, 10, 8 },
                                                                                           { 15, 13, 12, 14 },
                                                                                           { 15, 17, 16, 14 },
                                                                                           { 21, 19, 18, 20 },
                                                                                           { 21, 23, 22, 20 },
                                                                                           { 27, 25, 24, 26 },
                                                                                           { 27, 29, 28, 26 },
                                                                                           { 33, 31, 30, 32 },
                                                                                           { 33, 35, 34, 32 },
                                                                                           { 39, 37, 36, 38 },
                                                                                           { 39, 41, 40, 38 },
                                                                                           { 45, 43, 42, 44 },
                                                                                           { 45, 47, 46, 44 },
                                                                                           { 51, 49, 48, 50 },
                                                                                           { 51, 53, 52, 50 },
                                                                                           { 57, 55, 54, 56 },
                                                                                           { 57, 59, 58, 56 },
                                                                                           { 63, 61, 60, 62 },
                                                                                           { 63, 65, 64, 62 },
                                                                                           { 69, 67, 66, 68 },
                                                                                           { 69, 71, 70, 68 },
                                                                                           { 75, 73, 72, 74 },
                                                                                           { 75, 77, 76, 74 },
                                                                                           { 81, 79, 78, 80 },
                                                                                           { 81, 83, 82, 80 },
                                                                                           { 6, 12, 13, 7 },
                                                                                           { 18, 24, 25, 19 },
                                                                                           { 30, 36, 37, 31 },
                                                                                           { 42, 48, 49, 43 },
                                                                                           { 54, 60, 61, 55 },
                                                                                           { 66, 72, 73, 67 },
                                                                                           { 9, 15, 14, 8 },
                                                                                           { 21, 27, 26, 20 },
                                                                                           { 33, 39, 38, 32 },
                                                                                           { 45, 51, 50, 44 },
                                                                                           { 57, 63, 62, 56 },
                                                                                           { 69, 75, 74, 68 },
                                                                                           { 10, 16, 17, 11 },
                                                                                           { 22, 28, 29, 23 },
                                                                                           { 34, 40, 41, 35 },
                                                                                           { 46, 52, 53, 47 },
                                                                                           { 58, 64, 65, 59 },
                                                                                           { 70, 76, 77, 71 }

    };

    std::vector<int> input_adjacency
        = { 3,  1,  -1, -1, 2,  0,  -1, -1, 3,  5,  -1, -1, 2,  4,  -1, -1, 9,  7,  -1, -1, 8,  6,  -1, -1, 9,  11, -1, -1, 8,  10, -1, -1, 15, 13, -1, -1, 14, 12, -1, -1, 15, 17, -1, -1, 14, 16,
            -1, -1, 21, 19, -1, -1, 20, 18, -1, -1, 21, 23, -1, -1, 20, 22, -1, -1, 27, 25, -1, -1, 26, 24, -1, -1, 27, 29, -1, -1, 26, 28, -1, -1, 33, 31, -1, -1, 32, 30, -1, -1, 33, 35, -1, -1,
            32, 34, -1, -1, 39, 37, -1, -1, 38, 36, -1, -1, 39, 41, -1, -1, 38, 40, -1, -1, 45, 43, -1, -1, 44, 42, -1, -1, 45, 47, -1, -1, 44, 46, -1, -1, 51, 49, -1, -1, 50, 48, -1, -1, 51, 53,
            -1, -1, 50, 52, -1, -1, 57, 55, -1, -1, 56, 54, -1, -1, 57, 59, -1, -1, 56, 58, -1, -1, 63, 61, -1, -1, 62, 60, -1, -1, 63, 65, -1, -1, 62, 64, -1, -1, 69, 67, -1, -1, 68, 66, -1, -1,
            69, 71, -1, -1, 68, 70, -1, -1, 75, 73, -1, -1, 74, 72, -1, -1, 75, 77, -1, -1, 74, 76, -1, -1, 81, 79, -1, -1, 80, 78, -1, -1, 81, 83, -1, -1, 80, 82, -1, -1, 6,  12, -1, -1, 7,  13,
            -1, -1, 18, 24, -1, -1, 19, 25, -1, -1, 30, 36, -1, -1, 31, 37, -1, -1, 42, 48, -1, -1, 43, 49, -1, -1, 54, 60, -1, -1, 55, 61, -1, -1, 66, 72, -1, -1, 67, 73, -1, -1, 9,  15, -1, -1,
            8,  14, -1, -1, 21, 27, -1, -1, 20, 26, -1, -1, 33, 39, -1, -1, 32, 38, -1, -1, 45, 51, -1, -1, 44, 50, -1, -1, 57, 63, -1, -1, 56, 62, -1, -1, 69, 75, -1, -1, 68, 74, -1, -1, 10, 16,
            -1, -1, 11, 17, -1, -1, 22, 28, -1, -1, 23, 29, -1, -1, 34, 40, -1, -1, 35, 41, -1, -1, 46, 52, -1, -1, 47, 53, -1, -1, 58, 64, -1, -1, 59, 65, -1, -1, 70, 76, -1, -1, 71, 77, -1, -1 };
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    // xml_parser::write_xml_polylines(output_plines);
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_inplane_2_butterflies ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_inplane_2_butterflies");

    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 0] = 150; //
    // division_length wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 +
    // 2] = 3; // joint type wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] =
    // -10;

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES;
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction;

    std::vector<int> input_adjacency;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines, true);
    return true;
}

bool
type_plates_name_side_to_side_edge_inplane_hexshell ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_inplane_hexshell");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[0] = -20; // reduce the thickness of the joint
    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -10;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 0] = 40;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 1;
        }

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_inplane_differentdirections ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_inplane_differentdirections");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[0] = -10;
    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -75;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 0] = 40;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 1;
        }

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors = {
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -0.801031428, -0.598622294, 0 },
            { -0.683040714, -0.7303803, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -0.801031428, -0.598622294, 0 },
            { 0, 0, 0 },
            { -0.801031428, -0.598622294, 0 },
            { -0.978287523, -0.207252315, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -0.683040714, -0.7303803, 0 },
            { -0.683040714, -0.7303803, 0 },
            { -1, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -0.683040714, -0.7303803, 0 },
            { -0.978287523, -0.207252315, 0 },
            { -0.978287523, -0.207252315, 0 },
            { -1, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -1, 0, 0 },
            { 0, -1, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, -1, 0 },
            { -1, 0, 0 },
            { 0, -1, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -0.801031428, -0.598622294, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -0.873977409, 0.48596655, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -0.978287523, -0.207252315, 0 },
            { -0.873977409, 0.48596655, 0 },
            { 0, 0, 0 },
            { -1, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, -1, 0 },
            { -1, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
    };

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_outofplane_folding ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_outofplane_folding");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -20;
    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 12; // this
    // property is assigned to the individual joint.division_length
    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 1] = 0.95; //
    // this property is assignes to the individual joint.shift parameter

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors = {
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0.934259036, 9.956262354 },
            { 0, 0, 0 },
            { 0, 0.934259036, 9.956262354 },
            { 0, 0, 0 },
            { 0, 0.934259036, 9.956262354 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 7.071067812, 7.071067812 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0.934259036, 9.956262354 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
            { 0, 0.934259036, 9.956262354 },
            { 0, 0, 0 },
            { 0, 7.071067812, 7.071067812 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 7.071067812, 7.071067812 },
            { 0, 0, 0 },
            { 0, 7.071067812, 7.071067812 },
            { 0, 0, 0 },
            { 0, 7.071067812, 7.071067812 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 7.071067812, -7.071067812 },
            { 0, 0, 0 },
            { 0, 7.071067812, 7.071067812 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0.934259036, -9.956262354 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
            { 0, 7.071067812, -7.071067812 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 7.071067812, -7.071067812 },
            { 0, 0, 0 },
            { 0, 7.071067812, -7.071067812 },
            { 0, 0, 0 },
            { 0, 7.071067812, -7.071067812 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0.934259036, -9.956262354 },
            { 0, 0, 0 },
            { 0, 0.934259036, -9.956262354 },
            { 0, 0, 0 },
            { 0, 0.934259036, -9.956262354 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 7.071067812, -7.071067812 },
            { 0, 0, 0 },
            { 0, 0.934259036, -9.956262354 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0.934259036, 9.956262354 },
            { 0, 0, 0 },
            { 0, 0.934259036, 9.956262354 },
            { 0, 0, 0 },
            { 0, 0.934259036, 9.956262354 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 7.071067812, 7.071067812 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0.934259036, 9.956262354 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
            { 0, 0.934259036, 9.956262354 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 7.071067812, 7.071067812 },
            { 0, 0, 0 },
            { 0, 7.071067812, 7.071067812 },
            { 0, 0, 0 },
            { 0, 7.071067812, 7.071067812 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 7.071067812, -7.071067812 },
            { 0, 0, 0 },
            { 0, 7.071067812, 7.071067812 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0.934259036, -9.956262354 },
            { 0, 0, 0 },
            { 0, 10, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 7.071067812, -7.071067812 },
            { 0, 0, 0 },
            { 0, 7.071067812, -7.071067812 },
            { 0, 0, 0 },
            { 0, 7.071067812, -7.071067812 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0.934259036, -9.956262354 },
            { 0, 0, 0 },
            { 0, 0.934259036, -9.956262354 },
            { 0, 0, 0 },
            { 0, 0.934259036, -9.956262354 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 7.071067812, -7.071067812 },
            { 0, 0, 0 },
            { 0, 0.934259036, -9.956262354 },
            { 0, 0, 0 },
        },
    };

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_outofplane_box ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_outofplane_box");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -100;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 17;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };

    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_outofplane_tetra ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_outofplane_tetra");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -100;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] = 0.5;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 17;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };

    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_outofplane_dodecahedron ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_outofplane_dodecahedron");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -250;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 1000;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] = 0.5;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 14;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };

    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_outofplane_icosahedron ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_outofplane_icosahedron");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -250;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 1000;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] = 0.5;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 14;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };

    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_outofplane_octahedron ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_outofplane_octahedron");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -250;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 1000;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] = 0.5;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 14;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };

    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_inplane_"
                                                                                "outofplane_simple_corners");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -20;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 2;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 12;
        }

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_combined ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_inplane_"
                                                                                "outofplane_simple_corners_combined");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -20;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 2;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 12;
        }

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_inplane_outofplane_simple_"
                                                                                "corners_different_lengths");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -20;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 0] = 140;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 1] = 0.5;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 1;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 140;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] = 0.5;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10;
        }

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_inplane_hilti ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_inplane_hilti");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[5 * 3 + 0] = 500;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[5 * 3 + 1] = 1.0;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[5 * 3 + 2] = 55;
        }

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;
    wood::GLOBALS::FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED = true;
    wood::GLOBALS::FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE = true;

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// top-to-top data sets
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool
type_plates_name_top_to_top_pairs ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_top_to_top_pairs");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -10;
    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 10;   //
    // this property is assigned to the individual joint.division_length
    // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 1] = 0.95; //
    // this property is assignes to the individual joint.shift parameter

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_"
                                                                                "top_hexboxes");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -50;
    wood::GLOBALS::DISTANCE_SQUARED *= 100;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 0] = 140;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 1] = 0.5;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 1;

            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 140;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] = 0.5;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10;

            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 21;

            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 100;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 1] = 150;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 2] = 43;
        }

    std::cout << "WARNING THERE IS A BUG IN VIEWER FOR THIS CASE\n";

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_hex_block_rossiniere ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_hex_block_rossiniere");

    wood::GLOBALS::FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED = true;

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -20;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 0] = 140;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 1] = 0.5;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 1;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 140;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] = 0.5;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10;
        }

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// top-to-side data sets
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool
type_plates_name_top_to_side_snap_fit ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_top_to_side_pairs");

    // wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -10;
    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0] = 300; // this property is assigned to the individual
                                                                         // joint.division_length
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 1] = 0.5; // this property is assignes to the individual
                                                                         // joint.shift parameter
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
        }

    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_top_to_side_box ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_top_to_side_box");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0] = 300; // this property is assigned to the individual
                                                                         // joint.division_length
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 1] = 0.5; // this property is assignes to the individual
                                                                         // joint.shift parameter
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_top_to_side_corners ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_top_to_side_corners");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] = 0.66;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0] = 300; // this property is assigned to the individual
                                                                         // joint.division_length
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 1] = 0.5; // this property is assignes to the individual
                                                                         // joint.shift parameter
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 20;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_top_to_side_and_side_to_side_outofplane_annen_corner ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_top_to_side_and_side_to_"
                                                                                "side_outofplane_annen_corner");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0] = 300;
            // // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 20;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors = {
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 330.476008093, 0, -129.014338371 },
            { 355.671709183, 0, -138.850473602 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 86.308934016, 0, -86.308934016 },
            { 87.845014076, 0, -87.845014076 },
            { 0, 0, 0 },
        },
    };

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}
bool
type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_top_to_side_and_side_to_"
                                                                                "side_outofplane_annen_box");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 200;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0] = 300;
            // // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 20;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors = {
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -5.835731695, 19.397964143, 13.677616475 },
            { -5.835731695, 19.397964143, 13.677616475 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -5.835731695, 19.397964143, 13.677616475 },
            { -5.835731695, 19.397964143, 13.677616475 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
    };

    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}
bool
type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box_pair ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_top_to_side_and_side_to_"
                                                                                "side_outofplane_annen_box_pair");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 200;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0] = 300;
            // // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 20;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors = {
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -5.835731695, 19.397964143, 13.677616475 },
            { -5.835731695, 19.397964143, 13.677616475 },
            { 5.835731695, -19.397964143, -13.677616475 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { -5.835731695, 19.397964143, 13.677616475 },
            { -5.835731695, 19.397964143, 13.677616475 },
            { 5.835731695, -19.397964143, -13.677616475 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 27.603174796, 25.122241877, -9.293505458 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 27.603174796, 25.122241877, -9.293505458 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 27.603174796, 25.122241877, -9.293505458 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 27.603174796, 25.122241877, -9.293505458 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
        {
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
            { 0, 0, 0 },
        },
    };

    std::vector<std::vector<int> > input_JOINTS_TYPES = {
        {
            { -1 },
            { -1 },
            { 20 },
            { 20 },
            { 20 },
            { -1 },
        },
        {
            { -1 },
            { -1 },
            { 20 },
            { 20 },
            { 20 },
            { -1 },
        },
        {
            { -1 },
            { -1 },
            { -1 },
            { -1 },
            { 10 },
            { -1 },
        },
        {
            { -1 },
            { -1 },
            { 10 },
            { -1 },
            { 0 },
            { -1 },
        },
        {
            { -1 },
            { -1 },
            { 20 },
            { -1 },
            { -1 },
            { 20 },
        },
        {
            { -1 },
            { -1 },
            { 20 },
            { -1 },
            { -1 },
            { 20 },
        },
        {
            { -1 },
            { -1 },
            { -1 },
            { -1 },
            { 10 },
            { -1 },
        },
        {
            { -1 },
            { -1 },
            { 10 },
            { -1 },
            { -1 },
            { -1 },
        },
    };

    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction; // = {{1}, {16,
    // 10, 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {
        { 0 }, // first index indicates that the corner case of 3
               // valence joints belongs to the Annen joint type
        {
            0,
            7, // same - top plate
            4,
            7, // same - bottom plate
        },
        {
            1,
            7, // same - top plate
            5,
            7, // same - bottom plate
        },
    };

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}
bool
type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_small ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;
    std::vector<std::vector<int> > input_JOINTS_TYPES;
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction; // = {{1},
                                                                                        // {16, 10,
                                                                                        // 11, 17}};
    std::vector<int> input_adjacency;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
                                                          "type_plates_name_top_to_side_and_side_to_side_outofplane_annen_"
                                                          "grid_small",
                                                          false);

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 200;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0] = 300;
            // // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 20;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}
bool
type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_full_arch ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;
    std::vector<std::vector<int> > input_JOINTS_TYPES;
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction; // = {{1},
                                                                                        // {16, 10,
                                                                                        // 11, 17}};
    std::vector<int> input_adjacency;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
                                                          "type_plates_name_top_to_side_and_side_to_side_outofplane_annen_"
                                                          "grid_full_arch",
                                                          false);

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 200;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0] = 300;
            // // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 20;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_vda_floor_0 ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;
    std::vector<std::vector<int> > input_JOINTS_TYPES;
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction; // = {{1},
                                                                                        // {16, 10,
                                                                                        // 11, 17}};
    std::vector<int> input_adjacency;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
                                                          "type_plates_name_vda_floor_0", false);

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 8000;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] = 0.66;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 100;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 1] = 45;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 2] = 43;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_vda_floor_2 ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int a = 0;
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;
    std::vector<std::vector<int> > input_JOINTS_TYPES;
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction; // = {{1},
                                                                                        // {16, 10,
                                                                                        // 11, 17}};
    std::vector<int> input_adjacency;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
                                                          "type_plates_name_vda_floor_2", false);

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 200;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] = 0.66;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 10;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0] = 50; // this property is assigned to the individual
                                                                        // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 1] = 0.5;
            // // this property is assignes to the individual joint.shift
            // parameter
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 21;
        }
    int search_type = 0;
    std::vector<double> scale = { 1, 1, 1 };
    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -10;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cross data sets
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool
type_plates_name_cross_and_sides_corner ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_cross_and_sides_corner");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 3;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] = 0.66;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2] = 12;
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0] = 500; // this property is assigned to the individual
                                                                         // joint.division_length
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 1] = 0.5; // this property is assignes to the individual
                                                                         // joint.shift parameter
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 400;
        }
    int search_type = 2;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_cross_corners ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_cross_corners");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 3;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2]
            // = 12; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0]
            // = 500; // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 400;
        }
    int search_type = 1;
    wood::GLOBALS::JOINT_VOLUME_EXTENSION[1] = 2;
    std::vector<double> scale = { 1, 1.00, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_cross_vda_corner ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_cross_vda_corner");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 3;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2]
            // = 12; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0]
            // = 500; // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 400;
        }
    int search_type = 1;
    wood::GLOBALS::JOINT_VOLUME_EXTENSION[1] = 2;
    std::vector<double> scale = { 1, 1.00, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_cross_vda_hexshell ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_cross_vda_hexshell");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 3;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2]
            // = 12; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0]
            // = 500; // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 400;
        }
    int search_type = 1;
    std::vector<double> scale = { 1, 1.00, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_cross_vda_hexshell_reciprocal ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_cross_vda_hexshell_reciprocal");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 3;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2]
            // = 12; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0]
            // = 500; // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 400;
        }
    int search_type = 1;
    std::vector<double> scale = { 1, 1.00, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_cross_vda_single_arch ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_cross_vda_single_arch");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 3;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2]
            // = 12; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0]
            // = 500; // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 400;
        }
    int search_type = 1;
    std::vector<double> scale = { 1, 1.00, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_cross_vda_shell ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_cross_vda_shell");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 3;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2]
            // = 12; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0]
            // = 500; // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 400;
        }
    int search_type = 1;
    std::vector<double> scale = { 1, 1.00, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_cross_square_reciprocal_two_sides ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_cross_square_reciprocal_two_sides");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 3;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2]
            // = 12; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0]
            // = 500; // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 400;
        }
    int search_type = 1;
    std::vector<double> scale = { 1, 1.00, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_cross_square_reciprocal_iseya ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_cross_square_reciprocal_iseya");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 3;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2]
            // = 12; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0]
            // = 500; // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 400;
        }
    int search_type = 1;
    std::vector<double> scale = { 1, 1.00, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_cross_ibois_pavilion ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_cross_ibois_pavilion");

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 3;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2]
            // = 12; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0]
            // = 500; // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 400;
        }

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = -20;

    int search_type = 2;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

bool
type_plates_name_cross_brussels_sports_tower ()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_plates_name_cross_brussels_sports_tower");
    printf ("Number of polylines: %zu \n", input_polyline_pairs.size ());

    if (!wood::GLOBALS::RUN_COUNT) // this is needed to avoid overwriting
                                   // after the first "Run" click by the
                                   // user -> IMGUI
        {
            wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[3 * 3 + 2] = 35;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[0 * 3 + 2] = 3;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 1] =
            // 0.66; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 2]
            // = 12; wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 0]
            // = 500; // this property is assigned to the individual
            // joint.division_length
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3
            // + 1] = 0.5; // this property is assignes to the individual
            // joint.shift parameter
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[2 * 3 + 2] = 25;
            // wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[4 * 3 + 0] = 400;
        }

    wood::GLOBALS::OUTPUT_GEOMETRY_TYPE = 3;
    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = 0;

    int search_type = 1;
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;
    std::vector<std::vector<int> > top_face_triangulation;

    wood::main::get_connection_zones (
        // input
        input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, input_three_valence_element_indices_and_instruction, input_adjacency,
        // output
        output_plines, output_types, top_face_triangulation,
        // Global Parameters
        wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);

    for (const auto &innerVec : output_types)
        {
            for (const auto &value : innerVec)
                {
                    std::printf ("%d ", value);
                }
            std::printf ("\n");
        }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Display
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_plate_display (input_polyline_pairs, output_plines);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// beam methods
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool
type_beams_name_phanomema_node ()
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_axes;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_axes, "type_beams_name_phanomema_node");

    wood::GLOBALS::JOINT_VOLUME_EXTENSION[2] = 0;
    wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES[1 * 3 + 0] = 150; // division_length
    int search_type = 0;
    int output_type = wood::GLOBALS::OUTPUT_GEOMETRY_TYPE; // 0 - Plate outlines 1 - joint lines 2 -
                                                           // joint volumes 3 - joint geometry 4 - merge
    // std::cout << "\n output_type " << output_type << "\n";
    std::vector<double> scale = { 1, 1, 1 };
    std::vector<std::vector<IK::Vector_3> > input_insertion_vectors{};
    std::vector<std::vector<int> > input_JOINTS_TYPES{};
    // std::vector<std::vector<int>>
    // input_three_valence_element_indices_and_instruction = {{1}, {16, 10,
    // 11, 17}};
    std::vector<std::vector<int> > input_three_valence_element_indices_and_instruction = {};

    std::vector<int> input_adjacency = {};
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Main Method of Wood
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<double> > polylines_segment_radii;
    std::vector<std::vector<IK::Vector_3> > polylines_segment_direction;
    for (int i = 0; i < input_polyline_axes.size (); i++)
        {
            std::vector<double> polyline_segment_radii;
            // std::vector<IK::Vector_3> polyline_segment_direction;
            for (int j = 0; j < input_polyline_axes[i].size (); j++)
                {
                    polyline_segment_radii.emplace_back (150);
                    // polyline_segment_direction.emplace_back(0.037013,
                    // 0.909575, 0.413887);
                }
            polylines_segment_radii.emplace_back (polyline_segment_radii);
            // polylines_segment_direction.emplace_back(polyline_segment_direction);
        }

    std::vector<int> allowed_types{ 1 };

    double min_distance = 20;
    double volume_length = 500;
    double cross_or_side_to_end = 0.91;
    int flip_male = 1;

    // output of wood::joint areas
    std::vector<std::vector<int> > polyline0_id_segment0_id_polyline1_id_segment1_id;
    std::vector<std::vector<int> > index_polylines;
    std::vector<std::vector<int> > index_polylines_segment;
    std::vector<std::vector<double> > distance;
    std::vector<std::vector<IK::Point_3> > point_pairs;
    std::vector<std::vector<CGAL_Polyline> > volume_pairs;
    std::vector<CGAL_Polyline> joints_areas;
    std::vector<int> joints_types;

    // Global Parameters and output wood::joint selection and orientation
    std::vector<std::vector<CGAL_Polyline> > output_plines;
    std::vector<std::vector<wood::cut::cut_type> > output_types;

    bool compute_joints = true;                     // WARNING values are in unknown state
    double division_distance = 150;                 // WARNING values are in unknown state
    double shift = 0.5;                             // WARNING values are in unknown state
    bool use_eccentricities_to_scale_joints = true; // WARNING values are in unknown state

    wood::main::beam_volumes (input_polyline_axes, polylines_segment_radii, polylines_segment_direction, allowed_types, min_distance, volume_length, cross_or_side_to_end, flip_male,
                              // output of wood::joint areas
                              index_polylines, index_polylines_segment, distance, point_pairs, volume_pairs, joints_areas, joints_types,
                              // Global Parameters and output wood::joint selection and
                              // orientation
                              output_plines, output_types, compute_joints, division_distance, shift, wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, use_eccentricities_to_scale_joints);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::xml::path_and_file_for_output_polylines = wood::GLOBALS::DATA_SET_OUTPUT_FILE;
    output_plines.clear ();
    for (auto &pline : volume_pairs)
        for (int j = 0; j < 4; j++)
            output_plines.emplace_back (std::vector<CGAL_Polyline>{ pline[j] });

    wood::xml::write_xml_polylines_and_types (output_plines, output_types);

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// library of joints
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool
type_library_name_ss_e_ip_2 ()
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_library_name_ss_e_op_2");

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // joint parameters
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    wood::joint joint;

    // name
    joint.name = "ss_e_ip_2";

    // parameters that comes from the joint
    bool default_values = true;
    double edge_length = !default_values ? std::sqrt (CGAL::squared_distance (joint.joint_lines[0][0], joint.joint_lines[0][1])) : 1000;
    int divisions = !default_values ? joint.divisions : 5;
    double joint_volume_edge_length = !default_values ? std::sqrt (CGAL::squared_distance (joint.joint_volumes[0][1], joint.joint_volumes[0][2])) : 40;

    // scale down the edge, since wood_joint ->  bool
    // joint::orient_to_connection_area() make the distance between joint
    // volumes equal to 2nd joint volume edge
    edge_length *= joint.scale[2];

    // normalization to the unit space, joint_volume_edge_length is used
    // for parametrization
    double move_length_scaled = edge_length / (divisions * joint_volume_edge_length);
    double total_length_scaled = edge_length / joint_volume_edge_length;

    // movement vectors to translate the unit joint to the end of the edge
    // and then to its middle
    IK::Vector_3 dir (0, 0, 1);
    IK::Vector_3 move_from_center_to_the_end = dir * ((total_length_scaled * 0.5) - (move_length_scaled * 0.5));
    IK::Vector_3 move_length_dir = -dir * move_length_scaled;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Male default shape
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<CGAL_Polyline> male_0 = { {

                                              IK::Point_3 (0, -0.5, 0.1166666667),
                                              IK::Point_3 (-0.5, -0.5, 0.4),
                                              IK::Point_3 (-0.5, -0.5, -0.4),
                                              IK::Point_3 (0, -0.5, -0.1166666667),
                                          },
                                          {

                                              IK::Point_3 (0, -0.5, 0.1166666667),
                                              IK::Point_3 (0, -0.5, -0.1166666667),
                                          } };

    std::vector<CGAL_Polyline> male_1 = { {

                                              IK::Point_3 (0, 0.5, 0.1166666667),
                                              IK::Point_3 (-0.5, 0.5, 0.4),
                                              IK::Point_3 (-0.5, 0.5, -0.4),
                                              IK::Point_3 (0, 0.5, -0.1166666667),
                                          },

                                          {

                                              IK::Point_3 (0, 0.5, 0.1166666667),
                                              IK::Point_3 (0, 0.5, -0.1166666667),
                                          } };

    std::vector<wood::cut::cut_type> male_types{ wood::cut::edge_insertion, wood::cut::edge_insertion };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // female default shape
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<CGAL_Polyline> female_0 = { {

                                                IK::Point_3 (0, -0.5, 0.1166666667),
                                                IK::Point_3 (0.5, -0.5, 0.4),
                                                IK::Point_3 (0.5, -0.5, -0.4),
                                                IK::Point_3 (0, -0.5, -0.1166666667),
                                            },
                                            {

                                                IK::Point_3 (0, -0.5, 0.1166666667),
                                                IK::Point_3 (0, -0.5, -0.1166666667),
                                            } };

    std::vector<CGAL_Polyline> female_1 = { {

                                                IK::Point_3 (0, 0.5, 0.1166666667),
                                                IK::Point_3 (0.5, 0.5, 0.4),
                                                IK::Point_3 (0.5, 0.5, -0.4),
                                                IK::Point_3 (0, 0.5, -0.1166666667),
                                            },
                                            {

                                                IK::Point_3 (0, 0.5, 0.1166666667),
                                                IK::Point_3 (0, 0.5, -0.1166666667),
                                            } };

    std::vector<wood::cut::cut_type> female_types{ wood::cut::edge_insertion, wood::cut::edge_insertion };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Reserve memory for multiple copies
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    joint.m[0].reserve (2);
    joint.m[1].reserve (2);
    joint.m_boolean_type.reserve (2);
    joint.f[0].reserve (2);
    joint.f[1].reserve (2);
    joint.f_boolean_type.reserve (2);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Copy the default shapes and move them
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    auto a = joint.m[0];
    joint.m[0].emplace_back (CGAL_Polyline ());
    joint.m[1].emplace_back (CGAL_Polyline ());
    joint.f[0].emplace_back (CGAL_Polyline ());
    joint.f[1].emplace_back (CGAL_Polyline ());

    joint.m[0].back ().reserve (male_0[0].size () * divisions);
    joint.m[1].back ().reserve (male_1[0].size () * divisions);
    joint.f[0].back ().reserve (female_0[0].size () * divisions);
    joint.f[1].back ().reserve (female_1[0].size () * divisions);

    for (auto i = 0; i < divisions; i++)
        {
            // copy the first outline, be sure that the point order is
            // correct, so that the non-internsecting polyline can be
            // created, else reverse it
            CGAL_Polyline male_moved_0 = male_0[0];
            CGAL_Polyline male_moved_1 = male_1[0];
            CGAL_Polyline female_moved_0 = female_0[0];
            CGAL_Polyline female_moved_1 = female_1[0];

            // move joints that are positioned at the center to the end of
            // the segment and then back by half of the division length
            for (auto &p : male_moved_0)
                p += move_from_center_to_the_end + move_length_dir * i;

            for (auto &p : male_moved_1)
                p += move_from_center_to_the_end + move_length_dir * i;

            for (auto &p : female_moved_0)
                p += move_from_center_to_the_end + move_length_dir * i;

            for (auto &p : female_moved_1)
                p += move_from_center_to_the_end + move_length_dir * i;

            // merge with the main outline
            joint.m[0].back ().insert (joint.m[0].back ().end (), male_moved_0.begin (), male_moved_0.end ());
            joint.m[1].back ().insert (joint.m[1].back ().end (), male_moved_1.begin (), male_moved_1.end ());
            joint.f[0].back ().insert (joint.f[0].back ().end (), female_moved_0.begin (), female_moved_0.end ());
            joint.f[1].back ().insert (joint.f[1].back ().end (), female_moved_1.begin (), female_moved_1.end ());
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Add the insertion lines
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    joint.m[0].emplace_back (CGAL_Polyline{ joint.m[0].front ().front (), joint.m[0].front ().back () });
    joint.m[1].emplace_back (CGAL_Polyline{ joint.m[1].front ().front (), joint.m[1].front ().back () });
    joint.f[0].emplace_back (CGAL_Polyline{ joint.f[0].front ().front (), joint.f[0].front ().back () });
    joint.f[1].emplace_back (CGAL_Polyline{ joint.f[1].front ().front (), joint.f[1].front ().back () });

    joint.f_boolean_type = female_types;
    joint.m_boolean_type = male_types;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // is unit scale
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    joint.unit_scale = true;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // joint for preview
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    database_writer::SCALE = 1.0;
    std::vector<std::vector<CGAL_Polyline> > input_polyline_pairs0;
    input_polyline_pairs0.emplace_back (joint.m[0]);
    input_polyline_pairs0.emplace_back (joint.m[1]);
    database_writer::add_polylines (input_polyline_pairs0.begin (),
                                    input_polyline_pairs0.end ()); // grey
    std::vector<std::vector<CGAL_Polyline> > input_polyline_pairs1;
    input_polyline_pairs1.emplace_back (joint.f[0]);
    input_polyline_pairs1.emplace_back (joint.f[1]);
    CGAL_Polyline default_segment = { IK::Point_3 (0, 0, -total_length_scaled * 0.5), IK::Point_3 (0, 0, total_length_scaled * 0.5) };
    input_polyline_pairs1.push_back ({ default_segment });
    database_writer::add_polylines (input_polyline_pairs1.begin (),
                                    input_polyline_pairs1.end ()); // grey
    // reset
    database_writer::SCALE = 1000;
    return true;
}

bool
type_library_name_ss_e_op_4 ()
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot (input_polyline_pairs, "type_library_name_ss_e_op_4");

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // joint parameters
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    wood::joint joint;
    joint.name = "ss_e_op_4";
    int number_of_tenons = 4;
    std::array<double, 2> x = { -1.5, 0.5 };
    // std::array<double, 2> x_chamfer = {-2.0, 0.5};
    std::array<double, 2> y = { -0.50, 0.50 };

    // double z_extension = 0.01;
    std::array<double, 2> z_ext = { -0.5, 0.5 };
    number_of_tenons = std::min (50, std::max (2, number_of_tenons)) * 2;
    double step = 1 / ((double)number_of_tenons - 1);
    std::array<double, 2> z = { z_ext[0] + step, z_ext[1] - step };
    number_of_tenons -= 2;
    step = 1 / ((double)number_of_tenons - 1);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Male
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    double t = 0.5;
    for (int j = 0; j < 2; j++)
        {
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // memory and variables
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            joint.m[j].resize (2);
            joint.m[j][0].reserve (4 + number_of_tenons * 2);
            int sign = j == 0 ? -1 : 1;

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // tenon interpolation
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            joint.m[j][0].emplace_back (sign * x[1], y[j], z_ext[1]);
            joint.m[j][0].emplace_back (x[1] + j * 0.01, y[j], z_ext[1]);

            for (int i = 0; i < number_of_tenons; i++)
                {
                    double z_ = z[1] + (z[0] - z[1]) * step * i;

                    double z_chamfer = i % 2 == 0 ? z_ + (z[0] - z[1]) * step * ((t * 0.5) + ((1 - t) * 0.2)) : z_ - (z[0] - z[1]) * step * ((t * 0.5) + ((1 - t) * 0.2)); // chamfer
                    z_ = i % 2 == 0 ? z_ + (z[0] - z[1]) * step * t * 0.5 : z_ - (z[0] - z[1]) * step * t * 0.5;                                                           // for scaling down
                                                                                                                                                                           // the tenon

                    if (i % 2 == 1) // chamfer
                        joint.m[j][0].emplace_back (-0.1 + x[0], y[j],
                                                    z_chamfer); // chamfer

                    joint.m[j][0].emplace_back (x[(i + 1) % 2], y[j], z_);
                    joint.m[j][0].emplace_back (x[(i + 0) % 2], y[j], z_);

                    if (i % 2 == 0) // chamfer
                        joint.m[j][0].emplace_back (-0.1 + x[0], y[j],
                                                    z_chamfer); // chamfer
                }

            joint.m[j][0].emplace_back (x[1] + j * 0.01, y[j], z_ext[0]);
            joint.m[j][0].emplace_back (sign * x[1], y[j], z_ext[0]);

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // cut outlines
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            joint.m[j][1] = {
                IK::Point_3 (sign * x[1], y[j], z_ext[1]),
                IK::Point_3 (sign * x[1], y[j], z_ext[0]),
            };
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Female
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int j = 0; j < 2; j++)
        {
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // memory and variables
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            joint.f[j].resize (2 + number_of_tenons);
            int sign = j == 0 ? 1 : -1;
            int j_inv = j == 0 ? 1 : 0;
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // main outlines
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            joint.f[j][0] = {
                IK::Point_3 (y[j_inv], sign * y[1], z_ext[1]),
                IK::Point_3 (y[j_inv], 3 * y[0], z_ext[1]),
                IK::Point_3 (y[j_inv], 3 * y[0], z_ext[0]),
                IK::Point_3 (y[j_inv], sign * y[1], z_ext[0]),
            };

            joint.f[j][1] = {
                IK::Point_3 (y[j_inv], sign * y[1], z_ext[1]),
                IK::Point_3 (y[j_inv], sign * y[1], z_ext[1]),
            };

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // holes
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            for (int i = 0; i < number_of_tenons; i += 2)
                {
                    double z_ = z[1] + (z[0] - z[1]) * step * i;
                    z_ = i % 2 == 0 ? z_ + (z[0] - z[1]) * step * t * 0.5 : z_ - (z[0] - z[1]) * step * t * 0.5; // for scaling down
                                                                                                                 // the tenon

                    joint.f[j][2 + i].reserve (5);
                    joint.f[j][2 + i + 1].reserve (5);
                    joint.f[j][2 + i].emplace_back (y[j_inv], y[0], z_);
                    joint.f[j][2 + i].emplace_back (y[j_inv], y[1], z_);

                    z_ = z[1] + (z[0] - z[1]) * step * (i + 1);
                    z_ = i % 2 == 1 ? z_ + (z[0] - z[1]) * step * t * 0.5 : z_ - (z[0] - z[1]) * step * t * 0.5; // for scaling down
                                                                                                                 // the tenon

                    joint.f[j][2 + i].emplace_back (y[j_inv], y[1], z_);
                    joint.f[j][2 + i].emplace_back (y[j_inv], y[0], z_);

                    joint.f[j][2 + i].emplace_back (joint.f[j][2 + i].front ());
                    // copy
                    joint.f[j][2 + i + 1] = joint.f[j][2 + i];
                }
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // boolean
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    joint.m_boolean_type = { wood::cut::insert_between_multiple_edges, wood::cut::insert_between_multiple_edges };
    joint.f_boolean_type.resize (2 + number_of_tenons);
    joint.f_boolean_type[0] = wood::cut::insert_between_multiple_edges;
    joint.f_boolean_type[1] = wood::cut::insert_between_multiple_edges;
    for (int i = 0; i < number_of_tenons; i += 2)
        {
            joint.f_boolean_type[2 + i] = wood::cut::hole;
            joint.f_boolean_type[2 + i + 1] = wood::cut::hole;
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // joint for preview
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    database_writer::SCALE = 1.0;
    std::vector<std::vector<CGAL_Polyline> > input_polyline_pairs0;
    input_polyline_pairs0.emplace_back (joint.m[0]);
    input_polyline_pairs0.emplace_back (joint.m[1]);
    database_writer::add_polylines (input_polyline_pairs0.begin (),
                                    input_polyline_pairs0.end ()); // grey
    std::vector<std::vector<CGAL_Polyline> > input_polyline_pairs1;
    input_polyline_pairs1.emplace_back (joint.f[0]);
    input_polyline_pairs1.emplace_back (joint.f[1]);
    database_writer::add_polylines (input_polyline_pairs1.begin (),
                                    input_polyline_pairs1.end ()); // grey
    // reset
    database_writer::SCALE = 1000;
    return true;
}

bool
type_library_name_ts_e_p_5 ()
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<std::vector<IK::Point_3> > input_polyline_pairs;
    internal::set_file_path_for_input_xml_and_screenshot ("type_library_name_ts_e_p_5");

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // joint parameters
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    wood::joint joint;

    // name
    joint.name = "ts_e_p_5";

    // parameters that comes from the joint
    bool default_values = true;

    double edge_length = !default_values ? std::sqrt (CGAL::squared_distance (joint.joint_lines[0][0], joint.joint_lines[0][1])) : 1000;
    int divisions = !default_values ? joint.divisions : 5;
    double joint_volume_edge_length = !default_values ? std::sqrt (CGAL::squared_distance (joint.joint_volumes[0][1], joint.joint_volumes[0][2])) : 40;

    // scale down the edge, since wood_joint ->  bool
    // joint::orient_to_connection_area() make the distance between joint
    // volumes equal to 2nd joint volume edge
    edge_length *= joint.scale[2];

    // normalization to the unit space, joint_volume_edge_length is used
    // for parametrization
    double move_length_scaled = edge_length / (divisions * joint_volume_edge_length);
    double total_length_scaled = edge_length / joint_volume_edge_length;

    // movement vectors to translate the unit joint to the end of the edge
    // and then to its middle
    IK::Vector_3 dir (0, 0, 1);
    IK::Vector_3 move_from_center_to_the_end = dir * ((total_length_scaled * 0.5) - (move_length_scaled * 0.5));
    IK::Vector_3 move_length_dir = -dir * move_length_scaled;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Male default shape
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<CGAL_Polyline> male_0 = { {

                                              IK::Point_3 (-0.499996349848395, -0.499996349847159, 1.62789509252326),   IK::Point_3 (-0.499996349848395, -3.40695187221018, 1.62789509252326),
                                              IK::Point_3 (-0.499996349848395, -3.40695187221018, 1.10464309849792),    IK::Point_3 (-0.499996349848395, -0.0232556441796868, 1.10464309849801),
                                              IK::Point_3 (-0.499996349848395, -0.0232556441796339, 0.843017101485296), IK::Point_3 (-0.499996349848395, 0.49999634984571, 0.843017101485296),
                                              IK::Point_3 (-0.499996349848395, 0.49999634984571, 1.19185176416881),     IK::Point_3 (-0.499996349848395, 0.6038871791861, 1.19185176416881),
                                              IK::Point_3 (-0.499996349848395, 1.01079104575736, 1.04650398805065),     IK::Point_3 (-0.499996349848395, 1.01079104575736, 0.261625997012692),
                                              IK::Point_3 (-0.499996349848395, -3.40695187221018, 0.261625997012692),   IK::Point_3 (-0.499996349848395, -3.40695187221018, -0.261625997012652),
                                              IK::Point_3 (-0.499996349848395, 1.01079104575736, -0.261625997012652),   IK::Point_3 (-0.499996349848395, 1.01079104575736, -1.04650398805062),
                                              IK::Point_3 (-0.499996349848395, 0.6038871791861, -1.19185176416877),     IK::Point_3 (-0.499996349848395, 0.49999634984571, -1.19185176416877),
                                              IK::Point_3 (-0.499996349848395, 0.49999634984571, -0.843017101485257),   IK::Point_3 (-0.499996349848395, -0.0232556441796339, -0.843017101485257),
                                              IK::Point_3 (-0.499996349848395, -0.0232556441796868, -1.10464309849797), IK::Point_3 (-0.499996349848395, -3.40695187221018, -1.10464309849788),
                                              IK::Point_3 (-0.499996349848395, -3.40695187221018, -1.62789509252322),   IK::Point_3 (-0.499996349848395, -0.499996349847159, -1.62789509252322),
                                          },
                                          {

                                              IK::Point_3 (-0.499996349848395, -0.499996349847159, 1.62789509252326),
                                              IK::Point_3 (-0.499996349848395, -0.499996349847159, -1.62789509252322),
                                          } };

    std::vector<CGAL_Polyline> male_1 = { {

                                              IK::Point_3 (0.499996349844421, -0.49999634984737, 1.62789509252334),    IK::Point_3 (0.499996349844421, -3.40695187221039, 1.62789509252334),
                                              IK::Point_3 (0.499996349844421, -3.40695187221039, 1.10464309849799),    IK::Point_3 (0.499996349844421, -0.0232556441798983, 1.10464309849809),
                                              IK::Point_3 (0.499996349844421, -0.0232556441798454, 0.843017101485375), IK::Point_3 (0.499996349844421, 0.499996349845499, 0.843017101485375),
                                              IK::Point_3 (0.499996349844421, 0.499996349845499, 1.19185176416889),    IK::Point_3 (0.499996349844421, 0.603887179185889, 1.19185176416889),
                                              IK::Point_3 (0.499996349844421, 1.01079104575715, 1.04650398805073),     IK::Point_3 (0.499996349844421, 1.01079104575715, 0.261625997012771),
                                              IK::Point_3 (0.499996349844421, -3.40695187221039, 0.261625997012771),   IK::Point_3 (0.499996349844421, -3.40695187221039, -0.261625997012573),
                                              IK::Point_3 (0.499996349844421, 1.01079104575715, -0.261625997012573),   IK::Point_3 (0.499996349844421, 1.01079104575715, -1.04650398805054),
                                              IK::Point_3 (0.499996349844421, 0.603887179185889, -1.19185176416869),   IK::Point_3 (0.499996349844421, 0.499996349845499, -1.19185176416869),
                                              IK::Point_3 (0.499996349844421, 0.499996349845499, -0.843017101485177),  IK::Point_3 (0.499996349844421, -0.0232556441798454, -0.843017101485177),
                                              IK::Point_3 (0.499996349844421, -0.0232556441798983, -1.1046430984979),  IK::Point_3 (0.499996349844421, -3.40695187221039, -1.1046430984978),
                                              IK::Point_3 (0.499996349844421, -3.40695187221039, -1.62789509252314),   IK::Point_3 (0.499996349844421, -0.49999634984737, -1.62789509252314),
                                          },

                                          {
                                              IK::Point_3 (0.499996349844421, -0.49999634984737, 1.62789509252334),
                                              IK::Point_3 (0.499996349844421, -0.49999634984737, -1.62789509252314),
                                          } };

    std::vector<wood::cut::cut_type> male_types{ wood::cut::edge_insertion, wood::cut::edge_insertion };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // female default shape
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<CGAL_Polyline> female_0 = { {

                                                IK::Point_3 (-0.499996349848395, -0.499996349847212, 1.10464309849788),
                                                IK::Point_3 (-0.499996349848395, -0.499996349847212, -1.104643098498),
                                                IK::Point_3 (0.499996349844421, -0.499996349847317, -1.104643098498),
                                                IK::Point_3 (0.499996349844421, -0.499996349847317, 1.10464309849788),
                                                IK::Point_3 (-0.499996349848395, -0.499996349847212, 1.10464309849788),
                                            },
                                            {

                                                IK::Point_3 (-0.499996349848395, -0.499996349847212, 1.10464309849788),
                                                IK::Point_3 (-0.499996349848395, -0.499996349847212, -1.104643098498),
                                                IK::Point_3 (0.499996349844421, -0.499996349847317, -1.104643098498),
                                                IK::Point_3 (0.499996349844421, -0.499996349847317, 1.10464309849788),
                                                IK::Point_3 (-0.499996349848395, -0.499996349847212, 1.10464309849788),
                                            } };

    std::vector<CGAL_Polyline> female_1 = { {
                                                IK::Point_3 (-0.499996349848395, 0.499996349845604, 1.104643098498),
                                                IK::Point_3 (-0.499996349848395, 0.499996349845604, -1.10464309849788),
                                                IK::Point_3 (0.499996349844421, 0.499996349845499, -1.10464309849788),
                                                IK::Point_3 (0.499996349844421, 0.499996349845499, 1.104643098498),
                                                IK::Point_3 (-0.499996349848395, 0.499996349845604, 1.104643098498),
                                            },
                                            {

                                                IK::Point_3 (-0.499996349848395, 0.499996349845604, 1.104643098498),
                                                IK::Point_3 (-0.499996349848395, 0.499996349845604, -1.10464309849788),
                                                IK::Point_3 (0.499996349844421, 0.499996349845499, -1.10464309849788),
                                                IK::Point_3 (0.499996349844421, 0.499996349845499, 1.104643098498),
                                                IK::Point_3 (-0.499996349848395, 0.499996349845604, 1.104643098498),
                                            } };

    std::vector<wood::cut::cut_type> female_types{ wood::cut::hole, wood::cut::hole };
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Reserve memory for multiple copies
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    joint.m[0].reserve (2);
    joint.m[1].reserve (2);
    joint.m_boolean_type.reserve (2);
    joint.f[0].reserve (2 * divisions);
    joint.f[1].reserve (2 * divisions);
    joint.f_boolean_type.reserve (2 * divisions);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Copy the default shapes and move them
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    joint.m[0].emplace_back (CGAL_Polyline ());
    joint.m[1].emplace_back (CGAL_Polyline ());

    joint.m[0].back ().reserve (male_0[0].size () * divisions);
    joint.m[1].back ().reserve (male_1[0].size () * divisions);

    for (auto i = 0; i < divisions; i++)
        {
            // copy the first outline, be sure that the point order is
            // correct, so that the non-internsecting polyline can be
            // created, else reverse it
            CGAL_Polyline male_moved_0 = male_0[0];
            CGAL_Polyline male_moved_1 = male_1[0];

            // move joints that are positioned at the center to the end of
            // the segment and then back by half of the division length
            for (auto &p : male_moved_0)
                p += move_from_center_to_the_end + move_length_dir * i;

            for (auto &p : male_moved_1)
                p += move_from_center_to_the_end + move_length_dir * i;

            // merge with the main outline
            joint.m[0].back ().insert (joint.m[0].back ().end (), male_moved_0.begin (), male_moved_0.end ());
            joint.m[1].back ().insert (joint.m[1].back ().end (), male_moved_1.begin (), male_moved_1.end ());
        }

    for (auto i = 0; i < divisions; i++)
        {
            // copy the first outline, be sure that the point order is
            // correct, so that the non-internsecting polyline can be
            // created, else reverse it
            CGAL_Polyline female_moved_0 = female_0[0];
            CGAL_Polyline female_moved_1 = female_1[0];

            // move joints that are positioned at the center to the end of
            // the segment and then back by half of the division length
            for (auto &p : female_moved_0)
                p += move_from_center_to_the_end + move_length_dir * i;

            for (auto &p : female_moved_1)
                p += move_from_center_to_the_end + move_length_dir * i;

            // merge with the main outline
            joint.f[0].emplace_back (female_moved_0);
            joint.f[1].emplace_back (female_moved_1);
        }
    joint.f[0].emplace_back (std::initializer_list<IK::Point_3>{ joint.f[0].front ()[0], joint.f[0].front ()[3], joint.f[0].back ()[2], joint.f[0].back ()[1], joint.f[0].front ()[0] });
    joint.f[1].emplace_back (std::initializer_list<IK::Point_3>{ joint.f[1].front ()[0], joint.f[1].front ()[3], joint.f[1].back ()[2], joint.f[1].back ()[1], joint.f[1].front ()[0] });
    joint.f_boolean_type = std::vector<wood::cut::cut_type> (joint.f[0].size (), wood::cut::hole);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Add the insertion lines
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    joint.m[0].emplace_back (CGAL_Polyline{ joint.m[0].front ().front (), joint.m[0].front ().back () });
    joint.m[1].emplace_back (CGAL_Polyline{ joint.m[1].front ().front (), joint.m[1].front ().back () });

    joint.m_boolean_type = male_types;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // is unit scale
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    joint.unit_scale = true;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // joint for preview
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    database_writer::SCALE = 1.0;
    std::vector<std::vector<CGAL_Polyline> > input_polyline_pairs0;
    input_polyline_pairs0.emplace_back (joint.m[0]);
    input_polyline_pairs0.emplace_back (joint.m[1]);
    database_writer::add_polylines (input_polyline_pairs0.begin (),
                                    input_polyline_pairs0.end ()); // grey
    std::vector<std::vector<CGAL_Polyline> > input_polyline_pairs1;
    input_polyline_pairs1.emplace_back (joint.f[0]);
    input_polyline_pairs1.emplace_back (joint.f[1]);
    CGAL_Polyline default_segment = { IK::Point_3 (0, 0, -total_length_scaled * 0.5), IK::Point_3 (0, 0, total_length_scaled * 0.5) };
    input_polyline_pairs1.push_back ({ default_segment });
    database_writer::add_polylines (input_polyline_pairs0.begin (),
                                    input_polyline_pairs1.end ()); // grey
    // reset
    database_writer::SCALE = 1000;
    return true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// geometry methods
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool
type_geometry_name_cgal_mesh_boolean_0 ()
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The filename of the xml file and the screenshot directory
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    internal::set_file_path_for_input_xml_and_screenshot ("type_geometry_name_cgal_mesh_boolean_0");

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // data-set taken from https://schneide.blog/tag/cc/
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    CGAL::Surface_mesh<CGAL::Exact_predicates_inexact_constructions_kernel::Point_3> mesh_0;
    CGAL::Surface_mesh<CGAL::Exact_predicates_inexact_constructions_kernel::Point_3> mesh_1;

    mesh_0.add_vertex (CGAL::Epick::Point_3 (0, 0, 0));
    mesh_0.add_vertex (CGAL::Epick::Point_3 (0, 0, 1));
    mesh_0.add_vertex (CGAL::Epick::Point_3 (1, 0, 1));
    mesh_0.add_vertex (CGAL::Epick::Point_3 (1, 1, 1));
    mesh_0.add_vertex (CGAL::Epick::Point_3 (1, 0, 0));
    mesh_0.add_vertex (CGAL::Epick::Point_3 (1, 1, 0));
    mesh_0.add_vertex (CGAL::Epick::Point_3 (0, 1, 0));
    mesh_0.add_vertex (CGAL::Epick::Point_3 (0, 1, 1));

    mesh_0.add_face (CGAL::SM_Vertex_index (0), CGAL::SM_Vertex_index (4), CGAL::SM_Vertex_index (2));
    mesh_0.add_face (CGAL::SM_Vertex_index (4), CGAL::SM_Vertex_index (5), CGAL::SM_Vertex_index (3));
    mesh_0.add_face (CGAL::SM_Vertex_index (5), CGAL::SM_Vertex_index (6), CGAL::SM_Vertex_index (7));
    mesh_0.add_face (CGAL::SM_Vertex_index (6), CGAL::SM_Vertex_index (0), CGAL::SM_Vertex_index (1));
    mesh_0.add_face (CGAL::SM_Vertex_index (4), CGAL::SM_Vertex_index (0), CGAL::SM_Vertex_index (6));
    mesh_0.add_face (CGAL::SM_Vertex_index (6), CGAL::SM_Vertex_index (5), CGAL::SM_Vertex_index (4));
    mesh_0.add_face (CGAL::SM_Vertex_index (2), CGAL::SM_Vertex_index (7), CGAL::SM_Vertex_index (1));
    mesh_0.add_face (CGAL::SM_Vertex_index (7), CGAL::SM_Vertex_index (2), CGAL::SM_Vertex_index (3));
    mesh_0.add_face (CGAL::SM_Vertex_index (0), CGAL::SM_Vertex_index (2), CGAL::SM_Vertex_index (1));
    mesh_0.add_face (CGAL::SM_Vertex_index (4), CGAL::SM_Vertex_index (3), CGAL::SM_Vertex_index (2));
    mesh_0.add_face (CGAL::SM_Vertex_index (5), CGAL::SM_Vertex_index (7), CGAL::SM_Vertex_index (3));
    mesh_0.add_face (CGAL::SM_Vertex_index (6), CGAL::SM_Vertex_index (1), CGAL::SM_Vertex_index (7));

    mesh_1.add_vertex (CGAL::Epick::Point_3 (0.353451, 0.209961, 0.725612));
    mesh_1.add_vertex (CGAL::Epick::Point_3 (0.353451, 0.209961, 1.725612));
    mesh_1.add_vertex (CGAL::Epick::Point_3 (1.353451, 0.209961, 1.725612));
    mesh_1.add_vertex (CGAL::Epick::Point_3 (1.353451, 1.209961, 1.725612));
    mesh_1.add_vertex (CGAL::Epick::Point_3 (1.353451, 0.209961, 0.725612));
    mesh_1.add_vertex (CGAL::Epick::Point_3 (1.353451, 1.209961, 0.725612));
    mesh_1.add_vertex (CGAL::Epick::Point_3 (0.353451, 1.209961, 0.725612));
    mesh_1.add_vertex (CGAL::Epick::Point_3 (0.353451, 1.209961, 1.725612));

    mesh_1.add_face (CGAL::SM_Vertex_index (0), CGAL::SM_Vertex_index (4), CGAL::SM_Vertex_index (2));
    mesh_1.add_face (CGAL::SM_Vertex_index (4), CGAL::SM_Vertex_index (5), CGAL::SM_Vertex_index (3));
    mesh_1.add_face (CGAL::SM_Vertex_index (5), CGAL::SM_Vertex_index (6), CGAL::SM_Vertex_index (7));
    mesh_1.add_face (CGAL::SM_Vertex_index (6), CGAL::SM_Vertex_index (0), CGAL::SM_Vertex_index (1));
    mesh_1.add_face (CGAL::SM_Vertex_index (4), CGAL::SM_Vertex_index (0), CGAL::SM_Vertex_index (6));
    mesh_1.add_face (CGAL::SM_Vertex_index (6), CGAL::SM_Vertex_index (5), CGAL::SM_Vertex_index (4));
    mesh_1.add_face (CGAL::SM_Vertex_index (2), CGAL::SM_Vertex_index (7), CGAL::SM_Vertex_index (1));
    mesh_1.add_face (CGAL::SM_Vertex_index (7), CGAL::SM_Vertex_index (2), CGAL::SM_Vertex_index (3));
    mesh_1.add_face (CGAL::SM_Vertex_index (0), CGAL::SM_Vertex_index (2), CGAL::SM_Vertex_index (1));
    mesh_1.add_face (CGAL::SM_Vertex_index (4), CGAL::SM_Vertex_index (3), CGAL::SM_Vertex_index (2));
    mesh_1.add_face (CGAL::SM_Vertex_index (5), CGAL::SM_Vertex_index (7), CGAL::SM_Vertex_index (3));
    mesh_1.add_face (CGAL::SM_Vertex_index (6), CGAL::SM_Vertex_index (1), CGAL::SM_Vertex_index (7));

    std::vector<Mesh> mesh_list;
    mesh_list.reserve (2);
    mesh_list.emplace_back (mesh_0);
    mesh_list.emplace_back (mesh_1);

    // run the boolean intersection

    size_t difference_union_intersection = 0;

    std::vector<double> coord_out;
    std::vector<double> normals_out;
    int n_coord_out;
    std::vector<int> faces_out;
    int n_faces_out;
    std::vector<int> facesColors_out;
    int n_facesColors_out;
    int n_valid_meshes = 9;

    cgal::mesh_boolean::mesh_boolean_create_array_track_colors (

        // double *coord_mesh, int *n_coord_meshArray, // Flat array of
        // coordinates / 0 256 512 flat array of vertices array / number of
        // meshes int *faces_mesh, int *n_faces_meshArray, size_t n_mesh,
        mesh_list, difference_union_intersection,

        coord_out, n_coord_out, normals_out, faces_out, n_faces_out, facesColors_out, n_facesColors_out, n_valid_meshes

    );

    std::cout << "\n______________________________________________________"
                 "____\n "
                 "mesh_boolean_test() \n n_coord_out is 22 ? "
              << n_coord_out << "\n n_normals_out is 22 ? " << normals_out.size () / 3 << "\n n_faces_out is 40 ? " << n_faces_out << "\n n_facesColors_out is 40 ? " << n_facesColors_out << "\n n_valid_meshes is 2 ? "
              << n_valid_meshes
              << "\n______________________________________________________"
                 "____\n";
    return true;
}

bool
type_geometry_name_polygon_center ()
{
    // screenshot
    internal::set_file_path_for_input_xml_and_screenshot ("type_geometry_name_polygon_center");

    // data-set
    CGAL_Polyline polygon{
        IK::Point_3 (11.2247596176396, -4.60190643733344, 48.9387618537435), IK::Point_3 (-5.73799008527948, 27.7396887390699, 38.9994193935361), IK::Point_3 (-7.18751465466158, 52.2109764075134, 74.5663582911656),
        IK::Point_3 (1.68332949862521, 18.8956298287167, 52.2485552589767),  IK::Point_3 (14.0704361211272, 1.10346344393741, 69.2793807577245),  IK::Point_3 (18.9418950031543, 14.2294672539151, 109.735250000281),
        IK::Point_3 (33.4352381327694, -30.7000564810127, 89.2118909021016), IK::Point_3 (11.2247596176396, -4.60190643733344, 48.9387618537435),
    };

    // main method
    IK::Point_3 center = cgal::polyline_util::center (polygon);
    std::vector<IK::Point_3> points = { center };

    // display
    database_writer::SCALE = 10;
    std::vector<CGAL_Polyline> polylines = { polygon };
    database_writer::add_polylines (polylines.begin (),
                                    polylines.end ()); // grey
    database_writer::LINE_THICKNESS = 10;
    database_writer::add_points (points);
    database_writer::LINE_THICKNESS = 3;
    database_writer::SCALE = 1000;

    // test
    return true;
}

bool
type_geometry_name_polygon_center_polylabel ()
{
    // screenshot
    internal::set_file_path_for_input_xml_and_screenshot ("type_geometry_name_polygon_center_polylabel");

    // data-set
    std::vector<CGAL_Polyline> polylines;

    CGAL_Polyline p0{
        IK::Point_3 (-51.268164698738, 124.403259575835, 0),
        IK::Point_3 (-49.9053069162896, 125.207270883064, 0),
        IK::Point_3 (-49.2387999055025, 123.598641360433, 0),
        IK::Point_3 (-51.268164698738, 124.403259575835, 0),
    };

    CGAL_Polyline p1{
        IK::Point_3 (-80.6608706869908, 113.550451407233, 0), IK::Point_3 (-105.295437496349, 124.537127782527, 0), IK::Point_3 (-76.4480884206953, 152.30682138802, 0),
        IK::Point_3 (-23.6934094178782, 139.390066049447, 0), IK::Point_3 (-40.1282334662578, 100.282298806669, 0), IK::Point_3 (-69.6350088663798, 87.7972305312764, 0),
        IK::Point_3 (-74.8538706538141, 108.464053813429, 0), IK::Point_3 (-80.6608706869908, 113.550451407233, 0),
    };

    CGAL_Polyline p2{
        IK::Point_3 (-73.4808769486925, 119.997690099402, 0),
        IK::Point_3 (-90.6424155869293, 123.188472670033, 0),
        IK::Point_3 (-77.6978215210285, 136.742511096638, 0),
        IK::Point_3 (-73.4808769486925, 119.997690099402, 0),
    };

    polylines.push_back (p0);
    polylines.push_back (p1);
    polylines.push_back (p2);

    // main method
    std::tuple<IK::Point_3, IK::Plane_3, double> result = cgal::inscribe_util::get_polylabel (polylines, 1.0);
    IK::Point_3 center = std::get<0> (result);
    std::vector<IK::Point_3> points = { center };

    // display
    database_writer::SCALE = 100;
    database_writer::add_polylines (polylines.begin (), polylines.end ());
    database_writer::LINE_THICKNESS = 10;
    database_writer::add_points (points);
    database_writer::LINE_THICKNESS = 3;
    database_writer::SCALE = 1000;

    // test
    return true;
}

bool
type_geometry_name_circle_ponts_inscribed_in_a_polygon ()
{
    // screenshot
    internal::set_file_path_for_input_xml_and_screenshot ("type_geometry_name_circle_ponts_inscribed_in_a_polygon");

    // data-set
    CGAL_Polyline polygon{
        IK::Point_3 (11.2247596176396, -4.60190643733344, 48.9387618537435), IK::Point_3 (-5.73799008527948, 27.7396887390699, 38.9994193935361), IK::Point_3 (-7.18751465466158, 52.2109764075134, 74.5663582911656),
        IK::Point_3 (1.68332949862521, 18.8956298287167, 52.2485552589767),  IK::Point_3 (14.0704361211272, 1.10346344393741, 69.2793807577245),  IK::Point_3 (18.9418950031543, 14.2294672539151, 109.735250000281),
        IK::Point_3 (33.4352381327694, -30.7000564810127, 89.2118909021016), IK::Point_3 (11.2247596176396, -4.60190643733344, 48.9387618537435),
    };

    // main method
    std::vector<IK::Point_3> points;

    cgal::inscribe_util::get_polylabel_circle_division_points (IK::Vector_3 (0, 0, 0), { polygon }, points, 4, 0.75, 1.0, true);

    // display
    database_writer::SCALE = 10;
    std::vector<CGAL_Polyline> polylines = { polygon };
    database_writer::add_polylines (polylines.begin (), polylines.end ());
    database_writer::LINE_THICKNESS = 10;
    database_writer::add_points (points);
    database_writer::LINE_THICKNESS = 3;
    database_writer::SCALE = 1000;

    // test
    return true;
}

bool
type_geometry_name_grid_of_points_in_a_polygon ()
{
    // screenshot
    internal::set_file_path_for_input_xml_and_screenshot ("type_geometry_name_grid_of_points_in_a_polygon");

    // data-set
    CGAL_Polylines polygons{ {
                                 IK::Point_3 (11.2247596176396, -4.60190643733344, 48.9387618537435),
                                 IK::Point_3 (-5.73799008527948, 27.7396887390699, 38.9994193935361),
                                 IK::Point_3 (-7.18751465466158, 52.2109764075134, 74.5663582911656),
                                 IK::Point_3 (1.68332949862521, 18.8956298287167, 52.2485552589767),
                                 IK::Point_3 (14.0704361211272, 1.10346344393741, 69.2793807577245),
                                 IK::Point_3 (18.9418950031543, 14.2294672539151, 109.735250000281),
                                 IK::Point_3 (33.4352381327694, -30.7000564810127, 89.2118909021016),
                                 IK::Point_3 (11.2247596176396, -4.60190643733344, 48.9387618537435),
                             },
                             {
                                 IK::Point_3 (0, 0, 0),
                                 IK::Point_3 (20, 0 + 10, 0),
                                 IK::Point_3 (20, 20 + 20, 0),
                                 IK::Point_3 (0, 20, 0),
                                 IK::Point_3 (0, 0, 0),
                             } };

    for (auto &polygon : polygons)
        {
            // main method
            std::vector<IK::Point_3> points;
            cgal::rectangle_util::grid_of_points_in_a_polygon (polygon, -2.5, 2.5, 100, points);

            // display
            database_writer::SCALE = 10;
            std::vector<CGAL_Polyline> polylines = { polygon };
            database_writer::add_polylines (polylines.begin (), polylines.end ());
            database_writer::LINE_THICKNESS = 10;
            database_writer::add_points (points); // Error when running twice
            database_writer::LINE_THICKNESS = 3;
            database_writer::SCALE = 1000;
        }
    // test
    return true;
}

bool
type_geometry_name_offset_polygon_inside_and_divide_into_points ()
{
    // screenshot
    internal::set_file_path_for_input_xml_and_screenshot ("type_geometry_name_offset_polygon_inside_and_divide_into_points");

    // data-set
    CGAL_Polyline polygon{
        IK::Point_3 (11.2247596176396, -4.60190643733344, 48.9387618537435), IK::Point_3 (-5.73799008527948, 27.7396887390699, 38.9994193935361), IK::Point_3 (-7.18751465466158, 52.2109764075134, 74.5663582911656),
        IK::Point_3 (1.68332949862521, 18.8956298287167, 52.2485552589767),  IK::Point_3 (14.0704361211272, 1.10346344393741, 69.2793807577245),  IK::Point_3 (18.9418950031543, 14.2294672539151, 109.735250000281),
        IK::Point_3 (33.4352381327694, -30.7000564810127, 89.2118909021016), IK::Point_3 (11.2247596176396, -4.60190643733344, 48.9387618537435),
    };

    // main method
    std::vector<IK::Point_3> points;
    collider::clipper_util::offset_and_divide_to_points (points, polygon, -2.5, 4);

    // display
    database_writer::SCALE = 10;
    std::vector<CGAL_Polyline> polylines = { polygon }; //, polygon_copy
    database_writer::add_polylines (polylines.begin (), polylines.end ());
    database_writer::LINE_THICKNESS = 10;
    database_writer::add_points (points); // Error when running twice
    database_writer::LINE_THICKNESS = 3;
    database_writer::SCALE = 1000;

    // test
    return true;
}

bool
type_geometry_name_rectangle_points_inscribed_in_polygon ()
{
    // screenshot
    internal::set_file_path_for_input_xml_and_screenshot ("type_geometry_name_rectangle_ponts_inscribed_in_a_polygon");

    // data-set
    CGAL_Polyline polygon{
        IK::Point_3 (11.2247596176396, -4.60190643733344, 48.9387618537435),
        IK::Point_3 (-5.73799008527948, 27.7396887390699, 38.9994193935361),
        IK::Point_3 (-7.18751465466158, 52.2109764075134, 74.5663582911656),
        // IK::Point_3(1.68332949862521, 18.8956298287167, 52.2485552589767),
        // IK::Point_3(14.0704361211272, 1.10346344393741, 69.2793807577245),
        IK::Point_3 (18.9418950031543, 14.2294672539151, 109.735250000281),
        IK::Point_3 (33.4352381327694, -30.7000564810127, 89.2118909021016),
        IK::Point_3 (11.2247596176396, -4.60190643733344, 48.9387618537435),
    };

    // main method
    std::vector<IK::Point_3> points;
    CGAL_Polyline polygon_inscribed_rectangle;
    IK::Segment_3 segment;
    double division_distance = -10; // negative value = grid, positive = edge division
    double scale = 0.95;
    double precision = 1;
    cgal::inscribe_util::inscribe_rectangle_in_convex_polygon ({ polygon }, polygon_inscribed_rectangle, points, segment, scale, precision, division_distance);

    // display
    database_writer::SCALE = 10;
    std::vector<CGAL_Polyline> polylines = { polygon_inscribed_rectangle, polygon }; //, polygon_copy
    database_writer::add_polylines (polylines.begin (), polylines.end ());
    database_writer::LINE_THICKNESS = 10;
    database_writer::add_points (points); // Error when running twice
    database_writer::LINE_THICKNESS = 3;
    database_writer::SCALE = 1000;

    // test
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GoogleTest
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// plate methods
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// side-to-side in-plane and out-of-plane
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (wood, type_plates_name_hexbox_and_corner) { EXPECT_EQ (type_plates_name_hexbox_and_corner (), true); }

TEST (wood, type_plates_name_joint_linking_vidychapel_corner) { EXPECT_EQ (type_plates_name_joint_linking_vidychapel_corner (), true); }

TEST (wood, type_plates_name_joint_linking_vidychapel_one_layer) { EXPECT_EQ (type_plates_name_joint_linking_vidychapel_one_layer (), true); }

TEST (wood, type_plates_name_joint_linking_vidychapel_one_axis_two_layers) { EXPECT_EQ (type_plates_name_joint_linking_vidychapel_one_axis_two_layers (), true); }

TEST (wood, type_plates_name_joint_linking_vidychapel_full) { EXPECT_EQ (type_plates_name_joint_linking_vidychapel_full (), true); }

TEST (wood, type_plates_name_side_to_side_edge_inplane_2_butterflies) { EXPECT_EQ (type_plates_name_side_to_side_edge_inplane_2_butterflies (), true); }

TEST (wood, type_plates_name_side_to_side_edge_inplane_hexshell) { EXPECT_EQ (type_plates_name_side_to_side_edge_inplane_hexshell (), true); }

TEST (wood, type_plates_name_side_to_side_edge_inplane_differentdirections) { EXPECT_EQ (type_plates_name_side_to_side_edge_inplane_differentdirections (), true); }

TEST (wood, type_plates_name_side_to_side_edge_outofplane_folding) { EXPECT_EQ (type_plates_name_side_to_side_edge_outofplane_folding (), true); }

TEST (wood, type_plates_name_side_to_side_edge_outofplane_box) { EXPECT_EQ (type_plates_name_side_to_side_edge_outofplane_box (), true); }

TEST (wood, type_plates_name_side_to_side_edge_outofplane_tetra) { EXPECT_EQ (type_plates_name_side_to_side_edge_outofplane_tetra (), true); }

TEST (wood, type_plates_name_side_to_side_edge_outofplane_dodecahedron) { EXPECT_EQ (type_plates_name_side_to_side_edge_outofplane_dodecahedron (), true); }

TEST (wood, type_plates_name_side_to_side_edge_outofplane_icosahedron) { EXPECT_EQ (type_plates_name_side_to_side_edge_outofplane_icosahedron (), true); }

TEST (wood, type_plates_name_side_to_side_edge_outofplane_octahedron) { EXPECT_EQ (type_plates_name_side_to_side_edge_outofplane_octahedron (), true); }

TEST (wood, type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners) { EXPECT_EQ (type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners (), true); }

TEST (wood, type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_combined) { EXPECT_EQ (type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_combined (), true); }

TEST (wood, type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths) { EXPECT_EQ (type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths (), true); }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// top-to-top data sets
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (wood, type_plates_name_top_to_top_pairs) { EXPECT_EQ (type_plates_name_top_to_top_pairs (), true); }

TEST (wood, type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes) { EXPECT_EQ (type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes (), true); }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// top-to-side data sets
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (wood, type_plates_name_top_to_side_snap_fit) { EXPECT_EQ (type_plates_name_top_to_side_snap_fit (), true); }

TEST (wood, type_plates_name_top_to_side_box) { EXPECT_EQ (type_plates_name_top_to_side_box (), true); }

TEST (wood, type_plates_name_top_to_side_corners) { EXPECT_EQ (type_plates_name_top_to_side_corners (), true); }

TEST (wood, type_plates_name_top_to_side_and_side_to_side_outofplane_annen_corner) { EXPECT_EQ (type_plates_name_top_to_side_and_side_to_side_outofplane_annen_corner (), true); }

TEST (wood, type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box) { EXPECT_EQ (type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box (), true); }

TEST (wood, type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box_pair) { EXPECT_EQ (type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box_pair (), true); }

TEST (wood, type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_small) { EXPECT_EQ (type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_small (), true); }

TEST (wood, type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_full_arch) { EXPECT_EQ (type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_full_arch (), true); }

TEST (wood, type_plates_name_vda_floor_0) { EXPECT_EQ (type_plates_name_vda_floor_0 (), true); }

TEST (wood, type_plates_name_vda_floor_2) { EXPECT_EQ (type_plates_name_vda_floor_2 (), true); }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cross data sets
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (wood, type_plates_name_cross_and_sides_corner) { EXPECT_EQ (type_plates_name_cross_and_sides_corner (), true); }

TEST (wood, type_plates_name_cross_corners) { EXPECT_EQ (type_plates_name_cross_corners (), true); }

TEST (wood, type_plates_name_cross_vda_corner) { EXPECT_EQ (type_plates_name_cross_vda_corner (), true); }

TEST (wood, type_plates_name_cross_vda_hexshell) { EXPECT_EQ (type_plates_name_cross_vda_hexshell (), true); }

TEST (wood, type_plates_name_cross_vda_hexshell_reciprocal) { EXPECT_EQ (type_plates_name_cross_vda_hexshell_reciprocal (), true); }

TEST (wood, type_plates_name_cross_square_reciprocal_two_sides) { EXPECT_EQ (type_plates_name_cross_square_reciprocal_two_sides (), true); }

TEST (wood, type_plates_name_cross_square_reciprocal_iseya) { EXPECT_EQ (type_plates_name_cross_square_reciprocal_iseya (), true); }

TEST (wood, type_plates_name_cross_ibois_pavilion) { EXPECT_EQ (type_plates_name_cross_ibois_pavilion (), true); }

TEST (wood, type_plates_name_cross_brussels_sports_tower) { EXPECT_EQ (type_plates_name_cross_brussels_sports_tower (), true); }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// beam methods
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (wood, type_beams_name_phanomema_node) { EXPECT_EQ (type_beams_name_phanomema_node (), true); }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// library of joints
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (wood, type_library_name_ss_e_ip_2) { EXPECT_EQ (type_library_name_ss_e_ip_2 (), true); }

TEST (wood, type_library_name_ss_e_op_4) { EXPECT_EQ (type_library_name_ss_e_op_4 (), true); }

TEST (wood, type_library_name_ts_e_p_5) { EXPECT_EQ (type_library_name_ts_e_p_5 (), true); }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// geometry methods
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (wood, type_geometry_name_cgal_mesh_boolean_0) { EXPECT_EQ (type_geometry_name_cgal_mesh_boolean_0 (), true); }

TEST (wood, type_geometry_name_polygon_center) { EXPECT_EQ (type_geometry_name_polygon_center (), true); }

TEST (wood, type_geometry_name_polygon_center_polylabel) { EXPECT_EQ (type_geometry_name_polygon_center_polylabel (), true); }

TEST (wood, type_geometry_name_circle_ponts_inscribed_in_a_polygon) { EXPECT_EQ (type_geometry_name_circle_ponts_inscribed_in_a_polygon (), true); }

TEST (wood, type_geometry_name_grid_of_points_in_a_polygon) { EXPECT_EQ (type_geometry_name_grid_of_points_in_a_polygon (), true); }

TEST (wood, type_geometry_name_offset_polygon_inside_and_divide_into_points) { EXPECT_EQ (type_geometry_name_offset_polygon_inside_and_divide_into_points (), true); }

TEST (wood, type_geometry_name_rectangle_points_inscribed_in_polygon) { EXPECT_EQ (type_geometry_name_rectangle_points_inscribed_in_polygon (), true); }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// main method that calls all the tests, this function must be called in
// the main function
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
run_all_tests ()
{
    ::testing::InitGoogleTest (); //&argc, argv
    int test_result = RUN_ALL_TESTS ();
    // Use test_result as needed, or ignore it if not needed
}

void
unit_joints ()
{
}
} // namespace test
} // namespace wood