#pragma once

#include "pch.h"

#include "wood_element_beam.h"
#include "wood_element_block.h"
#include "wood_element_column.h"
#include "wood_element_plate.h"
#include "wood_config.h"
#include "wood_feature_construction.h"
#include "wood_interaction.h"

// ═══════════════════════════════════════════════════════════════════════════
// Joint detection pipeline
// ═══════════════════════════════════════════════════════════════════════════

/// WoodSession::compute_joints over loose plates, for callers without a scene: the plates are solved in place with `settings`, the sidecars the config names apply, and every detected joint is returned.
std::vector<wood_session::FeaturePlate> get_connection_zones(
        std::vector<std::shared_ptr<wood_session::Plate>>& elements,
        const wood_session::Settings& settings = wood_session::Settings(),
        SearchType search_type = face_to_face);

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - a Session whose elements are plates, columns and blocks
// ═══════════════════════════════════════════════════════════════════════════

/// A Session whose elements are plates, beams, columns and blocks, and whose graph edges each key an Interaction: every contact and joint between two elements is a record in `interactions`, found by the edge's guid, and the edge itself is the only place the pair is stored. Session has no virtual method, so never delete one through a Session*. Every plate holds two geometries: element_geometry_mesh(), the plate alone, the loft of its two outlines, never cut; and model_geometry_mesh(), the plate with its joints cut in, the one to inspect. compute_joints() fills the joints and the merged outlines but lofts nothing; pb_dump() lofts every plate that is not yet lofted, so the file carries the model geometry the viewer draws.
class WoodSession : public session_cpp::Session {
public:
    Settings settings; // Every tunable the solver reads; yaml_load fills it from the dataset, pb_dump writes it with the scene.
    std::map<std::string, Interaction> interactions; // The store: one record per graph edge, by the edge's guid.
    std::vector<std::pair<int, int>> adjacency; // Plate pairs by position that compute_joints classifies; empty lets adjacent_pairs() search. The adjacency sidecar fills it.
    std::vector<std::vector<int>> three_valence; // Three-valence groups: the first row [instruction], 0 annen alignment, 1 vidy shadow joints; then [s0, s1, e20, e31] rows. The three_valence sidecar fills it.

private:
    std::unordered_map<std::string, std::pair<std::string, std::string>> _edges; // Interaction guid -> the edge's (v0, v1), the pair every record refers to.

public:
    /// An empty scene; registers the four element factories with the kernel.
    WoodSession();

    /// An empty scene with a name.
    explicit WoodSession(const std::string& name);

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A session name (data/<name>.pb) or a .pb path; the elements come back as Plate / Column / Block / Beam and the interactions from field 100.
    static WoodSession pb_load(const std::filesystem::path& path);

    /// A scene from wood_proto.WoodSession bytes, which any Session reader also opens.
    static WoodSession pb_loads(const std::string& data);

    /// A dataset name (data/<name>.obj) or an .obj path: one Plate per consecutive outline pair, even bottom, odd top; duplicate_pts_tol > 0 removes consecutive duplicate points.
    static WoodSession obj_load(const std::filesystem::path& path, double duplicate_pts_tol = 0.0);

    /// A dataset name (data/<name>.yml) or a .yml path: its solver keys become the scene's settings, the obj it names its plates, its sidecars the adjacency, three-valence groups, insertion vectors and joint types.
    static WoodSession yaml_load(const std::filesystem::path& path);

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const WoodSession& scene);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Drops every contact from every interaction, so a recompute replaces rather than accumulates; the features stay and forget their contact.
    void clear_contacts();

    /// Drops every feature from every interaction; the contacts stay.
    void clear_features();

    /// Coplanar face-overlap detection over every element: a ContactFace per touching face pair, onto the pair's interaction.
    void compute_face_contacts();

    /// compute_face_contacts(), kept for existing callers.
    void compute_contacts();

    /// Elements that pass through each other: plane_to_face over every pair of plates, a ContactCross per crossing.
    void compute_cross_contacts(double angle_tol = 30.0);

    /// Crossings between elements' boundary polylines within `tolerance` mm (< 0 reads settings.distance), a ContactAxis per crossing.
    void compute_line_contacts(double tolerance = -1.0);

    /// The closest axis segments of every two beams within `min_distance`, a ContactAxis per beam pair.
    void compute_axis_contacts(double min_distance);

    /// A FeatureBeam for every axis contact between two beams: four volume rectangles of `volume_length`, `cross_or_side_to_end` separating a crossing from an end contact, `flip_male` rotating the male corners; earlier beam features are replaced.
    void compute_beam_features(double volume_length, double cross_or_side_to_end, int flip_male);

    /// The joinery pipeline over the plates, in place: load_sidecars, adjacent_pairs, detect_joints, the three-valence links, build_joint_geometry, merge_joints; every joint onto its pair's interaction as a FeaturePlate with its contact, onto both host elements as features, the merged outlines onto each plate, and the joints returned in detection order. No plate is lofted, model_geometry_mesh() or pb_dump() does that on demand.
    std::vector<FeaturePlate> compute_joints();

    /// compute_joints with the detection pass given instead of read from the settings.
    std::vector<FeaturePlate> compute_joints(SearchType search_type);

    /// The four sidecars the dataset yml names onto the scene: adjacency and three_valence when the scene has none, insertion vectors and joint types onto every plate that carries none.
    void load_sidecars();

    /// Candidate plate pairs by position: `adjacency` when the scene has one, else the OBB and BVH search within config::DISTANCE.
    std::vector<std::pair<int, int>> adjacent_pairs() const;

    /// face_to_face_wood on every pair, joints in pair order; a plate whose faces detection swapped is swapped in place.
    std::vector<FeaturePlate> detect_joints(const std::vector<std::pair<int, int>>& pairs, SearchType search_type);

    /// Unit joinery geometry and its orientation for every joint, in order; joint_types is the per-plate per-face id table, empty rows let the solver decide.
    void build_joint_geometry(std::vector<FeaturePlate>& joints, const std::vector<std::vector<int>>& joint_types);

    /// Merges every joint's cut outlines into its two plates' features.
    void merge_joints(std::vector<FeaturePlate>& joints);

    // ═══════════════════════════════════════════════════════════════════════════
    // Interactions
    // ═══════════════════════════════════════════════════════════════════════════

    /// The interaction of two elements, made with its graph edge when the pair has none; the edge's guid is the record's key and is written on both stored copies of the edge.
    Interaction& add_interaction(const std::string& a, const std::string& b);

    /// The interaction of two elements, or null when the pair has none.
    Interaction* get_interaction(const std::string& a, const std::string& b);

    /// The interaction of two elements, or null when the pair has none.
    const Interaction* get_interaction(const std::string& a, const std::string& b) const;

    /// The interaction with this guid; throws std::out_of_range when the scene holds none.
    const Interaction& get_interaction(const std::string& guid) const;

    /// The edge an interaction sits on, (v0, v1): the first and second element every record of it refers to; empty strings when the scene does not hold it.
    std::pair<std::string, std::string> edge_of(const Interaction& interaction) const;

    /// Stores a contact between two elements, oriented to the pair's edge, and returns its guid; one that coincides with a stored contact returns that one's guid.
    std::string add_contact(const std::string& a, const std::string& b, InteractionContact contact);

    /// Stores a solved joint on its pair's interaction: the contact it was solved from (a ContactCross for a cross joint), then the FeaturePlate with its two element features, both oriented to the edge; returns the feature's guid.
    std::string add_joint(const FeaturePlate& joint);

    /// Every contact in the scene, in interaction order.
    std::vector<InteractionContact> get_contacts() const;

    /// Every feature in the scene, in interaction order.
    std::vector<InteractionFeature> get_features() const;

    /// Every plate feature as a working joint: the pair from its edge, the contact from its interaction, the features and their guids.
    std::vector<FeaturePlate> get_joints() const;

    /// The joint features the interactions hold for one element: the side of each feature whose host it is.
    std::vector<session_cpp::ElementFeature> get_element_features(const std::string& guid) const;

    /// Puts every joint feature the interactions hold back on its host element, replacing the previous ones.
    void sync_joint_features();

    /// Arranges the scene for the viewer, one group per element: the element itself, then `outlines`, `contacts` and `joints` child groups, each flag adding or leaving out that part; pb_dump writes it.
    void add_to_tree(bool with_geometry = true, bool with_outlines = true, bool with_contacts = true, bool with_joints = true);

    /// Lofts every plate whose Element slot is stale, so the file carries the model geometry; the plates stay unlofted until this runs.
    void sync_geometry() const;

private:
    /// Every contact as a coloured ring or region under the `contacts` group of its first element.
    void add_contacts_to(const std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& groups, std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& children);

    /// Every joint's area, volumes, lines and male cuts under the `joints` group of its male element, the female cuts under the female's.
    void add_joints_to(const std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& groups, std::map<std::string, std::shared_ptr<session_cpp::TreeNode>>& children);

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// sync_geometry(), then pb_dumps() to a file.
    void pb_dump(const std::string& filename);

    /// sync_geometry(), then the scene as wood_proto.WoodSession bytes: the kernel's Session fields, then the interactions in guid order at field 100.
    std::string pb_dumps();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "WoodSession(name, elements, plates, columns, blocks, interactions)".
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

    /// Every element's guid in objects.elements order: the index space detection works in.
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

    /// Drops every contact of one kind ("face", "axis", "cross") from every interaction, so a recompute of that kind replaces rather than accumulates; the features' contact indices follow.
    void erase_contacts(std::string_view kind);

    /// The guid -> edge index over the graph, after a load or a merge.
    void index_edges();
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

/// "side_side" / "side_top" / "top_top" / "unknown" - the group name a face contact of that class is filed under.
std::string_view contact_type_name(ContactType type);

/// "ss_ip_12" / "ss_op_11" / "ss_rot_13" / "ts_20" / "cross_30" / "tt_40", or "type_<n>" for a code the table does not name.
std::string joint_type_name(int joint_type);

/// The colour of a face contact class: the colour of the joint type it refines to, grey when unknown.
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
