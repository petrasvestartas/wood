#include "src/templates/timber_floor/earth_floor.h"
#include "src/templates/module/module_contact.h"
#include "wood_profile.h"

namespace wood_timber_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
EarthFloor::EarthFloor(
    WoodSession& session, const EarthFloorGuide& guide, const Plane& base, const std::string& name
)
    : guide(guide), base(base), name(name), _session(session), _group(session.group_named(name)) {

    // bearings: the two beams the element lies on
    add_bearings();

    // beams: two halves side by side, one at either side of the element, a board under them
    add_beams();

    // earth: between every two beams, resting on their flanks
    add_earth();

    // panel: on the beams and the earth, its pins and the rods
    add_panel();
    add_pins();
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════
void EarthFloor::add_bearings() {

    const std::shared_ptr<TreeNode> bearings = _session.group_named(name + "_bearings", _group);
    const double half = 0.5 * guide.bearing_width;
    const std::array<double, 2> places = {half, guide.length - half};

    for (size_t end = 0; end < 2; end++) {
        const double level = -0.5 * guide.bearing_depth;
        const Polyline axis({local(places[end], 0.0, level), local(places[end], guide.width, level)});
        _session.add(
            std::make_shared<EdgeStraightElement>(
                axis,
                profile_rectangle(guide.bearing_width, guide.bearing_depth),
                std::vector<Vector>{base.z_axis()},
                fmt::format("{}_bearing_{}", name, end)
            ),
            bearings
        );
    }
}

void EarthFloor::add_beams() {

    const std::shared_ptr<TreeNode> beams = _session.group_named(name + "_beams", _group);
    const double foot = guide.lining;
    const double head = foot + guide.depth;
    const double spring = head - guide.flank;

    // lining: one board under the whole element
    if (guide.lining > 0.0)
        _session.add(
            FacePlanarElement::from_rectangle(
                local(0.0, 0.0, 0.0), base.x_axis(), base.y_axis(), guide.length, guide.width, foot, name + "_lining"
            ),
            beams
        );

    // halves: on either side of every bay, the flank towards the earth
    for (size_t bay = 0; bay < guide.bays(); bay++)
        for (size_t side = 0; side < 2; side++) {
            const double line = guide.pitch * static_cast<double>(bay + side);
            const double inwards = side == 0 ? 1.0 : -1.0;
            std::vector<std::array<double, 2>> section = {{line, foot}, {line + inwards * guide.half_bottom, foot}};

            if (spring > foot)
                section.push_back({line + inwards * guide.half_bottom, spring});

            section.push_back({line + inwards * guide.half_top, head});
            section.push_back({line, head});
            _session.add(along(section, fmt::format("beam_{}_{}", bay, side)), beams);
        }
}

void EarthFloor::add_earth() {

    const std::shared_ptr<TreeNode> earth = _session.group_named(name + "_earth", _group);
    const double head = guide.lining + guide.depth;
    const double spring = head - guide.flank;

    for (size_t bay = 0; bay < guide.bays(); bay++) {
        const double near = guide.pitch * static_cast<double>(bay);
        const double far = near + guide.pitch;
        std::vector<std::array<double, 2>> section = {{far - guide.half_top, head}, {near + guide.half_top, head}};

        // underside: from beam to beam, flat or a circle's arc of the rise
        const double start = near + guide.half_bottom;
        const double end = far - guide.half_bottom;
        section.push_back({start, spring});

        if (guide.rise > 0.0) {
            const double half = 0.5 * (end - start);
            const double radius = 0.5 * (half * half + guide.rise * guide.rise) / guide.rise;
            const double angle = std::asin(half / radius);

            for (size_t k = 1; k < guide.chords; k++) {
                const double turn = angle * (2.0 * static_cast<double>(k) / static_cast<double>(guide.chords) - 1.0);
                section.push_back({
                    start + half + radius * std::sin(turn),
                    spring + guide.rise - radius * (1.0 - std::cos(turn)),
                });
            }
        }

        section.push_back({end, spring});
        _session.add(along(section, fmt::format("earth_{}", bay)), earth);
    }
}

void EarthFloor::add_panel() {

    _session.add(
        FacePlanarElement::from_rectangle(
            local(0.0, 0.0, guide.lining + guide.depth),
            base.x_axis(),
            base.y_axis(),
            guide.length,
            guide.width,
            guide.panel,
            name + "_panel"
        ),
        _session.group_named(name + "_top", _group)
    );
}

void EarthFloor::add_pins() {

    const std::shared_ptr<TreeNode> group = _session.group_named(name + "_pins", _group);
    const std::shared_ptr<FacePlanarElement> panel = _session.get_element_by_name<FacePlanarElement>(name + "_panel");
    const std::vector<std::shared_ptr<EdgeStraightElement>> halves = _session.get_elements_under<EdgeStraightElement>(name + "_beams");

    // pins: from the panel's top into every half beam, in a row along it
    for (const std::shared_ptr<EdgeStraightElement>& half : halves) {
        if (guide.pin_spacing <= 0.0)
            break;

        const std::shared_ptr<InteractionContactFace> seat = _session.compute_face_contact(panel, half);
        const size_t count = static_cast<size_t>(std::ceil(guide.length / guide.pin_spacing)) + 1;
        const std::shared_ptr<JointBeamElement> pins = JointBeamElement::headed_pins(
            *panel, *half, *seat, PinLayout::horizontal, count, 35.0, 0.0, 3.0, 100.0
        );
        pins->name = fmt::format("{}_pins", half->name);
        _session.add(pins, group);
        _session.add_interaction(pins, panel, pins->interaction(0));
        _session.add_interaction(pins, half, pins->interaction(1));
    }

    // rods: across all beams and the earth, evenly along the element
    std::vector<std::shared_ptr<EdgeStraightElement>> members = halves;
    const std::vector<std::shared_ptr<EdgeStraightElement>> earth = _session.get_elements_under<EdgeStraightElement>(name + "_earth");
    members.insert(members.end(), earth.begin(), earth.end());

    for (size_t k = 0; k < guide.ties; k++) {
        const double x = guide.length * static_cast<double>(k + 1) / static_cast<double>(guide.ties + 1);
        const double z = guide.lining + guide.depth - 0.4 * guide.flank;
        const Line axis = Line::from_points(local(x, 0.0, z), local(x, guide.width, z));
        const std::shared_ptr<JointPinElement> rod = std::make_shared<JointPinElement>(axis, 8.0, 0.2);
        rod->name = fmt::format("{}_rod_{}", name, k);

        for (const std::shared_ptr<EdgeStraightElement>& member : members)
            rod->targets.push_back(member->guid());

        _session.add(rod, group);

        for (const std::shared_ptr<EdgeStraightElement>& member : members)
            _session.add_interaction(rod, member, std::make_shared<InteractionFeatureSolid>(std::vector<Line>{axis}, 8.0));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Steps
// ═══════════════════════════════════════════════════════════════════════════
std::shared_ptr<EdgeStraightElement>
EarthFloor::along(const std::vector<std::array<double, 2>>& section, const std::string& part) const {

    std::vector<Point> corners;

    for (const std::array<double, 2>& corner : section)
        corners.push_back(local(0.0, corner[0], corner[1]));

    const Line axis = Line::from_points(
        local(0.0, section[0][0], section[0][1]), local(guide.length, section[0][0], section[0][1])
    );

    return EdgeStraightElement::from_section(axis, Polyline(corners).closed(), fmt::format("{}_{}", name, part));
}

Point EarthFloor::local(double x, double y, double z) const {
    return base.origin() + base.x_axis() * x + base.y_axis() * y + base.z_axis() * z;
}

AssemblyModule EarthFloor::assembly() const {

    std::vector<AssemblyModule> pieces;

    // members: the beams, the earth, the panel, then the pins and the rods
    for (const std::string part : {"bearings", "beams", "earth", "top"})
        for (const std::shared_ptr<Element>& member : _session.get_elements_under<Element>(fmt::format("{}_{}", name, part)))
            pieces.push_back(AssemblyModule::part(member));

    for (const AssemblyModule& pins : wood_module::pin_parts(_session, _session.get_elements_under<JointBeamElement>(name + "_pins")))
        pieces.push_back(pins);

    return AssemblyModule::module(name, pieces);
}

// ═══════════════════════════════════════════════════════════════════════════
// EarthFloors
// ═══════════════════════════════════════════════════════════════════════════
EarthFloors::EarthFloors(
    const std::vector<std::pair<std::string, EarthFloorGuide>>& floors, const std::string& name
)
    : WoodSession(name), floors(floors) {

    // elements: side by side across, each assembled on its own
    double across = 0.0;

    for (const std::pair<std::string, EarthFloorGuide>& floor : floors) {
        const Plane base(Point(0.0, across, 0.0), Vector::x_axis(), Vector::y_axis());
        const EarthFloor element(*this, floor.second, base, floor.first);
        assemblies.push_back(element.assembly());
        across += floor.second.width + GAP;
    }

    // contacts: every two members that touch
    wood_module::add_module_contacts(*this);
}

}
