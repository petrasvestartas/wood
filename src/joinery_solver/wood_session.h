#pragma once

#include "pch.h"

#include "wood_element_beam.h"
#include "wood_element_joint.h"
#include "wood_element_joint_plate.h"
#include "wood_element_joint_beam.h"
#include "wood_element_pin.h"
#include "wood_element_connector_part.h"
#include "wood_element_block.h"
#include "wood_element_column.h"
#include "wood_element_cut_plane.h"
#include "wood_element_beam_variable.h"
#include "wood_element_support.h"
#include "wood_element_plate.h"
#include "wood_config.h"
#include "wood_feature_construction.h"
#include "wood_instance.h"
#include "wood_interaction.h"
#include "wood_io.h"
#include "wood_view.h"

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Joint detection pipeline
// ═══════════════════════════════════════════════════════════════════════════

namespace wood_session {

/// The solid cuts a plate, beam, column, block or connector carries, in its own frame; null for any other element.
const std::vector<InteractionFeatureSolid>* solid_features_of(const Element& element);

}

/// WoodSession::compute_features over loose plates, solved in place with `settings`; returns every detected joint.
std::vector<wood_session::InteractionFeaturePlate> get_connection_zones(
        std::vector<std::shared_ptr<wood_session::Plate>>& elements,
        const wood_session::Settings& settings = wood_session::Settings(),
        SearchType search_type = face_to_face);

namespace wood_session {

using io::pb_path;

/// True when the stored element is a T.
template <class T>
bool is_type(const Element& element) {
    return dynamic_cast<const T*>(&element) != nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - a Session whose elements are plates, columns and blocks
// ═══════════════════════════════════════════════════════════════════════════

/// A Session whose elements are plates, beams, columns and blocks, joined by interactions on its graph edges.
class WoodSession : public Session {
public:
    Settings settings; // Every tunable the solver reads; yaml_load fills it from the dataset, pb_dump writes it with the scene.
    std::vector<std::pair<int, int>> adjacency; // Plate pairs by position that compute_features classifies; empty lets adjacent_pairs() search. yaml_load fills it from the adjacency sidecar; pb_dump writes it.
    std::vector<std::array<int, 2>> borders; // Plate and side face of every boundary joint, the self-adjacency rows `v v f f` of the adjacency sidecar; compute_features makes a family 60 joint on each.
    std::vector<std::vector<int>> three_valence; // Three-valence groups: the first row [instruction], 0 annen alignment, 1 vidy shadow joints; then [s0, s1, e20, e31] rows. yaml_load fills it from the three_valence sidecar; pb_dump writes it.
    std::unordered_map<std::string, std::string> definition_keys; // Class key -> definition guid; rebuilt from the element definitions on first use, never written.

    /// An empty scene; registers the element and interaction factories with the kernel.
    WoodSession();

    /// An empty scene with a name.
    explicit WoodSession(const std::string& name);

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A session name (`data/<name>.pb`) or a .pb path; the elements come back as Plate / Column / Block / Beam and the interactions as their wood types.
    static WoodSession pb_load(const std::filesystem::path& path);

    /// A scene from wood_proto.WoodSession bytes, which any Session reader also opens.
    static WoodSession pb_loads(const std::string& data);

    /// A dataset name (`data/<name>.obj`) or an .obj path: one Plate per consecutive outline pair.
    static WoodSession obj_load(const std::filesystem::path& path, double duplicate_pts_tol = 0.0);

    /// A dataset name (`data/<name>.yml`) or a .yml path, with its settings, plates and sidecars.
    static WoodSession yaml_load(const std::filesystem::path& path);

    /// Session::jsonload, the elements and interactions as their wood types.
    static WoodSession jsonload(const nlohmann::json& data);

    /// Session::file_json_loads as a WoodSession.
    static WoodSession file_json_loads(const std::string& json_string);

    /// Session::file_json_load as a WoodSession.
    static WoodSession file_json_load(const std::string& filename);

    /// Session::from_proto as a WoodSession; the settings live only in the wood_proto bytes pb_loads reads.
    static WoodSession from_proto(const session_proto::Session& proto);

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const WoodSession& scene);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Removes the joints compute_features and compute_beam_features generated; contacts and user joints stay.
    void clear_features();

    /// Coplanar face-overlap detection: an InteractionContactFace per touching face pair within tree depth `level`.
    void compute_face_contacts(int level = 0);

    /// The largest face contact of the pair, nullptr when disjoint; plate contacts include joinery volumes.
    std::shared_ptr<InteractionContactFace> compute_face_contact(std::shared_ptr<Element> source, std::shared_ptr<Element> target);

    /// The border contact of a plate's side face, as 2024's border_to_face made it for a self-adjacency: the side quad as the polygon, the
    /// average of its two side edges as both lines, and two thin rectangles across the thickness around that line as the volumes, a quarter of
    /// the half thickness each way along the face's normal; nullptr for an outer face or a plate without that side.
    static std::shared_ptr<InteractionContactFace> compute_border_contact(const Plate& plate, int face);

    /// Joint types by points, as the plugin's dots set them: on every plate a point snaps to the side face whose middle line, between its
    /// bottom and top edges, lies nearest and within snap_radius, or for a negative type to the bottom or top face whose outline is nearer,
    /// and writes the absolute value of its type into that plate's feature_types, the table a *_joints_types.txt sidecar gives; a face
    /// several points reach takes the largest of their types, as a joint takes the larger type of its two faces. Like a sidecar, which
    /// gives every plate a row, a call with points gives every plate a table, -1 on the faces it lacked; a call with none changes nothing.
    void assign_joint_types_by_points(const std::vector<Point>& points, const std::vector<int>& types, double snap_radius);

    /// Joint types by points named as the user interface writes them, a text by each point: the name of a library design ("ss_e_ip_1",
    /// "ts_e_p_3", "ss_e_op/side_removal", JointPlate::library_id), or "" for no joint. A point takes the face it lies nearest to: a side
    /// face when it is nearer to a side face's middle line than to any bottom or top outline, else the bottom or top face, then as above.
    void assign_joint_types_by_points(const std::vector<Point>& points, const std::vector<std::string>& names, double snap_radius);

    /// Insertion vectors by lines: on every plate a line's start snaps to the side face whose middle line lies nearest and within
    /// snap_radius, and its direction becomes that face's insertion vector, the table a *_insertion_vectors.txt sidecar gives; other faces
    /// keep theirs. Like a sidecar, a call with lines gives every plate a table, zero on the faces it lacked; a call with none changes nothing.
    void assign_insertion_vectors_by_lines(const std::vector<Line>& lines, double snap_radius);

    /// Elements that pass through each other: plane_to_face over every pair of plates, an InteractionContactCross per crossing.
    void compute_cross_contacts(double angle_tol = 30.0);

    /// The crossing of two plates that pass through each other, nullptr when they do not cross within angle_tol degrees.
    std::shared_ptr<InteractionContactCross> compute_cross_contact(const std::shared_ptr<Plate>& a, const std::shared_ptr<Plate>& b, double angle_tol = 30.0) const;

    /// Crossings between elements' boundary polylines within `tolerance` mm (< 0 reads settings.distance), an InteractionContactAxis per crossing.
    void compute_line_contacts(double tolerance = -1.0);
    void compute_lines_contacts(double tolerance = -1.0) { compute_line_contacts(tolerance); }

    using Session::merge;
    using Session::graft;

    /// Session::merge, with both sides' plate adjacency and three-valence groups renumbered to the merged order.
    void merge(const WoodSession& other);

    /// Session::graft, the adjacency and three-valence groups appended as merge appends them.
    void graft(const WoodSession& other, std::shared_ptr<TreeNode> parent);

    /// Session::get_branch as a WoodSession, with its settings, adjacency and the pre-drill connectors drilling its members.
    WoodSession get_branch(const std::string& name) const;

    /// The closest axis segments of every two beams within `min_distance`, an InteractionContactAxis per beam pair.
    void compute_axis_contacts(double min_distance);

    /// An InteractionFeatureBeam for every axis contact between two beams, replacing earlier beam features.
    void compute_beam_features(double volume_length, double cross_or_side_to_end, int flip_male);

    /// The joinery pipeline over `world_elements<Plate>()`, in place, returning the joints in detection order.
    std::vector<InteractionFeaturePlate> compute_features();

    /// compute_features with the detection pass given instead of read from the settings.
    std::vector<InteractionFeaturePlate> compute_features(SearchType search_type);

    /// The four sidecars of the dataset config::load_yaml read last onto the scene, called by yaml_load only.
    void load_sidecars(const std::vector<std::shared_ptr<Plate>>& elements);

    /// Candidate pairs by position in elements: `adjacency` when the scene has one, else the OBB and BVH search within config::DISTANCE.
    std::vector<std::pair<int, int>> adjacent_pairs(const std::vector<std::shared_ptr<Plate>>& elements) const;

    /// face_to_face_wood on every pair of elements, joints in pair order; a plate whose faces detection swapped is swapped in place.
    std::vector<InteractionFeaturePlate> detect_features(const std::vector<std::shared_ptr<Plate>>& elements, const std::vector<std::pair<int, int>>& pairs, SearchType search_type);

    /// Unit joinery geometry and its orientation for every joint, in order.
    void build_feature_geometry(std::vector<std::shared_ptr<Plate>>& elements, std::vector<InteractionFeaturePlate>& joints, const std::vector<std::vector<int>>& feature_types);

    /// Merges every joint's cut outlines into its two plates' features.
    void merge_features(const std::vector<std::shared_ptr<Plate>>& elements, std::vector<InteractionFeaturePlate>& joints);

    /// True when each plate feature edge connects its joint element to the selected target, or a legacy pair matches its endpoints.
    bool consistent() const;

    /// Every contact in the scene, in edge-guid order.
    std::vector<std::shared_ptr<InteractionContact>> get_contacts() const;

    /// Every feature in the scene, in edge-guid order.
    std::vector<std::shared_ptr<InteractionFeature>> get_features() const;

    /// Every connection owned by a plate joint element, plus legacy pair features, each once.
    std::vector<InteractionFeaturePlate> get_plate_features() const;

    /// The joint features the interactions hold for one element: the side of each plate feature whose host it is.
    std::vector<ElementFeature> get_element_features(const std::string& guid) const;

    /// Shows or hides every feature of one type ("contact", "joint", "outline", ...) on every element.
    void set_features_visible(std::string_view feature_type, bool visible);

    // ═══════════════════════════════════════════════════════════════════════════
    // WoodSession - Interactions
    // ═══════════════════════════════════════════════════════════════════════════

    /// Session::get_interaction: the pair's interactions in either order.
    using Session::get_interaction;

    /// Session::has_interaction: the pair has an edge in either order.
    using Session::has_interaction;

    /// Stores the interaction on the undirected edge and applies what its kind does to the target.
    std::shared_ptr<Interaction> add_interaction(
        const std::shared_ptr<Element>& source,
        const std::shared_ptr<Element>& target,
        std::shared_ptr<Interaction> interaction
    );

    /// Session::remove_interaction, and the features its interactions put on the two elements.
    void remove_interaction(const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// pb_dumps() to a file; every stale element computes its geometry as it is written, BReps first.
    void pb_dump(const std::string& filename);

    /// The scene as wood_proto.WoodSession bytes: the Session fields, the interactions, then the settings at field 101.
    std::string pb_dumps();

    /// Puts every element's base plane, hidden, in an `attributes` group under the element.
    void sync_attributes();

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
    using Session::add;

    /// Session::add_element: the object itself, never a copy, so its guid is the guid on the wire.
    std::shared_ptr<TreeNode> add(
        std::shared_ptr<Element> element,
        std::shared_ptr<TreeNode> parent = nullptr
    );

    /// The element with this guid as T, or null when the scene does not hold it as that type.
    template <class T>
    std::shared_ptr<T> get_element(const std::string& guid) const {

        for (const std::shared_ptr<Element>& element : *objects.elements)
            if (element && element->guid() == guid)
                return std::dynamic_pointer_cast<T>(element);

        return nullptr;
    }

    /// Every element of type T, in objects.elements order.
    template <class T>
    std::vector<std::shared_ptr<T>> get_elements() const {

        std::vector<std::shared_ptr<T>> out;
        for (const std::shared_ptr<Element>& element : *objects.elements)
            if (const std::shared_ptr<T> object = std::dynamic_pointer_cast<T>(element))
                out.push_back(object);

        return out;
    }

    /// The element named name as T, or null when the scene holds none of that name and type.
    template <class T>
    std::shared_ptr<T> get_element_by_name(const std::string& name) const {

        for (const std::shared_ptr<Element>& element : *objects.elements)
            if (element && element->name == name)
                if (const std::shared_ptr<T> object = std::dynamic_pointer_cast<T>(element))
                    return object;

        return nullptr;
    }

    /// Every element of type T named `<prefix>_<n>`, n a number, in objects.elements order.
    template <class T>
    std::vector<std::shared_ptr<T>> get_elements_numbered(const std::string& prefix) const {

        std::vector<std::shared_ptr<T>> out;
        for (const std::shared_ptr<Element>& element : *objects.elements)
            if (element && numbered(element->name, prefix))
                if (const std::shared_ptr<T> object = std::dynamic_pointer_cast<T>(element))
                    out.push_back(object);

        return out;
    }

    /// Every element of type T named `<prefix>_<i>`, `<prefix>_<i>_<j>` and so on, its place indices, in objects.elements order.
    template <class T>
    std::vector<std::shared_ptr<T>> get_elements_placed(const std::string& prefix) const {

        std::vector<std::shared_ptr<T>> out;
        for (const std::shared_ptr<Element>& element : *objects.elements)
            if (element && placed(element->name, prefix))
                if (const std::shared_ptr<T> object = std::dynamic_pointer_cast<T>(element))
                    out.push_back(object);

        return out;
    }

    /// Whether name is `<prefix>_` followed by numbers joined by underscores.
    static bool placed(const std::string& name, const std::string& prefix);

    /// Whether name is `<prefix>_<n>`, n a number.
    static bool numbered(const std::string& name, const std::string& prefix);

    /// Every live element, in insertion order; the list objects.elements holds, no copy.
    const Collection<std::shared_ptr<Element>>& elements() const {
        return *objects.elements;
    }

    /// One past the highest n of an element named `<prefix>_<n>`, 0 when there is none.
    size_t next_number(const std::string& prefix) const;

    /// Writes every cut member, connector part, pin and support as its BRep instead of its mesh, the bores and round parts exact.
    void compute_breps();

    /// Every Plate, in objects.elements order.
    std::vector<std::shared_ptr<Plate>> plates() const {
        return get_elements<Plate>();
    }

    /// Every Column, in objects.elements order.
    std::vector<std::shared_ptr<Column>> columns() const {
        return get_elements<Column>();
    }

    /// Every BeamVariable, in objects.elements order.
    std::vector<std::shared_ptr<BeamVariable>> beam_variables() const {
        return get_elements<BeamVariable>();
    }

    /// Every Support, in objects.elements order.
    std::vector<std::shared_ptr<Support>> supports() const {
        return get_elements<Support>();
    }

    /// Every Block, in objects.elements order.
    std::vector<std::shared_ptr<Block>> blocks() const {
        return get_elements<Block>();
    }

    /// Every Beam, in objects.elements order.
    std::vector<std::shared_ptr<Beam>> beams() const {
        return get_elements<Beam>();
    }

    /// The guids of world_elements(), in order: the index space detection works in.
    std::vector<std::string> element_guids() const;

    /// The pre-drilled holes of one element in world coordinates, from every pre-drill connector targeting it.
    std::vector<Line> pre_drill_lines(const std::string& guid) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Instances
    // ═══════════════════════════════════════════════════════════════════════════

    /// Session::add_definition for any geometry.
    using Session::add_definition;

    /// Session::add_instance for a kernel InstanceRef.
    using Session::add_instance;

    /// Session::add_definition for an element in its own frame under a class key, reused when key was seen.
    std::string add_definition(std::shared_ptr<Element> definition, const std::string& key);

    /// A light placement of an element definition with its own guid; nullptr when there is none or xform mirrors.
    std::shared_ptr<TreeNode> add_instance(
        const std::string& definition_guid,
        const Xform& xform,
        const std::string& name = "",
        std::shared_ptr<TreeNode> parent = nullptr
    );

    /// A light world copy of the element or instance guid names, placed by world; nullptr when there is none or world mirrors.
    std::shared_ptr<Element> world_view(const std::string& guid, const Xform& world) const;

    /// Every element and element instance as world geometry for the passes, in a stable order.
    std::vector<std::shared_ptr<Element>> world_elements(const std::function<bool(const Element&)>& keep = nullptr) const;

    /// world_elements() of type T, instances included as views of T.
    template <class T>
    std::vector<std::shared_ptr<T>> world_elements() const {

        std::vector<std::shared_ptr<T>> out;
        for (const std::shared_ptr<Element>& element : world_elements(is_type<T>))
            out.push_back(std::static_pointer_cast<T>(element));

        return out;
    }

    /// Puts a feature given in world coordinates onto the element or instance guid names, moved into its own frame, guid kept.
    void host_feature(const std::string& guid, ElementFeature feature);

    /// Replaces each element key_of finds a class for by an instance of that class's definition; returns the number made.
    size_t instance_by_key(const std::function<std::optional<std::pair<std::string, Xform>>(const Element&)>& key_of = element_key);

    /// Writes a pass's world view back onto the element or instance under its guid.
    void promote(const std::shared_ptr<Element>& view);

    /// Drops every contact of one kind ("face", "axis", "cross") from every edge and its hosted feature.
    void erase_contacts(std::string_view kind);

private:

    bool merge_deferred = false; // True while compute_features adds its joints: each plate is merged once, in 2024's joint order, after the last.

    /// add_interaction for a joint, not a plate joint, on one target, which joins the joint's targets when new.
    std::shared_ptr<Interaction> add_joint_interaction(const std::shared_ptr<Joint>& joint, const std::shared_ptr<Element>& target, std::shared_ptr<Interaction> interaction);

    /// Hosts a solid cut on the target, replacing the source's earlier one, and stores it on their edge.
    std::shared_ptr<Interaction> add_solid_interaction(const std::shared_ptr<Element>& source, const std::shared_ptr<Element>& target, const std::shared_ptr<InteractionFeatureSolid>& cut);
};

} // namespace wood_session
