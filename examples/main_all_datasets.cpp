#include "wood_session.h"

int main() {

    int failures = 0;
    failures += !type_plates_name_hexbox_and_corner();
    failures += !type_plates_name_joint_linking_vidychapel_corner();
    failures += !type_plates_name_joint_linking_vidychapel_one_layer();
    failures += !type_plates_name_joint_linking_vidychapel_one_axis_two_layers();
    failures += !type_plates_name_joint_linking_vidychapel_full();
    failures += !type_plates_name_side_to_side_edge_inplane_2_butterflies();
    failures += !type_plates_name_side_to_side_edge_inplane_hexshell();
    failures += !type_plates_name_side_to_side_edge_inplane_differentdirections();
    failures += !type_plates_name_side_to_side_edge_outofplane_folding();
    failures += !type_plates_name_side_to_side_edge_outofplane_box();
    failures += !type_plates_name_side_to_side_edge_outofplane_box_miter();
    failures += !type_plates_name_side_to_side_edge_outofplane_tetra();
    failures += !type_plates_name_side_to_side_edge_outofplane_dodecahedron();
    failures += !type_plates_name_side_to_side_edge_outofplane_icosahedron();
    failures += !type_plates_name_side_to_side_edge_outofplane_octahedron();
    failures += !type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners();
    failures += !type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_combined();
    failures += !type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths();
    failures += !type_plates_name_side_to_side_edge_inplane_hilti();
    failures += !type_plates_name_top_to_top_pairs();
    failures += !type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes();
    failures += !type_plates_name_hex_block_rossiniere();
    failures += !type_plates_name_top_to_side_snap_fit();
    failures += !type_plates_name_top_to_side_box();
    failures += !type_plates_name_top_to_side_corners();
    failures += !type_plates_name_top_to_side_and_side_to_side_outofplane_annen_corner();
    failures += !type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box();
    failures += !type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box_pair();
    failures += !type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_small();
    failures += !type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_full_arch();
    failures += !type_plates_name_vda_floor_0();
    failures += !type_plates_name_vda_floor_2();
    failures += !type_plates_name_cross_and_sides_corner();
    failures += !type_plates_name_cross_corners();
    failures += !type_plates_name_cross_vda_corner();
    failures += !type_plates_name_cross_vda_hexshell();
    failures += !type_plates_name_cross_vda_hexshell_reciprocal();
    failures += !type_plates_name_cross_vda_single_arch();
    failures += !type_plates_name_cross_vda_shell();
    failures += !type_plates_name_cross_square_reciprocal_two_sides();
    failures += !type_plates_name_cross_square_reciprocal_iseya();
    failures += !type_plates_name_cross_ibois_pavilion();
    failures += !type_plates_name_cross_brussels_sports_tower();
    failures += !type_beams_name_phanomema_node();

    return failures ? 1 : 0;
}

/*
description: run every wood dataset -> each writes data/output/WoodF2F_<name>.pb.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_all_datasets --parallel 4 && tools/run_guarded.sh -- build/main_all_datasets
cloudflare: ../bash/publish-scene.sh data/output/WoodF2F_annen_corner.pb
view: https://petrasvestartas.github.io/session/
*/
