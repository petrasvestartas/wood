#pragma once
#include "src/templates/module/wall_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// RoomModuleGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of a timber room module in mm, after the modules of Blumer Lehmann: about 3 x 9 m in plan, the width a Swiss
/// standing permit carries on the road (3 m), 2.7 m clear inside; a floor and a ceiling element, framed lying flat, and four framed
/// walls standing on the floor, the two long walls running past the two end walls. Modules stand side by side along their long walls
/// and stack up to three storeys. x along the module, y across it, z up, the first module's corner at the origin.
struct RoomModuleGuide {
    double length = 9000.0; // Module length along x, the long walls.
    double width = 3000.0; // Module width along y, the end walls.
    double clear_height = 2700.0; // Clear height inside, floor deck to ceiling, the walls' height.
    double stud_width = 60.0; // Every member's size in the plane of its element.
    double stud_depth = 160.0; // Wall frame depth, the insulation space.
    double joist_depth = 200.0; // Floor and ceiling frame depth.
    double stud_spacing = 625.0; // Stud and joist centres from each element's start, half a 1250 sheet.
    double sheet_thickness = 15.0; // Wall sheathing, one sheet on each face.
    double deck_thickness = 25.0; // Floor and ceiling deck, one sheet on each face.
    std::array<std::vector<Opening>, 4> openings = {{
        {{1280.0, 1190.0, 900.0, 2100.0}, {5600.0, 1250.0, 900.0, 2100.0}},
        {{3500.0, 2000.0, 900.0, 2100.0}},
        {{810.0, 1000.0, 0.0, 2100.0}},
        {},
    }}; // Per wall in its own frame: the front and back long walls, the end walls at x 0 and at x length; a door in the first end wall.
    size_t columns = 1; // Modules side by side along y.
    size_t storeys = 1; // Modules stacked along z, at most three.
};

}
