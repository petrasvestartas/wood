#pragma once
#include "src/templates/floor/floor_guide.h"

using namespace session_cpp;
using namespace wood_session;

// The model of the timber floor: the elements built from a FloorGuide, the contact interactions between them, the connectors made from those, and the screws.

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════

/// A contact the floor's design puts between two members.
///
/// - `a`: the source member.
/// - `b`: the target member, which hosts the contact.
/// - `face`: the face they share, stored as their interaction.
struct Contact {
    std::shared_ptr<Element> a; // The source member.
    std::shared_ptr<Element> b; // The target member, which hosts the contact.
    std::shared_ptr<InteractionContactFace> face; // The face they share, stored as their interaction.
};

/// The contacts of one quarter, by the connector each gets.
///
/// - `seam_wedge`: seam beam 0 beside the next quarter's seam beam 2.
/// - `oculus_wedge`: the oculus beam on its ring beam.
/// - `column_plates[2]`: the column against outer rib k.
/// - `block_dowels[3][2]`: column block b against the rib on its side 0 or 1.
struct QuarterContacts {
    Contact seam_wedge; // Seam beam 0 beside the next quarter's seam beam 2: a wedge.
    Contact oculus_wedge; // The oculus beam's back face on its ring beam: a wedge.
    std::array<Contact, 2> column_plates; // The column against outer rib k: a rectangle plate.
    std::array<std::array<Contact, 2>, 3> block_dowels; // Column block b against the rib either side: dowels.
};

/// The connectors of one quarter, every one built from its contact before any is added.
///
/// - `seam_wedge`, `oculus_wedge`: a wedge each.
/// - `column_plates[2]`: a plate into outer rib k.
/// - `cross_lap`: the slots where the two plates cross.
/// - `block_dowels[3][2]`: dowels per block and side.
struct QuarterConnectors {
    std::shared_ptr<JointBeam> seam_wedge; // connector_seam_wedge_<n>.
    std::shared_ptr<JointBeam> oculus_wedge; // connector_oculus_wedge_<n>.
    std::array<std::shared_ptr<JointBeam>, 2> column_plates; // connector_column_plate_<n>, one per outer rib.
    std::shared_ptr<JointBeam> cross_lap; // connector_cross_lap_<n>, where the two column plates cross.
    std::array<std::array<std::shared_ptr<JointBeam>, 2>, 3> block_dowels; // connector_block_dowels_<n>, per block and side.
};

/// The assembly screws of one quarter, one screw connector per joint and side k = 0, 1, named by the member that butts, then the one it butts into.
///
/// - `outer_rib_seam_beam[2]`: outer rib k into the seam beam it ends on.
/// - `seam_beam_oculus_beam[2]`: seam beam k into the oculus beam.
/// - `oculus_beam_inner_rib[2]`: the oculus beam into inner rib k, through the seam beam when the screws pass it.
struct QuarterScrews {
    std::array<std::shared_ptr<JointBeam>, 2> outer_rib_seam_beam; // Each outer rib into the seam beam it ends on.
    std::array<std::shared_ptr<JointBeam>, 2> seam_beam_oculus_beam; // Each seam beam into the oculus beam ending on it.
    std::array<std::shared_ptr<JointBeam>, 2> oculus_beam_inner_rib; // The oculus beam into each inner rib, through the seam beam when the screws pass it.
};

// ═══════════════════════════════════════════════════════════════════════════
// Floor
// ═══════════════════════════════════════════════════════════════════════════

/// The column at corner q of the guide as a session, named `column_<q>`: the guide's sizes, `support_<q>` and its six cutter plates `column_cutters_<i>_<q>` handed to WoodSession::add_column, which glues the head on, joins the support and takes the inclined faces away. Floor::add_column grafts a copy of it into the floor.
WoodSession column(const FloorGuide& guide, size_t q);

/// The floor model, a session built step by step from a guide. The session holds every element and its tree the grouping: quarter_0 to quarter_3 each with its member families (`outer_ribs_q` > `outer_ribs_<i>_<q>` ...), its column (`column_q`) and its connectors and screws (`connectors_q`), and `oculus` with the four ring beams, the bottom wedges and the central plate. The oculus beam of a quarter is its inner beam along the oculus edge, `inner_beams_1_q`. Every two members that touch hold a contact interaction, named by its kind and place; the connectors are made from those interactions and named `connector_<kind>_<n>`, the cross laps `connector_cross_lap_<n>`, the screws `connector_screws_<n>`. Find any of them by name with get_element_by_name or get_elements_numbered.
///
/// Fields: `guide`, the FloorGuide it is built from, and the constants `SCREW_LENGTH`, `SCREW_SPACING`, `RIB_END_MARGIN`, `SEAM_SCREW_OFFSET`, `CORNER_LEVELS`, `SEAM_BEAM_OCULUS_BEAM_LEVELS`, `OCULUS_BEAM_INNER_RIB_LEVELS`, `SEAM_BEAMS`; every connector is JointBeam::CONNECTOR_COLOR.
/// Everything it builds is in the session, by name: `outer_ribs_<i>_<q>`, `inner_ribs_<i>_<q>`, `inner_beams_<i>_<q>`, `wedges_<i>_<q>`, `tsections_<i>_<q>`, `beds_<row>_<i>_<q>`, `oculus_<n>`, `column_<q>`, `support_<q>`, `connector_<kind>_<n>`, `connector_screws_<n>`.
class Floor : public WoodSession {
public:
    static constexpr double SCREW_LENGTH = 200.0; // mm, every assembly screw.
    static constexpr double SCREW_SPACING = 8.0; // mm, the closest two screw axes may come.
    static constexpr double RIB_END_MARGIN = 20.0; // mm a seam screw sits below the rib's top and above its bottom at its end when the seam runs through the rib band.
    static constexpr double SEAM_SCREW_OFFSET = 15.0; // mm the screws of the two ribs meeting at a seam sit either side of their axes, so their heads on the seam plane stay apart.
    static constexpr double CORNER_LEVELS = 7.0; // An oculus corner's depth in sevenths: six levels, one per screw on each side of the corner.
    static constexpr std::array<std::array<double, 2>, 2> SEAM_BEAM_OCULUS_BEAM_LEVELS = {{{2.0, 5.0}, {3.0, 6.0}}}; // Per side k, the levels of the two screws of the seam beam into the oculus beam; the two quarters' screws at a seam put their heads on the seam plane at one point, so their levels differ.
    static constexpr std::array<double, 2> OCULUS_BEAM_INNER_RIB_LEVELS = {1.0, 4.0}; // The levels of the two screws of the oculus beam into an inner rib, apart from the seam beam's screws they cross at that corner.
    static constexpr std::array<size_t, 2> SEAM_BEAMS = {0, 2}; // The inner beams on seam 0 and seam 1, k 0 and 1; between them inner beam 1, along the oculus edge, the oculus beam.

    const FloorGuide guide; // The geometry the model is built from; every element is in the session.

    /// The model of the guide, the session named name: the quarters, the oculus, the columns, their contacts, the connectors and the screws.
    explicit Floor(const FloorGuide& guide, const std::string& name = "floor");

    /// Not copied, as a session is not.
    Floor(const Floor&) = delete;

    /// Not assigned, as it is not copied.
    Floor& operator=(const Floor&) = delete;

    /// The screw lines of outer rib k of quarter q into the seam beam it meets: two along the rib from the beam's seam face into the rib end, RIB_END_MARGIN below its top and above its bottom and either side of its axis.
    std::vector<Line> outer_rib_seam_beam_screws(size_t q, size_t k) const;

    /// The screw lines of seam beam k of quarter q into the oculus beam ending on it, along the oculus beam from the seam plane.
    std::vector<Line> seam_beam_oculus_beam_screws(size_t q, size_t k) const;

    /// The screw lines of the oculus beam of quarter q into inner rib k ending on its back face, along the rib through the beam corner; throws when the bay is too narrow for them.
    std::vector<Line> oculus_beam_inner_rib_screws(size_t q, size_t k) const;

private:
    /// Adds the four quarters, each lifted to bay_height and grouped by family.
    void add_quarters();

    /// Adds the oculus lifted to bay_height, grouped by family under oculus: ring_beams, bottom_wedges and central_plate.
    void add_oculus();

    /// Adds the column at every corner.
    void add_columns();

    /// Adds the column at one corner: its support, the column, the support joint and the column's six head cuts.
    void add_column(size_t corner);

    /// Searches the contact between every two members the design joins, stores it as their interaction and returns it per quarter.
    std::array<QuarterContacts, 4> add_contacts();

    /// The connector of every contact: a wedge on each seam and oculus contact, a plate on each column contact with the cross lap of the two, dowels on each block contact; none is added yet, so each is built on uncut members.
    std::array<QuarterConnectors, 4> compute_connectors(const std::array<QuarterContacts, 4>& contacts) const;

    /// Adds the connectors kind by kind, quarter by quarter, under connectors_q of quarter_q, the oculus wedges under connectors of oculus, the cross laps last: each connector added, then put on its two members by add_interaction(connector, member, connector->interaction(i)).
    void add_connectors(const std::array<QuarterConnectors, 4>& connectors, const std::array<QuarterContacts, 4>& contacts);

    /// The assembly screws of every quarter, built before any is added; throws when a bay is too narrow for them.
    std::array<QuarterScrews, 4> compute_screws() const;

    /// Adds the screws quarter by quarter under connectors_q, named `connector_screws_<n>`.
    void add_screws(const std::array<QuarterScrews, 4>& screws);

    /// The quarter's group, made the first time.
    std::shared_ptr<TreeNode> quarter_group(size_t q);


    /// The connectors group of quarter q, made the first time.
    std::shared_ptr<TreeNode> connectors_group(size_t q);

    /// A rib as a variable beam: one section per soffit point, its far corners from the second loop, so the end sections lie in the end planes.
    static std::shared_ptr<BeamVariable> rib(const std::array<Polyline, 2>& loops, const std::string& name);

    /// A four-corner member as a variable beam between the end sections over corners start and end, start[i] and end[i] on one long edge.
    static std::shared_ptr<BeamVariable> beam(
        const std::array<Polyline, 2>& loops,
        const std::array<size_t, 2>& start,
        const std::array<size_t, 2>& end,
        const std::string& name
    );

    /// The contact the session's search finds between the members named a_name and b_name, stored as their interaction named name, `<kind>_<place>`; a pair that does not touch throws naming it.
    Contact add_contact(const std::string& name, const std::string& a_name, const std::string& b_name);


    /// The screw connector of lines through the members, the first two the joint's.
    std::shared_ptr<JointBeam> screws_of(const std::vector<const Element*>& members, const std::vector<Line>& lines) const;

    /// Whether the inner rib screws of quarter q at end k pass the seam beam's end: a head beyond the end plane, away from the oculus beam.
    bool passes_seam_beam(size_t q, size_t k, const std::vector<Line>& screws) const;

    /// The level of a screw in a corner's level set: down from the datum in sevenths of the depth.
    double corner_level(double levels) const;

    /// A screw of a member butting on another, at the floor: along the member's axis at level z, moved offset across it, its head where that line meets the face from, SCREW_LENGTH on towards the member's body.
    Line screw(
        const std::array<Plane, 2>& member,
        const Plane& from,
        const Point& toward,
        double z,
        double offset = 0.0
    ) const;

    /// The axis of a member between two faces at level z: the line midway between their traces.
    static Line axis(const std::array<Plane, 2>& faces, double z);
};

}
