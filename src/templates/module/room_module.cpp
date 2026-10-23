#include "src/templates/module/room_module.h"
#include "wood_profile.h"

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════

RoomModule::RoomModule(const RoomModuleGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // modules: per storey and column one module, its floor and ceiling boxes, its posts and its framed walls
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
    return guide.floor_depth;
}

double RoomModule::ceiling_thickness() const {
    return 2.0 * guide.board_thickness + guide.ceiling_joist_depth;
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

void RoomModule::add_box(const std::string& name, const Point& corner, bool floor, const std::array<double, 2>& ends, const std::array<double, 2>& hang, const std::shared_ptr<TreeNode>& parent) {

    const std::shared_ptr<TreeNode> box = group_named(name, parent);
    const std::shared_ptr<TreeNode> beams = group_named(name + "_beams", box);
    const std::shared_ptr<TreeNode> boards = group_named(name + "_boards", box);
    const Vector x(1.0, 0.0, 0.0);
    const Vector y(0.0, 1.0, 0.0);
    const Vector z(0.0, 0.0, 1.0);
    const double edge = wall_thickness();
    const double depth = floor ? floor_thickness() : ceiling_thickness();
    const double joist_width = floor ? guide.floor_joist_width : guide.ceiling_joist_width;
    const double joist_depth = floor ? guide.floor_joist_depth : guide.ceiling_joist_depth;
    const double room = floor ? guide.deck_thickness : guide.board_thickness;
    const double board = guide.board_thickness;
    const double inner = guide.width - 2.0 * edge;
    const double field_length = ends[1] - ends[0];
    const Point field = corner + x * ends[0] + y * edge;

    // edge beams: a wall wide, along both long sides over the whole length, each reaching its hang below
    for (size_t side = 0; side < 2; side++) {
        const Point start = corner + y * (side == 0 ? 0.5 * edge : guide.width - 0.5 * edge) + z * (0.5 * (depth - hang[side]));
        const Polyline axis({start, start + x * guide.length});
        add(std::make_shared<Beam>(axis, profile_rectangle(edge, depth + hang[side]), std::vector<Vector>{z}, fmt::format("{}_edge_{}", name, side)), beams);
    }

    // joists: across between the edge beams, the first and last at the field's ends, the rest dividing it evenly, under the deck of a
    // floor, on the lining of a ceiling
    const double joists_base = floor ? depth - room - joist_depth : room;
    const double span = field_length - joist_width;
    const size_t bays = static_cast<size_t>(std::ceil(span / guide.joist_spacing));

    for (size_t i = 0; i <= bays; i++) {
        const double along = ends[0] + 0.5 * joist_width + span * static_cast<double>(i) / static_cast<double>(bays);
        const Point start = corner + x * along + y * edge + z * (joists_base + 0.5 * joist_depth);
        const Polyline axis({start, start + y * inner});
        add(std::make_shared<Beam>(axis, profile_rectangle(joist_width, joist_depth), std::vector<Vector>{z}, fmt::format("{}_joist_{}", name, i)), beams);
    }

    // boards: in the field between the edge beams, the deck or lining flush with the face to the room, a board on the joists' other face
    const double room_base = floor ? depth - room : 0.0;
    const double other_base = floor ? joists_base - board : joists_base + joist_depth;
    const Polyline room_lower = Polyline::rectangle(field + z * room_base, x, y, field_length, inner);
    const Polyline room_upper = Polyline::rectangle(field + z * (room_base + room), x, y, field_length, inner);
    add(std::make_shared<Plate>(room_lower, room_upper, name + (floor ? "_deck" : "_lining")), boards);
    const Polyline other_lower = Polyline::rectangle(field + z * other_base, x, y, field_length, inner);
    const Polyline other_upper = Polyline::rectangle(field + z * (other_base + board), x, y, field_length, inner);
    add(std::make_shared<Plate>(other_lower, other_upper, name + "_board"), boards);
}

void RoomModule::add_module(size_t storey, size_t column) {

    const std::string module = fmt::format("module_{}_{}", storey, column);
    const std::shared_ptr<TreeNode> group = group_named(module);
    const Vector corner(0.0, (guide.width + guide.joint) * static_cast<double>(column), module_height() * static_cast<double>(storey));
    const Point origin = Point(0.0, 0.0, 0.0) + corner;
    const double wall = wall_thickness();
    const double floor_top = floor_thickness();
    const double ceiling_bottom = floor_top + guide.clear_height;
    const Vector x(1.0, 0.0, 0.0);
    const Vector y(0.0, 1.0, 0.0);
    const Vector z(0.0, 0.0, 1.0);

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
    const std::array<bool, 2> open = {!front_wall, !back_wall};

    // floor and ceiling: edge beams a wall wide under the walls and posts, joists across flush with the room, the floor's top under its
    // deck, the ceiling's underside on its lining, between the end walls; behind a porch the floor and ceiling run out to the start; on
    // a side without a wall the ceiling's edge beam one deeper beam reaching the downstand depth into the room, from post to post
    const std::array<double, 2> ends = {layout.porch > 0.0 ? 0.0 : wall, guide.length - wall};
    const std::array<double, 2> hang = {open[0] ? guide.downstand_depth : 0.0, open[1] ? guide.downstand_depth : 0.0};
    add_box(module + "_floor", origin, true, ends, {0.0, 0.0}, group);
    add_box(module + "_ceiling", origin + z * ceiling_bottom, false, ends, hang, group);

    // posts: at the four corners, a wall thick, on the floor under the ceiling or its deeper edge beam, and evenly along a side without a wall
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
        const double height = guide.clear_height - hang[places[k][1] < 0.5 * guide.width ? 0 : 1];
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

    // ends: a wall between the long edge beams over the module's full height, its openings lifted by the floor; the start wall set back
    // behind a porch stands on the floor's deck
    WallGuide end_guide = long_guide;
    end_guide.length = guide.width - 2.0 * wall;
    end_guide.height = module_height();
    end_guide.openings = layout.openings[3];

    for (Opening& opening : end_guide.openings) {
        opening.sill += floor_top;
        opening.head += floor_top;
    }

    const Plane end_plane(origin + x * (guide.length - 0.5 * wall) + y * wall, y, z);
    const FramedPanel end(*this, end_guide, end_plane, module + "_wall_end", group);
    end_guide.openings = layout.openings[2];

    if (layout.porch > 0.0)
        end_guide.height = guide.clear_height;
    else
        for (Opening& opening : end_guide.openings) {
            opening.sill += floor_top;
            opening.head += floor_top;
        }

    const double start_base = layout.porch > 0.0 ? floor_top : 0.0;
    const Plane start_plane(origin + x * (layout.porch + 0.5 * wall) + y * (guide.width - wall) + z * start_base, y * -1.0, z);
    const FramedPanel start(*this, end_guide, start_plane, module + "_wall_start", group);
}

}
