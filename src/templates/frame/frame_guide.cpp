#include "pch.h"
#include "src/templates/frame/frame_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_frame {

namespace {

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const Vector Z(0.0, 0.0, 1.0);

Polyline close_pts(std::vector<Point> pts) {
    if (!pts.empty() && pts.front().distance(pts.back()) >= 1e-6)
        pts.push_back(pts.front());
    return Polyline(pts);
}

/// Cross-section rectangle in a vertical XZ bent at `y_c` (`width` along Y): `x` from `x_lo` to `x_hi` at elevation `z`.
Polyline leg_section_xz(double x_lo, double x_hi, double y_c, double width, double z) {
    const double hw = 0.5 * width;
    return close_pts({
        Point(x_lo, y_c - hw, z),
        Point(x_hi, y_c - hw, z),
        Point(x_hi, y_c + hw, z),
        Point(x_lo, y_c + hw, z),
    });
}

/// Cross-section rectangle of a rafter/girder running along +X in a vertical XZ bent at `y_c`: `[z_bot, z_top]` at station `x`.
Polyline rafter_section_xz(double x, double y_c, double width, double z_bot, double z_top) {
    const double hw = 0.5 * width;
    return close_pts({
        Point(x, y_c + hw, z_bot),
        Point(x, y_c - hw, z_bot),
        Point(x, y_c - hw, z_top),
        Point(x, y_c + hw, z_top),
    });
}

MemberSpec make_variable_member(
    const std::vector<Polyline>& sections,
    const std::vector<Plane>& cuts,
    const std::string& family,
    int stage,
    const std::vector<Mesh>& subtractions = {}
) {
    MemberSpec spec;
    spec.sections = sections;
    spec.axis = Line::from_points(
        Point::centroid(sections.front().open_points()),
        Point::centroid(sections.back().open_points())
    );
    spec.cuts = cuts;
    spec.subtractions = subtractions;
    spec.family = family;
    spec.stage = stage;
    return spec;
}

MemberSpec make_straight_member(
    const Point& a,
    const Point& b,
    double width,
    double depth,
    const Vector& up,
    const std::vector<Plane>& cuts,
    const std::string& family,
    int stage
) {
    MemberSpec spec;
    spec.axis = Line::from_points(a, b);
    spec.sections = profile_rectangle(width, depth);
    spec.up = up;
    spec.cuts = cuts;
    spec.family = family;
    spec.stage = stage;
    return spec;
}

SupportSpec make_shoe(double x, double y, double z, double col_min) {
    SupportSpec s;
    s.plane = Plane(Point(x, y, z), X, Y);
    s.base_size = std::clamp(col_min * 0.85, 140.0, 220.0);
    s.head_diameter = std::clamp(col_min * 0.70, 100.0, 160.0);
    s.height = 110.0;
    return s;
}

struct KneeMiter {
    Plane keep_leg;
    Plane keep_rafter;
};

/// Exact coplanar knee miter between a tapered outer portal leg (`side = -1` left, `+1` right) and a flat or pitched haunched rafter.
KneeMiter compute_portal_knee_miter(
    double x_out,
    double y_c,
    double side,
    double z_foot,
    double z_eaves,
    double d_foot,
    double d_knee,
    double d_raf_0,
    double d_raf_h,
    double s_haunch,
    double slope
) {
    const double inward = -side; // +1 for left leg, -1 for right leg
    const double h_leg = std::max(100.0, z_eaves - z_foot);
    const double alpha = (d_knee - d_foot) / h_leg;
    const double k_raf = slope - (d_raf_h - d_raf_0) / std::max(100.0, s_haunch);
    const double denom = std::max(0.15, 1.0 - alpha * k_raf);
    const double s_in = (d_knee - alpha * d_raf_0) / denom;
    const double z_in = z_eaves - d_raf_0 + k_raf * s_in;

    const Point k_out(x_out, y_c, z_eaves);
    const Vector keep_rafter = Vector(inward * (z_eaves - z_in), 0.0, s_in).normalized();
    return KneeMiter{
        Plane::from_point_normal(k_out, -keep_rafter),
        Plane::from_point_normal(k_out, keep_rafter),
    };
}

/// Builds a tapered outer portal leg (`side = -1` left at `x_out`, `+1` right at `x_out`) sliced at the top by `keep_leg`.
MemberSpec build_outer_leg(
    double x_out,
    double y_c,
    double width,
    double side,
    double z_foot,
    double z_eaves,
    double d_foot,
    double d_knee,
    const Plane& keep_leg,
    int stage
) {
    const double inward = -side;
    const double h_leg = std::max(100.0, z_eaves - z_foot);
    const double z_top_ext = z_eaves + 80.0;
    const double d_ext = d_foot + (d_knee - d_foot) * (z_top_ext - z_foot) / h_leg;

    auto sec_at = [&](double z, double d) {
        const double x0 = side < 0.0 ? x_out : (x_out - d);
        const double x1 = side < 0.0 ? (x_out + d) : x_out;
        return leg_section_xz(x0, x1, y_c, width, z);
    };
    const Plane bot_cut = Plane::from_point_normal(Point(x_out + inward * 0.5 * d_foot, y_c, z_foot), Z);
    return make_variable_member(
        {sec_at(z_foot - 20.0, d_foot), sec_at(z_top_ext, d_ext)},
        {bot_cut, keep_leg},
        "column",
        stage
    );
}

/// Builds a symmetric centered tapered column at `x_c` stopping squarely at `z_top`.
MemberSpec build_centered_taper_column(
    double x_c,
    double y_c,
    double width,
    double z_foot,
    double z_top,
    double d_foot,
    double d_top,
    int stage
) {
    return make_variable_member(
        {
            leg_section_xz(x_c - 0.5 * d_foot, x_c + 0.5 * d_foot, y_c, width, z_foot),
            leg_section_xz(x_c - 0.5 * d_top, x_c + 0.5 * d_top, y_c, width, z_top),
        },
        {},
        "column",
        stage
    );
}

/// Sloped half-lap cutter & end plane pair in XZ at `(x_hinge, y_c)` (`into_drop_x = +1` or `-1`).
std::pair<Plane, Mesh> make_xz_gerber_half_lap(
    double x_hinge,
    double y_c,
    double width,
    double z_top,
    double d_hinge,
    double lap_length,
    double into_drop_x,
    bool is_anchor
) {
    const double hl = 0.5 * lap_length;
    const double h_half = 0.5 * d_hinge;
    const double splay = std::min(22.0, 0.25 * hl);
    const double pad = 25.0;
    const double hw = 0.5 * width + pad;
    const double z_mid = z_top - h_half;
    const double z_bot = z_top - d_hinge;

    auto quad_z = [&](double xa, double xb, double z) {
        const double x_lo = std::min(xa, xb);
        const double x_hi = std::max(xa, xb);
        return close_pts({
            Point(x_lo, y_c - hw, z),
            Point(x_hi, y_c - hw, z),
            Point(x_hi, y_c + hw, z),
            Point(x_lo, y_c + hw, z),
        });
    };

    if (is_anchor) {
        const Point p_mid(x_hinge + into_drop_x * (hl - splay), y_c, z_mid);
        const Vector keep_n = Vector(-into_drop_x * h_half, 0.0, -splay).normalized();
        const double b0 = x_hinge + into_drop_x * (-hl + splay);
        const double b1 = x_hinge + into_drop_x * (hl + pad);
        const double t0 = x_hinge + into_drop_x * (-hl - splay * pad / h_half);
        const double t1 = x_hinge + into_drop_x * (hl + pad);
        return {
            Plane::from_point_normal(p_mid, keep_n),
            Mesh::loft({quad_z(b0, b1, z_mid)}, {quad_z(t0, t1, z_top + pad)}),
        };
    }

    const Point p_mid(x_hinge + into_drop_x * (-hl + splay), y_c, z_mid);
    const Vector keep_n = Vector(into_drop_x * h_half, 0.0, splay).normalized();
    const double b0 = x_hinge - into_drop_x * (hl + pad);
    const double b1 = x_hinge + into_drop_x * (hl + splay * pad / h_half);
    const double t0 = x_hinge - into_drop_x * (hl + pad);
    const double t1 = x_hinge + into_drop_x * (hl - splay);
    return {
        Plane::from_point_normal(p_mid, keep_n),
        Mesh::loft({quad_z(b0, b1, z_bot - pad)}, {quad_z(t0, t1, z_mid)}),
    };
}

std::vector<double> symmetric_gradated_bays(double total, int count, double end_ratio) {
    const int n = std::max(1, count);
    if (n == 1)
        return {total};
    std::vector<double> w(static_cast<size_t>(n), 1.0);
    double sum = 0.0;
    const double mid = 0.5 * (n - 1);
    for (int i = 0; i < n; ++i) {
        const double dist = mid > 0.0 ? std::abs(i - mid) / mid : 0.0;
        w[static_cast<size_t>(i)] = 1.0 + (end_ratio - 1.0) * dist;
        sum += w[static_cast<size_t>(i)];
    }
    for (double& val : w)
        val *= total / sum;
    return w;
}

/// Appends a multi-panel Vierendeel truss running from `origin` along horizontal unit `along` (`length`), `z0 .. z1`.
void append_vierendeel_elevation(
    std::vector<MemberSpec>& members,
    const Point& origin,
    const Vector& along,
    double length,
    double thickness,
    double z0,
    double z1,
    double chord_depth,
    const std::vector<double>& post_centers,
    const std::vector<double>& post_widths,
    int stage_base
) {
    const Point b_start(origin[0], origin[1], z0 + 0.5 * chord_depth);
    const Point b_end = b_start + along * length;
    members.push_back(
        make_straight_member(b_start, b_end, thickness, chord_depth, Z, {}, "chord_bottom", stage_base)
    );

    const double z_post_0 = z0 + chord_depth;
    const double z_post_1 = z1 - chord_depth;
    const Vector across(-along[1], along[0], 0.0);
    for (size_t i = 0; i < post_centers.size(); ++i) {
        const double pw = i < post_widths.size() ? post_widths[i] : post_widths.back();
        const Point p_xy = origin + along * post_centers[i];
        members.push_back(make_straight_member(
            Point(p_xy[0], p_xy[1], z_post_0),
            Point(p_xy[0], p_xy[1], z_post_1),
            thickness,
            pw,
            along,
            {},
            "post",
            stage_base + 1
        ));
    }

    const Point t_start(origin[0], origin[1], z1 - 0.5 * chord_depth);
    const Point t_end = t_start + along * length;
    members.push_back(
        make_straight_member(t_start, t_end, thickness, chord_depth, Z, {}, "chord_top", stage_base + 2)
    );
    (void)across;
}

/// Appends a tubular cellular CLT box-frame unit (`Plattenrahmen`): bottom slab `[z0, z0+t]`, two side walls `[z0+t, z1-t]`, top slab `[z1-t, z1]`.
void append_box_tube(
    std::vector<PlateSpec>& plates,
    const Point& origin_xy,
    const Vector& along,
    const Vector& across,
    double length,
    double tube_width,
    double z0,
    double z1,
    double t,
    int wall_segments,
    bool skylight,
    int stage_base
) {
    auto rect_plate = [&](const Point& p0, double len_a, double len_c, double za, double zb, const std::string& fam, int st) {
        const Polyline bot = Polyline::rectangle(Point(p0[0], p0[1], za), along, across, len_a, len_c);
        const Polyline top = Polyline::rectangle(Point(p0[0], p0[1], zb), along, across, len_a, len_c);
        PlateSpec ps;
        ps.bottom = bot;
        ps.top = top;
        ps.family = fam;
        ps.stage = st;
        return ps;
    };

    // 1. Bottom slab `[z0, z0 + t]`.
    plates.push_back(rect_plate(origin_xy, length, tube_width, z0, z0 + t, "slab_bottom", stage_base));

    // 2. Left and right side walls `[z0 + t, z1 - t]`, optionally perforated into piers with window openings.
    const int n_seg = std::max(1, wall_segments);
    if (n_seg == 1) {
        plates.push_back(rect_plate(origin_xy, length, t, z0 + t, z1 - t, "wall", stage_base + 1));
        plates.push_back(
            rect_plate(origin_xy + across * (tube_width - t), length, t, z0 + t, z1 - t, "wall", stage_base + 1)
        );
    } else {
        // Alternate solid shear wall piers with clear window openings along the tube sides.
        const double open_w = 0.24 * length / std::max(1, n_seg - 1);
        const double pier_w = (length - (n_seg - 1) * open_w) / n_seg;
        for (int k = 0; k < n_seg; ++k) {
            const double s0 = k * (pier_w + open_w);
            plates.push_back(
                rect_plate(origin_xy + along * s0, pier_w, t, z0 + t, z1 - t, "wall", stage_base + 1)
            );
            plates.push_back(rect_plate(
                origin_xy + along * s0 + across * (tube_width - t),
                pier_w,
                t,
                z0 + t,
                z1 - t,
                "wall",
                stage_base + 1
            ));
        }
    }

    // 3. Top ceiling/floor slab `[z1 - t, z1]`, with optional central skylight cutout.
    PlateSpec top_ps = rect_plate(origin_xy, length, tube_width, z1 - t, z1, "slab_top", stage_base + 2);
    if (skylight && length > 3200.0 && tube_width > 2400.0) {
        const double hole_l = 0.28 * length;
        const double hole_w = 0.44 * tube_width;
        const Point h0 = origin_xy + along * (0.5 * (length - hole_l)) + across * (0.5 * (tube_width - hole_w));
        const Polyline hb = Polyline::rectangle(Point(h0[0], h0[1], z1 - t), across, along, hole_w, hole_l);
        const Polyline ht = Polyline::rectangle(Point(h0[0], h0[1], z1), across, along, hole_w, hole_l);
        top_ps.holes_bottom.push_back(hb);
        top_ps.holes_top.push_back(ht);
    }
    plates.push_back(std::move(top_ps));
}

/// Appends flat longitudinal roof purlins on top of rafters at `z_roof` across `bents` bents.
void append_flat_roof_purlins(
    std::vector<MemberSpec>& members,
    double x0,
    double x1,
    int bents,
    double bay_spacing,
    double bent_width,
    double z_roof,
    double spacing,
    int stage
) {
    if (bents < 2 || spacing <= 10.0)
        return;
    const double pw = 140.0;
    const double pd = 240.0;
    const double y0 = -0.5 * bent_width;
    const double y1 = (bents - 1) * bay_spacing + 0.5 * bent_width;
    const int n_div = std::max(2, static_cast<int>(std::round((x1 - x0 - pw) / spacing)));
    for (int i = 0; i <= n_div; ++i) {
        const double x = (x0 + 0.5 * pw) + (x1 - x0 - pw) * static_cast<double>(i) / static_cast<double>(n_div);
        members.push_back(make_straight_member(
            Point(x, y0, z_roof + 0.5 * pd),
            Point(x, y1, z_roof + 0.5 * pd),
            pw,
            pd,
            Z,
            {},
            "purlin",
            stage
        ));
    }
}

/// Appends between-bent flush floor ties at `[z_floor - pd, z_floor]` so upper columns can stand directly on the rafters.
void append_between_bent_ties(
    std::vector<MemberSpec>& members,
    const std::vector<double>& x_positions,
    int bents,
    double bay_spacing,
    double bent_width,
    double z_floor,
    int stage
) {
    const double pw = 150.0;
    const double pd = 240.0;
    for (int b = 0; b + 1 < bents; ++b) {
        const double ya = b * bay_spacing + 0.5 * bent_width;
        const double yb = (b + 1) * bay_spacing - 0.5 * bent_width;
        if (yb - ya <= 200.0)
            continue;
        for (double x : x_positions) {
            members.push_back(make_straight_member(
                Point(x, ya, z_floor - 0.5 * pd),
                Point(x, yb, z_floor - 0.5 * pd),
                pw,
                pd,
                Z,
                {},
                "purlin",
                stage
            ));
        }
    }
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// FrameDesign & FrameGuide Core
// ═══════════════════════════════════════════════════════════════════════════

FrameDesign::FrameDesign(const std::string& name_in, const std::function<FrameGuide(double)>& make_in)
    : name(name_in),
      make(make_in) {}

FrameGuide::FrameGuide(
    const std::vector<SupportSpec>& supports_in,
    const std::vector<MemberSpec>& members_in,
    const std::vector<PlateSpec>& plates_in,
    double offset_x,
    const std::string& name_in
)
    : WoodSession(name_in),
      _supports(supports_in),
      _members(members_in),
      _plates(plates_in) {
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

void FrameGuide::shift_x(double dx) {
    const Xform xf = Xform::translation(dx, 0.0, 0.0);
    for (SupportSpec& s : _supports)
        s.plane = Plane(s.plane.origin().transformed(xf), s.plane.x_axis(), s.plane.y_axis());
    for (MemberSpec& m : _members) {
        m.axis = m.axis.transformed(xf);
        if (m.sections.size() >= 2) {
            for (Polyline& sec : m.sections)
                sec = sec.transformed(xf);
        }
        for (Plane& c : m.cuts)
            c = Plane::from_point_normal(c.origin().transformed(xf), c.z_axis());
        for (Mesh& sub : m.subtractions)
            sub = sub.transformed(xf);
    }
    for (PlateSpec& p : _plates) {
        p.bottom = p.bottom.transformed(xf);
        p.top = p.top.transformed(xf);
        for (Polyline& hb : p.holes_bottom)
            hb = hb.transformed(xf);
        for (Polyline& ht : p.holes_top)
            ht = ht.transformed(xf);
    }
}

void FrameGuide::draw() {
    std::shared_ptr<TreeNode> axis_grp = add_group("axes");
    for (const MemberSpec& m : _members)
        add_polyline(Polyline({m.axis.start(), m.axis.end()}), axis_grp);
    if (!_plates.empty()) {
        std::shared_ptr<TreeNode> plate_grp = add_group("outlines");
        for (const PlateSpec& p : _plates)
            add_polyline(p.top, plate_grp);
    }
}

double FrameGuide::span_x() const {
    double lo = std::numeric_limits<double>::max();
    double hi = -lo;
    for (const MemberSpec& m : _members) {
        lo = std::min({lo, m.axis.start()[0], m.axis.end()[0]});
        hi = std::max({hi, m.axis.start()[0], m.axis.end()[0]});
        if (m.sections.size() >= 2) {
            for (const Polyline& s : m.sections)
                for (const Point& pt : s.get_points()) {
                    lo = std::min(lo, pt[0]);
                    hi = std::max(hi, pt[0]);
                }
        }
    }
    for (const PlateSpec& p : _plates) {
        for (const Point& pt : p.top.get_points()) {
            lo = std::min(lo, pt[0]);
            hi = std::max(hi, pt[0]);
        }
    }
    return hi > lo ? (hi - lo) : 0.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// Group 1 & 2: Portal, Gable, Articulated & Tiered Frames (Engel §3.2)
// ═══════════════════════════════════════════════════════════════════════════

FrameGuide::FrameGuide(const TwoHingedPortal& d, double offset_x)
    : WoodSession("two_hinged_portal") {
    const double s_haunch = 0.28 * d.span;
    for (int b = 0; b < d.bents; ++b) {
        const double yc = b * d.bay_spacing;
        const SupportSpec s_l = make_shoe(0.5 * d.foot_depth, yc, 0.0, d.foot_depth);
        const SupportSpec s_r = make_shoe(d.span - 0.5 * d.foot_depth, yc, 0.0, d.foot_depth);
        _supports.push_back(s_l);
        _supports.push_back(s_r);

        const KneeMiter km_l = compute_portal_knee_miter(
            0.0, yc, -1.0, s_l.height, d.height, d.foot_depth, d.knee_depth, d.knee_depth, d.mid_depth, s_haunch, 0.0
        );
        const KneeMiter km_r = compute_portal_knee_miter(
            d.span, yc, 1.0, s_r.height, d.height, d.foot_depth, d.knee_depth, d.knee_depth, d.mid_depth, s_haunch, 0.0
        );

        _members.push_back(build_outer_leg(
            0.0, yc, d.width, -1.0, s_l.height, d.height, d.foot_depth, d.knee_depth, km_l.keep_leg, 0
        ));
        _members.push_back(build_outer_leg(
            d.span, yc, d.width, 1.0, s_r.height, d.height, d.foot_depth, d.knee_depth, km_r.keep_leg, 0
        ));

        const double k_h = (d.mid_depth - d.knee_depth) / s_haunch;
        const double d_ext = d.knee_depth - k_h * 60.0;
        _members.push_back(make_variable_member(
            {
                rafter_section_xz(-60.0, yc, d.width, d.height - d_ext, d.height),
                rafter_section_xz(s_haunch, yc, d.width, d.height - d.mid_depth, d.height),
                rafter_section_xz(d.span - s_haunch, yc, d.width, d.height - d.mid_depth, d.height),
                rafter_section_xz(d.span + 60.0, yc, d.width, d.height - d_ext, d.height),
            },
            {km_l.keep_rafter, km_r.keep_rafter},
            "rafter",
            1
        ));
    }
    append_flat_roof_purlins(_members, 0.0, d.span, d.bents, d.bay_spacing, d.width, d.height, d.purlin_spacing, 2);
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const ThreeHingedGable& d, double offset_x)
    : WoodSession("three_hinged_gable") {
    const double half = 0.5 * d.span;
    const double rise = d.ridge_height - d.eaves_height;
    const double slope = rise / half;

    for (int b = 0; b < d.bents; ++b) {
        const double yc = b * d.bay_spacing;
        const SupportSpec s_l = make_shoe(0.5 * d.foot_depth, yc, 0.0, d.foot_depth);
        const SupportSpec s_r = make_shoe(d.span - 0.5 * d.foot_depth, yc, 0.0, d.foot_depth);
        _supports.push_back(s_l);
        _supports.push_back(s_r);

        const KneeMiter km_l = compute_portal_knee_miter(
            0.0, yc, -1.0, s_l.height, d.eaves_height, d.foot_depth, d.knee_depth, d.knee_depth, d.crown_depth, half, slope
        );
        const KneeMiter km_r = compute_portal_knee_miter(
            d.span, yc, 1.0, s_r.height, d.eaves_height, d.foot_depth, d.knee_depth, d.knee_depth, d.crown_depth, half, slope
        );

        _members.push_back(build_outer_leg(
            0.0, yc, d.width, -1.0, s_l.height, d.eaves_height, d.foot_depth, d.knee_depth, km_l.keep_leg, 0
        ));
        _members.push_back(build_outer_leg(
            d.span, yc, d.width, 1.0, s_r.height, d.eaves_height, d.foot_depth, d.knee_depth, km_r.keep_leg, 0
        ));

        // Left & right pitched rafters tapering to the ridge crown hinge (`Dreigelenk`).
        auto z_top_l = [&](double x) { return d.eaves_height + slope * x; };
        auto d_l = [&](double x) { return d.knee_depth + (d.crown_depth - d.knee_depth) * (x / half); };
        const Plane crown_keep_l = Plane::from_point_normal(Point(half, yc, d.ridge_height), -X);
        const Plane crown_keep_r = Plane::from_point_normal(Point(half, yc, d.ridge_height), X);

        _members.push_back(make_variable_member(
            {
                rafter_section_xz(-60.0, yc, d.width, z_top_l(-60.0) - d_l(-60.0), z_top_l(-60.0)),
                rafter_section_xz(half + 20.0, yc, d.width, z_top_l(half + 20.0) - d_l(half + 20.0), z_top_l(half + 20.0)),
            },
            {km_l.keep_rafter, crown_keep_l},
            "rafter",
            1
        ));

        auto z_top_r = [&](double x) { return d.eaves_height + slope * (d.span - x); };
        auto d_r = [&](double x) { return d.knee_depth + (d.crown_depth - d.knee_depth) * ((d.span - x) / half); };
        _members.push_back(make_variable_member(
            {
                rafter_section_xz(half - 20.0, yc, d.width, z_top_r(half - 20.0) - d_r(half - 20.0), z_top_r(half - 20.0)),
                rafter_section_xz(d.span + 60.0, yc, d.width, z_top_r(d.span + 60.0) - d_r(d.span + 60.0), z_top_r(d.span + 60.0)),
            },
            {crown_keep_r, km_r.keep_rafter},
            "rafter",
            1
        ));
    }

    // Sloped roof purlins on both gable pitches.
    if (d.bents >= 2 && d.purlin_spacing > 10.0) {
        const double pw = 140.0;
        const double pd = 220.0;
        const double y0 = -0.5 * d.width;
        const double y1 = (d.bents - 1) * d.bay_spacing + 0.5 * d.width;
        const Vector up_l = Vector(-rise, 0.0, half).normalized();
        const Vector up_r = Vector(rise, 0.0, half).normalized();
        const int n_p = std::max(2, static_cast<int>(std::round((half - 300.0) / d.purlin_spacing)));
        for (int i = 0; i <= n_p; ++i) {
            const double s = 180.0 + (half - 440.0) * static_cast<double>(i) / static_cast<double>(n_p);
            const Point pl = Point(s, 0.0, d.eaves_height + slope * s) + up_l * (0.5 * pd);
            const Point pr = Point(d.span - s, 0.0, d.eaves_height + slope * s) + up_r * (0.5 * pd);
            _members.push_back(
                make_straight_member(Point(pl[0], y0, pl[2]), Point(pl[0], y1, pl[2]), pw, pd, up_l, {}, "purlin", 2)
            );
            _members.push_back(
                make_straight_member(Point(pr[0], y0, pr[2]), Point(pr[0], y1, pr[2]), pw, pd, up_r, {}, "purlin", 2)
            );
        }
    }
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const ArticulatedTFrame& d, double offset_x)
    : WoodSession("cantilever_t_frame") {
    const double total_w = 2.0 * d.bay_span;
    const double x_mid = d.bay_span;
    const double x_hl = d.hinge_offset;
    const double x_hr = total_w - d.hinge_offset;
    const double hl = 0.5 * d.lap_length;
    const double flat_tip = hl + 35.0;

    for (int b = 0; b < d.bents; ++b) {
        const double yc = b * d.bay_spacing;
        const SupportSpec s_l = make_shoe(0.5 * d.foot_depth, yc, 0.0, d.foot_depth);
        const SupportSpec s_m = make_shoe(x_mid, yc, 0.0, d.foot_depth);
        const SupportSpec s_r = make_shoe(total_w - 0.5 * d.foot_depth, yc, 0.0, d.foot_depth);
        _supports.push_back(s_l);
        _supports.push_back(s_m);
        _supports.push_back(s_r);

        // Outer legs with 45-deg knee miters + central symmetric T-column.
        const KneeMiter km_l = compute_portal_knee_miter(
            0.0, yc, -1.0, s_l.height, d.height, d.foot_depth, d.knee_depth, d.knee_depth, d.hinge_depth, x_hl - flat_tip, 0.0
        );
        const KneeMiter km_r = compute_portal_knee_miter(
            total_w, yc, 1.0, s_r.height, d.height, d.foot_depth, d.knee_depth, d.knee_depth, d.hinge_depth, x_hl - flat_tip, 0.0
        );
        _members.push_back(build_outer_leg(
            0.0, yc, d.width, -1.0, s_l.height, d.height, d.foot_depth, d.knee_depth, km_l.keep_leg, 0
        ));
        _members.push_back(build_centered_taper_column(
            x_mid, yc, d.width, s_m.height, d.height - d.knee_depth, d.foot_depth, d.knee_depth, 0
        ));
        _members.push_back(build_outer_leg(
            total_w, yc, d.width, 1.0, s_r.height, d.height, d.foot_depth, d.knee_depth, km_r.keep_leg, 0
        ));

        // Central T-frame double-cantilever girder (`stage = 1`, carries lower corbel seats at both ends!).
        const std::pair<Plane, Mesh> t_lap_l =
            make_xz_gerber_half_lap(x_hl, yc, d.width, d.height, d.hinge_depth, d.lap_length, -1.0, true);
        const std::pair<Plane, Mesh> t_lap_r =
            make_xz_gerber_half_lap(x_hr, yc, d.width, d.height, d.hinge_depth, d.lap_length, 1.0, true);
        _members.push_back(make_variable_member(
            {
                rafter_section_xz(x_hl - hl - 10.0, yc, d.width, d.height - d.hinge_depth, d.height),
                rafter_section_xz(x_hl + flat_tip, yc, d.width, d.height - d.hinge_depth, d.height),
                rafter_section_xz(x_mid - 0.5 * d.knee_depth, yc, d.width, d.height - d.knee_depth, d.height),
                rafter_section_xz(x_mid + 0.5 * d.knee_depth, yc, d.width, d.height - d.knee_depth, d.height),
                rafter_section_xz(x_hr - flat_tip, yc, d.width, d.height - d.hinge_depth, d.height),
                rafter_section_xz(x_hr + hl + 10.0, yc, d.width, d.height - d.hinge_depth, d.height),
            },
            {t_lap_l.first, t_lap_r.first},
            "rafter",
            1,
            {t_lap_l.second, t_lap_r.second}
        ));

        // Left and right L-frame rafters (`stage = 2`, upper dapped nibs lower straight onto knee miter + T-frame seat!).
        const std::pair<Plane, Mesh> l_lap =
            make_xz_gerber_half_lap(x_hl, yc, d.width, d.height, d.hinge_depth, d.lap_length, -1.0, false);
        const double k_l = (d.hinge_depth - d.knee_depth) / (x_hl - flat_tip);
        const double d_ext = d.knee_depth - k_l * 60.0;
        _members.push_back(make_variable_member(
            {
                rafter_section_xz(-60.0, yc, d.width, d.height - d_ext, d.height),
                rafter_section_xz(x_hl - flat_tip, yc, d.width, d.height - d.hinge_depth, d.height),
                rafter_section_xz(x_hl + hl + 10.0, yc, d.width, d.height - d.hinge_depth, d.height),
            },
            {km_l.keep_rafter, l_lap.first},
            "rafter",
            2,
            {l_lap.second}
        ));

        const std::pair<Plane, Mesh> r_lap =
            make_xz_gerber_half_lap(x_hr, yc, d.width, d.height, d.hinge_depth, d.lap_length, 1.0, false);
        _members.push_back(make_variable_member(
            {
                rafter_section_xz(x_hr - hl - 10.0, yc, d.width, d.height - d.hinge_depth, d.height),
                rafter_section_xz(x_hr + flat_tip, yc, d.width, d.height - d.hinge_depth, d.height),
                rafter_section_xz(total_w + 60.0, yc, d.width, d.height - d_ext, d.height),
            },
            {r_lap.first, km_r.keep_rafter},
            "rafter",
            2,
            {r_lap.second}
        ));
    }
    append_flat_roof_purlins(_members, 0.0, total_w, d.bents, d.bay_spacing, d.width, d.height, 2250.0, 3);
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const DoubledColumnCantilever& d, double offset_x)
    : WoodSession("doubled_column_cantilever") {
    const double x_c0 = d.cantilever;
    const double x_c1 = d.cantilever + d.spine_span;
    const double total_w = 2.0 * d.cantilever + d.spine_span;

    for (int b = 0; b < d.bents; ++b) {
        const double yc = b * d.bay_spacing;
        const SupportSpec s0 = make_shoe(x_c0, yc, 0.0, d.foot_depth);
        const SupportSpec s1 = make_shoe(x_c1, yc, 0.0, d.foot_depth);
        _supports.push_back(s0);
        _supports.push_back(s1);

        _members.push_back(build_centered_taper_column(
            x_c0, yc, d.width, s0.height, d.height - d.knee_depth, d.foot_depth, d.knee_depth, 0
        ));
        _members.push_back(build_centered_taper_column(
            x_c1, yc, d.width, s1.height, d.height - d.knee_depth, d.foot_depth, d.knee_depth, 0
        ));

        _members.push_back(make_variable_member(
            {
                rafter_section_xz(0.0, yc, d.width, d.height - d.tip_depth, d.height),
                rafter_section_xz(x_c0 - 0.5 * d.knee_depth, yc, d.width, d.height - d.knee_depth, d.height),
                rafter_section_xz(x_c0 + 0.5 * d.knee_depth, yc, d.width, d.height - d.knee_depth, d.height),
                rafter_section_xz(0.5 * total_w, yc, d.width, d.height - 0.72 * d.knee_depth, d.height),
                rafter_section_xz(x_c1 - 0.5 * d.knee_depth, yc, d.width, d.height - d.knee_depth, d.height),
                rafter_section_xz(x_c1 + 0.5 * d.knee_depth, yc, d.width, d.height - d.knee_depth, d.height),
                rafter_section_xz(total_w, yc, d.width, d.height - d.tip_depth, d.height),
            },
            {},
            "rafter",
            1
        ));
    }
    append_flat_roof_purlins(_members, 0.0, total_w, d.bents, d.bay_spacing, d.width, d.height, d.purlin_spacing, 2);
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const CoupledTwoHinged& d, double offset_x)
    : WoodSession("coupled_two_hinged") {
    const double x_in_l = d.hall_span;
    const double x_mid = d.hall_span + d.corridor_cantilever;
    const double x_in_r = x_mid + d.corridor_cantilever;
    const double total_w = 2.0 * x_mid;
    const double s_haunch = 0.30 * d.hall_span;
    const double d_mid = 0.65 * d.knee_depth;

    for (int s = 0; s < d.storeys; ++s) {
        const double z0 = s * d.storey_height;
        const double z1 = (s + 1) * d.storey_height;
        const int st = s * 10;

        for (int b = 0; b < d.bents; ++b) {
            const double yc = b * d.bay_spacing;
            double z_foot = z0;
            if (s == 0) {
                const SupportSpec sh0 = make_shoe(0.5 * d.foot_depth, yc, 0.0, d.foot_depth);
                const SupportSpec sh1 = make_shoe(x_in_l, yc, 0.0, d.foot_depth);
                const SupportSpec sh2 = make_shoe(x_in_r, yc, 0.0, d.foot_depth);
                const SupportSpec sh3 = make_shoe(total_w - 0.5 * d.foot_depth, yc, 0.0, d.foot_depth);
                _supports.push_back(sh0);
                _supports.push_back(sh1);
                _supports.push_back(sh2);
                _supports.push_back(sh3);
                z_foot = sh0.height;
            }

            const KneeMiter km_l = compute_portal_knee_miter(
                0.0, yc, -1.0, z_foot, z1, d.foot_depth, d.knee_depth, d.knee_depth, d_mid, s_haunch, 0.0
            );
            const KneeMiter km_r = compute_portal_knee_miter(
                total_w, yc, 1.0, z_foot, z1, d.foot_depth, d.knee_depth, d.knee_depth, d_mid, s_haunch, 0.0
            );

            _members.push_back(build_outer_leg(
                0.0, yc, d.width, -1.0, z_foot, z1, d.foot_depth, d.knee_depth, km_l.keep_leg, st
            ));
            _members.push_back(build_centered_taper_column(
                x_in_l, yc, d.width, z_foot, z1 - d.knee_depth, d.foot_depth, d.knee_depth, st
            ));
            _members.push_back(build_centered_taper_column(
                x_in_r, yc, d.width, z_foot, z1 - d.knee_depth, d.foot_depth, d.knee_depth, st
            ));
            _members.push_back(build_outer_leg(
                total_w, yc, d.width, 1.0, z_foot, z1, d.foot_depth, d.knee_depth, km_r.keep_leg, st
            ));

            const double k_h = (d_mid - d.knee_depth) / s_haunch;
            const double d_ext = d.knee_depth - k_h * 60.0;
            const Plane mid_l = Plane::from_point_normal(Point(x_mid, yc, z1), -X);
            const Plane mid_r = Plane::from_point_normal(Point(x_mid, yc, z1), X);

            _members.push_back(make_variable_member(
                {
                    rafter_section_xz(-60.0, yc, d.width, z1 - d_ext, z1),
                    rafter_section_xz(s_haunch, yc, d.width, z1 - d_mid, z1),
                    rafter_section_xz(x_in_l - s_haunch, yc, d.width, z1 - d_mid, z1),
                    rafter_section_xz(x_in_l - 0.5 * d.knee_depth, yc, d.width, z1 - d.knee_depth, z1),
                    rafter_section_xz(x_in_l + 0.5 * d.knee_depth, yc, d.width, z1 - d.knee_depth, z1),
                    rafter_section_xz(x_mid, yc, d.width, z1 - d.tip_depth, z1),
                },
                {km_l.keep_rafter, mid_l},
                "rafter",
                st + 1
            ));
            _members.push_back(make_variable_member(
                {
                    rafter_section_xz(x_mid, yc, d.width, z1 - d.tip_depth, z1),
                    rafter_section_xz(x_in_r - 0.5 * d.knee_depth, yc, d.width, z1 - d.knee_depth, z1),
                    rafter_section_xz(x_in_r + 0.5 * d.knee_depth, yc, d.width, z1 - d.knee_depth, z1),
                    rafter_section_xz(x_in_r + s_haunch, yc, d.width, z1 - d_mid, z1),
                    rafter_section_xz(total_w - s_haunch, yc, d.width, z1 - d_mid, z1),
                    rafter_section_xz(total_w + 60.0, yc, d.width, z1 - d_ext, z1),
                },
                {mid_r, km_r.keep_rafter},
                "rafter",
                st + 1
            ));
        }

        if (s + 1 < d.storeys) {
            append_between_bent_ties(
                _members,
                {2400.0, 5600.0, x_mid, total_w - 5600.0, total_w - 2400.0},
                d.bents,
                d.bay_spacing,
                d.width,
                z1,
                st + 2
            );
        } else {
            append_flat_roof_purlins(_members, 0.0, total_w, d.bents, d.bay_spacing, d.width, z1, 2400.0, st + 2);
        }
    }
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const CoupledThreeHinged& d, double offset_x)
    : WoodSession("coupled_three_hinged") {
    const double half = 0.5 * d.bay_span;
    const double total_w = d.columns_per_bent * d.bay_span;

    for (int s = 0; s < d.storeys; ++s) {
        const double z0 = s * d.storey_height;
        const double z1 = (s + 1) * d.storey_height;
        const int st = s * 10;
        std::vector<double> tie_xs;
        for (int c = 0; c < d.columns_per_bent; ++c) {
            const double xc = (c + 0.5) * d.bay_span;
            tie_xs.push_back(xc - 0.55 * half);
            tie_xs.push_back(xc + 0.55 * half);
        }

        for (int b = 0; b < d.bents; ++b) {
            const double yc = b * d.bay_spacing;
            for (int c = 0; c < d.columns_per_bent; ++c) {
                const double xc = (c + 0.5) * d.bay_span;
                double z_foot = z0;
                if (s == 0) {
                    const SupportSpec sh = make_shoe(xc, yc, 0.0, d.foot_depth);
                    _supports.push_back(sh);
                    z_foot = sh.height;
                }
                _members.push_back(build_centered_taper_column(
                    xc, yc, d.width, z_foot, z1 - d.head_depth, d.foot_depth, d.head_depth, st
                ));
                _members.push_back(make_variable_member(
                    {
                        rafter_section_xz(xc - half, yc, d.width, z1 - d.tip_depth, z1),
                        rafter_section_xz(xc - 0.5 * d.head_depth, yc, d.width, z1 - d.head_depth, z1),
                        rafter_section_xz(xc + 0.5 * d.head_depth, yc, d.width, z1 - d.head_depth, z1),
                        rafter_section_xz(xc + half, yc, d.width, z1 - d.tip_depth, z1),
                    },
                    {},
                    "rafter",
                    st + 1
                ));
            }
        }
        if (s + 1 < d.storeys)
            append_between_bent_ties(_members, tie_xs, d.bents, d.bay_spacing, d.width, z1, st + 2);
        else
            append_flat_roof_purlins(_members, 0.0, total_w, d.bents, d.bay_spacing, d.width, z1, 2200.0, st + 2);
    }
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const TieredClerestoryHall& d, double offset_x)
    : WoodSession("tiered_clerestory_hall") {
    // Lower portals `[0, side_span]` + inward cantilever, and `[total_w - side_span, total_w]` + inward cantilever.
    const double gap_mid = d.upper_span - 2.0 * (0.5 * d.inward_cantilever);
    const double x_tip_l = d.side_span + d.inward_cantilever;
    const double x_tip_r = x_tip_l + gap_mid;
    const double x_col_r = x_tip_r + d.inward_cantilever;
    const double total_w = x_col_r + d.side_span;
    const double x_up_l = 0.5 * (total_w - d.upper_span);
    const double x_up_r = 0.5 * (total_w + d.upper_span);
    const double d_foot = 260.0;
    const double d_knee = 600.0;
    const double d_mid = 360.0;
    const double d_tip = 280.0;
    const double s_h = 0.28 * d.side_span;

    for (int b = 0; b < d.bents; ++b) {
        const double yc = b * d.bay_spacing;
        const SupportSpec s0 = make_shoe(0.5 * d_foot, yc, 0.0, d_foot);
        const SupportSpec s1 = make_shoe(d.side_span, yc, 0.0, d_foot);
        const SupportSpec s2 = make_shoe(x_col_r, yc, 0.0, d_foot);
        const SupportSpec s3 = make_shoe(total_w - 0.5 * d_foot, yc, 0.0, d_foot);
        _supports.insert(_supports.end(), {s0, s1, s2, s3});

        const KneeMiter km_l = compute_portal_knee_miter(
            0.0, yc, -1.0, s0.height, d.lower_height, d_foot, d_knee, d_knee, d_mid, s_h, 0.0
        );
        const KneeMiter km_r = compute_portal_knee_miter(
            total_w, yc, 1.0, s3.height, d.lower_height, d_foot, d_knee, d_knee, d_mid, s_h, 0.0
        );

        _members.push_back(build_outer_leg(0.0, yc, d.width, -1.0, s0.height, d.lower_height, d_foot, d_knee, km_l.keep_leg, 0));
        _members.push_back(build_centered_taper_column(d.side_span, yc, d.width, s1.height, d.lower_height - d_knee, d_foot, d_knee, 0));
        _members.push_back(build_centered_taper_column(x_col_r, yc, d.width, s2.height, d.lower_height - d_knee, d_foot, d_knee, 0));
        _members.push_back(build_outer_leg(total_w, yc, d.width, 1.0, s3.height, d.lower_height, d_foot, d_knee, km_r.keep_leg, 0));

        const double d_ext = d_knee - ((d_mid - d_knee) / s_h) * 60.0;
        _members.push_back(make_variable_member(
            {
                rafter_section_xz(-60.0, yc, d.width, d.lower_height - d_ext, d.lower_height),
                rafter_section_xz(s_h, yc, d.width, d.lower_height - d_mid, d.lower_height),
                rafter_section_xz(d.side_span - s_h, yc, d.width, d.lower_height - d_mid, d.lower_height),
                rafter_section_xz(d.side_span - 0.5 * d_knee, yc, d.width, d.lower_height - d_knee, d.lower_height),
                rafter_section_xz(d.side_span + 0.5 * d_knee, yc, d.width, d.lower_height - d_knee, d.lower_height),
                rafter_section_xz(x_tip_l, yc, d.width, d.lower_height - d_tip, d.lower_height),
            },
            {km_l.keep_rafter},
            "rafter",
            1
        ));
        _members.push_back(make_variable_member(
            {
                rafter_section_xz(x_tip_r, yc, d.width, d.lower_height - d_tip, d.lower_height),
                rafter_section_xz(x_col_r - 0.5 * d_knee, yc, d.width, d.lower_height - d_knee, d.lower_height),
                rafter_section_xz(x_col_r + 0.5 * d_knee, yc, d.width, d.lower_height - d_knee, d.lower_height),
                rafter_section_xz(x_col_r + s_h, yc, d.width, d.lower_height - d_mid, d.lower_height),
                rafter_section_xz(total_w - s_h, yc, d.width, d.lower_height - d_mid, d.lower_height),
                rafter_section_xz(total_w + 60.0, yc, d.width, d.lower_height - d_ext, d.lower_height),
            },
            {km_r.keep_rafter},
            "rafter",
            1
        ));

        // Upper clerestory two-hinged portal standing directly on the inward cantilevers (`stage = 10..11`).
        const double z_up = d.lower_height + d.upper_height;
        const double s_hu = 0.28 * d.upper_span;
        const KneeMiter ukm_l = compute_portal_knee_miter(
            x_up_l, yc, -1.0, d.lower_height, z_up, 240.0, 540.0, 540.0, 340.0, s_hu, 0.0
        );
        const KneeMiter ukm_r = compute_portal_knee_miter(
            x_up_r, yc, 1.0, d.lower_height, z_up, 240.0, 540.0, 540.0, 340.0, s_hu, 0.0
        );
        _members.push_back(build_outer_leg(x_up_l, yc, d.width, -1.0, d.lower_height, z_up, 240.0, 540.0, ukm_l.keep_leg, 10));
        _members.push_back(build_outer_leg(x_up_r, yc, d.width, 1.0, d.lower_height, z_up, 240.0, 540.0, ukm_r.keep_leg, 10));

        const double du_ext = 540.0 - ((340.0 - 540.0) / s_hu) * 60.0;
        _members.push_back(make_variable_member(
            {
                rafter_section_xz(x_up_l - 60.0, yc, d.width, z_up - du_ext, z_up),
                rafter_section_xz(x_up_l + s_hu, yc, d.width, z_up - 340.0, z_up),
                rafter_section_xz(x_up_r - s_hu, yc, d.width, z_up - 340.0, z_up),
                rafter_section_xz(x_up_r + 60.0, yc, d.width, z_up - du_ext, z_up),
            },
            {ukm_l.keep_rafter, ukm_r.keep_rafter},
            "rafter",
            11
        ));
    }
    append_flat_roof_purlins(_members, 0.0, x_up_l - 300.0, d.bents, d.bay_spacing, d.width, d.lower_height, 2100.0, 2);
    append_flat_roof_purlins(_members, x_up_r + 300.0, total_w, d.bents, d.bay_spacing, d.width, d.lower_height, 2100.0, 2);
    append_flat_roof_purlins(_members, x_up_l, x_up_r, d.bents, d.bay_spacing, d.width, d.lower_height + d.upper_height, 2200.0, 12);
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const TieredOutwardHall& d, double offset_x)
    : WoodSession("tiered_outward_hall") {
    const double total_w = d.lower_span + 2.0 * d.outward_cantilever;
    const double x_c0 = d.outward_cantilever;
    const double x_c1 = d.outward_cantilever + d.lower_span;
    const double x_mid = 0.5 * total_w;
    const double x_up_l = 0.5 * (total_w - d.upper_span);
    const double x_up_r = 0.5 * (total_w + d.upper_span);
    const double d_foot = 280.0;
    const double d_knee = 640.0;
    const double d_crown = 260.0;

    for (int b = 0; b < d.bents; ++b) {
        const double yc = b * d.bay_spacing;
        const SupportSpec s0 = make_shoe(x_c0, yc, 0.0, d_foot);
        const SupportSpec s1 = make_shoe(x_c1, yc, 0.0, d_foot);
        _supports.push_back(s0);
        _supports.push_back(s1);

        _members.push_back(build_centered_taper_column(x_c0, yc, d.width, s0.height, d.lower_height - d_knee, d_foot, d_knee, 0));
        _members.push_back(build_centered_taper_column(x_c1, yc, d.width, s1.height, d.lower_height - d_knee, d_foot, d_knee, 0));

        // Lower three-hinged double-cantilever portal (`crown` hinge at `x_mid`, outward cantilevers to `0` and `total_w`).
        const Plane crown_l = Plane::from_point_normal(Point(x_mid, yc, d.lower_height), -X);
        const Plane crown_r = Plane::from_point_normal(Point(x_mid, yc, d.lower_height), X);
        _members.push_back(make_variable_member(
            {
                rafter_section_xz(0.0, yc, d.width, d.lower_height - 280.0, d.lower_height),
                rafter_section_xz(x_c0 - 0.5 * d_knee, yc, d.width, d.lower_height - d_knee, d.lower_height),
                rafter_section_xz(x_c0 + 0.5 * d_knee, yc, d.width, d.lower_height - d_knee, d.lower_height),
                rafter_section_xz(x_mid, yc, d.width, d.lower_height - d_crown, d.lower_height),
            },
            {crown_l},
            "rafter",
            1
        ));
        _members.push_back(make_variable_member(
            {
                rafter_section_xz(x_mid, yc, d.width, d.lower_height - d_crown, d.lower_height),
                rafter_section_xz(x_c1 - 0.5 * d_knee, yc, d.width, d.lower_height - d_knee, d.lower_height),
                rafter_section_xz(x_c1 + 0.5 * d_knee, yc, d.width, d.lower_height - d_knee, d.lower_height),
                rafter_section_xz(total_w, yc, d.width, d.lower_height - 280.0, d.lower_height),
            },
            {crown_r},
            "rafter",
            1
        ));

        // Upper wide two-hinged portal standing on the lower outward cantilevers (`Engel p193-05/06`).
        const double z_up = d.lower_height + d.upper_height;
        const double s_hu = 0.26 * d.upper_span;
        const KneeMiter ukm_l = compute_portal_knee_miter(
            x_up_l, yc, -1.0, d.lower_height, z_up, 240.0, 580.0, 580.0, 360.0, s_hu, 0.0
        );
        const KneeMiter ukm_r = compute_portal_knee_miter(
            x_up_r, yc, 1.0, d.lower_height, z_up, 240.0, 580.0, 580.0, 360.0, s_hu, 0.0
        );
        _members.push_back(build_outer_leg(x_up_l, yc, d.width, -1.0, d.lower_height, z_up, 240.0, 580.0, ukm_l.keep_leg, 10));
        _members.push_back(build_outer_leg(x_up_r, yc, d.width, 1.0, d.lower_height, z_up, 240.0, 580.0, ukm_r.keep_leg, 10));

        const double du_ext = 580.0 - ((360.0 - 580.0) / s_hu) * 60.0;
        _members.push_back(make_variable_member(
            {
                rafter_section_xz(x_up_l - 60.0, yc, d.width, z_up - du_ext, z_up),
                rafter_section_xz(x_up_l + s_hu, yc, d.width, z_up - 360.0, z_up),
                rafter_section_xz(x_up_r - s_hu, yc, d.width, z_up - 360.0, z_up),
                rafter_section_xz(x_up_r + 60.0, yc, d.width, z_up - du_ext, z_up),
            },
            {ukm_l.keep_rafter, ukm_r.keep_rafter},
            "rafter",
            11
        ));
    }
    append_between_bent_ties(_members, {x_c0, x_mid, x_c1}, d.bents, d.bay_spacing, d.width, d.lower_height, 2);
    append_flat_roof_purlins(_members, x_up_l, x_up_r, d.bents, d.bay_spacing, d.width, d.lower_height + d.upper_height, 2400.0, 12);
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

// ═══════════════════════════════════════════════════════════════════════════
// Group 3: Multi-Panel Vierendeel Frames (Engel §3.2, pp. 194-199)
// ═══════════════════════════════════════════════════════════════════════════

FrameGuide::FrameGuide(const ColumnGradatedVierendeel& d, double offset_x)
    : WoodSession("column_gradated_vierendeel") {
    const int n_p = std::max(3, d.panels);
    std::vector<double> centers(static_cast<size_t>(n_p + 1), 0.0);
    std::vector<double> widths(static_cast<size_t>(n_p + 1), d.min_post_width);
    const double mid = 0.5 * n_p;
    for (int i = 0; i <= n_p; ++i) {
        const double t = mid > 0.0 ? std::abs(i - mid) / mid : 1.0;
        const double pw = d.min_post_width + (d.max_post_width - d.min_post_width) * t;
        widths[static_cast<size_t>(i)] = pw;
        const double raw_x = d.span * static_cast<double>(i) / static_cast<double>(n_p);
        centers[static_cast<size_t>(i)] =
            (i == 0) ? (0.5 * pw) : (i == n_p ? (d.span - 0.5 * pw) : raw_x);
    }

    const double z0 = d.pier_height;
    const double z1 = z0 + d.frame_depth;
    for (int b = 0; b < d.bents; ++b) {
        const double yc = b * d.bay_spacing;
        const SupportSpec s_l = make_shoe(0.5 * d.max_post_width, yc, 0.0, 280.0);
        const SupportSpec s_r = make_shoe(d.span - 0.5 * d.max_post_width, yc, 0.0, 280.0);
        _supports.push_back(s_l);
        _supports.push_back(s_r);
        _members.push_back(build_centered_taper_column(
            0.5 * d.max_post_width, yc, d.thickness, s_l.height, z0, 280.0, d.max_post_width, 0
        ));
        _members.push_back(build_centered_taper_column(
            d.span - 0.5 * d.max_post_width, yc, d.thickness, s_r.height, z0, 280.0, d.max_post_width, 0
        ));
        append_vierendeel_elevation(
            _members, Point(0.0, yc, 0.0), X, d.span, d.thickness, z0, z1, d.chord_depth, centers, widths, 1
        );
    }
    append_between_bent_ties(_members, centers, d.bents, d.bay_spacing, d.thickness, z0 + d.chord_depth, 2);
    append_between_bent_ties(_members, centers, d.bents, d.bay_spacing, d.thickness, z1, 4);
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const BayGradatedVierendeel& d, double offset_x)
    : WoodSession("bay_gradated_vierendeel") {
    const std::vector<double> bays = symmetric_gradated_bays(d.span - d.post_width, d.panels, d.end_bay_ratio);
    std::vector<double> centers = {0.5 * d.post_width};
    for (double bw : bays)
        centers.push_back(centers.back() + bw);
    const std::vector<double> widths(centers.size(), d.post_width);

    const double z0 = d.pier_height;
    const double z1 = z0 + d.frame_depth;
    for (int b = 0; b < 2; ++b) {
        const double yc = b * d.hall_depth;
        const SupportSpec sh = make_shoe(0.5 * d.span, yc, 0.0, 280.0);
        _supports.push_back(sh);
        _members.push_back(build_centered_taper_column(0.5 * d.span, yc, d.thickness, sh.height, z0, 280.0, 560.0, 0));
        append_vierendeel_elevation(
            _members, Point(0.0, yc, 0.0), X, d.span, d.thickness, z0, z1, d.chord_depth, centers, widths, 1
        );
    }
    append_between_bent_ties(_members, centers, 2, d.hall_depth, d.thickness, z0 + d.chord_depth, 2);
    append_between_bent_ties(_members, centers, 2, d.hall_depth, d.thickness, z1, 4);
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const ConcentricVierendeelGrid& d, double offset_x)
    : WoodSession("concentric_vierendeel_grid") {
    // 6 x 6 bays (`cell = size / 6`): four primary Vierendeel lines at `0, 2*cell, 4*cell, 6*cell` in BOTH X and Y on 8 perimeter columns!
    const double cell = d.size / 6.0;
    const double ht = 0.5 * d.thickness;
    const double z0 = d.pier_height;
    const double z1 = z0 + d.frame_depth;
    const std::array<double, 4> main_lines = {ht, 2.0 * cell, 4.0 * cell, d.size - ht};

    // 8 perimeter fin columns at `(2*cell, ht), (4*cell, ht), ...` (Engel p197-01 top).
    const std::array<std::pair<double, double>, 8> col_pts = {{
        {2.0 * cell, ht},
        {4.0 * cell, ht},
        {d.size - ht, 2.0 * cell},
        {d.size - ht, 4.0 * cell},
        {4.0 * cell, d.size - ht},
        {2.0 * cell, d.size - ht},
        {ht, 4.0 * cell},
        {ht, 2.0 * cell},
    }};
    for (const std::pair<double, double>& cp : col_pts) {
        _members.push_back(make_straight_member(
            Point(cp.first, cp.second, 0.0),
            Point(cp.first, cp.second, z0),
            d.thickness,
            480.0,
            std::abs(cp.second - ht) < 10.0 || std::abs(cp.second - (d.size - ht)) < 10.0 ? Y : X,
            {},
            "column",
            0
        ));
    }

    // Continuous X-direction Vierendeel trusses at `y = main_lines[0..3]`.
    std::vector<double> x_posts = {ht, cell, 2.0 * cell, 3.0 * cell, 4.0 * cell, 5.0 * cell, d.size - ht};
    std::vector<double> x_widths(x_posts.size(), d.post_width);
    for (double y : main_lines) {
        append_vierendeel_elevation(
            _members, Point(0.0, y, 0.0), X, d.size, d.thickness, z0, z1, d.chord_depth, x_posts, x_widths, 1
        );
    }

    // Intersecting Y-direction Vierendeel segments trimmed cleanly between the X-trusses!
    for (double x : main_lines) {
        for (size_t seg = 0; seg < 3; ++seg) {
            const double ya = main_lines[seg] + ht;
            const double yb = main_lines[seg + 1] - ht;
            const double seg_len = yb - ya;
            const double y_mid_post = 0.5 * seg_len;
            append_vierendeel_elevation(
                _members,
                Point(x, ya, 0.0),
                Y,
                seg_len,
                d.thickness,
                z0,
                z1,
                d.chord_depth,
                {y_mid_post},
                {d.post_width},
                1
            );
        }
    }
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const CrosswiseStackedVierendeel& d, double offset_x)
    : WoodSession("crosswise_stacked_vierendeel") {
    const double cell = d.length / static_cast<double>(d.panels);
    const double ht = 0.5 * d.thickness;
    std::vector<double> posts;
    for (int i = 0; i <= d.panels; ++i) {
        const double raw = i * cell;
        posts.push_back(i == 0 ? ht : (i == d.panels ? d.length - ht : raw));
    }
    const std::vector<double> widths(posts.size(), d.post_width);

    // 4 ground columns at `(2*cell, 2*cell)` and `(4*cell, 4*cell)` carrying the crosswise stack (Engel p197-01 bottom).
    const std::array<double, 2> spine_pos = {2.0 * cell, (d.panels - 2) * cell};
    for (double cx : spine_pos) {
        for (double cy : spine_pos) {
            _members.push_back(make_straight_member(
                Point(cx, cy, 0.0), Point(cx, cy, d.pier_height), 360.0, 360.0, Y, {}, "column", 0
            ));
        }
    }

    for (int s = 0; s < d.storeys; ++s) {
        const double z0 = d.pier_height + s * d.frame_depth;
        const double z1 = z0 + d.frame_depth;
        const int st = 1 + s * 10;
        const bool along_x = (s % 2) == 0;
        const std::vector<double> offsets =
            (s == 0) ? std::vector<double>{spine_pos[0], spine_pos[1]}
                     : std::vector<double>{cell, spine_pos[0], spine_pos[1], (d.panels - 1) * cell};

        for (double pos : offsets) {
            if (along_x) {
                append_vierendeel_elevation(
                    _members, Point(0.0, pos, 0.0), X, d.length, d.thickness, z0, z1, d.chord_depth, posts, widths, st
                );
            } else {
                append_vierendeel_elevation(
                    _members, Point(pos, 0.0, 0.0), Y, d.length, d.thickness, z0, z1, d.chord_depth, posts, widths, st
                );
            }
        }
    }
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

// ═══════════════════════════════════════════════════════════════════════════
// Group 4: Cellular Slab Box Frames (`Plattenrahmen`, Engel §3.4, p. 209)
// ═══════════════════════════════════════════════════════════════════════════

FrameGuide::FrameGuide(const OpenBoxFrames& d, double offset_x)
    : WoodSession("open_box_frames") {
    const double t = d.thickness;
    // Three adjacent ground open box portals along X (`2` rows deep along Y), plus a raised central cross-box (Engel p209-01 #1).
    for (int row = 0; row < 2; ++row) {
        const double y0 = row * (d.depth + 800.0);
        for (int col = 0; col < 3; ++col) {
            const double x0 = col * (d.bay_width + 400.0);
            append_box_tube(_plates, Point(x0, y0, 0.0), Y, X, d.depth, d.bay_width, 0.0, d.height, t, 1, false, 0);
        }
    }
    // Second-level central bridge box straddling the middle row.
    const double x_mid = d.bay_width + 400.0;
    append_box_tube(
        _plates,
        Point(x_mid - 1200.0, 0.35 * d.depth, 0.0),
        X,
        Y,
        d.bay_width + 2400.0,
        1.30 * d.depth + 800.0,
        d.height,
        1.92 * d.height,
        t,
        2,
        true,
        10
    );
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const ClosedPilotisBox& d, double offset_x)
    : WoodSession("closed_pilotis_box") {
    // Ground pilotis columns supporting two parallel long perforated CLT box tubes linked by two cross-boxes (Engel p209-01 #2).
    const double t = d.thickness;
    const double y_a = 0.0;
    const double y_b = d.grid_size - d.tube_width;
    const double clear_y = y_b - (y_a + d.tube_width);

    const std::array<double, 3> col_xs = {2600.0, 0.5 * d.grid_size, d.grid_size - 2600.0};
    const std::array<double, 2> col_ys = {0.5 * d.tube_width, y_b + 0.5 * d.tube_width};
    for (double cx : col_xs) {
        for (double cy : col_ys) {
            _members.push_back(make_straight_member(
                Point(cx, cy, 0.0), Point(cx, cy, d.pier_height), 320.0, 320.0, Y, {}, "column", 0
            ));
        }
    }

    const double z0 = d.pier_height;
    const double z1 = z0 + d.box_height;
    append_box_tube(_plates, Point(0.0, y_a, 0.0), X, Y, d.grid_size, d.tube_width, z0, z1, t, 3, true, 1);
    append_box_tube(_plates, Point(0.0, y_b, 0.0), X, Y, d.grid_size, d.tube_width, z0, z1, t, 3, true, 1);
    if (clear_y > 1200.0) {
        append_box_tube(
            _plates, Point(2200.0, d.tube_width, 0.0), Y, X, clear_y, 0.82 * d.tube_width, z0, z1, t, 1, true, 1
        );
        append_box_tube(
            _plates,
            Point(d.grid_size - 2200.0 - 0.82 * d.tube_width, d.tube_width, 0.0),
            Y,
            X,
            clear_y,
            0.82 * d.tube_width,
            z0,
            z1,
            t,
            1,
            true,
            1
        );
    }
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const CantileveredMultiBox& d, double offset_x)
    : WoodSession("cantilevered_multi_box") {
    const double t = d.thickness;
    const double w = d.tube_width;
    const double h = d.box_height;

    // Ground U-portal piers (`[0, d.pier_height]`).
    append_box_tube(_plates, Point(2600.0, 2200.0, 0.0), Y, X, 10800.0, w, 0.0, d.pier_height, t, 2, false, 0);
    append_box_tube(_plates, Point(2600.0 + w + 2800.0, 2200.0, 0.0), Y, X, 10800.0, w, 0.0, d.pier_height, t, 2, false, 0);

    // Level 1 cantilevered boxes along X (`[pier_height, pier_height + h]`).
    const double z1 = d.pier_height;
    append_box_tube(_plates, Point(0.0, 1400.0, 0.0), X, Y, 18400.0, w, z1, z1 + h, t, 3, false, 10);
    append_box_tube(_plates, Point(1600.0, 1400.0 + w + 2400.0, 0.0), X, Y, 18400.0, w, z1, z1 + h, t, 3, false, 10);

    // Level 2 shifted cantilevered boxes along Y (`[z1 + h, z1 + 2*h]`).
    const double z2 = z1 + h;
    append_box_tube(_plates, Point(1800.0, 0.0, 0.0), Y, X, 15200.0, w, z2, z2 + h, t, 3, true, 20);
    append_box_tube(_plates, Point(1800.0 + w + 3600.0, 1800.0, 0.0), Y, X, 14400.0, w, z2, z2 + h, t, 3, true, 20);

    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

FrameGuide::FrameGuide(const CrosswiseStackedBoxes& d, double offset_x)
    : WoodSession("crosswise_stacked_boxes") {
    const double t = d.thickness;
    const double w = d.tube_width;
    const double L = d.length;
    const double p0 = 0.24 * L;
    const double p1 = L - p0 - w;

    // Ground portal box piers along Y (`[0, pier_height]`).
    append_box_tube(_plates, Point(p0, p0, 0.0), Y, X, L - 2.0 * p0, w, 0.0, d.pier_height, t, 2, false, 0);
    append_box_tube(_plates, Point(p1, p0, 0.0), Y, X, L - 2.0 * p0, w, 0.0, d.pier_height, t, 2, false, 0);

    // Crosswise-stacked perforated box tubes alternating X and Y (`Engel p209-01 #4, Kreuzweise gestapelte Plattenrahmen`).
    for (int lv = 0; lv < d.levels; ++lv) {
        const double za = d.pier_height + lv * d.box_height;
        const double zb = za + d.box_height;
        const int st = 10 + lv * 10;
        const bool along_x = (lv % 2) == 0;
        if (along_x) {
            append_box_tube(_plates, Point(0.0, p0, 0.0), X, Y, L, w, za, zb, t, 3, false, st);
            append_box_tube(_plates, Point(0.0, p1, 0.0), X, Y, L, w, za, zb, t, 3, false, st);
        } else {
            append_box_tube(_plates, Point(p0, 0.0, 0.0), Y, X, L, w, za, zb, t, 3, false, st);
            append_box_tube(_plates, Point(p1, 0.0, 0.0), Y, X, L, w, za, zb, t, 3, false, st);
        }
    }
    if (std::abs(offset_x) > 1e-6)
        shift_x(offset_x);
    draw();
}

} // namespace wood_frame
