#include "src/templates/module/room_module.h"
#include "wood_profile.h"

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════

RoomModule::RoomModule(const RoomModuleGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // modules: per storey and column one module, its floor and ceiling, its posts and its framed walls
    for (size_t storey = 0; storey < guide.storeys; storey++)
        for (size_t column = 0; column < columns(); column++)
            add_module(storey, column);

    // contacts: every two members and sheets that touch, in a module, side by side and stacked
    add_module_contacts(*this);
}

// ═══════════════════════════════════════════════════════════════════════════
// Sizes
// ═══════════════════════════════════════════════════════════════════════════

double RoomModule::slab_thickness() const {
    return guide.primary_depth + guide.deck_thickness;
}

double RoomModule::wall_thickness() const {
    return guide.stud_depth + 2.0 * guide.sheet_thickness;
}

double RoomModule::module_height() const {
    return 2.0 * slab_thickness() + guide.clear_height;
}

size_t RoomModule::columns() const {
    size_t count = 0;

    for (const size_t modules : guide.rooms)
        count += modules;

    return count;
}

std::array<size_t, 2> RoomModule::room_of(size_t column) const {
    size_t start = 0;

    for (size_t room = 0; room < guide.rooms.size(); room++) {
        if (column < start + guide.rooms[room])
            return {room, guide.rooms[room]};

        start += guide.rooms[room];
    }

    return {guide.rooms.size(), 0};
}

// ═══════════════════════════════════════════════════════════════════════════
// Module
// ═══════════════════════════════════════════════════════════════════════════

void RoomModule::add_slab(const std::string& name, const Point& corner, bool floor, const std::shared_ptr<TreeNode>& parent) {

    const std::shared_ptr<TreeNode> slab = group_named(name, parent);
    const std::shared_ptr<TreeNode> beams = group_named(name + "_beams", slab);
    const std::shared_ptr<TreeNode> decks = group_named(name + "_decks", slab);
    const Vector x(1.0, 0.0, 0.0);
    const Vector y(0.0, 1.0, 0.0);
    const Vector z(0.0, 0.0, 1.0);
    const double primary = guide.primary_width;
    const double deck = guide.deck_thickness;
    const double beams_base = floor ? 0.0 : deck;
    const double joists_base = floor ? guide.primary_depth - guide.joist_depth : deck;
    const double inner = guide.width - 2.0 * primary;

    // primary beams: along both long sides over the whole length
    for (size_t side = 0; side < 2; side++) {
        const double across = side == 0 ? 0.5 * primary : guide.width - 0.5 * primary;
        const Point start = corner + y * across + z * (beams_base + 0.5 * guide.primary_depth);
        const Polyline axis({start, start + x * guide.length});
        add(std::make_shared<Beam>(axis, profile_rectangle(primary, guide.primary_depth), std::vector<Vector>{z}, fmt::format("{}_primary_{}", name, side)), beams);
    }

    // joists: across between the primary beams, the end joists at the module's ends, the length divided evenly
    const double bays = std::ceil((guide.length - guide.joist_width) / guide.joist_spacing);

    for (size_t i = 0; i <= static_cast<size_t>(bays); i++) {
        const double along = 0.5 * guide.joist_width + (guide.length - guide.joist_width) * static_cast<double>(i) / bays;
        const Point start = corner + x * along + y * primary + z * (joists_base + 0.5 * guide.joist_depth);
        const Polyline axis({start, start + y * inner});
        add(std::make_shared<Beam>(axis, profile_rectangle(guide.joist_width, guide.joist_depth), std::vector<Vector>{z}, fmt::format("{}_joist_{}", name, i)), beams);
    }

    // deck: a floor's over its top, a ceiling's lining under it, over the whole module
    const double deck_z = floor ? guide.primary_depth : 0.0;
    const Polyline deck_bottom = Polyline::rectangle(corner + z * deck_z, x, y, guide.length, guide.width);
    const Polyline deck_top = Polyline::rectangle(corner + z * (deck_z + deck), x, y, guide.length, guide.width);
    add(std::make_shared<Plate>(deck_bottom, deck_top, name + "_deck"), decks);

    // board: on the joists' other face between the primary beams
    const double board_z = floor ? joists_base - deck : deck + guide.joist_depth;
    const Polyline board_bottom = Polyline::rectangle(corner + y * primary + z * board_z, x, y, guide.length, inner);
    const Polyline board_top = Polyline::rectangle(corner + y * primary + z * (board_z + deck), x, y, guide.length, inner);
    add(std::make_shared<Plate>(board_bottom, board_top, name + "_board"), decks);
}

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

    // floor and ceiling: two primary beams along the long sides, joists across between them, the floor's deck on top, the ceiling's lining below
    add_slab(module + "_floor", origin, true, group);
    add_slab(module + "_ceiling", origin + z * (floor_top + guide.clear_height), false, group);

    // long sides: open towards a neighbour of the same room; between two rooms one wall, in the module of the smaller room, the lower
    // column on a tie; a wall on the outside
    const std::array<size_t, 2> room = room_of(column);
    const std::array<size_t, 2> previous = column > 0 ? room_of(column - 1) : std::array<size_t, 2>{room[0], 0};
    const std::array<size_t, 2> next = column + 1 < columns() ? room_of(column + 1) : std::array<size_t, 2>{room[0], 0};
    const bool front_inside = column > 0 && previous[0] == room[0];
    const bool back_inside = column + 1 < columns() && next[0] == room[0];
    const bool front_wall = column == 0 || (!front_inside && room[1] < previous[1]);
    const bool back_wall = column + 1 == columns() || (!back_inside && room[1] <= next[1]);
    const ModuleLayout& layout = guide.layouts[column % guide.layouts.size()];

    // posts: at the four corners, a wall thick, on the floor deck under the ceiling, and evenly along a side open inside the room
    const std::shared_ptr<TreeNode> posts = group_named(module + "_posts", group);
    std::vector<std::array<double, 2>> places = {
        {0.5 * wall, 0.5 * wall},
        {guide.length - 0.5 * wall, 0.5 * wall},
        {guide.length - 0.5 * wall, guide.width - 0.5 * wall},
        {0.5 * wall, guide.width - 0.5 * wall},
    };

    for (size_t k = 1; k <= guide.side_posts; k++) {
        const double along = guide.length * static_cast<double>(k) / static_cast<double>(guide.side_posts + 1);

        if (front_inside)
            places.push_back({along, 0.5 * wall});

        if (back_inside)
            places.push_back({along, guide.width - 0.5 * wall});
    }

    for (size_t k = 0; k < places.size(); k++) {
        const Point foot = origin + x * places[k][0] + y * places[k][1] + z * floor_top;
        const Polyline axis({foot, foot + z * guide.clear_height});
        add(std::make_shared<Beam>(axis, profile_rectangle(wall, wall), std::vector<Vector>{x}, fmt::format("{}_post_{}", module, k)), posts);
    }

    // long walls: between the posts on the sides this module closes
    WallGuide long_guide;
    long_guide.length = guide.length - 2.0 * wall;
    long_guide.height = guide.clear_height;
    long_guide.stud_width = guide.stud_width;
    long_guide.stud_depth = guide.stud_depth;
    long_guide.stud_spacing = guide.stud_spacing;
    long_guide.sheet_thickness = guide.sheet_thickness;

    if (front_wall) {
        long_guide.openings = layout.openings[0];
        const Plane front_plane(origin + x * wall + y * (0.5 * wall) + z * floor_top, x, z);
        const FramedPanel front(*this, long_guide, front_plane, module + "_wall_front", group);
    }

    // back wall: stopping at a porch's start wall, the porch open on this side
    if (back_wall) {
        long_guide.length = guide.length - 2.0 * wall + (layout.porch > 0.0 ? wall - layout.porch : 0.0);
        long_guide.openings = layout.openings[1];
        const Plane back_plane(origin + x * (guide.length - wall) + y * (guide.width - 0.5 * wall) + z * floor_top, x * -1.0, z);
        const FramedPanel back(*this, long_guide, back_plane, module + "_wall_back", group);
    }

    // ends: a wall between the long sides, the start wall set back behind a porch
    WallGuide end_guide = long_guide;
    end_guide.length = guide.width - 2.0 * wall;
    end_guide.openings = layout.openings[2];
    const Plane start_plane(origin + x * (layout.porch + 0.5 * wall) + y * (guide.width - wall) + z * floor_top, y * -1.0, z);
    const FramedPanel start(*this, end_guide, start_plane, module + "_wall_start", group);
    end_guide.openings = layout.openings[3];
    const Plane end_plane(origin + x * (guide.length - 0.5 * wall) + y * wall + z * floor_top, y, z);
    const FramedPanel end(*this, end_guide, end_plane, module + "_wall_end", group);
}

}
