#pragma once

#include "pch.h"

#include "wood_element_beam.h"
#include "wood_element_block.h"
#include "wood_element_column.h"
#include "wood_element_plate.h"
#include "wood_config.h"
#include "wood_feature_construction.h"
#include "wood_instance.h"
#include "wood_interaction.h"
#include "wood_io.h"
#include "wood_view.h"

// ═══════════════════════════════════════════════════════════════════════════
// Joint detection pipeline
// ═══════════════════════════════════════════════════════════════════════════

/// WoodSession::compute_features over loose plates, for callers without a scene: the plates are solved in place with `settings`, the sidecars the config names apply, and every detected joint is returned.
std::vector<wood_session::FeaturePlate> get_connection_zones(
        std::vector<std::shared_ptr<wood_session::Plate>>& elements,
        const wood_session::Settings& settings = wood_session::Settings(),
        SearchType search_type = face_to_face);

namespace wood_session {

using io::pb_path;

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - a Session whose elements are plates, columns and blocks
// ═══════════════════════════════════════════════════════════════════════════

/// A Session whose elements are plates, beams, columns and blocks, and whose graph edges each key an Interaction: every contact and joint between two elements is a record in `interactions`, found by the edge's guid, and the edge itself is the only place the pair is stored. Session has no virtual method, so never delete one through a Session*. Every plate holds two geometries: element_geometry_mesh() / element_geometry_brep(), the plate alone, the loft of its two outlines, never cut; and model_geometry_mesh() / model_geometry_brep(), the plate with its joints cut in, the one to inspect. compute_features() fills the joints and the merged outlines but lofts nothing; pb_dump() lofts every plate that is not yet lofted, so the file carries the model geometry the viewer draws.
class WoodSession : public session_cpp::Session {
public:
    Settings settings; // Every tunable the solver reads; yaml_load fills it from the dataset, pb_dump writes it with the scene.
    std::map<std::string, Interaction> interactions; // The store: one record per graph edge, by the edge's guid.
    std::vector<std::pair<int, int>> adjacency; // Plate pairs by position that compute_features classifies; empty lets adjacent_pairs() search. The adjacency sidecar fills it.
    std::vector<std::vector<int>> three_valence; // Three-valence groups: the first row [instruction], 0 annen alignment, 1 vidy shadow joints; then [s0, s1, e20, e31] rows. The three_valence sidecar fills it.
    std::unordered_map<std::string, std::string> definition_keys; // Class key -> definition guid; rebuilt from the element definitions on first use, never written.

private:
    std::unordered_map<std::string, std::pair<std::string, std::string>> _edges; // Interaction guid -> the edge's (v0, v1), the pair every record refers to.

public:
    /// An empty scene; registers the four element factories with the kernel.
    WoodSession();

    /// An empty scene with a name.
    explicit WoodSession(const std::string& name);

    /// A copy whose records point at the copy.
    WoodSession(const WoodSession& other);

    /// A move whose records point at the moved-to scene.
    WoodSession(WoodSession&& other) noexcept;

    /// Copy-assign; the records point at this scene afterwards.
    WoodSession& operator=(const WoodSession& other);

    /// Move-assign; the records point at this scene afterwards.
    WoodSession& operator=(WoodSession&& other) noexcept;

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A session name (`data/<name>.pb`) or a .pb path; the elements come back as Plate / Column / Block / Beam and the interactions from field 100.
    static WoodSession pb_load(const std::filesystem::path& path);

    /// A scene from wood_proto.WoodSession bytes, which any Session reader also opens.
    static WoodSession pb_loads(const std::string& data);

    /// A dataset name (`data/<name>.obj`) or an .obj path: one Plate per consecutive outline pair, even bottom, odd top; duplicate_pts_tol > 0 removes consecutive duplicate points.
    static WoodSession obj_load(const std::filesystem::path& path, double duplicate_pts_tol = 0.0);

    /// A dataset name (`data/<name>.yml`) or a .yml path: its solver keys become the scene's settings, the obj it names its plates, its sidecars the adjacency, three-valence groups, insertion vectors and joint types.
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

    /// Coplanar face-overlap detection: a ContactFace per touching face pair, onto the pair's interaction; only elements under the same tree node at depth `level` are paired, 0 the root and so every element, 1 each branch of the root on its own.
    void compute_face_contacts(int level = 0);

    /// compute_face_contacts(level), kept for existing callers.
    void compute_contacts(int level = 0);

    /// Elements that pass through each other: plane_to_face over every pair of plates, a ContactCross per crossing.
    void compute_cross_contacts(double angle_tol = 30.0);

    /// Crossings between elements' boundary polylines within `tolerance` mm (< 0 reads settings.distance), a ContactAxis per crossing.
    void compute_line_contacts(double tolerance = -1.0);

    /// The closest axis segments of every two beams within `min_distance`, a ContactAxis per beam pair.
    void compute_axis_contacts(double min_distance);

    /// A FeatureBeam for every axis contact between two beams: four volume rectangles of `volume_length`, `cross_or_side_to_end` separating a crossing from an end contact, `flip_male` rotating the male corners; earlier beam features are replaced.
    void compute_beam_features(double volume_length, double cross_or_side_to_end, int flip_male);

    /// The joinery pipeline over world_elements<Plate>(), in place: load_sidecars, adjacent_pairs, detect_features, the three-valence links, build_feature_geometry, merge_features; every jointed instance promoted, every joint onto its pair's interaction as a FeaturePlate with its contact, onto both host elements as features, the merged outlines onto each plate, and the joints returned in detection order. No plate is lofted, model_geometry_mesh() / model_geometry_brep() or pb_dump() does that on demand.
    std::vector<FeaturePlate> compute_features();

    /// compute_features with the detection pass given instead of read from the settings.
    std::vector<FeaturePlate> compute_features(SearchType search_type);

    /// The four sidecars the dataset yml names onto the scene: adjacency and three_valence when the scene has none, insertion vectors and joint types onto every plate of elements, by position, that carries none.
    void load_sidecars(const std::vector<std::shared_ptr<Plate>>& elements);

    /// Candidate pairs by position in elements: `adjacency` when the scene has one, else the OBB and BVH search within config::DISTANCE.
    std::vector<std::pair<int, int>> adjacent_pairs(const std::vector<std::shared_ptr<Plate>>& elements) const;

    /// face_to_face_wood on every pair of elements, joints in pair order; a plate whose faces detection swapped is swapped in place.
    std::vector<FeaturePlate> detect_features(const std::vector<std::shared_ptr<Plate>>& elements, const std::vector<std::pair<int, int>>& pairs, SearchType search_type);

    /// Unit joinery geometry and its orientation for every joint, in order; feature_types is the per-plate per-face id table, empty rows let the solver decide.
    void build_feature_geometry(std::vector<std::shared_ptr<Plate>>& elements, std::vector<FeaturePlate>& joints, const std::vector<std::vector<int>>& feature_types);

    /// Merges every joint's cut outlines into its two plates' features.
    void merge_features(const std::vector<std::shared_ptr<Plate>>& elements, std::vector<FeaturePlate>& joints);

    // ═══════════════════════════════════════════════════════════════════════════
    // Interactions
    // ═══════════════════════════════════════════════════════════════════════════

    /// The interaction of two elements, made with its graph edge when the pair has none; the edge's guid is the record's key and is written on both stored copies of the edge.
    Interaction& add_interaction(const std::string& a, const std::string& b);

    /// Merge a record into the pair: contacts are relative to (a, b), feature.contact indexes this incoming record's contacts, and a supplied structure replaces the existing one. Returns the stored record with the edge's identity. Invalid contact indices or plate endpoints throw before mutation.
    Interaction& add_interaction(const std::string& a, const std::string& b, Interaction interaction);

    /// Add a contact to the pair, oriented from (a, b); coincident contacts are reused.
    Interaction& add_interaction(const std::string& a, const std::string& b, InteractionContact contact);

    /// Add a feature to the pair; contact is -1 or an index in its existing contacts. Plate endpoints may be omitted; beam volumes follow (a, b). Stores host features without running the solver or applying cuts.
    Interaction& add_interaction(const std::string& a, const std::string& b, InteractionFeature feature);

    /// Store or replace the pair's structural record.
    Interaction& add_interaction(const std::string& a, const std::string& b, InteractionStructure structure);

    /// True when the pair has a graph edge in either order, including a bare edge awaiting payload.
    bool has_interaction(const std::string& a, const std::string& b) const;

    /// Remove the edge, its stored record and its hosted contact/joint features; no-op when absent. Already merged geometry is not recomputed.
    void remove_interaction(const std::string& a, const std::string& b);

    /// Add an interaction using element identities.
    Interaction& add_interaction(const session_cpp::Element& a, const session_cpp::Element& b) { return add_interaction(a.guid(), b.guid()); }

    /// Add an interaction using element handles; null handles throw std::invalid_argument.
    Interaction& add_interaction(const std::shared_ptr<session_cpp::Element>& a, const std::shared_ptr<session_cpp::Element>& b) {
        if (!a || !b)
            throw std::invalid_argument("WoodSession::add_interaction: null element");
        return add_interaction(*a, *b);
    }

    /// Add an interaction using element identities.
    Interaction& add_interaction(const session_cpp::Element& a, const session_cpp::Element& b, Interaction data) { return add_interaction(a.guid(), b.guid(), std::move(data)); }

    /// Add an interaction using element handles; null handles throw std::invalid_argument.
    Interaction& add_interaction(const std::shared_ptr<session_cpp::Element>& a, const std::shared_ptr<session_cpp::Element>& b, Interaction data) {
        if (!a || !b)
            throw std::invalid_argument("WoodSession::add_interaction: null element");
        return add_interaction(*a, *b, std::move(data));
    }

    /// Add an interaction using element identities.
    Interaction& add_interaction(const session_cpp::Element& a, const session_cpp::Element& b, InteractionContact data) { return add_interaction(a.guid(), b.guid(), std::move(data)); }

    /// Add an interaction using element handles; null handles throw std::invalid_argument.
    Interaction& add_interaction(const std::shared_ptr<session_cpp::Element>& a, const std::shared_ptr<session_cpp::Element>& b, InteractionContact data) {
        if (!a || !b)
            throw std::invalid_argument("WoodSession::add_interaction: null element");
        return add_interaction(*a, *b, std::move(data));
    }

    /// Add an interaction using element identities.
    Interaction& add_interaction(const session_cpp::Element& a, const session_cpp::Element& b, InteractionFeature data) { return add_interaction(a.guid(), b.guid(), std::move(data)); }

    /// Add an interaction using element handles; null handles throw std::invalid_argument.
    Interaction& add_interaction(const std::shared_ptr<session_cpp::Element>& a, const std::shared_ptr<session_cpp::Element>& b, InteractionFeature data) {
        if (!a || !b)
            throw std::invalid_argument("WoodSession::add_interaction: null element");
        return add_interaction(*a, *b, std::move(data));
    }

    /// Add an interaction using element identities.
    Interaction& add_interaction(const session_cpp::Element& a, const session_cpp::Element& b, InteractionStructure data) { return add_interaction(a.guid(), b.guid(), std::move(data)); }

    /// Add an interaction using element handles; null handles throw std::invalid_argument.
    Interaction& add_interaction(const std::shared_ptr<session_cpp::Element>& a, const std::shared_ptr<session_cpp::Element>& b, InteractionStructure data) {
        if (!a || !b)
            throw std::invalid_argument("WoodSession::add_interaction: null element");
        return add_interaction(*a, *b, std::move(data));
    }

    /// has_interaction using element identities.
    bool has_interaction(const session_cpp::Element& a, const session_cpp::Element& b) const { return has_interaction(a.guid(), b.guid()); }

    /// has_interaction using element handles; null handles act as an absent pair.
    bool has_interaction(const std::shared_ptr<session_cpp::Element>& a, const std::shared_ptr<session_cpp::Element>& b) const { return a && b && has_interaction(*a, *b); }

    /// remove_interaction using element identities.
    void remove_interaction(const session_cpp::Element& a, const session_cpp::Element& b) { return remove_interaction(a.guid(), b.guid()); }

    /// remove_interaction using element handles; null handles act as an absent pair.
    void remove_interaction(const std::shared_ptr<session_cpp::Element>& a, const std::shared_ptr<session_cpp::Element>& b) { if (a && b) remove_interaction(*a, *b); }

    /// The interaction of two elements, or null when the pair has none.
    Interaction* get_interaction(const std::string& a, const std::string& b);

    /// The interaction of two elements, or null when the pair has none.
    const Interaction* get_interaction(const std::string& a, const std::string& b) const;

    /// The interaction of two elements, or null when absent.
    Interaction* get_interaction(const session_cpp::Element& a, const session_cpp::Element& b) { return get_interaction(a.guid(), b.guid()); }

    /// The interaction of two elements, or null when absent.
    const Interaction* get_interaction(const session_cpp::Element& a, const session_cpp::Element& b) const { return get_interaction(a.guid(), b.guid()); }

    /// The interaction with this guid; throws std::out_of_range when the scene holds none.
    const Interaction& get_interaction(const std::string& guid) const;

    /// The edge an interaction sits on, (v0, v1): the first and second element every record of it refers to; empty strings when the scene does not hold it.
    std::pair<std::string, std::string> edge_of(const Interaction& interaction) const;

    /// Stores a contact between two elements, oriented to the pair's edge, and returns its guid, a new one also onto the edge's first element as a "contact" feature; one that coincides with a stored contact returns that one's guid.
    std::string add_contact(const std::string& a, const std::string& b, InteractionContact contact);

    /// Stores a solved joint on its pair's interaction: the contact it was solved from (a ContactCross for a cross joint), then the FeaturePlate, and puts its two sides onto the host elements as "joint" features in the colour of its type; returns the feature's guid.
    std::string add_feature(const FeaturePlate& joint);

    /// True when every feature has a guid and a valid optional contact index, and every plate feature's own copy of its pair and contact agrees with the edge and the stored contact.
    bool consistent() const;

    /// Every contact in the scene, in interaction order.
    std::vector<InteractionContact> get_contacts() const;

    /// Every feature in the scene, in interaction order.
    std::vector<InteractionFeature> get_features() const;

    /// Every plate feature as a working joint: the pair from its edge, the contact from its interaction, the features and their guids.
    std::vector<FeaturePlate> get_plate_features() const;

    /// The joint features the interactions hold for one element: the side of each feature whose host it is.
    std::vector<session_cpp::ElementFeature> get_element_features(const std::string& guid) const;

    /// Shows or hides every feature of one type ("contact", "joint", "outline", ...) on every element.
    void set_features_visible(std::string_view feature_type, bool visible);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// pb_dumps() to a file; every stale element computes its geometry as it is written.
    void pb_dump(const std::string& filename);

    /// The scene as wood_proto.WoodSession bytes, every stale element computing its geometry as it is written: the kernel's Session fields, then the interactions in guid order at field 100.
    std::string pb_dumps();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// The Session block with a third section listing the joints on every edge.
    std::string str() const;

    /// "WoodSession(name, elements, plates, columns, blocks, definitions, instances, interactions)".
    std::string repr() const;

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

    /// Every element, in insertion order; the list objects.elements holds, no copy.
    const std::vector<std::shared_ptr<session_cpp::Element>>& elements() const { return *objects.elements; }

    /// Every Plate, in objects.elements order.
    std::vector<std::shared_ptr<Plate>> plates() const { return get_elements<Plate>(); }

    /// Every Column, in objects.elements order.
    std::vector<std::shared_ptr<Column>> columns() const { return get_elements<Column>(); }

    /// Every Block, in objects.elements order.
    std::vector<std::shared_ptr<Block>> blocks() const { return get_elements<Block>(); }

    /// Every Beam, in objects.elements order.
    std::vector<std::shared_ptr<Beam>> beams() const { return get_elements<Beam>(); }

    /// The guids of world_elements(), in order: the index space detection works in.
    std::vector<std::string> element_guids() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Instances
    // ═══════════════════════════════════════════════════════════════════════════

    /// Session::add_definition for any geometry.
    using session_cpp::Session::add_definition;

    /// Session::add_instance for a kernel InstanceRef.
    using session_cpp::Session::add_instance;

    /// Session::add_definition for an element in its own frame under a class key: the guid already stored under key when key was seen, else the new one.
    std::string add_definition(std::shared_ptr<session_cpp::Element> definition, const std::string& key);

    /// A light placement of an element definition with its own guid, named name or else as the definition; nullptr when definition_guid names no element definition or xform mirrors.
    std::shared_ptr<session_cpp::TreeNode> add_instance(
        const std::string& definition_guid,
        const session_cpp::Xform& xform,
        const std::string& name = "",
        std::shared_ptr<session_cpp::TreeNode> parent = nullptr
    );

    /// A light world copy of the element or instance guid names, placed by world: the stored element's or the definition's parameters, features, outlines and planes moved, an instance's guid, name and features; lofts nothing but a stored solid never lofted yet; nullptr when there is none or world mirrors.
    std::shared_ptr<session_cpp::Element> world_view(const std::string& guid, const session_cpp::Xform& world) const;

    /// Every element and element instance as world geometry for the passes, in list order until the session holds an element definition and in tree order from then on, so no conversion moves an index: an element placed by identity is itself, anything else a world_view; keep, when given, picks by the stored type before any view is built.
    std::vector<std::shared_ptr<session_cpp::Element>> world_elements(const std::function<bool(const session_cpp::Element&)>& keep = nullptr) const;

    /// world_elements() of type T, instances included as views of T.
    template <class T>
    std::vector<std::shared_ptr<T>> world_elements() const {

        std::vector<std::shared_ptr<T>> out;
        for (const std::shared_ptr<session_cpp::Element>& element : world_elements([](const session_cpp::Element& stored) { return dynamic_cast<const T*>(&stored) != nullptr; }))
            out.push_back(std::static_pointer_cast<T>(element));

        return out;
    }

    /// Puts a feature given in world coordinates onto the element or instance guid names, moved into its own frame, guid kept.
    void host_feature(const std::string& guid, session_cpp::ElementFeature feature);

    /// Replaces each element key_of finds a class for by an instance of that class's definition, keeping guid, name, tree node, edges with their guids, so every interaction stays found, and its contact and joint features; recorded only inside a transaction the caller opened, so a build-time dedup holds no undo copies; returns the number made.
    size_t instance_by_key(const std::function<std::optional<std::pair<std::string, session_cpp::Xform>>(const session_cpp::Element&)>& key_of = element_key);

    /// Writes a pass's world view back: an instance is exploded, its edges keeping their guids, then the element under its guid replaced by the view moved into its own frame; a stored element passed as its own view stays as it is.
    void promote(const std::shared_ptr<session_cpp::Element>& view);

    /// Drops every contact of one kind ("face", "axis", "cross") from every interaction, so a recompute of that kind replaces rather than accumulates; the features' contact indices follow.
    void erase_contacts(std::string_view kind);

    /// The guid -> edge index over the graph, after a load or a merge.
    void index_edges();

    /// Stores a contact already oriented to the interaction's edge and returns its index; a new one also goes onto the edge's first element as a "contact" feature.
    int place_contact(Interaction& interaction, InteractionContact contact);

    /// Every stored record pointed at this scene, after a copy or a move.
    void claim_records();
};

} // namespace wood_session
