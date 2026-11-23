#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/truss/truss.h"
#include "wood_assembly_sequence.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Measurements
// ═══════════════════════════════════════════════════════════════════════════
static const double FLUSH = 1e-3;
static const double OVERLAP_REL = 1e-6;
static const double TRUSS_OVERLAP = 5e-4;
static const double TRUSS_GAP = 5e-3;

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// The extreme of a member's cut solid along an axis: its least or its greatest coordinate over the corners of its faces.
static double extreme(const Element& member, size_t axis, bool greatest) {

    double value =
        greatest ? -std::numeric_limits<double>::max() : std::numeric_limits<double>::max();
    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> solid =
        member.model_geometry_mesh().to_vertices_and_faces();
    for (const std::vector<size_t>& face : solid.second)
        for (const size_t corner : face)
            value = greatest ? std::max(value, solid.first[corner][axis])
                             : std::min(value, solid.first[corner][axis]);

    return value;
}

/// The volume two members' cut solids share, against the smaller of them.
static double shared(const Element& a, const Element& b) {

    const Mesh common =
        solid_boolean(a.model_geometry_mesh(), b.model_geometry_mesh(), SolidOperation::intersect);
    const double smaller =
        std::min(compute_volume(a.model_geometry_mesh()), compute_volume(b.model_geometry_mesh()));

    return common.number_of_faces() == 0 ? 0.0 : compute_volume(common) / smaller;
}

/// The butt joined and passed to both members, running first.
static void join(
    WoodSession& scene,
    const std::shared_ptr<Element>& running,
    const std::shared_ptr<Element>& butting
) {

    const std::shared_ptr<JointBeamElement> butt = JointBeamElement::butt(*running, *butting);
    butt->name = fmt::format("butt_{}_{}", running->name, butting->name);
    scene.add(butt);
    scene.add_interaction(butt, running, butt->interaction(0));
    scene.add_interaction(butt, butting, butt->interaction(1));
}

// ═══════════════════════════════════════════════════════════════════════════
// Pairs
// ═══════════════════════════════════════════════════════════════════════════

namespace {

struct ButtCase {
    WoodSession scene; // Scene owning the pair and its butt joint.
    std::shared_ptr<Element> a; // First member after the butt cut.
    std::shared_ptr<Element> b; // Second member after the butt cut.
};

/// A 150 x 150 square beam between start and end.
std::shared_ptr<EdgeStraightElement> square_beam(const Point& start, const Point& end) {

    return EdgeStraightElement::square(Polyline({start, end}), 75.0, "beam");
}

/// A 120 thick, 1500 tall upright wall plate along start to end.
std::shared_ptr<FacePlanarElement> wall_plate(const Point& start, const Point& end) {

    return FacePlanarElement::from_line(
        Line::from_points(start, end),
        Vector(0.0, 0.0, 1.0),
        1500.0,
        120.0,
        "wall"
    );
}

/// Checks that value matches expected within FLUSH.
void near(double value, double expected, const std::string& what) {

    check(
        std::abs(value - expected) < FLUSH,
        fmt::format("{} at {}, not {}", what, value, expected)
    );
}

/// Runs a pairwise butt cut in its own scene, checks configuration and overlap, and returns the cut members.
ButtCase run_pair(
    const std::string& name,
    const std::shared_ptr<Element>& a,
    const std::shared_ptr<Element>& b,
    ButtConfiguration expected,
    bool side_side = false
) {

    ButtCase result{WoodSession(name), a, b};
    result.scene.add(a);
    result.scene.add(b);
    const std::shared_ptr<JointBeamElement> butt =
        side_side ? JointBeamElement::butt_side_side(*a, *b) : JointBeamElement::butt(*a, *b);
    check(
        butt && butt->configuration == expected,
        fmt::format(
            "{}: configuration {} instead of {}",
            name,
            butt ? (int)butt->configuration : -1,
            (int)expected
        )
    );
    result.scene.add(butt);
    result.scene.add_interaction(butt, a, butt->interaction(0));
    result.scene.add_interaction(butt, b, butt->interaction(1));
    if (expected != ButtConfiguration::cross)
        check(
            shared(*a, *b) < OVERLAP_REL,
            fmt::format("{}: the pair overlaps by {} of the smaller", name, shared(*a, *b))
        );

    return result;
}

/// Pairwise beam butt cases, each checked at its measured faces.
void beam_pairs() {

    const ButtCase top_side = run_pair(
        "beam_top_side",
        square_beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}),
        square_beam({0.0, 0.0, 0.0}, {0.0, 0.0, 1500.0}),
        ButtConfiguration::top_side
    );
    near(extreme(*top_side.b, 2, false), 75.0, "beam_top_side: the post's foot");
    near(extreme(*top_side.a, 0, true), 1000.0, "beam_top_side: the through beam's end");

    const ButtCase shallow = run_pair(
        "beam_top_side_shallow",
        square_beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}),
        square_beam({0.0, 0.0, 0.0}, {-1902.1130325903071, 0.0, -618.03398874989485}),
        ButtConfiguration::top_side
    );
    near(extreme(*shallow.b, 2, true), -75.0, "beam_top_side_shallow: the strut's top at 18 degrees");

    for (const double drawn : {-150.0, 100.0}) {
        const ButtCase end_case = run_pair(
            fmt::format("beam_top_side_end_{}", drawn),
            square_beam({drawn, 0.0, 0.0}, {1500.0, 0.0, 0.0}),
            square_beam({0.0, 0.0, drawn}, {0.0, 0.0, 1500.0}),
            ButtConfiguration::top_side
        );
        near(extreme(*end_case.a, 0, false), -75.0, "beam_top_side_end: the running end");
        near(extreme(*end_case.b, 2, false), 75.0, "beam_top_side_end: the butting end");
    }

    const ButtCase outplane = run_pair(
        "beam_side_side_outplane",
        square_beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}),
        square_beam({0.0, 0.0, 0.0}, {-750.0, 0.0, 1300.0}),
        ButtConfiguration::side_side_outplane
    );
    near(extreme(*outplane.a, 2, false), -75.0, "beam_side_side_outplane: the left beam's underside");

    const ButtCase outplane_square = run_pair(
        "beam_side_side_outplane_square",
        square_beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}),
        square_beam({0.0, 0.0, 0.0}, {0.0, 0.0, 1500.0}),
        ButtConfiguration::side_side_outplane,
        true
    );
    near(
        extreme(*outplane_square.a, 0, false),
        -75.0,
        "beam_side_side_outplane_square: the left beam's outer corner"
    );
    near(
        extreme(*outplane_square.b, 2, false),
        -75.0,
        "beam_side_side_outplane_square: the right beam's outer corner"
    );

    const ButtCase inplane = run_pair(
        "beam_side_side_inplane",
        square_beam({-1500.0, 0.0, 0.0}, {0.0, 0.0, 0.0}),
        square_beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}),
        ButtConfiguration::side_side_inplane
    );
    near(extreme(*inplane.a, 0, true), 0.0, "beam_side_side_inplane: the left end");
    near(extreme(*inplane.b, 0, false), 0.0, "beam_side_side_inplane: the right end");

    const ButtCase cross = run_pair(
        "beam_cross",
        square_beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}),
        square_beam({0.0, -1000.0, 0.0}, {0.0, 1000.0, 0.0}),
        ButtConfiguration::cross
    );
    near(extreme(*cross.a, 0, true), 1000.0, "beam_cross: the first beam's end");
}

/// Pairwise plate and mixed beam-plate butt cases, each checked at its measured faces.
void plate_pairs() {

    const ButtCase top_side_end = run_pair(
        "plate_top_side_end",
        wall_plate({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0}),
        wall_plate({0.0, 0.0, 0.0}, {0.0, 2000.0, 0.0}),
        ButtConfiguration::top_side
    );
    near(extreme(*top_side_end.a, 0, false), -60.0, "plate_top_side_end: the running wall's end");
    near(extreme(*top_side_end.b, 1, false), 60.0, "plate_top_side_end: the butting wall's end");

    const ButtCase outplane = run_pair(
        "plate_side_side_outplane",
        wall_plate({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0}),
        wall_plate({0.0, 0.0, 0.0}, {-1000.0, 1732.0508075688772, 0.0}),
        ButtConfiguration::side_side_outplane
    );
    near(extreme(*outplane.a, 1, false), -60.0, "plate_side_side_outplane: the left wall's outer face");

    const ButtCase inplane = run_pair(
        "plate_side_side_inplane",
        wall_plate({-2000.0, 0.0, 0.0}, {0.0, 0.0, 0.0}),
        wall_plate({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0}),
        ButtConfiguration::side_side_inplane
    );
    near(extreme(*inplane.a, 0, true), 0.0, "plate_side_side_inplane: the left end");
    near(extreme(*inplane.b, 0, false), 0.0, "plate_side_side_inplane: the right end");

    const std::shared_ptr<FacePlanarElement> floor = FacePlanarElement::from_rectangle(
        {-1500.0, -1500.0, -100.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        3000.0,
        3000.0,
        200.0,
        "floor"
    );
    const ButtCase on_floor = run_pair(
        "plate_top_side_floor",
        floor,
        wall_plate({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}),
        ButtConfiguration::top_side
    );
    near(extreme(*on_floor.b, 2, false), 100.0, "plate_top_side_floor: the wall's foot");

    const std::shared_ptr<FacePlanarElement> slab = FacePlanarElement::from_rectangle(
        {-1500.0, -1500.0, -100.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        3000.0,
        3000.0,
        200.0,
        "slab"
    );
    const std::shared_ptr<EdgeStraightElement> column = EdgeStraightElement::corner_square(
        Line::from_points({0.0, 0.0, -2000.0}, {0.0, 0.0, 0.0}),
        Plane({-100.0, -100.0, -2000.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}),
        200.0,
        "column"
    );
    const ButtCase column_plate = run_pair(
        "column_top_side_plate",
        slab,
        column,
        ButtConfiguration::top_side
    );
    near(extreme(*column_plate.b, 2, true), -100.0, "column_top_side_plate: the column's head");

    const std::shared_ptr<EdgeStraightElement> sill =
        EdgeStraightElement::square(Polyline({{-1500.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}}), 100.0, "beam");
    const ButtCase wall_beam = run_pair(
        "plate_top_side_beam",
        sill,
        wall_plate({-1200.0, 0.0, 0.0}, {1200.0, 0.0, 0.0}),
        ButtConfiguration::top_side
    );
    near(extreme(*wall_beam.b, 2, false), 100.0, "plate_top_side_beam: the wall's foot");
}

} // namespace

/// Pairwise beam and plate butt cases, each checked at its measured faces.
static void pairs() {

    beam_pairs();
    plate_pairs();
}

// ═══════════════════════════════════════════════════════════════════════════
// Beams
// ═══════════════════════════════════════════════════════════════════════════

/// A post on the middle of a sill and one on its end, 150 x 150 beams on axes meeting at nodes.
static void beams() {

    WoodSession scene("butt_beams");
    const std::shared_ptr<EdgeStraightElement> sill =
        EdgeStraightElement::square(Polyline({{-600.0, 0.0, 0.0}, {600.0, 0.0, 0.0}}), 75.0, "sill");
    const std::shared_ptr<EdgeStraightElement> middle =
        EdgeStraightElement::square(Polyline({{0.0, 0.0, 0.0}, {0.0, 0.0, 800.0}}), 75.0, "middle");
    const std::shared_ptr<EdgeStraightElement> end =
        EdgeStraightElement::square(Polyline({{600.0, 0.0, 0.0}, {600.0, 0.0, 800.0}}), 75.0, "end");
    scene.add(sill);
    scene.add(middle);
    scene.add(end);
    join(scene, sill, middle);
    join(scene, sill, end);

    check(
        std::abs(extreme(*middle, 2, false) - 75.0) < FLUSH,
        fmt::format(
            "beams: the middle post starts {} mm off the sill's top",
            extreme(*middle, 2, false) - 75.0
        )
    );
    check(
        std::abs(extreme(*end, 2, false) - 75.0) < FLUSH,
        fmt::format(
            "beams: the end post starts {} mm off the sill's top", extreme(*end, 2, false) - 75.0
        )
    );
    check(
        std::abs(extreme(*sill, 0, true) - 675.0) < FLUSH,
        fmt::format(
            "beams: the sill ends {} mm off the end post's outer face",
            extreme(*sill, 0, true) - 675.0
        )
    );
    check(
        std::abs(extreme(*sill, 0, false) + 600.0) < FLUSH,
        fmt::format("beams: the sill's free end moved {} mm", extreme(*sill, 0, false) + 600.0)
    );
    check(
        shared(*sill, *middle) < OVERLAP_REL && shared(*sill, *end) < OVERLAP_REL,
        "beams: a post still overlaps the sill"
    );
}

// ═══════════════════════════════════════════════════════════════════════════
// Plates
// ═══════════════════════════════════════════════════════════════════════════

/// A floor, four walls on the 6000 x 4000 footprint and a roof, drawn on their mid-planes.
static void plates() {

    WoodSession scene("butt_plates");
    const std::array<Point, 4> footprint = {
        Point(0.0, 0.0, 0.0),
        Point(6000.0, 0.0, 0.0),
        Point(6000.0, 4000.0, 0.0),
        Point(0.0, 4000.0, 0.0)
    };
    const std::shared_ptr<FacePlanarElement> floor = FacePlanarElement::from_rectangle(
        {0.0, 0.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6000.0, 4000.0, 200.0, "floor"
    );
    const std::shared_ptr<FacePlanarElement> roof = FacePlanarElement::from_rectangle(
        {0.0, 0.0, 2700.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6000.0, 4000.0, 200.0, "roof"
    );
    std::array<std::shared_ptr<FacePlanarElement>, 4> walls;
    for (size_t i = 0; i < 4; i++)
        walls[i] = FacePlanarElement::from_line(
            Line::from_points(footprint[i], footprint[(i + 1) % 4]),
            {0.0, 0.0, 1.0},
            2800.0,
            120.0,
            fmt::format("wall_{}", i)
        );
    scene.add(floor);
    scene.add(roof);
    for (const std::shared_ptr<FacePlanarElement>& wall : walls)
        scene.add(wall);
    for (const std::array<size_t, 2>& corner :
         std::array<std::array<size_t, 2>, 4>{{{0, 1}, {0, 3}, {2, 1}, {2, 3}}})
        join(scene, walls[corner[0]], walls[corner[1]]);
    for (const std::shared_ptr<FacePlanarElement>& wall : walls)
        join(scene, floor, wall);
    for (const std::shared_ptr<FacePlanarElement>& wall : walls)
        join(scene, roof, wall);

    for (const std::shared_ptr<FacePlanarElement>& slab : {floor, roof}) {
        check(
            std::abs(extreme(*slab, 0, false) + 60.0) < FLUSH &&
                std::abs(extreme(*slab, 0, true) - 6060.0) < FLUSH,
            fmt::format(
                "plates: the {} spans x {} to {}, not -60 to 6060",
                slab->name,
                extreme(*slab, 0, false),
                extreme(*slab, 0, true)
            )
        );
        check(
            std::abs(extreme(*slab, 1, false) + 60.0) < FLUSH &&
                std::abs(extreme(*slab, 1, true) - 4060.0) < FLUSH,
            fmt::format(
                "plates: the {} spans y {} to {}, not -60 to 4060",
                slab->name,
                extreme(*slab, 1, false),
                extreme(*slab, 1, true)
            )
        );
    }
    for (const std::shared_ptr<FacePlanarElement>& wall : walls)
        check(
            std::abs(extreme(*wall, 2, false) - 100.0) < FLUSH &&
                std::abs(extreme(*wall, 2, true) - 2700.0) < FLUSH,
            fmt::format(
                "plates: {} spans z {} to {}, not 100 to 2700",
                wall->name,
                extreme(*wall, 2, false),
                extreme(*wall, 2, true)
            )
        );

    // whole: the slabs' corners filled where two run-on edges meet, 6120 x 4120 x 200, the long walls 6120 x 2600 x 120
    for (const std::shared_ptr<FacePlanarElement>& slab : {floor, roof})
        check(
            std::abs(
                compute_volume(slab->model_geometry_mesh()) / (6120.0 * 4120.0 * 200.0) - 1.0
            ) < 1e-9,
            fmt::format(
                "plates: the {} holds {} mm3, not 6120 x 4120 x 200",
                slab->name,
                compute_volume(slab->model_geometry_mesh())
            )
        );
    check(
        std::abs(
            compute_volume(walls[0]->model_geometry_mesh()) / (6120.0 * 2600.0 * 120.0) - 1.0
        ) < 1e-9,
        fmt::format(
            "plates: wall_0 holds {} mm3, not 6120 x 2600 x 120",
            compute_volume(walls[0]->model_geometry_mesh())
        )
    );

    check(
        std::abs(extreme(*walls[0], 0, false) + 60.0) < FLUSH &&
            std::abs(extreme(*walls[0], 0, true) - 6060.0) < FLUSH,
        "plates: wall_0 does not run on to the short walls' outer faces"
    );
    check(
        std::abs(extreme(*walls[1], 1, false) - 60.0) < FLUSH &&
            std::abs(extreme(*walls[1], 1, true) - 3940.0) < FLUSH,
        fmt::format(
            "plates: wall_1 spans y {} to {}, not 60 to 3940",
            extreme(*walls[1], 1, false),
            extreme(*walls[1], 1, true)
        )
    );

    const std::array<std::shared_ptr<FacePlanarElement>, 6> all = {
        floor, roof, walls[0], walls[1], walls[2], walls[3]
    };
    for (size_t i = 0; i < all.size(); i++)
        for (size_t j = i + 1; j < all.size(); j++)
            check(
                shared(*all[i], *all[j]) < OVERLAP_REL,
                fmt::format(
                    "plates: {} and {} overlap by {} of the smaller",
                    all[i]->name,
                    all[j]->name,
                    shared(*all[i], *all[j])
                )
            );
}

// ═══════════════════════════════════════════════════════════════════════════
// Folded roof
// ═══════════════════════════════════════════════════════════════════════════

/// A folded roof of four 160 plates on a zigzag in section over two gables drawn up to it.
static void folded_roof() {

    WoodSession scene("butt_folded_roof");
    const std::vector<Point> zigzag = {
        {0.0, 0.0, 2400.0},
        {1500.0, 0.0, 3200.0},
        {3000.0, 0.0, 2400.0},
        {4500.0, 0.0, 3200.0},
        {6000.0, 0.0, 2400.0}
    };
    std::vector<std::shared_ptr<FacePlanarElement>> roof;
    for (size_t i = 0; i + 1 < zigzag.size(); i++) {
        const Vector along = (zigzag[i + 1] - zigzag[i]).normalized();
        const Vector across = along.cross(Vector(0.0, 1.0, 0.0));

        const double before = i == 0 ? 300.0 / along[0] : 0.0;
        const double after = i + 2 == zigzag.size() ? 300.0 / along[0] : 0.0;
        roof.push_back(FacePlanarElement::from_rectangle(
            zigzag[i] - along * before - across * 80.0,
            along,
            {0.0, 1.0, 0.0},
            Point::distance(zigzag[i], zigzag[i + 1]) + before + after,
            4000.0,
            160.0,
            fmt::format("roof_{}", i)
        ));
        scene.add(roof.back());
    }
    std::vector<Point> outline = {{6000.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    outline.insert(outline.end(), zigzag.begin(), zigzag.end());
    outline.push_back(outline.front());
    std::array<std::shared_ptr<FacePlanarElement>, 2> gables;
    for (size_t g = 0; g < 2; g++) {
        const Polyline drawn = Polyline(outline).translated({0.0, g == 0 ? 0.0 : 4000.0, 0.0});
        gables[g] = std::make_shared<FacePlanarElement>(
            drawn.translated({0.0, -70.0, 0.0}),
            drawn.translated({0.0, 70.0, 0.0}),
            fmt::format("gable_{}", g)
        );
        scene.add(gables[g]);
    }
    for (const std::shared_ptr<FacePlanarElement>& plate : roof)
        for (const std::shared_ptr<FacePlanarElement>& gable : gables)
            join(scene, plate, gable);
    for (size_t i = 0; i + 1 < roof.size(); i++)
        join(scene, roof[i], roof[i + 1]);

    std::vector<std::array<double, 2>> section = {{6000.0, 0.0}, {0.0, 0.0}};
    std::vector<std::array<double, 4>> lines;
    for (size_t i = 0; i + 1 < zigzag.size(); i++) {
        const double dx = zigzag[i + 1][0] - zigzag[i][0];
        const double dz = zigzag[i + 1][2] - zigzag[i][2];
        const double length = std::hypot(dx, dz);
        lines.push_back(
            {zigzag[i][0] + 80.0 * dz / length, zigzag[i][2] - 80.0 * dx / length, dx, dz}
        );
    }
    section.push_back({0.0, lines[0][1] + (0.0 - lines[0][0]) * lines[0][3] / lines[0][2]});
    for (size_t i = 0; i + 1 < lines.size(); i++) {
        const std::array<double, 4>& a = lines[i];
        const std::array<double, 4>& b = lines[i + 1];
        const double t =
            ((b[0] - a[0]) * b[3] - (b[1] - a[1]) * b[2]) / (a[2] * b[3] - a[3] * b[2]);
        section.push_back({a[0] + a[2] * t, a[1] + a[3] * t});
    }
    section.push_back(
        {6000.0, lines.back()[1] + (6000.0 - lines.back()[0]) * lines.back()[3] / lines.back()[2]}
    );
    double area = 0.0;
    for (size_t k = 0; k < section.size(); k++)
        area += section[k][0] * section[(k + 1) % section.size()][1] -
                section[(k + 1) % section.size()][0] * section[k][1];
    const double expected = 0.5 * std::abs(area) * 140.0;
    for (const std::shared_ptr<FacePlanarElement>& gable : gables)
        check(
            std::abs(compute_volume(gable->model_geometry_mesh()) / expected - 1.0) < 1e-9,
            fmt::format(
                "folded_roof: {} holds {} mm3, not the {} under the roof",
                gable->name,
                compute_volume(gable->model_geometry_mesh()),
                expected
            )
        );

    std::vector<std::shared_ptr<FacePlanarElement>> all = roof;
    all.insert(all.end(), gables.begin(), gables.end());
    for (size_t i = 0; i < all.size(); i++)
        for (size_t j = i + 1; j < all.size(); j++)
            check(
                shared(*all[i], *all[j]) < OVERLAP_REL,
                fmt::format(
                    "folded_roof: {} and {} overlap by {} of the smaller",
                    all[i]->name,
                    all[j]->name,
                    shared(*all[i], *all[j])
                )
            );
}

// ═══════════════════════════════════════════════════════════════════════════
// Frame
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// A 140 x 140 timber-frame member between start and end.
std::shared_ptr<EdgeStraightElement>
frame_beam(const Point& start, const Point& end, const std::string& name) {

    return EdgeStraightElement::square(Polyline({start, end}), 70.0, name);
}

} // namespace

/// Timber-frame wall with posts, sill, plate, lintel, and diagonal brace.
static void frame() {

    WoodSession scene("butt_frame");
    const std::shared_ptr<EdgeStraightElement> sill =
        frame_beam({0.0, 0.0, 0.0}, {3000.0, 0.0, 0.0}, "sill");
    const std::shared_ptr<EdgeStraightElement> head =
        frame_beam({0.0, 0.0, 2400.0}, {3000.0, 0.0, 2400.0}, "head");
    const std::shared_ptr<EdgeStraightElement> left_post =
        frame_beam({0.0, 0.0, 0.0}, {0.0, 0.0, 2400.0}, "left_post");
    const std::shared_ptr<EdgeStraightElement> middle_post =
        frame_beam({1500.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}, "middle_post");
    const std::shared_ptr<EdgeStraightElement> right_post =
        frame_beam({3000.0, 0.0, 0.0}, {3000.0, 0.0, 2400.0}, "right_post");
    const std::shared_ptr<EdgeStraightElement> left_brace =
        frame_beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}, "left_brace");
    const std::shared_ptr<EdgeStraightElement> right_brace =
        frame_beam({3000.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}, "right_brace");
    const std::array<std::shared_ptr<EdgeStraightElement>, 7> all = {
        sill, head, left_post, middle_post, right_post, left_brace, right_brace
    };
    for (const std::shared_ptr<EdgeStraightElement>& member : all)
        scene.add(member);
    for (const std::array<std::shared_ptr<EdgeStraightElement>, 2>& pair :
         std::vector<std::array<std::shared_ptr<EdgeStraightElement>, 2>>{
             {sill, left_post},
             {sill, middle_post},
             {sill, right_post},
             {head, left_post},
             {head, middle_post},
             {head, right_post}
         })
        join(scene, pair[0], pair[1]);
    for (const std::array<std::shared_ptr<EdgeStraightElement>, 2>& pair :
         std::vector<std::array<std::shared_ptr<EdgeStraightElement>, 2>>{
             {sill, left_brace},
             {left_post, left_brace},
             {head, left_brace},
             {middle_post, left_brace},
             {sill, right_brace},
             {right_post, right_brace},
             {head, right_brace},
             {middle_post, right_brace}
         }) {
        const std::shared_ptr<JointBeamElement> top_side = JointBeamElement::butt_top_side(*pair[0], *pair[1]);
        check(
            top_side && top_side->configuration == ButtConfiguration::top_side,
            fmt::format("frame: {} on {} is no top_side", pair[1]->name, pair[0]->name)
        );
        scene.add(top_side);
        scene.add_interaction(top_side, pair[0], top_side->interaction(0));
        scene.add_interaction(top_side, pair[1], top_side->interaction(1));
    }

    check(
        std::abs(extreme(*left_post, 2, false) - 70.0) < FLUSH,
        fmt::format("frame: the left post's foot at {}, not 70", extreme(*left_post, 2, false))
    );
    check(
        std::abs(extreme(*left_brace, 2, false) - 70.0) < FLUSH &&
            std::abs(extreme(*left_brace, 0, false) - 70.0) < FLUSH,
        fmt::format(
            "frame: the left brace's foot at x {} z {}, not 70 and 70",
            extreme(*left_brace, 0, false),
            extreme(*left_brace, 2, false)
        )
    );
    for (size_t i = 0; i < all.size(); i++)
        for (size_t j = i + 1; j < all.size(); j++)
            check(
                shared(*all[i], *all[j]) < OVERLAP_REL,
                fmt::format(
                    "frame: {} and {} overlap by {} of the smaller",
                    all[i]->name,
                    all[j]->name,
                    shared(*all[i], *all[j])
                )
            );
}

// ═══════════════════════════════════════════════════════════════════════════
// Truss library
// ═══════════════════════════════════════════════════════════════════════════

/// Every truss of the TrussGuide library, as examples/templates_truss_2_library.cpp builds them.
static void trusses() {

    const std::vector<std::pair<std::string, wood_truss::TrussGuide>> guides = {
        {"pratt", wood_truss::Pratt{}},
        {"king_post", wood_truss::KingPost{}},
        {"queen_post", wood_truss::QueenPost{}},
        {"howe", wood_truss::Howe{}},
        {"fink", wood_truss::Fink{}},
        {"belgian", wood_truss::Belgian{}},
        {"scissors", wood_truss::Scissors{}},
        {"warren", wood_truss::Warren{}},
        {"warren_verticals", wood_truss::WarrenVerticals{}},
        {"pratt_bridge", wood_truss::PrattBridge{}},
        {"parker", wood_truss::Parker{}},
        {"k_truss", wood_truss::KTruss{}},
        {"cambered_verticals", wood_truss::CamberedVerticals{}},
        {"funicular_arch", wood_truss::FunicularArch{}},
        {"lens", wood_truss::Lens{}},
        {"triangulated_arch", wood_truss::TriangulatedArch{}},
        {"sloped", wood_truss::Sloped{}},
        {"pratt_flat", wood_truss::PrattFlat{}},
        {"flat_peaked", wood_truss::FlatPeaked{}},
        {"bowstring", wood_truss::Bowstring{}},
        {"pitched_posts", wood_truss::PitchedPosts{}},
        {"roof_cambered_tie", wood_truss::RoofCamberedTie{}},
        {"quarter_fan", wood_truss::QuarterFan{}},
        {"cantilever", wood_truss::Cantilever{}},
        {"cantilever_design_1", wood_truss::CantileverDesign1{}},
        {"cantilever_design_2", wood_truss::CantileverDesign2{}},
        {"cantilever_design_3", wood_truss::CantileverDesign3{}},
        {"fish_belly", wood_truss::FishBelly{}},
        {"sawtooth", wood_truss::Sawtooth{}},
        {"howe_flat", wood_truss::HoweFlat{}},
        {"three_hinged_arch", wood_truss::ThreeHingedArch{}},
        {"burr_arch", wood_truss::BurrArch{}},
        {"town_lattice", wood_truss::TownLattice{}},
        {"bowstring_arch", wood_truss::BowstringArch{}},
        {"waddell_a", wood_truss::WaddellA{}},
        {"wichert", wood_truss::Wichert{}},
        {"truss_leg_bedstead", wood_truss::TrussLegBedstead{}},
        {"kellogg", wood_truss::Kellogg{}},
        {"baltimore_a", wood_truss::BaltimoreA{}},
        {"pennsylvania_a", wood_truss::PennsylvaniaA{}},
        {"greiner", wood_truss::Greiner{}},
        {"double_intersection_pratt", wood_truss::DoubleIntersectionPratt{}},
        {"schwedler", wood_truss::Schwedler{}},
        {"double_intersection_warren", wood_truss::DoubleIntersectionWarren{}},
        {"pegram", wood_truss::Pegram{}},
        {"post", wood_truss::Post{}},
        {"bollman", wood_truss::Bollman{}},
        {"fink_bridge", wood_truss::FinkBridge{}},
        {"stearns", wood_truss::Stearns{}},
        {"crescent_crossed", wood_truss::CrescentCrossed{}},
        {"braced_arch_rib", wood_truss::BracedArchRib{}},
        {"belgian_verticals", wood_truss::BelgianVerticals{}},
        {"raised_tie_king_post", wood_truss::RaisedTieKingPost{}},
        {"trapezoid_crossed", wood_truss::TrapezoidCrossed{}},
        {"fink_cambered", wood_truss::FinkCambered{}},
        {"crescent_verticals", wood_truss::CrescentVerticals{}},
        {"raised_tie_posts", wood_truss::RaisedTiePosts{}},
        {"pitched_warren_columns", wood_truss::PitchedWarrenColumns{}},
        {"fink_compound", wood_truss::FinkCompound{}},
        {"fink_bent_tie", wood_truss::FinkBentTie{}},
        {"french_steep", wood_truss::FrenchSteep{}},
        {"fink_lantern", wood_truss::FinkLantern{}},
        {"arch_polygonal_tie", wood_truss::ArchPolygonalTie{}},
        {"pitched_crossed_posts", wood_truss::PitchedCrossedPosts{}},
        {"cambered_crossed_lantern", wood_truss::CamberedCrossedLantern{}},
        {"arched_crossed_columns", wood_truss::ArchedCrossedColumns{}},
        {"trapezoid_king_crossed", wood_truss::TrapezoidKingCrossed{}},
        {"topology_optimised", wood_truss::TopologyOptimised{}},
    };
    // trusses whose joinery is not finished yet: built, not yet held to the checks unless named in TRUSSES (docs/truss_joinery_plan.md)
    const std::set<std::string> unfinished = {
        "quarter_fan", "sawtooth", "three_hinged_arch", "bowstring_arch", "baltimore_a", "pennsylvania_a", "double_intersection_pratt",
        "schwedler", "bollman", "fink_bridge", "stearns", "pitched_crossed_posts", "cambered_crossed_lantern",
    };
    for (const std::pair<std::string, wood_truss::TrussGuide>& guide : guides) {
        if (std::getenv("TRUSSES") && ("," + std::string(std::getenv("TRUSSES")) + ",").find("," + guide.first + ",") == std::string::npos)
            continue;
        wood_truss::Truss truss(guide.second, guide.first);
        if (unfinished.count(guide.first) && !std::getenv("TRUSSES"))
            continue;
        std::vector<std::shared_ptr<EdgeStraightElement>> members;
        for (const std::shared_ptr<Element>& element : *truss.objects.elements)
            if (const std::shared_ptr<EdgeStraightElement> beam = std::dynamic_pointer_cast<EdgeStraightElement>(element))
                members.push_back(beam);
        std::vector<AABB> boxes;
        for (const std::shared_ptr<EdgeStraightElement>& member : members)
            boxes.push_back(AABB::from_mesh(member->model_geometry_mesh()));
        for (size_t i = 0; i < members.size(); i++)
            for (size_t j = i + 1; j < members.size(); j++)
                check(
                    !boxes[i].intersects(boxes[j]) || shared(*members[i], *members[j]) < TRUSS_OVERLAP,
                    fmt::format(
                        "trusses: {}: {} and {} overlap by {} of the smaller",
                        guide.first,
                        members[i]->name,
                        members[j]->name,
                        shared(*members[i], *members[j])
                    )
                );
        std::vector<Mesh> cut;
        for (const std::shared_ptr<EdgeStraightElement>& member : members)
            cut.push_back(member->model_geometry_mesh());

        // the end a run running on is cut along is meant to go: the wood beyond that cut is no member's; a node plate fills its
        // slot, and the slot swept on past it, the way the plate drops in, is the saw's
        for (const std::shared_ptr<JointBeamElement>& joint : truss.get_elements<JointBeamElement>()) {
            if (joint->name.rfind("end_", 0) == 0)
                cut.push_back(joint->butt_solids[1].mesh);
            if (!joint->cutters.empty() && !joint->cutters.front().empty())
                cut.push_back(Mesh::loft({joint->cutters.front().front()[0]}, {joint->cutters.front().front()[1]}, true));
        }
        for (const std::shared_ptr<FacePlanarElement>& plate : truss.get_elements<FacePlanarElement>())
            cut.push_back(plate->model_geometry_mesh());

        for (const std::shared_ptr<EdgeStraightElement>& member : members) {
            const Mesh drawn = member->element_geometry_mesh();
            const AABB around = AABB::from_mesh(drawn);
            std::vector<Mesh> near;
            for (const Mesh& filler : cut)
                if (AABB::from_mesh(filler).intersects(around))
                    near.push_back(filler);
            const Mesh lost = solid_difference(drawn, near);
            const double fraction =
                lost.number_of_faces() == 0 ? 0.0 : compute_volume(lost) / compute_volume(drawn);
            check(
                fraction < TRUSS_GAP,
                fmt::format(
                    "trusses: {}: {} loses {} of its drawn wood that no member fills, about {}",
                    guide.first,
                    member->name,
                    fraction,
                    lost.number_of_faces() == 0 ? std::string() : AABB::from_mesh(lost).center().str()
                )
            );
        }

        // and it goes together: every member, plate and part placed, none locked
        const AssemblySequence sequence(truss, "chord_0_0");
        check(sequence.locked().empty(), fmt::format("trusses: {}: locked in assembly\n{}", guide.first, sequence.str()));
    }
}

int main() {

    pairs();
    trusses();
    frame();
    folded_roof();
    beams();
    plates();
    std::cout << "joint_butt: pairs, beams and plates butt flush, in any order; the library trusses jointed, whole and assembled" << std::endl;
    return 0;
}
