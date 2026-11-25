#pragma once
#include "src/templates/module/spatial_module_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// ModuleBuildingGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of a building of spatial modules in mm: stacked on each other, or set into a skeleton.
struct ModuleBuildingGuide {
    std::vector<SpatialModuleGuide> modules = {SpatialModuleGuide()}; // Per module across, cycled; all of one size.
    std::vector<size_t> rooms = {1, 1, 1}; // Modules side by side along y that form each room, the long sides between them open.
    size_t storeys = 3; // Modules above each other.
    double corridor = 0.0; // Clear width between two rows of modules facing each other, 0 for one row.
    double corridor_panel = 160.0; // Thickness of the cross-laminated corridor floors and roof.
    double joint = 50.0; // Between two modules side by side, filled with mineral wool.

    double batten = 40.0; // Height of the timber battens under a module, on the ceiling below, a deck or the ground.
    double strip = 12.0; // Elastic bearing strip on them, the acoustic decoupling.
    double bearing = 80.0; // Width of a batten and a strip.
    double coupler = 0.0; // Thickness of the plates that couple the modules at their start ends, 0 for none.
    double coupler_width = 400.0; // Width of a plate across a joint, half of it on each module.

    double post = 0.0; // Size of the skeleton's posts and the width of its beams, 0 for modules that carry each other.
    double beam = 320.0; // Depth of the skeleton's beams.
    double deck = 0.0; // Thickness of the deck on the beams; 0 hangs the modules between the beams on ledgers.
    double ledger = 80.0; // Width and height of a ledger on a beam's face.
    double clearance = 20.0; // Between a module and the skeleton.
    size_t bay = 1; // Modules side by side between two rows of posts.
    size_t level = 1; // Modules above each other on one level of the skeleton; 1 on ledgers.

    bool pod = false; // A smaller module set into every module.
    SpatialModuleGuide pod_guide = wet_room(); // The module set in.
    Point pod_at = Point(2200.0, 200.0, 0.0); // Its corner from the module's corner, on the module's floor.

    /// A wet room: one unit of thin solid plates with a door, small enough to be set into a module.
    static SpatialModuleGuide wet_room() {

        SpatialModuleGuide room;
        room.length = 2200.0;
        room.width = 1650.0;
        room.clear_height = 2200.0;
        room.solid_walls = true;
        room.solid_floor = true;
        room.solid_ceiling = true;
        room.wall_panel = 60.0;
        room.floor_panel = 60.0;
        room.ceiling_panel = 60.0;
        room.openings[3] = {{300.0, 800.0, 0.0, 2000.0}};

        return room;
    }
};

}
