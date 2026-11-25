#include "src/templates/timber_floor/timber_floor.h"
#include "src/templates/module/module_contact.h"
#include "wood_profile.h"

namespace wood_timber_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
TimberFloor::TimberFloor(
    WoodSession& session, const TimberFloorGuide& guide, const Plane& base, const std::string& name
)
    : guide(guide), base(base), name(name), _session(session), _group(session.group_named(name)) {

    // beams: two across the span's ends, plain, rebated or with battens
    add_beams();

    // ribs: along the span, a board under and on them
    add_ribs();

    // panels: side by side, or one notched plate under concrete
    add_panels();
    add_composite();

    // joints: along every joint between two panels
    add_joints();
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════
void TimberFloor::add_beams() {

    const std::shared_ptr<TreeNode> beams = _session.group_named(name + "_beams", _group);
    const double half = 0.5 * guide.beam_width;
    const double foot = -guide.beam_depth;

    for (size_t end = 0; end < 2; end++) {

        // section: in the plane across the beam, x along the span measured towards the floor
        const double middle = end == 0 ? 0.0 : guide.span;
        const double inwards = end == 0 ? 1.0 : -1.0;
        const double top = guide.seat == 0 ? 0.0 : guide.depth();
        std::vector<std::array<double, 2>> section = {{-half, foot}, {half, foot}, {half, top}, {-half, top}};

        if (guide.seat == 1)
            section = {{-half, foot}, {half, foot}, {half, 0.0}, {half - guide.seat_length, 0.0}, {half - guide.seat_length, top}, {-half, top}};

        std::vector<Point> corners;

        for (const std::array<double, 2>& corner : section)
            corners.push_back(local(middle + inwards * corner[0], 0.0, corner[1]));

        const Line axis = Line::from_points(local(middle, 0.0, 0.0), local(middle, guide.width, 0.0));
        _session.add(
            EdgeStraightElement::from_section(axis, Polyline(corners).closed(), fmt::format("{}_beam_{}", name, end)),
            beams
        );

        if (guide.seat != 2)
            continue;

        // batten: along the beam's inner face, the floor on it
        const double batten = 80.0;
        const double place = middle + inwards * (half + 0.5 * guide.seat_length);
        const Polyline run({local(place, 0.0, -0.5 * batten), local(place, guide.width, -0.5 * batten)});
        _session.add(
            std::make_shared<EdgeStraightElement>(
                run,
                profile_rectangle(guide.seat_length, batten),
                std::vector<Vector>{base.z_axis()},
                fmt::format("{}_batten_{}", name, end)
            ),
            beams
        );
    }
}

void TimberFloor::add_ribs() {

    if (guide.rib_depth <= 0.0)
        return;

    const std::shared_ptr<TreeNode> ribs = _session.group_named(name + "_ribs", _group);
    const std::shared_ptr<TreeNode> boards = _session.group_named(name + "_boards", _group);
    const std::array<double, 2> ends = reach();
    const double inner = guide.width - guide.rib_width;
    const size_t bays = static_cast<size_t>(std::ceil(inner / guide.rib_spacing - 1e-6));
    const double foot = guide.bottom;

    // ribs: the first and last at the floor's sides, the rest dividing the width evenly
    for (size_t i = 0; i <= bays; i++) {
        const double near = inner * static_cast<double>(i) / static_cast<double>(bays);
        const double far = near + guide.rib_width;
        const double top = foot + guide.rib_depth;
        _session.add(along({{near, foot}, {far, foot}, {far, top}, {near, top}}, fmt::format("rib_{}", i)), ribs);
    }

    // boards: under and on the ribs over the whole floor
    const std::array<double, 2> thicknesses = {guide.bottom, guide.top};
    const std::array<double, 2> levels = {0.0, foot + guide.rib_depth};
    const std::array<std::string, 2> names = {"bottom", "top"};

    for (size_t k = 0; k < 2; k++) {
        if (thicknesses[k] <= 0.0)
            continue;

        _session.add(
            FacePlanarElement::from_rectangle(
                local(ends[0], 0.0, levels[k]),
                base.x_axis(),
                base.y_axis(),
                ends[1] - ends[0],
                guide.width,
                thicknesses[k],
                fmt::format("{}_{}", name, names[k])
            ),
            boards
        );
    }
}

void TimberFloor::add_panels() {

    if (guide.panel <= 0.0 || guide.concrete > 0.0)
        return;

    const std::shared_ptr<TreeNode> group = _session.group_named(name + "_panels", _group);
    const std::pair<size_t, double> divided = panels();
    const double foot = guide.bottom + guide.rib_depth + guide.top;
    const double head = foot + guide.panel;
    const double third = guide.panel / 3.0;
    const double tongue = 20.0;
    const double groove = 60.0;
    const double strip = 27.0;
    const double rebate = 80.0;

    for (size_t i = 0; i < divided.first; i++) {
        const double near = divided.second * static_cast<double>(i);
        const double far = near + divided.second;
        const bool first = i == 0;
        const bool last = i + 1 == divided.first;

        if (guide.lamella > 0.0) {
            add_lamellas(i, near);
            continue;
        }

        std::vector<std::array<double, 2>> left = {{near, head}, {near, foot}};
        std::vector<std::array<double, 2>> right = {{far, foot}, {far, head}};
        const double middle = 0.5 * (foot + head);

        // tongue in groove: a groove on the near edge, a tongue on the far one
        if (guide.edge == 1) {
            if (!first)
                left = {{near, head}, {near, head - third}, {near + tongue, head - third}, {near + tongue, foot + third}, {near, foot + third}, {near, foot}};

            if (!last)
                right = {{far, foot}, {far, foot + third}, {far + tongue, foot + third}, {far + tongue, head - third}, {far, head - third}, {far, head}};
        }

        // strip: a groove in both edges
        if (guide.edge == 2) {
            const double half = 0.5 * strip;

            if (!first)
                left = {{near, head}, {near, middle + half}, {near + groove, middle + half}, {near + groove, middle - half}, {near, middle - half}, {near, foot}};

            if (!last)
                right = {{far, foot}, {far, middle - half}, {far - groove, middle - half}, {far - groove, middle + half}, {far, middle + half}, {far, head}};
        }

        // board: a rebate in the top of both edges
        if (guide.edge == 3) {
            if (!first)
                left = {{near + rebate, head}, {near + rebate, head - strip}, {near, head - strip}, {near, foot}};

            if (!last)
                right = {{far, foot}, {far, head - strip}, {far - rebate, head - strip}, {far - rebate, head}};
        }

        std::vector<std::array<double, 2>> section = left;
        section.insert(section.end(), right.begin(), right.end());

        if (guide.skin <= 0.0) {
            _session.add(along(section, fmt::format("panel_{}", i)), group);
            continue;
        }

        // hollow: a lower and an upper shell, webs between them at the edges and evenly between
        const double skin = guide.skin;
        _session.add(along({{near, foot}, {far, foot}, {far, foot + skin}, {near, foot + skin}}, fmt::format("panel_{}_lower", i)), group);
        const size_t webs = static_cast<size_t>(std::round(divided.second / 200.0));

        for (size_t w = 0; w <= webs; w++) {
            const double start = near + (divided.second - skin) * static_cast<double>(w) / static_cast<double>(webs);
            _session.add(
                along(
                    {{start, foot + skin}, {start + skin, foot + skin}, {start + skin, head - skin}, {start, head - skin}},
                    fmt::format("panel_{}_web_{}", i, w)
                ),
                group
            );
        }

        _session.add(along({{near, head - skin}, {far, head - skin}, {far, head}, {near, head}}, fmt::format("panel_{}_upper", i)), group);
    }
}

void TimberFloor::add_lamellas(size_t panel, double start) {

    const std::shared_ptr<TreeNode> group = _session.group_named(fmt::format("{}_panel_{}", name, panel), _session.group_named(name + "_panels", _group));
    const std::shared_ptr<TreeNode> dowels = _session.group_named(fmt::format("{}_dowels_{}", name, panel), group);
    const double width = panels().second;
    const size_t count = static_cast<size_t>(std::round(width / guide.lamella));
    const double lamella = width / static_cast<double>(count);
    const std::array<double, 2> ends = reach();
    std::vector<std::shared_ptr<EdgeStraightElement>> boards;

    // boards: on edge side by side, over the whole span
    for (size_t k = 0; k < count; k++) {
        const double near = start + lamella * static_cast<double>(k);
        const double far = near + lamella;
        boards.push_back(along({{near, 0.0}, {far, 0.0}, {far, guide.panel}, {near, guide.panel}}, fmt::format("panel_{}_board_{}", panel, k)));
        _session.add(boards.back(), group);
    }

    // dowels: through all boards across the span, at half the depth
    const double run = ends[1] - ends[0];
    const size_t rows = static_cast<size_t>(std::ceil(run / guide.dowel_spacing));

    for (size_t row = 0; row < rows; row++) {
        const double x = ends[0] + run * (static_cast<double>(row) + 0.5) / static_cast<double>(rows);
        const Line axis = Line::from_points(
            local(x, start, 0.5 * guide.panel), local(x, start + width, 0.5 * guide.panel)
        );
        const std::shared_ptr<JointPinElement> dowel = std::make_shared<JointPinElement>(axis, 0.5 * guide.dowel, 0.2);
        dowel->name = fmt::format("{}_pin_{}_{}", name, panel, row);

        for (const std::shared_ptr<EdgeStraightElement>& board : boards)
            dowel->targets.push_back(board->guid());

        _session.add(dowel, dowels);

        for (const std::shared_ptr<EdgeStraightElement>& board : boards)
            _session.add_interaction(
                dowel, board, std::make_shared<InteractionFeatureSolid>(std::vector<Line>{axis}, 0.5 * guide.dowel)
            );
    }
}

void TimberFloor::add_composite() {

    if (guide.concrete <= 0.0)
        return;

    const std::shared_ptr<TreeNode> group = _session.group_named(name + "_panels", _group);
    const std::array<double, 2> ends = reach();
    const double head = guide.panel;
    const double length = 150.0;
    const double flank = 10.0;
    const double pitch = 450.0;
    std::vector<Point> seam = {local(ends[0], 0.0, head)};

    // seam: the top of the panel, the notches of each half from its end towards mid-span
    std::vector<double> starts;

    for (size_t k = 0; k < guide.notches; k++) {
        const double from = 300.0 + pitch * static_cast<double>(k);
        starts.push_back(ends[0] + from);
        starts.push_back(ends[1] - from - length);
    }

    std::sort(starts.begin(), starts.end());

    for (const double start : starts) {
        seam.push_back(local(start, 0.0, head));
        seam.push_back(local(start + flank, 0.0, head - guide.notch));
        seam.push_back(local(start + length - flank, 0.0, head - guide.notch));
        seam.push_back(local(start + length, 0.0, head));
    }

    seam.push_back(local(ends[1], 0.0, head));

    // plates: under and over the seam, drawn along the span, as wide as the floor
    std::vector<Point> timber = seam;
    timber.push_back(local(ends[1], 0.0, 0.0));
    timber.push_back(local(ends[0], 0.0, 0.0));
    std::vector<Point> topping(seam.rbegin(), seam.rend());
    topping.push_back(local(ends[0], 0.0, head + guide.concrete));
    topping.push_back(local(ends[1], 0.0, head + guide.concrete));
    const Line across = Line::from_points(local(0.0, 0.0, 0.0), local(0.0, guide.width, 0.0));
    _session.add(EdgeStraightElement::from_section(across, Polyline(timber).closed(), name + "_panel"), group);
    _session.add(EdgeStraightElement::from_section(across, Polyline(topping).closed(), name + "_concrete"), group);
}

void TimberFloor::add_joints() {

    if (guide.panel <= 0.0 || guide.edge < 2 || guide.concrete > 0.0)
        return;

    const std::shared_ptr<TreeNode> joints = _session.group_named(name + "_joints", _group);
    const std::pair<size_t, double> divided = panels();
    const std::array<double, 2> ends = reach();
    const double foot = guide.bottom + guide.rib_depth + guide.top;
    const double head = foot + guide.panel;
    const double middle = 0.5 * (foot + head);
    const double half = 13.5;

    for (size_t i = 1; i < divided.first; i++) {
        const double line = divided.second * static_cast<double>(i);
        const std::shared_ptr<EdgeStraightElement> near = _session.get_element_by_name<EdgeStraightElement>(fmt::format("{}_panel_{}", name, i - 1));
        const std::shared_ptr<EdgeStraightElement> far = _session.get_element_by_name<EdgeStraightElement>(fmt::format("{}_panel_{}", name, i));

        // strip: along the whole joint, half in each groove
        if (guide.edge == 2) {
            _session.add(
                along({{line - 60.0, middle - half}, {line + 60.0, middle - half}, {line + 60.0, middle + half}, {line - 60.0, middle + half}}, fmt::format("strip_{}", i)),
                joints
            );
            continue;
        }

        // board: in both rebates, flush with the panels, pinned into each
        if (guide.edge == 3) {
            const std::shared_ptr<FacePlanarElement> board = FacePlanarElement::from_rectangle(
                local(ends[0], line - 80.0, head - 27.0), base.x_axis(), base.y_axis(), ends[1] - ends[0], 160.0, 27.0, fmt::format("{}_board_{}", name, i)
            );
            _session.add(board, joints);

            for (const std::shared_ptr<EdgeStraightElement>& panel : {near, far}) {
                const std::shared_ptr<InteractionContactFace> seat = _session.compute_face_contact(board, panel);
                const size_t count = static_cast<size_t>(std::ceil((ends[1] - ends[0]) / 400.0));
                const std::shared_ptr<JointBeamElement> pins = JointBeamElement::headed_pins(
                    *board, *panel, *seat, PinLayout::horizontal, count, 30.0, 0.0, 4.0, 100.0
                );
                pins->name = fmt::format("{}_pins_{}", board->name, panel->name);
                _session.add(pins, joints);
                _session.add_interaction(pins, board, pins->interaction(0));
                _session.add_interaction(pins, panel, pins->interaction(1));
            }

            continue;
        }

        // keys: one butterfly across the joint at its middle, flush with the panels
        const std::shared_ptr<InteractionContactFace> face = _session.compute_face_contact(near, far);
        const std::shared_ptr<JointBeamElement> key = JointBeamElement::butterfly_key(
            *near, *far, *face, 0.0, 260.0, 60.0, 60.0, 30.0, 60.0, 60.0, 60.0
        );
        key->name = fmt::format("{}_key_{}", name, i);
        _session.add(key, joints);
        _session.add_interaction(key, near, key->interaction(0));
        _session.add_interaction(key, far, key->interaction(1));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Steps
// ═══════════════════════════════════════════════════════════════════════════
std::shared_ptr<EdgeStraightElement>
TimberFloor::along(const std::vector<std::array<double, 2>>& section, const std::string& part) const {

    const std::array<double, 2> ends = reach();
    std::vector<Point> corners;

    for (const std::array<double, 2>& corner : section)
        corners.push_back(local(ends[0], corner[0], corner[1]));

    const double middle = 0.5 * (section[0][0] + section[section.size() / 2][0]);
    const Line axis = Line::from_points(local(ends[0], middle, 0.0), local(ends[1], middle, 0.0));

    return EdgeStraightElement::from_section(axis, Polyline(corners).closed(), fmt::format("{}_{}", name, part));
}

Point TimberFloor::local(double x, double y, double z) const {
    return base.origin() + base.x_axis() * x + base.y_axis() * y + base.z_axis() * z;
}

std::array<double, 2> TimberFloor::reach() const {

    const double half = 0.5 * guide.beam_width;
    const double start = guide.seat == 0 ? -half : guide.seat == 1 ? half - guide.seat_length : half;

    return {start, guide.span - start};
}

std::pair<size_t, double> TimberFloor::panels() const {

    if (guide.panel_width <= 0.0)
        return {1, guide.width};

    const size_t count = static_cast<size_t>(std::round(guide.width / guide.panel_width));

    return {count, guide.width / static_cast<double>(count)};
}

AssemblyModule TimberFloor::assembly() const {

    std::vector<AssemblyModule> pieces;

    // members: the beams, the ribs and boards, the panels in their order, then what joins them
    for (const std::string part : {"beams", "ribs", "boards", "panels", "joints"})
        for (const std::shared_ptr<Element>& member : _session.get_elements_under<Element>(fmt::format("{}_{}", name, part)))
            if (!std::dynamic_pointer_cast<JointElement>(member))
                pieces.push_back(AssemblyModule::part(member));

    return AssemblyModule::module(name, pieces);
}

// ═══════════════════════════════════════════════════════════════════════════
// TimberFloors
// ═══════════════════════════════════════════════════════════════════════════
TimberFloors::TimberFloors(
    const std::vector<std::pair<std::string, TimberFloorGuide>>& floors, const std::string& name
)
    : WoodSession(name), floors(floors) {

    // floors: side by side across, each assembled on its own
    double across = 0.0;

    for (const std::pair<std::string, TimberFloorGuide>& floor : floors) {
        across += floor.second.width;
        const Plane base(Point(across, 0.0, 0.0), Vector::y_axis(), Vector::x_axis() * -1.0);
        const TimberFloor bay(*this, floor.second, base, floor.first);
        assemblies.push_back(bay.assembly());
        across += GAP;
    }

    // contacts: every two members and boards that touch
    wood_module::add_module_contacts(*this);
}

}
