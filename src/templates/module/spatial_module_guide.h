#pragma once
#include "src/templates/module/wall_guide.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// SpatialModuleGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of one spatial module in mm: a floor, walls on it and a ceiling on them, each framed or one solid plate.
struct SpatialModuleGuide {
    double length = 6000.0; // Outside length along x, the long sides; 6000, 9000 or 12000.
    double width = 2950.0; // Outside width along y: a 3000 grid less the 50 joint, inside the 3000 transport width.
    double clear_height = 2500.0; // Between the floor's top and the ceiling's underside, the walls' height.

    bool solid_walls = false; // Walls of one cross-laminated plate each, else framed and sheathed on both faces.
    bool solid_floor = false; // Floor of one cross-laminated plate, else a box of ribs between two boards.
    bool solid_ceiling = false; // Ceiling of one cross-laminated plate, else a box of ribs between two boards.
    double wall_panel = 100.0; // Solid wall thickness.
    double floor_panel = 140.0; // Solid floor thickness.
    double ceiling_panel = 80.0; // Solid ceiling thickness; the ceiling carries only itself.

    double stud_width = 60.0; // Every wall frame member's size in the plane of its wall.
    double stud_depth = 120.0; // Wall frame depth, the insulation space.
    double wall_sheet = 15.0; // Wall sheathing, one sheet on each face.
    double rib_width = 80.0; // Rib width of a floor and a ceiling box, their edge ribs all round too.
    double floor_rib = 160.0; // Floor rib depth, the ribs across the module, its short span.
    double floor_sheet = 27.0; // Three-layer board on each face of the floor's ribs.
    double ceiling_rib = 120.0; // Ceiling rib depth.
    double ceiling_sheet = 15.0; // Board on each face of the ceiling's ribs.
    double spacing = 625.0; // Stud and rib centres, half a 1250 board.

    std::array<bool, 4> closed = {true, true, true, true}; // Walls: the front side at y 0, the back side, the start end at x 0, the end.
    std::array<std::vector<Opening>, 4> openings; // Per wall, from its left corner as seen from outside.
    std::vector<Opening> shafts; // Holes through the floor and the ceiling, from the corner of both along x and y.
    double downstand = 240.0; // Depth of the beam under the ceiling along an open long side.
    size_t side_posts = 0; // Posts along an open long side, evenly between its corner posts.
    double brace = 0.0; // Board thickness of the crossed transport braces in the open long sides, 0 for none.
    double pin_spacing = 0.0; // Spacing of the pins along the edges of solid plates, 0 for none.

    /// Wall thickness, a post's size too.
    double wall_thickness() const {
        return solid_walls ? wall_panel : stud_depth + 2.0 * wall_sheet;
    }

    /// Floor thickness.
    double floor_thickness() const {
        return solid_floor ? floor_panel : floor_rib + 2.0 * floor_sheet;
    }

    /// Ceiling thickness.
    double ceiling_thickness() const {
        return solid_ceiling ? ceiling_panel : ceiling_rib + 2.0 * ceiling_sheet;
    }

    /// Outside height.
    double height() const {
        return floor_thickness() + clear_height + ceiling_thickness();
    }
};

}
