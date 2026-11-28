#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

namespace {

Mesh box_mesh(const Point& c, double hx, double hy, double hz) {

    const Polyline loop({
        Point(c[0] - hx, c[1] - hy, c[2] - hz),
        Point(c[0] + hx, c[1] - hy, c[2] - hz),
        Point(c[0] + hx, c[1] + hy, c[2] - hz),
        Point(c[0] - hx, c[1] + hy, c[2] - hz),
        Point(c[0] - hx, c[1] - hy, c[2] - hz),
    });
    return Mesh::loft({loop}, {loop.translated(Vector(0.0, 0.0, 2.0 * hz))}, true);
}

void subtract_box(
    WoodSession& scene,
    const std::shared_ptr<Element>& target,
    const Mesh& cutter,
    const Vector& way_in,
    const std::string& name,
    const std::shared_ptr<TreeNode>& group
) {
    const std::shared_ptr<JointElement> cut =
        std::make_shared<JointElement>(cutter, SolidOperation::subtract);
    cut->name = name;
    cut->is_visible = false;
    cut->target_insertions = {{way_in}};
    scene.add(cut, group);
    scene.add_interaction(cut, target, cut->interaction(0));
}

std::shared_ptr<JointBeamElement> make_binary_box_joint(
    const Element& a,
    const Element& b,
    const Mesh& cut_a,
    const Mesh& cut_b,
    const Vector& in_a,
    const Vector& in_b,
    const std::string& name
) {
    const std::shared_ptr<JointBeamElement> joint = std::make_shared<JointBeamElement>();
    joint->name = name;
    joint->is_visible = false;
    joint->targets = {a.guid(), b.guid()};
    joint->target_insertions = {{in_a}, {in_b}};

    JointCutStep sa;
    sa.label = "A2";
    sa.target = 0;
    sa.removal = cut_a;

    JointCutStep sb;
    sb.label = "B2";
    sb.target = 1;
    sb.removal = cut_b;

    joint->steps = {sa, sb};
    return joint;
}

/// Builds an explicit world-+Z oriented flared half-dovetail lap at `corner` (`u_tenon` points out of the tenon bar's end into the carrier).
/// When `tenon_drops_from_top` is true, `tenon_bar` keeps its upper flared dovetail (`z >= z_c`) and drops along `-Z` into `carrier_bar`'s top socket.
/// When `tenon_drops_from_top` is false (the 4th closure corner!), `tenon_bar` keeps an upward-standing flared dovetail (`z >= z_c`) and `carrier_bar` drops along `-Z` over it via an underside socket!
void add_upward_half_dovetail_corner(
    WoodSession& scene,
    const std::shared_ptr<EdgeRawElement>& tenon_bar,
    const std::shared_ptr<EdgeRawElement>& carrier_bar,
    const Point& corner,
    const Vector& u_tenon,
    bool tenon_drops_from_top,
    const std::string& prefix,
    const std::shared_ptr<TreeNode>& group
) {
    const Vector v_perp(-u_tenon[1], u_tenon[0], 0.0);
    const double zc = corner[2];
    const double z_split = tenon_drops_from_top ? (zc + 1.0) : (zc - 1.0);

    // Flared dovetail cheeks in XY: neck half-width 17 mm at corner - u_tenon*48 widening to 26 mm at corner + u_tenon*30
    for (int side : {-1, 1}) {
        const double s = static_cast<double>(side);
        const Point p_neck = corner - u_tenon * 48.0 + v_perp * (s * 17.0);
        const Point p_head = corner + u_tenon * 32.0 + v_perp * (s * 26.0);
        const Point p_out1 = corner + u_tenon * 32.0 + v_perp * (s * 105.0);
        const Point p_out0 = corner - u_tenon * 48.0 + v_perp * (s * 105.0);
        std::vector<Point> ring = {p_neck, p_head, p_out1, p_out0};
        if (side > 0)
            std::reverse(ring.begin(), ring.end());
        for (Point& pt : ring)
            pt[2] = tenon_drops_from_top ? (zc - 2.0) : (zc - 104.0);
        ring.push_back(ring.front());
        subtract_box(
            scene,
            tenon_bar,
            Mesh::loft({Polyline(ring)}, {Polyline(ring).translated(Vector(0.0, 0.0, 106.0))}, true),
            Vector(0.0, 0.0, tenon_drops_from_top ? -1.0 : 1.0),
            fmt::format("{}_cheek_{}", prefix, side < 0 ? "n" : "p"),
            group
        );
    }

    // Tenon bar loses its lower half when dropping from top (z < z_split), or upper half when staying below (z > z_split)
    const double cut_z0 = tenon_drops_from_top ? (z_split - 105.0) : z_split;
    const Polyline half_ring({
        corner - u_tenon * 50.0 - v_perp * 105.0 + Vector(0.0, 0.0, cut_z0 - zc),
        corner + u_tenon * 34.0 - v_perp * 105.0 + Vector(0.0, 0.0, cut_z0 - zc),
        corner + u_tenon * 34.0 + v_perp * 105.0 + Vector(0.0, 0.0, cut_z0 - zc),
        corner - u_tenon * 50.0 + v_perp * 105.0 + Vector(0.0, 0.0, cut_z0 - zc),
        corner - u_tenon * 50.0 - v_perp * 105.0 + Vector(0.0, 0.0, cut_z0 - zc),
    });
    const Mesh cut_tenon_half =
        Mesh::loft({half_ring}, {half_ring.translated(Vector(0.0, 0.0, 105.0))}, true);

    if (tenon_drops_from_top) {
        // Carrier bar loses an upward-opening flared dovetail pocket (z >= zc - 0.5) + shoulder relief so tenon_bar drops along -Z!
        const std::vector<Point> sock = {
            corner - u_tenon * 64.0 - v_perp * 18.8 + Vector(0.0, 0.0, -0.5),
            corner + u_tenon * 30.5 - v_perp * 27.5 + Vector(0.0, 0.0, -0.5),
            corner + u_tenon * 30.5 + v_perp * 27.5 + Vector(0.0, 0.0, -0.5),
            corner - u_tenon * 64.0 + v_perp * 18.8 + Vector(0.0, 0.0, -0.5),
            corner - u_tenon * 64.0 - v_perp * 18.8 + Vector(0.0, 0.0, -0.5),
        };
        const Mesh cut_carrier_top =
            Mesh::loft({Polyline(sock)}, {Polyline(sock).translated(Vector(0.0, 0.0, 105.0))}, true);

        // Spot-face inner flank of carrier_bar where tenon_bar enters so irregular bark never snags during vertical -Z drop!
        const Point flank_c = corner - u_tenon * 54.0;
        const Polyline flank_ring({
            flank_c - u_tenon * 24.0 - v_perp * 105.0 - Vector(0.0, 0.0, 105.0),
            flank_c + u_tenon * 10.0 - v_perp * 105.0 - Vector(0.0, 0.0, 105.0),
            flank_c + u_tenon * 10.0 + v_perp * 105.0 - Vector(0.0, 0.0, 105.0),
            flank_c - u_tenon * 24.0 + v_perp * 105.0 - Vector(0.0, 0.0, 105.0),
            flank_c - u_tenon * 24.0 - v_perp * 105.0 - Vector(0.0, 0.0, 105.0),
        });
        subtract_box(
            scene,
            carrier_bar,
            Mesh::loft({flank_ring}, {flank_ring.translated(Vector(0.0, 0.0, 210.0))}, true),
            Vector(0.0, 0.0, 1.0),
            fmt::format("{}_flank_relief", prefix),
            group
        );

        const std::shared_ptr<JointBeamElement> dt = make_binary_box_joint(
            *carrier_bar,
            *tenon_bar,
            cut_carrier_top,
            cut_tenon_half,
            Vector(0.0, 0.0, 1.0),
            Vector(0.0, 0.0, -1.0),
            prefix
        );
        scene.add(dt, group);
        scene.add_interaction(dt, carrier_bar, dt->interaction(0));
        scene.add_interaction(dt, tenon_bar, dt->interaction(1));
    } else {
        // Closing 4th corner: tenon_bar (fan_3) stays below; carrier_bar (fan_0) loses an UNDERSIDE socket (z <= zc + 0.5) so fan_0 drops along -Z!
        const std::vector<Point> sock = {
            corner - u_tenon * 64.0 - v_perp * 18.8 + Vector(0.0, 0.0, -105.0),
            corner + u_tenon * 30.5 - v_perp * 27.5 + Vector(0.0, 0.0, -105.0),
            corner + u_tenon * 30.5 + v_perp * 27.5 + Vector(0.0, 0.0, -105.0),
            corner - u_tenon * 64.0 + v_perp * 18.8 + Vector(0.0, 0.0, -105.0),
            corner - u_tenon * 64.0 - v_perp * 18.8 + Vector(0.0, 0.0, -105.0),
        };
        const Mesh cut_carrier_bot =
            Mesh::loft({Polyline(sock)}, {Polyline(sock).translated(Vector(0.0, 0.0, 105.5))}, true);

        const Point flank_c = corner - u_tenon * 54.0;
        const Polyline flank_ring({
            flank_c - u_tenon * 24.0 - v_perp * 105.0 - Vector(0.0, 0.0, 105.0),
            flank_c + u_tenon * 10.0 - v_perp * 105.0 - Vector(0.0, 0.0, 105.0),
            flank_c + u_tenon * 10.0 + v_perp * 105.0 - Vector(0.0, 0.0, 105.0),
            flank_c - u_tenon * 24.0 + v_perp * 105.0 - Vector(0.0, 0.0, 105.0),
            flank_c - u_tenon * 24.0 - v_perp * 105.0 - Vector(0.0, 0.0, 105.0),
        });
        subtract_box(
            scene,
            carrier_bar,
            Mesh::loft({flank_ring}, {flank_ring.translated(Vector(0.0, 0.0, 105.5))}, true),
            Vector(0.0, 0.0, -1.0),
            fmt::format("{}_flank_relief", prefix),
            group
        );

        const std::shared_ptr<JointBeamElement> dt = make_binary_box_joint(
            *tenon_bar,
            *carrier_bar,
            cut_tenon_half,
            cut_carrier_bot,
            Vector(0.0, 0.0, 1.0),
            Vector(0.0, 0.0, -1.0),
            prefix
        );
        scene.add(dt, group);
        scene.add_interaction(dt, tenon_bar, dt->interaction(0));
        scene.add_interaction(dt, carrier_bar, dt->interaction(1));
    }
}

} // namespace

/// Side-to-top reciprocal raw-wood systems (Thesis Fig. 5.45, 5.46, 5.47, 9.8): oriented half-dovetail pinwheel, dowel-nut T-joints, serrated fingers, and recessed roof panels.
int main() {

    WoodSession scene("element_edge_raw_side_top_nexorade");

    const std::array<std::string, 4> stocks = {"raw_log_0", "raw_log_1", "raw_log_2", "raw_log_3"};

    // 1. Left: Four-bar side-to-top reciprocal pinwheel with upward-oriented half-dovetails (Thesis Fig. 5.45, 5.46)
    const std::shared_ptr<TreeNode> fan_group = scene.group_named("half_dovetail_pinwheel");
    const double z_fan = 110.0;
    const double w = 215.0;
    const double tail = 485.0;

    const std::shared_ptr<EdgeRawElement> fan_3 = EdgeRawElement::from_stock(
        Line::from_points({tail + 20.0, -w, z_fan}, {-w - 40.0, -w, z_fan}),
        50.0,
        stocks[3],
        20.0,
        true,
        "pinwheel_fan_3"
    );
    fan_3->cuts = {
        Plane::from_point_normal({tail, -w, z_fan}, {-1.0, 0.0, 0.0}),
        Plane::from_point_normal({-w - 28.0, -w, z_fan}, {1.0, 0.0, 0.0}),
    };

    const std::shared_ptr<EdgeRawElement> fan_2 = EdgeRawElement::from_stock(
        Line::from_points({w, tail + 20.0, z_fan}, {w, -w - 40.0, z_fan}),
        50.0,
        stocks[2],
        75.0,
        true,
        "pinwheel_fan_2"
    );
    fan_2->cuts = {
        Plane::from_point_normal({w, tail, z_fan}, {0.0, -1.0, 0.0}),
        Plane::from_point_normal({w, -w - 28.0, z_fan}, {0.0, 1.0, 0.0}),
    };

    const std::shared_ptr<EdgeRawElement> fan_1 = EdgeRawElement::from_stock(
        Line::from_points({-tail - 20.0, w, z_fan}, {w + 40.0, w, z_fan}),
        50.0,
        stocks[1],
        130.0,
        true,
        "pinwheel_fan_1"
    );
    fan_1->cuts = {
        Plane::from_point_normal({-tail, w, z_fan}, {1.0, 0.0, 0.0}),
        Plane::from_point_normal({w + 28.0, w, z_fan}, {-1.0, 0.0, 0.0}),
    };

    const std::shared_ptr<EdgeRawElement> fan_0 = EdgeRawElement::from_stock(
        Line::from_points({-w, -tail - 20.0, z_fan}, {-w, w + 40.0, z_fan}),
        50.0,
        stocks[0],
        190.0,
        true,
        "pinwheel_fan_0"
    );
    fan_0->cuts = {
        Plane::from_point_normal({-w, -tail, z_fan}, {0.0, 1.0, 0.0}),
        Plane::from_point_normal({-w, w + 28.0, z_fan}, {0.0, -1.0, 0.0}),
    };

    for (const std::shared_ptr<EdgeRawElement>& m : {fan_3, fan_2, fan_1, fan_0})
        scene.add(m, fan_group);

    // Sequential vertical (-Z) assembly: fan_3 (base) -> fan_2 -> fan_1 -> closing fan_0!
    add_upward_half_dovetail_corner(
        scene, fan_2, fan_3, Point(w, -w, z_fan), Vector(0.0, -1.0, 0.0), true, "pinwheel_dt_2_3", fan_group
    );
    add_upward_half_dovetail_corner(
        scene, fan_1, fan_2, Point(w, w, z_fan), Vector(1.0, 0.0, 0.0), true, "pinwheel_dt_1_2", fan_group
    );
    add_upward_half_dovetail_corner(
        scene, fan_0, fan_1, Point(-w, w, z_fan), Vector(0.0, 1.0, 0.0), true, "pinwheel_dt_0_1", fan_group
    );
    add_upward_half_dovetail_corner(
        scene, fan_3, fan_0, Point(-w, -w, z_fan), Vector(-1.0, 0.0, 0.0), false, "pinwheel_dt_3_0", fan_group
    );

    const std::array<Point, 4> corners = {{
        Point(-w, -w, z_fan),
        Point(-w, w, z_fan),
        Point(w, w, z_fan),
        Point(w, -w, z_fan),
    }};
    const std::array<std::pair<std::shared_ptr<EdgeRawElement>, std::shared_ptr<EdgeRawElement>>, 4>
        corner_members = {{{fan_3, fan_0}, {fan_0, fan_1}, {fan_1, fan_2}, {fan_2, fan_3}}};
    for (size_t c = 0; c < 4; c++) {
        const Line pin_axis = Line::from_points(
            corners[c] + Vector(0.0, 0.0, -52.0), corners[c] + Vector(0.0, 0.0, 52.0)
        );
        const std::shared_ptr<JointPinElement> pin =
            std::make_shared<JointPinElement>(pin_axis, 8.0, 0.04);
        pin->name = fmt::format("pinwheel_pin_{}", c);
        scene.add(pin, fan_group);

        Line bore_axis = pin_axis;
        bore_axis.extend_equally(30.0);
        for (size_t m = 0; m < 2; m++) {
            const std::shared_ptr<EdgeRawElement>& member =
                m == 0 ? corner_members[c].first : corner_members[c].second;
            const std::shared_ptr<JointElement> bore = JointElement::drill(bore_axis, 8.0);
            bore->name = fmt::format("pinwheel_bore_{}_{}", c, m);
            bore->is_visible = false;
            bore->target_insertions = {{{0.0, 0.0, -1.0}}};
            scene.add(bore, fan_group);
            scene.add_interaction(bore, member, bore->interaction(0));
        }
    }

    // 2. Right: Panelled raw-wood frame with Dowel-Nut T-joints (Fig. 5.47) and Serrated Finger pockets (Fig. 9.8)
    const std::shared_ptr<TreeNode> bay_group = scene.group_named("dowel_nut_panel_bay");
    const Vector shift(1520.0, -880.0, 0.0);
    const double z_bay = 110.0;
    const double z_rebate = 144.0;
    const double x_left = -450.0;
    const double x_right = 450.0;
    const double x_cheek = 402.0;

    const std::shared_ptr<EdgeRawElement> girder_left = EdgeRawElement::from_stock(
        Line::from_points(Point(x_left, -540.0, z_bay) + shift, Point(x_left, 540.0, z_bay) + shift),
        56.0,
        "raw_log_0",
        15.0,
        true,
        "bay_girder_left"
    );
    girder_left->cuts = {
        Plane::from_point_normal(Point(x_left, -520.0, z_bay) + shift, {0.0, 1.0, 0.0}),
        Plane::from_point_normal(Point(x_left, 520.0, z_bay) + shift, {0.0, -1.0, 0.0}),
    };
    scene.add(girder_left, bay_group);

    subtract_box(
        scene,
        girder_left,
        box_mesh(Point(x_left + 58.0, 0.0, z_rebate + 48.0) + shift, 58.0, 520.0, 48.0),
        Vector(1.0, 0.0, 0.0),
        "girder_left_rebate",
        bay_group
    );

    const std::array<double, 3> y_joists = {-350.0, 0.0, 350.0};
    std::vector<std::shared_ptr<EdgeRawElement>> joists;
    for (size_t j = 0; j < y_joists.size(); j++) {
        const double yj = y_joists[j];
        const std::shared_ptr<EdgeRawElement> joist = EdgeRawElement::from_stock(
            Line::from_points(
                Point(-x_cheek - 15.0, yj, z_bay) + shift, Point(x_cheek + 40.0, yj, z_bay) + shift
            ),
            52.0,
            stocks[(j + 2) % 4],
            45.0 * static_cast<double>(j),
            true,
            fmt::format("bay_joist_{}", j)
        );
        joist->cuts = {
            Plane::from_point_normal(Point(-x_cheek + 1.0, yj, z_bay) + shift, {1.0, 0.0, 0.0}),
            Plane::from_point_normal(Point(x_cheek + 22.0, yj, z_bay) + shift, {-1.0, 0.0, 0.0}),
            Plane::from_point_normal(
                Point(0.0, yj, z_rebate + (j == 1 ? 0.0 : 28.0)) + shift, {0.0, 0.0, -1.0}
            ),
        };
        scene.add(joist, bay_group);
        joists.push_back(joist);

        if (j != 1) {
            const double inner_sign = j == 0 ? 1.0 : -1.0;
            subtract_box(
                scene,
                joist,
                box_mesh(Point(0.0, yj + inner_sign * 52.0, z_rebate + 48.0) + shift, 425.0, 58.0, 48.0),
                Vector(-1.0, 0.0, 0.0),
                fmt::format("joist_panel_rebate_{}", j),
                bay_group
            );
        }

        const std::shared_ptr<JointBeamElement> dn_joint = make_binary_box_joint(
            *girder_left,
            *joist,
            box_mesh(Point(-x_cheek + 26.0, yj, z_bay) + shift, 26.0, 95.0, 95.0),
            box_mesh(Point(-x_cheek + 10.0, yj, z_bay - 68.0) + shift, 12.0, 95.0, 30.0),
            Vector(1.0, 0.0, 0.0),
            Vector(-1.0, 0.0, 0.0),
            fmt::format("dowel_nut_joint_{}", j)
        );
        scene.add(dn_joint, bay_group);
        scene.add_interaction(dn_joint, girder_left, dn_joint->interaction(0));
        scene.add_interaction(dn_joint, joist, dn_joint->interaction(1));

        const Line axial_dowel = Line::from_points(
            Point(x_left - 44.0, yj, z_bay) + shift, Point(-x_cheek + 92.0, yj, z_bay) + shift
        );
        const std::shared_ptr<JointPinElement> h_pin =
            std::make_shared<JointPinElement>(axial_dowel, 7.5, 0.04);
        h_pin->name = fmt::format("dowel_nut_axial_pin_{}", j);
        scene.add(h_pin, bay_group);

        Line h_bore = axial_dowel;
        h_bore.extend_equally(25.0);
        for (size_t m = 0; m < 2; m++) {
            const std::shared_ptr<EdgeRawElement>& target = m == 0 ? girder_left : joist;
            const std::shared_ptr<JointElement> bore = JointElement::drill(h_bore, 7.5);
            bore->name = fmt::format("dowel_nut_axial_bore_{}_{}", j, m);
            bore->is_visible = false;
            bore->target_insertions = {{{m == 0 ? 1.0 : -1.0, 0.0, 0.0}}};
            scene.add(bore, bay_group);
            scene.add_interaction(bore, target, bore->interaction(0));
        }

        const Line nut_axis = Line::from_points(
            Point(-x_cheek + 62.0, yj, z_bay - 36.0) + shift,
            Point(-x_cheek + 62.0, yj, z_rebate + 6.0) + shift
        );
        const std::shared_ptr<JointPinElement> nut_pin =
            std::make_shared<JointPinElement>(nut_axis, 12.0, 0.03);
        nut_pin->name = fmt::format("dowel_nut_cross_plug_{}", j);
        scene.add(nut_pin, bay_group);

        Line nut_bore = nut_axis;
        nut_bore.extend_equally(30.0);
        const std::shared_ptr<JointElement> jb = JointElement::drill(nut_bore, 12.0);
        jb->name = fmt::format("dowel_nut_cross_bore_{}", j);
        jb->is_visible = false;
        jb->target_insertions = {{{-1.0, 0.0, 0.0}}};
        scene.add(jb, bay_group);
        scene.add_interaction(jb, joist, jb->interaction(0));
    }

    const std::shared_ptr<EdgeRawElement> girder_right = EdgeRawElement::from_stock(
        Line::from_points(Point(x_right, -540.0, z_bay) + shift, Point(x_right, 540.0, z_bay) + shift),
        56.0,
        "raw_log_1",
        95.0,
        true,
        "bay_girder_right"
    );
    girder_right->cuts = {
        Plane::from_point_normal(Point(x_right, -520.0, z_bay) + shift, {0.0, 1.0, 0.0}),
        Plane::from_point_normal(Point(x_right, 520.0, z_bay) + shift, {0.0, -1.0, 0.0}),
    };
    scene.add(girder_right, bay_group);

    subtract_box(
        scene,
        girder_right,
        box_mesh(Point(x_right - 58.0, 0.0, z_rebate + 48.0) + shift, 58.0, 520.0, 48.0),
        Vector(-1.0, 0.0, 0.0),
        "girder_right_rebate",
        bay_group
    );

    for (size_t j = 0; j < joists.size(); j++) {
        const double yj = y_joists[j];
        // Carve 3 clean horizontal finger tenons at yj - 22, yj, yj + 22 (each 10 mm wide, z in [z_bay - 24, z_rebate])
        subtract_box(
            scene,
            joists[j],
            box_mesh(Point(x_cheek + 14.0, yj - 11.0, z_bay) + shift, 15.0, 6.0, 95.0),
            Vector(1.0, 0.0, 0.0),
            fmt::format("joist_fslot_a_{}", j),
            bay_group
        );
        subtract_box(
            scene,
            joists[j],
            box_mesh(Point(x_cheek + 14.0, yj + 11.0, z_bay) + shift, 15.0, 6.0, 95.0),
            Vector(1.0, 0.0, 0.0),
            fmt::format("joist_fslot_b_{}", j),
            bay_group
        );
        subtract_box(
            scene,
            joists[j],
            box_mesh(Point(x_cheek + 14.0, yj - 65.0, z_bay) + shift, 15.0, 38.0, 95.0),
            Vector(1.0, 0.0, 0.0),
            fmt::format("joist_fcheek_a_{}", j),
            bay_group
        );
        subtract_box(
            scene,
            joists[j],
            box_mesh(Point(x_cheek + 14.0, yj + 65.0, z_bay) + shift, 15.0, 38.0, 95.0),
            Vector(1.0, 0.0, 0.0),
            fmt::format("joist_fcheek_b_{}", j),
            bay_group
        );

        for (int f_idx = -1; f_idx <= 1; f_idx++) {
            subtract_box(
                scene,
                girder_right,
                box_mesh(
                    Point(x_cheek + 11.0, yj + f_idx * 22.0, z_bay + 10.0) + shift,
                    12.5,
                    6.8,
                    42.0
                ),
                Vector(-1.0, 0.0, 0.0),
                fmt::format("girder_fpocket_{}_{}", j, f_idx + 1),
                bay_group
            );
        }

        const std::shared_ptr<JointBeamElement> serrated = make_binary_box_joint(
            *joists[j],
            *girder_right,
            box_mesh(Point(x_cheek + 14.0, yj, z_bay - 69.0) + shift, 15.0, 95.0, 45.0),
            box_mesh(Point(x_cheek - 32.0, yj, z_bay) + shift, 33.0, 95.0, 95.0),
            Vector(1.0, 0.0, 0.0),
            Vector(-1.0, 0.0, 0.0),
            fmt::format("serrated_finger_joint_{}", j)
        );
        scene.add(serrated, bay_group);
        scene.add_interaction(serrated, joists[j], serrated->interaction(0));
        scene.add_interaction(serrated, girder_right, serrated->interaction(1));

        const Line f_pin_axis = Line::from_points(
            Point(x_cheek + 11.0, yj, z_bay - 32.0) + shift,
            Point(x_cheek + 11.0, yj, z_rebate + 18.0) + shift
        );
        const std::shared_ptr<JointPinElement> f_pin =
            std::make_shared<JointPinElement>(f_pin_axis, 6.5, 0.04);
        f_pin->name = fmt::format("serrated_pin_{}", j);
        scene.add(f_pin, bay_group);

        Line f_bore = f_pin_axis;
        f_bore.extend_equally(28.0);
        for (size_t m = 0; m < 2; m++) {
            const std::shared_ptr<EdgeRawElement>& target = m == 0 ? joists[j] : girder_right;
            const std::shared_ptr<JointElement> bore = JointElement::drill(f_bore, 6.5);
            bore->name = fmt::format("serrated_bore_{}_{}", j, m);
            bore->is_visible = false;
            bore->target_insertions = {{{m == 0 ? 1.0 : -1.0, 0.0, 0.0}}};
            scene.add(bore, bay_group);
            scene.add_interaction(bore, target, bore->interaction(0));
        }
    }

    // Two recessed planar timber cladding panels seated flush into the surfaced rebates (1.5 mm perimeter clearance)
    for (size_t p = 0; p < 2; p++) {
        const double yc = p == 0 ? -175.0 : 175.0;
        const double px = p == 0 ? -180.0 : 180.0;
        const double py = p == 0 ? -18.0 : 18.0;
        const Polyline panel_loop = Polyline::rectangle(
            Point(-409.0, yc - 168.0, z_rebate + 1.2) + shift, Vector::x_axis(), Vector::y_axis(), 818.0, 336.0
        );
        const std::shared_ptr<FacePlanarElement> panel = std::make_shared<FacePlanarElement>(
            panel_loop,
            panel_loop.translated(Vector(0.0, 0.0, 26.8)),
            fmt::format("recessed_panel_{}", p)
        );
        scene.add(panel, bay_group);

        const std::shared_ptr<JointBeamElement> p_seat = make_binary_box_joint(
            *joists[1],
            *panel,
            box_mesh(Point(px, py, z_rebate + 2.0) + shift, 14.0, 10.0, 4.0),
            box_mesh(Point(-px, py, z_rebate + 0.5) + shift, 14.0, 10.0, 3.5),
            Vector(0.0, 0.0, 1.0),
            Vector(0.0, 0.0, -1.0),
            fmt::format("panel_seat_{}", p)
        );
        scene.add(p_seat, bay_group);
        scene.add_interaction(p_seat, joists[1], p_seat->interaction(0));
        scene.add_interaction(p_seat, panel, p_seat->interaction(1));

        const Line p_axis = Line::from_points(
            Point(px, py, z_bay + 10.0) + shift, Point(px, py, z_rebate + 28.0) + shift
        );
        const std::shared_ptr<JointPinElement> ppin =
            std::make_shared<JointPinElement>(p_axis, 6.5, 0.03);
        ppin->name = fmt::format("panel_pin_{}", p);
        scene.add(ppin, bay_group);

        Line pbore = p_axis;
        pbore.extend_equally(20.0);
        for (size_t m = 0; m < 2; m++) {
            const std::shared_ptr<Element> target =
                m == 0 ? std::static_pointer_cast<Element>(joists[1])
                       : std::static_pointer_cast<Element>(panel);
            const std::shared_ptr<JointElement> bore = JointElement::drill(pbore, 6.5);
            bore->name = fmt::format("panel_bore_{}_{}", p, m);
            bore->is_visible = false;
            bore->target_insertions = {{{0.0, 0.0, -1.0}}};
            scene.add(bore, bay_group);
            scene.add_interaction(bore, target, bore->interaction(0));
        }
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Side-to-top reciprocal raw-wood systems (Thesis Fig. 5.45, 5.46, 5.47 and 9.8): a four-log cyclic reciprocal pinwheel with upward-oriented stopped half-dovetail joints and a collision-free closing corner (left), beside a panelled raw-wood roof bay combining Dowel-Nut T-joints (axial dowel anchored by an intersecting vertical dowel-nut), 3-tooth serrated finger pockets, and recessed planar timber cladding panels seated into surfaced log rebates (right).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_side_top_nexorade --parallel 6 && ./build/element_edge_raw_side_top_nexorade && ../bash/publish-scene.sh --target element_edge_raw_side_top_nexorade

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
