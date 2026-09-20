#pragma once

#include "pch.h"


#include "wood_settings.h"

namespace wood_session {
namespace config {
    /// The data folder every yml, obj, txt and pb is named relative to; absolute, baked from __FILE__, settable from a binding.
    extern std::string DATA_SET_INPUT_FOLDER;

    /// Every dataset shipped in data/ as <name>.yml, in sweep order.
    extern const std::vector<std::string> DATASET_NAMES;

    /// Every session shipped in data/ as <name>.pb.
    extern const std::vector<std::string> SESSION_NAMES;

    /// Named access to every string in DATASET_NAMES, same strings and sweep order; kept in sync by dataset_names_test.cpp.
    struct Dataset {
        static constexpr std::string_view hexbox_and_corner = "hexbox_and_corner";
        static constexpr std::string_view vidy_corner = "vidy_corner";
        static constexpr std::string_view vidy_one_layer = "vidy_one_layer";
        static constexpr std::string_view vidy_one_axis_two_layers = "vidy_one_axis_two_layers";
        static constexpr std::string_view vidy_full = "vidy_full";
        static constexpr std::string_view inplane_butterflies = "inplane_butterflies";
        static constexpr std::string_view inplane_hexshell = "inplane_hexshell";
        static constexpr std::string_view inplane_differentdirections = "inplane_differentdirections";
        static constexpr std::string_view vidy_folding = "vidy_folding";
        static constexpr std::string_view outofplane_box = "outofplane_box";
        static constexpr std::string_view outofplane_box_miter = "outofplane_box_miter";
        static constexpr std::string_view outofplane_tetra = "outofplane_tetra";
        static constexpr std::string_view outofplane_dodecahedron = "outofplane_dodecahedron";
        static constexpr std::string_view outofplane_icosahedron = "outofplane_icosahedron";
        static constexpr std::string_view outofplane_octahedron = "outofplane_octahedron";
        static constexpr std::string_view simple_corners = "simple_corners";
        static constexpr std::string_view simple_corners_combined = "simple_corners_combined";
        static constexpr std::string_view simple_corners_diff_lengths = "simple_corners_diff_lengths";
        static constexpr std::string_view inplane_hilti = "inplane_hilti";
        static constexpr std::string_view top_to_top_pairs = "top_to_top_pairs";
        static constexpr std::string_view hexboxes = "hexboxes";
        static constexpr std::string_view hex_block_rossiniere = "hex_block_rossiniere";
        static constexpr std::string_view top_to_side_snap_fit = "top_to_side_snap_fit";
        static constexpr std::string_view top_to_side_box = "top_to_side_box";
        static constexpr std::string_view top_to_side_corners = "top_to_side_corners";
        static constexpr std::string_view annen_corner = "annen_corner";
        static constexpr std::string_view annen_box = "annen_box";
        static constexpr std::string_view annen_box_pair = "annen_box_pair";
        static constexpr std::string_view annen_grid_small = "annen_grid_small";
        static constexpr std::string_view annen_grid_full_arch = "annen_grid_full_arch";
        static constexpr std::string_view vda_floor_0 = "vda_floor_0";
        static constexpr std::string_view vda_floor_2 = "vda_floor_2";
        static constexpr std::string_view cross_and_sides_corner = "cross_and_sides_corner";
        static constexpr std::string_view cross_corners = "cross_corners";
        static constexpr std::string_view cross_vda_corner = "cross_vda_corner";
        static constexpr std::string_view cross_vda_hexshell = "cross_vda_hexshell";
        static constexpr std::string_view cross_vda_hexshell_reciprocal = "cross_vda_hexshell_reciprocal";
        static constexpr std::string_view cross_vda_single_arch = "cross_vda_single_arch";
        static constexpr std::string_view cross_vda_shell = "cross_vda_shell";
        static constexpr std::string_view cross_square_reciprocal_two_sides = "cross_square_reciprocal_two_sides";
        static constexpr std::string_view cross_square_reciprocal_iseya = "cross_square_reciprocal_iseya";
        static constexpr std::string_view cross_ibois_pavilion = "cross_ibois_pavilion";
        static constexpr std::string_view cross_brussels_sports_tower = "cross_brussels_sports_tower";
        static constexpr std::string_view phanomema_node = "phanomema_node";
        static constexpr std::string_view hello = "hello";
        static constexpr std::string_view top_to_side_test = "top_to_side_test";
        static constexpr std::string_view vda_floor_1 = "vda_floor_1";
        static constexpr std::string_view cross_brg_slab_0 = "cross_brg_slab_0";

        /// Ordinary plate outlines: compute_face_contacts / compute_features(face_to_face).
        struct Face {
            static constexpr std::string_view hexbox_and_corner = Dataset::hexbox_and_corner;
            static constexpr std::string_view vidy_corner = Dataset::vidy_corner;
            static constexpr std::string_view vidy_one_layer = Dataset::vidy_one_layer;
            static constexpr std::string_view vidy_one_axis_two_layers = Dataset::vidy_one_axis_two_layers;
            static constexpr std::string_view vidy_full = Dataset::vidy_full;
            static constexpr std::string_view inplane_butterflies = Dataset::inplane_butterflies;
            static constexpr std::string_view inplane_hexshell = Dataset::inplane_hexshell;
            static constexpr std::string_view inplane_differentdirections = Dataset::inplane_differentdirections;
            static constexpr std::string_view vidy_folding = Dataset::vidy_folding;
            static constexpr std::string_view outofplane_box = Dataset::outofplane_box;
            static constexpr std::string_view outofplane_box_miter = Dataset::outofplane_box_miter;
            static constexpr std::string_view outofplane_tetra = Dataset::outofplane_tetra;
            static constexpr std::string_view outofplane_dodecahedron = Dataset::outofplane_dodecahedron;
            static constexpr std::string_view outofplane_icosahedron = Dataset::outofplane_icosahedron;
            static constexpr std::string_view outofplane_octahedron = Dataset::outofplane_octahedron;
            static constexpr std::string_view simple_corners = Dataset::simple_corners;
            static constexpr std::string_view simple_corners_combined = Dataset::simple_corners_combined;
            static constexpr std::string_view simple_corners_diff_lengths = Dataset::simple_corners_diff_lengths;
            static constexpr std::string_view inplane_hilti = Dataset::inplane_hilti;
            static constexpr std::string_view top_to_top_pairs = Dataset::top_to_top_pairs;
            static constexpr std::string_view hexboxes = Dataset::hexboxes;
            static constexpr std::string_view hex_block_rossiniere = Dataset::hex_block_rossiniere;
            static constexpr std::string_view top_to_side_snap_fit = Dataset::top_to_side_snap_fit;
            static constexpr std::string_view top_to_side_box = Dataset::top_to_side_box;
            static constexpr std::string_view top_to_side_corners = Dataset::top_to_side_corners;
            static constexpr std::string_view annen_corner = Dataset::annen_corner;
            static constexpr std::string_view annen_box = Dataset::annen_box;
            static constexpr std::string_view annen_box_pair = Dataset::annen_box_pair;
            static constexpr std::string_view annen_grid_small = Dataset::annen_grid_small;
            static constexpr std::string_view annen_grid_full_arch = Dataset::annen_grid_full_arch;
            static constexpr std::string_view vda_floor_0 = Dataset::vda_floor_0;
            static constexpr std::string_view vda_floor_2 = Dataset::vda_floor_2;
            static constexpr std::string_view hello = Dataset::hello;
            static constexpr std::string_view top_to_side_test = Dataset::top_to_side_test;
            static constexpr std::string_view vda_floor_1 = Dataset::vda_floor_1;
        };

        /// Plate outlines whose solved joints include type-30 crossings: compute_cross_contacts / compute_features(cross_joint).
        struct Cross {
            static constexpr std::string_view cross_and_sides_corner = Dataset::cross_and_sides_corner;
            static constexpr std::string_view cross_corners = Dataset::cross_corners;
            static constexpr std::string_view cross_vda_corner = Dataset::cross_vda_corner;
            static constexpr std::string_view cross_vda_hexshell = Dataset::cross_vda_hexshell;
            static constexpr std::string_view cross_vda_hexshell_reciprocal = Dataset::cross_vda_hexshell_reciprocal;
            static constexpr std::string_view cross_vda_single_arch = Dataset::cross_vda_single_arch;
            static constexpr std::string_view cross_vda_shell = Dataset::cross_vda_shell;
            static constexpr std::string_view cross_square_reciprocal_two_sides = Dataset::cross_square_reciprocal_two_sides;
            static constexpr std::string_view cross_square_reciprocal_iseya = Dataset::cross_square_reciprocal_iseya;
            static constexpr std::string_view cross_ibois_pavilion = Dataset::cross_ibois_pavilion;
            static constexpr std::string_view cross_brussels_sports_tower = Dataset::cross_brussels_sports_tower;
            static constexpr std::string_view cross_brg_slab_0 = Dataset::cross_brg_slab_0;
        };

        /// Beam axes, not plate outlines (type_beams_name_*): compute_line_contacts / Beam::joint_volumes, never obj_load.
        struct Curves {
            static constexpr std::string_view phanomema_node = Dataset::phanomema_node;
        };
    };

    /// DATASET_NAMES.at(index): a stray index throws std::out_of_range instead of reading past the end.
    const std::string& dataset_name(size_t index);

    /// data/<SESSION_NAMES[index]>.pb for Session::pb_load; out of range throws.
    std::string session_pb(size_t index);

    /// Dataset name: the yml stem.
    extern std::string DATA_SET_INPUT_NAME;

    /// Obj path named by the dataset yaml.
    extern std::string DATA_SET_OBJ;

    /// Adjacency txt path from the yaml, empty when absent.
    extern std::string DATA_SET_ADJACENCY;

    /// Three-valence txt path from the yaml, empty when absent.
    extern std::string DATA_SET_THREE_VALENCE;

    /// Insertion-vectors txt path from the yaml, empty when absent.
    extern std::string DATA_SET_INSERTION_VECTORS;

    /// Joint-types txt path from the yaml, empty when absent.
    extern std::string DATA_SET_JOINTS_TYPES;

    /// WoodF2F_<yml stem>.pb, written into data/output/.
    extern std::string DATA_SET_OUTPUT_FILE;

    /// Clears every dataset path above.
    void reset_defaults();

    /// Load `data/<name>.yml` (or the given .yml path): its solver keys as Settings, its file keys into the dataset paths above.
    Settings load_yaml(const std::string& dataset_name);

    /// The dataset folder, DATA_SET_INPUT_FOLDER; absolute, so the working directory does not matter.
    std::filesystem::path session_data_dir();

    /// Absolute path to data/output/, created on first call.
    std::filesystem::path output_dir();

    /// A bare name resolves to <session_data_dir>/<name><ext>; a path already ending in ext is returned as is.
    std::filesystem::path dataset_path(const std::string& name, const std::string& ext);

    /// True iff data/<name>.obj exists.
    bool plates_exist(const std::string& name);

}} // namespace wood_session::config
