#pragma once

#include "../src/element.h"
#include "../src/polyline.h"
#include "../src/session.h"
#include "wood_element_block.h"
#include "wood_element_column.h"
#include "wood_element_plate.h"
#include "wood_globals.h"
#include "wood_joint.h"
#include "wood_joint_detection.h"

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════════
// Datasets
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
std::vector<wood_session::Plate> load_plates(
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
// Joint detection pipeline
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
        std::vector<wood_session::Plate>& elements,
        SearchType search_type = face_to_face);

// ═══════════════════════════════════════════════════════════════════════════
// ChevronJoineryData — pre-computed joinery metadata for chevron assemblies.
//
// When passed to the overload below, bypasses txt-file loading
// (DATA_SET_INPUT_NAME) and uses in-memory data instead.

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
        std::vector<wood_session::Plate>& elements,
        SearchType search_type,
        const wood_session::ChevronJoineryData& joinery_data);

// ═══════════════════════════════════════════════════════════════════════════
// fill_session — splat the result of get_connection_zones into a Session for
// visualization / .pb persistence. Recreates the legacy group layout:
//   "Elements"                               — input plates as Element (WoodElement::to_element),
//                                              each detected joint attached as a "joint" ElementFeature
//   "JointAreas_SS_11" / "_TS_20" / "_Other" — per-type joint area polygons
//   "JointLines_SS_11" / "_TS_20" / "_Other" — per-type joint centerlines

//   "element_<i>"                            — per-element merged outlines + cut polylines
//   "MergedMeshes"                           — loft of features.top/bottom per element
//                                              (only if include_loft = true)
// ═══════════════════════════════════════════════════════════════════════════
void fill_session(
        session_cpp::Session& session,
        const std::vector<wood_session::Plate>& elements,
        const std::vector<wood_session::WoodJoint>&   joints,
        bool include_loft = true);

// ═══════════════════════════════════════════════════════════════════════════
// A scene — the elements, what relates them, and how it is written
//
// fill_session above is the solver's full legacy layout. A WoodSession is the
// model: a session_cpp::Session whose elements wood knows the type of, and
// whose graph edges carry what the detector and the solver found between them.

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// WoodInteraction - what one graph edge carries
// ═══════════════════════════════════════════════════════════════════════════

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
// WoodSession - a Session whose elements are plates, columns and blocks
// ═══════════════════════════════════════════════════════════════════════════

/// A Session, extended: the objects, tree, graph, xforms and history ARE the kernel's. What it
/// adds is typed access to the elements and the reading and writing of a WoodInteraction on a
/// graph edge. Session has no virtual method, so never delete one through a Session*.
class WoodSession : public session_cpp::Session {
public:
    WoodSession();
    explicit WoodSession(const std::string& name);

    using session_cpp::Session::add;
    /// Session::add_element: the object itself, never a copy, so its guid is the guid on the wire.
    std::shared_ptr<session_cpp::TreeNode> add(std::shared_ptr<session_cpp::Element> element,
                                               std::shared_ptr<session_cpp::TreeNode> parent = nullptr);

    /// The element with this guid as T, or null when the scene does not hold it as that type.
    template <class T>
    std::shared_ptr<T> get_element(const std::string& guid) const {
        for (const std::shared_ptr<session_cpp::Element>& element : *objects.elements)
            if (element && element->guid() == guid) return std::dynamic_pointer_cast<T>(element);
        return nullptr;
    }
    /// Every element of type T, in objects.elements order.
    template <class T>
    std::vector<std::shared_ptr<T>> get_elements() const {
        std::vector<std::shared_ptr<T>> out;
        for (const std::shared_ptr<session_cpp::Element>& element : *objects.elements)
            if (const std::shared_ptr<T> object = std::dynamic_pointer_cast<T>(element)) out.push_back(object);
        return out;
    }
    std::vector<std::shared_ptr<Plate>>  plates()  const { return get_elements<Plate>(); }
    std::vector<std::shared_ptr<Column>> columns() const { return get_elements<Column>(); }
    std::vector<std::shared_ptr<Block>>  blocks()  const { return get_elements<Block>(); }
    /// Every element's guid in objects.elements order: the index space every ContactPair uses.
    std::vector<std::string> element_guids() const;

    /// Drop every contact, or every joint, so a recompute replaces rather than accumulates.
    void clear_contacts();
    void clear_joints();
    /// Coplanar face-overlap detection over every element, one interaction per touching pair.
    void compute_face_contacts();
    void compute_contacts();
    /// Elements that pass through each other: plane_to_face over every pair of plates, stored as ContactType::cross.
    void compute_cross_contacts(double angle_tol = 30.0);
    /// Crossings between elements' boundary polylines within `tolerance` mm (< 0 reads globals::DISTANCE), stored as ContactType::line.
    void compute_line_contacts(double tolerance = -1.0);
    /// get_connection_zones over the plates, in place; every joint onto its pair's edge.
    void compute_joints(SearchType search_type = face_to_face);

    /// The interaction on the edge joining two elements, read from `a`; empty when there is none.
    WoodInteraction get_interaction(const std::string& a, const std::string& b) const;
    /// Store one on that pair's edge, adding the edge when the pair has none.
    void set_interaction(const std::string& a, const std::string& b, const WoodInteraction& interaction);
    /// Every pair with an interaction, each once as (a, b) with a < b.
    std::vector<std::tuple<std::string, std::string, WoodInteraction>> get_interactions() const;
    /// Session::get_collisions with the interactions kept.
    std::vector<std::pair<std::string, std::string>> get_collisions();

    /// Every contact as detection produced it: element positions in element_guids(), face_a on element_a.
    std::vector<ContactPair> contacts() const;
    /// Every joint in the scene, in edge order.
    std::vector<WoodJoint> joints() const;
    /// The joint features the graph holds for one element: side [0] of a joint belongs to its element_a, side [1] to its element_b.
    std::vector<session_cpp::ElementFeature> get_element_features(const std::string& guid) const;
    /// Put every joint feature the graph holds back on its host element, replacing the previous ones.
    void sync_joint_features();

    /// A session name (data/<name>.pb) or a .pb path; the elements come back as Plate / Column / Block.
    static WoodSession pb_load(const std::filesystem::path& path);
    /// A dataset name (data/<name>.yml) or a .yml path: its globals apply, and the obj it names becomes the scene's plates.
    static WoodSession yaml_load(const std::filesystem::path& path);

    std::string str() const;
    friend std::ostream& operator<<(std::ostream& os, const WoodSession& scene);
};

// ═══════════════════════════════════════════════════════════════════════════
// Writing a scene
// ═══════════════════════════════════════════════════════════════════════════

/// data/output/pb/<name>.pb, with the directory created.
std::filesystem::path pb_path(const std::string& name);
/// Write the session to pb_path(name) and return that path; "live" is the file session_viewer watches.
std::filesystem::path pb_dump(const session_cpp::Session& session, const std::string& name = "live");
/// The same, with every joint feature put back on its host element first.
std::filesystem::path pb_dump(WoodSession& scene, const std::string& name = "live");

// ═══════════════════════════════════════════════════════════════════════════
// Colours
// ═══════════════════════════════════════════════════════════════════════════

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
//   contact cross      yellow ->  joint 30 cross            yellow
//   contact unknown    grey   ->  no joint equivalent
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

// ═══════════════════════════════════════════════════════════════════════════
// Viewer geometry
// ═══════════════════════════════════════════════════════════════════════════

/// Every element as its outlines: a plate its bottom and top, anything else every face it has.
void add_outlines(session_cpp::Session& session, const WoodSession& scene, const std::string& prefix = "Elements");
/// Contacts as rings, one group per contact class that occurs, named `<prefix>_<class>`; line contacts are skipped.
void add_contacts_by_type(session_cpp::Session& session, const std::vector<ContactPair>& contacts, const std::string& prefix = "Contacts");
/// ContactType::line contacts only, as the short segment between the two curves' closest points.
void add_line_contacts_by_type(session_cpp::Session& session, const std::vector<ContactPair>& contacts, const std::string& prefix = "LineContacts");
/// Joint areas, volumes, lines and cut outlines, one group per joint type that occurs, named `<prefix>_<code>`.
void add_joints_by_type(session_cpp::Session& session, const std::vector<WoodJoint>& joints, const std::string& prefix = "Joints");

} // namespace wood_session

// ═══════════════════════════════════════════════════════════════════════════
// Beams
// ═══════════════════════════════════════════════════════════════════════════

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
// Datasets as tests
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
