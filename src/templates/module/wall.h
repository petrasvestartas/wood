#pragma once
#include "src/templates/module/wall_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Wall
// ═══════════════════════════════════════════════════════════════════════════

/// A framed timber wall module, a session built from a guide: rails, studs, the frame of every opening, sheathing on both faces and the
/// contacts between them; no connector is made.
///
/// Fields: `guide`, the WallGuide it is built from.
/// Members are named `bottom_rail_<i>`, `top_rail`, `stud_<i>`, `king_stud_<o>_<side>`, `header_<o>`, `sill_<o>` and
/// `sheathing_<side>_<p>`, cripples as `cripple_top_<o>_<i>` and `cripple_bottom_<o>_<i>`, read back by name.
class Wall : public WoodSession {
public:
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

    /// Searches the contact between every two frame members and every panel and frame member, stores it as their interaction and
    /// draws its polygon in contacts_frame or contacts_sheathing.
    void add_contacts();

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
