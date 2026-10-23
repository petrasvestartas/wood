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
/// permit carries on the road (3 m), 2.7 m clear inside. A module is a frame: a floor box and a lighter ceiling box, each edge beams
/// all round and joists across of one depth between a board under and a board over it, carried at the four corners by posts, a framed
/// wall on each closed side and a ceiling downstand beam from post to post on each open one. Modules stand side by side along their
/// long sides; those of one room leave the sides between them open, with a pair of posts at mid-length, so a room spans several modules. Between two rooms stands one wall, in the
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
    double floor_depth = 240.0; // Floor box depth, its edge beams and joists flush; it carries the room's loads.
    double floor_edge_width = 200.0; // Floor edge beams all round, a wall standing on them; 200 x 240 spans the 4.4 m between the posts of an open side.
    double floor_joist_width = 80.0; // Floor joists across between the long edge beams, 80 x 240 over the 2.6 m between them.
    double floor_deck_thickness = 27.0; // Floor deck over the box, a three-layer board.
    double ceiling_depth = 160.0; // Ceiling box depth, lighter than the floor: it carries only itself.
    double ceiling_edge_width = 120.0; // Ceiling edge beams all round, on the walls or on the downstand beams.
    double ceiling_joist_width = 60.0; // Ceiling joists across between the long edge beams, 60 x 160 over the 2.76 m between them.
    double board_thickness = 15.0; // The board under the floor box and on both faces of the ceiling box.
    double joist_spacing = 625.0; // Largest joist spacing, the joists dividing the length between the end edge beams evenly.
    double downstand_depth = 300.0; // Ceiling downstand beam under a long side without a wall, a wall wide, carrying the box above from post to post.
    size_t side_posts = 1; // Posts along a long side without a wall, evenly between the corner posts, one at mid-length by default.
    std::vector<ModuleLayout> layouts = {
        {{{{}, {}, {{560.0, 1500.0, 900.0, 2100.0}}, {{810.0, 1000.0, 0.0, 2100.0}}}}, 0.0},
    }; // Per module across, cycled: a window in the facade end at x 0, a door in the corridor end.
    std::vector<size_t> rooms = {2}; // Modules side by side along y that form each room, the sides between them open.
    size_t storeys = 1; // Modules stacked along z, at most three.
};

}
