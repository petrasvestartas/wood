#include "src/templates/module/module_building.h"
#include "wood_profile.h"

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
ModuleBuilding::ModuleBuilding(const ModuleBuildingGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // skeleton: posts, beams, and decks or ledgers, when the modules do not carry each other
    add_skeleton();

    // modules: per storey and column the battens and strips a module stands on, then the module
    for (size_t storey = 0; storey < guide.storeys; storey++)
        for (size_t row = 0; row < rows(); row++)
            for (size_t column = 0; column < columns(); column++) {
                add_bearings(row, storey, column);
                add_module(row, storey, column);
            }

    // corridor: planar floors and a roof between two rows
    add_corridor();

    // contacts: every two members and plates that touch
    add_module_contacts(*this);

    // connectors: plates over the joints between modules, pins between beams and posts
    add_couplers();
    add_beam_pins();

    // assembly: the skeleton, then storey by storey the bearings and the modules, the couplers last
    assemblies = {compute_assembly()};
}

// ═══════════════════════════════════════════════════════════════════════════
// Sizes
// ═══════════════════════════════════════════════════════════════════════════
size_t ModuleBuilding::columns() const {

    size_t count = 0;

    for (const size_t modules : guide.rooms)
        count += modules;

    return count;
}

size_t ModuleBuilding::rows() const {
    return guide.corridor > 0.0 ? 2 : 1;
}

size_t ModuleBuilding::bays() const {
    return guide.post > 0.0 ? (columns() + guide.bay - 1) / guide.bay : 0;
}

size_t ModuleBuilding::levels() const {
    return guide.post > 0.0 ? (guide.storeys + guide.level - 1) / guide.level : 0;
}

double ModuleBuilding::bearing_height() const {
    return guide.batten + guide.strip;
}

double ModuleBuilding::level_height() const {

    const double height = guide.modules[0].height();

    if (guide.deck <= 0.0)
        return guide.ledger + guide.strip + height + guide.clearance;

    const double stack = static_cast<double>(guide.level) * (height + bearing_height());

    return guide.beam + guide.deck + stack + guide.clearance;
}

double ModuleBuilding::bay_width() const {

    const double modules = static_cast<double>(guide.bay);

    return modules * guide.modules[0].width + (modules - 1.0) * guide.joint + 2.0 * guide.clearance + guide.post;
}

Point ModuleBuilding::corner(size_t storey, size_t column) const {

    const double step = guide.modules[0].width + guide.joint;
    const double rise = guide.modules[0].height() + bearing_height();

    if (guide.post <= 0.0)
        return Point(0.0, step * static_cast<double>(column), bearing_height() + rise * static_cast<double>(storey));

    const double bay = static_cast<double>(column / guide.bay);
    const double place = static_cast<double>(column % guide.bay);
    const double level = static_cast<double>(storey / guide.level);
    const double stacked = static_cast<double>(storey % guide.level);
    const double across = bay * bay_width() + 0.5 * guide.post + guide.clearance + place * step;
    const double seat = guide.deck > 0.0 ? guide.beam + guide.deck + bearing_height() : guide.ledger + guide.strip;

    return Point(0.0, across, level * level_height() + seat + stacked * rise);
}

size_t ModuleBuilding::room_of(size_t column) const {

    size_t start = 0;

    for (size_t room = 0; room < guide.rooms.size(); room++) {
        start += guide.rooms[room];

        if (column < start)
            return room;
    }

    return guide.rooms.size();
}

SpatialModuleGuide ModuleBuilding::module_guide(size_t column) const {

    SpatialModuleGuide module = guide.modules[column % guide.modules.size()];

    if (column > 0 && room_of(column - 1) == room_of(column))
        module.closed[0] = false;

    if (column + 1 < columns() && room_of(column + 1) == room_of(column))
        module.closed[1] = false;

    return module;
}

Plane ModuleBuilding::module_base(size_t row, size_t storey, size_t column) const {

    const Point start = corner(storey, column);

    if (row == 0)
        return Plane(start, Vector::x_axis(), Vector::y_axis());

    // second row: turned half round, its start ends outside too, its columns the other way
    const double far = 2.0 * guide.modules[0].length + guide.corridor;
    const Point turned(far, corner(storey, columns() - 1 - column)[1] + guide.modules[0].width, start[2]);

    return Plane(turned, Vector::x_axis() * -1.0, Vector::y_axis() * -1.0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Skeleton
// ═══════════════════════════════════════════════════════════════════════════
void ModuleBuilding::add_skeleton() {

    if (guide.post <= 0.0)
        return;

    const std::shared_ptr<TreeNode> skeleton = group_named("skeleton");
    const std::shared_ptr<TreeNode> posts = group_named("posts", skeleton);
    const double length = guide.modules[0].length;
    const double height = static_cast<double>(levels()) * level_height();
    const std::array<double, 2> ends = {0.5 * guide.post, length - 0.5 * guide.post};
    const Vector x = Vector::x_axis();
    const Vector y = Vector::y_axis();
    const Vector z = Vector::z_axis();

    // posts: a pair per row, over the full height
    for (size_t line = 0; line <= bays(); line++)
        for (size_t end = 0; end < 2; end++) {
            const Point foot(ends[end], static_cast<double>(line) * bay_width(), 0.0);
            const Polyline axis({foot, foot + z * height});
            add(std::make_shared<EdgeStraightElement>(
                    axis,
                    profile_rectangle(guide.post, guide.post),
                    std::vector<Vector>{x},
                    fmt::format("skeleton_post_{}_{}", line, end)
                ),
                posts);
        }

    for (size_t level = 0; level < levels(); level++) {
        const std::shared_ptr<TreeNode> beams = group_named(fmt::format("beams_{}", level), skeleton);
        const double bottom = static_cast<double>(level) * level_height();
        const double middle = bottom + 0.5 * guide.beam;

        // main beams: along the modules, between the two posts of a row
        for (size_t line = 0; line <= bays(); line++) {
            const double across = static_cast<double>(line) * bay_width();
            const Polyline axis({Point(guide.post, across, middle), Point(length - guide.post, across, middle)});
            add(std::make_shared<EdgeStraightElement>(
                    axis,
                    profile_rectangle(guide.post, guide.beam),
                    std::vector<Vector>{z},
                    fmt::format("skeleton_beam_{}_{}", level, line)
                ),
                beams);

            // ledgers: along the foot of both faces of a beam, a module's edge on each
            for (size_t side = 0; guide.deck <= 0.0 && side < 2; side++) {
                if ((line == 0 && side == 0) || (line == bays() && side == 1))
                    continue;

                const double outwards = side == 0 ? -1.0 : 1.0;
                const double place = across + outwards * 0.5 * (guide.post + guide.ledger);
                const double seat = bottom + 0.5 * guide.ledger;
                const Polyline run({Point(guide.post, place, seat), Point(length - guide.post, place, seat)});
                add(std::make_shared<EdgeStraightElement>(
                        run,
                        profile_rectangle(guide.ledger, guide.ledger),
                        std::vector<Vector>{z},
                        fmt::format("skeleton_ledger_{}_{}_{}", level, line, side)
                    ),
                    beams);
            }
        }

        if (guide.deck <= 0.0)
            continue;

        const std::shared_ptr<TreeNode> decks = group_named(fmt::format("decks_{}", level), skeleton);

        for (size_t bay = 0; bay < bays(); bay++) {
            const double near = static_cast<double>(bay) * bay_width();
            const double far = near + bay_width();
            const double half = 0.5 * guide.post;

            // end beams: across both ends of a bay, between the posts
            for (size_t end = 0; end < 2; end++) {
                const Polyline axis({Point(ends[end], near + half, middle), Point(ends[end], far - half, middle)});
                add(std::make_shared<EdgeStraightElement>(
                        axis,
                        profile_rectangle(guide.post, guide.beam),
                        std::vector<Vector>{z},
                        fmt::format("skeleton_end_beam_{}_{}_{}", level, bay, end)
                    ),
                    beams);
            }

            // deck: on the beams up to their middles, notched round the posts
            const double top = bottom + guide.beam;
            const double post = guide.post;
            const Polyline outline({
                Point(post, near, top),
                Point(length - post, near, top),
                Point(length - post, near + half, top),
                Point(length, near + half, top),
                Point(length, far - half, top),
                Point(length - post, far - half, top),
                Point(length - post, far, top),
                Point(post, far, top),
                Point(post, far - half, top),
                Point(0.0, far - half, top),
                Point(0.0, near + half, top),
                Point(post, near + half, top),
                Point(post, near, top),
            });
            add(std::make_shared<FacePlanarElement>(
                    outline, outline.translated(z * guide.deck), fmt::format("skeleton_deck_{}_{}", level, bay)
                ),
                decks);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Modules
// ═══════════════════════════════════════════════════════════════════════════
void ModuleBuilding::add_bearings(size_t row, size_t storey, size_t column) {

    const std::string place = fmt::format("{}_{}_{}", row, storey, column);
    const std::shared_ptr<TreeNode> group =
        group_named("bearings_" + place, group_named(fmt::format("bearings_{}", storey)));
    const SpatialModuleGuide module = module_guide(column);
    const Plane base = module_base(row, storey, column);
    const Vector x = base.x_axis();
    const Vector y = base.y_axis();
    const Vector z = base.z_axis();
    const double wall = module.wall_thickness();
    const bool hung = guide.post > 0.0 && guide.deck <= 0.0;

    for (size_t side = 0; side < 2; side++) {

        // line: under the wall of a closed side, on a ledger the width of the ledger less the clearance
        const double width = hung ? guide.ledger - guide.clearance : guide.bearing;
        const double across = hung ? (side == 0 ? 0.5 * width : module.width - 0.5 * width)
                                   : (side == 0 ? 0.5 * wall : module.width - 0.5 * wall);
        const double inset = hung ? guide.post : 0.0;
        std::vector<std::array<double, 2>> runs = {{inset, module.length - inset}};

        // points: under every post of an open side, its loads come down there
        if (!module.closed[side] && !hung) {
            runs.clear();

            for (size_t k = 0; k < module.side_posts + 2; k++) {
                const double share = static_cast<double>(k) / static_cast<double>(module.side_posts + 1);
                const double start = (module.length - wall) * share;
                runs.push_back({start, start + wall});
            }
        }

        for (size_t k = 0; k < runs.size(); k++) {
            const Point start = base.origin() + x * runs[k][0] + y * across - z * guide.strip;
            const Point end = base.origin() + x * runs[k][1] + y * across - z * guide.strip;

            if (!hung) {
                const Polyline axis({start - z * (0.5 * guide.batten), end - z * (0.5 * guide.batten)});
                add(std::make_shared<EdgeStraightElement>(
                        axis,
                        profile_rectangle(width, guide.batten),
                        std::vector<Vector>{z},
                        fmt::format("batten_{}_{}_{}", place, side, k)
                    ),
                    group);
            }

            const Polyline strip = Polyline::rectangle(start - y * (0.5 * width), x, y, runs[k][1] - runs[k][0], width);
            add(std::make_shared<FacePlanarElement>(
                    strip, strip.translated(z * guide.strip), fmt::format("strip_{}_{}_{}", place, side, k)
                ),
                group);
        }
    }
}

void ModuleBuilding::add_module(size_t row, size_t storey, size_t column) {

    const std::string module = fmt::format("module_{}_{}_{}", row, storey, column);
    const std::shared_ptr<TreeNode> group = group_named(fmt::format("storey_{}", storey));
    const SpatialModuleGuide sizes = module_guide(column);
    const Plane base = module_base(row, storey, column);
    const SpatialModule made(*this, sizes, base, module, group);

    if (!guide.pod) {
        _modules.push_back(made.assembly());
        return;
    }

    // pod: on the module's floor, clear of its walls
    const Point seat = base.origin() + base.x_axis() * guide.pod_at[0] + base.y_axis() * guide.pod_at[1] +
                       base.z_axis() * (sizes.floor_thickness() + guide.pod_at[2]);
    const SpatialModule pod(*this, guide.pod_guide, Plane(seat, base.x_axis(), base.y_axis()), module + "_pod", group);
    _modules.push_back(made.assembly({pod.assembly()}));
}

void ModuleBuilding::add_corridor() {

    if (rows() < 2)
        return;

    const std::shared_ptr<TreeNode> corridor = group_named("corridor");
    const double start = guide.modules[0].length;
    const double end = start + guide.corridor;
    const double gap = 10.0;
    const double size = guide.ledger;
    const Vector x = Vector::x_axis();
    const Vector y = Vector::y_axis();
    const Vector z = Vector::z_axis();

    for (size_t storey = 0; storey <= guide.storeys; storey++) {
        const bool roof = storey == guide.storeys;
        const SpatialModuleGuide sizes = guide.modules[0];
        const double top = corner(roof ? storey - 1 : storey, 0)[2] + (roof ? sizes.height() : sizes.floor_thickness());
        const double seat = top - guide.corridor_panel;
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("corridor_{}", storey), corridor);

        for (size_t column = 0; column < columns(); column++) {
            const double across = corner(0, column)[1];

            // ledgers: on the ends of the two modules facing each other
            for (size_t row = 0; row < 2; row++) {
                const double face = row == 0 ? start + 0.5 * size : end - 0.5 * size;
                const Point from(face, across, seat - 0.5 * size);
                const Polyline axis({from, from + y * sizes.width});
                add(std::make_shared<EdgeStraightElement>(
                        axis,
                        profile_rectangle(size, size),
                        std::vector<Vector>{z},
                        fmt::format("corridor_ledger_{}_{}_{}", storey, column, row)
                    ),
                    group);
            }

            // panel: from ledger to ledger
            const Polyline bottom = Polyline::rectangle(
                Point(start + gap, across, seat), x, y, guide.corridor - 2.0 * gap, sizes.width
            );
            add(std::make_shared<FacePlanarElement>(
                    bottom, bottom.translated(z * guide.corridor_panel), fmt::format("corridor_panel_{}_{}", storey, column)
                ),
                group);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Connectors
// ═══════════════════════════════════════════════════════════════════════════
void ModuleBuilding::add_couplers() {

    if (guide.coupler <= 0.0)
        return;

    const std::shared_ptr<TreeNode> couplers = group_named("couplers");
    const SpatialModuleGuide sizes = guide.modules[0];
    const double half = 0.5 * guide.coupler_width;
    const Vector x = Vector::x_axis();

    for (size_t row = 0; row < rows(); row++)
        for (size_t storey = 1; storey <= guide.storeys; storey++)
            for (size_t line = 0; line <= columns(); line++) {
                const bool left = line > 0;
                const bool right = line < columns();
                const bool over = storey < guide.storeys;

                // modules on two skeleton bays or two skeleton levels are not coupled, nor a roof's corner
                if (guide.post > 0.0 && (line % guide.bay == 0 || storey % guide.level == 0))
                    continue;

                if (!over && !(left && right))
                    continue;

                // plate: over the joint, from the ceiling below to the floor above, flat on the modules' start ends
                const double bottom = corner(storey - 1, 0)[2] + sizes.height() - sizes.ceiling_thickness();
                const double top = over ? corner(storey, 0)[2] + sizes.floor_thickness() : bottom + sizes.ceiling_thickness();
                const double near = left ? corner(0, line - 1)[1] + sizes.width - half : corner(0, 0)[1];
                const double far = right ? corner(0, line)[1] + half : corner(0, line - 1)[1] + sizes.width;
                const double face = row == 0 ? 0.0 : 2.0 * sizes.length + guide.corridor;
                const double outwards = row == 0 ? -1.0 : 1.0;
                const Polyline outline({
                    Point(face, near, bottom),
                    Point(face, far, bottom),
                    Point(face, far, top),
                    Point(face, near, top),
                    Point(face, near, bottom),
                });
                const std::string place = fmt::format("{}_{}_{}", row, storey, line);
                const std::shared_ptr<FacePlanarElement> plate = std::make_shared<FacePlanarElement>(
                    outline, outline.translated(x * (outwards * guide.coupler)), "coupler_" + place
                );
                add(plate, couplers);

                // pins: from the plate into every floor and ceiling under it, four each
                for (size_t k = 0; k < 4; k++) {
                    const bool beside = k % 2 == 0 ? left : right;
                    const bool floor = k >= 2;

                    if (!beside || (floor && !over))
                        continue;

                    const size_t column = k % 2 == 0 ? line - 1 : line;
                    const size_t place_column = row == 0 ? column : columns() - 1 - column;
                    const std::string module = fmt::format("module_{}_{}_{}", row, floor ? storey : storey - 1, place_column);
                    const std::shared_ptr<Element> member = end_member(
                        module + (floor ? "_floor" : "_ceiling"), floor ? sizes.solid_floor : sizes.solid_ceiling
                    );
                    const std::shared_ptr<InteractionContactFace> seat = compute_face_contact(plate, member);

                    if (!seat)
                        throw std::runtime_error(plate->name + " does not lie on " + member->name);

                    seat->name = fmt::format("{}_on_{}", plate->name, member->name);
                    add_interaction(plate, member, seat);
                    const std::shared_ptr<JointBeamElement> pins = JointBeamElement::headed_pins(
                        *plate, *member, *seat, PinLayout::corners, 4, 20.0, 0.0, 4.0, guide.coupler + 80.0
                    );

                    if (!pins)
                        throw std::runtime_error("no room for the pins of " + seat->name);

                    pins->name = fmt::format("coupler_pins_{}_{}", place, k);
                    add(pins, couplers);
                    add_interaction(pins, plate, pins->interaction(0));
                    add_interaction(pins, member, pins->interaction(1));
                }
            }
}

void ModuleBuilding::add_beam_pins() {

    if (guide.post <= 0.0)
        return;

    const std::shared_ptr<TreeNode> group = group_named("beam_pins", group_named("skeleton"));

    for (const std::shared_ptr<EdgeStraightElement>& beam : get_elements<EdgeStraightElement>()) {
        if (!beam->name.starts_with("skeleton_beam") && !beam->name.starts_with("skeleton_end_beam"))
            continue;

        for (const std::shared_ptr<EdgeStraightElement>& post : get_elements_under<EdgeStraightElement>("posts")) {
            const std::shared_ptr<InteractionContactFace> seat = compute_face_contact(post, beam);

            if (!seat)
                continue;

            // pins: leaning from the beam's top into the post, near its middle, clear of those of the beam across
            const std::shared_ptr<JointBeamElement> pins = JointBeamElement::skewed_pins(
                *beam, *post, *seat, 6.0, 80.0, 45.0, 0.5 * guide.post - 30.0, 10.0, 16, true
            );

            if (!pins)
                throw std::runtime_error("no room for the pins of " + beam->name);

            pins->name = fmt::format("{}_pins_{}", beam->name, post->name);
            add(pins, group);
            add_interaction(pins, beam, pins->interaction(0));
            add_interaction(pins, post, pins->interaction(1));
        }
    }
}

std::shared_ptr<Element> ModuleBuilding::end_member(const std::string& panel, bool solid) const {
    return get_element_by_name<Element>(solid ? panel : panel + "_stud_0");
}

// ═══════════════════════════════════════════════════════════════════════════
// Assembly
// ═══════════════════════════════════════════════════════════════════════════
AssemblyModule ModuleBuilding::compute_assembly() const {

    std::vector<AssemblyModule> whole;

    // skeleton: posts, level by level the beams with their ledgers, the decks, the pins between beams and posts
    if (guide.post > 0.0) {
        std::vector<AssemblyModule> skeleton = AssemblyModule::parts_of(get_elements_under<Element>("posts"));

        for (size_t level = 0; level < levels(); level++)
            for (const std::string part : {"beams", "decks"})
                for (const std::shared_ptr<Element>& member : get_elements_under<Element>(fmt::format("{}_{}", part, level)))
                    skeleton.push_back(AssemblyModule::part(member));

        for (const AssemblyModule& pins : pin_parts(*this, get_elements_under<JointBeamElement>("beam_pins")))
            skeleton.push_back(pins);

        whole.push_back(AssemblyModule::module("skeleton", skeleton));
    }

    const size_t across = rows() * columns();

    for (size_t storey = 0; storey < guide.storeys; storey++) {

        // storey: every module on its bearings
        for (size_t place = 0; place < across; place++) {
            const std::string bearings =
                fmt::format("bearings_{}_{}_{}", place / columns(), storey, place % columns());
            whole.push_back(AssemblyModule::part(bearings, get_elements_under<Element>(bearings)));
            whole.push_back(_modules[storey * across + place]);
        }

        // corridor: its floor once the modules on both sides stand, its roof on the last storey
        for (const std::shared_ptr<Element>& member : get_elements_under<Element>(fmt::format("corridor_{}", storey)))
            whole.push_back(AssemblyModule::part(member));
    }

    for (const std::shared_ptr<Element>& member : get_elements_under<Element>(fmt::format("corridor_{}", guide.storeys)))
        whole.push_back(AssemblyModule::part(member));

    // couplers: the plates over the joints and their pins
    for (const std::shared_ptr<FacePlanarElement>& plate : get_elements_under<FacePlanarElement>("couplers"))
        whole.push_back(AssemblyModule::part(plate));

    for (const AssemblyModule& pins : pin_parts(*this, get_elements_under<JointBeamElement>("couplers")))
        whole.push_back(pins);

    return AssemblyModule::module(name, whole);
}

}
