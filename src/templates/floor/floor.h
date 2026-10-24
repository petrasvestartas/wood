#pragma once
#include "src/templates/floor/floor_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════

/// A contact the floor's design puts between two members. - `a`.
struct Contact {
    std::shared_ptr<Element> a; // The source member.
    std::shared_ptr<Element> b; // The target member, which hosts the contact.
    std::shared_ptr<InteractionContactFace>
        face; // The face they share, stored as their interaction.
};

/// The contacts of one quarter, by the connector each gets. - `seam_wedge`.
struct QuarterContacts {
    Contact seam_wedge; // Seam beam 0 beside the next quarter's seam beam 2: a wedge.
    Contact oculus_wedge; // The oculus beam's back face on its ring beam: a wedge.
    std::array<Contact, 2> column_plates; // The column against outer rib k: a rectangle plate.
    std::array<std::array<Contact, 2>, 3>
        block_pins; // Column block b against the rib either side: pins.
    std::array<Contact, 2>
        outer_rib_seam_beam; // Seam beam k against the outer rib ending on it: pins.
    std::array<Contact, 2> seam_beam_oculus_beam; // Seam beam k against the oculus beam: pins.
    std::array<Contact, 2> oculus_beam_inner_rib; // The oculus beam against inner rib k: pins.
    Contact ring_corner; // Ring beam q against ring beam q + 1 at their corner: pins.
};

/// The connectors of one quarter.
struct QuarterConnectors {
    std::shared_ptr<JointBeam> seam_wedge; // connector_seam_wedge_<q>.
    std::shared_ptr<JointBeam> oculus_wedge; // connector_oculus_wedge_<q>.
    std::array<std::shared_ptr<Plate>, 2> column_plates; // column_plate_<q>_<k>, one per outer rib.
    std::array<std::shared_ptr<JointBeam>, 2>
        column_plate_pins; // connector_column_plate_<q>_<k>, its pocket in the column and the rib, two pins in each.
    std::shared_ptr<JointPlate>
        cross_lap; // connector_cross_lap_<q>, the cross half lap where the two column plates cross.
    std::array<std::array<std::shared_ptr<JointBeam>, 2>, 3>
        block_pins; // connector_block_pins_<q>_<b>_<side>, per block and side.
    std::array<std::shared_ptr<JointBeam>, 2>
        outer_rib_seam_beam; // connector_pins_outer_rib_<q>_<k>, seam beam k into the outer rib ending on it.
    std::array<std::shared_ptr<JointBeam>, 2>
        seam_beam_oculus_beam; // connector_pins_seam_beam_<q>_<k>, seam beam k into the oculus beam.
    std::array<std::shared_ptr<JointBeam>, 2>
        oculus_beam_inner_rib; // connector_pins_inner_rib_<q>_<k>, the oculus beam into inner rib k.
    std::shared_ptr<JointBeam>
        ring_corner; // connector_pins_ring_corner_<q>, ring beam q and ring beam q + 1 at their corner.
};

// ═══════════════════════════════════════════════════════════════════════════
// Floor
// ═══════════════════════════════════════════════════════════════════════════

/// The floor model, a session built step by step from a guide, grouped by quarter and oculus.
class Floor : public WoodSession {
public:
    static constexpr double PIN_LENGTH = 200.0; // mm, every assembly pin.
    static constexpr double PIN_RADIUS = 2.0; // mm, every assembly pin.
    static constexpr double PIN_INSET = 20.0; // mm the pins stand in from the contact's edges.
    static constexpr double PIN_SHIFT =
        15.0; // mm the seam pins stand off the contact's middle, so the two quarters' heads at a seam stay 30 apart.
    static constexpr std::array<size_t, 2> SEAM_BEAMS = {
        0, 2
    }; // The inner beams on seam 0 and seam 1.

    const FloorGuide
        guide; // The geometry the model is built from; every element is in the session.

    /// The model of the guide as the session named name.
    explicit Floor(const FloorGuide& guide, const std::string& name = "floor");

    /// Not copied, as a session is not.
    Floor(const Floor&) = delete;

    /// Not assigned, as it is not copied.
    Floor& operator=(const Floor&) = delete;

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

    /// The connector of every contact, each built on uncut members before any is added.
    std::array<QuarterConnectors, 4>
    compute_connectors(const std::array<QuarterContacts, 4>& contacts) const;

    /// Adds the connectors kind by kind, quarter by quarter, the cross laps last.
    void add_connectors(
        const std::array<QuarterConnectors, 4>& connectors,
        const std::array<QuarterContacts, 4>& contacts
    );

    /// The quarter's group, made the first time.
    std::shared_ptr<TreeNode> quarter_group(size_t q);

    /// The connectors group of quarter q, made the first time.
    std::shared_ptr<TreeNode> connectors_group(size_t q);

    /// A rib as a variable beam, one section per soffit point.
    static std::shared_ptr<BeamVariable>
    rib(const std::array<Polyline, 2>& loops, const std::string& name);

    /// A four-corner member as a variable beam between the end sections over corners start and end.
    static std::shared_ptr<BeamVariable> beam(
        const std::array<Polyline, 2>& loops,
        const std::array<size_t, 2>& start,
        const std::array<size_t, 2>& end,
        const std::string& name
    );

    /// The contact between members a_name and b_name, stored as their interaction name; throws when they do not touch.
    Contact
    add_contact(const std::string& name, const std::string& a_name, const std::string& b_name);
};

}
