#pragma once

#include "wood_pch.h"

#include "wood_element_block.h"
#include "wood_element_column.h"
#include "wood_element_plate.h"
#include "wood_globals.h"
#include "wood_joint.h"
#include "wood_joint_detection.h"

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
    std::vector<std::pair<int, int>> adjacency; // Adjacent plate pairs, by position.
    std::vector<std::array<double, 18>> insertion_vectors; // Six vectors per element, flat: 18 doubles.
    std::vector<std::array<int, 6>> joints_per_face; // Joint type code per face per element.
    std::vector<std::array<int, 4>> three_valence; // Annen three-valence groups [s0, s1, e20, e31].
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

/// Everything the relation between two elements is made of: where they touch, and what the solver made of it.
struct WoodInteraction {
    std::vector<FaceContact> contacts; // Every overlap region between the pair, face_a on the element the edge was read from.
    std::vector<WoodJoint> joints; // What get_connection_zones made of them; a joint names its own two elements.
    static constexpr const char* TYPE = "WoodInteraction"; // Value of "type" the attribute is written under, and the grammar's whole guard.

    /// True when there is neither a contact nor a joint.
    bool empty() const { return contacts.empty() && joints.empty(); }

    /// face_a and face_b swapped in every contact: the edge read from the other end.
    WoodInteraction flipped() const;

    /// jsondump() as the string a graph edge stores.
    std::string to_attribute() const;

    /// Total: an attribute this grammar does not describe ("bvh_collision", "default", "") comes back empty.
    static WoodInteraction from_attribute(const std::string& attribute);

    /// The interaction as JSON: contacts, joints, type.
    nlohmann::ordered_json jsondump() const;

    /// An interaction from its JSON.
    static WoodInteraction jsonload(const nlohmann::json& data);

    /// "WoodInteraction(contacts, joints)".
    std::string str() const;

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const WoodInteraction& interaction);
};

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - a Session whose elements are plates, columns and blocks
// ═══════════════════════════════════════════════════════════════════════════

/// A Session with typed element access and a WoodInteraction on each graph edge; Session has no virtual method, so never delete one through a Session*.
class WoodSession : public session_cpp::Session {
public:
    /// An empty scene; registers the three element factories with the kernel.
    WoodSession();

    /// An empty scene with a name.
    explicit WoodSession(const std::string& name);

    /// Session::add for tree nodes.
    using session_cpp::Session::add;

    /// Session::add_element: the object itself, never a copy, so its guid is the guid on the wire.
    std::shared_ptr<session_cpp::TreeNode> add(
        std::shared_ptr<session_cpp::Element> element,
        std::shared_ptr<session_cpp::TreeNode> parent = nullptr
    );

    /// The element with this guid as T, or null when the scene does not hold it as that type.
    template <class T>
    std::shared_ptr<T> get_element(const std::string& guid) const {
        for (const std::shared_ptr<session_cpp::Element>& element : *objects.elements)
            if (element && element->guid() == guid)
                return std::dynamic_pointer_cast<T>(element);
        return nullptr;
    }

    /// Every element of type T, in objects.elements order.
    template <class T>
    std::vector<std::shared_ptr<T>> get_elements() const {
        std::vector<std::shared_ptr<T>> out;
        for (const std::shared_ptr<session_cpp::Element>& element : *objects.elements)
            if (const std::shared_ptr<T> object = std::dynamic_pointer_cast<T>(element))
                out.push_back(object);
        return out;
    }

    /// Every Plate, in objects.elements order.
    std::vector<std::shared_ptr<Plate>> plates() const { return get_elements<Plate>(); }

    /// Every Column, in objects.elements order.
    std::vector<std::shared_ptr<Column>> columns() const { return get_elements<Column>(); }

    /// Every Block, in objects.elements order.
    std::vector<std::shared_ptr<Block>> blocks() const { return get_elements<Block>(); }

    /// Every element's guid in objects.elements order: the index space every ContactPair uses.
    std::vector<std::string> element_guids() const;

    /// Drops every contact, so a recompute replaces rather than accumulates; the joints stay.
    void clear_contacts();

    /// Drops every joint; the contacts stay.
    void clear_joints();

    /// Coplanar face-overlap detection over every element, one interaction per touching pair.
    void compute_face_contacts();

    /// compute_face_contacts(), kept for existing callers.
    void compute_contacts();

    /// Elements that pass through each other: plane_to_face over every pair of plates, stored as ContactType::cross.
    void compute_cross_contacts(double angle_tol = 30.0);

    /// Crossings between elements' boundary polylines within `tolerance` mm (< 0 reads globals::DISTANCE), stored as ContactType::line.
    void compute_line_contacts(double tolerance = -1.0);

    /// get_connection_zones over the plates, in place; every joint onto its pair's edge and onto both host elements as features, every plate lofted with its cuts.
    void compute_joints(SearchType search_type = globals::SEARCH_TYPE);

    /// The interaction on the edge joining two elements, read from `a`; empty when there is none.
    WoodInteraction get_interaction(const std::string& a, const std::string& b) const;

    /// Stores one on that pair's edge, adding the edge when the pair has none.
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

    /// Puts every joint feature the graph holds back on its host element, replacing the previous ones.
    void sync_joint_features();

    /// Arranges the scene for the viewer, one group per element: the element itself, then `outlines`, `contacts` and `joints` child groups, each flag adding or leaving out that part; pb_dump writes it.
    void add_to_tree(bool geometry = true, bool outlines = true, bool contacts = true, bool joints = true);

    /// Lofts every plate whose Element slot is stale, so the file carries the model geometry; the plates stay unlofted until this runs.
    void sync_geometry() const;

    /// sync_geometry(), then the kernel's writer.
    void pb_dump(const std::string& filename) const;

    /// sync_geometry(), then the kernel's serializer.
    std::string pb_dumps() const;

    /// A session name (data/<name>.pb) or a .pb path; the elements come back as Plate / Column / Block.
    static WoodSession pb_load(const std::filesystem::path& path);

    /// A dataset name (data/<name>.yml) or a .yml path: its globals apply, and the obj it names becomes the scene's plates.
    static WoodSession yaml_load(const std::filesystem::path& path);

    /// "WoodSession(name, elements, plates, columns, blocks, edges)".
    std::string str() const;

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const WoodSession& scene);

private:
    /// Every contact as a coloured ring under the `contacts` group of its first element.
    void add_contacts_to(const std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& groups, std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& children);

    /// Every joint's area, volumes, lines and male cuts under the `joints` group of its male element, the female cuts under the female's.
    void add_joints_to(const std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& groups, std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& children);
};

// ═══════════════════════════════════════════════════════════════════════════
// Writing a scene
// ═══════════════════════════════════════════════════════════════════════════

/// data/output/pb/<name>.pb, with the directory created; "live" is the file session_viewer watches.
std::filesystem::path pb_path(const std::string& name);

/// <pb>_meta.txt and <pb>_coords.txt beside a dataset's .pb: every plate's merged outlines, the parity record a refactor is diffed against.
void write_parity_dumps(const WoodSession& scene, const std::filesystem::path& pb);

// ═══════════════════════════════════════════════════════════════════════════
// Colours
// ═══════════════════════════════════════════════════════════════════════════

/// "side_side" / "side_top" / "top_top" / "unknown" / "cross" / "line" - the group name a contact of that class is filed under.
const char* contact_type_name(ContactType type);

/// "ss_ip_12" / "ss_op_11" / "ss_rot_13" / "ts_20" / "cross_30" / "tt_40", or "type_<n>" for a code the table does not name.
std::string joint_type_name(int joint_type);

/// The colour of a contact class: the colour of the joint type it refines to, grey when unknown.
session_cpp::Color contact_color(ContactType type);

/// The colour of a joint type: 12 navy, 11 orange, 13 deep pink, 20 pink, 40 green, 30 yellow, grey otherwise.
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

/// data/phanomema_node.yml through beam_volumes_pipeline; false on failure.
bool type_beams_name_phanomema_node();
