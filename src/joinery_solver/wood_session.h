#pragma once

#include "pch.h"

#include "wood_element_beam.h"
#include "wood_element_block.h"
#include "wood_element_column.h"
#include "wood_element_plate.h"
#include "wood_config.h"
#include "wood_joint.h"
#include "wood_joint_data.h"
#include "wood_session_interaction.h"
#include "wood_joint_detection.h"

// ═══════════════════════════════════════════════════════════════════════════
// Joint detection pipeline
// ═══════════════════════════════════════════════════════════════════════════

/// WoodSession::compute_joints over loose plates, for callers without a scene: the plates are solved in place and every detected joint returned.
std::vector<wood_session::WoodJoint> get_connection_zones(
        std::vector<std::shared_ptr<wood_session::Plate>>& elements,
        SearchType search_type = face_to_face);

/// The same with the joint data given instead of read from the sidecar files.
std::vector<wood_session::WoodJoint> get_connection_zones(
        std::vector<std::shared_ptr<wood_session::Plate>>& elements,
        SearchType search_type,
        const wood_session::JointData& data);

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - a Session whose elements are plates, columns and blocks
// ═══════════════════════════════════════════════════════════════════════════

/// A Session with typed element access and a WoodInteraction per element pair, held as objects and never parsed on read: pb_dump writes them onto the graph edges, pb_load reads them back once; Session has no virtual method, so never delete one through a Session*. Every plate holds two geometries: element_geometry_mesh(), the plate alone, the loft of its two outlines, never cut; and model_geometry_mesh(), the plate with its joints cut in, the one to inspect. compute_joints() fills the joints and the merged outlines but lofts nothing; pb_dump() lofts every plate that is not yet lofted, so the file carries the model geometry the viewer draws.
class WoodSession : public session_cpp::Session {
private:
    std::map<std::pair<std::string, std::string>, WoodInteraction> interactions; // The store: one interaction per pair, oriented to the low guid.
    std::unordered_map<std::string, std::pair<std::string, std::string>> contact_pairs; // Contact guid -> the pair holding it.

public:
    /// An empty scene; registers the four element factories with the kernel.
    WoodSession();

    /// An empty scene with a name.
    explicit WoodSession(const std::string& name);

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A session name (data/<name>.pb) or a .pb path; the elements come back as Plate / Column / Block.
    static WoodSession pb_load(const std::filesystem::path& path);

    /// A dataset name (data/<name>.obj) or an .obj path: one Plate per consecutive outline pair, even bottom, odd top; duplicate_pts_tol > 0 removes consecutive duplicate points.
    static WoodSession obj_load(const std::filesystem::path& path, double duplicate_pts_tol = 0.0);

    /// A dataset name (data/<name>.yml) or a .yml path: its globals apply, and the obj it names becomes the scene's plates.
    static WoodSession yaml_load(const std::filesystem::path& path);

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const WoodSession& scene);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

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

    /// Crossings between elements' boundary polylines within `tolerance` mm (< 0 reads config::DISTANCE), stored as ContactType::line.
    void compute_line_contacts(double tolerance = -1.0);

    /// The joinery pipeline over the plates, in place: config::load_joint_data, adjacent_pairs, detect_joints, the three-valence links, build_joint_geometry, merge_joints; every joint onto its pair and onto both host elements as features, the merged outlines onto each plate, and the joints returned in detection order. No plate is lofted, model_geometry_mesh() or pb_dump() does that on demand.
    std::vector<WoodJoint> compute_joints(SearchType search_type = config::SEARCH_TYPE);

    /// The same with the joint data given instead of read from the sidecar files: its adjacency and three-valence groups, and its insertion vectors and joint types for plates that carry none.
    std::vector<WoodJoint> compute_joints(SearchType search_type, const JointData& data);

    /// Candidate plate pairs by position: `adjacency` when given, else the OBB and BVH search within config::DISTANCE.
    std::vector<std::pair<int, int>> adjacent_pairs(const std::vector<std::pair<int, int>>& adjacency = {}) const;

    /// face_to_face_wood on every pair, joints in pair order; a plate whose faces detection swapped is swapped in place.
    std::vector<WoodJoint> detect_joints(const std::vector<std::pair<int, int>>& pairs, SearchType search_type);

    /// Unit joinery geometry and its orientation for every joint, in order; joint_types is the per-plate per-face id table, empty rows let the solver decide.
    void build_joint_geometry(std::vector<WoodJoint>& joints, const std::vector<std::vector<int>>& joint_types);

    /// Merges every joint's cut outlines into its two plates' features.
    void merge_joints(std::vector<WoodJoint>& joints);

    /// The interaction between two elements, read from `a`; empty when there is none.
    WoodInteraction get_interaction(const std::string& a, const std::string& b) const;

    /// Stores the pair's interaction, adding the graph edge when the pair has none; a contact without a guid gets one.
    void set_interaction(const std::string& a, const std::string& b, const WoodInteraction& interaction);

    /// Every pair with an interaction, each once as (a, b) with a < b.
    std::vector<std::tuple<std::string, std::string, WoodInteraction>> get_interactions() const;

    /// The contact with this guid, face_a on its element_a; throws std::out_of_range when the scene holds none.
    const FaceContact& get_contact(const std::string& guid) const;

    /// Every contact of one class, in pair order: side_side, side_top, top_top and unknown come from compute_face_contacts, cross from compute_cross_contacts, line from compute_line_contacts.
    std::vector<FaceContact> get_contacts(ContactType type) const;

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

private:
    /// Every contact as a coloured ring under the `contacts` group of its first element.
    void add_contacts_to(const std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& groups, std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& children);

    /// Every joint's area, volumes, lines and male cuts under the `joints` group of its male element, the female cuts under the female's.
    void add_joints_to(const std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& groups, std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& children);

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// sync_geometry() and sync_interactions(), then the kernel's writer.
    void pb_dump(const std::string& filename);

    /// sync_geometry() and sync_interactions(), then the kernel's serializer.
    std::string pb_dumps();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "WoodSession(name, elements, plates, columns, blocks, edges)".
    std::string str() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Elements
    // ═══════════════════════════════════════════════════════════════════════════

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

    /// Every Beam, in objects.elements order.
    std::vector<std::shared_ptr<Beam>> beams() const { return get_elements<Beam>(); }

    /// Every element's guid in objects.elements order: the index space every ContactPair uses.
    std::vector<std::string> element_guids() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Assignment - the sidecar tables filled from points and lines placed on the plates
    // ═══════════════════════════════════════════════════════════════════════════

    /// Every plate's joint_types reset to -1 per slot (bottom, top, one per side), then each point's type written into the slot of every plate whose nearest outline segment lies within 10 x config::DISTANCE: a negative type names the bottom or top face, a positive one the side; the absolute value is stored.
    void assign_joint_types(const std::vector<session_cpp::Point>& points, const std::vector<int>& types);

    /// Every plate's insertion vectors reset to zero per slot, then each line's vector written into the side slot of every plate whose outline segment nearest the line start lies within 10 x config::DISTANCE.
    void assign_insertion_vectors(const std::vector<session_cpp::Line>& lines);

private:
    /// The slot of a plate nearest to a point, bottom 0, top 1, sides from 2, or -1 when it lies farther than sqrt(threshold); `faces` picks the face slot instead of the side slot.
    static int nearest_slot(const Plate& plate, const session_cpp::Point& point, double threshold, bool faces);

    /// An r-tree over the plates' boxes inflated by radius, keyed by position; plates without outlines are left out.
    session_cpp::SpatialRTree<int, double, 3> plate_rtree(const std::vector<std::shared_ptr<Plate>>& plates, double radius) const;

    /// Drops every contact of one class from every pair, so a recompute of that class replaces rather than accumulates.
    void erase_contacts(ContactType type);

    /// Every interaction's JSON onto its graph edge: the saved form pb_dump writes.
    void sync_interactions();

    /// Every graph edge's JSON into the store: what pb_load reads.
    void load_interactions();
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
std::string_view contact_type_name(ContactType type);

/// "ss_ip_12" / "ss_op_11" / "ss_rot_13" / "ts_20" / "cross_30" / "tt_40", or "type_<n>" for a code the table does not name.
std::string joint_type_name(int joint_type);

/// The colour of a contact class: the colour of the joint type it refines to, grey when unknown.
session_cpp::Color contact_color(ContactType type);

/// The colour of a joint type: 12 navy, 11 orange, 13 deep pink, 20 pink, 40 green, 30 yellow, grey otherwise.
session_cpp::Color joint_color(int joint_type);

} // namespace wood_session

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
