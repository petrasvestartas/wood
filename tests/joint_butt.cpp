#include "wood_session.h"
#include "wood_element_geometry.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Measurements
// ═══════════════════════════════════════════════════════════════════════════

static const double FLUSH = 1e-3; // mm, how far a butted face may lie from the face it butts on
static const double OVERLAP_REL = 1e-6; // relative, the volume two butted members may share against the smaller one

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// The extreme of a member's cut solid along an axis: its least or its greatest coordinate.
static double extreme(const Element& member, size_t axis, bool greatest) {

    double value = greatest ? -std::numeric_limits<double>::max() : std::numeric_limits<double>::max();
    for (const Point& point : member.model_geometry_mesh().to_vertices_and_faces().first)
        value = greatest ? std::max(value, point[axis]) : std::min(value, point[axis]);

    return value;
}

/// The volume two members' cut solids share, against the smaller of them.
static double shared(const Element& a, const Element& b) {

    const Mesh common = solid_boolean(a.model_geometry_mesh(), b.model_geometry_mesh(), SolidOperation::intersect);
    const double smaller = std::min(compute_volume(a.model_geometry_mesh()), compute_volume(b.model_geometry_mesh()));

    return common.number_of_faces() == 0 ? 0.0 : compute_volume(common) / smaller;
}

/// The butt joined and passed to both members, running first.
static void join(WoodSession& scene, const std::shared_ptr<Element>& running, const std::shared_ptr<Element>& butting) {

    const std::shared_ptr<JointBeam> butt = JointBeam::butt(*running, *butting);
    butt->name = fmt::format("butt_{}_{}", running->name, butting->name);
    scene.add(butt);
    scene.add_interaction(butt, running, butt->interaction(0));
    scene.add_interaction(butt, butting, butt->interaction(1));
}

// ═══════════════════════════════════════════════════════════════════════════
// Beams
// ═══════════════════════════════════════════════════════════════════════════

/// A post on the middle of a sill and one on its end, 150 x 150 beams on axes meeting at nodes.
static void beams() {

    WoodSession scene("butt_beams");
    const std::shared_ptr<Beam> sill = std::make_shared<Beam>(Polyline({{-600.0, 0.0, 0.0}, {600.0, 0.0, 0.0}}), 75.0, "sill");
    const std::shared_ptr<Beam> middle = std::make_shared<Beam>(Polyline({{0.0, 0.0, 0.0}, {0.0, 0.0, 800.0}}), 75.0, "middle");
    const std::shared_ptr<Beam> end = std::make_shared<Beam>(Polyline({{600.0, 0.0, 0.0}, {600.0, 0.0, 800.0}}), 75.0, "end");
    scene.add(sill);
    scene.add(middle);
    scene.add(end);
    join(scene, sill, middle);
    join(scene, sill, end);

    check(std::abs(extreme(*middle, 2, false) - 75.0) < FLUSH, fmt::format("beams: the middle post starts {} mm off the sill's top", extreme(*middle, 2, false) - 75.0));
    check(std::abs(extreme(*end, 2, false) - 75.0) < FLUSH, fmt::format("beams: the end post starts {} mm off the sill's top", extreme(*end, 2, false) - 75.0));
    check(std::abs(extreme(*sill, 0, true) - 675.0) < FLUSH, fmt::format("beams: the sill ends {} mm off the end post's outer face", extreme(*sill, 0, true) - 675.0));
    check(std::abs(extreme(*sill, 0, false) + 600.0) < FLUSH, fmt::format("beams: the sill's free end moved {} mm", extreme(*sill, 0, false) + 600.0));
    check(shared(*sill, *middle) < OVERLAP_REL && shared(*sill, *end) < OVERLAP_REL, "beams: a post still overlaps the sill");
}

// ═══════════════════════════════════════════════════════════════════════════
// Plates
// ═══════════════════════════════════════════════════════════════════════════

/// A floor, four walls on the 6000 x 4000 footprint and a roof, drawn on their mid-planes, as examples/21_butt_house.cpp.
static void plates() {

    WoodSession scene("butt_plates");
    const std::array<Point, 4> footprint = {Point(0.0, 0.0, 0.0), Point(6000.0, 0.0, 0.0), Point(6000.0, 4000.0, 0.0), Point(0.0, 4000.0, 0.0)};
    const std::shared_ptr<Plate> floor = Plate::from_rectangle({0.0, 0.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6000.0, 4000.0, 200.0, "floor");
    const std::shared_ptr<Plate> roof = Plate::from_rectangle({0.0, 0.0, 2700.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6000.0, 4000.0, 200.0, "roof");
    std::array<std::shared_ptr<Plate>, 4> walls;
    for (size_t i = 0; i < 4; i++)
        walls[i] = Plate::from_line(Line::from_points(footprint[i], footprint[(i + 1) % 4]), {0.0, 0.0, 1.0}, 2800.0, 120.0, fmt::format("wall_{}", i));
    scene.add(floor);
    scene.add(roof);
    for (const std::shared_ptr<Plate>& wall : walls)
        scene.add(wall);
    for (const std::shared_ptr<Plate>& wall : walls)
        join(scene, floor, wall);
    for (const std::shared_ptr<Plate>& wall : walls)
        join(scene, roof, wall);
    for (const std::array<size_t, 2>& corner : std::array<std::array<size_t, 2>, 4>{{{0, 1}, {0, 3}, {2, 1}, {2, 3}}})
        join(scene, walls[corner[0]], walls[corner[1]]);

    // the floor and the roof out to the walls' outer faces, the walls between the floor's top and the roof's bottom
    for (const std::shared_ptr<Plate>& slab : {floor, roof}) {
        check(std::abs(extreme(*slab, 0, false) + 60.0) < FLUSH && std::abs(extreme(*slab, 0, true) - 6060.0) < FLUSH, fmt::format("plates: the {} spans x {} to {}, not -60 to 6060", slab->name, extreme(*slab, 0, false), extreme(*slab, 0, true)));
        check(std::abs(extreme(*slab, 1, false) + 60.0) < FLUSH && std::abs(extreme(*slab, 1, true) - 4060.0) < FLUSH, fmt::format("plates: the {} spans y {} to {}, not -60 to 4060", slab->name, extreme(*slab, 1, false), extreme(*slab, 1, true)));
    }
    for (const std::shared_ptr<Plate>& wall : walls)
        check(std::abs(extreme(*wall, 2, false) - 100.0) < FLUSH && std::abs(extreme(*wall, 2, true) - 2700.0) < FLUSH, fmt::format("plates: {} spans z {} to {}, not 100 to 2700", wall->name, extreme(*wall, 2, false), extreme(*wall, 2, true)));

    // the long walls out to the short walls' outer faces, the short ones between the long ones' inner faces
    check(std::abs(extreme(*walls[0], 0, false) + 60.0) < FLUSH && std::abs(extreme(*walls[0], 0, true) - 6060.0) < FLUSH, "plates: wall_0 does not run on to the short walls' outer faces");
    check(std::abs(extreme(*walls[1], 1, false) - 60.0) < FLUSH && std::abs(extreme(*walls[1], 1, true) - 3940.0) < FLUSH, fmt::format("plates: wall_1 spans y {} to {}, not 60 to 3940", extreme(*walls[1], 1, false), extreme(*walls[1], 1, true)));

    // and no two plates share any volume
    const std::array<std::shared_ptr<Plate>, 6> all = {floor, roof, walls[0], walls[1], walls[2], walls[3]};
    for (size_t i = 0; i < all.size(); i++)
        for (size_t j = i + 1; j < all.size(); j++)
            check(shared(*all[i], *all[j]) < OVERLAP_REL, fmt::format("plates: {} and {} overlap by {} of the smaller", all[i]->name, all[j]->name, shared(*all[i], *all[j])));
}

int main() {

    beams();
    plates();
    std::cout << "joint_butt: beams and plates butt flush" << std::endl;
    return 0;
}
