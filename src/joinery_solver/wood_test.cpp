#include "wood_session.h"

#include <fmt/core.h>

using namespace session_cpp;
using wood_session::WoodSession;

/// One dataset: its yml globals, its plates, the joints, and data/output/<DATA_SET_OUTPUT_FILE> with the outline dumps beside it.
static bool run_dataset(const char* name, const SearchType search_type) {
    try {
        WoodSession scene = WoodSession::yaml_load(name);
        scene.compute_joints(search_type);
        scene.write(wood_session::globals::DATA_SET_OUTPUT_FILE);
        return true;
    } catch (const std::exception& e) {
        fmt::print("  ERROR [{}]: {}\n", name, e.what());
        return false;
    }
}

bool type_plates_name_hexbox_and_corner() { return run_dataset("hexbox_and_corner", face_to_face); }
bool type_plates_name_joint_linking_vidychapel_corner() { return run_dataset("vidy_corner", face_to_face); }
bool type_plates_name_joint_linking_vidychapel_one_layer() { return run_dataset("vidy_one_layer", face_to_face); }
bool type_plates_name_joint_linking_vidychapel_one_axis_two_layers() { return run_dataset("vidy_one_axis_two_layers", face_to_face); }
bool type_plates_name_joint_linking_vidychapel_full() { return run_dataset("vidy_full", face_to_face); }
bool type_plates_name_side_to_side_edge_inplane_2_butterflies() { return run_dataset("inplane_butterflies", face_to_face); }
bool type_plates_name_side_to_side_edge_inplane_hexshell() { return run_dataset("inplane_hexshell", face_to_face); }
bool type_plates_name_side_to_side_edge_inplane_differentdirections() { return run_dataset("inplane_differentdirections", face_to_face); }
bool type_plates_name_side_to_side_edge_outofplane_folding() { return run_dataset("vidy_folding", face_to_face); }
bool type_plates_name_side_to_side_edge_outofplane_box() { return run_dataset("outofplane_box", face_to_face); }
bool type_plates_name_side_to_side_edge_outofplane_box_miter() { return run_dataset("outofplane_box_miter", face_to_face); }
bool type_plates_name_side_to_side_edge_outofplane_tetra() { return run_dataset("outofplane_tetra", face_to_face); }
bool type_plates_name_side_to_side_edge_outofplane_dodecahedron() { return run_dataset("outofplane_dodecahedron", face_to_face); }
bool type_plates_name_side_to_side_edge_outofplane_icosahedron() { return run_dataset("outofplane_icosahedron", face_to_face); }
bool type_plates_name_side_to_side_edge_outofplane_octahedron() { return run_dataset("outofplane_octahedron", face_to_face); }
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners() { return run_dataset("simple_corners", face_to_face); }
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_combined() { return run_dataset("simple_corners_combined", face_to_face); }
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths() { return run_dataset("simple_corners_diff_lengths", face_to_face); }
bool type_plates_name_side_to_side_edge_inplane_hilti() { return run_dataset("inplane_hilti", face_to_face); }
bool type_plates_name_top_to_top_pairs() { return run_dataset("top_to_top_pairs", face_to_face); }
bool type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes() { return run_dataset("hexboxes", face_to_face); }
bool type_plates_name_hex_block_rossiniere() { return run_dataset("hex_block_rossiniere", face_to_face); }
bool type_plates_name_top_to_side_snap_fit() { return run_dataset("top_to_side_snap_fit", face_to_face); }
bool type_plates_name_top_to_side_box() { return run_dataset("top_to_side_box", face_to_face); }
bool type_plates_name_top_to_side_corners() { return run_dataset("top_to_side_corners", face_to_face); }
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_corner() { return run_dataset("annen_corner", face_to_face); }
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box() { return run_dataset("annen_box", face_to_face); }
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box_pair() { return run_dataset("annen_box_pair", face_to_face); }
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_small() { return run_dataset("annen_grid_small", face_to_face); }
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_full_arch() { return run_dataset("annen_grid_full_arch", face_to_face); }
bool type_plates_name_vda_floor_0() { return run_dataset("vda_floor_0", face_to_face); }
bool type_plates_name_vda_floor_2() { return run_dataset("vda_floor_2", face_to_face); }
bool type_plates_name_cross_and_sides_corner() { return run_dataset("cross_and_sides_corner", face_to_face_then_cross); }
bool type_plates_name_cross_corners() { return run_dataset("cross_corners", cross_joint); }
bool type_plates_name_cross_vda_corner() { return run_dataset("cross_vda_corner", cross_joint); }
bool type_plates_name_cross_vda_hexshell() { return run_dataset("cross_vda_hexshell", cross_joint); }
bool type_plates_name_cross_vda_hexshell_reciprocal() { return run_dataset("cross_vda_hexshell_reciprocal", cross_joint); }
bool type_plates_name_cross_vda_single_arch() { return run_dataset("cross_vda_single_arch", cross_joint); }
bool type_plates_name_cross_vda_shell() { return run_dataset("cross_vda_shell", cross_joint); }
bool type_plates_name_cross_square_reciprocal_two_sides() { return run_dataset("cross_square_reciprocal_two_sides", cross_joint); }
bool type_plates_name_cross_square_reciprocal_iseya() { return run_dataset("cross_square_reciprocal_iseya", cross_joint); }
bool type_plates_name_cross_ibois_pavilion() { return run_dataset("cross_ibois_pavilion", face_to_face_then_cross); }
bool type_plates_name_cross_brussels_sports_tower() { return run_dataset("cross_brussels_sports_tower", cross_joint); }

bool type_beams_name_phanomema_node() {
    try {
    using namespace wood_session::globals;
    if (!internal::plates_exist("phanomema_node")) {
        fmt::print("\n=== phanomema_node: dataset missing, skipping ===\n");
        return false;
    }
    globals_yaml("phanomema_node");
    auto axes = internal::load_polylines("phanomema_node");

    std::vector<std::vector<double>> segment_radii;
    segment_radii.reserve(axes.size());
    for (const auto& ax : axes) {
        std::vector<double> r;
        size_t nseg = ax.point_count() > 1 ? ax.point_count() - 1 : 0;
        r.assign(nseg, 150.0);
        segment_radii.push_back(std::move(r));
    }
    std::vector<std::vector<Vector>> segment_direction;

    std::vector<int> allowed_types{ 1 };
    double min_distance         = 20.0;
    double volume_length        = 500.0;
    double cross_or_side_to_end = 0.91;
    int    flip_male            = 1;

    beam_volumes_pipeline(
        axes, segment_radii, segment_direction,
        allowed_types,
        min_distance, volume_length, cross_or_side_to_end, flip_male);
    return true;
    } catch (const std::exception& e) {
        fmt::print("  ERROR [type_beams_name_phanomema_node]: {}\n", e.what());
        return false;
    }
}
