#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/truss/truss.h"

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

    const std::shared_ptr<JointBeam> butt = JointBeam::butt(*running, *butting);
    butt->name = fmt::format("butt_{}_{}", running->name, butting->name);
    scene.add(butt);
    scene.add_interaction(butt, running, butt->interaction(0));
    scene.add_interaction(butt, butting, butt->interaction(1));
}

// ═══════════════════════════════════════════════════════════════════════════
// Pairs
// ═══════════════════════════════════════════════════════════════════════════

/// A pair butted, or butted side_side, in its own scene.
static void pair(
    const std::string& name,
    const std::shared_ptr<Element>& a,
    const std::shared_ptr<Element>& b,
    ButtConfiguration expected,
    const std::function<void()>& measure,
    bool side_side = false
) {

    WoodSession scene(name);
    scene.add(a);
    scene.add(b);
    const std::shared_ptr<JointBeam> butt =
        side_side ? JointBeam::butt_side_side(*a, *b) : JointBeam::butt(*a, *b);
    check(
        butt && butt->configuration == expected,
        fmt::format(
            "{}: configuration {} instead of {}",
            name,
            butt ? (int)butt->configuration : -1,
            (int)expected
        )
    );
    scene.add(butt);
    scene.add_interaction(butt, a, butt->interaction(0));
    scene.add_interaction(butt, b, butt->interaction(1));
    measure();
    if (expected != ButtConfiguration::cross)
        check(
            shared(*a, *b) < OVERLAP_REL,
            fmt::format("{}: the pair overlaps by {} of the smaller", name, shared(*a, *b))
        );
}

/// Pairwise beam and plate butt cases, each checked at its measured faces.
static void pairs() {

    const auto beam = [](const Point& start, const Point& end) {
        return std::make_shared<Beam>(Polyline({start, end}), 75.0, "beam");
    };
    const auto wall = [](const Point& start, const Point& end) {
        return Plate::from_line(
            Line::from_points(start, end), Vector(0.0, 0.0, 1.0), 1500.0, 120.0, "wall"
        );
    };
    const auto near = [](double value, double expected, const std::string& what) {
        check(
            std::abs(value - expected) < FLUSH,
            fmt::format("{} at {}, not {}", what, value, expected)
        );
    };

    // beams: top_side, top_side at 18 degrees, top_side at an end drawn past and short, side_side_outplane, side_side_inplane and a cross
    std::shared_ptr<Beam> a = beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0});
    std::shared_ptr<Beam> b = beam({0.0, 0.0, 0.0}, {0.0, 0.0, 1500.0});
    pair("beam_top_side", a, b, ButtConfiguration::top_side, [&] {
        near(extreme(*b, 2, false), 75.0, "beam_top_side: the post's foot");
        near(extreme(*a, 0, true), 1000.0, "beam_top_side: the through beam's end");
    });
    a = beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0});
    b = beam({0.0, 0.0, 0.0}, {-1902.1130325903071, 0.0, -618.03398874989485});
    pair("beam_top_side_shallow", a, b, ButtConfiguration::top_side, [&] {
        near(extreme(*b, 2, true), -75.0, "beam_top_side_shallow: the strut's top at 18 degrees");
    });
    for (const double drawn : {-150.0, 100.0}) {
        a = beam({drawn, 0.0, 0.0}, {1500.0, 0.0, 0.0});
        b = beam({0.0, 0.0, drawn}, {0.0, 0.0, 1500.0});
        pair(fmt::format("beam_top_side_end_{}", drawn), a, b, ButtConfiguration::top_side, [&] {
            near(extreme(*a, 0, false), -75.0, "beam_top_side_end: the running end");
            near(extreme(*b, 2, false), 75.0, "beam_top_side_end: the butting end");
        });
    }
    a = beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0});
    b = beam({0.0, 0.0, 0.0}, {-750.0, 0.0, 1300.0});
    pair("beam_side_side_outplane", a, b, ButtConfiguration::side_side_outplane, [&] {
        near(extreme(*a, 2, false), -75.0, "beam_side_side_outplane: the left beam's underside");
    });
    a = beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0});
    b = beam({0.0, 0.0, 0.0}, {0.0, 0.0, 1500.0});
    pair(
        "beam_side_side_outplane_square",
        a,
        b,
        ButtConfiguration::side_side_outplane,
        [&] {
            near(
                extreme(*a, 0, false),
                -75.0,
                "beam_side_side_outplane_square: the left beam's outer corner"
            );
            near(
                extreme(*b, 2, false),
                -75.0,
                "beam_side_side_outplane_square: the right beam's outer corner"
            );
        },
        true
    );
    a = beam({-1500.0, 0.0, 0.0}, {0.0, 0.0, 0.0});
    b = beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0});
    pair("beam_side_side_inplane", a, b, ButtConfiguration::side_side_inplane, [&] {
        near(extreme(*a, 0, true), 0.0, "beam_side_side_inplane: the left end");
        near(extreme(*b, 0, false), 0.0, "beam_side_side_inplane: the right end");
    });
    a = beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0});
    b = beam({0.0, -1000.0, 0.0}, {0.0, 1000.0, 0.0});
    pair("beam_cross", a, b, ButtConfiguration::cross, [&] {
        near(extreme(*a, 0, true), 1000.0, "beam_cross: the first beam's end");
    });

    // plates: top_side at an end, side_side_outplane at 120 degrees, side_side_inplane, top_side on a floor
    std::shared_ptr<Plate> p = wall({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0});
    std::shared_ptr<Plate> q = wall({0.0, 0.0, 0.0}, {0.0, 2000.0, 0.0});
    pair("plate_top_side_end", p, q, ButtConfiguration::top_side, [&] {
        near(extreme(*p, 0, false), -60.0, "plate_top_side_end: the running wall's end");
        near(extreme(*q, 1, false), 60.0, "plate_top_side_end: the butting wall's end");
    });
    p = wall({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0});
    q = wall({0.0, 0.0, 0.0}, {-1000.0, 1732.0508075688772, 0.0});
    pair("plate_side_side_outplane", p, q, ButtConfiguration::side_side_outplane, [&] {
        near(extreme(*p, 1, false), -60.0, "plate_side_side_outplane: the left wall's outer face");
    });
    p = wall({-2000.0, 0.0, 0.0}, {0.0, 0.0, 0.0});
    q = wall({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0});
    pair("plate_side_side_inplane", p, q, ButtConfiguration::side_side_inplane, [&] {
        near(extreme(*p, 0, true), 0.0, "plate_side_side_inplane: the left end");
        near(extreme(*q, 0, false), 0.0, "plate_side_side_inplane: the right end");
    });
    p = Plate::from_rectangle(
        {-1500.0, -1500.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 3000.0, 3000.0, 200.0, "floor"
    );
    q = wall({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0});
    pair("plate_top_side_floor", p, q, ButtConfiguration::top_side, [&] {
        near(extreme(*q, 2, false), 100.0, "plate_top_side_floor: the wall's foot");
    });

    // beams and plates: a column under a slab, a wall on a beam
    p = Plate::from_rectangle(
        {-1500.0, -1500.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 3000.0, 3000.0, 200.0, "slab"
    );
    const std::shared_ptr<Column> column = Column::square(
        Line::from_points({0.0, 0.0, -2000.0}, {0.0, 0.0, 0.0}),
        Plane({-100.0, -100.0, -2000.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}),
        200.0,
        "column"
    );
    pair("column_top_side_plate", p, column, ButtConfiguration::top_side, [&] {
        near(extreme(*column, 2, true), -100.0, "column_top_side_plate: the column's head");
    });
    const std::shared_ptr<Beam> sill =
        std::make_shared<Beam>(Polyline({{-1500.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}}), 100.0, "beam");
    q = wall({-1200.0, 0.0, 0.0}, {1200.0, 0.0, 0.0});
    pair("plate_top_side_beam", sill, q, ButtConfiguration::top_side, [&] {
        near(extreme(*q, 2, false), 100.0, "plate_top_side_beam: the wall's foot");
    });
}

// ═══════════════════════════════════════════════════════════════════════════
// Beams
// ═══════════════════════════════════════════════════════════════════════════

/// A post on the middle of a sill and one on its end, 150 x 150 beams on axes meeting at nodes.
static void beams() {

    WoodSession scene("butt_beams");
    const std::shared_ptr<Beam> sill =
        std::make_shared<Beam>(Polyline({{-600.0, 0.0, 0.0}, {600.0, 0.0, 0.0}}), 75.0, "sill");
    const std::shared_ptr<Beam> middle =
        std::make_shared<Beam>(Polyline({{0.0, 0.0, 0.0}, {0.0, 0.0, 800.0}}), 75.0, "middle");
    const std::shared_ptr<Beam> end =
        std::make_shared<Beam>(Polyline({{600.0, 0.0, 0.0}, {600.0, 0.0, 800.0}}), 75.0, "end");
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
    const std::shared_ptr<Plate> floor = Plate::from_rectangle(
        {0.0, 0.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6000.0, 4000.0, 200.0, "floor"
    );
    const std::shared_ptr<Plate> roof = Plate::from_rectangle(
        {0.0, 0.0, 2700.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6000.0, 4000.0, 200.0, "roof"
    );
    std::array<std::shared_ptr<Plate>, 4> walls;
    for (size_t i = 0; i < 4; i++)
        walls[i] = Plate::from_line(
            Line::from_points(footprint[i], footprint[(i + 1) % 4]),
            {0.0, 0.0, 1.0},
            2800.0,
            120.0,
            fmt::format("wall_{}", i)
        );
    scene.add(floor);
    scene.add(roof);
    for (const std::shared_ptr<Plate>& wall : walls)
        scene.add(wall);
    for (const std::array<size_t, 2>& corner :
         std::array<std::array<size_t, 2>, 4>{{{0, 1}, {0, 3}, {2, 1}, {2, 3}}})
        join(scene, walls[corner[0]], walls[corner[1]]);
    for (const std::shared_ptr<Plate>& wall : walls)
        join(scene, floor, wall);
    for (const std::shared_ptr<Plate>& wall : walls)
        join(scene, roof, wall);

    for (const std::shared_ptr<Plate>& slab : {floor, roof}) {
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
    for (const std::shared_ptr<Plate>& wall : walls)
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
    for (const std::shared_ptr<Plate>& slab : {floor, roof})
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

    const std::array<std::shared_ptr<Plate>, 6> all = {
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
    std::vector<std::shared_ptr<Plate>> roof;
    for (size_t i = 0; i + 1 < zigzag.size(); i++) {
        const Vector along = (zigzag[i + 1] - zigzag[i]).normalized();
        const Vector across = along.cross(Vector(0.0, 1.0, 0.0));

        const double before = i == 0 ? 300.0 / along[0] : 0.0;
        const double after = i + 2 == zigzag.size() ? 300.0 / along[0] : 0.0;
        roof.push_back(Plate::from_rectangle(
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
    std::array<std::shared_ptr<Plate>, 2> gables;
    for (size_t g = 0; g < 2; g++) {
        const Polyline drawn = Polyline(outline).translated({0.0, g == 0 ? 0.0 : 4000.0, 0.0});
        gables[g] = std::make_shared<Plate>(
            drawn.translated({0.0, -70.0, 0.0}),
            drawn.translated({0.0, 70.0, 0.0}),
            fmt::format("gable_{}", g)
        );
        scene.add(gables[g]);
    }
    for (const std::shared_ptr<Plate>& plate : roof)
        for (const std::shared_ptr<Plate>& gable : gables)
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
    for (const std::shared_ptr<Plate>& gable : gables)
        check(
            std::abs(compute_volume(gable->model_geometry_mesh()) / expected - 1.0) < 1e-9,
            fmt::format(
                "folded_roof: {} holds {} mm3, not the {} under the roof",
                gable->name,
                compute_volume(gable->model_geometry_mesh()),
                expected
            )
        );

    std::vector<std::shared_ptr<Plate>> all = roof;
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

/// Timber-frame wall with posts, sill, plate, lintel, and diagonal brace.
static void frame() {

    WoodSession scene("butt_frame");
    const auto beam = [](const Point& start, const Point& end, const std::string& name) {
        return std::make_shared<Beam>(Polyline({start, end}), 70.0, name);
    };
    const std::shared_ptr<Beam> sill = beam({0.0, 0.0, 0.0}, {3000.0, 0.0, 0.0}, "sill");
    const std::shared_ptr<Beam> head = beam({0.0, 0.0, 2400.0}, {3000.0, 0.0, 2400.0}, "head");
    const std::shared_ptr<Beam> left_post = beam({0.0, 0.0, 0.0}, {0.0, 0.0, 2400.0}, "left_post");
    const std::shared_ptr<Beam> middle_post =
        beam({1500.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}, "middle_post");
    const std::shared_ptr<Beam> right_post =
        beam({3000.0, 0.0, 0.0}, {3000.0, 0.0, 2400.0}, "right_post");
    const std::shared_ptr<Beam> left_brace =
        beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}, "left_brace");
    const std::shared_ptr<Beam> right_brace =
        beam({3000.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}, "right_brace");
    const std::array<std::shared_ptr<Beam>, 7> all = {
        sill, head, left_post, middle_post, right_post, left_brace, right_brace
    };
    for (const std::shared_ptr<Beam>& member : all)
        scene.add(member);
    for (const std::array<std::shared_ptr<Beam>, 2>& pair :
         std::vector<std::array<std::shared_ptr<Beam>, 2>>{
             {sill, left_post},
             {sill, middle_post},
             {sill, right_post},
             {head, left_post},
             {head, middle_post},
             {head, right_post}
         })
        join(scene, pair[0], pair[1]);
    for (const std::array<std::shared_ptr<Beam>, 2>& pair :
         std::vector<std::array<std::shared_ptr<Beam>, 2>>{
             {sill, left_brace},
             {left_post, left_brace},
             {head, left_brace},
             {middle_post, left_brace},
             {sill, right_brace},
             {right_post, right_brace},
             {head, right_brace},
             {middle_post, right_brace}
         }) {
        const std::shared_ptr<JointBeam> top_side = JointBeam::butt_top_side(*pair[0], *pair[1]);
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
        {"pratt", wood_truss::TrussGuide::pratt()},
        {"king_post", wood_truss::TrussGuide::king_post()},
        {"queen_post", wood_truss::TrussGuide::queen_post()},
        {"howe", wood_truss::TrussGuide::howe()},
        {"fink", wood_truss::TrussGuide::fink()},
        {"belgian", wood_truss::TrussGuide::belgian()},
        {"scissors", wood_truss::TrussGuide::scissors()},
        {"warren", wood_truss::TrussGuide::warren()},
        {"warren_verticals", wood_truss::TrussGuide::warren_verticals()},
        {"pratt_bridge", wood_truss::TrussGuide::pratt_bridge()},
        {"parker", wood_truss::TrussGuide::parker()},
        {"k_truss", wood_truss::TrussGuide::k_truss()},
        {"cambered_verticals", wood_truss::TrussGuide::cambered_verticals()},
        {"funicular_arch", wood_truss::TrussGuide::funicular_arch()},
        {"lens", wood_truss::TrussGuide::lens()},
        {"triangulated_arch", wood_truss::TrussGuide::triangulated_arch()},
        {"sloped", wood_truss::TrussGuide::sloped()},
        {"pratt_flat", wood_truss::TrussGuide::pratt_flat()},
        {"flat_peaked", wood_truss::TrussGuide::flat_peaked()},
        {"bowstring", wood_truss::TrussGuide::bowstring()},
        {"pitched_posts", wood_truss::TrussGuide::pitched_posts()},
        {"roof_cambered_tie", wood_truss::TrussGuide::roof_cambered_tie()},
        {"quarter_fan", wood_truss::TrussGuide::quarter_fan()},
        {"cantilever", wood_truss::TrussGuide::cantilever()},
        {"cantilever_design_1", wood_truss::TrussGuide::cantilever_design_1()},
        {"cantilever_design_2", wood_truss::TrussGuide::cantilever_design_2()},
        {"cantilever_design_3", wood_truss::TrussGuide::cantilever_design_3()},
        {"fish_belly", wood_truss::TrussGuide::fish_belly()},
        {"sawtooth", wood_truss::TrussGuide::sawtooth()},
        {"howe_flat", wood_truss::TrussGuide::howe_flat()},
        {"three_hinged_arch", wood_truss::TrussGuide::three_hinged_arch()},
        {"burr_arch", wood_truss::TrussGuide::burr_arch()},
        {"town_lattice", wood_truss::TrussGuide::town_lattice()},
        {"bowstring_arch", wood_truss::TrussGuide::bowstring_arch()},
        {"waddell_a", wood_truss::TrussGuide::waddell_a()},
        {"wichert", wood_truss::TrussGuide::wichert()},
        {"truss_leg_bedstead", wood_truss::TrussGuide::truss_leg_bedstead()},
        {"kellogg", wood_truss::TrussGuide::kellogg()},
        {"baltimore_a", wood_truss::TrussGuide::baltimore_a()},
        {"pennsylvania_a", wood_truss::TrussGuide::pennsylvania_a()},
        {"greiner", wood_truss::TrussGuide::greiner()},
        {"double_intersection_pratt", wood_truss::TrussGuide::double_intersection_pratt()},
        {"schwedler", wood_truss::TrussGuide::schwedler()},
        {"double_intersection_warren", wood_truss::TrussGuide::double_intersection_warren()},
        {"pegram", wood_truss::TrussGuide::pegram()},
        {"post", wood_truss::TrussGuide::post()},
        {"bollman", wood_truss::TrussGuide::bollman()},
        {"fink_bridge", wood_truss::TrussGuide::fink_bridge()},
        {"stearns", wood_truss::TrussGuide::stearns()},
        {"crescent_crossed", wood_truss::TrussGuide::crescent_crossed()},
        {"braced_arch_rib", wood_truss::TrussGuide::braced_arch_rib()},
        {"belgian_verticals", wood_truss::TrussGuide::belgian_verticals()},
        {"raised_tie_king_post", wood_truss::TrussGuide::raised_tie_king_post()},
        {"trapezoid_crossed", wood_truss::TrussGuide::trapezoid_crossed()},
        {"fink_cambered", wood_truss::TrussGuide::fink_cambered()},
        {"crescent_verticals", wood_truss::TrussGuide::crescent_verticals()},
        {"raised_tie_posts", wood_truss::TrussGuide::raised_tie_posts()},
        {"pitched_warren_columns", wood_truss::TrussGuide::pitched_warren_columns()},
        {"fink_compound", wood_truss::TrussGuide::fink_compound()},
        {"fink_bent_tie", wood_truss::TrussGuide::fink_bent_tie()},
        {"french_steep", wood_truss::TrussGuide::french_steep()},
        {"fink_lantern", wood_truss::TrussGuide::fink_lantern()},
        {"arch_polygonal_tie", wood_truss::TrussGuide::arch_polygonal_tie()},
        {"pitched_crossed_posts", wood_truss::TrussGuide::pitched_crossed_posts()},
        {"cambered_crossed_lantern", wood_truss::TrussGuide::cambered_crossed_lantern()},
        {"arched_crossed_columns", wood_truss::TrussGuide::arched_crossed_columns()},
        {"trapezoid_king_crossed", wood_truss::TrussGuide::trapezoid_king_crossed()},
        {"topology_optimised", wood_truss::TrussGuide::topology_optimised()},
    };
    for (const std::pair<std::string, wood_truss::TrussGuide>& guide : guides) {
        const wood_truss::Truss truss(guide.second, guide.first);
        std::vector<std::shared_ptr<Beam>> members;
        for (const std::shared_ptr<Element>& element : *truss.objects.elements)
            if (const std::shared_ptr<Beam> beam = std::dynamic_pointer_cast<Beam>(element))
                members.push_back(beam);
        for (size_t i = 0; i < members.size(); i++)
            for (size_t j = i + 1; j < members.size(); j++)
                check(
                    shared(*members[i], *members[j]) < TRUSS_OVERLAP,
                    fmt::format(
                        "trusses: {}: {} and {} overlap by {} of the smaller",
                        guide.first,
                        members[i]->name,
                        members[j]->name,
                        shared(*members[i], *members[j])
                    )
                );
        std::vector<Mesh> cut;
        for (const std::shared_ptr<Beam>& member : members)
            cut.push_back(member->model_geometry_mesh());
        for (const std::shared_ptr<Beam>& member : members) {
            const Mesh drawn = member->element_geometry_mesh();
            const Mesh lost = solid_difference(drawn, cut);
            const double fraction =
                lost.number_of_faces() == 0 ? 0.0 : compute_volume(lost) / compute_volume(drawn);
            check(
                fraction < TRUSS_GAP,
                fmt::format(
                    "trusses: {}: {} loses {} of its drawn wood that no member fills",
                    guide.first,
                    member->name,
                    fraction
                )
            );
        }
        if (guide.first == "pratt")
            check(
                std::abs(extreme(*truss.get_element_by_name<Beam>("web_0"), 2, false) - 80.0) <
                    FLUSH,
                "trusses: pratt: the first vertical does not stand on the bottom chord's top"
            );
    }
}

int main() {

    pairs();
    trusses();
    frame();
    folded_roof();
    beams();
    plates();
    std::cout << "joint_butt: pairs, beams and plates butt flush, in any order" << std::endl;
    return 0;
}
