#include "../../../stdafx.h"
#include "wood_test.h"

namespace wood_test
{

    void test_cgal_mesh_boolean_0()
    {
        cgal_mesh_boolean::mesh_boolean_test();
    }

    void test_side_to_top()
    {

        // run ninja-> g++ main.cpp -o main
        // cmake  --fresh -DGET_LIBS=OFF -DBUILD_MY_PROJECTS=ON -DRELEASE_DEBUG=ON -DCMAKE_BUILD_TYPE="Release" -G "Ninja" .. && cmake  --build . --config Release
        // cmake --fresh -DGET_LIBS=OFF -DBUILD_MY_PROJECTS=ON -DRELEASE_DEBUG=ON -DCMAKE_BUILD_TYPE="Release"  -G "Visual Studio 17 2022" -A x64 .. && cmake --build . --config Release
        // cmake  -E time --build . -v --config Release -- -j0
        // cmake  --build . -v --config Release -- -j0
        // cmake  --build . --config Release -v

        // #include "stdafx.h"

        // #include "viewer/include/opengl/opengl_meshes.h"
        // #include "viewer/include/opengl/opengl_cameras.h"
        // #define gui
        // //#ifdef(gui) //only declare this if the following header is outside of the precompiled header
        // #include "viewer/include/imgui/imgui_render.h"
        // //#endif//only declare this if the following header is outside of the precompiled header
        // #include "viewer/include/opengl/opengl_globals_geometry.h"
        // #include "viewer/include/opengl/opengl_render.h"

        // // temp
        // #include "wood/include/wood_main.h"
        // #include "wood/include/wood_xml.h" // xml parser and viewer customization
        // #include "wood/include/wood_test.h"

        // // viewer
        // #include "wood/include/viewer_wood.h"

        // // data_set
        // #include "wood/include/wood_data_set.h"

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // viewer type and shader location
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        opengl_globals::shader_type_0default_1transparent_2shaded_3shadedwireframe_4wireframe_5normals_6explode = 3;
        opengl_globals::shaders_folder = "C:\\IBOIS57\\_Code\\Software\\CPP\\CMAKE\\super_build\\wood\\src\\viewer\\shaders\\";
        opengl_globals_geometry::add_grid();

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Input
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        bool simple_case = false;
        std::vector<std::vector<IK::Point_3>> input_polyline_pairs; //= wood_data_set::ss_21;

        // Global Parameters and output joint selection and orientation
        double division_length = 300;
        std::vector<double> default_parameters_for_joint_types{
            300,
            0.5,
            8,
            450,
            0.64,
            10,
            450, // top-to-side
            0.5,
            20,
            300,
            0.5,
            30,
            300,
            0.5,
            40,
            300,
            0.5,
            58,
            300,
            1.0,
            60

        };

        bool compute_joints = true;

        int search_type = 0;
        int output_type = 4;
        std::vector<double> scale = {1, 1, 1};

        std::vector<std::vector<IK::Vector_3>> input_insertion_vectors;
        std::vector<std::vector<int>> input_joint_types;
        std::vector<std::vector<int>> input_three_valence_element_indices_and_instruction;
        std::vector<int> input_adjacency;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Main Method of Wood
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // output
        std::vector<std::vector<CGAL_Polyline>> output_plines;
        std::vector<std::vector<cut_type>> output_types;
        std::vector<std::vector<int>> top_face_triangulation;

        get_connection_zones(

            // input
            input_polyline_pairs,
            input_insertion_vectors,
            input_joint_types,
            input_three_valence_element_indices_and_instruction,
            input_adjacency,

            // output
            output_plines,
            output_types,
            top_face_triangulation,

            // Global Parameters
            default_parameters_for_joint_types,
            scale,
            search_type,
            output_type,
            0);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Display
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        viewer_wood::add(output_plines); // grey
        viewer_wood::add_loft(output_plines);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Render
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // opengl_render::render();
    }

    // Minor correction Joint side-to-side 13, in get_joints_geometry - 4 - merge joints produces outlines with duplicated points
    void test_chapel()
    {
        std::cout << "imgui_render -> test_chapel\n";

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Input for the Main Method
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        bool simple_case = false;
        path_and_file_for_input_polylines = "C:\\IBOIS57\\_Code\\Software\\CPP\\CMAKE\\super_build\\wood\\src\\wood\\dataset\\chapel_simple_case.xml";
        std::vector<std::vector<IK::Point_3>>
            input_polyline_pairs;
        path_and_file_for_input_polylines = xml_parser::read_xml_polylines(input_polyline_pairs, simple_case);
        double division_length = 300;
        std::vector<double> joint_types = wood_globals::joint_types;
        joint_types[1 * 3 + 0] = 100;
        std::cout
            << "\nwood_test -> joint_types\n";
        for (auto &joint_type : joint_types)
            std::cout << joint_type << "\n";
        std::cout << "\n";
        int search_type = 0;
        int output_type = wood_globals::output_geometry_type; // 0 - Plate outlines 1 - joint lines 2 - joint volumes 3 - joint geometry 4 - merge
        std::cout << "\n output_type " << output_type << "\n";
        std::vector<double> scale = {1, 1, 1};
        std::vector<std::vector<IK::Vector_3>> input_insertion_vectors;
        std::vector<std::vector<int>> input_joint_types;
        std::vector<std::vector<int>> input_three_valence_element_indices_and_instruction;
        std::vector<int> input_adjacency = {
            2, 4, -1, -1, 3, 5, -1, -1, 8, 10, -1, -1, 9, 11, -1, -1, 14, 16, -1, -1, 15, 17, -1, -1, 20, 22, -1, -1, 21, 23, -1, -1, 26, 28, -1, -1, 27, 29, -1, -1, 32, 34, -1, -1, 33, 35, -1, -1, 38, 40, -1, -1, 39, 41, -1, -1, 44, 46, -1, -1, 45, 47, -1, -1, 50, 52, -1, -1, 51, 53, -1, -1, 56, 58, -1, -1, 57, 59, -1, -1, 62, 64, -1, -1, 63, 65, -1, -1, 68, 70, -1, -1, 69, 71, -1, -1, 74, 76, -1, -1, 75, 77, -1, -1, 80, 82, -1, -1, 81, 83, -1, -1, 70, 76, -1, -1, 71, 77, -1, -1, 68, 74, -1, -1, 69, 75, -1, -1, 66, 72, -1, -1, 67, 73, -1, -1, 0, 2, -1, -1, 1, 3, -1, -1, 6, 8, -1, -1, 7, 9, -1, -1, 12, 14, -1, -1, 13, 15, -1, -1, 18, 20, -1, -1, 19, 21, -1, -1, 24, 26, -1, -1, 25, 27, -1, -1, 30, 32, -1, -1, 31, 33, -1, -1, 36, 38, -1, -1, 37, 39, -1, -1, 42, 44, -1, -1, 43, 45, -1, -1, 48, 50, -1, -1, 49, 51, -1, -1, 54, 56, -1, -1, 55, 57, -1, -1, 60, 62, -1, -1, 61, 63, -1, -1, 66, 68, -1, -1, 67, 69, -1, -1, 72, 74, -1, -1, 73, 75, -1, -1, 78, 80, -1, -1, 79, 81, -1, -1, 58, 64, -1, -1, 59, 65, -1, -1, 56, 62, -1, -1, 57, 63, -1, -1, 54, 60, -1, -1, 55, 61, -1, -1, 46, 52, -1, -1, 47, 53, -1, -1, 44, 50, -1, -1, 45, 51, -1, -1, 42, 48, -1, -1, 43, 49, -1, -1, 34, 40, -1, -1, 35, 41, -1, -1, 32, 38, -1, -1, 33, 39, -1, -1, 30, 36, -1, -1, 31, 37, -1, -1, 22, 28, -1, -1, 23, 29, -1, -1, 20, 26, -1, -1, 21, 27, -1, -1, 18, 24, -1, -1, 19, 25, -1, -1, 10, 16, -1, -1, 11, 17, -1, -1, 8, 14, -1, -1, 9, 15, -1, -1, 6, 12, -1, -1, 7, 13, -1, -1};

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Main Method of Wood
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        std::vector<std::vector<CGAL_Polyline>> output_plines;
        std::vector<std::vector<cut_type>> output_types;
        std::vector<std::vector<int>> top_face_triangulation;

        get_connection_zones(
            // input
            input_polyline_pairs,
            input_insertion_vectors,
            input_joint_types,
            input_three_valence_element_indices_and_instruction,
            input_adjacency,
            // output
            output_plines,
            output_types,
            top_face_triangulation,
            // Global Parameters
            joint_types,
            scale,
            search_type,
            output_type,
            0);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Export
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        path_and_file_for_output_polylines = "C:\\IBOIS57\\_Code\\Software\\CPP\\CMAKE\\super_build\\wood\\src\\wood\\dataset\\chapel_out.xml";
        // xml_parser::write_xml_polylines(output_plines);
        xml_parser::write_xml_polylines_and_types(output_plines, output_types);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Display
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        opengl_globals_geometry::add_grid();
        viewer_wood::line_thickness = 1;

        switch (output_type)
        {
        case (0):
        case (2):
            viewer_wood::add(input_polyline_pairs); // grey
            viewer_wood::add_areas(output_plines);
            break;

        case (3):
            viewer_wood::add(input_polyline_pairs); // grey
            viewer_wood::line_thickness = 3;
            viewer_wood::add(output_plines, 0); // grey
            break;

        case (4):

            viewer_wood::line_thickness = 3;
            viewer_wood::add(output_plines, 3);   // grey
            viewer_wood::add_loft(output_plines); // grey
            break;
        }
    }

    void ss_e_op_4()
    {

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // viewer type and shader location
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        opengl_globals::shader_type_0default_1transparent_2shaded_3shadedwireframe_4wireframe_5normals_6explode = 3;
        opengl_globals::shaders_folder = "C:\\IBOIS57\\_Code\\Software\\CPP\\CMAKE\\super_build\\wood\\src\\viewer\\shaders\\";
        opengl_globals_geometry::add_grid();

        joint joint;
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // joint parameters
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        joint.name = "ss_e_op_4";
        int number_of_tenons = 3;
        std::array<double, 2> x = {-1, 0.5};
        std::array<double, 2> y = {-0.50, 0.50};
        std::array<double, 2> z = {-0.45, 0.45};
        double z_extension = 0.01;
        std::array<double, 2> z_ext = {z[0] - z_extension, z[1] + z_extension};
        number_of_tenons = std::min(50, std::max(1, number_of_tenons)) * 2;
        double step = 1 / ((double)number_of_tenons - 1);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Male
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        for (int j = 0; j < 2; j++)
        {
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // memory and variables
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            joint.m[j].resize(2);
            joint.m[j][0].reserve(4 + number_of_tenons * 2);
            int sign = j == 0 ? -1 : 1;

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // tenon interpolation
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            joint.m[j][0].emplace_back(sign * x[1], y[j], z_ext[1]);
            joint.m[j][0].emplace_back(x[1] + j * 0.01, y[j], z_ext[1]);

            for (int i = 0; i < number_of_tenons; i++)
            {
                double z_ = z[1] + (z[0] - z[1]) * step * i;
                joint.m[j][0].emplace_back(x[(i + 1) % 2], y[j], z_);
                joint.m[j][0].emplace_back(x[(i + 0) % 2], y[j], z_);
            }

            joint.m[j][0].emplace_back(x[1] + j * 0.01, y[j], z_ext[0]);
            joint.m[j][0].emplace_back(sign * x[1], y[j], z_ext[0]);

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // cut outlines
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            joint.m[j][1] = {
                IK::Point_3(sign * x[1], y[j], z_ext[1]),
                IK::Point_3(sign * x[1], y[j], z_ext[0]),
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
            joint.f[j].resize(2 + number_of_tenons);
            int sign = j == 0 ? 1 : -1;
            int j_inv = j == 0 ? 1 : 0;
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // main outlines
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            joint.f[j][0] =
                {
                    IK::Point_3(y[j_inv], sign * x[1], z_ext[1]),
                    IK::Point_3(y[j_inv], 1.5 * x[0], z_ext[1]),
                    IK::Point_3(y[j_inv], 1.5 * x[0], z_ext[0]),
                    IK::Point_3(y[j_inv], sign * x[1], z_ext[0]),
                };

            joint.f[j][1] =
                {
                    IK::Point_3(y[j_inv], sign * x[1], z_ext[1]),
                    IK::Point_3(y[j_inv], sign * x[1], z_ext[1]),
                };

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // holes
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            for (int i = 0; i < number_of_tenons; i += 2)
            {
                double z_ = z[1] + (z[0] - z[1]) * step * i;
                joint.f[j][2 + i].reserve(5);
                joint.f[j][2 + i + 1].reserve(5);
                joint.f[j][2 + i].emplace_back(y[j_inv], 0.5 * x[0], z_);
                joint.f[j][2 + i].emplace_back(y[j_inv], x[1], z_);

                z_ = z[1] + (z[0] - z[1]) * step * (i + 1);
                joint.f[j][2 + i].emplace_back(y[j_inv], x[1], z_);
                joint.f[j][2 + i].emplace_back(y[j_inv], 0.5 * x[0], z_);

                joint.f[j][2 + i].emplace_back(joint.f[j][2 + i].front());
                // copy
                joint.f[j][2 + i + 1] = joint.f[j][2 + i];
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // boolean
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        joint.m_boolean_type = {insert_between_multiple_edges, insert_between_multiple_edges};
        joint.f_boolean_type.resize(2 + number_of_tenons);
        joint.f_boolean_type[0] = insert_between_multiple_edges;
        joint.f_boolean_type[1] = insert_between_multiple_edges;
        for (int i = 0; i < number_of_tenons; i += 2)
        {
            joint.f_boolean_type[2 + i] = hole;
            joint.f_boolean_type[2 + i + 1] = hole;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // joint for preview
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        viewer_wood::scale = 1.0;
        std::vector<std::vector<CGAL_Polyline>> input_polyline_pairs0;
        input_polyline_pairs0.emplace_back(joint.m[0]);
        input_polyline_pairs0.emplace_back(joint.m[1]);
        viewer_wood::add(input_polyline_pairs0, 0); // grey
        std::vector<std::vector<CGAL_Polyline>> input_polyline_pairs1;
        input_polyline_pairs1.emplace_back(joint.f[0]);
        input_polyline_pairs1.emplace_back(joint.f[1]);
        viewer_wood::add(input_polyline_pairs1, 2); // grey
    }
}