#pragma once
#include "src/templates/module/framed_panel.h"
#include "src/templates/module/module_contact.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Wall
// ═══════════════════════════════════════════════════════════════════════════

/// A framed timber wall module, a session built from a guide: one FramedPanel standing in the xz plane, its rails, studs, the frame
/// of every opening and a sheet on each face, and the contacts between them; no connector is made.
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
};

}
