#pragma once

#include "pch.h"

// ═══════════════════════════════════════════════════════════════════════════
// Datasets as tests
// ═══════════════════════════════════════════════════════════════════════════

/// data/hexbox_and_corner.yml through run_dataset; false on failure.
bool type_plates_name_hexbox_and_corner();

/// data/vidy_corner.yml through run_dataset; false on failure.
bool type_plates_name_joint_linking_vidychapel_corner();

/// data/vidy_one_layer.yml through run_dataset; false on failure.
bool type_plates_name_joint_linking_vidychapel_one_layer();

/// data/vidy_one_axis_two_layers.yml through run_dataset; false on failure.
bool type_plates_name_joint_linking_vidychapel_one_axis_two_layers();

/// data/vidy_full.yml through run_dataset; false on failure.
bool type_plates_name_joint_linking_vidychapel_full();

/// data/inplane_butterflies.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_inplane_2_butterflies();

/// data/inplane_hexshell.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_inplane_hexshell();

/// data/inplane_differentdirections.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_inplane_differentdirections();

/// data/vidy_folding.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_outofplane_folding();

/// data/outofplane_box.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_outofplane_box();

/// data/outofplane_box_miter.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_outofplane_box_miter();

/// data/outofplane_tetra.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_outofplane_tetra();

/// data/outofplane_dodecahedron.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_outofplane_dodecahedron();

/// data/outofplane_icosahedron.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_outofplane_icosahedron();

/// data/outofplane_octahedron.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_outofplane_octahedron();

/// data/simple_corners.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners();

/// data/simple_corners_combined.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_combined();

/// data/simple_corners_diff_lengths.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths();

/// data/inplane_hilti.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_inplane_hilti();

/// data/top_to_top_pairs.yml through run_dataset; false on failure.
bool type_plates_name_top_to_top_pairs();

/// data/hexboxes.yml through run_dataset; false on failure.
bool type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes();

/// data/hex_block_rossiniere.yml through run_dataset; false on failure.
bool type_plates_name_hex_block_rossiniere();

/// data/top_to_side_snap_fit.yml through run_dataset; false on failure.
bool type_plates_name_top_to_side_snap_fit();

/// data/top_to_side_box.yml through run_dataset; false on failure.
bool type_plates_name_top_to_side_box();

/// data/top_to_side_corners.yml through run_dataset; false on failure.
bool type_plates_name_top_to_side_corners();

/// data/annen_corner.yml through run_dataset; false on failure.
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_corner();

/// data/annen_box.yml through run_dataset; false on failure.
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box();

/// data/annen_box_pair.yml through run_dataset; false on failure.
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box_pair();

/// data/annen_grid_small.yml through run_dataset; false on failure.
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_small();

/// data/annen_grid_full_arch.yml through run_dataset; false on failure.
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_full_arch();

/// data/vda_floor_0.yml through run_dataset; false on failure.
bool type_plates_name_vda_floor_0();

/// data/vda_floor_2.yml through run_dataset; false on failure.
bool type_plates_name_vda_floor_2();

/// data/cross_and_sides_corner.yml through run_dataset; false on failure.
bool type_plates_name_cross_and_sides_corner();

/// data/cross_corners.yml through run_dataset; false on failure.
bool type_plates_name_cross_corners();

/// data/cross_vda_corner.yml through run_dataset; false on failure.
bool type_plates_name_cross_vda_corner();

/// data/cross_vda_hexshell.yml through run_dataset; false on failure.
bool type_plates_name_cross_vda_hexshell();

/// data/cross_vda_hexshell_reciprocal.yml through run_dataset; false on failure.
bool type_plates_name_cross_vda_hexshell_reciprocal();

/// data/cross_vda_single_arch.yml through run_dataset; false on failure.
bool type_plates_name_cross_vda_single_arch();

/// data/cross_vda_shell.yml through run_dataset; false on failure.
bool type_plates_name_cross_vda_shell();

/// data/cross_square_reciprocal_two_sides.yml through run_dataset; false on failure.
bool type_plates_name_cross_square_reciprocal_two_sides();

/// data/cross_square_reciprocal_iseya.yml through run_dataset; false on failure.
bool type_plates_name_cross_square_reciprocal_iseya();

/// data/cross_ibois_pavilion.yml through run_dataset; false on failure.
bool type_plates_name_cross_ibois_pavilion();

/// data/cross_brussels_sports_tower.yml through run_dataset; false on failure.
bool type_plates_name_cross_brussels_sports_tower();

/// data/phanomema_node.yml through Beam::joint_volumes; false on failure.
bool type_beams_name_phanomema_node();
