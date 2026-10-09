#pragma once
#include "src/templates/floor/floor_guide.h"

using namespace session_cpp;
using namespace wood_session;

// The model of the timber floor: the elements built from a FloorGuide, the contact interactions between them, the connectors made from those, and the screws.

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════

/// The kinds of contact the floor's design puts between two members; each is the name of the contact interaction between them, and decides the connector that goes there.
enum class ContactKind {
    seam_wedge, // The two seam beams either side of a seam: a wedge.
    oculus_wedge, // A quarter's oculus beam and its ring beam: a wedge.
    column_plate, // A column and an outer rib: a rectangle plate; the two plates of a corner get a cross lap.
    block_dowels, // A column block and a rib: dowels.
};

// ═══════════════════════════════════════════════════════════════════════════
// Floor
// ═══════════════════════════════════════════════════════════════════════════

/// The column at corner q of the guide as a session, named `column_<q>`: the guide's sizes, `support_<q>` and its six cutter plates `column_cutters_<i>_<q>` handed to WoodSession::add_column, which glues the head on, joins the support and takes the inclined faces away. Floor::add_column grafts a copy of it into the floor.
WoodSession column(const FloorGuide& guide, size_t q);

/// The floor model, a session built step by step from a guide. The session holds every element and its tree the grouping: quarter_0 to quarter_3 each with its member families (`outer_ribs_q` > `outer_ribs_<i>_<q>` ...), its ring beam and bottom wedge (`oculus_q`), its column (`column_q`) and its connectors and screws (`connectors_q`), and `oculus` with the central plate. Every two members that touch hold a contact interaction, named by its kind and place; the connectors are made from those interactions and named `connector_<kind>_<n>`, the cross laps `connector_cross_lap_<n>`, the screws `connector_screws_<n>`. Find any of them by name with get_element_by_name or get_elements_numbered.
class Floor : public WoodSession {
public:
    static inline const std::array<std::string, 4> CONTACT_NAMES = {"seam_wedge", "oculus_wedge", "column_plate", "block_dowels"}; // The interaction name of each kind, in ContactKind order.
    static constexpr double SCREW_LENGTH = 200.0; // mm, every assembly screw.
    static constexpr double SCREW_SPACING = 8.0; // mm, the closest two screw axes may come.
    static constexpr double RIB_END_MARGIN = 20.0; // mm a seam screw sits below the rib's top and above its bottom at its end when the seam runs through the rib band.
    static constexpr double SEAM_SCREW_OFFSET = 15.0; // mm the screws of the two ribs meeting at a seam sit either side of their axes, so their heads on the seam plane stay apart.
    static constexpr double CORNER_LEVELS = 7.0; // An oculus corner's depth in sevenths: six levels, one per screw on each side of the corner.
    static constexpr std::array<std::array<double, 2>, 2> MITRE_LEVELS = {{{2.0, 5.0}, {3.0, 6.0}}}; // Per mitre k, the levels of its two screws; the two quarters' mitres at a seam put their heads on the seam plane at one point, so they differ.
    static constexpr std::array<double, 2> RIB_CORNER_LEVELS = {1.0, 4.0}; // The inner rib end screws at both corners, apart from that corner's mitre and oculus screws they cross.
    static inline const Color CONNECTOR_COLOR = Color(33.0f / 255.0f, 150.0f / 255.0f, 234.0f / 255.0f, 1.0f, "brg_blue"); // Every connector node and every part and dowel node under it: the Block Research Group's primary blue.

    const FloorGuide guide; // The geometry the model is built from; every element is in the session.

    /// The model of the guide, the session named name: the quarters, the oculus, the columns, their contacts, the connectors and the screws.
    explicit Floor(const FloorGuide& guide, const std::string& name = "floor");

    /// Not copied, as a session is not.
    Floor(const Floor&) = delete;

    /// Not assigned, as it is not copied.
    Floor& operator=(const Floor&) = delete;

    /// The screw lines of outer rib k of quarter q into the seam beam it meets: two along the rib from the beam's seam face into the rib end, RIB_END_MARGIN below its top and above its bottom and either side of its axis.
    std::vector<Line> rib_beam_screws(size_t q, size_t k) const;

    /// The screw lines of seam beam 0 (k 0) or 2 (k 1) of quarter q into the oculus beam ending on it, along the oculus beam from the seam plane.
    std::vector<Line> beam_mitre_screws(size_t q, size_t k) const;

    /// The screw lines of the oculus beam of quarter q into inner rib k ending on its back face, along the rib through the beam corner; throws when the bay is too narrow for them.
    std::vector<Line> rib_corner_screws(size_t q, size_t k) const;

private:
    /// Adds the four quarters, each lifted to bay_height and grouped by family.
    void add_quarters();

    /// Adds the oculus lifted to bay_height: ring beam q and bottom wedge q in oculus_q of quarter q, the central plate in oculus.
    void add_oculus();

    /// Adds the column at every corner.
    void add_columns();

    /// Adds the column at one corner: its support, the column, the support joint and the column's six head cuts.
    void add_column(size_t corner);

    /// Searches the contact between every two members the design joins and stores it as their interaction.
    void add_contacts();

    /// Adds one connector per contact interaction, by kind and then name, under connectors_q of its quarter, named `<prefix>_<n>`; the two column plates of a corner get their cross lap. All are built before any is added, so a pair without its contact throws with nothing added.
    void add_connectors();

    /// Adds the assembly screws on the members they join, after every other connector so nothing before them changes.
    void add_screws();

    /// The quarter's group, made the first time.
    std::shared_ptr<TreeNode> quarter_group(size_t q);

    /// A rib as a variable beam: one section per soffit point, its far corners from the second loop, so the end sections lie in the end planes.
    static std::shared_ptr<BeamVariable> rib(const std::array<Polyline, 2>& loops, const std::string& name);

    /// A four-corner member as a variable beam between the end sections over corners start and end, start[i] and end[i] on one long edge.
    static std::shared_ptr<BeamVariable> beam(const std::array<Polyline, 2>& loops, const std::array<size_t, 2>& start, const std::array<size_t, 2>& end, const std::string& name);

    /// The contact the session's search finds between two members, stored as their interaction named `<kind>_<place>`; a pair that does not touch throws naming it.
    void add_contact(ContactKind kind, const std::string& place, const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b);

    /// The connector a contact interaction gets, by its kind and the place its name ends in (quarter, then rib or block index): a wedge sized by the thicker member, a plate by the rib's thickness, or dowels.
    std::shared_ptr<JointBeam> connector_of(ContactKind kind, const std::vector<size_t>& place, const Element& a, const Element& b, const InteractionContactFace& contact) const;

    /// The name prefix of a connector of that kind: `connector_<kind>`.
    static std::string connector_prefix(ContactKind kind);

    /// The element named name as T; throws naming it when the session holds none.
    template <class T>
    std::shared_ptr<T> member(const std::string& name) const {

        const std::shared_ptr<T> element = get_element_by_name<T>(name);

        if (!element)
            throw std::runtime_error("the floor has no element named " + name);

        return element;
    }

    /// Names a connector `<prefix>_<n>`, numbered on from the session, and adds it under connectors_q of quarter q in CONNECTOR_COLOR.
    void add_named_connector(const std::shared_ptr<JointBeam>& connector, const std::string& prefix, size_t q, std::map<std::string, size_t>& numbers);

    /// The screw connector of lines through the members, the first two the joint's.
    std::shared_ptr<JointBeam> screws_of(const std::vector<const Element*>& members, const std::vector<Line>& lines) const;

    /// Whether the inner rib screws of quarter q at end k pass the seam beam's end at the beam corner.
    bool passes_seam_beam(size_t q, size_t k, const std::vector<Line>& screws) const;

    /// Lifts screw lines from the datum up to the floor.
    std::vector<Line> lifted(const std::vector<Line>& screws) const;

    /// The level of a screw in a corner's level set: down from the datum in sevenths of the depth.
    double corner_level(double levels) const;

    /// A screw at level z through a side member into the member butting on it, along the butting member's axis: the head where that axis leaves the side member's far face, the tip on towards the butting member's body.
    static Line along_axis(const std::array<Plane, 2>& butting, const Plane& far_face, const Point& butting_body, double z);

    /// A screw at level z along a rib ending on a seam beam that runs through the rib band, its axis offset across the rib, from the beam's seam face through the beam into the rib end.
    static Line from_seam_face(const std::array<Plane, 2>& rib, const std::array<Plane, 2>& beam, double z, double offset);

    /// The axis of a member between two faces at level z: the line midway between their traces.
    static Line axis(const std::array<Plane, 2>& faces, double z);
};

}
