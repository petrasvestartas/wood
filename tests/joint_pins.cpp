#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include <numbers>

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Measurements
// ═══════════════════════════════════════════════════════════════════════════

static const double HOLE_REL = 1e-6; // relative, a bored mesh against its polygon's area times its length
static const int PIN_SIDES = 16; // headed_pins' default polygon, the mesh a bore takes
static const double FACE = 0.5; // mm, how far past a pin's end a point lies outside its member

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// The faces of a BRep on a rational surface: its cylinders.
static size_t cylinders(const BRep& brep) {

    size_t count = 0;
    for (const BRepFace& face : brep.m_faces)
        count += brep.m_surfaces[face.surface_index].is_rational();

    return count;
}

/// The length of a pin inside a member's stock.
static double length_inside(const Element& member, const Line& pin) {

    double length = 0.0;
    for (const std::array<double, 2>& stretch : inside_stretches(member.element_geometry_mesh(), pin))
        length += std::max(0.0, std::min(stretch[1], pin.length()) - std::max(stretch[0], 0.0));

    return length;
}

// ═══════════════════════════════════════════════════════════════════════════
// Headed pins: the pre-drilled holes in the members' solids
// ═══════════════════════════════════════════════════════════════════════════

/// Three headed pins in a column through a beam into the end of a joist: each member bored along every pin inside it, an exact cylinder each in the BRep, the mesh losing each bore's polygon over its length.
static void check_headed_pins() {

    WoodSession scene("headed_pins");
    const std::shared_ptr<BeamVariable> beam = BeamVariable::between(
        Polyline::rectangle({0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 120.0, 300.0),
        Polyline::rectangle({600.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 120.0, 300.0),
        "beam"
    );
    const std::shared_ptr<BeamVariable> joist = BeamVariable::between(
        Polyline::rectangle({250.0, 120.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 100.0, 300.0),
        Polyline::rectangle({250.0, 720.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 100.0, 300.0),
        "joist"
    );
    scene.add(beam);
    scene.add(joist);
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(beam, joist);
    const std::shared_ptr<JointBeam> pins = JointBeam::headed_pins(*beam, *joist, *contact, PinLayout::vertical, 3, 20.0, 0.0, 5.0);
    check(pins != nullptr, "headed pins on the beam and the joist");
    scene.add(pins);
    scene.add_interaction(pins, beam, pins->interaction(0));
    scene.add_interaction(pins, joist, pins->interaction(1));

    for (const std::shared_ptr<BeamVariable>& member : {beam, joist}) {
        double length = 0.0;
        size_t inside = 0;
        for (const Line& pin : pins->drill_lines) {
            const double stretch = length_inside(*member, pin);
            length += stretch;
            inside += stretch > 1e-6;
        }
        const double removed = compute_volume(member->element_geometry_mesh()) - compute_volume(member->model_geometry_mesh());
        // the mesh bores a polygon of PIN_SIDES inscribed in the pin's circle
        const double polygon = 0.5 * PIN_SIDES * pins->line_radius * pins->line_radius * std::sin(2.0 * std::numbers::pi / PIN_SIDES);
        const double expected = polygon * length;
        check(inside == 3, fmt::format("{} has {} of the 3 pins inside it", member->name, inside));
        check(std::abs(removed - expected) <= HOLE_REL * expected, fmt::format("{} lost {:.1f} mm3 to its pre-drilled holes, their 16-sided bores give {:.1f}", member->name, removed, expected));
        check(cylinders(member->model_geometry_brep()) == inside, fmt::format("{} BRep has {} exact bores for {} pins", member->name, cylinders(member->model_geometry_brep()), inside));
    }

    // the beam and the joist come apart across their contact, the joist away from the beam
    const Vector beam_way = pins->insertion(0);
    const Vector joist_way = pins->insertion(1);
    check(std::abs(beam_way.dot(joist_way) + 1.0) <= 1e-9 && joist_way.dot(Vector(0.0, 1.0, 0.0)) > 1.0 - 1e-9, fmt::format("the joist comes off along +y, the beam along -y: ({:.3f} {:.3f} {:.3f})", joist_way[0], joist_way[1], joist_way[2]));

    std::cout << "joint_pins: headed pins bore 3 exact pre-drilled holes into the beam and the joist" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Rectangle plate: the pins flush with their members
// ═══════════════════════════════════════════════════════════════════════════

/// The let-in plate of a 220 column and a 120 rib: every pin runs across its member, both ends on the member's faces, 220 in the column and 120 in the rib.
static void check_rectangle_plate() {

    WoodSession scene("rectangle_plate");
    const std::shared_ptr<Column> column = std::make_shared<Column>(
        Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 1200.0}),
        Polyline::rectangle({-110.0, -110.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 220.0, 220.0),
        "column"
    );
    const std::shared_ptr<BeamVariable> rib = BeamVariable::between(
        Polyline::rectangle({110.0, -60.0, 700.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 120.0, 400.0),
        Polyline::rectangle({1400.0, -60.0, 700.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 120.0, 400.0),
        "rib"
    );
    scene.add(column);
    scene.add(rib);
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(column, rib);
    const std::shared_ptr<Plate> plate = JointBeam::let_in_plate(*rib, *contact);
    const std::shared_ptr<JointBeam> pins = JointBeam::rectangle_plate(*column, *rib, *plate, *contact);
    check(pins->drill_lines.size() == 4, fmt::format("{} pins, not 4", pins->drill_lines.size()));

    // the first two pins stand in the column, the last two in the rib
    const std::array<double, 2> widths = {220.0, 120.0};
    for (size_t i = 0; i < 4; i++) {
        const Line& pin = pins->drill_lines[i];
        const Element& member = i < 2 ? static_cast<const Element&>(*column) : static_cast<const Element&>(*rib);
        const Vector d = pin.to_vector().normalized();
        const std::vector<PlanarFace> faces = planar_faces(member.element_geometry_mesh());
        check(std::abs(pin.length() - widths[i / 2]) <= 1e-6, fmt::format("pin {} is {:.6f} long across a {:g} member", i, pin.length(), widths[i / 2]));
        check(length_inside(member, pin) >= pin.length() - 1e-6, fmt::format("pin {} leaves its member", i));
        check(inside_stretches(member.element_geometry_mesh(), Line::from_points(pin.end() + d * FACE, pin.end() + d * (2.0 * FACE))).empty()
                  || length_inside(member, Line::from_points(pin.end() + d * FACE, pin.end() + d * (2.0 * FACE))) <= 1e-9,
              fmt::format("pin {} ends inside its member, not on its face", i));
        check(length_inside(member, Line::from_points(pin.start() - d * (2.0 * FACE), pin.start() - d * FACE)) <= 1e-9, fmt::format("pin {} starts inside its member, not on its face", i));
    }

    std::cout << "joint_pins: the let-in plate's pins run across their members face to face, 220 in the column and 120 in the rib" << std::endl;
}

int main() {

    check_headed_pins();
    check_rectangle_plate();
    return 0;
}
