// ═══════════════════════════════════════════════════════════════════════════
// wood/wood_session.h — shared API surface between the wood pipeline
// (wood/wood_main.cpp) and the test harness (wood/wood_test.cpp).
//
// Mirrors the relevant declarations from wood's source tree:
//   wood::GLOBALS  → wood_session::globals   (wood_globals.cpp)
//   internal::     → internal::              (wood_internal.cpp)
//   wood::main::   → get_connection_zones    (wood_main.cpp)
//   wood_test.h    → 43 type_plates_name_*() (wood_test.cpp)
//
// Implementations are split across the translation units listed above, plus
// wood_session.cpp, which owns WoodSession and WoodInteraction at the bottom of
// this header.
//
// Structs: wood/wood_joint.h (WoodJoint) and wood/wood_element.h (WoodElement).
// Pipeline helpers (orient, merge) remain in wood_main.cpp's anonymous namespace.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <variant>
#include <vector>

// Polyline is held by value inside CrossJoint → need the full type here.
#include "../src/polyline.h"
#include "../src/element.h"
// WoodSession derives from Session → the whole kernel, not a forward declaration.
#include "../src/session.h"
// WoodElement / WoodJoint are passed by value/ref through this API surface.
#include "wood_element.h"

// ═══════════════════════════════════════════════════════════════════════════
// wood_session::CrossJoint + plane_to_face — side-to-side cross/lap joint
// detection between two plate elements. Wood-domain geometry (joint area /
// volumes / lines + plate face indices + type-30 code), so it lives here
// rather than in the general session_cpp::Intersection kernel.
// Implementations in wood_joint_detection.cpp.
// ═══════════════════════════════════════════════════════════════════════════
namespace wood_session {

struct CrossJoint {
    int type = 30;                                          ///< Joint type code (30 = side-to-side cross)
    std::pair<int,int> face_ids_a{-1,-1};                   ///< Two side-face indices of element A involved
    std::pair<int,int> face_ids_b{-1,-1};                   ///< Two side-face indices of element B involved
    session_cpp::Polyline joint_area;                       ///< Closed quad on the mid-plane (5 pts)
    std::array<session_cpp::Polyline,2> joint_lines;        ///< Two perpendicular centerlines of joint_area
    std::array<session_cpp::Polyline,2> joint_volumes;      ///< Two parallel quads bounding the joint volume
};

/// Cross/lap joint detection between two plate elements (side-to-side).
/// Core overload: by reference, so the hot detection loop can pass the
/// element's stored polylines/planes without deep-copying 4 Polylines and
/// 4 Planes per candidate pair just to reach the parallelism reject.
bool plane_to_face(
    const session_cpp::Polyline& a_bottom, const session_cpp::Polyline& a_top,
    const session_cpp::Polyline& b_bottom, const session_cpp::Polyline& b_top,
    const session_cpp::Plane& a_plane_bottom, const session_cpp::Plane& a_plane_top,
    const session_cpp::Plane& b_plane_bottom, const session_cpp::Plane& b_plane_top,
    CrossJoint& result,
    double angle_tol = 5.0,
    const std::array<double,3>& extension = {0.0, 0.0, 0.0});

bool plane_to_face(
    const std::array<session_cpp::Polyline,2>& polylines_a,
    const std::array<session_cpp::Polyline,2>& polylines_b,
    const std::array<session_cpp::Plane,2>& planes_a,
    const std::array<session_cpp::Plane,2>& planes_b,
    CrossJoint& result,
    double angle_tol = 5.0,
    const std::array<double,3>& extension = {0.0, 0.0, 0.0});

/// Set the near-coplanar rejection threshold used internally by plane_to_face.
/// Wood reads from wood_session::globals::DISTANCE_SQUARED which some tests
/// (hexboxes) mutate. Caller syncs this before face_to_face iteration.
void set_cross_joint_distance_squared(double dist_sq);

} // namespace wood_session

// ═══════════════════════════════════════════════════════════════════════════
// wood_session::globals — mirror of wood's `wood::GLOBALS`. Definitions +
// `reset_defaults()` live in wood_globals.cpp. Each `type_plates_name_*()`
// wrapper calls `reset_defaults()` then overrides whichever entries the
// corresponding wood test overrides.
// ═══════════════════════════════════════════════════════════════════════════
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

// ═══════════════════════════════════════════════════════════════════════════
// internal:: — mirror of wood's `internal::` helpers from wood_test.cpp.
// Implementations live in wood_internal.cpp.
// ═══════════════════════════════════════════════════════════════════════════
namespace internal {

// The dataset folder: globals::DATA_SET_INPUT_FOLDER when set, else the
// repo's data/ next to this source tree. Absolute, so the working directory
// of the executable or the binding host does not matter.
std::filesystem::path session_data_dir();

// Absolute path to `data/output/` — creates the directory on first call.
std::filesystem::path output_dir();

// A bare name resolves to <session_data_dir>/<name><ext>; a path already
// ending in ext is returned as is.
std::filesystem::path dataset_path(const std::string& name, const std::string& ext);

// True iff data/<name>.obj exists.
bool plates_exist(const std::string& name);

// Load a named wood dataset from session_data/ and return one WoodElement per
// timber plate (planes, sides, thickness ready).
// Consecutive polylines (even index = bottom, odd = top) are paired.
// Also sets globals DATA_SET_INPUT_NAME and DATA_SET_OUTPUT_FILE as side effects.
// dataset_name — a name (data/<name>.obj) or a path ending in .obj
// duplicate_pts_tol — if > 0, removes consecutive duplicate points (vidychapel datasets)
std::vector<wood_session::WoodElement> load_plates(
        const std::string& dataset_name,
        double duplicate_pts_tol = 0.0);

// Load raw polylines from a named dataset (no top/bottom pairing).
// Used by beam datasets where each polyline is a beam axis.
// Also sets globals DATA_SET_INPUT_NAME and DATA_SET_OUTPUT_FILE as side effects.
std::vector<session_cpp::Polyline> load_polylines(
        const std::string& dataset_name,
        double duplicate_pts_tol = 0.0);

} // namespace internal

// ═══════════════════════════════════════════════════════════════════════════
// SearchType — controls which joint detection pass get_connection_zones runs.
// ═══════════════════════════════════════════════════════════════════════════
enum SearchType : int {
    face_to_face            = 0,  // coplanar face detection: ss_e_ip/op/r, ts_e_p
    cross_joint             = 1,  // crossing elements: plane_to_face (type-30)
    face_to_face_then_cross = 2,  // face-to-face first, then cross-joint fallback
};

// ═══════════════════════════════════════════════════════════════════════════
// get_connection_zones — 9-stage wood joint detection pipeline.
//
// Stages: BVH adjacency → face_to_face_wood detection → three-valence linking
//         → joint geometry creation → orientation → merge into plate outlines.
//
// elements    — timber plates as WoodElements (built via the WoodElement
//               (bot, top) ctor, or returned by internal::load_plates(name)).
//               IN-OUT, and the second half of the result: each element's
//               `features` is populated with the merged top/bottom outlines from
//               the merge pass, `insertion_vectors` with the vectors the solver
//               resolved, and a reversed plate has its (bot, top) pair swapped.
//               fill_session() and every other consumer read the outlines back
//               off these elements, so never hold your plates in a `const`
//               vector - the only way to pass one is to copy it, and the copy is
//               what carries the result you then throw away.
// search_type — face_to_face (default), cross_joint, or face_to_face_then_cross.
// Returns:    — every detected joint (per-pair), with type / area / lines /
//               volumes / male+female cut outlines populated.
//
// To visualize the result, build a Session and call fill_session(session,
// elements, joints, /*include_loft=*/true) — see below.
// ═══════════════════════════════════════════════════════════════════════════
std::vector<wood_session::WoodJoint> get_connection_zones(
        std::vector<wood_session::WoodElement>& elements,
        SearchType search_type = face_to_face);

// ═══════════════════════════════════════════════════════════════════════════
// ChevronJoineryData — pre-computed joinery metadata for chevron assemblies.
//
// When passed to the overload below, bypasses txt-file loading
// (DATA_SET_INPUT_NAME) and uses in-memory data instead.
// ═══════════════════════════════════════════════════════════════════════════
namespace wood_session {
struct ChevronJoineryData {
    std::vector<std::pair<int,int>>    adjacency;         ///< adjacent plate-pair indices
    std::vector<std::array<double,18>> insertion_vectors; ///< 6 Vec3 per element, flat (18 doubles)
    std::vector<std::array<int,6>>     joints_per_face;   ///< joint-type code per face per element
    std::vector<std::array<int,4>>     three_valence;     ///< Annen [s0,s1,e20,e31] groups
};
} // namespace wood_session

/// Overload: uses in-memory chevron joinery data instead of DATA_SET_INPUT_NAME txt files.
std::vector<wood_session::WoodJoint> get_connection_zones(
        std::vector<wood_session::WoodElement>& elements,
        SearchType search_type,
        const wood_session::ChevronJoineryData& joinery_data);

// ═══════════════════════════════════════════════════════════════════════════
// fill_session — splat the result of get_connection_zones into a Session for
// visualization / .pb persistence. Recreates the legacy group layout:
//   "Elements"                               — input plates as Element (WoodElement::to_element),
//                                              each detected joint attached as a "joint" ElementFeature
//   "JointAreas_SS_11" / "_TS_20" / "_Other" — per-type joint area polygons
//   "JointLines_SS_11" / "_TS_20" / "_Other" — per-type joint centerlines
//   "JointVols_SS_11"  / "_TS_20" / "_Other" — per-type joint volume quads
//   "element_<i>"                            — per-element merged outlines + cut polylines
//   "MergedMeshes"                           — loft of features.top/bottom per element
//                                              (only if include_loft = true)
// ═══════════════════════════════════════════════════════════════════════════
void fill_session(
        session_cpp::Session& session,
        const std::vector<wood_session::WoodElement>& elements,
        const std::vector<wood_session::WoodJoint>&   joints,
        bool include_loft = true);

// ═══════════════════════════════════════════════════════════════════════════
// A scene — the elements, what relates them, and how it is written
//
// fill_session above is the solver's full legacy layout. A WoodSession is the
// model: a session_cpp::Session whose elements wood knows the type of, and
// whose graph edges carry what the detector and the solver found between them.
// ═══════════════════════════════════════════════════════════════════════════
namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// WoodGeometry — the element types a wood scene holds
// ═══════════════════════════════════════════════════════════════════════════

/// The wood object wrapping one session_cpp::Element. Elements ONLY: a contact and a joint
/// are how two elements RELATE, so they live on the graph edge between them
/// (WoodInteraction below) and never in this list. A new element type is one more
/// alternative here, a tag and a from_element factory - no schema change.
using WoodGeometry = std::variant<
    std::shared_ptr<WoodElement>,
    std::shared_ptr<WoodColumn>,
    std::shared_ptr<BlockElement>>;

// ═══════════════════════════════════════════════════════════════════════════
// WoodInteraction — what one graph edge carries
// ═══════════════════════════════════════════════════════════════════════════

/// Everything the relation between two elements is made of: where they touch, and what the
/// solver made of it. compas_model keeps these as two edge attributes ("contacts",
/// "modifiers"); a session_cpp::Edge has room for one string, so both travel in it.
struct WoodInteraction {
    /// Every overlap region between the pair, face_a on the element the edge was read from.
    std::vector<FaceContact> contacts;
    /// What get_connection_zones made of them. A joint names its own two elements, because
    /// male and female is a solver decision and not the edge's ordering.
    std::vector<WoodJoint> joints;

    /// Value of "type" the attribute is written under, and the grammar's whole guard.
    static constexpr const char* TYPE = "WoodInteraction";

    bool empty() const { return contacts.empty() && joints.empty(); }
    /// face_a and face_b swapped in every contact - the edge read from the other end.
    WoodInteraction flipped() const;

    std::string to_attribute() const;
    /// Total: an attribute this grammar does not describe - Session::get_collisions()'s
    /// "bvh_collision", add_relationship's "default", "" - comes back empty.
    static WoodInteraction from_attribute(const std::string& attribute);

    nlohmann::ordered_json jsondump() const;
    static WoodInteraction jsonload(const nlohmann::json& data);

    std::string str() const;
    friend std::ostream& operator<<(std::ostream& os, const WoodInteraction& interaction);
};

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession — a session_cpp::Session that knows what its elements are
// ═══════════════════════════════════════════════════════════════════════════

/// A Session, extended rather than wrapped: the objects, tree, graph, xforms and history ARE
/// the kernel's, so anything a Session does a WoodSession does. What it adds is a typed view
/// of `objects.elements` and the reading and writing of a WoodInteraction on a graph edge.
///
/// Session has no virtual method and no virtual destructor, so this is a plain extension:
/// never delete one through a Session*. Copy is deleted because a Session copy duplicates the
/// elements while `elements` below would still name the originals, so the two halves would
/// disagree; from_session is how you get a second scene from one.
class WoodSession : public session_cpp::Session {
public:
    WoodSession() = default;
    explicit WoodSession(const std::string& name) : session_cpp::Session(name) {}
    WoodSession(const WoodSession&) = delete;
    WoodSession& operator=(const WoodSession&) = delete;
    WoodSession(WoodSession&&) = default;
    WoodSession& operator=(WoodSession&&) = default;

    /// Element guid -> the wood object wrapping it: Session::lookup, one variant up. An
    /// element the session holds and this map does not - one added straight through
    /// Session::add_element - is skipped by every accessor below, so a missing entry is
    /// inert rather than wrong. Order comes from `objects.elements`, which pb_load preserves.
    std::unordered_map<std::string, WoodGeometry> elements;

    using session_cpp::Session::add;
    /// Session::add_element, one variant up: the object's own Element into the session under
    /// `parent`, and the object into `elements`. The SAME Element, never a copy - an Element
    /// copy mints a new guid and the two halves drift apart from there.
    std::shared_ptr<session_cpp::TreeNode> add(const WoodGeometry& object,
                                               const std::shared_ptr<session_cpp::TreeNode>& parent = nullptr);
    /// Session::remove_object, plus the `elements` entry.
    bool remove_object(const std::string& guid);

    /// The wood object with this guid: null when the scene does not hold it, or holds it as
    /// another type. Named apart from Session::get_object, which is not virtual and has to
    /// keep working through this class.
    template <class T>
    std::shared_ptr<T> get_element(const std::string& guid) const {
        const auto it = elements.find(guid);
        if (it == elements.end()) { return nullptr; }
        const std::shared_ptr<T>* object = std::get_if<std::shared_ptr<T>>(&it->second);
        return object ? *object : nullptr;
    }

    /// Every wood object of one type, in `objects.elements` order.
    template <class T>
    std::vector<std::shared_ptr<T>> get_elements() const {
        std::vector<std::shared_ptr<T>> out;
        for (const std::shared_ptr<session_cpp::Element>& element : *objects.elements)
            if (const std::shared_ptr<T> object = get_element<T>(element->guid())) { out.push_back(object); }
        return out;
    }
    std::vector<std::shared_ptr<WoodElement>>  plates() const  { return get_elements<WoodElement>(); }
    std::vector<std::shared_ptr<WoodColumn>>   columns() const { return get_elements<WoodColumn>(); }
    std::vector<std::shared_ptr<BlockElement>> solids() const  { return get_elements<BlockElement>(); }

    /// Every wrapped element's guid in `objects.elements` order - the index space every
    /// ContactPair uses. An unwrapped element is skipped here and in detection alike, so the
    /// two index spaces agree.
    std::vector<std::string> element_guids() const;

    /// Drop every contact, or every joint, in the scene, so a recompute replaces rather than
    /// accumulates. The edges stay, and the other half of each interaction with them.
    void clear_contacts();
    void clear_joints();
    /// Coplanar face-overlap detection (side_side / side_top / top_top / unknown) over every
    /// element, stored one interaction per touching pair. `compute_contacts` is the same call,
    /// kept for existing callers.
    void compute_face_contacts();
    void compute_contacts();
    /// Elements that pass through each other rather than touch face-to-face - plane_to_face /
    /// CrossJoint over every pair of plates - stored as ContactType::cross, same edges as
    /// compute_face_contacts leaves untouched. `angle_tol` degrees, as plane_to_face takes.
    void compute_cross_contacts(double angle_tol = 30.0);
    /// Crossings between elements' boundary polylines (or NURBS curves sampled to one):
    /// every segment pair within `tolerance` mm of each other becomes a ContactType::line
    /// contact whose area is the short segment between the two closest points. `tolerance`
    /// < 0 reads globals::DISTANCE.
    void compute_line_contacts(double tolerance = -1.0);
    /// get_connection_zones over the plates, in place; every joint onto its pair's edge.
    void compute_joints(SearchType search_type = face_to_face);

    /// The interaction on the edge joining two elements, read from `a`: empty when the pair
    /// has no edge, or its attribute is not this grammar.
    WoodInteraction get_interaction(const std::string& a, const std::string& b) const;
    /// Store one on that pair's edge, adding the edge when the pair has none. An edge carries
    /// ONE attribute, so a writer that wants to keep the other half reads first: get, change,
    /// set.
    void set_interaction(const std::string& a, const std::string& b, const WoodInteraction& interaction);
    /// Every pair with an interaction, and it. An edge is stored twice, once per direction;
    /// this takes each once, as (a, b) with a < b.
    std::vector<std::tuple<std::string, std::string, WoodInteraction>> get_interactions() const;

    /// Session::get_collisions, with the interactions kept. The base writes "bvh_collision"
    /// over the attribute of every pair its broad phase finds, and that attribute is where a
    /// contact and a joint live, so the base alone would silently empty them.
    std::vector<std::pair<std::string, std::string>> get_collisions();

    /// Every contact in the scene as detection produced it: element positions in
    /// element_guids(), face_a on element_a. What the scene writers take.
    std::vector<ContactPair> contacts() const;
    /// Every joint in the scene, in edge order.
    std::vector<WoodJoint> joints() const;

    /// The joint features the GRAPH holds for one element: side [0] of a joint belongs to its
    /// element_a, side [1] to its element_b. compas_model's compute_modelgeometry, which
    /// reads its modifiers off its own edges rather than off the element. The guids are the
    /// joint's own, so a feature keeps its identity across a round trip.
    std::vector<session_cpp::ElementFeature> get_element_features(const std::string& guid) const;

    /// A wood scene built from the session's ELEMENTS, by `element_type`: "Plate" a
    /// WoodElement, "Column" a WoodColumn, anything else a solid. `session` is only READ.
    ///
    /// The tree is walked, so every element lands under the node it was under and groups nest
    /// as they did; adding an element makes its graph node, so the vertices come with them;
    /// and the xforms and the edges - with the contacts and joints they carry - come across by
    /// guid, because no guid changes. A wood scene is elements, so anything else the session
    /// held is NOT carried, and a warning on stderr says how much.
    static WoodSession from_session(const session_cpp::Session& session);
    /// This session, ready to write: every element payload refreshed from its wood fields,
    /// then every joint feature the graph holds put back on its host element. A WoodSession
    /// IS a Session, so the conversion is that refresh and nothing else.
    const session_cpp::Session& to_session() const;

    /// A session name (data/<name>.pb) or a .pb path, absorbed by from_session.
    static WoodSession pb_load(const std::filesystem::path& path);
    /// A dataset name (data/<name>.yml) or a .yml path: its globals apply, and the obj it
    /// names becomes the scene's plates.
    static WoodSession yaml_load(const std::filesystem::path& path);

    std::string str() const;
    friend std::ostream& operator<<(std::ostream& os, const WoodSession& scene);
};

// ═══════════════════════════════════════════════════════════════════════════
// Writing a scene
// ═══════════════════════════════════════════════════════════════════════════

/// data/output/pb/<name>.pb, with the directory created. The one place a wood scene's path
/// is spelled out.
std::filesystem::path pb_path(const std::string& name);

/// Write the session to pb_path(name) and return that path. The default "live" is the file
/// session_viewer watches.
std::filesystem::path pb_dump(const session_cpp::Session& session,
                              const std::string& name = "live");

/// The same, to_session() first. Session::pb_dumps is not virtual, so a WoodSession must
/// never reach the writer stale; taking the derived type by exact match is what guarantees
/// it, and the Session overload stays correct because a Session has nothing to refresh.
std::filesystem::path pb_dump(const WoodSession& scene, const std::string& name = "live");

// ═══════════════════════════════════════════════════════════════════════════
// The contact and joint coloring scheme
//
// One scheme across both layers: a joint takes a shade of the family its contact belongs
// to, so the two views read together.
//
//   contact side_side  navy   ->  joint 12 ss in-plane      navy
//                             ->  joint 11 ss out-of-plane  orange
//                             ->  joint 13 ss rotated       deep pink
//   contact side_top   pink   ->  joint 20 top-to-side      pink
//   contact top_top    green  ->  joint 40 top-to-top       green
//   contact unknown    grey   ->  joint 30 cross            yellow
//
// Colors are floats in [0,1]. session_cpp::Color clamps to that range, so an 0-255 literal
// silently saturates to white.
// ═══════════════════════════════════════════════════════════════════════════

/// "side_side" / "side_top" / "top_top" / "unknown" / "cross" / "line" - the group name a
/// contact of that class is filed under.
const char* contact_type_name(ContactType type);

/// "ss_ip_12" / "ss_op_11" / "ss_rot_13" / "ts_20" / "cross_30" / "tt_40", or "type_<n>"
/// for a code the table does not name.
std::string joint_type_name(int joint_type);

session_cpp::Color contact_color(ContactType type);
session_cpp::Color joint_color(int joint_type);

/// Every element as its outlines rather than its solid: a plate is its bottom and its top,
/// anything without that convention is every face it has. A wireframe scene is what lets the
/// contact and joint rings below be seen at all - inside a lofted solid they are invisible.
void add_outlines(session_cpp::Session& session, const WoodSession& scene,
                  const std::string& prefix = "Elements");

/// Contacts split into one group per contact class that actually occurs, named
/// `<prefix>_<class>`, each ring drawn as a polyline colored by contact_color. Groups are flat -
/// Session::add_group always attaches to the root - which is why the class goes in the
/// name, the way fill_session already names JointAreas_SS_11. `contact.area` must be a
/// closed polygon (>= 3 points) - side_side/side_top/top_top/unknown/cross all are;
/// ContactType::line is not and is skipped here, drawn by add_line_contacts_by_type instead.
void add_contacts_by_type(session_cpp::Session& session,
                          const std::vector<ContactPair>& contacts,
                          const std::string& prefix = "Contacts");

/// ContactType::line contacts only, one group per element-pair polyline class, each drawn as
/// the short open polyline between the two curves' closest points (contact.area, 2 points).
void add_line_contacts_by_type(session_cpp::Session& session,
                               const std::vector<ContactPair>& contacts,
                               const std::string& prefix = "LineContacts");

/// Joint areas split into one group per joint_type that actually occurs, named
/// `<prefix>_<code>`, each ring drawn as a polyline colored by joint_color.
void add_joints_by_type(session_cpp::Session& session,
                        const std::vector<WoodJoint>& joints,
                        const std::string& prefix = "Joints");

} // namespace wood_session

// ═══════════════════════════════════════════════════════════════════════════
// beam_volumes_pipeline — beam (axis+radius) entry point. Equivalent of
// wood's `wood::main::beam_volumes`. Used by type_beams_name_* datasets
// (e.g. phanomema_node) that store beam axes rather than plate outlines.
// ═══════════════════════════════════════════════════════════════════════════
void beam_volumes_pipeline(
        const std::vector<session_cpp::Polyline>& axes,
        const std::vector<std::vector<double>>& segment_radii,
        const std::vector<std::vector<session_cpp::Vector>>& segment_direction,
        const std::vector<int>& allowed_types_per_polyline,
        double min_distance,
        double volume_length,
        double cross_or_side_to_end,
        int    flip_male);

// ═══════════════════════════════════════════════════════════════════════════
// wood_test.h — 43 test function declarations (wood line-number order).
// ═══════════════════════════════════════════════════════════════════════════
bool type_plates_name_hexbox_and_corner();                                            // 204
bool type_plates_name_joint_linking_vidychapel_corner();                              // 265
bool type_plates_name_joint_linking_vidychapel_one_layer();                           // 428
bool type_plates_name_joint_linking_vidychapel_one_axis_two_layers();                 // 488
bool type_plates_name_joint_linking_vidychapel_full();                                // 611
bool type_plates_name_side_to_side_edge_inplane_2_butterflies();                      // 888
bool type_plates_name_side_to_side_edge_inplane_hexshell();                           // 940
bool type_plates_name_side_to_side_edge_inplane_differentdirections();                // 998
bool type_plates_name_side_to_side_edge_outofplane_folding();                         // 1129
bool type_plates_name_side_to_side_edge_outofplane_box();                             // 1384
bool type_plates_name_side_to_side_edge_outofplane_box_miter();
bool type_plates_name_side_to_side_edge_outofplane_tetra();                           // 1440
bool type_plates_name_side_to_side_edge_outofplane_dodecahedron();                    // 1497
bool type_plates_name_side_to_side_edge_outofplane_icosahedron();                     // 1555
bool type_plates_name_side_to_side_edge_outofplane_octahedron();                      // 1613
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners();          // 1671
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_combined(); // 1729
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths(); // 1787
bool type_plates_name_side_to_side_edge_inplane_hilti();                              // 1849
bool type_plates_name_top_to_top_pairs();                                             // 1912
bool type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes(); // 1965
bool type_plates_name_hex_block_rossiniere();                                         // 2037
bool type_plates_name_top_to_side_snap_fit();                                         // 2104
bool type_plates_name_top_to_side_box();                                              // 2163
bool type_plates_name_top_to_side_corners();                                          // 2220
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_corner();         // 2279
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box();            // 2391
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box_pair();       // 2488
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_small();     // 2698
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_full_arch(); // 2763
bool type_plates_name_vda_floor_0();                                                  // 2829
bool type_plates_name_vda_floor_2();                                                  // 2888
bool type_plates_name_cross_and_sides_corner();                                       // 2955
bool type_plates_name_cross_corners();                                                // 3016
bool type_plates_name_cross_vda_corner();                                             // 3080
bool type_plates_name_cross_vda_hexshell();                                           // 3144
bool type_plates_name_cross_vda_hexshell_reciprocal();                                // 3207
bool type_plates_name_cross_vda_single_arch();                                        // 3270
bool type_plates_name_cross_vda_shell();                                              // 3333
bool type_plates_name_cross_square_reciprocal_two_sides();                            // 3396
bool type_plates_name_cross_square_reciprocal_iseya();                                // 3459
bool type_plates_name_cross_ibois_pavilion();                                         // 3522
bool type_plates_name_cross_brussels_sports_tower();                                  // 3588
bool type_beams_name_phanomema_node();                                                // 3670
