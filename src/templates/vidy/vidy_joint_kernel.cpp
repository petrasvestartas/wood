#include "src/templates/vidy/vidy_joint_kernel.h"

namespace wood_vidy {

laika_cpp::Vector vidy_inplane_outward(
    const laika_cpp::Point& centroid,
    const laika_cpp::Vector& normal,
    const laika_cpp::Point& p_start,
    const laika_cpp::Point& p_end
) {

    laika_cpp::Vector out = (p_end - p_start).normalized().cross(normal).normalized();

    if ((laika_cpp::Point::mid_point(p_start, p_end) - centroid).dot(out) < 0.0)
        out = -out;

    return out;
}

size_t vidy_find_shared_side(const std::vector<size_t>& vertices, size_t u, size_t v) {

    const size_t m = vertices.size();

    for (size_t k = 0; k < m; k++) {
        const size_t a = vertices[k];
        const size_t b = vertices[(k + 1) % m];

        if ((a == u && b == v) || (a == v && b == u))
            return k;
    }

    return 0;
}

double vidy_first_pin_offset(int count, double pitch) {

    if (count % 2 == 0)
        return static_cast<double>(count / 2 - 1) * pitch + 0.5 * pitch;

    return static_cast<double>((count - 1) / 2) * pitch;
}

laika_cpp::Point vidy_align_top_ref(
    const laika_cpp::Point& m_b0,
    const laika_cpp::Point& top_start,
    const laika_cpp::Point& top_end,
    const laika_cpp::Vector& nf,
    const laika_cpp::Vector& v_ins
) {

    const laika_cpp::Line top_line = laika_cpp::Line::from_points(top_start, top_end);
    const laika_cpp::Vector w_p = nf - v_ins * nf.dot(v_ins);

    if (w_p.magnitude() < 1e-6)
        return top_line.closest_point(m_b0, false).second;

    return laika_cpp::Intersection::line_plane(
               top_line, laika_cpp::Plane::from_point_normal(m_b0, w_p.normalized()), false
    )
        .value_or(top_line.closest_point(m_b0, false).second);
}

double vidy_band_half_span(
    const std::array<laika_cpp::Point, 4>& ends,
    const laika_cpp::Point& ref_pt,
    const laika_cpp::Vector& v_ins,
    double corner_keep
) {

    const laika_cpp::Vector nf_b = (ends[1] - ends[0]).normalized();
    const laika_cpp::Vector nf_t = (ends[3] - ends[2]).normalized();
    const double len_b = ends[0].distance(ends[1]);
    const double len_t = ends[2].distance(ends[3]);
    const laika_cpp::Point m_b0 =
        laika_cpp::Line::from_points(ends[0], ends[1]).closest_point(ref_pt, false).second;
    const laika_cpp::Point m_t0 = vidy_align_top_ref(m_b0, ends[2], ends[3], nf_b, v_ins);
    const double sb = (m_b0 - ends[0]).dot(nf_b);
    const double st = (m_t0 - ends[2]).dot(nf_t);

    return std::min({sb, len_b - sb, st, len_t - st}) - corner_keep;
}

std::vector<laika_cpp::Point> vidy_double_pin(
    const laika_cpp::Point& pm,
    const laika_cpp::Vector& nf,
    const laika_cpp::Vector& v_ins,
    double le_tab,
    double le_offs,
    double scale,
    std::pair<laika_cpp::Point, laika_cpp::Point>& out_base,
    std::pair<laika_cpp::Point, laika_cpp::Point>& out_tip
) {

    const laika_cpp::Point mi1 = pm - nf * (135.0 * scale);
    const laika_cpp::Point pl1 = pm + nf * (135.0 * scale);
    const laika_cpp::Point mi2 = mi1 + v_ins * (0.30 * le_offs);
    const laika_cpp::Point pl2 = pl1 + v_ins * (0.30 * le_offs);
    const laika_cpp::Point mi3 = (pm - nf * (125.0 * scale)) + v_ins * le_offs;
    const laika_cpp::Point pl3 = (pm + nf * (125.0 * scale)) + v_ins * le_offs;
    const laika_cpp::Point mi4 = mi3 + nf * (35.0 * scale);
    const laika_cpp::Point pl4 = pl3 - nf * (35.0 * scale);
    const laika_cpp::Point mi5 = mi4 + v_ins * (1.20 * le_tab);
    const laika_cpp::Point pl5 = pl4 + v_ins * (1.20 * le_tab);
    const laika_cpp::Point mi6 = (mi4 + v_ins * (2.00 * le_tab)) + nf * (20.0 * scale);
    const laika_cpp::Point pl6 = (pl4 + v_ins * (2.00 * le_tab)) - nf * (20.0 * scale);
    out_base = {mi1, pl1};
    out_tip = {mi4, pl4};

    return {mi1, mi2, mi3, mi4, mi5, mi6, pl6, pl5, pl4, pl3, pl2, pl1};
}

std::vector<laika_cpp::Point> vidy_single_pin(
    const laika_cpp::Point& pm,
    const laika_cpp::Vector& nf,
    const laika_cpp::Vector& v_ins,
    double le_tab,
    double scale,
    std::pair<laika_cpp::Point, laika_cpp::Point>& out_shaft
) {

    const laika_cpp::Point mi1 = pm - nf * (90.0 * scale);
    const laika_cpp::Point pl1 = pm + nf * (90.0 * scale);
    const laika_cpp::Point mi2 = mi1 + v_ins * (1.20 * le_tab);
    const laika_cpp::Point pl2 = pl1 + v_ins * (1.20 * le_tab);
    const laika_cpp::Point mi3 = (mi1 + v_ins * (2.00 * le_tab)) + nf * (20.0 * scale);
    const laika_cpp::Point pl3 = (pl1 + v_ins * (2.00 * le_tab)) - nf * (20.0 * scale);
    out_shaft = {mi1, pl1};

    return {mi1, mi2, mi3, pl3, pl2, pl1};
}

VidyMortiseSlot vidy_project_slot(
    const std::pair<laika_cpp::Point, laika_cpp::Point>& shaft_bot,
    const std::pair<laika_cpp::Point, laika_cpp::Point>& shaft_top,
    const laika_cpp::Vector& v_ins,
    const laika_cpp::Plane& p_bot,
    const laika_cpp::Plane& p_top
) {

    const std::array<laika_cpp::Point, 4> arris = {
        shaft_bot.first, shaft_bot.second, shaft_top.second, shaft_top.first
    };
    VidyMortiseSlot slot;

    for (size_t c = 0; c < 4; c++) {
        const laika_cpp::Line ray = laika_cpp::Line::from_point_and_vector(arris[c], v_ins);
        slot.bottom[c] =
            laika_cpp::Intersection::line_plane(ray, p_bot, false).value_or(arris[c]);
        slot.top[c] = laika_cpp::Intersection::line_plane(ray, p_top, false).value_or(arris[c]);
    }

    const laika_cpp::Point cb = laika_cpp::Point::centroid(
        {slot.bottom[0], slot.bottom[1], slot.bottom[2], slot.bottom[3]}
    );
    const laika_cpp::Point ct =
        laika_cpp::Point::centroid({slot.top[0], slot.top[1], slot.top[2], slot.top[3]});

    for (size_t c = 0; c < 4; c++) {
        slot.bottom[c] = slot.bottom[c] + (slot.bottom[c] - cb).normalized() * 0.5;
        slot.top[c] = slot.top[c] + (slot.top[c] - ct).normalized() * 0.5;
    }

    return slot;
}

VidyMortiseSlot vidy_project_facet_slot(
    const std::pair<laika_cpp::Point, laika_cpp::Point>& shaft_bot,
    const std::pair<laika_cpp::Point, laika_cpp::Point>& shaft_top,
    const laika_cpp::Vector& v_ins,
    const laika_cpp::Point& centroid,
    const laika_cpp::Vector& normal,
    double h_bot,
    double h_top
) {

    const laika_cpp::Plane p_bot =
        laika_cpp::Plane::from_point_normal(centroid + normal * h_bot, normal);
    const laika_cpp::Plane p_top =
        laika_cpp::Plane::from_point_normal(centroid + normal * h_top, normal);

    return vidy_project_slot(shaft_bot, shaft_top, v_ins, p_bot, p_top);
}

std::array<std::vector<laika_cpp::Point>, 2> vidy_emit_double_pins(
    const VidyPinBand& band,
    const laika_cpp::Point& fem_centroid,
    const laika_cpp::Vector& fem_normal,
    const std::array<double, 4>& heights,
    size_t same_layer,
    std::vector<VidyMortiseSlot>& same_slots,
    std::vector<VidyMortiseSlot>& opp_slots
) {

    const laika_cpp::Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const laika_cpp::Point m_b0 = laika_cpp::Line::from_points(band.ends[0], band.ends[1])
                                        .closest_point(band.ref_pt, false)
                                        .second;
    const laika_cpp::Point m_t0 =
        vidy_align_top_ref(m_b0, band.ends[2], band.ends[3], nf, band.v_ins);
    const laika_cpp::Point pm_b = m_b0 - band.v_ins * (band.step_bot ? band.le_tab + 0.5 : 0.5);
    const laika_cpp::Point pm_t = m_t0 - band.v_ins * (band.step_bot ? 0.5 : band.le_tab + 0.5);
    const double sc = band.scale > 0.0 ? band.scale : std::min(1.0, band.pitch / 500.0);
    const double f_off = vidy_first_pin_offset(band.count, band.pitch);
    const double sh_half = std::max(
        0.5 * static_cast<double>(band.count) * band.pitch + band.pocket_extra,
        f_off + std::abs(band.shift) + 145.0 * sc
    );
    const size_t opp_layer = 1 - same_layer;
    std::vector<laika_cpp::Point> bot = {m_b0 - nf * sh_half, pm_b - nf * sh_half};
    std::vector<laika_cpp::Point> top = {m_t0 - nf * sh_half, pm_t - nf * sh_half};
    const laika_cpp::Point ptc_b = (pm_b + nf * band.shift) - nf * f_off;
    const laika_cpp::Point ptc_t = (pm_t + nf * band.shift) - nf * f_off;

    for (int i = 0; i < band.count; i++) {
        const double step = band.pitch * static_cast<double>(i);
        std::pair<laika_cpp::Point, laika_cpp::Point> base_b;
        std::pair<laika_cpp::Point, laika_cpp::Point> tip_b;
        std::pair<laika_cpp::Point, laika_cpp::Point> base_t;
        std::pair<laika_cpp::Point, laika_cpp::Point> tip_t;
        const std::vector<laika_cpp::Point> pb = vidy_double_pin(
            ptc_b + nf * step, nf, band.v_ins, band.le_tab, band.le_offs, sc, base_b, tip_b
        );
        const std::vector<laika_cpp::Point> pt = vidy_double_pin(
            ptc_t + nf * step, nf, band.v_ins, band.le_tab, band.le_offs, sc, base_t, tip_t
        );
        bot.insert(bot.end(), pb.begin(), pb.end());
        top.insert(top.end(), pt.begin(), pt.end());
        same_slots.push_back(vidy_project_facet_slot(
            base_b,
            base_t,
            band.v_ins,
            fem_centroid,
            fem_normal,
            heights[2 * same_layer],
            heights[2 * same_layer + 1]
        ));
        opp_slots.push_back(vidy_project_facet_slot(
            tip_b,
            tip_t,
            band.v_ins,
            fem_centroid,
            fem_normal,
            heights[2 * opp_layer],
            heights[2 * opp_layer + 1]
        ));
    }

    bot.push_back(pm_b + nf * sh_half);
    bot.push_back(m_b0 + nf * sh_half);
    top.push_back(pm_t + nf * sh_half);
    top.push_back(m_t0 + nf * sh_half);

    return {bot, top};
}

std::array<std::vector<laika_cpp::Point>, 2> vidy_emit_single_pins(
    const VidyPinBand& band,
    const laika_cpp::Point& fem_centroid,
    const laika_cpp::Vector& fem_normal,
    double h_bot,
    double h_top,
    std::vector<VidyMortiseSlot>& slots
) {

    const laika_cpp::Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const laika_cpp::Point m_b0 = laika_cpp::Line::from_points(band.ends[0], band.ends[1])
                                        .closest_point(band.ref_pt, false)
                                        .second;
    const laika_cpp::Point m_t0 =
        vidy_align_top_ref(m_b0, band.ends[2], band.ends[3], nf, band.v_ins);
    const laika_cpp::Point pm_b = m_b0 - band.v_ins * (band.step_bot ? band.le_tab + 0.5 : 0.5);
    const laika_cpp::Point pm_t = m_t0 - band.v_ins * (band.step_bot ? 0.5 : band.le_tab + 0.5);
    const double sc = band.scale > 0.0 ? band.scale : std::min(1.0, band.pitch / 500.0);
    const double f_off = vidy_first_pin_offset(band.count, band.pitch);
    const double sh_half = std::max(
        0.5 * static_cast<double>(band.count) * band.pitch + band.pocket_extra,
        f_off + std::abs(band.shift) + 100.0 * sc
    );
    std::vector<laika_cpp::Point> bot = {m_b0 - nf * sh_half, pm_b - nf * sh_half};
    std::vector<laika_cpp::Point> top = {m_t0 - nf * sh_half, pm_t - nf * sh_half};
    const laika_cpp::Point ptc_b = (pm_b + nf * band.shift) - nf * f_off;
    const laika_cpp::Point ptc_t = (pm_t + nf * band.shift) - nf * f_off;

    for (int i = 0; i < band.count; i++) {
        const double step = band.pitch * static_cast<double>(i);
        std::pair<laika_cpp::Point, laika_cpp::Point> sh_b;
        std::pair<laika_cpp::Point, laika_cpp::Point> sh_t;
        const std::vector<laika_cpp::Point> pb =
            vidy_single_pin(ptc_b + nf * step, nf, band.v_ins, band.le_tab, sc, sh_b);
        const std::vector<laika_cpp::Point> pt =
            vidy_single_pin(ptc_t + nf * step, nf, band.v_ins, band.le_tab, sc, sh_t);
        bot.insert(bot.end(), pb.begin(), pb.end());
        top.insert(top.end(), pt.begin(), pt.end());
        slots.push_back(
            vidy_project_facet_slot(sh_b, sh_t, band.v_ins, fem_centroid, fem_normal, h_bot, h_top)
        );
    }

    bot.push_back(pm_b + nf * sh_half);
    bot.push_back(m_b0 + nf * sh_half);
    top.push_back(pm_t + nf * sh_half);
    top.push_back(m_t0 + nf * sh_half);

    return {bot, top};
}

std::array<std::vector<laika_cpp::Point>, 2> vidy_emit_extend_pins(
    const VidyPinBand& band,
    const laika_cpp::Point& fem_centroid,
    const laika_cpp::Vector& fem_normal,
    double h_bot,
    double h_top,
    std::vector<VidyMortiseSlot>& slots
) {

    const laika_cpp::Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const laika_cpp::Point m_b0 = laika_cpp::Line::from_points(band.ends[0], band.ends[1])
                                        .closest_point(band.ref_pt, false)
                                        .second +
                                    nf * band.shift;
    const laika_cpp::Point m_t0 =
        vidy_align_top_ref(m_b0, band.ends[2], band.ends[3], nf, band.v_ins);
    const laika_cpp::Point pm_b = m_b0 - band.v_ins * (band.step_bot ? band.le_tab + 0.5 : 0.5);
    const laika_cpp::Point pm_t = m_t0 - band.v_ins * (band.step_bot ? 0.5 : band.le_tab + 0.5);
    const double span = 0.5 * static_cast<double>(band.count) * band.pitch + band.pocket_extra;
    const double f_off = vidy_first_pin_offset(band.count, band.pitch);
    const laika_cpp::Point p3_b = (pm_b - nf * span) + band.v_ins * band.le_offs;
    const laika_cpp::Point p4_b = (pm_b + nf * span) + band.v_ins * band.le_offs;
    const laika_cpp::Point p3_t = (pm_t - nf * span) + band.v_ins * band.le_offs;
    const laika_cpp::Point p4_t = (pm_t + nf * span) + band.v_ins * band.le_offs;
    std::vector<laika_cpp::Point> bot = {m_b0 - nf * span, p3_b};
    std::vector<laika_cpp::Point> top = {m_t0 - nf * span, p3_t};
    const double sc = band.scale > 0.0 ? band.scale : std::min(1.0, band.pitch / 500.0);
    const laika_cpp::Point ptc_b = laika_cpp::Point::mid_point(p3_b, p4_b) - nf * f_off;
    const laika_cpp::Point ptc_t = laika_cpp::Point::mid_point(p3_t, p4_t) - nf * f_off;

    for (int i = 0; i < band.count; i++) {
        const double step = band.pitch * static_cast<double>(i);
        std::pair<laika_cpp::Point, laika_cpp::Point> sh_b;
        std::pair<laika_cpp::Point, laika_cpp::Point> sh_t;
        const std::vector<laika_cpp::Point> pb =
            vidy_single_pin(ptc_b + nf * step, nf, band.v_ins, band.le_tab, sc, sh_b);
        const std::vector<laika_cpp::Point> pt =
            vidy_single_pin(ptc_t + nf * step, nf, band.v_ins, band.le_tab, sc, sh_t);
        bot.insert(bot.end(), pb.begin(), pb.end());
        top.insert(top.end(), pt.begin(), pt.end());
        slots.push_back(
            vidy_project_facet_slot(sh_b, sh_t, band.v_ins, fem_centroid, fem_normal, h_bot, h_top)
        );
    }

    bot.push_back(p4_b);
    bot.push_back(m_b0 + nf * span);
    top.push_back(p4_t);
    top.push_back(m_t0 + nf * span);

    return {bot, top};
}

std::array<std::vector<laika_cpp::Point>, 2>
vidy_emit_extend_lip(const VidyPinBand& band, bool is_shelf) {

    const laika_cpp::Vector nf = (band.ends[1] - band.ends[0]).normalized();
    const laika_cpp::Point m_b0 = laika_cpp::Line::from_points(band.ends[0], band.ends[1])
                                        .closest_point(band.ref_pt, false)
                                        .second +
                                    nf * band.shift;
    const laika_cpp::Point m_t0 =
        vidy_align_top_ref(m_b0, band.ends[2], band.ends[3], nf, band.v_ins);
    const double span =
        0.5 * static_cast<double>(band.count) * band.pitch + band.pocket_extra - 0.5;
    const double ext_b = is_shelf ? (band.le_offs - (band.step_bot ? band.le_tab : 0.0) - 0.5)
                                  : (band.le_tab * (band.step_bot ? 2.0 : 1.0) - 0.5);
    const double ext_t = is_shelf ? (band.le_offs - (band.step_bot ? 0.0 : band.le_tab) - 0.5)
                                  : (band.le_tab * (band.step_bot ? 1.0 : 2.0) - 0.5);
    const std::vector<laika_cpp::Point> bot = {
        m_b0 - nf * span,
        (m_b0 - nf * span) + band.v_ins * ext_b,
        (m_b0 + nf * span) + band.v_ins * ext_b,
        m_b0 + nf * span
    };
    const std::vector<laika_cpp::Point> top = {
        m_t0 - nf * span,
        (m_t0 - nf * span) + band.v_ins * ext_t,
        (m_t0 + nf * span) + band.v_ins * ext_t,
        m_t0 + nf * span
    };

    return {bot, top};
}

bool vidy_quads_closer_than_2d(
    const laika_cpp::Point (&qa)[4],
    const laika_cpp::Point (&qb)[4],
    const laika_cpp::Vector& normal,
    double min_gap
) {

    const laika_cpp::Plane frame = laika_cpp::Plane::from_point_normal(qa[0], normal);
    const laika_cpp::Vector ex = frame.x_axis();
    const laika_cpp::Vector ey = frame.y_axis();
    std::pair<double, double> a2[4];
    std::pair<double, double> b2[4];

    for (size_t i = 0; i < 4; i++) {
        const laika_cpp::Vector da = qa[i] - qa[0];
        const laika_cpp::Vector db = qb[i] - qa[0];
        a2[i] = {da.dot(ex), da.dot(ey)};
        b2[i] = {db.dot(ex), db.dot(ey)};
    }

    for (int which = 0; which < 2; which++) {
        const std::pair<double, double>* poly = which == 0 ? a2 : b2;

        for (size_t i = 0; i < 4; i++) {
            const double dx = poly[(i + 1) % 4].first - poly[i].first;
            const double dy = poly[(i + 1) % 4].second - poly[i].second;
            const double len = std::hypot(dx, dy);

            if (len < 1e-9)
                continue;

            const double nx = -dy / len;
            const double ny = dx / len;
            double min_a = 1e18;
            double max_a = -1e18;
            double min_b = 1e18;
            double max_b = -1e18;

            for (size_t c = 0; c < 4; c++) {
                const double pa = a2[c].first * nx + a2[c].second * ny;
                const double pb = b2[c].first * nx + b2[c].second * ny;
                min_a = std::min(min_a, pa);
                max_a = std::max(max_a, pa);
                min_b = std::min(min_b, pb);
                max_b = std::max(max_b, pb);
            }

            if (max_a + min_gap <= min_b || max_b + min_gap <= min_a)
                return false;
        }
    }

    return true;
}

std::pair<double, double> vidy_plane_corner_snips(
    const laika_cpp::Point& apex,
    const laika_cpp::Vector& ua,
    const laika_cpp::Vector& ub,
    const laika_cpp::Plane& limit_plane,
    double clearance,
    double min_cos,
    double max_snip
) {

    const double raw = limit_plane.signed_distance(apex);
    const double ra = ua.dot(limit_plane.z_axis());
    const double rb = ub.dot(limit_plane.z_axis());

    if (raw <= 0.02 || std::max(ra, rb) < min_cos)
        return {0.0, 0.0};

    const double sd = raw + clearance;
    const double cut_cos = 0.8 * min_cos;

    return {
        ra >= cut_cos ? std::min(max_snip, sd / ra) : 0.0,
        rb >= cut_cos ? std::min(max_snip, sd / rb) : 0.0
    };
}

std::pair<laika_cpp::Polyline, laika_cpp::Polyline> vidy_stitch_layer_rings(
    const std::array<std::vector<laika_cpp::Point>, 4>& miter_rings,
    const laika_cpp::Vector& normal,
    size_t h_bot,
    size_t h_top,
    const std::vector<std::pair<double, double>>& snips_b,
    const std::vector<std::pair<double, double>>& snips_t,
    const std::vector<std::vector<laika_cpp::Point>>& prof_bot,
    const std::vector<std::vector<laika_cpp::Point>>& prof_top,
    double min_lim,
    double prof_frac) {

    const size_t m = miter_rings[h_bot].size();
    std::vector<laika_cpp::Point> pts_b;
    std::vector<laika_cpp::Point> pts_t;
    pts_b.reserve(64);
    pts_t.reserve(64);

    for (size_t k = 0; k < m; k++) {
        const laika_cpp::Point& apex_b = miter_rings[h_bot][k];
        const laika_cpp::Point& apex_t = miter_rings[h_top][k];
        const laika_cpp::Point& raw_prev_b = miter_rings[h_bot][(k + m - 1) % m];
        const laika_cpp::Point& raw_next_b = miter_rings[h_bot][(k + 1) % m];
        const laika_cpp::Point& raw_prev_t = miter_rings[h_top][(k + m - 1) % m];
        const laika_cpp::Point& raw_next_t = miter_rings[h_top][(k + 1) % m];
        const laika_cpp::Point prev_b =
            prof_bot[(k + m - 1) % m].empty() ? raw_prev_b : prof_bot[(k + m - 1) % m].back();
        const laika_cpp::Point next_b = prof_bot[k].empty() ? raw_next_b : prof_bot[k].front();
        const laika_cpp::Point prev_t =
            prof_top[(k + m - 1) % m].empty() ? raw_prev_t : prof_top[(k + m - 1) % m].back();
        const laika_cpp::Point next_t = prof_top[k].empty() ? raw_next_t : prof_top[k].front();
        const std::pair<double, double>& sb = snips_b[k];
        const std::pair<double, double>& st = snips_t[k];

        if (std::max({sb.first, sb.second, st.first, st.second}) < 0.5) {
            pts_b.push_back(apex_b);
            pts_t.push_back(apex_t);
        } else {
            const laika_cpp::Vector uab = (apex_b - prev_b).normalized();
            const laika_cpp::Vector ubb = (apex_b - next_b).normalized();
            const laika_cpp::Vector uat = (apex_t - prev_t).normalized();
            const laika_cpp::Vector ubt = (apex_t - next_t).normalized();
            const double cb1 = std::max(sb.first, 0.35 * st.first);
            const double cb2 = std::max(sb.second, 0.35 * st.second);
            const double ct1 = std::max(st.first, 0.35 * sb.first);
            const double ct2 = std::max(st.second, 0.35 * sb.second);
            const double lim_b1 = std::max(
                min_lim,
                std::min(0.35 * apex_b.distance(raw_prev_b), prof_frac * apex_b.distance(prev_b))
            );
            const double lim_b2 = std::max(
                min_lim,
                std::min(0.35 * apex_b.distance(raw_next_b), prof_frac * apex_b.distance(next_b))
            );
            const double lim_t1 = std::max(
                min_lim,
                std::min(0.35 * apex_t.distance(raw_prev_t), prof_frac * apex_t.distance(prev_t))
            );
            const double lim_t2 = std::max(
                min_lim,
                std::min(0.35 * apex_t.distance(raw_next_t), prof_frac * apex_t.distance(next_t))
            );
            pts_b.push_back(apex_b - uab * std::clamp(cb1, 2.0, lim_b1));
            pts_b.push_back(apex_b - ubb * std::clamp(cb2, 2.0, lim_b2));
            pts_t.push_back(apex_t - uat * std::clamp(ct1, 2.0, lim_t1));
            pts_t.push_back(apex_t - ubt * std::clamp(ct2, 2.0, lim_t2));
        }

        pts_b.insert(pts_b.end(), prof_bot[k].begin(), prof_bot[k].end());
        pts_t.insert(pts_t.end(), prof_top[k].begin(), prof_top[k].end());
    }

    if (wood_session::compute_newell(pts_b).dot(normal) > 0.0) {
        std::reverse(pts_b.begin(), pts_b.end());
        std::reverse(pts_t.begin(), pts_t.end());
    }

    pts_b.push_back(pts_b.front());
    pts_t.push_back(pts_t.front());

    return {laika_cpp::Polyline(pts_b), laika_cpp::Polyline(pts_t)};
}

void vidy_attach_plate_features(
    wood_session::FacePlanarElement& plate,
    const laika_cpp::Vector& normal,
    const laika_cpp::Polyline& bot_outline,
    const laika_cpp::Polyline& top_outline,
    const std::vector<VidyMortiseSlot>& slots
) {

    plate.features.bottom = {bot_outline};
    plate.features.top = {top_outline};

    for (const VidyMortiseSlot& slot : slots) {
        std::vector<laika_cpp::Point> hb(std::begin(slot.bottom), std::end(slot.bottom));
        std::vector<laika_cpp::Point> ht(std::begin(slot.top), std::end(slot.top));

        if (wood_session::compute_newell(hb).dot(normal) < 0.0) {
            std::reverse(hb.begin(), hb.end());
            std::reverse(ht.begin(), ht.end());
        }

        hb.push_back(hb.front());
        ht.push_back(ht.front());
        plate.features.bottom.push_back(laika_cpp::Polyline(hb));
        plate.features.top.push_back(laika_cpp::Polyline(ht));
    }

    plate.invalidate_geometry();
}

}
