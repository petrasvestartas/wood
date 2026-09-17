#include "wood_session.h"

#include <fmt/core.h>

using namespace session_cpp;
using wood_session::WoodSession;

/// One dataset: its yml globals, its plates, the joints, and data/output/<DATA_SET_OUTPUT_FILE> with the outline dumps beside it.
static bool run_dataset(const char* name) {
    try {
        WoodSession scene = WoodSession::yaml_load(name);
        scene.compute_joints();
        scene.write(wood_session::globals::DATA_SET_OUTPUT_FILE);
        return true;
    } catch (const std::exception& e) {
        fmt::print("  ERROR [{}]: {}\n", name, e.what());
        return false;
    }
}

bool type_plates_name_hexbox_and_corner() { return run_dataset("hexbox_and_corner"); }
bool type_plates_name_joint_linking_vidychapel_corner() { return run_dataset("vidy_corner"); }
bool type_plates_name_joint_linking_vidychapel_one_layer() { return run_dataset("vidy_one_layer"); }
bool type_plates_name_joint_linking_vidychapel_one_axis_two_layers() { return run_dataset("vidy_one_axis_two_layers"); }
bool type_plates_name_joint_linking_vidychapel_full() { return run_dataset("vidy_full"); }
bool type_plates_name_side_to_side_edge_inplane_2_butterflies() { return run_dataset("inplane_butterflies"); }
bool type_plates_name_side_to_side_edge_inplane_hexshell() { return run_dataset("inplane_hexshell"); }
bool type_plates_name_side_to_side_edge_inplane_differentdirections() { return run_dataset("inplane_differentdirections"); }
bool type_plates_name_side_to_side_edge_outofplane_folding() { return run_dataset("vidy_folding"); }
bool type_plates_name_side_to_side_edge_outofplane_box() { return run_dataset("outofplane_box"); }
bool type_plates_name_side_to_side_edge_outofplane_box_miter() { return run_dataset("outofplane_box_miter"); }
bool type_plates_name_side_to_side_edge_outofplane_tetra() { return run_dataset("outofplane_tetra"); }
bool type_plates_name_side_to_side_edge_outofplane_dodecahedron() { return run_dataset("outofplane_dodecahedron"); }
bool type_plates_name_side_to_side_edge_outofplane_icosahedron() { return run_dataset("outofplane_icosahedron"); }
bool type_plates_name_side_to_side_edge_outofplane_octahedron() { return run_dataset("outofplane_octahedron"); }
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners() { return run_dataset("simple_corners"); }
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_combined() { return run_dataset("simple_corners_combined"); }
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths() { return run_dataset("simple_corners_diff_lengths"); }
bool type_plates_name_side_to_side_edge_inplane_hilti() { return run_dataset("inplane_hilti"); }
bool type_plates_name_top_to_top_pairs() { return run_dataset("top_to_top_pairs"); }
bool type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes() { return run_dataset("hexboxes"); }
bool type_plates_name_hex_block_rossiniere() { return run_dataset("hex_block_rossiniere"); }
bool type_plates_name_top_to_side_snap_fit() { return run_dataset("top_to_side_snap_fit"); }
bool type_plates_name_top_to_side_box() { return run_dataset("top_to_side_box"); }
bool type_plates_name_top_to_side_corners() { return run_dataset("top_to_side_corners"); }
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_corner() { return run_dataset("annen_corner"); }
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box() { return run_dataset("annen_box"); }
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box_pair() { return run_dataset("annen_box_pair"); }
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_small() { return run_dataset("annen_grid_small"); }
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_full_arch() { return run_dataset("annen_grid_full_arch"); }
bool type_plates_name_vda_floor_0() { return run_dataset("vda_floor_0"); }
bool type_plates_name_vda_floor_2() { return run_dataset("vda_floor_2"); }
bool type_plates_name_cross_and_sides_corner() { return run_dataset("cross_and_sides_corner"); }
bool type_plates_name_cross_corners() { return run_dataset("cross_corners"); }
bool type_plates_name_cross_vda_corner() { return run_dataset("cross_vda_corner"); }
bool type_plates_name_cross_vda_hexshell() { return run_dataset("cross_vda_hexshell"); }
bool type_plates_name_cross_vda_hexshell_reciprocal() { return run_dataset("cross_vda_hexshell_reciprocal"); }
bool type_plates_name_cross_vda_single_arch() { return run_dataset("cross_vda_single_arch"); }
bool type_plates_name_cross_vda_shell() { return run_dataset("cross_vda_shell"); }
bool type_plates_name_cross_square_reciprocal_two_sides() { return run_dataset("cross_square_reciprocal_two_sides"); }
bool type_plates_name_cross_square_reciprocal_iseya() { return run_dataset("cross_square_reciprocal_iseya"); }
bool type_plates_name_cross_ibois_pavilion() { return run_dataset("cross_ibois_pavilion"); }
bool type_plates_name_cross_brussels_sports_tower() { return run_dataset("cross_brussels_sports_tower"); }

bool type_beams_name_phanomema_node() {
    try {
        using namespace wood_session::globals;
        if (!internal::plates_exist("phanomema_node"))
            return false;
        globals_yaml("phanomema_node");
        if (BEAMS.size() != 6)
            throw std::runtime_error("phanomema_node.yml has no beams block");
        const std::vector<Polyline> axes = internal::load_polylines("phanomema_node");
        std::vector<std::vector<double>> segment_radii;
        segment_radii.reserve(axes.size());
        for (const Polyline& ax : axes)
            segment_radii.emplace_back(ax.segment_count(), BEAMS[0]);
        const std::vector<std::vector<Vector>> segment_direction;
        const std::vector<int> allowed_types{static_cast<int>(BEAMS[1])};
        beam_volumes_pipeline(axes, segment_radii, segment_direction, allowed_types, BEAMS[2], BEAMS[3], BEAMS[4], static_cast<int>(BEAMS[5]));
        return true;
    } catch (const std::exception& e) {
        fmt::print("  ERROR [type_beams_name_phanomema_node]: {}\n", e.what());
        return false;
    }
}
