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

/// Topology class of a face contact, derived from the two face indices alone.
/// Available on any element type, because it needs no geometry beyond the plate
/// face convention (index < 2 = outer face, >= 2 = side face).
///
/// NOT the same vocabulary as WoodJoint::joint_type. That one is the refined
/// solver code (11/12/13/20/30/40) and needs plate geometry - dihedral angle,
/// alignment chords, thickness - to compute. ContactType is what a contact can
/// say about itself; joint_type is what the solver decided afterwards. The two
/// spaces do not even agree numerically: ContactType::side_top is 1, the
/// top-to-side joint code is 20.
enum class ContactType : int {
    unknown   = -1,  ///< no plate face convention - every BlockElement contact
    side_side = 0,   ///< both faces are sides     (refines to 11 / 12 / 13)
    side_top  = 1,   ///< one side, one outer face (refines to 20)
    top_top   = 2,   ///< both outer faces         (refines to 40)
};

// ═══════════════════════════════════════════════════════════════════════════
// TaggedElement — the kernel half a wood object holds
// ═══════════════════════════════════════════════════════════════════════════

/// An Element with a domain tag. The kernel has no setter for element_type / element_data,
/// only the two virtuals, so a tagged element is a derived one. Held by shared_ptr and
/// handed to a Session as the same object: never copied, so the guid is never re-minted.
class TaggedElement final : public session_cpp::Element {
public:
    TaggedElement(const std::string& name, std::string type)
        : session_cpp::Element(name), _type(std::move(type)) {}
    /// Wraps a loaded element once, keeping its guid.
    TaggedElement(const session_cpp::Element& base, std::string type, std::string data)
        : session_cpp::Element(base), _type(std::move(type)), _data(std::move(data)) {
        guid() = base.guid();
    }

    std::string element_type_name() const override { return _type; }
    std::string element_data_dumps() const override { return _data; }
    void set_element_data(std::string data) { _data = std::move(data); }

private:
    std::string _type;
    std::string _data;
};

/// One face pair in contact: the two faces, the class, the overlap region (closed, in the
/// first face's plane). Which ELEMENTS is on the graph edge, by guid, not here.
///
/// Lives here rather than in wood_face_to_face.h because WoodJoint embeds one
/// by value, and that header includes wood_session.h, which includes this one.
struct FaceContact {
    int face_a = 0;
    int face_b = 0;
    ContactType type = ContactType::unknown;
    session_cpp::Polyline area{std::vector<session_cpp::Point>{}};
};

/// face_contacts() output: one element pair as positions in the vector it was given, and
/// every overlap polygon between them. A scene turns this into an edge and a WoodContact.
struct ContactPair {
    int element_a = -1;
    int element_b = -1;
    std::vector<FaceContact> faces;
};

/// Every overlap region between ONE pair of elements; which pair is the graph edge. On the
/// wire the rings are ElementFeature outlines (verbatim), the mesh is for drawing only.
struct WoodContact {
    WoodContact();
    explicit WoodContact(std::vector<FaceContact> faces, const std::string& name = "contact");

    static constexpr const char* ELEMENT_TYPE = "Contact";

    std::shared_ptr<TaggedElement> element;
    std::vector<FaceContact> faces;

    session_cpp::Mesh mesh() const;
    void sync_element() const;
    std::shared_ptr<session_cpp::Element> to_element() const;
    static WoodContact from_element(const session_cpp::Element& e);

    std::string str() const;
    friend std::ostream& operator<<(std::ostream& os, const WoodContact& c);
};

/// A read-only view of any element, for contact detection over a MIXED set.
///
/// Detection needs three things from an element - its face outlines, their planes, and a
/// name to filter on - plus one bit: whether the plate face convention applies, which is
/// what lets a contact be classified side/top. A WoodElement has it, a WoodColumn and a
/// BlockElement do not. Wrapping all three in one view is what lets a model of 153 plates,
/// 4 columns and 80 solids go through a single detection pass.
///
/// Holds POINTERS into the elements it views. It must not outlive them, and the vectors it
/// points into must not be reallocated while it is alive.
struct ContactElement {
    const std::vector<session_cpp::Polyline>* polylines = nullptr;
    const std::vector<session_cpp::Plane>*    planes    = nullptr;
    const std::string*                        name      = nullptr;
    bool plate_convention = false;
};

struct WoodJoint {
    WoodJoint();

    /// The two elements, by guid - a male, b female; swapped by the solver, so not ordered.
    /// index_of() turns one into a position.
    std::string element_a;
    std::string element_b;
    /// Which faces touched, and where.
    FaceContact contact;
    /// Type-30 (cross) joints only: the SECOND side face of each element that
    /// the crossing involves, from CrossJoint::face_ids_a/.face_ids_b. Every
    /// other joint has one face per element and leaves this at {-1,-1}.
    std::array<int, 2> cross_faces{-1, -1};
    /// Refined solver code: 11/12/13 side-side, 20 top-side, 30 cross, 40
    /// top-top. See ContactType above - a different vocabulary, not this one.
    int joint_type;
    std::string name;
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
    // (element_a, detected on face contact.face_a), [1] the female side
    // (element_b, contact.face_b). Identity lives here - element_features[k].guid()
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
    /// Value of `element_type` a stored joint is written under, beside "Plate" / "Contact".
    static constexpr const char* ELEMENT_TYPE = "Joint";
    /// Null while the joint is only the solver's, which copies joints freely; to_element()
    /// creates it on first use. A cache of this joint, hence mutable.
    mutable std::shared_ptr<TaggedElement> element;
    /// Geometry the contact area, features the two host sides, element_data jsondump() minus
    /// those features.
    std::shared_ptr<session_cpp::Element> to_element() const;
    static WoodJoint from_element(const session_cpp::Element& e);

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
    /// `name` is the plate's type flag: face_contacts() filters on it.
    WoodElement(const session_cpp::Polyline& bot, const session_cpp::Polyline& top,
                const std::string& name = "plate");

    /// Value of `element_type` this plate is written under. A DOMAIN name, not this
    /// struct's name: the tag says what the thing is, so a producer that has never heard
    /// of wood (compas_tf) and a consumer that has (wood) agree on one vocabulary, and
    /// wood's class names stay free to change. LEGACY_ELEMENT_TYPE is the name wood wrote
    /// before that, still accepted on read.
    static constexpr const char* ELEMENT_TYPE = "Plate";
    static constexpr const char* LEGACY_ELEMENT_TYPE = "WoodElement";

    /// The kernel half. Identity (guid, name) is authoritative here; everything else on it
    /// mirrors the wood fields below and is refreshed by sync_element(). Read it after a
    /// sync, or take a fresh copy with to_element(). Serialize through WoodElement, not
    /// through this member: only WoodElement knows the element_type / element_data pair.
    /// The kernel half - SHARED, not owned by value. This is the same object a Session
    /// holds, so adding it to one copies nothing and its guid is the guid on the wire.
    std::shared_ptr<TaggedElement> element;

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
    void sync_element() const;
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

/// An element for CONTACT DETECTION only, and a SOLID is all it is: the mesh in
/// `element`, one n-gon face per closed loop. `polylines` / `planes` are that solid's
/// face outlines (Mesh::face_outlines(), via Element::polylines()) and one plane each,
/// cached because the O(faces²) scan reads them per candidate pair.
///
/// WoodElement carries the plate convention the joint classifier depends on -
/// polylines[0] is the top face, [1] the bottom, [2..] the sides, in that
/// order, plus thickness, insertion vectors and merged features. That
/// convention is exactly what loose geometry does NOT have: a list of closed
/// loops off a brep says nothing about which loop is which, and neither does
/// mesh face order.
///
/// BlockElement drops all of it. It is enough for adjacency_search,
/// faces_coplanar and face_overlap_area - which only ever read `polylines` and
/// `planes` - and deliberately not enough for face_to_face_wood, which needs
/// the ordering to tell a side joint from a top joint.
struct BlockElement {
    BlockElement();

    /// Builds the solid from `loops` - one face per loop, vertices unwelded - and reads
    /// the face views back off it. Loops with fewer than 3 points are dropped.
    /// `name` is the block's type flag - "column", "inner_ribs" - and face_contacts()
    /// filters on it.
    explicit BlockElement(const std::vector<session_cpp::Polyline>& loops,
                          const std::string& name = "block");

    static constexpr const char* ELEMENT_TYPE = "Solid";
    static constexpr const char* LEGACY_ELEMENT_TYPE = "BlockElement";

    /// The kernel half, and the block itself: its geometry is the solid, so a block needs
    /// no payload beyond the mesh to come back whole. Identity (guid, name) lives here.
    /// The kernel half - SHARED, not owned by value. This is the same object a Session
    /// holds, so adding it to one copies nothing and its guid is the guid on the wire.
    std::shared_ptr<TaggedElement> element;

    /// The solid's face outlines and their planes; refreshed by sync_faces().
    std::vector<session_cpp::Polyline> polylines;
    std::vector<session_cpp::Plane>    planes;

    /// The solid. Empty when the element carries no mesh.
    session_cpp::Mesh mesh() const;

    /// Refresh `polylines` / `planes` from the solid, after replacing the geometry.
    void sync_faces();
    /// Nothing to do - the solid in `element` IS the block.
    void sync_element() const;
    std::shared_ptr<session_cpp::Element> to_element() const;
    /// Any Element whose geometry is a Mesh: its face outlines become the block's faces.
    /// An element with no mesh degrades to an empty block.
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

/// A column: a solid that knows its own axis.
///
/// For CONTACT detection a column is exactly a BlockElement - `polylines` / `planes` are
/// the solid's faces, and it has no top/bottom convention, so every contact it takes part
/// in is ContactType::unknown. What it adds is `axis` and `section`, which is what a
/// plate-to-column joint needs and a bare mesh cannot supply: the centreline gives the
/// notch direction, the section the stock it is cut from.
///
/// Nothing in wood consumes `axis` / `section` yet. They are carried because the producer
/// has them and a mesh cannot be reverse-engineered back into them.
struct WoodColumn {
    WoodColumn();

    static constexpr const char* ELEMENT_TYPE = "Column";

    /// The kernel half - identity (guid, name) and the solid.
    /// The kernel half - SHARED, not owned by value. This is the same object a Session
    /// holds, so adding it to one copies nothing and its guid is the guid on the wire.
    std::shared_ptr<TaggedElement> element;

    /// Centreline, base to head, in world space.
    session_cpp::Line axis;
    /// Closed cross-section outline about the axis base. Empty when the producer had none.
    session_cpp::Polyline section;

    /// The solid's face outlines and their planes; refreshed by sync_faces().
    std::vector<session_cpp::Polyline> polylines;
    std::vector<session_cpp::Plane>    planes;

    session_cpp::Mesh mesh() const;
    void sync_faces();
    std::shared_ptr<session_cpp::Element> to_element() const;
    /// An Element tagged "Column": the mesh becomes the faces, `element_data`
    /// {"axis","section"} the axis and section. A missing payload leaves those default and
    /// still yields a usable solid.
    static WoodColumn from_element(const session_cpp::Element& e);

    std::string str() const;
    friend std::ostream& operator<<(std::ostream& os, const WoodColumn& e);
};

/// Position of the element with this guid, or -1. A linear scan: the vectors are small.
int index_of(const std::vector<WoodElement>& elements, const std::string& guid);

} // namespace wood_session
