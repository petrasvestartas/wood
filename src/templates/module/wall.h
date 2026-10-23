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
/// Members are named `bottom_rail_<i>`, `top_rail`, `stud_<i>`, `jamb_stud_<o>_<side>`, `header_<o>`, `sill_<o>` and
/// `sheathing_<side>`, cripples as `cripple_<o>_<i>_<run>`, read back by name; every member has the same section.
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

    /// Adds per opening a full-height jamb stud either side, the header between them and under a window its sill, in opening_<o>.
    void add_openings();

    /// Adds the edge studs and a stud at every multiple of the stud spacing from the wall's start where no edge or jamb stud stands; one
    /// inside an opening is a cripple over its header and under its sill, in opening_<o>.
    void add_studs();

    /// Adds one panel on each face of the whole wall, every door a notch and every window a hole, sheathing_0 and sheathing_1.
    void add_sheathing();

    /// Searches the contact between every two frame members and every panel and frame member, stores it as their interaction and
    /// draws its polygon as a red mesh in contacts_frame or contacts_sheathing.
    void add_contacts();

    /// A frame member from start to end, stud_depth across the wall and size along up.
    std::shared_ptr<Beam> member(
        const Point& start,
        const Point& end,
        double size,
        const Vector& up,
        const std::string& name
    ) const;
};

}
