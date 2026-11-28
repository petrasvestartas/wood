#include "pch.h"
#include "src/templates/delorme/delorme_guide.h"

namespace wood_delorme {

namespace {

DelormeRibCurve make_circular_arch_rib(
    const std::string& name,
    const Point& center_xy,
    const Vector& u_axis,
    double half_span,
    double rise,
    int samples = 96
) {
    DelormeRibCurve rib;
    rib.name = name;
    rib.origin = center_xy;
    rib.u_axis = u_axis.normalized();
    rib.normal = rib.u_axis.cross(Vector::z_axis()).normalized();

    const double h = std::max(rise, 80.0);
    const double R = (half_span * half_span + h * h) / (2.0 * h);
    const double z_center = h - R;
    const double max_a = std::asin(std::clamp(half_span / R, -0.999999, 0.999999));

    rib.pts.reserve(static_cast<size_t>(samples + 1));
    rib.tangents.reserve(static_cast<size_t>(samples + 1));
    rib.radials.reserve(static_cast<size_t>(samples + 1));
    rib.arc_s.assign(static_cast<size_t>(samples + 1), 0.0);

    for (int i = 0; i <= samples; i++) {
        const double a = -max_a + 2.0 * max_a * static_cast<double>(i) / samples;
        const double u = R * std::sin(a);
        const double z = z_center + R * std::cos(a);
        rib.pts.push_back(center_xy + rib.u_axis * u + Vector(0.0, 0.0, z));
        rib.tangents.push_back((rib.u_axis * std::cos(a) - Vector(0.0, 0.0, std::sin(a))).normalized());
        rib.radials.push_back((rib.u_axis * std::sin(a) + Vector(0.0, 0.0, std::cos(a))).normalized());
        if (i > 0)
            rib.arc_s[static_cast<size_t>(i)] =
                rib.arc_s[static_cast<size_t>(i - 1)]
                + Point::distance(rib.pts[static_cast<size_t>(i - 1)], rib.pts[static_cast<size_t>(i)]);
    }
    return rib;
}

/// Pointed "comble a l'imperiale" (carene de navire) rib built from two circular arcs meeting at a pointed ridge.
DelormeRibCurve make_pointed_imperiale_rib(
    const std::string& name,
    const Point& center_xy,
    const Vector& u_axis,
    double half_span,
    double rise,
    int half_samples = 56
) {
    DelormeRibCurve rib;
    rib.name = name;
    rib.origin = center_xy;
    rib.u_axis = u_axis.normalized();
    rib.normal = rib.u_axis.cross(Vector::z_axis()).normalized();

    const double u_c = (rise * rise - half_span * half_span) / (2.0 * half_span);
    const double R = half_span + u_c;
    const double top_a = std::asin(std::clamp(rise / R, 0.05, 0.999999));

    for (int i = 0; i <= half_samples; i++) {
        const double a = top_a * static_cast<double>(i) / half_samples;
        const double u = u_c - R * std::cos(a);
        const double z = R * std::sin(a);
        rib.pts.push_back(center_xy + rib.u_axis * u + Vector(0.0, 0.0, z));
        rib.tangents.push_back((rib.u_axis * std::sin(a) + Vector(0.0, 0.0, std::cos(a))).normalized());
        rib.radials.push_back((rib.u_axis * (-std::cos(a)) + Vector(0.0, 0.0, std::sin(a))).normalized());
    }
    const size_t crown_idx = rib.pts.size() - 1;
    for (int i = 1; i <= half_samples; i++) {
        const double a = top_a * (1.0 - static_cast<double>(i) / half_samples);
        const double u = -u_c + R * std::cos(a);
        const double z = R * std::sin(a);
        rib.pts.push_back(center_xy + rib.u_axis * u + Vector(0.0, 0.0, z));
        rib.tangents.push_back((rib.u_axis * std::sin(a) - Vector(0.0, 0.0, std::cos(a))).normalized());
        rib.radials.push_back((rib.u_axis * std::cos(a) + Vector(0.0, 0.0, std::sin(a))).normalized());
    }

    rib.arc_s.assign(rib.pts.size(), 0.0);
    for (size_t i = 1; i < rib.pts.size(); i++)
        rib.arc_s[i] = rib.arc_s[i - 1] + Point::distance(rib.pts[i - 1], rib.pts[i]);
    rib.kink_s = rib.arc_s[crown_idx];
    return rib;
}

/// Quarter-circle meridian rib rising from radius R_base at z = 0 to oculus radius R_oculus (Halle au Ble cupola).
DelormeRibCurve make_dome_meridian_rib(
    const std::string& name,
    double azimuth_rad,
    double R_base,
    double rise,
    double R_oculus,
    int samples = 64
) {
    DelormeRibCurve rib;
    rib.name = name;
    rib.origin = Point(0.0, 0.0, 0.0);
    rib.u_axis = Vector(std::cos(azimuth_rad), std::sin(azimuth_rad), 0.0);
    rib.normal = rib.u_axis.cross(Vector::z_axis()).normalized();

    const double phi_end = std::acos(std::clamp(R_oculus / R_base, 0.08, 0.92));
    for (int i = 0; i <= samples; i++) {
        const double phi = phi_end * static_cast<double>(i) / samples;
        const double r = R_base * std::cos(phi);
        const double z = rise * std::sin(phi);
        const Vector dr = rib.u_axis * (-R_base * std::sin(phi)) + Vector(0.0, 0.0, rise * std::cos(phi));
        const Vector ut = dr.normalized();
        const Vector ur = rib.normal.cross(ut).normalized();
        rib.pts.push_back(rib.origin + rib.u_axis * r + Vector(0.0, 0.0, z));
        rib.tangents.push_back(ut);
        rib.radials.push_back(ur);
    }
    rib.arc_s.assign(rib.pts.size(), 0.0);
    for (size_t i = 1; i < rib.pts.size(); i++)
        rib.arc_s[i] = rib.arc_s[i - 1] + Point::distance(rib.pts[i - 1], rib.pts[i]);
    return rib;
}

} // namespace

double DelormeRibCurve::total_length() const {
    return arc_s.empty() ? 0.0 : arc_s.back();
}

void DelormeRibCurve::evaluate(double s, Point& out_pt, Vector& out_tan, Vector& out_rad) const {

    if (pts.empty()) {
        out_pt = origin;
        out_tan = u_axis;
        out_rad = Vector::z_axis();
        return;
    }
    const double total = total_length();
    const double clamped = std::clamp(s, 0.0, total);
    const auto it = std::upper_bound(arc_s.begin(), arc_s.end(), clamped);
    const size_t idx = std::min<size_t>(
        pts.size() >= 2 ? pts.size() - 2 : 0,
        static_cast<size_t>(std::max<std::ptrdiff_t>(0, (it - arc_s.begin()) - 1))
    );
    if (pts.size() < 2) {
        out_pt = pts[0];
        out_tan = tangents[0];
        out_rad = radials[0];
        return;
    }
    const double ds = std::max(1e-9, arc_s[idx + 1] - arc_s[idx]);
    const double t = (clamped - arc_s[idx]) / ds;
    out_pt = pts[idx] + (pts[idx + 1] - pts[idx]) * t;
    if (kink_s > 0.0 && std::abs(clamped - kink_s) < 1e-4) {
        out_tan = tangents[idx];
        out_rad = radials[idx];
    } else {
        out_tan = (tangents[idx] * (1.0 - t) + tangents[idx + 1] * t).normalized();
        out_rad = normal.cross(out_tan).normalized();
    }
}

DelormeGuide::DelormeGuide(const DelormeParams& p) : params(p) {
    build_default_ribs();
    build_planks_and_fasteners();
}

DelormeGuide::DelormeGuide(const DelormeParams& p, const std::vector<DelormeRibCurve>& custom_ribs)
    : params(p), _ribs(custom_ribs) {
    this->params.kind = DelormeKind::CustomRibs;
    build_planks_and_fasteners();
}

DelormeGuide DelormeGuide::barrel(
    double span,
    double rise,
    double length,
    int n_ribs,
    int n_plies,
    double plank_length,
    double rib_depth,
    double plank_thickness
) {
    DelormeParams p;
    p.kind = DelormeKind::SemicircularBarrel;
    p.span = span;
    p.rise = rise;
    p.length = length;
    p.n_ribs = std::max(2, n_ribs);
    p.n_plies = std::clamp(n_plies, 2, 4);
    p.plank_length = plank_length;
    p.rib_depth = rib_depth;
    p.plank_thickness = plank_thickness;
    return DelormeGuide(p);
}

DelormeGuide DelormeGuide::pointed_imperiale(
    double span,
    double rise,
    double length,
    int n_ribs,
    int n_plies,
    double plank_length,
    double rib_depth,
    double plank_thickness
) {
    DelormeParams p;
    p.kind = DelormeKind::PointedImperiale;
    p.span = span;
    p.rise = rise;
    p.length = length;
    p.n_ribs = std::max(2, n_ribs);
    p.n_plies = std::clamp(n_plies, 2, 4);
    p.plank_length = plank_length;
    p.rib_depth = rib_depth;
    p.plank_thickness = plank_thickness;
    return DelormeGuide(p);
}

DelormeGuide DelormeGuide::dome(
    double diameter,
    double oculus_diameter,
    int n_ribs,
    int n_plies,
    double plank_length,
    double rib_depth,
    double plank_thickness
) {
    DelormeParams p;
    p.kind = DelormeKind::HemisphericalDome;
    p.span = diameter;
    p.rise = 0.5 * diameter;
    p.oculus_radius = 0.5 * oculus_diameter;
    p.n_ribs = std::max(6, n_ribs - (n_ribs % 2));
    p.n_plies = std::clamp(n_plies, 2, 3);
    p.plank_length = plank_length;
    p.rib_depth = rib_depth;
    p.plank_thickness = plank_thickness;
    p.lierne_overhang = 115.0;
    return DelormeGuide(p);
}

DelormeGuide DelormeGuide::crossed_vault(
    double span,
    double rise,
    int ribs_per_axis,
    int n_plies,
    double plank_length,
    double rib_depth,
    double plank_thickness
) {
    DelormeParams p;
    p.kind = DelormeKind::CrossedGroin;
    p.span = span;
    p.rise = rise;
    p.n_ribs = std::max(2, ribs_per_axis);
    p.n_plies = std::clamp(n_plies, 2, 3);
    p.plank_length = plank_length;
    p.rib_depth = rib_depth;
    p.plank_thickness = plank_thickness;
    return DelormeGuide(p);
}

double DelormeGuide::total_rib_thickness() const {
    return params.n_plies * params.plank_thickness
           + std::max(0, params.n_plies - 1) * params.ply_gap;
}

void DelormeGuide::build_default_ribs() {

    _ribs.clear();
    const int nr = std::max(2, params.n_ribs);
    const double half_span = 0.5 * params.span;

    if (params.kind == DelormeKind::SemicircularBarrel || params.kind == DelormeKind::PointedImperiale) {
        for (int r = 0; r < nr; r++) {
            const double y = nr == 1
                                 ? 0.0
                                 : -0.5 * params.length
                                       + params.length * static_cast<double>(r) / (nr - 1);
            if (params.kind == DelormeKind::SemicircularBarrel) {
                _ribs.push_back(make_circular_arch_rib(
                    fmt::format("rib_{}", r), Point(0.0, y, 0.0), Vector::x_axis(), half_span, params.rise
                ));
            } else {
                _ribs.push_back(make_pointed_imperiale_rib(
                    fmt::format("rib_{}", r), Point(0.0, y, 0.0), Vector::x_axis(), half_span, params.rise
                ));
            }
        }
        return;
    }

    if (params.kind == DelormeKind::HemisphericalDome) {
        for (int r = 0; r < nr; r++) {
            const double az = Tolerance::TWO_PI * static_cast<double>(r) / nr;
            _ribs.push_back(make_dome_meridian_rib(
                fmt::format("meridian_rib_{}", r),
                az,
                half_span,
                params.rise,
                params.oculus_radius
            ));
        }
        return;
    }

    if (params.kind == DelormeKind::CrossedGroin) {
        const double ring_r = 260.0;
        const double portal_half = half_span * 0.84;
        const std::array<double, 2> portals = {-half_span, half_span};
        for (size_t i = 0; i < portals.size(); i++) {
            _ribs.push_back(make_circular_arch_rib(
                fmt::format("portal_x_{}", i),
                Point(0.0, portals[i], 0.0),
                Vector::x_axis(),
                portal_half,
                params.rise * 0.84
            ));
        }
        for (size_t i = 0; i < portals.size(); i++) {
            _ribs.push_back(make_circular_arch_rib(
                fmt::format("portal_y_{}", i),
                Point(portals[i], 0.0, 0.0),
                Vector::y_axis(),
                portal_half,
                params.rise * 0.84
            ));
        }
        const double diag_r = std::hypot(half_span, half_span);
        for (int d = 0; d < 4; d++) {
            const double az = 0.25 * Tolerance::PI + 0.5 * Tolerance::PI * d;
            _ribs.push_back(make_dome_meridian_rib(
                fmt::format("groin_diag_{}", d), az, diag_r, params.rise, ring_r, 56
            ));
        }
    }
}

void DelormeGuide::build_planks_and_fasteners() {

    _planks.clear();
    _liernes.clear();
    _chevilles.clear();
    if (_ribs.empty())
        return;

    const int n_plies = std::max(1, params.n_plies);
    const double pitch = params.plank_thickness + params.ply_gap;

    std::vector<std::vector<double>> rib_all_seams(_ribs.size());

    for (size_t r = 0; r < _ribs.size(); r++) {
        const DelormeRibCurve& rib = _ribs[r];
        const double L = rib.total_length();
        const std::vector<std::pair<double, double>> spans =
            rib.kink_s > 0.0
                ? std::vector<std::pair<double, double>>{{0.0, rib.kink_s}, {rib.kink_s, L}}
                : std::vector<std::pair<double, double>>{{0.0, L}};

        for (int p = 0; p < n_plies; p++) {
            const double lat = (static_cast<double>(p) - 0.5 * (n_plies - 1)) * pitch;
            size_t plank_counter = 0;

            for (size_t sp_idx = 0; sp_idx < spans.size(); sp_idx++) {
                const double s_lo = spans[sp_idx].first;
                const double s_hi = spans[sp_idx].second;
                const double sub_len = s_hi - s_lo;
                const int n_full = std::max(2, static_cast<int>(std::round(sub_len / params.plank_length)));
                const double step = sub_len / n_full;
                const double phase = (static_cast<double>(p) / n_plies) * step;

                std::vector<double> breaks;
                breaks.push_back(s_lo);
                if (phase > 0.22 * step) {
                    for (int k = 0; k < n_full; k++) {
                        const double sb = s_lo + phase + k * step;
                        if (sb > s_lo + 0.22 * step && sb < s_hi - 0.22 * step)
                            breaks.push_back(sb);
                    }
                } else {
                    for (int k = 1; k < n_full; k++)
                        breaks.push_back(s_lo + k * step);
                }
                breaks.push_back(s_hi);

                for (size_t k = 0; k + 1 < breaks.size(); k++) {
                    DelormePlankSpec ps;
                    ps.rib_index = r;
                    ps.ply_index = static_cast<size_t>(p);
                    ps.plank_index = plank_counter++;
                    ps.s0 = breaks[k];
                    ps.s1 = breaks[k + 1];
                    ps.lateral_offset = lat;
                    ps.has_start_notch = (k > 0) && (params.stub_tenon_length > 1.0);
                    ps.has_end_tenon = (k + 2 < breaks.size()) && (params.stub_tenon_length > 1.0);
                    ps.trim_start_horizontal =
                        params.flat_springing && (sp_idx == 0) && (k == 0);
                    ps.trim_end_horizontal =
                        params.flat_springing
                        && (params.kind != DelormeKind::HemisphericalDome)
                        && (params.kind != DelormeKind::CrossedGroin || r < 4)
                        && (sp_idx + 1 == spans.size())
                        && (k + 2 == breaks.size());
                    _planks.push_back(ps);

                    if (k > 0)
                        rib_all_seams[r].push_back(breaks[k]);
                }
            }
        }
    }

    const auto seam_clearance_at = [&](size_t r, double s) -> double {
        double best = std::min(s, _ribs[r].total_length() - s);
        if (_ribs[r].kink_s > 0.0)
            best = std::min(best, std::abs(s - _ribs[r].kink_s));
        for (const double bs : rib_all_seams[r])
            best = std::min(best, std::abs(s - bs));
        return best;
    };

    const double min_margin = params.stub_tenon_length + 0.8 * params.lierne_width + 22.0;
    const double rib_thick = total_rib_thickness();

    if (params.kind == DelormeKind::SemicircularBarrel || params.kind == DelormeKind::PointedImperiale
        || params.kind == DelormeKind::CustomRibs) {
        const double L0 = _ribs[0].total_length();
        const int n_stations = std::max(2, static_cast<int>(std::round(L0 / params.lierne_spacing)) - 1);
        std::vector<double> stations;
        for (int m = 1; m <= n_stations; m++) {
            double cand = L0 * static_cast<double>(m) / (n_stations + 1);
            double best_s = cand;
            double best_clear = -1.0;
            for (int step_i = -12; step_i <= 12; step_i++) {
                const double test_s = cand + step_i * 14.0;
                if (test_s < 140.0 || test_s > L0 - 140.0)
                    continue;
                double c_all = 1e9;
                for (size_t r = 0; r < _ribs.size(); r++)
                    c_all = std::min(c_all, seam_clearance_at(r, test_s));
                if (c_all > best_clear) {
                    best_clear = c_all;
                    best_s = test_s;
                }
            }
            if (best_clear >= 0.55 * min_margin)
                stations.push_back(best_s);
        }

        for (size_t idx = 0; idx < stations.size(); idx++) {
            const double s_val = stations[idx];
            Point p_first, p_last;
            Vector t0, rad0, t_last, rad_last;
            _ribs.front().evaluate(s_val, p_first, t0, rad0);
            _ribs.back().evaluate(s_val, p_last, t_last, rad_last);

            Vector axis_dir = (p_last - p_first).normalized();
            if (axis_dir.magnitude() < 1e-6)
                axis_dir = _ribs.front().normal * -1.0;
            const double ext = 0.5 * rib_thick + params.clavette_width + params.lierne_overhang;

            DelormeLierneSpec ls;
            ls.name = fmt::format("lierne_{}", idx);
            ls.start = p_first - axis_dir * ext;
            ls.end = p_last + axis_dir * ext;
            ls.axis_dir = axis_dir;
            ls.up_dir = (rad0 - axis_dir * rad0.dot(axis_dir)).normalized();
            ls.width_dir = ls.up_dir.cross(ls.axis_dir).normalized();

            for (size_t r = 0; r < _ribs.size(); r++) {
                Point pr;
                Vector tr, radr;
                _ribs[r].evaluate(s_val, pr, tr, radr);
                const double cos_a = std::max(0.35, std::abs(axis_dir.dot(_ribs[r].normal)));
                DelormeLiernePiercing pier;
                pier.rib_index = r;
                pier.rib_s = s_val;
                pier.t_center = (pr - ls.start).dot(axis_dir);
                pier.half_span_along_lierne =
                    (0.5 * rib_thick + 0.5 * params.clavette_thickness * std::sqrt(std::max(0.0, 1.0 - cos_a * cos_a)))
                    / cos_a;
                ls.piercings.push_back(pier);
            }
            _liernes.push_back(ls);
        }
    } else if (params.kind == DelormeKind::HemisphericalDome) {
        const double L0 = _ribs[0].total_length();
        const int n_rings = std::max(2, static_cast<int>(std::round(L0 / params.lierne_spacing)));
        size_t lierne_idx = 0;
        for (int ring = 0; ring < n_rings; ring++) {
            for (int parity = 0; parity < 2; parity++) {
                const double raw_s =
                    L0 * (0.20 + 0.64 * (static_cast<double>(ring) + 0.42 * parity) / n_rings);
                if (raw_s < 160.0 || raw_s > L0 - 150.0)
                    continue;
                double s_use = raw_s;
                double best_c = -1.0;
                for (int d = -10; d <= 10; d++) {
                    const double cand = raw_s + d * 12.0;
                    if (cand < 150.0 || cand > L0 - 140.0)
                        continue;
                    const double c = seam_clearance_at(0, cand);
                    if (c > best_c) {
                        best_c = c;
                        s_use = cand;
                    }
                }

                for (size_t r = static_cast<size_t>(parity); r < _ribs.size(); r += 2) {
                    const size_t r_next = (r + 1) % _ribs.size();
                    Point pa, pb;
                    Vector ta, rada, tb, radb;
                    _ribs[r].evaluate(s_use, pa, ta, rada);
                    _ribs[r_next].evaluate(s_use, pb, tb, radb);

                    const Vector axis_dir = (pb - pa).normalized();
                    const Vector mid_rad = (rada + radb).normalized();
                    const Vector up_dir = (mid_rad - axis_dir * mid_rad.dot(axis_dir)).normalized();
                    const double cos_a = std::max(0.4, std::abs(axis_dir.dot(_ribs[r].normal)));
                    const double half_span =
                        (0.5 * rib_thick
                         + 0.5 * params.clavette_thickness * std::sqrt(std::max(0.0, 1.0 - cos_a * cos_a)))
                        / cos_a;
                    const double ext = half_span + params.clavette_width + params.lierne_overhang;

                    DelormeLierneSpec ls;
                    ls.name = fmt::format("dome_lierne_{}", lierne_idx++);
                    ls.start = pa - axis_dir * ext;
                    ls.end = pb + axis_dir * ext;
                    ls.axis_dir = axis_dir;
                    ls.up_dir = up_dir;
                    ls.width_dir = up_dir.cross(axis_dir).normalized();

                    for (size_t end_i = 0; end_i < 2; end_i++) {
                        const size_t r_id = end_i == 0 ? r : r_next;
                        const Point& pr = end_i == 0 ? pa : pb;
                        DelormeLiernePiercing pier;
                        pier.rib_index = r_id;
                        pier.rib_s = s_use;
                        pier.t_center = (pr - ls.start).dot(axis_dir);
                        pier.half_span_along_lierne = half_span;
                        ls.piercings.push_back(pier);
                    }
                    _liernes.push_back(ls);
                }
            }
        }
    } else if (params.kind == DelormeKind::CrossedGroin && _ribs.size() >= 8) {
        struct PairLink {
            size_t r_a;
            double s_frac_a;
            size_t r_b;
            double s_frac_b;
        };
        const std::array<PairLink, 8> links = {{
            {0, 0.24, 6, 0.26},
            {0, 0.76, 7, 0.26},
            {1, 0.24, 5, 0.26},
            {1, 0.76, 4, 0.26},
            {2, 0.24, 6, 0.42},
            {2, 0.76, 5, 0.42},
            {3, 0.24, 7, 0.42},
            {3, 0.76, 4, 0.42},
        }};
        for (size_t k = 0; k < links.size(); k++) {
            const double sa = _ribs[links[k].r_a].total_length() * links[k].s_frac_a;
            const double sb = _ribs[links[k].r_b].total_length() * links[k].s_frac_b;
            Point pa, pb;
            Vector ta, rada, tb, radb;
            _ribs[links[k].r_a].evaluate(sa, pa, ta, rada);
            _ribs[links[k].r_b].evaluate(sb, pb, tb, radb);

            const Vector axis_dir = (pb - pa).normalized();
            const Vector mid_rad = (rada + radb).normalized();
            const Vector up_dir = (mid_rad - axis_dir * mid_rad.dot(axis_dir)).normalized();
            const double ca = std::max(0.35, std::abs(axis_dir.dot(_ribs[links[k].r_a].normal)));
            const double cb = std::max(0.35, std::abs(axis_dir.dot(_ribs[links[k].r_b].normal)));
            const double sa_sin = std::sqrt(std::max(0.0, 1.0 - ca * ca));
            const double sb_sin = std::sqrt(std::max(0.0, 1.0 - cb * cb));
            const double w_env = std::max(params.lierne_width, params.clavette_thickness);
            const double hsa = (0.5 * rib_thick + 0.5 * w_env * sa_sin) / ca + 3.0;
            const double hsb = (0.5 * rib_thick + 0.5 * w_env * sb_sin) / cb + 3.0;
            const double ext_a = hsa + params.clavette_width + params.lierne_overhang;
            const double ext_b = hsb + params.clavette_width + params.lierne_overhang;

            DelormeLierneSpec ls;
            ls.name = fmt::format("groin_lierne_{}", k);
            ls.start = pa - axis_dir * ext_a;
            ls.end = pb + axis_dir * ext_b;
            ls.axis_dir = axis_dir;
            ls.up_dir = up_dir;
            ls.width_dir = up_dir.cross(axis_dir).normalized();
            ls.piercings.push_back({links[k].r_a, sa, (pa - ls.start).dot(axis_dir), hsa});
            ls.piercings.push_back({links[k].r_b, sb, (pb - ls.start).dot(axis_dir), hsb});
            _liernes.push_back(ls);
        }
    }

    if (params.cheville_radius > 0.5 && n_plies >= 2) {
        for (size_t r = 0; r < _ribs.size(); r++) {
            std::vector<double> knots = {0.0, _ribs[r].total_length()};
            if (_ribs[r].kink_s > 0.0)
                knots.push_back(_ribs[r].kink_s);
            for (const double bs : rib_all_seams[r])
                knots.push_back(bs);
            std::sort(knots.begin(), knots.end());

            for (size_t k = 0; k + 1 < knots.size(); k++) {
                const double ds = knots[k + 1] - knots[k];
                if (ds < 190.0)
                    continue;
                const double sm = 0.5 * (knots[k] + knots[k + 1]);
                bool near_lierne = false;
                for (const DelormeLierneSpec& ls : _liernes) {
                    for (const DelormeLiernePiercing& pier : ls.piercings) {
                        if (pier.rib_index == r && std::abs(pier.rib_s - sm) < 95.0)
                            near_lierne = true;
                    }
                }
                if (near_lierne)
                    continue;

                Point pm;
                Vector tm, radm;
                _ribs[r].evaluate(sm, pm, tm, radm);
                const double half_pin = 0.5 * rib_thick + 8.0;
                DelormeChevilleSpec cs;
                cs.rib_index = r;
                cs.rib_s = sm;
                cs.axis = Line::from_points(
                    pm - _ribs[r].normal * half_pin, pm + _ribs[r].normal * half_pin
                );
                _chevilles.push_back(cs);
            }
        }
    }
}

} // namespace wood_delorme
