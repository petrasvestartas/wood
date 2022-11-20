#include "stdafx.h"

// temp
//#include "wood_main.h"
//#include "wood_xml.h"	 // xml parser and viewer customization
//#include "viewer_wood.h" // viewer
#include "wood_test.h" // test

int main(int argc, char **argv)
{

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Display
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	opengl_globals::shader_type_0default_1transparent_2shaded_3shadedwireframe_4wireframe_5normals_6explode = 2;
	opengl_globals::shaders_folder = "C:\\IBOIS57\\_Code\\Software\\CPP\\CMAKE\\super_build\\wood\\src\\viewer\\shaders\\";
	opengl_globals_geometry::add_grid();
	opengl_render::render(wood_test::test__function_three_valence_joint_addition_vidy__dataset_chapel); // chapel_corner// chapel_one_layer // chapel_simple // chapel
	// opengl_render::render(wood_test::ss_e_op_4);

	return 0;
}

// run ninja-> g++ main.cpp -o main
// cmake  --fresh -DGET_LIBS=OFF -DBUILD_MY_PROJECTS=ON -DRELEASE_DEBUG=ON -DCMAKE_BUILD_TYPE="Release" -G "Ninja" .. && cmake  --build . --config Release
// cmake --fresh -DGET_LIBS=OFF -DBUILD_MY_PROJECTS=ON -DRELEASE_DEBUG=ON -DCMAKE_BUILD_TYPE="Release"  -G "Visual Studio 17 2022" -A x64 .. && cmake --build . --config Release
// cmake  -E time --build . -v --config Release -- -j0
// cmake  --build . -v --config Release -- -j0
// cmake  --build . --config Release -v