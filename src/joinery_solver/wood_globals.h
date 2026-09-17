#pragma once

#include "../src/polyline.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wood_session {
namespace globals {
    // ── Joint algorithm tunables (pipeline reads these every run) ─────────
    /// Flat array of joint-family parameters; read as consecutive triples (i*3+0, i*3+1, i*3+2):
    ///   [i*3+0] division_length — spacing between fingers/notches along the joint line (mm)
    ///   [i*3+1] shift           — lateral offset of the joint pattern (mm); 0 = centred
    ///   [i*3+2] joint_type_id   — selects the joint geometry variant (e.g. 1=zigzag, 12=ss_e_op_0)
    /// Family indices: 0 = ss_e_ip (in-plane), 1 = ss_e_op (out-of-plane), 2–6 = ts/cr/tt/b/ss_e_r families.
    extern std::vector<double> JOINTS_PARAMETERS_AND_TYPES;

    /// Additive extension of joint cut volumes (mm); positive = grow, negative = shrink.
    /// Read as consecutive triples per joint-type override; default is one shared triple (indices 0–2):
    ///   [0] width  — extends/shrinks edges 0 and 2 of the volume quad (across the plate face)
    ///   [1] height — extends/shrinks edges 1 and 3 of the volume quad (through the plate thickness)
    ///   [2] length — extends/shrinks the joint centerline (along the shared edge / fold line)
    /// To reduce the volume along the fold edge, set index [2] to a negative value, e.g. {0, 0, -5}.
    extern std::vector<double> JOINT_VOLUME_EXTENSION;

    /// Multiplicative scale applied to joint geometry before insertion; 1.0 = no change.
    ///   [0] sx — scale along joint local X (width direction)
    ///   [1] sy — scale along joint local Y (height / thickness direction)
    ///   [2] sz — scale along joint local Z (length / edge direction)
    /// Used by joint types: ss_e_ip_2, ss_e_r_*, ts_e_p_5.
    extern std::array<double, 3> JOINT_SCALE;
    extern int    OUTPUT_GEOMETRY_TYPE;                      ///< 4 = merged outlines + lofts
    extern double FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE;       ///< degrees; rotated-joint threshold
    extern bool   FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED;///< force rotated geometry path
    extern bool   FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE;///< averaged plane for rotated joints

    // ── Tolerances (heavy use across the kernel) ──────────────────────────
    extern double DISTANCE;                                  ///< inflate AABBs / point-merge tolerance (mm)
    extern double DISTANCE_SQUARED;                          ///< squared coplanarity tolerance (mm²)
    extern double ANGLE;                                     ///< angular tolerance, RADIANS (cos-tolerance)
    extern double DUPLICATE_PTS_TOL;                         ///< consecutive-duplicate-points removal in load_plates
    extern double LIMIT_MIN_JOINT_LENGTH;                    ///< filters out joints whose centerline is shorter

    // ── Clipper2 layer (face_overlap_area, wood_face_to_face.cpp) ────────
    extern int64_t CLIPPER_SCALE;                            ///< mm -> int64 scale for the 2D boolean (1e6 = nanometre grid)
    extern double  CLIPPER_AREA;                             ///< overlap areas at or below this (mm²) are not a contact

    // ── Filesystem strings ────────────────────────────────────────────────
    /// The data folder every yml, obj, txt and pb is named relative to, and where output/
    /// is written. Absolute, baked from __FILE__, so the working directory of the executable
    /// or the binding host does not matter. Set it from a binding to relocate the whole set.
    extern std::string DATA_SET_INPUT_FOLDER;
    extern const std::vector<std::string> DATASET_NAMES;     ///< every dataset shipped in data/ as <name>.yml, in sweep order
    extern const std::vector<std::string> SESSION_NAMES;     ///< every session shipped in data/ as <name>.pb

    /// Named access to every string in DATASET_NAMES: type `Dataset::` and the editor lists
    /// every dataset shipped in data/, so a call site never carries a bare index into a
    /// vector whose length - and therefore whose valid range - changes as datasets are added.
    /// Same strings, same sweep order; kept in sync by dataset_names_test.cpp.
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

        /// Plate outlines whose solved joints include type-30 crossings: compute_cross_contacts
        /// / compute_joints(cross_joint).
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

        /// Beam axes, not plate outlines (type_beams_name_*): compute_line_contacts /
        /// beam_volumes_pipeline, never load_plates.
        struct Curves {
            static constexpr const char* phanomema_node = Dataset::phanomema_node;
        };
    };

    /// DATASET_NAMES.at(index), so a stray index throws std::out_of_range instead of an
    /// operator[] read past the end - the difference between "dataset 49 does not exist"
    /// and a std::bad_alloc from whatever garbage bytes followed the vector in memory.
    const std::string& dataset_name(size_t index);

    /// data/<SESSION_NAMES[index]>.pb, for handing to session_cpp::Session::pb_load. Out of
    /// range throws rather than returning a path that is not there.
    std::string session_pb(size_t index);
    extern std::string DATA_SET_INPUT_NAME;                  ///< dataset name: the obj stem (set by globals_yaml and load_plates)
    extern std::string DATA_SET_OBJ;                         ///< obj path named by the dataset yaml
    extern std::string DATA_SET_ADJACENCY;                   ///< adjacency txt path from the yaml, empty when absent
    extern std::string DATA_SET_THREE_VALENCE;               ///< three-valence txt path from the yaml, empty when absent
    extern std::string DATA_SET_INSERTION_VECTORS;           ///< insertion-vectors txt path from the yaml, empty when absent
    extern std::string DATA_SET_JOINTS_TYPES;                ///< joint-types txt path from the yaml, empty when absent
    extern std::string DATA_SET_OUTPUT_FILE;                 ///< output .pb filename, written into session_data/
    extern std::string DATA_SET_OUTPUT_DATABASE;             ///< sqlite output path; informational, unused
    extern std::string PATH_AND_FILE_FOR_JOINTS;             ///< wood custom-joint-config file path; informational

    // ── Misc upstream-parity globals ──────────────────────────────────────
    extern std::vector<std::string> EXISTING_TYPES;          ///< upstream display table of joint variant names
    extern std::size_t RUN_COUNT;                            ///< upstream IMGUI loop counter; informational

    // ── Custom joint polylines (set at C++ runtime; YAML loader skips these) ─
    // Pairs (i, i+1) = (male, female) for one variant. Empty by default.
    // Wood's `wood_joint_lib.cpp` reads these to override the per-family
    // unit-cube geometry. No session-port consumer wired yet.
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

    // Reset every global above to wood baseline values. Used by the tutorial
    // mains (main_wood_01/02/03) that build geometry from scratch instead of
    // loading a named dataset. Test wrappers should prefer `globals_yaml(name)`.
    void reset_defaults();

    // Loads a dataset yaml - `data/<name>.yml` for a bare name, or the given
    // path when it ends in .yml - and applies every key to the globals above.
    // The file keys (obj, adjacency, three_valence, insertion_vectors,
    // joints_types) resolve relative to the yaml and land in DATA_SET_*.
    void globals_yaml(const std::string& dataset_name);
}} // namespace wood_session::globals
