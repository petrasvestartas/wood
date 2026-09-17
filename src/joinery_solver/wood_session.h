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

/// The dataset folder, globals::DATA_SET_INPUT_FOLDER; absolute, so the working directory does not matter.
std::filesystem::path session_data_dir();

/// Absolute path to data/output/, created on first call.
std::filesystem::path output_dir();

/// A bare name resolves to <session_data_dir>/<name><ext>; a path already ending in ext is returned as is.
std::filesystem::path dataset_path(const std::string& name, const std::string& ext);

/// True iff data/<name>.obj exists.
bool plates_exist(const std::string& name);

/// One Plate per consecutive outline pair (even = bottom, odd = top) of data/<name>.obj or an .obj path; duplicate_pts_tol > 0 removes consecutive duplicate points.
std::vector<std::shared_ptr<wood_session::Plate>> load_plates(
        const std::string& dataset_name,
        double duplicate_pts_tol = 0.0);

/// The raw polylines of data/<name>.obj or an .obj path, unpaired: beam datasets, one axis per polyline.
std::vector<session_cpp::Polyline> load_polylines(
        const std::string& dataset_name,
        double duplicate_pts_tol = 0.0);

} // namespace internal

// ═══════════════════════════════════════════════════════════════════════════
// Joint detection pipeline
// ═══════════════════════════════════════════════════════════════════════════


/// The 9-stage detection pipeline over the plates, in place: every plate's `features` and `insertion_vectors` are filled, and every detected joint is returned.
std::vector<wood_session::WoodJoint> get_connection_zones(
        std::vector<std::shared_ptr<wood_session::Plate>>& elements,
        SearchType search_type = face_to_face);

namespace wood_session {
/// Pre-computed joinery metadata for chevron assemblies, used instead of the DATA_SET_INPUT_NAME txt files.
struct ChevronJoineryData {
    std::vector<std::pair<int,int>>    adjacency;         ///< adjacent plate-pair indices
    std::vector<std::array<double,18>> insertion_vectors; ///< 6 Vec3 per element, flat (18 doubles)
    std::vector<std::array<int,6>>     joints_per_face;   ///< joint-type code per face per element
    std::vector<std::array<int,4>>     three_valence;     ///< Annen [s0,s1,e20,e31] groups
};
} // namespace wood_session

/// Overload: uses in-memory chevron joinery data instead of DATA_SET_INPUT_NAME txt files.
std::vector<wood_session::WoodJoint> get_connection_zones(
        std::vector<std::shared_ptr<wood_session::Plate>>& elements,
        SearchType search_type,
        const wood_session::ChevronJoineryData& joinery_data);

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// WoodInteraction - what one graph edge carries
// ═══════════════════════════════════════════════════════════════════════════

struct WoodInteraction {
    /// Every overlap region between the pair, face_a on the element the edge was read from.
    std::vector<FaceContact> contacts;
    /// What get_connection_zones made of them; a joint names its own two elements.
    std::vector<WoodJoint> joints;

    /// Value of "type" the attribute is written under, and the grammar's whole guard.
    static constexpr const char* TYPE = "WoodInteraction";

    bool empty() const { return contacts.empty() && joints.empty(); }
    /// face_a and face_b swapped in every contact - the edge read from the other end.
    WoodInteraction flipped() const;

    std::string to_attribute() const;
    /// Total: an attribute this grammar does not describe ("bvh_collision", "default", "") comes back empty.
    static WoodInteraction from_attribute(const std::string& attribute);

    nlohmann::ordered_json jsondump() const;
    static WoodInteraction jsonload(const nlohmann::json& data);

    std::string str() const;
    friend std::ostream& operator<<(std::ostream& os, const WoodInteraction& interaction);
};

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - a Session whose elements are plates, columns and blocks
// ═══════════════════════════════════════════════════════════════════════════

/// A Session with typed element access and a WoodInteraction on each graph edge; Session has no virtual method, so never delete one through a Session*.
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
    void compute_joints(SearchType search_type = globals::SEARCH_TYPE);

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

    /// Every element as its outlines under `prefix`: a plate its bottom and top, anything else every face it has.
    void add_outlines(const std::string& prefix = "Elements");
    /// Every contact as a coloured ring, one group per contact class that occurs, named `<prefix>_<class>`.
    void add_contacts(const std::string& prefix = "Contacts");
    /// Every joint's area, volumes, lines and cut outlines, one group per joint type that occurs, named `<prefix>_<code>`.
    void add_joints(const std::string& prefix = "Joints");
    /// Write the scene: a bare name goes to pb_path(name) ("live" is what session_viewer watches); a name ending in .pb goes to data/output/ with the parity dumps beside it. Returns the path.
    std::filesystem::path write(const std::string& name = "live");

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

// ═══════════════════════════════════════════════════════════════════════════
// Colours
// ═══════════════════════════════════════════════════════════════════════════

/// "side_side" / "side_top" / "top_top" / "unknown" / "cross" / "line" - the group name a contact of that class is filed under.
const char* contact_type_name(ContactType type);

/// "ss_ip_12" / "ss_op_11" / "ss_rot_13" / "ts_20" / "cross_30" / "tt_40", or "type_<n>" for a code the table does not name.
std::string joint_type_name(int joint_type);

session_cpp::Color contact_color(ContactType type);
session_cpp::Color joint_color(int joint_type);

} // namespace wood_session

// ═══════════════════════════════════════════════════════════════════════════
// Beams
// ═══════════════════════════════════════════════════════════════════════════

/// Beam (axis + radius) entry point for the type_beams_name_* datasets: joint volumes per axis contact, written to data/output/<DATA_SET_OUTPUT_FILE>.
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
// Datasets as tests
// ═══════════════════════════════════════════════════════════════════════════

bool type_plates_name_hexbox_and_corner();
bool type_plates_name_joint_linking_vidychapel_corner();
bool type_plates_name_joint_linking_vidychapel_one_layer();
bool type_plates_name_joint_linking_vidychapel_one_axis_two_layers();
bool type_plates_name_joint_linking_vidychapel_full();
bool type_plates_name_side_to_side_edge_inplane_2_butterflies();
bool type_plates_name_side_to_side_edge_inplane_hexshell();
bool type_plates_name_side_to_side_edge_inplane_differentdirections();
bool type_plates_name_side_to_side_edge_outofplane_folding();
bool type_plates_name_side_to_side_edge_outofplane_box();
bool type_plates_name_side_to_side_edge_outofplane_box_miter();
bool type_plates_name_side_to_side_edge_outofplane_tetra();
bool type_plates_name_side_to_side_edge_outofplane_dodecahedron();
bool type_plates_name_side_to_side_edge_outofplane_icosahedron();
bool type_plates_name_side_to_side_edge_outofplane_octahedron();
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners();
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_combined();
bool type_plates_name_side_to_side_edge_inplane_outofplane_simple_corners_different_lengths();
bool type_plates_name_side_to_side_edge_inplane_hilti();
bool type_plates_name_top_to_top_pairs();
bool type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes();
bool type_plates_name_hex_block_rossiniere();
bool type_plates_name_top_to_side_snap_fit();
bool type_plates_name_top_to_side_box();
bool type_plates_name_top_to_side_corners();
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_corner();
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box();
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_box_pair();
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_small();
bool type_plates_name_top_to_side_and_side_to_side_outofplane_annen_grid_full_arch();
bool type_plates_name_vda_floor_0();
bool type_plates_name_vda_floor_2();
bool type_plates_name_cross_and_sides_corner();
bool type_plates_name_cross_corners();
bool type_plates_name_cross_vda_corner();
bool type_plates_name_cross_vda_hexshell();
bool type_plates_name_cross_vda_hexshell_reciprocal();
bool type_plates_name_cross_vda_single_arch();
bool type_plates_name_cross_vda_shell();
bool type_plates_name_cross_square_reciprocal_two_sides();
bool type_plates_name_cross_square_reciprocal_iseya();
bool type_plates_name_cross_ibois_pavilion();
bool type_plates_name_cross_brussels_sports_tower();
bool type_beams_name_phanomema_node();
