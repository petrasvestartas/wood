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

/// Timber room modules.
class RoomModule : public WoodSession {
public:
    const RoomModuleGuide
        guide; // The geometry the modules are built from; every element is in the session.

    /// The modules of the guide as the session named name.
    explicit RoomModule(
        const RoomModuleGuide& guide = RoomModuleGuide(), const std::string& name = "room_module"
    );

    /// Not copied, as a session is not.
    RoomModule(const RoomModule&) = delete;

    /// Not assigned, as it is not copied.
    RoomModule& operator=(const RoomModule&) = delete;

    /// Floor thickness: its joists between the board under them and the deck.
    double floor_thickness() const;

    /// Ceiling thickness: its joists between its lining and its top board.
    double ceiling_thickness() const;

    /// Wall element thickness: the studs and a sheet on each face.
    double wall_thickness() const;

    /// One module's height: the floor, the clear height and the ceiling.
    double module_height() const;

    /// Modules side by side: those of every room.
    size_t columns() const;

private:
    /// Adds the module of a storey and a column.
    void add_module(size_t storey, size_t column);

    /// Adds a floor or a ceiling named name from the module's corner at its underside.
    void add_box(
        const std::string& name,
        const Point& corner,
        bool floor,
        const std::shared_ptr<TreeNode>& parent
    );

    /// The room of the module in a column and the number of modules it has.
    std::array<size_t, 2> room_of(size_t column) const;
};

}
