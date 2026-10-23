#pragma once
#include "src/templates/module/framed_panel.h"
#include "src/templates/module/module_contact.h"
#include "src/templates/module/room_module_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// RoomModule
// ═══════════════════════════════════════════════════════════════════════════

/// Timber room modules, a session built from a guide: per storey and column one frame module, a floor and a ceiling lying flat, each
/// a rim of boundary beams around the joists with noggings at mid-width, four corner posts between them, and a framed wall between the
/// posts on every closed side; the long sides between the modules of one room are open, a post at mid-length on each, so the room spans
/// them; between two rooms one wall, in the module of the smaller room. Then the contacts in every module and between modules side by
/// side and stacked. No connector is made.
///
/// Fields: `guide`, the RoomModuleGuide it is built from.
/// Every module is a group `module_<storey>_<column>` of the FramedPanels `<module>_floor`, `<module>_ceiling`, `<module>_wall_start`,
/// `<module>_wall_end` and, on a long side it closes, `<module>_wall_front` or `<module>_wall_back`, and of the posts `<module>_post_<k>`;
/// every member of a panel is named `<panel>_<part>`, read back by name.
class RoomModule : public WoodSession {
public:
    const RoomModuleGuide guide; // The geometry the modules are built from; every element is in the session.

    /// The modules of the guide as the session named name.
    explicit RoomModule(const RoomModuleGuide& guide = RoomModuleGuide(), const std::string& name = "room_module");

    /// Not copied, as a session is not.
    RoomModule(const RoomModule&) = delete;

    /// Not assigned, as it is not copied.
    RoomModule& operator=(const RoomModule&) = delete;

    /// Floor or ceiling element thickness: the joists and a deck on each face.
    double slab_thickness() const;

    /// Wall element thickness: the studs and a sheet on each face.
    double wall_thickness() const;

    /// One module's height: the floor, the clear height and the ceiling.
    double module_height() const;

    /// Modules side by side: those of every room.
    size_t columns() const;

private:
    /// Adds the module of a storey and a column: its floor and ceiling, its corner posts, the posts along its sides open inside its room,
    /// and its walls on the sides it closes.
    void add_module(size_t storey, size_t column);

    /// The room of the module in a column and the number of modules it has.
    std::array<size_t, 2> room_of(size_t column) const;
};

}
