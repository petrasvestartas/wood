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
// wood_session.cpp, which owns the scene-writing block near the bottom of this
// header. `main_5.cpp` is a near-empty entry point that only contains `int main()`.
//
// Structs: wood/wood_joint.h (WoodJoint) and wood/wood_element.h (WoodElement).
// Pipeline helpers (orient, merge) remain in wood_main.cpp's anonymous namespace.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Polyline is held by value inside CrossJoint → need the full type here.
#include "../src/polyline.h"
#include "../src/element.h"
// WoodElement / WoodJoint are passed by value/ref through this API surface.
#include "wood_element.h"

namespace session_cpp { class Vector; class Plane; class Line; class Session; class TreeNode; }

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

/// Override the directory globals_yaml() reads configs from. Empty string
/// restores the compile-time default (a __FILE__-derived path that only
/// exists on the machine that built the binary).
namespace globals { void set_config_dir(const std::string& dir); }

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
    extern std::string DATA_SET_INPUT_NAME;                  ///< short obj basename (set by load_plates)
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

    // Loads `wood/config/<dataset_name>.yml` and applies all keys to the globals
    // above. Replaces the old `reset_defaults()` + per-dataset hardcoded
    // overrides — users edit the YAML to retune a run without rebuilding.
    // Throws std::runtime_error if the YAML file is missing.
    void globals_yaml(const std::string& dataset_name);
}} // namespace wood_session::globals

// ═══════════════════════════════════════════════════════════════════════════
// internal:: — mirror of wood's `internal::` helpers from wood_test.cpp.
// Implementations live in wood_internal.cpp.
// ═══════════════════════════════════════════════════════════════════════════
namespace internal {

// Absolute path to the `data/` directory at the repo root (input geometry).
std::filesystem::path session_data_dir();

// Absolute path to `data/output/` — creates the directory on first call.
std::filesystem::path output_dir();

// Returns true iff the dataset for the given wood test function name exists
// in session_data/ (checks the underlying .obj file).
bool plates_exist(const std::string& wood_name);

// Load a named wood dataset from session_data/ and return one WoodElement per
// timber plate (planes, sides, thickness ready).
// Consecutive polylines (even index = bottom, odd = top) are paired.
// Also sets globals DATA_SET_INPUT_NAME and DATA_SET_OUTPUT_FILE as side effects.
// dataset_name — full wood test function name, e.g. "type_plates_name_hexbox_and_corner"
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
// Writing a scene — every way wood puts geometry into a session .pb.
//
// fill_session above is the solver's full legacy layout. These are the smaller
// scenes an example wants: what was in front of the detector, and what it found.
// They used to be copied into each example as a local write_live(), which is why
// they drifted; the .pb path lived in each copy too.
//
// FaceContact is only named here - wood_face_to_face.h includes this header, so
// this one cannot include it back.
// ═══════════════════════════════════════════════════════════════════════════
namespace wood_session {

struct FaceContact;

/// data/output/pb/<name>.pb, with the directory created. The one place a wood
/// scene's path is spelled out.
std::filesystem::path pb_path(const std::string& name);

/// Write the session to pb_path(name) and return that path. The default "live"
/// is the file session_viewer watches.
std::filesystem::path pb_dump(const session_cpp::Session& session,
                              const std::string& name = "live");

/// The scene an example asks for after running contact detection: every element
/// face outline under "Inputs", every contact area under "Contacts", written to
/// pb_path(name). `title` names the session as the viewer shows it; `name` is
/// the file stem, and the default is the one session_viewer watches. Two
/// overloads rather than a template, so this header need not pull in session.h.
std::filesystem::path write_element_and_contacts(
        const std::string& title,
        const std::vector<WoodElement>& elements,
        const std::vector<FaceContact>& contacts,
        const std::string& name = "live");

std::filesystem::path write_element_and_contacts(
        const std::string& title,
        const std::vector<BlockElement>& elements,
        const std::vector<FaceContact>& contacts,
        const std::string& name = "live");

// ── Pieces, for a scene that needs more than the above ────────────────────

/// Every face outline of every element, named `element_<i>_face_<f>`.
void add_faces(session_cpp::Session& session,
               const std::shared_ptr<session_cpp::TreeNode>& parent,
               const std::vector<WoodElement>& elements);

void add_faces(session_cpp::Session& session,
               const std::shared_ptr<session_cpp::TreeNode>& parent,
               const std::vector<BlockElement>& elements);

/// The overlap region of each face pair in contact, as a red mesh named
/// `contact_<a>_<b>`. A ring that triangulates to nothing draws nothing and is
/// dropped by Session::add_mesh.
void add_contacts(session_cpp::Session& session,
                  const std::shared_ptr<session_cpp::TreeNode>& parent,
                  const std::vector<FaceContact>& contacts);

/// Joint areas, centerlines and volumes, each into its own group under the
/// session root - the three belong together, so the groups are made here.
void add_joints(session_cpp::Session& session, const std::vector<WoodJoint>& joints);

/// One lofted solid per element, from its merged bottom outlines to its top
/// ones. An element missing either side contributes nothing.
void add_lofts(session_cpp::Session& session,
               const std::shared_ptr<session_cpp::TreeNode>& parent,
               const std::vector<WoodElement>& elements);

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
