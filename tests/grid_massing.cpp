#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_grid;

namespace {

constexpr double CLEARANCE = 1.0;

void check(bool ok, const std::string& message) {
    if (!ok)
        throw std::runtime_error(message);
}

std::vector<Plane> outward_planes(const Mesh& mesh) {
    const Point centre = mesh.centroid();
    std::vector<Plane> planes;
    for (const size_t face : mesh.faces()) {
        const std::vector<Point> pts = mesh.face_polygon(face)->get_points();
        const Vector n = compute_newell(pts).normalized();
        const Point orig = Point::centroid(pts);
        planes.push_back(
            Plane::from_point_normal(orig, (orig - centre).dot(n) < 0.0 ? -n : n)
        );
    }
    return planes;
}

bool is_convex_mesh(const Mesh& mesh, const std::vector<Plane>& planes) {
    for (const Plane& p : planes)
        for (const size_t v : mesh.vertices())
            if ((*mesh.vertex_point(v) - p.origin()).dot(p.z_axis()) > CLEARANCE)
                return false;
    return true;
}

double convex_overlap(const Mesh& convex, const Mesh& other) {
    Mesh part = other;
    for (const Plane& p : outward_planes(convex)) {
        part = part.cut_by_plane(
            Plane::from_point_normal(p.origin() - p.z_axis() * CLEARANCE, -p.z_axis())
        );
        if (part.faces().empty())
            return 0.0;
    }
    return std::abs(part.volume());
}

std::pair<Point, Point> mesh_box(const Mesh& mesh) {
    std::pair<Point, Point> b(Point(1e18, 1e18, 1e18), Point(-1e18, -1e18, -1e18));
    for (const size_t v : mesh.vertices()) {
        const Point p = *mesh.vertex_point(v);
        for (int a = 0; a < 3; ++a) {
            b.first[a] = std::min(b.first[a], p[a]);
            b.second[a] = std::max(b.second[a], p[a]);
        }
    }
    return b;
}

bool boxes_near(const Mesh& a, const Mesh& b) {
    const auto ba = mesh_box(a);
    const auto bb = mesh_box(b);
    for (int axis = 0; axis < 3; ++axis)
        if (ba.second[axis] <= bb.first[axis] + CLEARANCE ||
            bb.second[axis] <= ba.first[axis] + CLEARANCE)
            return false;
    return true;
}

double largest_clash(const WoodSession& session) {
    double worst = 0.0;
    const auto& elems = session.elements();
    for (size_t i = 0; i < elems.size(); ++i) {
        const Mesh& a = elems[i]->model_geometry_mesh();
        const auto pa = outward_planes(a);
        const bool ca = is_convex_mesh(a, pa);
        for (size_t j = i + 1; j < elems.size(); ++j) {
            const Mesh& b = elems[j]->model_geometry_mesh();
            if (!boxes_near(a, b))
                continue;
            const auto pb = outward_planes(b);
            const bool cb = is_convex_mesh(b, pb);
            if (!ca && !cb)
                continue;
            worst = std::max(worst, ca ? convex_overlap(a, b) : convex_overlap(b, a));
        }
    }
    return worst;
}

void verify_grid(const std::string& label, const Grid& grid) {
    check(!grid.elements().empty(), label + ": produced zero elements");
    check(grid.objects.meshes->empty(), label + ": plan must not emit triangulated meshes");
    const double clash = largest_clash(grid);
    std::cout << fmt::format(
        "{}: {} elements, {} contacts, largest clash {:.3f} mm3\n",
        label,
        grid.elements().size(),
        grid.graph.number_of_edges(),
        clash
    );
    check(clash <= CLEARANCE, fmt::format("{}: overlap {:.3f} mm3 exceeds tolerance", label, clash));
}

} // namespace

int main() {
    try {
    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);
    const Plane FLOOR = Plane(Point(0.0, 0.0, 4000.0), X, Y);

    // 1. Bay isolation invariants (`Plan::bay`).
    const Polyline l_ring({
        Point(0.0, 0.0, 0.0),
        Point(12000.0, 0.0, 0.0),
        Point(12000.0, 4000.0, 0.0),
        Point(8000.0, 4000.0, 0.0),
        Point(8000.0, 8000.0, 0.0),
        Point(0.0, 8000.0, 0.0),
        Point(0.0, 0.0, 0.0),
    });
    const Plan l_plan = Plan::from_footprint(
        {l_ring}, Pattern::orthogonal({4000.0, 4000.0, 4000.0}, {4000.0, 4000.0}), {}, FLOOR
    );
    check(l_plan.bay_count() == 5, "L-plan must have 5 bays");
    const Plan sub_bay = l_plan.bay(0);
    check(sub_bay.bay_count() == 1, "isolated bay must have 1 cell");
    check(sub_bay.edges.size() == 4, "isolated rectangular bay must have 4 boundary edges");
    verify_grid(
        "isolated_bay",
        Grid(sub_bay, Framing{.system = System::PostAndBeam, .node = Node::Flush, .span = 0, .deck = 160.0})
    );

    // 2. Single-bay framing modes (Slab+Head, PostAndBeam+Head, Flush, Through, StackedPurlin).
    const Plan one = Plan::from_pattern(Pattern::orthogonal({6000.0}, {6000.0}), FLOOR);
    verify_grid(
        "slab_head",
        Grid(one, Framing{.system = System::Slab, .node = Node::Head, .deck = 240.0, .panel = 3000.0})
    );
    verify_grid(
        "beam_head",
        Grid(one, Framing{.system = System::PostAndBeam, .node = Node::Head, .span = -1})
    );
    verify_grid(
        "purlin_flush",
        Grid(one, Framing{.system = System::GirderPurlin, .node = Node::Flush, .spacing = 2000.0, .deck = 140.0})
    );
    verify_grid(
        "node_through",
        Grid(one, Framing{.system = System::GirderPurlin, .node = Node::Through, .spacing = 2000.0})
    );
    verify_grid(
        "purlin_stacked",
        Grid(one, Framing{.system = System::StackedPurlin, .node = Node::Flush, .spacing = 2000.0})
    );

    // 3. Non-orthogonal footprints: skewed, radial (with overhang check), triangular, hexagonal, irregular.
    verify_grid(
        "skewed",
        Grid(
            Plan::from_pattern(Pattern::orthogonal({6000.0, 6000.0}, {6000.0, 6000.0}, 30.0), FLOOR),
            Framing{.system = System::GirderPurlin, .node = Node::Flush, .spacing = 2000.0}
        )
    );

    const Plan radial_plan =
        Plan::from_pattern(Pattern::radial({5000.0, 10000.0, 15000.0}, 16, true), FLOOR);
    check(radial_plan.bay_count() == 32, "16-sector 2-annulus radial grid must have 32 quad bays");
    const Grid radial_grid(
        radial_plan,
        Framing{
            .system = System::PostAndBeam,
            .node = Node::Flush,
            .span = 0,
            .profiles = {.column = profile_round(280.0)},
        }
    );
    verify_grid("radial", radial_grid);
    for (const auto& el : radial_grid.elements()) {
        for (const size_t v : el->model_geometry_mesh().vertices()) {
            const Point p = *el->model_geometry_mesh().vertex_point(v);
            check(std::hypot(p[0], p[1]) <= 15300.0, "radial element exceeds outer radius overhang");
        }
    }

    verify_grid(
        "triangular",
        Grid(
            Plan::from_pattern(Pattern::triangular(6000.0, 3, 2), FLOOR),
            Framing{
                .system = System::PostAndBeam,
                .node = Node::Flush,
                .span = -1,
                .profiles = {.column = profile_round(280.0)},
            }
        )
    );
    verify_grid(
        "hexagonal",
        Grid(
            Plan::from_pattern(Pattern::hexagonal(4000.0, 3, 2), FLOOR),
            Framing{
                .system = System::PostAndBeam,
                .node = Node::Flush,
                .span = -1,
                .profiles = {.column = profile_round(220.0), .girder = profile_rectangle(220.0, 420.0)},
            }
        )
    );
    verify_grid(
        "irregular_t_joints",
        Grid(
            Plan::from_footprint(
                {Polyline({
                    Point(0.0, 0.0, 0.0),
                    Point(26000.0, 1500.0, 0.0),
                    Point(28000.0, 13500.0, 0.0),
                    Point(14000.0, 18000.0, 0.0),
                    Point(-1000.0, 11500.0, 0.0),
                    Point(0.0, 0.0, 0.0),
                })},
                Pattern::from_lines({
                    Line::from_points(Point(-2000.0, 4800.0, 0.0), Point(30000.0, 6000.0, 0.0)),
                    Line::from_points(Point(-2000.0, 9000.0, 0.0), Point(30000.0, 10200.0, 0.0)),
                    Line::from_points(Point(7200.0, -2000.0, 0.0), Point(5800.0, 20000.0, 0.0)),
                    Line::from_points(Point(14800.0, -2000.0, 0.0), Point(15600.0, 20000.0, 0.0)),
                    Line::from_points(Point(21200.0, -2000.0, 0.0), Point(22000.0, 20000.0, 0.0)),
                }),
                {},
                FLOOR
            ),
            Framing{
                .system = System::PostAndBeam,
                .node = Node::Flush,
                .span = 0,
                .profiles =
                    {.column = profile_rectangle(240.0, 240.0),
                     .girder = profile_rectangle(240.0, 460.0)},
            }
        )
    );

    // 4. Vertical facade grid (`Plan::facade`) with X-braces.
    const Plan facade_plan = Plan::facade(
        Line::from_points(Point(0.0, 0.0, 0.0), Point(12000.0, 0.0, 0.0)),
        7200.0,
        {4000.0, 4000.0, 4000.0},
        {3600.0, 3600.0}
    );
    verify_grid(
        "braced_facade",
        Grid(
            facade_plan,
            Framing{
                .system = System::PostAndBeam,
                .brace = Brace::Cross,
                .braced_bays = {0, 2},
                .span = 1,
                .height = 0.0,
            }
        )
    );

    // 5. Multi-storey `Stack` with through-columns & notched CLT decks.
    const Stack stack = Stack::from_footprint(
        {l_ring},
        {0.0, 3600.0, 7200.0},
        Pattern::orthogonal({4000.0, 4000.0, 4000.0}, {4000.0, 4000.0}),
        Framing{.system = System::PostAndBeam, .node = Node::Through, .span = -1, .deck = 160.0}
    );
    verify_grid("multistorey_stack", Grid(stack));

    // 6. Multi-storey `Stack` with pinwheel CLT core walls + perimeter braced facades.
    const Polyline box_outer = Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 16000.0, 12000.0);
    const Polyline box_core = Polyline::rectangle(Point(4000.0, 3000.0, 0.0), X, Y, 8000.0, 6000.0);
    Stack core_stack = Stack::from_footprint(
        {box_outer},
        {0.0, 3800.0, 7600.0},
        Pattern::orthogonal({8000.0, 8000.0}, {6000.0, 6000.0}),
        Framing{
            .system = System::GirderPurlin,
            .node = Node::Flush,
            .spacing = 2000.0,
            .deck = 160.0,
            .wall = 240.0,
        },
        {box_core}
    );
    core_stack.add_facades(
        3800.0,
        Framing{
            .system = System::PostAndBeam,
            .brace = Brace::Cross,
            .span = 1,
            .profiles =
                {.girder = profile_rectangle(140.0, 180.0),
                 .beam = profile_rectangle(120.0, 180.0),
                 .brace = profile_rectangle(120.0, 140.0)},
        }
    );
    verify_grid("core_and_facade_stack", Grid(core_stack));

    // 7. In-line chamfered column heads (`[-beam_depth, 0]`) + notched flush cassette decks.
    const Stack inline_head_stack = Stack::from_footprint(
        {l_ring},
        {0.0, 3400.0, 6800.0},
        Pattern::orthogonal({4000.0, 4000.0, 4000.0}, {4000.0, 4000.0}),
        Framing{
            .system = System::PostAndBeam,
            .node = Node::Head,
            .span = -1,
            .deck = 180.0,
            .deck_flush = true,
            .reach = 220.0,
        }
    );
    verify_grid("inline_heads_five_bay", Grid(inline_head_stack));

    // 8. Shifted (`from_levels`) and morphed (`morph`: tapered & twisted) stacks (`0.000 mm3` clash).
    const Polyline sq_fp = Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 12000.0, 12000.0);
    const Point sq_pivot(6000.0, 6000.0, 0.0);
    const Pattern sq_pat = Pattern::orthogonal({6000.0, 6000.0}, {6000.0, 6000.0});
    const Pattern shift_pat =
        Pattern::orthogonal(std::vector<double>(6, 3000.0), std::vector<double>(6, 3000.0));
    const Framing shift_frm{.system = System::PostAndBeam, .deck = 160.0};
    auto shift_lvl = [&](double z, double dx, double dy) {
        return Stack::Level{
            Plan::from_footprint(
                {sq_fp.transformed(Xform::translation(dx, dy, 0.0))},
                shift_pat,
                {},
                Plane(Point(0.0, 0.0, z), X, Y)
            ),
            shift_frm,
        };
    };
    verify_grid(
        "shifted_stack",
        Grid(Stack::from_levels({
            shift_lvl(3600.0, 0.0, 0.0),
            shift_lvl(7200.0, 3000.0, 0.0),
            shift_lvl(10800.0, 3000.0, 3000.0),
        }))
    );
    verify_grid(
        "tapered_stack",
        Grid(Stack::morph(
            {sq_fp},
            {0.0, 3800.0, 7600.0, 11400.0},
            sq_pat,
            [sq_pivot](double t) {
                const double s = 1.0 - 0.35 * t;
                return Xform::scale_non_uniform(sq_pivot, s, s, 1.0);
            },
            Framing{
                .system = System::GirderPurlin,
                .node = Node::Through,
                .spacing = 2200.0,
                .deck = 160.0,
            }
        ))
    );
    verify_grid(
        "twisted_stack",
        Grid(Stack::morph(
            {sq_fp},
            {0.0, 3800.0, 7600.0, 11400.0},
            sq_pat,
            [sq_pivot](double t) {
                return Xform::translation(sq_pivot[0], sq_pivot[1], 0.0) *
                       Xform::rotation_z(36.0 * t, true) *
                       Xform::translation(-sq_pivot[0], -sq_pivot[1], 0.0);
            },
            Framing{
                .system = System::PostAndBeam,
                .node = Node::Through,
                .deck = 160.0,
                .profiles = {.column = profile_round(300.0)},
            }
        ))
    );
    const Polyline flatiron_fp({
        Point(0.0, 0.0, 0.0),
        Point(6000.0, 6000.0, 0.0),
        Point(18000.0, 6000.0, 0.0),
        Point(18000.0, 12000.0, 0.0),
        Point(0.0, 12000.0, 0.0),
        Point(0.0, 0.0, 0.0),
    });
    verify_grid(
        "oblique_flatiron_stack",
        Grid(Stack::from_footprint(
            {flatiron_fp},
            {0.0, 3800.0, 7600.0},
            Pattern::orthogonal({6000.0, 6000.0, 6000.0}, {6000.0, 6000.0}),
            Framing{
                .system = System::GirderPurlin,
                .node = Node::Flush,
                .spacing = 2000.0,
                .deck = 160.0,
            }
        ))
    );

    // 9. Developed two-layer modulated fin screen (`System::StackedPurlin` on vertical Plane).
    const Plan fin_plan = Plan::facade(
        Line::from_points(Point(0.0, 0.0, 0.0), Point(9000.0, 0.0, 0.0)),
        7200.0,
        {950.0, 680.0, 1240.0, 880.0, 750.0, 950.0, 680.0, 1240.0, 880.0, 750.0},
        {3600.0, 3600.0}
    );
    verify_grid(
        "stammhaus_fin_screen",
        Grid(
            fin_plan,
            Framing{
                .system = System::StackedPurlin,
                .span = 0,
                .height = 0.0,
                .spacing = 100000.0,
                .profiles =
                    {.girder = profile_rectangle(760.0, 180.0),
                     .purlin = profile_rectangle(95.0, 320.0)},
            }
        )
    );

    std::cout << "grid checks passed" << std::endl;
    return 0;
    } catch (const std::exception& e) {
        std::cout << "FAILED: " << e.what() << std::endl;
        return 1;
    }
}
