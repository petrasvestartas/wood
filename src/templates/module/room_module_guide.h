#pragma once
#include "src/templates/module/wall_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// ModuleLayout
// ═══════════════════════════════════════════════════════════════════════════

/// What differs from one module across to the next: the openings of its walls and a porch at its start.
struct ModuleLayout {
    std::array<std::vector<Opening>, 4>
        openings; // Per wall in its own frame: the front and back long sides, the start end at x 0, the end.
    double porch = 0.0; // The start wall set back by this from the module's start.
};

// ═══════════════════════════════════════════════════════════════════════════
// RoomModuleGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of timber room modules in mm.
struct RoomModuleGuide {
    double length = 9000.0; // Module length along x, the long sides.
    double width = 3000.0; // Module width along y, the ends.
    double clear_height =
        2700.0; // Clear height inside, floor deck to ceiling, the walls' and the posts' height.
    double stud_width = 60.0; // Every wall frame member's size in the plane of its wall.
    double stud_depth = 160.0; // Wall frame depth, the insulation space.
    double stud_spacing = 625.0; // Stud centres from each wall's start, half a 1250 sheet.
    double sheet_thickness =
        15.0; // Wall sheathing, one sheet on each face; a post is as wide as the wall.
    double floor_joist_width =
        80.0; // Floor joists across between the long edge beams, 80 x 240 over the 2.62 m between them.
    double floor_joist_depth = 240.0; // Floor joist depth.
    double deck_thickness =
        25.0; // Floor deck on the joists between the edge beams, flush with their tops, a three-layer board.
    double ceiling_joist_width =
        60.0; // Ceiling joists across between the long edge beams, 60 x 140; the ceiling carries only itself.
    double ceiling_joist_depth =
        140.0; // Ceiling joist depth, a board on each face flush with the edge beams: the ceiling 170 thick.
    double board_thickness =
        15.0; // The board under every box's joists, the ceiling's lining, and the ceiling's top board.
    double joist_spacing =
        625.0; // Largest joist spacing, the joists dividing the length between the end edge beams evenly.
    double downstand_depth = 300.0; // How far the ceiling's long edge beams reach below it.
    size_t side_posts =
        1; // Posts along a long side without a wall, evenly between the corner posts, one at mid-length by default.
    std::vector<ModuleLayout> layouts = {
        {{{{}, {}, {{260.0, 2100.0, 600.0, 2500.0}}, {{810.0, 1000.0, 0.0, 2100.0}}}}, 0.0},
    }; // Per module across, cycled: a window in the facade end at x 0, a door in the corridor end.
    std::vector<size_t> rooms = {2
    }; // Modules side by side along y that form each room, the sides between them open.
    size_t storeys = 1; // Modules stacked along z, at most three.
};

}
