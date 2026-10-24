#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Opening
// ═══════════════════════════════════════════════════════════════════════════

/// A door or window in a wall.
struct Opening {
    double x = 0.0; // Clear opening start along the wall, mm from its left end.
    double width = 0.0; // Clear opening width along the wall.
    double sill = 0.0; // Clear opening bottom above the wall's foot, 0 for a door.
    double head = 0.0; // Clear opening top above the wall's foot.
};

// ═══════════════════════════════════════════════════════════════════════════
// WallGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of a framed wall module in mm.
struct WallGuide {
    double length = 4800.0; // Wall length along x.
    double height = 2700.0; // Wall height from foot to the top of the top rail.
    double stud_width =
        60.0; // Every member's size in the plane of the wall: studs, rails, jamb studs, headers and sills.
    double stud_depth = 160.0; // Frame depth across the wall, the insulation space.
    double stud_spacing =
        625.0; // Stud centres from the wall's start, half a 1250 sheet; every opening is framed by its own jamb studs.
    double sheet_thickness = 15.0; // Sheathing thickness, one panel on each face of the frame.
    std::vector<Opening> openings = {
        {600.0, 1000.0, 0.0, 2100.0},
        {2525.0, 1250.0, 900.0, 2100.0},
    }; // A door and a window by default.
};

}
