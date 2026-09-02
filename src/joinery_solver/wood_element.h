// wood/wood_element.h — the wood element types, composed over the session kernel.
//
// Three types, each owning its kernel counterpart rather than deriving from it:
//
//   WoodElement  owns a session_cpp::Element          (a plate: bottom/top outlines + sides)
//   BlockElement owns a session_cpp::Element          (loose closed loops, contact detection only)
//   WoodJoint    owns two session_cpp::ElementFeature (one per host element)
//
// Composition, not inheritance, because the solver mutates the wood fields (polylines,
// planes, insertion vectors, merged outlines) thousands of times per run, and none of those
// have a home on Element. The kernel object is the plate's identity (guid, name) plus the
// state every Session consumer understands - geometry, insertion vectors, nominal
// dimensions, features - and it is refreshed from the wood fields on demand by
// sync_element() / to_element(). Serialization goes through it: a WoodElement written with
// pb_dumps() is a session_proto.Element whose `element_type` is "WoodElement" and whose
// `element_data` carries the two outlines the plate cannot be rebuilt without, so a viewer
// that has never heard of wood still draws the plate and keeps the payload on re-save.
#pragma once

#include <array>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../src/element.h"
#include "../src/line.h"
#include "../src/mesh.h"
#include "../src/plane.h"
#include "../src/point.h"
#include "../src/polyline.h"
#include "../src/vector.h"

namespace wood_session {

struct WoodJoint {
    WoodJoint();

    std::pair<int, int> el_ids;
    std::pair<std::array<int, 2>, std::array<int, 2>> face_ids;
    int joint_type;
    std::string name;
    session_cpp::Polyline joint_area;
    std::array<session_cpp::Line, 2> joint_lines;
    std::array<std::optional<session_cpp::Polyline>, 4> joint_volumes_pair_a_pair_b;
    std::array<std::vector<session_cpp::Polyline>, 2> m_outlines;
    std::array<std::vector<session_cpp::Polyline>, 2> f_outlines;
    std::array<std::vector<int>, 2> m_cut_types;
    std::array<std::vector<int>, 2> f_cut_types;
    int divisions;
    double shift;
    double length;
    double division_length;
    std::array<double, 3> scale;
    bool unit_scale;
    double unit_scale_distance;
    std::vector<int> linked_joints;
    std::vector<std::vector<std::array<int, 4>>> linked_joints_seq;
    bool link;
    bool no_orient;
    int dbg_coplanar;
    int dbg_boolean;
    std::string dbg_fail_reason;

    // ── Kernel view, by composition ────────────────────────────────────────
    //
    // The joint as each of its two host elements carries it: [0] is the male side
    // (el_ids.first, detected on face face_ids.first[0]), [1] the female side
    // (el_ids.second, face_ids.second[0]). Identity lives here - element_features[k].guid()
    // is the handle a Session consumer uses to name this side of the joint again. Copying
    // an ElementFeature mints a fresh guid, so copying a joint copies its geometry, not its
    // identity, exactly as the kernel does.
    //
    // feature_type is "joint", name is the joint-library variant when the solver set one
    // ("tt_e_p_3", "side_removal") and "joint_<type>" otherwise, face_index is the face the
    // contact was detected on, and outlines are that side's cut outlines on both plate faces
    // (m_outlines / f_outlines, face 0 then face 1, flattened - ElementFeature has one list).
    // The solver keeps its two-face split because the merge stage needs it; the feature is
    // the shape every other consumer reads. get_connection_zones syncs these before it
    // returns, so a joint it hands back is always current.
    std::array<session_cpp::ElementFeature, 2> element_features;
    void sync_features();
    /// sync_features() applied to copies: identity preserved, the joint itself untouched.
    std::array<session_cpp::ElementFeature, 2> to_features() const;

    /// The whole joint, solver fields included, as JSON. There is no protobuf message for a
    /// joint - on the wire a joint IS its two ElementFeatures, written with their elements.
    nlohmann::ordered_json jsondump() const;
    static WoodJoint jsonload(const nlohmann::json& data);
    std::string file_json_dumps() const;
    static WoodJoint file_json_loads(const std::string& json_string);
    void file_json_dump(const std::string& filename) const;
    static WoodJoint file_json_load(const std::string& filename);

    std::string str() const;
    friend std::ostream& operator<<(std::ostream& os, const WoodJoint& j);
};

struct Features {
    std::vector<session_cpp::Polyline> top;
    std::vector<session_cpp::Polyline> bottom;
};

struct WoodElement {
    WoodElement();
    WoodElement(const session_cpp::Polyline& bot, const session_cpp::Polyline& top);

    /// Value of `element_type` this plate is written under.
    static constexpr const char* ELEMENT_TYPE = "WoodElement";

    /// The kernel half. Identity (guid, name) is authoritative here; everything else on it
    /// mirrors the wood fields below and is refreshed by sync_element(). Read it after a
    /// sync, or take a fresh copy with to_element(). Serialize through WoodElement, not
    /// through this member: only WoodElement knows the element_type / element_data pair.
    session_cpp::Element element;

    std::vector<session_cpp::Polyline> polylines;
    std::vector<session_cpp::Plane>    planes;
    std::vector<session_cpp::Vector>   insertion_vectors;
    std::vector<int>                   joint_types;   // per-face codes; empty = auto
    bool reversed;
    double thickness;
    Features features;

    /// Loft polylines[0] (bottom) and polylines[1] (top) into a solid plate mesh.
    session_cpp::Mesh loft_mesh() const;

    /// NOMINAL plate box in the plate's OWN frame: outline extent in x/y, thickness in z.
    /// Measured against planes[0]'s axes rather than a world AABB, so a plate keeps the same
    /// numbers however it is oriented - a world box would report a tilted plate as thick.
    session_cpp::Vector nominal_dimensions() const;

    /// One ElementFeature per face that has a joint type, cut outlines, or both.
    /// joint_types is indexed by face - [0] bottom, [1] top, [2..] sides (wood_assign.cpp) -
    /// and features.bottom/top hold the merged cut outlines for faces 0 and 1. Two
    /// descriptions of the same thing, so they collapse into one list: the face index that
    /// used to be implied by array position becomes ElementFeature::face_index.
    /// feature_type is "joint_type_<code>" for an assigned type (the code, not a label: wood
    /// has no joint-type vocabulary, and an invented one would drift from the solver) and
    /// "cut" for outlines on a face with no assignment. Detected joints are NOT here; they
    /// are WoodJoint::element_features, attached by fill_session.
    std::vector<session_cpp::ElementFeature> face_features() const;

    /// Refresh `element` from the wood fields: loft mesh, insertion vectors, nominal
    /// dimensions, face features.
    void sync_element();
    /// A synced copy for a Session, tagged with ELEMENT_TYPE and the outline payload so it
    /// serializes as a WoodElement wherever it ends up. Same guid as `element`.
    std::shared_ptr<session_cpp::Element> to_element() const;
    /// Rebuild a plate from an Element written by to_element() - or loaded back from a
    /// Session, where it arrives as a base Element carrying element_type / element_data.
    /// Anything that is not a WoodElement degrades to an empty element with a warning on
    /// stderr, the same way the (bottom, top) constructor treats bad outlines.
    static WoodElement from_element(const session_cpp::Element& e);

    nlohmann::ordered_json jsondump() const;
    static WoodElement jsonload(const nlohmann::json& data);
    std::string file_json_dumps() const;
    static WoodElement file_json_loads(const std::string& json_string);
    void file_json_dump(const std::string& filename) const;
    static WoodElement file_json_load(const std::string& filename);

    std::string pb_dumps() const;
    static WoodElement pb_loads(const std::string& data);
    void pb_dump(const std::string& filename) const;
    static WoodElement pb_load(const std::string& filename);

    std::string str() const;
    std::string repr() const;
    friend std::ostream& operator<<(std::ostream& os, const WoodElement& e);
};

/// A minimal element for CONTACT DETECTION only: closed outlines plus one
/// plane per outline, and nothing else.
///
/// WoodElement carries the plate convention the joint classifier depends on -
/// polylines[0] is the top face, [1] the bottom, [2..] the sides, in that
/// order, plus thickness, insertion vectors and merged features. That
/// convention is exactly what loose geometry does NOT have: a list of closed
/// loops off a brep says nothing about which loop is which.
///
/// BlockElement drops all of it. It is enough for adjacency_search,
/// faces_coplanar and face_overlap_area - which only ever read `polylines` and
/// `planes` - and deliberately not enough for face_to_face_wood, which needs
/// the ordering to tell a side joint from a top joint.
struct BlockElement {
    BlockElement();

    /// One plane per loop: origin at the loop centroid, normal from
    /// Vector::average_normal (Newell). Loops with fewer than 3 points are
    /// dropped, matching WoodElement's degrade-rather-than-throw behaviour.
    explicit BlockElement(const std::vector<session_cpp::Polyline>& loops);

    static constexpr const char* ELEMENT_TYPE = "BlockElement";

    /// The kernel half; see WoodElement::element. Its geometry is mesh(): the loops are the
    /// faces, so a block needs no payload beyond the mesh to come back whole.
    session_cpp::Element element;

    std::vector<session_cpp::Polyline> polylines;
    std::vector<session_cpp::Plane>    planes;

    /// One n-gon face per loop, vertices unwelded. Nothing says the loops close up into a
    /// solid, so nothing here pretends they do.
    session_cpp::Mesh mesh() const;

    void sync_element();
    std::shared_ptr<session_cpp::Element> to_element() const;
    /// Any Element whose geometry is a Mesh: its faces become the loops. That is the
    /// contact-detection view of an arbitrary mesh element, not just of one written by
    /// to_element(). An element with no mesh degrades to an empty block.
    static BlockElement from_element(const session_cpp::Element& e);

    nlohmann::ordered_json jsondump() const;
    static BlockElement jsonload(const nlohmann::json& data);
    std::string file_json_dumps() const;
    static BlockElement file_json_loads(const std::string& json_string);
    void file_json_dump(const std::string& filename) const;
    static BlockElement file_json_load(const std::string& filename);

    std::string pb_dumps() const;
    static BlockElement pb_loads(const std::string& data);
    void pb_dump(const std::string& filename) const;
    static BlockElement pb_load(const std::string& filename);

    std::string str() const;
    friend std::ostream& operator<<(std::ostream& os, const BlockElement& e);
};

} // namespace wood_session
