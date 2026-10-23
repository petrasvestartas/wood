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
    std::array<std::vector<Opening>, 4> openings; // Per wall in its own frame: the front and back long sides, the start end at x 0, the end.
    double porch = 0.0; // The start wall set back by this from the module's start, the back wall stopping at it, a covered porch outside.
};

// ═══════════════════════════════════════════════════════════════════════════
// RoomModuleGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of timber room modules in mm, after the modules of Blumer Lehmann: about 3 x 9 m in plan, the width a Swiss standing
/// permit carries on the road (3 m), 2.7 m clear inside. A module is a frame: a floor and a ceiling element lying flat, each a rim of
/// wide boundary beams around thinner joists across it with a row of noggings at mid-width, carried at the four corners by posts, and
/// a framed wall only on each closed side. Modules stand side by side along their long sides; those of one room leave the sides
/// between them open, with a pair of posts at mid-length, so a room spans several modules. Between two rooms stands one wall, in the
/// module of the smaller room, the neighbour open against it. Modules stack up to three storeys. x along the module, y across it, z
/// up, the first module's corner at the origin.
struct RoomModuleGuide {
    double length = 9000.0; // Module length along x, the long sides.
    double width = 3000.0; // Module width along y, the ends.
    double clear_height = 2700.0; // Clear height inside, floor deck to ceiling, the walls' and the posts' height.
    double stud_width = 60.0; // Every wall frame member's size in the plane of its wall.
    double stud_depth = 160.0; // Wall frame depth, the insulation space.
    double stud_spacing = 625.0; // Stud centres from each wall's start, half a 1250 sheet.
    double sheet_thickness = 15.0; // Wall sheathing, one sheet on each face; a post is as wide as the wall.
    double rim_width = 160.0; // Floor and ceiling boundary beams, the long rims and the end rims, which carry the module between its posts.
    double joist_width = 100.0; // Floor and ceiling joists across the module between the long rims.
    double joist_depth = 200.0; // Floor and ceiling frame depth, rims and joists flush.
    double joist_spacing = 1250.0; // Largest joist spacing, the joists dividing the module's length evenly between the end rims.
    double deck_thickness = 25.0; // Floor and ceiling deck, one sheet on each face.
    size_t side_posts = 1; // Posts along a long side open inside a room, evenly between the corner posts, one at mid-length by default.
    std::vector<ModuleLayout> layouts = {
        {{{{}, {}, {{560.0, 1500.0, 900.0, 2100.0}}, {{810.0, 1000.0, 0.0, 2100.0}}}}, 0.0},
    }; // Per module across, cycled: a window in the facade end at x 0, a door in the corridor end.
    std::vector<size_t> rooms = {2}; // Modules side by side along y that form each room, the sides between them open.
    size_t storeys = 1; // Modules stacked along z, at most three.
};

}
