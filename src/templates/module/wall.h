#pragma once
#include "src/templates/module/module_contact.h"
#include "src/templates/module/wall_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// WallContacts
// ═══════════════════════════════════════════════════════════════════════════

/// The contacts of a wall, by the connector each gets.
///
/// - `frame`: a member's end on the member it stands on, `a` the member the pins pass through first, named `<b>_on_<a>`.
/// - `sheathing`: a sheathing panel on a frame member it covers, `a` the panel, named `<a>_on_<b>`.
struct WallContacts {
    std::vector<Contact> frame; // Every end of a frame member on another.
    std::vector<Contact> sheathing; // Every panel on every member under it.
};

/// The connectors of a wall, field for field the contacts.
struct WallConnectors {
    std::vector<std::shared_ptr<JointBeam>> frame; // connector_frame_<k>, headed pins through a into the end of b.
    std::vector<std::shared_ptr<JointBeam>> sheathing; // connector_sheathing_<k>, a row of headed pins through the panel into the member.
};

// ═══════════════════════════════════════════════════════════════════════════
// Wall
// ═══════════════════════════════════════════════════════════════════════════

/// A framed timber wall module, a session built from a guide: rails, studs, the frame of every opening, sheathing on both faces, the
/// contacts between them and the pins of every contact.
///
/// Fields: `guide`, the WallGuide it is built from, and the pin constants.
/// Members are named `bottom_rail_<i>`, `top_rail`, `stud_<i>`, `king_stud_<o>_<side>`, `header_<o>`, `sill_<o>` and
/// `sheathing_<side>_<p>`, cripples as `cripple_top_<o>_<i>` and `cripple_bottom_<o>_<i>`, read back by name.
class Wall : public WoodSession {
public:
    static constexpr double FRAME_PIN_LENGTH = 100.0; // mm, a pin through a rail into a stud end.
    static constexpr double SHEATHING_PIN_LENGTH = 50.0; // mm, a pin through a panel into a member.
    static constexpr double PIN_RADIUS = 2.0; // mm, every pin.
    static constexpr double PIN_INSET = 20.0; // mm the frame pins stand in from the contact's edges.
    static constexpr double SHEATHING_PIN_INSET = 10.0; // mm the sheathing pins stand in, so a panel on half a stud at a sheet break keeps its row.
    static constexpr double PIN_SPACING = 150.0; // mm between the sheathing pins along a member.

    const WallGuide guide; // The geometry the wall is built from; every element is in the session.

    /// The wall of the guide as the session named name.
    explicit Wall(const WallGuide& guide = WallGuide(), const std::string& name = "wall");

    /// Not copied, as a session is not.
    Wall(const Wall&) = delete;

    /// Not assigned, as it is not copied.
    Wall& operator=(const Wall&) = delete;

private:
    /// Adds the bottom rail, split at every door, and the top rail, in rails.
    void add_rails();

    /// Adds the studs at the stud spacing that no opening takes, in studs.
    void add_studs();

    /// Adds per opening its two king studs, header, sill and the cripples above and below it, in openings.
    void add_openings();

    /// Adds the panels on both faces, split at the sheet width and at every opening edge, in sheathing_0 and sheathing_1.
    void add_sheathing();

    /// Searches the contact between every two frame members and every panel and frame member, stores it as their interaction and returns them.
    WallContacts add_contacts();

    /// The pins of every contact, each built on the uncut members before any is added.
    WallConnectors compute_connectors(const WallContacts& contacts) const;

    /// Adds the frame pins, then the sheathing pins, each to both members it joins.
    void add_connectors(const WallConnectors& connectors, const WallContacts& contacts);

    /// A frame member from start to end, stud_depth across the wall and size along up.
    std::shared_ptr<Beam> member(
        const Point& start,
        const Point& end,
        double size,
        const Vector& up,
        const std::string& name
    ) const;

    /// The stud centres along the wall at the stud spacing, the two end studs flush with the wall's ends.
    std::vector<double> compute_stud_positions() const;

    /// The x intervals the panels of one face split into: the sheet width and every opening edge, a sheet break near an opening edge dropped.
    std::vector<std::array<double, 2>> compute_strips() const;
};

}
