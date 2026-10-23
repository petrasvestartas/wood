#include "src/templates/module/room_module.h"
#include "wood_profile.h"

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════

RoomModule::RoomModule(const RoomModuleGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // modules: per storey and column one module, its floor and ceiling boxes, its downstand beams, its posts and its framed walls
    for (size_t storey = 0; storey < guide.storeys; storey++)
        for (size_t column = 0; column < columns(); column++)
            add_module(storey, column);

    // contacts: every two members and sheets that touch, in a module, side by side and stacked
    add_module_contacts(*this);
}

// ═══════════════════════════════════════════════════════════════════════════
// Sizes
// ═══════════════════════════════════════════════════════════════════════════

double RoomModule::floor_thickness() const {
    return guide.board_thickness + guide.floor_depth + guide.floor_deck_thickness;
}

double RoomModule::ceiling_thickness() const {
    return 2.0 * guide.board_thickness + guide.ceiling_depth;
}

double RoomModule::wall_thickness() const {
    return guide.stud_depth + 2.0 * guide.sheet_thickness;
}

double RoomModule::module_height() const {
    return floor_thickness() + guide.clear_height + ceiling_thickness();
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

void RoomModule::add_box(const std::string& name, const Point& corner, double depth, double edge, double joist, double bottom, double top, const std::shared_ptr<TreeNode>& parent) {

    const std::shared_ptr<TreeNode> box = group_named(name, parent);
    const std::shared_ptr<TreeNode> beams = group_named(name + "_beams", box);
    const std::shared_ptr<TreeNode> boards = group_named(name + "_boards", box);
    const Vector x(1.0, 0.0, 0.0);
    const Vector y(0.0, 1.0, 0.0);
    const Vector z(0.0, 0.0, 1.0);
    const Point base = corner + z * (bottom + 0.5 * depth);
    const double inner = guide.width - 2.0 * edge;

    // edge beams: along both long sides over the whole length, and across both ends between them
    for (size_t side = 0; side < 2; side++) {
        const Point start = base + y * (side == 0 ? 0.5 * edge : guide.width - 0.5 * edge);
        const Polyline axis({start, start + x * guide.length});
        add(std::make_shared<Beam>(axis, profile_rectangle(edge, depth), std::vector<Vector>{z}, fmt::format("{}_edge_{}", name, side)), beams);
    }

    for (size_t side = 0; side < 2; side++) {
        const Point start = base + x * (side == 0 ? 0.5 * edge : guide.length - 0.5 * edge) + y * edge;
        const Polyline axis({start, start + y * inner});
        add(std::make_shared<Beam>(axis, profile_rectangle(edge, depth), std::vector<Vector>{z}, fmt::format("{}_end_{}", name, side)), beams);
    }

    // joists: across between the long edge beams, dividing the length between the end beams evenly
    const double span = guide.length - edge;
    const size_t bays = static_cast<size_t>(std::ceil(span / guide.joist_spacing));

    for (size_t i = 1; i < bays; i++) {
        const Point start = base + x * (0.5 * edge + span * static_cast<double>(i) / static_cast<double>(bays)) + y * edge;
        const Polyline axis({start, start + y * inner});
        add(std::make_shared<Beam>(axis, profile_rectangle(joist, depth), std::vector<Vector>{z}, fmt::format("{}_joist_{}", name, i - 1)), beams);
    }

    // boards: one under the box and one over it, over the whole module
    const Polyline bottom_lower = Polyline::rectangle(corner, x, y, guide.length, guide.width);
    const Polyline bottom_upper = Polyline::rectangle(corner + z * bottom, x, y, guide.length, guide.width);
    add(std::make_shared<Plate>(bottom_lower, bottom_upper, name + "_bottom"), boards);
    const Polyline top_lower = Polyline::rectangle(corner + z * (bottom + depth), x, y, guide.length, guide.width);
    const Polyline top_upper = Polyline::rectangle(corner + z * (bottom + depth + top), x, y, guide.length, guide.width);
    add(std::make_shared<Plate>(top_lower, top_upper, name + "_top"), boards);
}

void RoomModule::add_module(size_t storey, size_t column) {

    const std::string module = fmt::format("module_{}_{}", storey, column);
    const std::shared_ptr<TreeNode> group = group_named(module);
    const Vector corner(0.0, guide.width * static_cast<double>(column), module_height() * static_cast<double>(storey));
    const Point origin = Point(0.0, 0.0, 0.0) + corner;
    const double wall = wall_thickness();
    const double floor_top = floor_thickness();
    const double ceiling_bottom = floor_top + guide.clear_height;
    const Vector x(1.0, 0.0, 0.0);
    const Vector y(0.0, 1.0, 0.0);
    const Vector z(0.0, 0.0, 1.0);

    // floor and ceiling: a box each, the floor's deeper with its deck on top, the ceiling's lighter between its lining and its top board
    add_box(module + "_floor", origin, guide.floor_depth, guide.floor_edge_width, guide.floor_joist_width, guide.board_thickness, guide.floor_deck_thickness, group);
    add_box(module + "_ceiling", origin + z * ceiling_bottom, guide.ceiling_depth, guide.ceiling_edge_width, guide.ceiling_joist_width, guide.board_thickness, guide.board_thickness, group);

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

    // downstand beams: under the ceiling along every long side without a wall, a wall wide, carrying the ceiling box from post to post
    const std::array<bool, 2> open = {!front_wall, !back_wall};
    const std::shared_ptr<TreeNode> downstands = group_named(module + "_downstands", group);

    for (size_t side = 0; side < 2; side++) {
        if (!open[side])
            continue;

        const Point start = origin + y * (side == 0 ? 0.5 * wall : guide.width - 0.5 * wall) + z * (ceiling_bottom - 0.5 * guide.downstand_depth);
        const Polyline axis({start, start + x * guide.length});
        add(std::make_shared<Beam>(axis, profile_rectangle(wall, guide.downstand_depth), std::vector<Vector>{z}, fmt::format("{}_downstand_{}", module, side == 0 ? "front" : "back")), downstands);
    }

    // posts: at the four corners, a wall thick, on the floor deck under the ceiling or its downstand beam, and evenly along a side without a wall
    const std::shared_ptr<TreeNode> posts = group_named(module + "_posts", group);
    std::vector<std::array<double, 2>> places = {
        {0.5 * wall, 0.5 * wall},
        {guide.length - 0.5 * wall, 0.5 * wall},
        {guide.length - 0.5 * wall, guide.width - 0.5 * wall},
        {0.5 * wall, guide.width - 0.5 * wall},
    };

    for (size_t k = 1; k <= guide.side_posts; k++) {
        const double along = guide.length * static_cast<double>(k) / static_cast<double>(guide.side_posts + 1);

        if (open[0])
            places.push_back({along, 0.5 * wall});

        if (open[1])
            places.push_back({along, guide.width - 0.5 * wall});
    }

    for (size_t k = 0; k < places.size(); k++) {
        const bool under_downstand = open[places[k][1] < 0.5 * guide.width ? 0 : 1];
        const double height = guide.clear_height - (under_downstand ? guide.downstand_depth : 0.0);
        const Point foot = origin + x * places[k][0] + y * places[k][1] + z * floor_top;
        const Polyline axis({foot, foot + z * height});
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
