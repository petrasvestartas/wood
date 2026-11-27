#include "pch.h"
#include "src/templates/grid/grid_guide.h"

using namespace session_cpp;

namespace wood_grid {

namespace {

using PlanEdge = Plan::Edge;
using PlanCell = Plan::Cell;

constexpr double PLAN_TOL = 1.0;
constexpr double PLAN_MERGE = 5.0;

std::vector<Point> strip_closing_point(const Polyline& pl) {
    std::vector<Point> pts = pl.get_points();
    if (pts.size() > 1 && pts.front().distance(pts.back()) < PLAN_TOL)
        pts.pop_back();
    return pts;
}

Polyline close_loop(std::vector<Point> pts) {
    if (!pts.empty() && pts.front().distance(pts.back()) >= 1e-6)
        pts.push_back(pts.front());
    return Polyline(pts);
}

double signed_area_2d(const std::vector<Point>& pts) {
    double area = 0.0;
    const size_t n = pts.size();
    for (size_t i = 0; i < n; ++i) {
        const Point& a = pts[i];
        const Point& b = pts[(i + 1) % n];
        area += a[0] * b[1] - b[0] * a[1];
    }
    return 0.5 * area;
}

Point interior_sample_2d(const std::vector<Point>& outer, const std::vector<std::vector<Point>>& holes) {
    if (outer.size() < 3)
        return outer.empty() ? Point(0.0, 0.0, 0.0) : outer.front();

    // Try ear centroids / midpoints nudged inward along edges until strictly inside outer & outside holes.
    const Polyline outer_pl = close_loop(outer);
    std::vector<Polyline> hole_pls;
    hole_pls.reserve(holes.size());
    for (const auto& h : holes)
        hole_pls.push_back(close_loop(h));

    const double sign = signed_area_2d(outer) >= 0.0 ? 1.0 : -1.0;
    for (size_t i = 0; i < outer.size(); ++i) {
        const Point& a = outer[i];
        const Point& b = outer[(i + 1) % outer.size()];
        const Vector ab(b[0] - a[0], b[1] - a[1], 0.0);
        const double len = ab.magnitude();
        if (len < PLAN_TOL)
            continue;
        const Vector dir = ab / len;
        const Vector inward(-dir[1] * sign, dir[0] * sign, 0.0);
        for (double step : {10.0, 1.0, 0.2}) {
            const Point probe(
                0.5 * (a[0] + b[0]) + inward[0] * step,
                0.5 * (a[1] + b[1]) + inward[1] * step,
                0.0
            );
            if (!outer_pl.point_in_polygon_2d(probe))
                continue;
            bool in_hole = false;
            for (const Polyline& hp : hole_pls)
                if (hp.point_in_polygon_2d(probe)) {
                    in_hole = true;
                    break;
                }
            if (!in_hole)
                return probe;
        }
    }
    return Point::centroid(outer);
}

bool inside_even_odd_2d(const std::vector<Polyline>& rings, const Point& pt) {
    int count = 0;
    for (const Polyline& ring : rings)
        if (ring.point_in_polygon_2d(pt))
            ++count;
    return (count % 2) == 1;
}

bool point_on_ring_2d(const Polyline& ring, const Point& pt, double tol) {
    const std::vector<Point> corners = strip_closing_point(ring);
    for (size_t i = 0; i < corners.size(); ++i) {
        const Point& a = corners[i];
        const Point& b = corners[(i + 1) % corners.size()];
        const Vector ab(b[0] - a[0], b[1] - a[1], 0.0);
        const double len2 = ab.dot(ab);
        if (len2 < 1e-12)
            continue;
        const double t = std::clamp(Vector(pt[0] - a[0], pt[1] - a[1], 0.0).dot(ab) / len2, 0.0, 1.0);
        const Point proj(a[0] + ab[0] * t, a[1] + ab[1] * t, 0.0);
        if (std::hypot(pt[0] - proj[0], pt[1] - proj[1]) <= tol)
            return true;
    }
    return false;
}

bool point_on_any_ring_2d(const std::vector<Polyline>& rings, const Point& pt, double tol) {
    for (const Polyline& r : rings)
        if (point_on_ring_2d(r, pt, tol))
            return true;
    return false;
}

std::vector<Line> clip_line_to_region_2d(
    const Line& line, const std::vector<Polyline>& boundary_rings, const std::vector<Polyline>& core_rings
) {
    std::vector<double> ts = {0.0, 1.0};
    auto collect_crossings = [&](const std::vector<Polyline>& rings) {
        for (const Polyline& ring : rings) {
            const std::vector<Point> pts = strip_closing_point(ring);
            for (size_t i = 0; i < pts.size(); ++i) {
                const Line seg = Line::from_points(pts[i], pts[(i + 1) % pts.size()]);
                double t = 0.0;
                double s = 0.0;
                if (Intersection::line_line_parameters(line, seg, t, s, PLAN_TOL, true, false) &&
                    line.point_at(t).distance(seg.point_at(s)) <= PLAN_TOL) {
                    ts.push_back(std::clamp(t, 0.0, 1.0));
                }
            }
        }
    };
    collect_crossings(boundary_rings);
    collect_crossings(core_rings);
    std::sort(ts.begin(), ts.end());

    std::vector<Line> pieces;
    const double len = line.length();
    for (size_t i = 0; i + 1 < ts.size(); ++i) {
        if ((ts[i + 1] - ts[i]) * len <= PLAN_TOL)
            continue;
        const Point mid = line.point_at(0.5 * (ts[i] + ts[i + 1]));
        const bool in_bounds =
            boundary_rings.empty() || inside_even_odd_2d(boundary_rings, mid) ||
            point_on_any_ring_2d(boundary_rings, mid, PLAN_TOL * 2.0);
        const bool in_core = !core_rings.empty() && inside_even_odd_2d(core_rings, mid) &&
                             !point_on_any_ring_2d(core_rings, mid, PLAN_TOL * 2.0);
        if (in_bounds && !in_core)
            pieces.push_back(Line::from_points(line.point_at(ts[i]), line.point_at(ts[i + 1])));
    }
    return pieces;
}

void recompute_standing(Plan& plan) {
    plan.standing.assign(plan.nodes.size(), false);
    std::vector<std::vector<Vector>> dirs(plan.nodes.size());
    for (const PlanEdge& e : plan.edges) {
        if (e.core)
            continue;
        const Vector d = (plan.nodes[e.b] - plan.nodes[e.a]).normalized();
        dirs[e.a].push_back(d);
        dirs[e.b].push_back(-d);
    }

    for (size_t v = 0; v < plan.nodes.size(); ++v) {
        if (point_on_any_ring_2d(plan.cores, plan.nodes[v], PLAN_MERGE))
            continue;
        bool corner_or_crossing = false;
        for (size_t i = 0; i < dirs[v].size() && !corner_or_crossing; ++i)
            for (size_t j = i + 1; j < dirs[v].size(); ++j)
                if (std::abs(dirs[v][i][0] * dirs[v][j][1] - dirs[v][i][1] * dirs[v][j][0]) > 0.20) {
                    corner_or_crossing = true;
                    break;
                }
        plan.standing[v] = corner_or_crossing;
    }
}

Plan build_plan_uv(
    const std::vector<Polyline>& boundary_uv,
    const Pattern& pattern_uv,
    const std::vector<Polyline>& cores_uv,
    const Plane& frame
) {
    Plan result;
    result.frame = frame;
    result.boundary = boundary_uv.empty() ? pattern_uv.boundary : boundary_uv;
    result.cores = cores_uv;

    std::vector<Line> clipped_lines;
    std::vector<int> clipped_families;
    for (size_t i = 0; i < pattern_uv.lines.size(); ++i) {
        const Point s = pattern_uv.lines[i].start();
        const Point e = pattern_uv.lines[i].end();
        const Line flat = Line::from_points(Point(s[0], s[1], 0.0), Point(e[0], e[1], 0.0));
        if (flat.length() <= PLAN_TOL)
            continue;
        const int fam = i < pattern_uv.families.size() ? pattern_uv.families[i] : 0;
        for (const Line& piece : clip_line_to_region_2d(flat, result.boundary, result.cores)) {
            clipped_lines.push_back(piece);
            clipped_families.push_back(fam);
        }
    }

    std::vector<Line> ring_lines;
    auto append_ring_segments = [&](const std::vector<Polyline>& rings) {
        for (const Polyline& ring : rings) {
            const std::vector<Point> pts = strip_closing_point(ring);
            for (size_t i = 0; i < pts.size(); ++i) {
                const Point a(pts[i][0], pts[i][1], 0.0);
                const Point b(pts[(i + 1) % pts.size()][0], pts[(i + 1) % pts.size()][1], 0.0);
                if (a.distance(b) > PLAN_TOL)
                    ring_lines.push_back(Line::from_points(a, b));
            }
        }
    };
    append_ring_segments(result.boundary);
    append_ring_segments(result.cores);

    Mesh arr = Mesh::from_arrangement(clipped_lines, ring_lines, PLAN_TOL, PLAN_MERGE);

    std::map<size_t, size_t> vmap;
    for (const size_t vk : arr.vertices()) {
        const Point p = *arr.vertex_point(vk);
        vmap[vk] = result.nodes.size();
        result.nodes.emplace_back(p[0], p[1], 0.0);
    }

    // Extract clean polygonal bays (outer + holes), filtering out exterior or core interiors.
    std::map<std::pair<size_t, size_t>, int> edge_cell_count;
    auto bump_loop_edges = [&](const std::vector<size_t>& loop) {
        for (size_t i = 0; i < loop.size(); ++i) {
            size_t u = loop[i];
            size_t v = loop[(i + 1) % loop.size()];
            if (u > v)
                std::swap(u, v);
            edge_cell_count[{u, v}]++;
        }
    };

    for (const size_t fk : arr.faces()) {
        const std::vector<size_t> raw_outer = *arr.face_vertices(fk);
        if (raw_outer.size() < 3)
            continue;

        std::vector<size_t> outer_idx;
        std::vector<Point> outer_pts;
        outer_idx.reserve(raw_outer.size());
        outer_pts.reserve(raw_outer.size());
        for (const size_t vk : raw_outer) {
            const size_t idx = vmap.at(vk);
            outer_idx.push_back(idx);
            outer_pts.push_back(result.nodes[idx]);
        }
        if (signed_area_2d(outer_pts) < 0.0) {
            std::reverse(outer_idx.begin(), outer_idx.end());
            std::reverse(outer_pts.begin(), outer_pts.end());
        }

        std::vector<std::vector<size_t>> holes_idx;
        std::vector<std::vector<Point>> holes_pts;
        if (auto hole_it = arr.get_face_holes().find(fk); hole_it != arr.get_face_holes().end()) {
            for (const std::vector<size_t>& rh : hole_it->second) {
                if (rh.size() < 3)
                    continue;
                std::vector<size_t> h_idx;
                std::vector<Point> h_pts;
                for (const size_t vk : rh) {
                    const size_t idx = vmap.at(vk);
                    h_idx.push_back(idx);
                    h_pts.push_back(result.nodes[idx]);
                }
                if (signed_area_2d(h_pts) > 0.0) {
                    std::reverse(h_idx.begin(), h_idx.end());
                    std::reverse(h_pts.begin(), h_pts.end());
                }
                holes_idx.push_back(std::move(h_idx));
                holes_pts.push_back(std::move(h_pts));
            }
        }

        const Point sample = interior_sample_2d(outer_pts, holes_pts);
        if (!result.boundary.empty() && !inside_even_odd_2d(result.boundary, sample))
            continue;
        if (!result.cores.empty() && inside_even_odd_2d(result.cores, sample))
            continue;

        bump_loop_edges(outer_idx);
        for (const auto& h : holes_idx)
            bump_loop_edges(h);

        result.cells.push_back(PlanCell{std::move(outer_idx), std::move(holes_idx)});
    }

    // Collect canonical family directions so pure boundary segments aligned with an axis inherit its family.
    std::map<int, Vector> canonical_dirs;
    for (size_t i = 0; i < clipped_lines.size(); ++i) {
        if (!canonical_dirs.count(clipped_families[i]))
            canonical_dirs[clipped_families[i]] = clipped_lines[i].to_vector().normalized();
    }

    for (const std::pair<size_t, size_t>& raw_e : arr.edges()) {
        const double src = arr.edge_attribute(raw_e, "line").value_or(-1.0);
        if (src < 0.0)
            continue;

        size_t u = vmap.at(raw_e.first);
        size_t v = vmap.at(raw_e.second);
        if (u == v)
            continue;
        if (u > v)
            std::swap(u, v);

        const int cell_adj = edge_cell_count.count({u, v}) ? edge_cell_count.at({u, v}) : 0;
        if (!result.cells.empty() && cell_adj == 0) {
            const Point mid = Point::mid_point(result.nodes[u], result.nodes[v]);
            const bool keep_free =
                (result.boundary.empty() || inside_even_odd_2d(result.boundary, mid)) &&
                (result.cores.empty() || !inside_even_odd_2d(result.cores, mid));
            if (!keep_free)
                continue;
        }

        const Point mid = Point::mid_point(result.nodes[u], result.nodes[v]);
        const bool on_core = !result.cores.empty() && point_on_any_ring_2d(result.cores, mid, PLAN_MERGE);
        const bool is_boundary = (cell_adj <= 1) && !on_core;

        int family = -1;
        const size_t src_idx = static_cast<size_t>(src);
        if (src_idx < clipped_families.size()) {
            family = clipped_families[src_idx];
        } else {
            const Vector dir = (result.nodes[v] - result.nodes[u]).normalized();
            for (const auto& [fam, fdir] : canonical_dirs) {
                if (std::abs(dir.dot(fdir)) > 0.999) {
                    family = fam;
                    break;
                }
            }
        }

        result.edges.push_back(PlanEdge{u, v, family, is_boundary, on_core});
    }

    recompute_standing(result);
    return result;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Pattern
// ═══════════════════════════════════════════════════════════════════════════

std::vector<double> compute_bays(double length, double target) {
    const int count = std::max(1, static_cast<int>(std::round(length / std::max(1.0, target))));
    return std::vector<double>(count, length / count);
}

std::vector<double> compute_gradated_bays(double length, int count, double end_ratio) {
    const int n = std::max(1, count);
    if (n == 1)
        return {length};

    std::vector<double> weights(static_cast<size_t>(n), 1.0);
    double sum = 0.0;
    const double mid = 0.5 * (n - 1);
    for (int i = 0; i < n; ++i) {
        const double dist = mid > 0.0 ? std::abs(i - mid) / mid : 0.0;
        weights[static_cast<size_t>(i)] = 1.0 + (end_ratio - 1.0) * dist;
        sum += weights[static_cast<size_t>(i)];
    }
    for (double& w : weights)
        w *= length / std::max(1e-6, sum);
    return weights;
}

Pattern Pattern::orthogonal(
    const std::vector<double>& u_bays, const std::vector<double>& v_bays, double skew_degrees
) {
    std::vector<double> us = {0.0};
    for (double du : u_bays)
        us.push_back(us.back() + du);

    std::vector<double> vs = {0.0};
    for (double dv : v_bays)
        vs.push_back(vs.back() + dv);

    const double rad = skew_degrees * Tolerance::TO_RADIANS;
    const Vector u_dir(1.0, 0.0, 0.0);
    const Vector v_dir(std::sin(rad), std::cos(rad), 0.0);

    Pattern pat;
    for (double v : vs) {
        const Point a = Point(0.0, 0.0, 0.0) + v_dir * v;
        pat.lines.push_back(Line::from_points(a, a + u_dir * us.back()));
        pat.families.push_back(0);
    }
    for (double u : us) {
        const Point a = Point(0.0, 0.0, 0.0) + u_dir * u;
        pat.lines.push_back(Line::from_points(a, a + v_dir * vs.back()));
        pat.families.push_back(1);
    }
    return pat;
}

Pattern Pattern::radial(const std::vector<double>& radii, int sectors, bool inner_hole) {
    Pattern pat;
    if (radii.empty() || sectors < 3)
        return pat;

    std::vector<double> sorted_r = radii;
    std::sort(sorted_r.begin(), sorted_r.end());

    const bool has_oculus = inner_hole && sorted_r.size() >= 2 && sorted_r.front() > PLAN_TOL;
    const double r_start = has_oculus ? sorted_r.front() : 0.0;
    const double r_end = sorted_r.back();

    // Family 0: radial rays.
    for (int s = 0; s < sectors; ++s) {
        const double ang = Tolerance::TWO_PI * s / sectors;
        const Vector dir(std::cos(ang), std::sin(ang), 0.0);
        pat.lines.push_back(
            Line::from_points(Point(0.0, 0.0, 0.0) + dir * r_start, Point(0.0, 0.0, 0.0) + dir * r_end)
        );
        pat.families.push_back(0);
    }

    // Family 1: concentric polygon rings.
    auto make_ring = [&](double r) {
        std::vector<Point> pts;
        pts.reserve(sectors + 1);
        for (int s = 0; s <= sectors; ++s) {
            const double ang = Tolerance::TWO_PI * (s % sectors) / sectors;
            pts.emplace_back(r * std::cos(ang), r * std::sin(ang), 0.0);
        }
        return Polyline(pts);
    };

    for (double r : sorted_r) {
        if (r <= PLAN_TOL)
            continue;
        const Polyline ring = make_ring(r);
        for (int s = 0; s < sectors; ++s) {
            pat.lines.push_back(Line::from_points(ring.get_point(s), ring.get_point(s + 1)));
            pat.families.push_back(1);
        }
    }

    pat.boundary.push_back(make_ring(r_end));
    if (has_oculus)
        pat.boundary.push_back(make_ring(sorted_r.front()));

    return pat;
}

Pattern Pattern::triangular(double side, int nu, int nv) {
    Pattern pat;
    if (side <= PLAN_TOL || nu < 1 || nv < 1)
        return pat;

    const double h = side * std::sqrt(3.0) * 0.5;
    auto pt = [&](int i, int j) { return Point((i + 0.5 * j) * side, j * h, 0.0); };

    // Family 0: 0 deg lines.
    for (int j = 0; j <= nv; ++j) {
        pat.lines.push_back(Line::from_points(pt(0, j), pt(nu, j)));
        pat.families.push_back(0);
    }
    // Family 1: 60 deg lines.
    for (int i = 0; i <= nu; ++i) {
        pat.lines.push_back(Line::from_points(pt(i, 0), pt(i, nv)));
        pat.families.push_back(1);
    }
    // Family 2: 120 deg diagonals across each parallelogram cell.
    for (int s = 1; s < nu + nv; ++s) {
        const int j_min = std::max(0, s - nu);
        const int j_max = std::min(nv, s);
        if (j_max > j_min) {
            pat.lines.push_back(Line::from_points(pt(s - j_min, j_min), pt(s - j_max, j_max)));
            pat.families.push_back(2);
        }
    }
    return pat;
}

Pattern Pattern::hexagonal(double side, int nu, int nv) {
    Pattern pat;
    if (side <= PLAN_TOL || nu < 1 || nv < 1)
        return pat;

    const double h = std::sqrt(3.0) * side;
    std::vector<std::pair<Point, Point>> unique_edges;

    auto same_pt = [](const Point& a, const Point& b) { return a.distance(b) < PLAN_TOL; };
    auto has_edge = [&](const Point& a, const Point& b) {
        for (const auto& [ea, eb] : unique_edges)
            if ((same_pt(a, ea) && same_pt(b, eb)) || (same_pt(a, eb) && same_pt(b, ea)))
                return true;
        return false;
    };

    for (int col = 0; col < nu; ++col) {
        for (int row = 0; row < nv; ++row) {
            const double cx = side + col * 1.5 * side;
            const double cy = 0.5 * h + (row + 0.5 * (col % 2)) * h;
            std::array<Point, 6> corners;
            for (int k = 0; k < 6; ++k) {
                const double ang = Tolerance::PI / 3.0 * k;
                corners[k] = Point(cx + side * std::cos(ang), cy + side * std::sin(ang), 0.0);
            }
            for (int k = 0; k < 6; ++k) {
                const Point& a = corners[k];
                const Point& b = corners[(k + 1) % 6];
                if (!has_edge(a, b)) {
                    unique_edges.emplace_back(a, b);
                    pat.lines.push_back(Line::from_points(a, b));
                    pat.families.push_back(k % 3);
                }
            }
        }
    }
    return pat;
}

Pattern Pattern::diagonal(double width, double depth, double pitch) {
    Pattern pat;
    if (width <= PLAN_TOL || depth <= PLAN_TOL || pitch <= PLAN_TOL)
        return pat;

    const double step = pitch * std::sqrt(2.0);
    const double reach = width + depth + step;

    // Family 0: +45 deg axes (u - v = const), half-step shifted so corners form stiff triangles.
    const int k0_min = static_cast<int>(std::floor(-depth / step)) - 1;
    const int k0_max = static_cast<int>(std::ceil(width / step)) + 1;
    for (int k = k0_min; k <= k0_max; ++k) {
        const double c = (k + 0.5) * step;
        pat.lines.push_back(Line::from_points(Point(c - reach, -reach, 0.0), Point(c + reach, reach, 0.0)));
        pat.families.push_back(0);
    }

    // Family 1: -45 deg axes (u + v = const), half-step shifted symmetrically.
    const int k1_max = static_cast<int>(std::ceil((width + depth) / step)) + 1;
    for (int k = -1; k <= k1_max; ++k) {
        const double c = (k + 0.5) * step;
        pat.lines.push_back(Line::from_points(Point(c + reach, -reach, 0.0), Point(c - reach, reach, 0.0)));
        pat.families.push_back(1);
    }

    pat.boundary.push_back(
        Polyline::rectangle(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), width, depth)
    );
    return pat;
}

Pattern Pattern::concentric(double width, double depth, int rings, double ratio) {
    Pattern pat;
    if (width <= PLAN_TOL || depth <= PLAN_TOL || rings < 1)
        return pat;

    const double alpha = std::clamp(ratio, 0.15, 0.85);
    std::array<Point, 4> quad = {
        Point(0.0, 0.0, 0.0),
        Point(width, 0.0, 0.0),
        Point(width, depth, 0.0),
        Point(0.0, depth, 0.0),
    };
    pat.boundary.push_back(close_loop({quad[0], quad[1], quad[2], quad[3]}));

    for (int r = 0; r <= rings; ++r) {
        for (size_t k = 0; k < 4; ++k) {
            const Point& a = quad[k];
            const Point& b = quad[(k + 1) % 4];
            if (a.distance(b) > PLAN_TOL) {
                pat.lines.push_back(Line::from_points(a, b));
                pat.families.push_back(r);
            }
        }
        std::array<Point, 4> next_quad;
        for (size_t k = 0; k < 4; ++k) {
            const Point& a = quad[k];
            const Point& b = quad[(k + 1) % 4];
            next_quad[k] = Point(
                (1.0 - alpha) * a[0] + alpha * b[0],
                (1.0 - alpha) * a[1] + alpha * b[1],
                0.0
            );
        }
        quad = next_quad;
    }
    return pat;
}

Pattern Pattern::ramified(double width, double depth, int bays) {
    Pattern pat;
    if (width <= PLAN_TOL || depth <= PLAN_TOL || bays < 1)
        return pat;

    const int n = std::max(1, bays);
    const double bay_u = width / (n + 1);
    const double y_lo = 0.27 * depth;
    const double y_hi = 0.73 * depth;
    const double y_mid = 0.5 * depth;

    // Family 0: short primary trunks stepping inward from perimeter supports to Y-fork nodes.
    for (int i = 1; i <= n; ++i) {
        const double u = i * bay_u;
        pat.lines.push_back(Line::from_points(Point(u, 0.0, 0.0), Point(u, y_lo, 0.0)));
        pat.families.push_back(0);
        pat.lines.push_back(Line::from_points(Point(u, depth, 0.0), Point(u, y_hi, 0.0)));
        pat.families.push_back(0);
    }
    const double u_end_l = 0.5 * bay_u;
    const double u_end_r = width - 0.5 * bay_u;
    pat.lines.push_back(Line::from_points(Point(0.0, y_mid, 0.0), Point(u_end_l, y_mid, 0.0)));
    pat.families.push_back(0);
    pat.lines.push_back(Line::from_points(Point(width, y_mid, 0.0), Point(u_end_r, y_mid, 0.0)));
    pat.families.push_back(0);

    // Family 1: interior branching Y-arms and central hexagonal/diamond rings.
    pat.lines.push_back(Line::from_points(Point(u_end_l, y_mid, 0.0), Point(bay_u, y_lo, 0.0)));
    pat.families.push_back(1);
    pat.lines.push_back(Line::from_points(Point(u_end_l, y_mid, 0.0), Point(bay_u, y_hi, 0.0)));
    pat.families.push_back(1);
    pat.lines.push_back(Line::from_points(Point(u_end_r, y_mid, 0.0), Point(n * bay_u, y_lo, 0.0)));
    pat.families.push_back(1);
    pat.lines.push_back(Line::from_points(Point(u_end_r, y_mid, 0.0), Point(n * bay_u, y_hi, 0.0)));
    pat.families.push_back(1);

    for (int i = 1; i < n; ++i) {
        const double u0 = i * bay_u;
        const double u1 = (i + 1) * bay_u;
        const double um = 0.5 * (u0 + u1);
        pat.lines.push_back(Line::from_points(Point(u0, y_lo, 0.0), Point(u1, y_lo, 0.0)));
        pat.families.push_back(1);
        pat.lines.push_back(Line::from_points(Point(u0, y_hi, 0.0), Point(u1, y_hi, 0.0)));
        pat.families.push_back(1);
        pat.lines.push_back(Line::from_points(Point(u0, y_lo, 0.0), Point(um, y_mid, 0.0)));
        pat.families.push_back(2);
        pat.lines.push_back(Line::from_points(Point(u1, y_lo, 0.0), Point(um, y_mid, 0.0)));
        pat.families.push_back(2);
        pat.lines.push_back(Line::from_points(Point(u0, y_hi, 0.0), Point(um, y_mid, 0.0)));
        pat.families.push_back(2);
        pat.lines.push_back(Line::from_points(Point(u1, y_hi, 0.0), Point(um, y_mid, 0.0)));
        pat.families.push_back(2);
    }

    pat.boundary.push_back(
        Polyline::rectangle(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), width, depth)
    );
    return pat;
}

Pattern Pattern::from_lines(const std::vector<Line>& lines, const std::vector<int>& families) {
    Pattern pat;
    pat.lines = lines;
    if (families.size() == lines.size()) {
        pat.families = families;
    } else {
        for (const Line& l : lines) {
            const Vector d = l.to_vector().normalized();
            pat.families.push_back(std::abs(d[0]) >= std::abs(d[1]) ? 0 : 1);
        }
    }
    return pat;
}

Pattern Pattern::transformed(const Xform& xform) const {
    Pattern out;
    out.families = families;
    out.lines.reserve(lines.size());
    for (const Line& l : lines)
        out.lines.push_back(l.transformed(xform));
    out.boundary.reserve(boundary.size());
    for (const Polyline& b : boundary)
        out.boundary.push_back(b.transformed(xform));
    return out;
}

// ═══════════════════════════════════════════════════════════════════════════
// Plan
// ═══════════════════════════════════════════════════════════════════════════

Point Plan::to_world(double u, double v, double w) const {
    return frame.origin() + frame.x_axis() * u + frame.y_axis() * v + frame.z_axis() * w;
}

Plan Plan::bay(size_t index) const {
    return bays({index});
}

Plan Plan::bays(const std::vector<size_t>& indices) const {
    Plan sub;
    sub.frame = frame;
    if (indices.empty())
        return sub;

    std::map<size_t, size_t> v_remap;
    auto map_v = [&](size_t old_v) {
        auto it = v_remap.find(old_v);
        if (it != v_remap.end())
            return it->second;
        const size_t nv = sub.nodes.size();
        v_remap[old_v] = nv;
        sub.nodes.push_back(nodes[old_v]);
        return nv;
    };

    std::map<std::pair<size_t, size_t>, int> sub_adj;
    auto record_loop = [&](const std::vector<size_t>& old_loop) {
        std::vector<size_t> mapped;
        mapped.reserve(old_loop.size());
        for (size_t ov : old_loop)
            mapped.push_back(map_v(ov));
        for (size_t i = 0; i < mapped.size(); ++i) {
            size_t a = mapped[i];
            size_t b = mapped[(i + 1) % mapped.size()];
            if (a > b)
                std::swap(a, b);
            sub_adj[{a, b}]++;
        }
        return mapped;
    };

    for (size_t idx : indices) {
        if (idx >= cells.size())
            continue;
        PlanCell sc;
        sc.outer = record_loop(cells[idx].outer);
        for (const auto& h : cells[idx].holes)
            sc.holes.push_back(record_loop(h));
        sub.cells.push_back(std::move(sc));
    }

    // Map old edge metadata by unordered old vertex pair.
    std::map<std::pair<size_t, size_t>, const PlanEdge*> old_edge_map;
    for (const PlanEdge& e : edges) {
        size_t a = e.a;
        size_t b = e.b;
        if (a > b)
            std::swap(a, b);
        old_edge_map[{a, b}] = &e;
    }

    std::map<size_t, size_t> inv_remap;
    for (const auto& [old_v, new_v] : v_remap)
        inv_remap[new_v] = old_v;

    for (const auto& [uv, adj] : sub_adj) {
        size_t oa = inv_remap.at(uv.first);
        size_t ob = inv_remap.at(uv.second);
        if (oa > ob)
            std::swap(oa, ob);
        int fam = -1;
        bool core = false;
        if (auto it = old_edge_map.find({oa, ob}); it != old_edge_map.end()) {
            fam = it->second->family;
            core = it->second->core;
        }
        sub.edges.push_back(PlanEdge{uv.first, uv.second, fam, adj <= 1 && !core, core});
    }

    recompute_standing(sub);
    return sub;
}

Plan Plan::transformed(const Xform& xform) const {
    Plan copy = *this;
    const Point new_origin = frame.origin().transformed(xform);
    const Point px = (frame.origin() + frame.x_axis()).transformed(xform);
    const Point py = (frame.origin() + frame.y_axis()).transformed(xform);
    copy.frame = Plane(
        new_origin, (px - new_origin).normalized(), (py - new_origin).normalized()
    );
    for (std::optional<Point>& foot : copy.column_feet)
        if (foot.has_value())
            foot = foot->transformed(xform);
    return copy;
}

Plan Plan::with_cantilever(double overhang_u, double overhang_v) const {
    Plan copy = *this;
    if (copy.nodes.empty())
        return copy;

    const double ov_u = std::max(0.0, overhang_u);
    const double ov_v = overhang_v < 0.0 ? ov_u : std::max(0.0, overhang_v);
    double min_u = std::numeric_limits<double>::max();
    double max_u = -min_u;
    double min_v = min_u;
    double max_v = -min_u;
    for (const Point& n : copy.nodes) {
        min_u = std::min(min_u, n[0]);
        max_u = std::max(max_u, n[0]);
        min_v = std::min(min_v, n[1]);
        max_v = std::max(max_v, n[1]);
    }
    copy.standing.resize(copy.nodes.size(), false);
    for (size_t i = 0; i < copy.nodes.size(); ++i) {
        const Point& n = copy.nodes[i];
        if ((ov_u > PLAN_TOL && (n[0] < min_u + ov_u - PLAN_TOL || n[0] > max_u - ov_u + PLAN_TOL)) ||
            (ov_v > PLAN_TOL && (n[1] < min_v + ov_v - PLAN_TOL || n[1] > max_v - ov_v + PLAN_TOL))) {
            copy.standing[i] = false;
        }
    }
    return copy;
}

Plan Plan::with_column_stride(int stride_u, int stride_v) const {
    Plan copy = *this;
    const int su = std::max(1, stride_u);
    const int sv = stride_v <= 0 ? su : stride_v;
    if (copy.nodes.empty() || (su == 1 && sv == 1))
        return copy;

    const std::function<std::vector<double>(int)> collect_unique = [&](int coord) -> std::vector<double> {
        std::vector<double> vals;
        for (size_t i = 0; i < copy.nodes.size(); ++i)
            if (i < copy.standing.size() && copy.standing[i])
                vals.push_back(copy.nodes[i][coord]);
        std::sort(vals.begin(), vals.end());
        std::vector<double> uniq;
        for (double val : vals)
            if (uniq.empty() || std::abs(val - uniq.back()) > PLAN_MERGE)
                uniq.push_back(val);
        return uniq;
    };
    const std::vector<double> us = collect_unique(0);
    const std::vector<double> vs = collect_unique(1);
    const std::function<int(const std::vector<double>&, double)> index_of =
        [](const std::vector<double>& uniq, double val) -> int {
        for (size_t k = 0; k < uniq.size(); ++k)
            if (std::abs(uniq[k] - val) <= PLAN_MERGE)
                return static_cast<int>(k);
        return -1;
    };

    for (size_t i = 0; i < copy.nodes.size(); ++i) {
        if (i >= copy.standing.size() || !copy.standing[i])
            continue;
        const int iu = index_of(us, copy.nodes[i][0]);
        const int iv = index_of(vs, copy.nodes[i][1]);
        if (iu < 0 || iv < 0 || (iu % su) != 0 || (iv % sv) != 0)
            copy.standing[i] = false;
    }
    return copy;
}

Plan Plan::perimeter_columns_only() const {
    Plan copy = *this;
    std::vector<bool> on_bnd(copy.nodes.size(), false);
    for (const PlanEdge& e : copy.edges) {
        if (e.boundary) {
            on_bnd[e.a] = true;
            on_bnd[e.b] = true;
        }
    }
    for (size_t i = 0; i < copy.standing.size(); ++i)
        if (!on_bnd[i])
            copy.standing[i] = false;
    return copy;
}

Plan Plan::from_pattern(const Pattern& pattern, const Plane& frame) {
    return build_plan_uv({}, pattern, {}, frame);
}

Plan Plan::from_footprint(
    const std::vector<Polyline>& footprint,
    const Pattern& pattern,
    const std::vector<Polyline>& cores,
    const Plane& frame
) {
    const bool world_xy_frame =
        std::abs(frame.z_axis()[2] - 1.0) < 1e-6 &&
        std::abs(frame.x_axis()[0] - 1.0) < 1e-6 &&
        std::abs(frame.y_axis()[1] - 1.0) < 1e-6;

    auto project_ring = [&](const Polyline& ring) {
        std::vector<Point> pts;
        for (const Point& p : ring.get_points()) {
            if (world_xy_frame) {
                pts.emplace_back(p[0] - frame.origin()[0], p[1] - frame.origin()[1], 0.0);
            } else {
                const Vector d = p - frame.origin();
                pts.emplace_back(d.dot(frame.x_axis()), d.dot(frame.y_axis()), 0.0);
            }
        }
        return Polyline(pts);
    };

    std::vector<Polyline> bound_uv;
    bound_uv.reserve(footprint.size());
    for (const Polyline& r : footprint)
        bound_uv.push_back(project_ring(r));

    std::vector<Polyline> cores_uv;
    cores_uv.reserve(cores.size());
    for (const Polyline& c : cores)
        cores_uv.push_back(project_ring(c));

    Pattern pat_uv = pattern;
    if (world_xy_frame && (std::abs(frame.origin()[0]) > 1e-6 || std::abs(frame.origin()[1]) > 1e-6)) {
        pat_uv = pattern.transformed(Xform::translation(-frame.origin()[0], -frame.origin()[1], 0.0));
    }

    return build_plan_uv(bound_uv, pat_uv, cores_uv, frame);
}

Plan Plan::facade(
    const Line& baseline,
    double height,
    const std::vector<double>& u_bays,
    const std::vector<double>& v_levels,
    const std::vector<Polyline>& openings_uv,
    const Polyline& silhouette_uv
) {
    const Vector u_axis = baseline.to_vector().normalized();
    const Vector v_axis =
        std::abs(u_axis[2]) > 0.99 ? Vector(1.0, 0.0, 0.0) : Vector(0.0, 0.0, 1.0);
    const Plane wall_frame = Plane(baseline.start(), u_axis, v_axis);

    const double width = baseline.length();
    std::vector<Polyline> rings_uv;
    if (silhouette_uv.point_count() >= 3) {
        rings_uv.push_back(close_loop(strip_closing_point(silhouette_uv)));
    } else {
        rings_uv.push_back(Polyline::rectangle(
            Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), width, height
        ));
    }
    for (const Polyline& op : openings_uv)
        rings_uv.push_back(close_loop(strip_closing_point(op)));

    const Pattern pat = Pattern::orthogonal(u_bays, v_levels);
    Plan plan = build_plan_uv(rings_uv, pat, {}, wall_frame);
    // Facades are planar wall frames without normal posts unless explicitly requested.
    std::fill(plan.standing.begin(), plan.standing.end(), false);
    return plan;
}

// ═══════════════════════════════════════════════════════════════════════════
// Stack
// ═══════════════════════════════════════════════════════════════════════════

Stack Stack::from_footprint(
    const std::vector<Polyline>& footprint,
    const std::vector<double>& elevations,
    const Pattern& pattern,
    const Framing& framing,
    const std::vector<Polyline>& cores
) {
    Stack stack;
    if (elevations.size() < 2)
        return stack;

    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);
    for (size_t l = 1; l < elevations.size(); ++l) {
        const Plane floor_frame(Point(0.0, 0.0, elevations[l]), X, Y);
        Framing floor_framing = framing;
        floor_framing.height = elevations[l] - elevations[l - 1];
        stack.levels.push_back(
            Level{Plan::from_footprint(footprint, pattern, cores, floor_frame), floor_framing}
        );
    }
    return stack;
}

Stack Stack::from_levels(std::vector<Level> levels, double base_z, bool plumb_columns) {
    double prev_z = base_z;
    for (size_t i = 0; i < levels.size(); ++i) {
        const double z = levels[i].plan.frame.origin()[2];
        levels[i].framing.height = std::max(0.0, z - prev_z);

        if (plumb_columns && i > 0 && !levels[i - 1].plan.boundary.empty()) {
            std::vector<Polyline> prev_rings;
            prev_rings.reserve(levels[i - 1].plan.boundary.size());
            for (const Polyline& r : levels[i - 1].plan.boundary) {
                std::vector<Point> pts;
                pts.reserve(r.point_count());
                for (const Point& p : r.get_points()) {
                    const Point w = levels[i - 1].plan.to_world(p[0], p[1], 0.0);
                    pts.emplace_back(w[0], w[1], 0.0);
                }
                prev_rings.emplace_back(std::move(pts));
            }
            for (size_t v = 0; v < levels[i].plan.nodes.size(); ++v) {
                if (v >= levels[i].plan.standing.size() || !levels[i].plan.standing[v])
                    continue;
                const Point w =
                    levels[i].plan.to_world(levels[i].plan.nodes[v][0], levels[i].plan.nodes[v][1], 0.0);
                const Point flat(w[0], w[1], 0.0);
                if (!inside_even_odd_2d(prev_rings, flat) &&
                    !point_on_any_ring_2d(prev_rings, flat, PLAN_MERGE * 2.0)) {
                    levels[i].plan.standing[v] = false;
                }
            }
        }
        prev_z = z;
    }
    return Stack{std::move(levels), {}};
}

Stack Stack::morph(
    const std::vector<Polyline>& footprint,
    const std::vector<double>& elevations,
    const Pattern& pattern,
    const std::function<Xform(double)>& xform_at,
    const Framing& framing,
    const std::vector<Polyline>& cores
) {
    Stack stack;
    if (elevations.size() < 2)
        return stack;

    const double z0 = elevations.front();
    const double z_span = std::max(1.0, elevations.back() - z0);
    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);

    auto make_plan_at = [&](size_t idx) {
        const double t = (elevations[idx] - z0) / z_span;
        const Xform xf = xform_at(t);
        std::vector<Polyline> fp;
        fp.reserve(footprint.size());
        for (const Polyline& r : footprint)
            fp.push_back(r.transformed(xf));
        const Plane fl_frame(Point(0.0, 0.0, elevations[idx]), X, Y);
        return std::make_pair(
            Plan::from_footprint(fp, pattern.transformed(xf), cores, fl_frame), xf
        );
    };

    auto [prev_plan, prev_xf] = make_plan_at(0);
    for (size_t l = 1; l < elevations.size(); ++l) {
        auto [curr_plan, curr_xf] = make_plan_at(l);
        curr_plan.column_feet.assign(curr_plan.nodes.size(), std::nullopt);

        const Xform inv_prev = prev_xf.inverse().value_or(Xform::identity());
        for (size_t v = 0; v < curr_plan.nodes.size(); ++v) {
            if (v >= curr_plan.standing.size() || !curr_plan.standing[v])
                continue;
            const Point curr_xy =
                curr_plan.to_world(curr_plan.nodes[v][0], curr_plan.nodes[v][1], 0.0);
            double best_d = 80.0;
            std::optional<Point> match_foot;
            for (size_t u = 0; u < prev_plan.nodes.size(); ++u) {
                if (u < prev_plan.standing.size() && !prev_plan.standing[u])
                    continue;
                const Point prev_w =
                    prev_plan.to_world(prev_plan.nodes[u][0], prev_plan.nodes[u][1], 0.0);
                const Point prev_predicted =
                    Point(prev_w[0], prev_w[1], 0.0).transformed(inv_prev).transformed(curr_xf);
                const double d =
                    std::hypot(curr_xy[0] - prev_predicted[0], curr_xy[1] - prev_predicted[1]);
                if (d < best_d) {
                    best_d = d;
                    match_foot = prev_w;
                }
            }
            if (match_foot.has_value())
                curr_plan.column_feet[v] = match_foot;
            else
                curr_plan.standing[v] = false;
        }

        Framing fl_frm = framing;
        fl_frm.height = elevations[l] - elevations[l - 1];
        stack.levels.push_back(Level{std::move(curr_plan), fl_frm});
        prev_plan = stack.levels.back().plan;
        prev_xf = curr_xf;
    }
    return stack;
}

void Stack::add_facades(double target_bay, const Framing& framing, double base_z) {
    if (levels.empty())
        return;

    for (size_t li = 0; li < levels.size(); ++li) {
        const Level& lvl = levels[li];
        const double z_top_datum = lvl.plan.frame.origin()[2];
        const double col_w = wood_session::compute_size(lvl.framing.profiles.column).first;

        std::vector<std::vector<Vector>> bnd_dirs(lvl.plan.nodes.size());
        for (const PlanEdge& e : lvl.plan.edges) {
            if (!e.boundary)
                continue;
            const Vector d = (lvl.plan.nodes[e.b] - lvl.plan.nodes[e.a]).normalized();
            bnd_dirs[e.a].push_back(d);
            bnd_dirs[e.b].push_back(-d);
        }
        auto is_bldg_corner = [&](size_t nv) {
            return bnd_dirs[nv].size() >= 2 &&
                   std::abs(bnd_dirs[nv][0][0] * bnd_dirs[nv][1][1] -
                            bnd_dirs[nv][0][1] * bnd_dirs[nv][1][0]) > 0.2;
        };

        const auto [girder_w, girder_d] = wood_session::compute_size(lvl.framing.profiles.girder);
        const auto [sec_w, sec_d] = wood_session::compute_size(
            lvl.framing.profiles.beam.empty() ? lvl.framing.profiles.girder : lvl.framing.profiles.beam
        );
        const auto [purlin_w, purlin_d] = wood_session::compute_size(lvl.framing.profiles.purlin);
        const double fm_hw = 0.5 * wood_session::compute_size(framing.profiles.girder).first;
        const auto& ft_prof =
            framing.profiles.beam.empty() ? framing.profiles.girder : framing.profiles.beam;
        const double ft_hw = 0.5 * wood_session::compute_size(ft_prof).first;

        double z_lo = (li == 0) ? base_z : levels[li - 1].plan.frame.origin()[2];
        if (li > 0 && levels[li - 1].framing.deck > PLAN_TOL && !levels[li - 1].framing.deck_flush)
            z_lo += levels[li - 1].framing.deck;

        std::map<std::pair<size_t, size_t>, std::pair<size_t, size_t>> ccw_dir;
        for (const PlanCell& cell : lvl.plan.cells) {
            for (size_t i = 0; i < cell.outer.size(); ++i) {
                const size_t u = cell.outer[i];
                const size_t v = cell.outer[(i + 1) % cell.outer.size()];
                ccw_dir[std::minmax(u, v)] = {u, v};
            }
        }

        for (const PlanEdge& edge : lvl.plan.edges) {
            if (!edge.boundary)
                continue;
            const auto key = std::minmax(edge.a, edge.b);
            const auto [u, v] = ccw_dir.count(key) ? ccw_dir.at(key) : std::make_pair(edge.a, edge.b);

            const bool is_primary =
                (lvl.framing.system == 1 && lvl.framing.span < 0) ||
                edge.family == std::max(0, lvl.framing.span);
            double beam_w = col_w;
            double soffit_d = 0.0;
            if (lvl.framing.system == 1) {
                beam_w = is_primary ? girder_w : sec_w;
                soffit_d = is_primary ? girder_d : sec_d;
            } else if (lvl.framing.system == 2 ||
                       lvl.framing.system == 3) {
                const bool stacked = (lvl.framing.system == 3);
                if (is_primary) {
                    beam_w = girder_w;
                    soffit_d = stacked ? (purlin_d + girder_d) : girder_d;
                } else if (!stacked) {
                    beam_w = sec_w;
                    soffit_d = sec_d;
                } else {
                    beam_w = purlin_w;
                    soffit_d = purlin_d;
                }
            } else if (lvl.framing.system == 4) {
                beam_w = lvl.framing.profiles.beam.empty() ? purlin_w : sec_w;
                soffit_d = lvl.framing.profiles.beam.empty() ? purlin_d : sec_d;
            }
            const double clear_h = (z_top_datum - soffit_d) - z_lo - 2.0 * ft_hw;
            if (clear_h <= ft_hw * 2.0)
                continue;

            const Point p0 = lvl.plan.to_world(lvl.plan.nodes[u][0], lvl.plan.nodes[u][1], 0.0);
            const Point p1 = lvl.plan.to_world(lvl.plan.nodes[v][0], lvl.plan.nodes[v][1], 0.0);
            const Point c0(p0[0], p0[1], z_lo + ft_hw);
            const Point c1(p1[0], p1[1], z_lo + ft_hw);
            const Vector dir = (c1 - c0).normalized();
            const double end_inset = 0.5 * col_w + fm_hw;
            const double clear_len = c0.distance(c1) - 2.0 * end_inset;
            if (clear_len <= target_bay * 0.35)
                continue;

            const Vector outward(dir[1], -dir[0], 0.0);
            const Vector align = outward * (0.5 * std::min(col_w, beam_w));
            const Line base =
                Line::from_points(c0 + dir * end_inset + align, c1 - dir * end_inset + align);
            const std::vector<double> u_bays = compute_bays(clear_len, target_bay);
            const Plan wall_plan = Plan::facade(base, clear_h, u_bays, {clear_h});

            Framing ff = framing;
            ff.height = 0.0;
            if (ff.brace != 0 && !wall_plan.cells.empty()) {
                const bool at_start = is_bldg_corner(u);
                const bool at_end = is_bldg_corner(v);
                size_t min_ci = 0;
                size_t max_ci = 0;
                double min_u = std::numeric_limits<double>::max();
                double max_u = -min_u;
                for (size_t ci = 0; ci < wall_plan.cells.size(); ++ci) {
                    double sum_u = 0.0;
                    for (size_t vi : wall_plan.cells[ci].outer)
                        sum_u += wall_plan.nodes[vi][0];
                    const double cu =
                        sum_u / std::max<size_t>(1, wall_plan.cells[ci].outer.size());
                    if (cu < min_u) {
                        min_u = cu;
                        min_ci = ci;
                    }
                    if (cu > max_u) {
                        max_u = cu;
                        max_ci = ci;
                    }
                }
                ff.braced_bays.clear();
                if (at_start)
                    ff.braced_bays.push_back(min_ci);
                if (at_end && max_ci != min_ci)
                    ff.braced_bays.push_back(max_ci);
                if (ff.braced_bays.empty())
                    ff.brace = 0;
            }
            facades.push_back(Level{wall_plan, ff});
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Design
// ═══════════════════════════════════════════════════════════════════════════

Design::Design(const std::string& name, const Plan& plan, const Framing& framing, double width)
    : name(name),
      plan(plan),
      framing(framing),
      width(width) {
}

Design::Design(const std::string& name, const std::function<Stack(double)>& stack, double width)
    : name(name),
      stack(stack),
      width(width) {
}

} // namespace wood_grid
