#include "src/templates/module/wall.h"

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
Wall::Wall(const WallGuide& guide, const std::string& name) : WoodSession(name), guide(guide) {

    // frame and sheathing: the wall as a framed panel standing in the xz plane, its foot on the x axis
    const Plane upright(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0));
    const FramedPanel panel(*this, guide, upright);

    // contacts: every member end on the member it stands on, every sheet on every member under it, each drawn
    add_module_contacts(*this);
}

}
