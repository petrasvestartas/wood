#pragma once

#include "../src/polyline.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/// Which detection pass compute_joints runs.
enum SearchType : int {
    face_to_face            = 0,  ///< coplanar faces: ss_e_ip / ss_e_op / ss_e_r / ts_e_p / tt_e_p
    cross_joint             = 1,  ///< elements passing through each other: plane_to_face, type 30
    face_to_face_then_cross = 2,  ///< face-to-face first, cross as the fallback
};

namespace wood_session {
namespace globals {
    /// The detection pass the dataset asks for (yml `search_type`).
    extern SearchType SEARCH_TYPE;

    /// Joint-family triples [division_length (mm), shift, joint_type_id]; families 0=ss_e_ip 1=ss_e_op 2=ts_e_p 3=cr_c_ip 4=tt_e_p 5=ss_e_r 6=b.
    extern std::vector<double> JOINTS_PARAMETERS_AND_TYPES;

    /// Additive [width, height, length] extension (mm) of joint volumes: one triple for every joint type, or one per type
    /// (side-side, top-side, top-top, cross); width and height grow the volume, length the joint line; unit-scale joints
    /// (ss_e_ip_2, ss_e_r_*, ts_e_p_5) keep their axial size at the plate thickness.
    extern std::vector<double> JOINT_VOLUME_EXTENSION;

    /// Multiplicative [sx, sy, sz] scale of joint geometry before insertion (ss_e_ip_2, ss_e_r_*, ts_e_p_5); 1 = no change.
    extern std::array<double, 3> JOINT_SCALE;
    extern double FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE;       ///< degrees; rotated-joint threshold
    extern bool   FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED;///< force rotated geometry path
    extern bool   FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE;///< averaged plane for rotated joints

    extern double DISTANCE;                                  ///< inflate AABBs / point-merge tolerance (mm)
    extern double DISTANCE_SQUARED;                          ///< squared coplanarity tolerance (mm²)
    extern double ANGLE;                                     ///< angular tolerance, RADIANS (cos-tolerance)
    extern double DUPLICATE_PTS_TOL;                         ///< consecutive-duplicate-points removal in load_plates
    extern double LIMIT_MIN_JOINT_LENGTH;                    ///< filters out joints whose centerline is shorter

    extern int64_t CLIPPER_SCALE;                            ///< mm -> int64 scale for the 2D boolean (1e6 = nanometre grid)
    extern double  CLIPPER_AREA;                             ///< overlap areas at or below this (mm²) are not a contact

    /// The data folder every yml, obj, txt and pb is named relative to; absolute, baked from __FILE__, settable from a binding.
    extern std::string DATA_SET_INPUT_FOLDER;
    extern const std::vector<std::string> DATASET_NAMES;     ///< every dataset shipped in data/ as <name>.yml, in sweep order
    extern const std::vector<std::string> SESSION_NAMES;     ///< every session shipped in data/ as <name>.pb

    /// Named access to every string in DATASET_NAMES, same strings and sweep order; kept in sync by dataset_names_test.cpp.
    struct Dataset {
        static constexpr const char* hexbox_and_corner = "hexbox_and_corner";
        static constexpr const char* vidy_corner = "vidy_corner";
        static constexpr const char* vidy_one_layer = "vidy_one_layer";
        static constexpr const char* vidy_one_axis_two_layers = "vidy_one_axis_two_layers";
        static constexpr const char* vidy_full = "vidy_full";
        static constexpr const char* inplane_butterflies = "inplane_butterflies";
        static constexpr const char* inplane_hexshell = "inplane_hexshell";
        static constexpr const char* inplane_differentdirections = "inplane_differentdirections";
        static constexpr const char* vidy_folding = "vidy_folding";
        static constexpr const char* outofplane_box = "outofplane_box";
        static constexpr const char* outofplane_box_miter = "outofplane_box_miter";
        static constexpr const char* outofplane_tetra = "outofplane_tetra";
        static constexpr const char* outofplane_dodecahedron = "outofplane_dodecahedron";
        static constexpr const char* outofplane_icosahedron = "outofplane_icosahedron";
        static constexpr const char* outofplane_octahedron = "outofplane_octahedron";
        static constexpr const char* simple_corners = "simple_corners";
        static constexpr const char* simple_corners_combined = "simple_corners_combined";
        static constexpr const char* simple_corners_diff_lengths = "simple_corners_diff_lengths";
        static constexpr const char* inplane_hilti = "inplane_hilti";
        static constexpr const char* top_to_top_pairs = "top_to_top_pairs";
        static constexpr const char* hexboxes = "hexboxes";
        static constexpr const char* hex_block_rossiniere = "hex_block_rossiniere";
        static constexpr const char* top_to_side_snap_fit = "top_to_side_snap_fit";
        static constexpr const char* top_to_side_box = "top_to_side_box";
        static constexpr const char* top_to_side_corners = "top_to_side_corners";
        static constexpr const char* annen_corner = "annen_corner";
        static constexpr const char* annen_box = "annen_box";
        static constexpr const char* annen_box_pair = "annen_box_pair";
        static constexpr const char* annen_grid_small = "annen_grid_small";
        static constexpr const char* annen_grid_full_arch = "annen_grid_full_arch";
        static constexpr const char* vda_floor_0 = "vda_floor_0";
        static constexpr const char* vda_floor_2 = "vda_floor_2";
        static constexpr const char* cross_and_sides_corner = "cross_and_sides_corner";
        static constexpr const char* cross_corners = "cross_corners";
        static constexpr const char* cross_vda_corner = "cross_vda_corner";
        static constexpr const char* cross_vda_hexshell = "cross_vda_hexshell";
        static constexpr const char* cross_vda_hexshell_reciprocal = "cross_vda_hexshell_reciprocal";
        static constexpr const char* cross_vda_single_arch = "cross_vda_single_arch";
        static constexpr const char* cross_vda_shell = "cross_vda_shell";
        static constexpr const char* cross_square_reciprocal_two_sides = "cross_square_reciprocal_two_sides";
        static constexpr const char* cross_square_reciprocal_iseya = "cross_square_reciprocal_iseya";
        static constexpr const char* cross_ibois_pavilion = "cross_ibois_pavilion";
        static constexpr const char* cross_brussels_sports_tower = "cross_brussels_sports_tower";
        static constexpr const char* phanomema_node = "phanomema_node";               ///< beam axes, not plates - use beam_volumes_pipeline
        static constexpr const char* hello = "hello";
        static constexpr const char* top_to_side_test = "top_to_side_test";
        static constexpr const char* vda_floor_1 = "vda_floor_1";
        static constexpr const char* cross_brg_slab_0 = "cross_brg_slab_0";

        /// Ordinary plate outlines: compute_face_contacts / compute_joints(face_to_face).
        struct Face {
            static constexpr const char* hexbox_and_corner = Dataset::hexbox_and_corner;
            static constexpr const char* vidy_corner = Dataset::vidy_corner;
            static constexpr const char* vidy_one_layer = Dataset::vidy_one_layer;
            static constexpr const char* vidy_one_axis_two_layers = Dataset::vidy_one_axis_two_layers;
            static constexpr const char* vidy_full = Dataset::vidy_full;
            static constexpr const char* inplane_butterflies = Dataset::inplane_butterflies;
            static constexpr const char* inplane_hexshell = Dataset::inplane_hexshell;
            static constexpr const char* inplane_differentdirections = Dataset::inplane_differentdirections;
            static constexpr const char* vidy_folding = Dataset::vidy_folding;
            static constexpr const char* outofplane_box = Dataset::outofplane_box;
            static constexpr const char* outofplane_box_miter = Dataset::outofplane_box_miter;
            static constexpr const char* outofplane_tetra = Dataset::outofplane_tetra;
            static constexpr const char* outofplane_dodecahedron = Dataset::outofplane_dodecahedron;
            static constexpr const char* outofplane_icosahedron = Dataset::outofplane_icosahedron;
            static constexpr const char* outofplane_octahedron = Dataset::outofplane_octahedron;
            static constexpr const char* simple_corners = Dataset::simple_corners;
            static constexpr const char* simple_corners_combined = Dataset::simple_corners_combined;
            static constexpr const char* simple_corners_diff_lengths = Dataset::simple_corners_diff_lengths;
            static constexpr const char* inplane_hilti = Dataset::inplane_hilti;
            static constexpr const char* top_to_top_pairs = Dataset::top_to_top_pairs;
            static constexpr const char* hexboxes = Dataset::hexboxes;
            static constexpr const char* hex_block_rossiniere = Dataset::hex_block_rossiniere;
            static constexpr const char* top_to_side_snap_fit = Dataset::top_to_side_snap_fit;
            static constexpr const char* top_to_side_box = Dataset::top_to_side_box;
            static constexpr const char* top_to_side_corners = Dataset::top_to_side_corners;
            static constexpr const char* annen_corner = Dataset::annen_corner;
            static constexpr const char* annen_box = Dataset::annen_box;
            static constexpr const char* annen_box_pair = Dataset::annen_box_pair;
            static constexpr const char* annen_grid_small = Dataset::annen_grid_small;
            static constexpr const char* annen_grid_full_arch = Dataset::annen_grid_full_arch;
            static constexpr const char* vda_floor_0 = Dataset::vda_floor_0;
            static constexpr const char* vda_floor_2 = Dataset::vda_floor_2;
            static constexpr const char* hello = Dataset::hello;
            static constexpr const char* top_to_side_test = Dataset::top_to_side_test;
            static constexpr const char* vda_floor_1 = Dataset::vda_floor_1;
        };

        /// Plate outlines whose solved joints include type-30 crossings: compute_cross_contacts / compute_joints(cross_joint).
        struct Cross {
            static constexpr const char* cross_and_sides_corner = Dataset::cross_and_sides_corner;
            static constexpr const char* cross_corners = Dataset::cross_corners;
            static constexpr const char* cross_vda_corner = Dataset::cross_vda_corner;
            static constexpr const char* cross_vda_hexshell = Dataset::cross_vda_hexshell;
            static constexpr const char* cross_vda_hexshell_reciprocal = Dataset::cross_vda_hexshell_reciprocal;
            static constexpr const char* cross_vda_single_arch = Dataset::cross_vda_single_arch;
            static constexpr const char* cross_vda_shell = Dataset::cross_vda_shell;
            static constexpr const char* cross_square_reciprocal_two_sides = Dataset::cross_square_reciprocal_two_sides;
            static constexpr const char* cross_square_reciprocal_iseya = Dataset::cross_square_reciprocal_iseya;
            static constexpr const char* cross_ibois_pavilion = Dataset::cross_ibois_pavilion;
            static constexpr const char* cross_brussels_sports_tower = Dataset::cross_brussels_sports_tower;
            static constexpr const char* cross_brg_slab_0 = Dataset::cross_brg_slab_0;
        };

        /// Beam axes, not plate outlines (type_beams_name_*): compute_line_contacts / beam_volumes_pipeline, never load_plates.
        struct Curves {
            static constexpr const char* phanomema_node = Dataset::phanomema_node;
        };
    };

    /// DATASET_NAMES.at(index): a stray index throws std::out_of_range instead of reading past the end.
    const std::string& dataset_name(size_t index);

    /// data/<SESSION_NAMES[index]>.pb for Session::pb_load; out of range throws.
    std::string session_pb(size_t index);
    extern std::string DATA_SET_INPUT_NAME;                  ///< dataset name: the yml stem
    extern std::string DATA_SET_OBJ;                         ///< obj path named by the dataset yaml
    extern std::string DATA_SET_ADJACENCY;                   ///< adjacency txt path from the yaml, empty when absent
    extern std::string DATA_SET_THREE_VALENCE;               ///< three-valence txt path from the yaml, empty when absent
    extern std::string DATA_SET_INSERTION_VECTORS;           ///< insertion-vectors txt path from the yaml, empty when absent
    extern std::string DATA_SET_JOINTS_TYPES;                ///< joint-types txt path from the yaml, empty when absent
    extern std::string DATA_SET_OUTPUT_FILE;                 ///< WoodF2F_<yml stem>.pb, written into data/output/


    /// Custom joint polylines set at runtime, pairs (i, i+1) = (male, female) per variant; the yaml loader skips them.
    /// Beam datasets (yml `beams`): [radius, allowed joint type, min_distance, volume_length, cross_or_side_to_end, flip_male].
    extern std::vector<double> BEAMS;

    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_IP_MALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_IP_FEMALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_OP_MALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_OP_FEMALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_TS_E_P_MALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_TS_E_P_FEMALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_CR_C_IP_MALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_CR_C_IP_FEMALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_TT_E_P_MALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_TT_E_P_FEMALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_R_MALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_R_FEMALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_B_MALE;
    extern std::vector<session_cpp::Polyline> CUSTOM_JOINTS_B_FEMALE;

    /// Reset every global above to the wood baseline values.
    void reset_defaults();

    /// Load `data/<name>.yml` (or the given .yml path) and apply every key to the globals above.
    void globals_yaml(const std::string& dataset_name);
}} // namespace wood_session::globals
