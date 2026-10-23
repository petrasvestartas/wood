#include "src/templates/module/room_module.h"

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════

RoomModule::RoomModule(const RoomModuleGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // modules: per storey and column one module, its floor, four walls and ceiling, each a framed panel
    for (size_t storey = 0; storey < guide.storeys; storey++)
        for (size_t column = 0; column < guide.columns; column++)
            add_module(storey, column);

    // contacts: every two members and sheets that touch, in a module, side by side and stacked
    add_module_contacts(*this);
}

// ═══════════════════════════════════════════════════════════════════════════
// Sizes
// ═══════════════════════════════════════════════════════════════════════════

double RoomModule::slab_thickness() const {
    return guide.joist_depth + 2.0 * guide.deck_thickness;
}

double RoomModule::wall_thickness() const {
    return guide.stud_depth + 2.0 * guide.sheet_thickness;
}

double RoomModule::module_height() const {
    return 2.0 * slab_thickness() + guide.clear_height;
}

// ═══════════════════════════════════════════════════════════════════════════
// Module
// ═══════════════════════════════════════════════════════════════════════════

void RoomModule::add_module(size_t storey, size_t column) {

    const std::string module = fmt::format("module_{}_{}", storey, column);
    const std::shared_ptr<TreeNode> group = group_named(module);
    const Vector corner(0.0, guide.width * static_cast<double>(column), module_height() * static_cast<double>(storey));
    const Point origin = Point(0.0, 0.0, 0.0) + corner;
    const double slab = slab_thickness();
    const double wall = wall_thickness();
    const double floor_top = slab;
    const Vector x(1.0, 0.0, 0.0);
    const Vector y(0.0, 1.0, 0.0);
    const Vector z(0.0, 0.0, 1.0);

    // floor and ceiling: framed lying flat over the whole module, rim joists along it, joists across it on the grid
    WallGuide slab_guide;
    slab_guide.length = guide.length;
    slab_guide.height = guide.width;
    slab_guide.stud_width = guide.stud_width;
    slab_guide.stud_depth = guide.joist_depth;
    slab_guide.stud_spacing = guide.stud_spacing;
    slab_guide.sheet_thickness = guide.deck_thickness;
    slab_guide.openings = {};
    const Plane floor_plane(origin + z * (0.5 * slab), x, y);
    const Plane ceiling_plane(origin + z * (floor_top + guide.clear_height + 0.5 * slab), x, y);
    const FramedPanel floor(*this, slab_guide, floor_plane, module + "_floor", group);
    const FramedPanel ceiling(*this, slab_guide, ceiling_plane, module + "_ceiling", group);

    // long walls: on the floor deck under the ceiling along the whole module, running past the end walls
    WallGuide long_guide;
    long_guide.length = guide.length;
    long_guide.height = guide.clear_height;
    long_guide.stud_width = guide.stud_width;
    long_guide.stud_depth = guide.stud_depth;
    long_guide.stud_spacing = guide.stud_spacing;
    long_guide.sheet_thickness = guide.sheet_thickness;
    long_guide.openings = guide.openings[0];
    const Plane front_plane(origin + y * (0.5 * wall) + z * floor_top, x, z);
    const FramedPanel front(*this, long_guide, front_plane, module + "_wall_front", group);
    long_guide.openings = guide.openings[1];
    const Plane back_plane(origin + x * guide.length + y * (guide.width - 0.5 * wall) + z * floor_top, x * -1.0, z);
    const FramedPanel back(*this, long_guide, back_plane, module + "_wall_back", group);

    // end walls: between the long walls, at the module's start and at its end
    WallGuide end_guide = long_guide;
    end_guide.length = guide.width - 2.0 * wall;
    end_guide.openings = guide.openings[2];
    const Plane start_plane(origin + x * (0.5 * wall) + y * (guide.width - wall) + z * floor_top, y * -1.0, z);
    const FramedPanel start(*this, end_guide, start_plane, module + "_wall_start", group);
    end_guide.openings = guide.openings[3];
    const Plane end_plane(origin + x * (guide.length - 0.5 * wall) + y * wall + z * floor_top, y, z);
    const FramedPanel end(*this, end_guide, end_plane, module + "_wall_end", group);
}

}
