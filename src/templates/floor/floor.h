#pragma once
#include "src/templates/floor/floor_guide.h"

// The model of the timber floor: the elements built from a FloorGuide, the contact interactions between them, the connectors made from those, and the screws.

namespace wood_floor {

/// Every contact kind, the connectors add_connectors makes by default.
const std::vector<ContactKind> CONNECTOR_CONTACTS = {ContactKind::seam_wedge, ContactKind::oculus_wedge, ContactKind::column_plate, ContactKind::block_dowels};

/// The colour of every connector node and of every part and dowel node nested under it: the Block Research Group's primary blue.
const session_cpp::Color CONNECTOR_COLOR = session_cpp::Color(33.0f / 255.0f, 150.0f / 255.0f, 234.0f / 255.0f, 1.0f, "brg_blue");

// ═══════════════════════════════════════════════════════════════════════════
// Contacts and screws
// ═══════════════════════════════════════════════════════════════════════════

/// Where the assembly screws go, as 200 mm lines in world coordinates, between members that butt.
class ScrewLines {
public:
    /// The screws of the guide's floor.
    explicit ScrewLines(const FloorGuide& guide);

    /// Outer rib k of quarter q into the seam beam it meets: two screws along the rib from the beam's seam face into the rib end, 20 mm below its top and above its bottom and either side of its axis.
    std::vector<session_cpp::Line> rib_beam(size_t q, size_t k) const;

    /// Seam beam 0 (k 0) or 2 (k 1) of quarter q into the oculus beam ending on it, along the oculus beam from the seam plane.
    std::vector<session_cpp::Line> beam_mitre(size_t q, size_t k) const;

    /// The oculus beam of quarter q into inner rib k ending on its back face, along the rib through the beam corner; throws when the bay is too narrow for them.
    std::vector<session_cpp::Line> rib_corner(size_t q, size_t k) const;

    /// Whether the inner rib screws of quarter q at end k pass the seam beam's end at the beam corner.
    bool passes_seam_beam(size_t q, size_t k, const std::vector<session_cpp::Line>& screws) const;



private:
    const FloorGuide& guide;
    const session_cpp::Xform lift; // Up from the datum to the floor.

    /// The lines lifted to the floor.
    std::vector<session_cpp::Line> lifted(const std::vector<session_cpp::Line>& lines) const;

    /// A screw at level z through a side member into the member butting on it, along the butting member's axis: the head where that axis leaves the side member's far face, the tip on towards the butting member's body.
    static session_cpp::Line along_axis(const std::array<session_cpp::Plane, 2>& butting, const session_cpp::Plane& far_face, const session_cpp::Point& butting_body, double z);

    /// A screw at level z along a rib ending on a seam beam that runs through the rib band, its axis offset across the rib, from the beam's seam face through the beam into the rib end.
    static session_cpp::Line from_seam_face(const std::array<session_cpp::Plane, 2>& rib, const std::array<session_cpp::Plane, 2>& beam, double z, double offset);

    /// The axis of a member between two faces at level z: the line midway between their traces.
    static session_cpp::Line axis(const std::array<session_cpp::Plane, 2>& faces, double z);

    /// The level of a screw in a corner's level set: down from the datum in sevenths of the depth.
    double corner_level(double levels) const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Floor
// ═══════════════════════════════════════════════════════════════════════════

/// The elements of one quarter in the scene, by family, each family in member order.
class QuarterMembers {
public:
    std::vector<std::shared_ptr<wood_session::BeamVariable>> outer_ribs; // outer_ribs_<i>_<q>.
    std::vector<std::shared_ptr<wood_session::BeamVariable>> inner_ribs; // inner_ribs_<i>_<q>.
    std::vector<std::shared_ptr<wood_session::BeamVariable>> inner_beams; // inner_beams_<i>_<q>.
    std::vector<std::shared_ptr<wood_session::Plate>> wedges; // wedges_<i>_<q>, the column blocks.
    std::vector<std::shared_ptr<wood_session::Plate>> tsections; // tsections_<i>_<q>.
    std::vector<std::vector<std::shared_ptr<wood_session::Plate>>> beds; // beds_<row>_<i>_<q>.
};

/// A column in the scene: the support and the column, carved by its six head cuts.
class ColumnModel {
public:
    std::shared_ptr<wood_session::Support> support; // support_<q>, on the slab.
    std::shared_ptr<wood_session::Column> column; // column_<q>, carved by the head cuts.
};

/// The floor model, a session built step by step from a guide, grouped by quarter: quarter_0 to quarter_3 each with its members, its column, its part of the oculus ring and its connectors and screws, and the oculus with the central plate. Every two members that touch hold a contact interaction, named by its kind and place; the connectors are made from those interactions.
class Floor : public wood_session::WoodSession {
public:
    const FloorGuide guide; // The geometry the model is built from.
    std::array<QuarterMembers, 4> quarters; // The elements of quarter q.
    std::vector<std::shared_ptr<wood_session::BeamVariable>> ring; // The four ring beams, oculus_<q>.
    std::vector<std::shared_ptr<wood_session::Plate>> oculus_plates; // The four bottom wedges oculus_4 to oculus_7 and the central plate oculus_8.
    std::vector<ColumnModel> columns; // Column q at corner q, empty until the columns are added.
    std::vector<std::shared_ptr<wood_session::JointBeam>> connectors; // Every connector added: wedges, plates, cross laps and dowels.
    std::vector<std::shared_ptr<wood_session::JointBeam>> screws; // Every screw connector added.

    /// An empty model of the guide, the session named name.
    explicit Floor(const FloorGuide& guide, const std::string& name = "floor");

    /// Not copied: the members name this session's own objects.
    Floor(const Floor&) = delete;

    /// Not assigned, as it is not copied.
    Floor& operator=(const Floor&) = delete;

    /// Adds the quarters, the oculus and the columns, then the contacts between them: every member of the floor.
    void add_members();

    /// Adds the four quarters, each lifted to bay_height and grouped by family.
    void add_quarters();

    /// Adds the oculus lifted to bay_height: ring beam q and bottom wedge q in oculus_q of quarter q, the central plate in oculus.
    void add_oculus();

    /// Adds the column at every corner.
    void add_columns();

    /// Adds the column at one corner: its support, the column, the support joint and the column's six head cuts.
    void add_column(size_t corner);

    /// Searches the contact between every two members the design joins and stores it as their interaction; members not yet in the session are skipped, a contact already there is kept.
    void add_contacts();

    /// Adds one connector per contact interaction of the kinds asked for, in the order of their names, under connectors_q of its quarter, named `<prefix>_<n>` and numbered on from those already in the session, and returns them; the two column plates of a corner get their cross lap. All are built before any is added, so a pair without its contact throws with nothing added.
    std::vector<std::shared_ptr<wood_session::JointBeam>> add_connectors(const std::vector<ContactKind>& kinds = CONNECTOR_CONTACTS);

    /// Adds the assembly screws on the members they join, after every other connector so nothing before them changes, and returns them.
    std::vector<std::shared_ptr<wood_session::JointBeam>> add_screws();

private:
    /// The quarter's group, made the first time.
    std::shared_ptr<session_cpp::TreeNode> quarter_group(size_t q);

    /// Lifts an element to the floor, names it and adds it under the group.
    void add_placed(const std::shared_ptr<session_cpp::Element>& element, const std::string& name, const std::shared_ptr<session_cpp::TreeNode>& group);

    /// A rib as a variable beam: one section per soffit point, its far corners from the second loop, so the end sections lie in the end planes.
    static std::shared_ptr<wood_session::BeamVariable> rib(const Loops& loops, const std::string& name);

    /// A four-corner member as a variable beam between the end sections over corners start and end, start[i] and end[i] on one long edge.
    static std::shared_ptr<wood_session::BeamVariable> beam(const Loops& loops, const std::array<size_t, 2>& start, const std::array<size_t, 2>& end, const std::string& name);

    /// The contact the session's search finds between two members, stored as their interaction named `<kind>_<place>`; one already there is kept, and a pair that does not touch throws naming it.
    void add_contact(ContactKind kind, const std::string& place, const std::shared_ptr<session_cpp::Element>& a, const std::shared_ptr<session_cpp::Element>& b);

    /// The connector a contact interaction gets, by its kind and the place its name ends in (quarter, then rib or block index): a wedge sized by the thicker member, a plate by the rib's thickness, or dowels.
    std::shared_ptr<wood_session::JointBeam> connector_of(ContactKind kind, const std::vector<size_t>& place, const session_cpp::Element& a, const session_cpp::Element& b, const wood_session::InteractionContactFace& contact) const;

    /// The name prefix of a connector of that kind.
    static std::string connector_prefix(ContactKind kind);

    /// Names a connector `<prefix>_<n>`, numbered on from the session, and adds it under connectors_q of quarter q in CONNECTOR_COLOR.
    void add_named_connector(const std::shared_ptr<wood_session::JointBeam>& connector, const std::string& prefix, size_t q, std::map<std::string, size_t>& numbers);

    /// The screw connector of lines through the members, the first two the joint's.
    std::shared_ptr<wood_session::JointBeam> screws_of(const std::vector<const session_cpp::Element*>& members, const std::vector<session_cpp::Line>& lines) const;
};

}
