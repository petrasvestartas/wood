#pragma once
#include "src/templates/module/wall_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// RoomModuleGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of a timber room module in mm, after the modules of Blumer Lehmann: about 3 x 9 m in plan, the width a Swiss
/// standing permit carries on the road (3 m), 2.7 m clear inside. A module is a frame: a floor and a ceiling element, framed lying
/// flat, carried at the four corners by posts, and a framed wall only on each closed side. Modules stand side by side along their long
/// sides; those of one room leave the sides between them open, so a room spans several modules with only the posts at the joints,
/// and rooms keep a wall each between them. The end at x 0 is the facade, the end at x length opens to a corridor. Modules stack up
/// to three storeys. x along the module, y across it, z up, the first module's corner at the origin.
struct RoomModuleGuide {
    double length = 9000.0; // Module length along x, the long sides.
    double width = 3000.0; // Module width along y, the ends.
    double clear_height = 2700.0; // Clear height inside, floor deck to ceiling, the walls' and the posts' height.
    double stud_width = 60.0; // Every frame member's size in the plane of its element.
    double stud_depth = 160.0; // Wall frame depth, the insulation space.
    double joist_depth = 200.0; // Floor and ceiling frame depth.
    double stud_spacing = 625.0; // Stud and joist centres from each element's start, half a 1250 sheet.
    double sheet_thickness = 15.0; // Wall sheathing, one sheet on each face; a post is as wide as the wall.
    double deck_thickness = 25.0; // Floor and ceiling deck, one sheet on each face.
    std::array<std::vector<Opening>, 4> openings = {{
        {},
        {},
        {{560.0, 1500.0, 900.0, 2100.0}},
        {{810.0, 1000.0, 0.0, 2100.0}},
    }}; // Per wall in its own frame: the front and back long sides, the facade end at x 0 with a window, the corridor end with a door.
    size_t columns = 2; // Modules side by side along y.
    size_t modules_per_room = 2; // Modules side by side that form one room, the sides between them open.
    size_t storeys = 1; // Modules stacked along z, at most three.
};

}
