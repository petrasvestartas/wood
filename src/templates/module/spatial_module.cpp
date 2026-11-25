#include "src/templates/module/spatial_module.h"
#include "wood_profile.h"

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
SpatialModule::SpatialModule(
    WoodSession& session,
    const SpatialModuleGuide& guide,
    const Plane& base,
    const std::string& name,
    const std::shared_ptr<TreeNode>& parent
)
    : guide(guide), base(base), name(name), _session(session), _group(session.group_named(name, parent)) {

    // floor: one plate, or ribs across between two edge ribs, a board on each face
    add_floor();

    // walls: on the floor, the long ones over the whole length, the ends between them
    add_walls();

    // frames: posts and a downstand beam along every open long side
    add_frames();

    // ceiling: on the walls and the downstand beams
    add_ceiling();

    // braces: crossed boards that hold an open side while the module is carried
    add_braces();

    // pins: along every edge between two solid plates
    add_pins();
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════
void SpatialModule::add_floor() {

    const double thickness = guide.floor_thickness();
    const Plane plane(local(0.0, 0.0, 0.5 * thickness), base.x_axis(), base.y_axis());
    add_panel("floor", box_guide(guide.floor_rib, guide.floor_sheet), plane, guide.solid_floor, thickness);
}

void SpatialModule::add_ceiling() {

    const double thickness = guide.ceiling_thickness();
    const double level = guide.floor_thickness() + guide.clear_height + 0.5 * thickness;
    const Plane plane(local(0.0, 0.0, level), base.x_axis(), base.y_axis());
    add_panel("ceiling", box_guide(guide.ceiling_rib, guide.ceiling_sheet), plane, guide.solid_ceiling, thickness);
}

void SpatialModule::add_walls() {

    const double wall = guide.wall_thickness();
    const double foot = guide.floor_thickness();
    const Vector x = base.x_axis();
    const Vector y = base.y_axis();
    const Vector z = base.z_axis();
    const std::array<std::string, 4> parts = {"wall_front", "wall_back", "wall_start", "wall_end"};
    const std::array<double, 4> lengths = {
        guide.length, guide.length, guide.width - 2.0 * wall, guide.width - 2.0 * wall
    };
    const std::array<Plane, 4> planes = {
        Plane(local(0.0, 0.5 * wall, foot), x, z),
        Plane(local(guide.length, guide.width - 0.5 * wall, foot), x * -1.0, z),
        Plane(local(0.5 * wall, guide.width - wall, foot), y * -1.0, z),
        Plane(local(guide.length - 0.5 * wall, wall, foot), y, z),
    };

    for (size_t side = 0; side < 4; side++) {
        if (!guide.closed[side])
            continue;

        WallGuide panel;
        panel.length = lengths[side];
        panel.height = guide.clear_height;
        panel.stud_width = guide.stud_width;
        panel.stud_depth = guide.stud_depth;
        panel.stud_spacing = guide.spacing;
        panel.sheet_thickness = guide.wall_sheet;
        panel.openings = guide.openings[side];
        add_panel(parts[side], panel, planes[side], guide.solid_walls, wall);
    }
}

void SpatialModule::add_frames() {

    const double wall = guide.wall_thickness();
    const double foot = guide.floor_thickness();
    const double head = foot + guide.clear_height;
    const double inner = guide.length - wall;

    for (size_t side = 0; side < 2; side++) {
        if (guide.closed[side])
            continue;

        const std::shared_ptr<TreeNode> frame = _session.group_named(fmt::format("{}_frame_{}", name, side), _group);
        const double across = side == 0 ? 0.5 * wall : guide.width - 0.5 * wall;

        // posts: at both corners and evenly between them, a wall thick
        for (size_t k = 0; k < guide.side_posts + 2; k++) {
            const double share = static_cast<double>(k) / static_cast<double>(guide.side_posts + 1);
            const double along = 0.5 * wall + inner * share;
            const Polyline axis({local(along, across, foot), local(along, across, head - guide.downstand)});
            _session.add(
                std::make_shared<EdgeStraightElement>(
                    axis,
                    profile_rectangle(wall, wall),
                    std::vector<Vector>{base.x_axis()},
                    fmt::format("{}_post_{}_{}", name, side, k)
                ),
                frame
            );
        }

        // downstand: the beam on the posts under the ceiling, over the whole length
        const double level = head - 0.5 * guide.downstand;
        const Polyline axis({local(0.0, across, level), local(guide.length, across, level)});
        _session.add(
            std::make_shared<EdgeStraightElement>(
                axis,
                profile_rectangle(wall, guide.downstand),
                std::vector<Vector>{base.z_axis()},
                fmt::format("{}_downstand_{}", name, side)
            ),
            frame
        );
    }
}

void SpatialModule::add_braces() {

    if (guide.brace <= 0.0)
        return;

    const double wall = guide.wall_thickness();
    const double foot = guide.floor_thickness();
    const double head = foot + guide.clear_height - guide.downstand;
    const double half = 60.0;

    for (size_t side = 0; side < 2; side++) {
        if (guide.closed[side])
            continue;

        const std::shared_ptr<TreeNode> braces = _session.group_named(fmt::format("{}_braces_{}", name, side), _group);
        const double face = side == 0 ? wall : guide.width - wall;
        const double inwards = side == 0 ? 1.0 : -1.0;

        // boards: one down and one up the side, from post to post, the second on the first
        for (size_t k = 0; k < 2; k++) {
            const double level = face + inwards * guide.brace * static_cast<double>(k);
            const Point start = local(0.5 * wall, level, k == 0 ? head - 2.0 * half : foot + 2.0 * half);
            const Point end = local(guide.length - 0.5 * wall, level, k == 0 ? foot + 2.0 * half : head - 2.0 * half);
            const Vector along = (end - start).normalized();
            const Vector up = base.y_axis().cross(along);
            const Polyline bottom = Polyline::rectangle(
                start - up * half, along, up, (end - start).magnitude(), 2.0 * half
            );
            _session.add(
                std::make_shared<FacePlanarElement>(
                    bottom,
                    bottom.translated(base.y_axis() * (inwards * guide.brace)),
                    fmt::format("{}_brace_{}_{}", name, side, k)
                ),
                braces
            );
        }
    }
}

void SpatialModule::add_pins() {

    if (guide.pin_spacing <= 0.0)
        return;

    const std::shared_ptr<TreeNode> pins = _session.group_named(name + "_pins", _group);
    const std::array<std::array<std::string, 2>, 12> pairs = {{
        {"_floor", "_wall_front"},
        {"_floor", "_wall_back"},
        {"_floor", "_wall_start"},
        {"_floor", "_wall_end"},
        {"_wall_front", "_wall_start"},
        {"_wall_front", "_wall_end"},
        {"_wall_back", "_wall_start"},
        {"_wall_back", "_wall_end"},
        {"_ceiling", "_wall_front"},
        {"_ceiling", "_wall_back"},
        {"_ceiling", "_wall_start"},
        {"_ceiling", "_wall_end"},
    }};

    for (const std::array<std::string, 2>& pair : pairs) {
        const std::shared_ptr<FacePlanarElement> through = _session.get_element_by_name<FacePlanarElement>(name + pair[0]);
        const std::shared_ptr<FacePlanarElement> into = _session.get_element_by_name<FacePlanarElement>(name + pair[1]);

        if (!through || !into)
            continue;

        // edge: the pins in a row along it, from the far face of the first plate into the edge of the second
        const std::shared_ptr<InteractionContactFace> edge = _session.compute_face_contact(through, into);
        const bool standing = pair[0].starts_with("_wall");
        const double run = standing ? guide.clear_height : edge->polygon.length() * 0.5;
        const size_t count = std::max<size_t>(2, static_cast<size_t>(std::ceil(run / guide.pin_spacing)) + 1);
        const std::shared_ptr<JointBeamElement> joint = JointBeamElement::headed_pins(
            *through,
            *into,
            *edge,
            standing ? PinLayout::vertical : PinLayout::horizontal,
            count,
            20.0,
            0.0,
            4.0,
            2.0 * through->thickness
        );

        if (!joint)
            throw std::runtime_error(fmt::format("{}{}: no room for pins on {}{}", name, pair[0], name, pair[1]));

        joint->name = fmt::format("{}_pins{}{}", name, pair[0], pair[1]);
        _session.add(joint, pins);
        _session.add_interaction(joint, through, joint->interaction(0));
        _session.add_interaction(joint, into, joint->interaction(1));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Steps
// ═══════════════════════════════════════════════════════════════════════════
void SpatialModule::add_panel(
    const std::string& part, const WallGuide& panel, const Plane& plane, bool solid, double thickness
) {

    const std::string named = fmt::format("{}_{}", name, part);

    if (!solid) {
        const FramedPanel framed(_session, panel, plane, named, _group);
        return;
    }

    const std::array<double, 2> levels = {-0.5 * thickness, 0.5 * thickness};
    _session.add(panel_plate(panel, plane, levels, named), _session.group_named(named + "_panel", _group));
}

Point SpatialModule::local(double along, double across, double up) const {
    return base.origin() + base.x_axis() * along + base.y_axis() * across + base.z_axis() * up;
}

WallGuide SpatialModule::box_guide(double rib, double sheet) const {

    WallGuide box;
    box.length = guide.length;
    box.height = guide.width;
    box.stud_width = guide.rib_width;
    box.stud_depth = rib;
    box.stud_spacing = guide.spacing;
    box.sheet_thickness = sheet;
    box.openings = guide.shafts;

    return box;
}

AssemblyModule SpatialModule::assembly(const std::vector<AssemblyModule>& inside) const {

    std::vector<AssemblyModule> pieces;

    // panels: the floor, the walls and the ceiling, each framed on its own
    const std::array<std::string, 8> parts = {
        "floor", "wall_front", "wall_back", "wall_start", "wall_end", "frame_0", "frame_1", "ceiling"
    };

    for (const std::string& part : parts) {
        const std::string panel = fmt::format("{}_{}", name, part);
        std::vector<std::shared_ptr<Element>> members = _session.get_elements_under<Element>(panel);

        if (members.empty())
            members = _session.get_elements_under<Element>(panel + "_panel");

        if (part == "ceiling")
            pieces.insert(pieces.end(), inside.begin(), inside.end());

        if (members.size() == 1)
            pieces.push_back(AssemblyModule::part(members[0]));
        else if (!members.empty())
            pieces.push_back(panel_assembly(panel, members));
    }

    // pins: along their axes, then the braces on the posts
    for (const AssemblyModule& pins : pin_parts(_session, _session.get_elements_under<JointBeamElement>(name + "_pins")))
        pieces.push_back(pins);

    for (size_t side = 0; side < 2; side++)
        for (const std::shared_ptr<Element>& brace : _session.get_elements_under<Element>(fmt::format("{}_braces_{}", name, side)))
            pieces.push_back(AssemblyModule::part(brace));

    return AssemblyModule::module(name, pieces);
}

// ═══════════════════════════════════════════════════════════════════════════
// ModuleTypes
// ═══════════════════════════════════════════════════════════════════════════
ModuleTypes::ModuleTypes(
    const std::vector<std::pair<std::string, SpatialModuleGuide>>& types, const std::string& name
)
    : WoodSession(name), types(types) {

    // modules: side by side across, each assembled on its own
    double across = 0.0;

    for (const std::pair<std::string, SpatialModuleGuide>& type : types) {
        const Plane base(Point(0.0, across, 0.0), Vector::x_axis(), Vector::y_axis());
        const SpatialModule module(*this, type.second, base, type.first);
        assemblies.push_back(module.assembly());
        across += type.second.width + GAP;
    }

    // contacts: every two members and plates that touch
    add_module_contacts(*this);
}

}
