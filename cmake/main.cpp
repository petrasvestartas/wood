#include "stdafx.h"
#include "wood_test.h" // test
#include "shapes.h"
#include "step_reader.h"
#include "session.h"
#include "wood_test_runner.h"
#include <functional>
#include <set>
namespace closest_points_joints { void run_benchmark(session_cpp::Session& session); }
namespace closest_lines_insertion { void run(session_cpp::Session& session); }
namespace wood_test { void run_suites(); }

int main(int argc, char **argv)
{

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Display
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	wood::GLOBALS::DISTANCE = 0.1;
	wood::GLOBALS::DISTANCE_SQUARED = 0.01;
	wood::GLOBALS::ANGLE = 0.11;
	wood::GLOBALS::OUTPUT_GEOMETRY_TYPE = 3;

	wood::GLOBALS::DATA_SET_INPUT_FOLDER = std::filesystem::current_path().parent_path().string() + "/src/wood/dataset/";
	wood::GLOBALS::DATA_SET_OUTPUT_FILE = wood::GLOBALS::DATA_SET_INPUT_FOLDER + "out.xml";
	wood::GLOBALS::DATA_SET_OUTPUT_DATABASE = wood::GLOBALS::DATA_SET_INPUT_FOLDER + "out.db";

	wood::GLOBALS::DATA_SET_OUTPUT_DATABASE = std::filesystem::current_path().parent_path().parent_path().parent_path().string() +"/database_viewer/cmake/src/viewer/database/database_viewer.db";

	wood::GLOBALS::OUTPUT_GEOMETRY_TYPE = 3;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Dataset selection — argv[1] chooses the test (default: "hilti")
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	using TestFn = std::function<bool()>;
	std::map<std::string, TestFn> tests = {
		// side-to-side in-plane
		{"hexbox_and_corner",                                              wood::test::type_plates_name_hexbox_and_corner},
		{"joint_linking_vidychapel_corner",                                wood::test::type_plates_name_joint_linking_vidychapel_corner},
		{"joint_linking_vidychapel_one_layer",                             wood::test::type_plates_name_joint_linking_vidychapel_one_layer},
		{"joint_linking_vidychapel_one_axis_two_layers",                   wood::test::type_plates_name_joint_linking_vidychapel_one_axis_two_layers},
		{"joint_linking_vidychapel_full",                                  wood::test::type_plates_name_joint_linking_vidychapel_full},
		{"side_to_side_edge_inplane_2_butterflies",                        wood::test::type_plates_name_side_to_side_edge_inplane_2_butterflies},
		{"side_to_side_edge_inplane_hexshell",                             wood::test::type_plates_name_side_to_side_edge_inplane_hexshell},
		{"side_to_side_edge_inplane_differentdirections",                  wood::test::type_plates_name_side_to_side_edge_inplane_differentdirections},
		// side-to-side out-of-plane
		{"side_to_side_edge_outofplane_folding",                           wood::test::type_plates_name_side_to_side_edge_outofplane_folding},
		{"side_to_side_edge_outofplane_box",                               wood::test::type_plates_name_side_to_side_edge_outofplane_box},
		{"side_to_side_edge_outofplane_tetra",                             wood::test::type_plates_name_side_to_side_edge_outofplane_tetra},
		{"side_to_side_edge_outofplane_dodecahedron",                      wood::test::type_plates_name_side_to_side_edge_outofplane_dodecahedron},
		{"side_to_side_edge_outofplane_icosahedron",                       wood::test::type_plates_name_side_to_side_edge_outofplane_icosahedron},
		{"side_to_side_edge_outofplane_octahedron",                        wood::test::type_plates_name_side_to_side_edge_outofplane_octahedron},
		// combined in/out-of-plane corners
		{"side_to_side_edge_inplane_outofplane_simple_corners",            wood::test::type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners},
		{"side_to_side_edge_inplane_outofplane_simple_corners_combined",   wood::test::type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_combined},
		{"side_to_side_edge_inplane_outofplane_simple_corners_diff_len",   wood::test::type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths},
		// hilti
		{"side_to_side_edge_inplane_hilti",                                wood::test::type_plates_name_side_to_side_edge_inplane_hilti},
		{"hilti",                                                           wood::test::type_plates_name_side_to_side_edge_inplane_hilti},
		// top-to-top
		{"top_to_top_pairs",                                               wood::test::type_plates_name_top_to_top_pairs},
		{"side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes",   wood::test::type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes},
		{"hex_block_rossiniere",                                            wood::test::type_plates_name_hex_block_rossiniere},
		// top-to-side
		{"top_to_side_snap_fit",                                           wood::test::type_plates_name_top_to_side_snap_fit},
		{"top_to_side_box",                                                wood::test::type_plates_name_top_to_side_box},
		{"top_to_side_corners",                                            wood::test::type_plates_name_top_to_side_corners},
		// annen
		{"top_to_side_outofplane_annen_corner",                            wood::test::type_plates_name_top_to_side_and_side_to_side_outofplane_annen_corner},
		{"top_to_side_outofplane_annen_box",                               wood::test::type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box},
		{"top_to_side_outofplane_annen_box_pair",                          wood::test::type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box_pair},
		{"top_to_side_outofplane_annen_grid_small",                        wood::test::type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_small},
		{"top_to_side_outofplane_annen_grid_full_arch",                    wood::test::type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_full_arch},
		// vda
		{"vda_floor_0",                                                    wood::test::type_plates_name_vda_floor_0},
		{"vda_floor_2",                                                    wood::test::type_plates_name_vda_floor_2},
		// cross
		{"cross_and_sides_corner",                                         wood::test::type_plates_name_cross_and_sides_corner},
		{"cross_corners",                                                   wood::test::type_plates_name_cross_corners},
		{"cross_vda_corner",                                               wood::test::type_plates_name_cross_vda_corner},
		{"cross_vda_hexshell",                                             wood::test::type_plates_name_cross_vda_hexshell},
		{"cross_vda_hexshell_reciprocal",                                  wood::test::type_plates_name_cross_vda_hexshell_reciprocal},
		{"cross_vda_single_arch",                                          wood::test::type_plates_name_cross_vda_single_arch},
		{"cross_vda_shell",                                                wood::test::type_plates_name_cross_vda_shell},
		{"cross_square_reciprocal_two_sides",                              wood::test::type_plates_name_cross_square_reciprocal_two_sides},
		{"cross_square_reciprocal_iseya",                                  wood::test::type_plates_name_cross_square_reciprocal_iseya},
		{"cross_ibois_pavilion",                                           wood::test::type_plates_name_cross_ibois_pavilion},
		{"cross_brussels_sports_tower",                                    wood::test::type_plates_name_cross_brussels_sports_tower},
		// beams
		{"beams_phanomema_node",                                           wood::test::type_beams_name_phanomema_node},
	};

	// Parse dataset name from argv[1], or default to "hilti"
	std::string test_name = (argc > 1) ? argv[1] : "hilti";

	auto it = tests.find(test_name);
	if (it == tests.end()) {
		std::cerr << "[wood] Unknown dataset: \"" << test_name << "\"\n";
		std::cerr << "[wood] Available datasets:\n";
		for (auto& [name, _] : tests)
			std::cerr << "  " << name << "\n";
		return 1;
	}

	// search_type per dataset: 0=face_to_face, 1=plane_to_face, 2=both
	// Cross joint tests require search_type=1 or 2 (plane-to-face detection).
	// Beam tests use a completely different pipeline (beam_volumes) — skip get_connection_zones.
	static const std::map<std::string, int> test_search_types = {
		{"cross_and_sides_corner",          2},
		{"cross_corners",                   1},
		{"cross_vda_corner",                1},
		{"cross_vda_hexshell",              1},
		{"cross_vda_hexshell_reciprocal",   1},
		{"cross_vda_single_arch",           1},
		{"cross_vda_shell",                 1},
		{"cross_square_reciprocal_two_sides", 1},
		{"cross_square_reciprocal_iseya",   1},
		{"cross_ibois_pavilion",            2},
		{"cross_brussels_sports_tower",     1},
	};
	static const std::set<std::string> beam_tests = {
		"beams_phanomema_node",
	};
	int search_type = 0;
	{
		auto sit = test_search_types.find(test_name);
		if (sit != test_search_types.end()) search_type = sit->second;
	}
	bool is_beam_test = beam_tests.count(test_name) > 0;

	std::cerr << "[wood] Running dataset: " << test_name << "\n";
	it->second();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Session_cpp — wood joints → protobuf
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{
		std::string data_dir = std::filesystem::current_path().parent_path().string() + "/data/";
		std::filesystem::create_directories(data_dir);

		// path_and_file_for_input_polylines was set by the test function above.
		// Try the full reader first (loads insertion_vectors, joint_types, adjacency etc.
		// for datasets that embed them).  If it returns nothing fall back to the
		// simple reader used by basic plate datasets.
		std::vector<Polyline> input_plines;
		std::vector<std::vector<session_cpp::Vector>> ins_vecs;
		std::vector<std::vector<int>> joint_types, three_val;
		std::vector<int> adjacency;
		{
			std::vector<std::vector<session_cpp::Point>> tmp;
			wood::xml::read_xml_polylines_and_properties(tmp, ins_vecs, joint_types, three_val, adjacency, false, false);
			input_plines.reserve(tmp.size());
			for (auto& row : tmp) {
				Polyline pl;
				pl.reserve(row.size());
				for (auto& p : row) pl.push_back(p);
				input_plines.push_back(std::move(pl));
			}
			if (input_plines.empty()) {
				// Fallback: simple XML format (no embedded properties).
				// Use remove_duplicates=true to avoid degenerate geometry that can
				// cause "Bad optional access" in CGAL inside get_elements.
				ins_vecs.clear(); joint_types.clear(); three_val.clear(); adjacency.clear();
				wood::xml::read_xml_polylines(input_plines, false, true);
			}
		}
		std::cerr << "[" << test_name << "] loaded " << input_plines.size() << " input polylines\n";
		std::vector<double> scale = {1, 1, 1};
		std::vector<std::vector<Polyline>> out_plines;
		std::vector<std::vector<wood::cut::cut_type>> out_types;
		std::vector<std::vector<int>> top_tris;

		if (!is_beam_test) {
			wood::main::get_connection_zones(
				input_plines, ins_vecs, joint_types, three_val, adjacency,
				out_plines, out_types, top_tris,
				wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES, scale, search_type,
				wood::GLOBALS::OUTPUT_GEOMETRY_TYPE, 0);
		}

		size_t total_joint_plines = 0;
		for (auto& g : out_plines) total_joint_plines += g.size();
		std::cerr << "[" << test_name << "] out_plines groups=" << out_plines.size()
		          << "  total polylines=" << total_joint_plines << "\n";

		session_cpp::Session session;

		auto grp_in = session.add_group(test_name + "_input");
		for (auto& raw : input_plines) {
			session_cpp::Polyline pl;
			for (auto& pt : raw) pl.add_point(pt);
			session.add(session.add_polyline(std::make_shared<session_cpp::Polyline>(std::move(pl))), grp_in);
		}

		auto grp_out = session.add_group(test_name + "_joints");
		for (auto& joint_group : out_plines)
			for (auto& raw : joint_group) {
				session_cpp::Polyline pl;
				for (auto& pt : raw) pl.add_point(pt);
				session.add(session.add_polyline(std::make_shared<session_cpp::Polyline>(std::move(pl))), grp_out);
			}

		session.pb_dump(data_dir + "session.pb");
		std::cerr << "[session] written to " << data_dir << "session.pb\n";
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// closest_points_joints benchmark (only for hilti — small dataset)
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (test_name == "hilti" || test_name == "side_to_side_edge_inplane_hilti") {
		session_cpp::Session s;
		closest_points_joints::run_benchmark(s);
		closest_points_joints::run_benchmark(s);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Wood minitest — run suites and write testData.js for the Vue viewer
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{
		wood_test::run_suites();

		auto find_wood_test_pub = [&]() -> std::filesystem::path {
			std::vector<std::filesystem::path> roots = {
				std::filesystem::current_path(),
				std::filesystem::current_path().parent_path(),
				std::filesystem::current_path().parent_path().parent_path(),
				std::filesystem::path(argv[0]).parent_path(),
				std::filesystem::path(argv[0]).parent_path().parent_path(),
				std::filesystem::path(argv[0]).parent_path().parent_path().parent_path(),
			};
			for (auto& root : roots) {
				auto candidate = (root / "wood_test/public").lexically_normal();
				if (std::filesystem::is_directory(candidate)) {
					return candidate / "testData.js";
				}
			}
			return std::filesystem::current_path().parent_path() / "wood_test/public/testData.js";
		};

		auto js_path = find_wood_test_pub();
		std::cerr << "[wood_test] Writing testData.js to: " << js_path << std::endl;
		wood_test::dump_testdata_js(js_path.string());
	}

	return 0;
}
