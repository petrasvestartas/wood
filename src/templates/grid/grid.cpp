#include "pch.h"
#include "src/templates/grid/grid.h"
#include "wood_element_geometry.h"
#include "boolean_polyline.h"

using namespace session_cpp;

namespace wood_grid {

namespace {

constexpr double TOL = 1.0;

std::vector<Point> loop_points(const Polyline& pl) {
    std::vector<Point> pts = pl.get_points();
    if (pts.size() > 1 && pts.front().distance(pts.back()) < TOL * 0.1)
        pts.pop_back();
    return pts;
}

Polyline close_ring(std::vector<Point> pts) {
    if (!pts.empty() && pts.front().distance(pts.back()) >= 1e-6)
        pts.push_back(pts.front());
    return Polyline(pts);
}

double area_2d(const std::vector<Point>& pts) {
    double a = 0.0;
    for (size_t i = 0; i < pts.size(); ++i) {
        const Point& p = pts[i];
        const Point& q = pts[(i + 1) % pts.size()];
        a += p[0] * q[1] - q[0] * p[1];
    }
    return 0.5 * a;
}

Vector left_2d(const Vector& d) {
    return Vector(-d[1], d[0], 0.0);
}

Vector right_2d(const Vector& d) {
    return Vector(d[1], -d[0], 0.0);
}

double wrap_ccw(double rad) {
    while (rad < 0.0)
        rad += Tolerance::TWO_PI;
    while (rad >= Tolerance::TWO_PI)
        rad -= Tolerance::TWO_PI;
    return rad;
}

/// Intersection `c` of `{x . n1 = d1}` and `{x . n2 = d2}` in 2D.
std::optional<Vector> solve_2d_lines(const Vector& n1, double d1, const Vector& n2, double d2) {
    const double det = n1[0] * n2[1] - n1[1] * n2[0];
    if (std::abs(det) < 1e-7)
        return std::nullopt;
    return Vector((d1 * n2[1] - n1[1] * d2) / det, (n1[0] * d2 - d1 * n2[0]) / det, 0.0);
}

/// Counter-clockwise 2D convex hull (monotone chain) in `(u, v, 0)`.
std::vector<Point> convex_hull_2d(std::vector<Point> pts) {
    std::sort(pts.begin(), pts.end(), [](const Point& a, const Point& b) {
        return std::abs(a[0] - b[0]) > 1e-6 ? a[0] < b[0] : a[1] < b[1];
    });
    pts.erase(
        std::unique(pts.begin(), pts.end(), [](const Point& a, const Point& b) {
            return std::hypot(a[0] - b[0], a[1] - b[1]) < 0.05;
        }),
        pts.end()
    );
    if (pts.size() <= 2)
        return pts;

    auto cross = [](const Point& o, const Point& a, const Point& b) {
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0]);
    };
    std::vector<Point> hull;
    for (const Point& p : pts) {
        while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull.back(), p) <= 1e-5)
            hull.pop_back();
        hull.push_back(p);
    }
    const size_t lower_n = hull.size();
    for (size_t i = pts.size() - 1; i > 0; --i) {
        const Point& p = pts[i - 1];
        while (hull.size() > lower_n && cross(hull[hull.size() - 2], hull.back(), p) <= 1e-5)
            hull.pop_back();
        hull.push_back(p);
    }
    hull.pop_back();
    return hull;
}

/// Clips a 2D CCW polygon against half-plane `(P - origin) . normal_in >= 0`.
std::vector<Point> clip_polygon_halfplane_2d(
    const std::vector<Point>& poly, const Point& origin, const Vector& normal_in
) {
    if (poly.empty())
        return {};
    std::vector<Point> out;
    for (size_t i = 0; i < poly.size(); ++i) {
        const Point& a = poly[i];
        const Point& b = poly[(i + 1) % poly.size()];
        const double da = (a - origin).dot(normal_in);
        const double db = (b - origin).dot(normal_in);
        if (da >= -1e-5)
            out.push_back(a);
        if ((da >= -1e-5) != (db >= -1e-5) && std::abs(da - db) > 1e-9) {
            const double t = da / (da - db);
            out.emplace_back(a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, 0.0);
        }
    }
    return out;
}

void add_unique_plane(std::vector<Plane>& planes, const Plane& candidate) {
    for (const Plane& p : planes) {
        if (p.z_axis().dot(candidate.z_axis()) > 0.9999 &&
            std::abs((candidate.origin() - p.origin()).dot(p.z_axis())) < 0.05) {
            return;
        }
    }
    planes.push_back(candidate);
}

Plane lift_plane_uv(const Plan& plan, const Point& origin_uv, const Vector& normal_uv) {
    const Point w_orig = plan.to_world(origin_uv[0], origin_uv[1], 0.0);
    const Vector w_norm =
        (plan.frame.x_axis() * normal_uv[0] + plan.frame.y_axis() * normal_uv[1] +
         plan.frame.z_axis() * normal_uv[2])
            .normalized();
    return Plane::from_point_normal(w_orig, w_norm);
}

// ═══════════════════════════════════════════════════════════════════════════
// Member Classification
// ═══════════════════════════════════════════════════════════════════════════

enum class Role { None, Girder, Beam, Purlin };

struct EdgeSpec {
    Role role = Role::None;
    int tier = 0;
    double width = 0.0;
    double depth = 0.0;
    double w_bot = 0.0;
    double w_top = 0.0;
    std::vector<Polyline> profile;
};

const std::vector<Polyline>& secondary_beam_profile(const Profiles& p) {
    return p.beam.empty() ? p.girder : p.beam;
}

EdgeSpec classify_edge(const PlanEdge& edge, const Framing& framing) {
    EdgeSpec spec;
    if (edge.core || framing.system == System::Slab)
        return spec;

    if (framing.system == System::PostAndBeam) {
        const bool is_primary = (framing.span < 0) || (edge.family == framing.span);
        spec.role = is_primary ? Role::Girder : Role::Beam;
        spec.profile = is_primary ? framing.profiles.girder : secondary_beam_profile(framing.profiles);
        const auto [w, d] = wood_session::compute_size(spec.profile);
        spec.width = w;
        spec.depth = d;
        spec.w_top = 0.0;
        spec.w_bot = -d;
        if (framing.span < 0)
            spec.tier = 2;
        else
            spec.tier = (is_primary || (edge.boundary && edge.family < 0)) ? 3 : 2;
        return spec;
    }

    // System::GirderPurlin
    const int primary_fam = std::max(0, framing.span);
    const auto [gw, gd] = wood_session::compute_size(framing.profiles.girder);
    const auto [pw, pd] = wood_session::compute_size(framing.profiles.purlin);

    if (edge.family == primary_fam) {
        spec.role = Role::Girder;
        spec.profile = framing.profiles.girder;
        spec.width = gw;
        spec.depth = gd;
        if (framing.purlin == PurlinMode::Stacked) {
            spec.w_top = -pd;
            spec.w_bot = -pd - gd;
            spec.tier = 2;
        } else {
            spec.w_top = 0.0;
            spec.w_bot = -gd;
            spec.tier = 3;
        }
        return spec;
    }

    // Non-primary boundary/rim edge in `PurlinMode::Flush` carries a full edge beam (`beam` or `girder`)
    // at tier 2 so primary girders run flush to the corner face and secondary rim beams T-butt cleanly!
    // (Oblique silhouette edges `edge.family < 0` get tier 3 so continuous diagonal/rake walls aren't chopped.)
    if (framing.purlin == PurlinMode::Flush && edge.boundary) {
        spec.role = Role::Beam;
        spec.profile = secondary_beam_profile(framing.profiles);
        const auto [bw, bd] = wood_session::compute_size(spec.profile);
        spec.width = bw;
        spec.depth = bd;
        spec.w_top = 0.0;
        spec.w_bot = -bd;
        spec.tier = (edge.family < 0) ? 3 : 2;
        return spec;
    }

    // Interior non-primary grid line (or `PurlinMode::Stacked` upper layer) carries a purlin.
    spec.role = Role::Purlin;
    spec.profile = framing.profiles.purlin;
    spec.width = pw;
    spec.depth = pd;
    spec.w_top = 0.0;
    spec.w_bot = -pd;
    spec.tier = edge.boundary ? 2 : 1;
    return spec;
}

bool layers_overlap(const EdgeSpec& a, const EdgeSpec& b) {
    return std::min(a.w_top, b.w_top) - std::max(a.w_bot, b.w_bot) > 1e-3;
}

// ═══════════════════════════════════════════════════════════════════════════
// Standing Column Footprints & Exit Cuts in (u, v)
// ═══════════════════════════════════════════════════════════════════════════

struct ColumnFootprint {
    std::vector<Point> raw_uv;         // Unclipped CCW polygon in (u, v)
    std::vector<Point> inline_head_uv; // Non-empty when `NodeMode::Head` sits in-line with beams!
    std::vector<Plane> facade_clips;   // Exterior boundary planes clipping oblique corner columns/heads
    Vector up_uv = Vector(0.0, 1.0, 0.0);
    double seat_w = 0.0;               // Underside of supported beams/slab at this node
};

std::map<size_t, ColumnFootprint> compute_column_footprints(
    const Plan& plan, const Framing& framing, const std::vector<EdgeSpec>& edge_specs
) {
    std::map<size_t, ColumnFootprint> out;
    if (framing.post_height <= TOL || framing.profiles.column.empty())
        return out;

    const std::vector<Point> prof_pts = loop_points(framing.profiles.column.front());
    if (prof_pts.size() < 3)
        return out;

    // Precompute incident edges per node.
    std::vector<std::vector<size_t>> adj(plan.nodes.size());
    for (size_t ei = 0; ei < plan.edges.size(); ++ei) {
        adj[plan.edges[ei].a].push_back(ei);
        adj[plan.edges[ei].b].push_back(ei);
    }

    // Precompute cell interior side for boundary edges so exterior facade planes point inward.
    std::map<std::pair<size_t, size_t>, Vector> boundary_inward_uv;
    for (const PlanCell& cell : plan.cells) {
        auto record_ring = [&](const std::vector<size_t>& loop) {
            for (size_t i = 0; i < loop.size(); ++i) {
                const size_t u = loop[i];
                const size_t v = loop[(i + 1) % loop.size()];
                const Vector d = (plan.nodes[v] - plan.nodes[u]).normalized();
                // Loop is CCW around kept cell interior -> left_2d(d) points into cell!
                const auto key = std::minmax(u, v);
                boundary_inward_uv[key] = left_2d(d);
            }
        };
        record_ring(cell.outer);
        for (const auto& h : cell.holes)
            record_ring(h);
    }

    for (size_t v = 0; v < plan.nodes.size(); ++v) {
        if (v >= plan.standing.size() || !plan.standing[v])
            continue;

        // Orient column x-axis along first boundary edge (or primary edge) at v.
        Vector x_uv(1.0, 0.0, 0.0);
        std::vector<Vector> bnd_dirs;
        for (size_t ei : adj[v]) {
            const PlanEdge& e = plan.edges[ei];
            const size_t other = (e.a == v) ? e.b : e.a;
            const Vector d = (plan.nodes[other] - plan.nodes[v]).normalized();
            if (e.boundary)
                bnd_dirs.push_back(d);
        }
        if (!bnd_dirs.empty()) {
            x_uv = bnd_dirs.front();
        } else if (!adj[v].empty()) {
            const PlanEdge& e = plan.edges[adj[v].front()];
            const size_t other = (e.a == v) ? e.b : e.a;
            x_uv = (plan.nodes[other] - plan.nodes[v]).normalized();
        }
        const Vector y_uv = left_2d(x_uv);

        ColumnFootprint cf;
        cf.up_uv = y_uv;
        for (const Point& p : prof_pts) {
            cf.raw_uv.push_back(plan.nodes[v] + x_uv * p[0] + y_uv * p[1]);
        }
        if (area_2d(cf.raw_uv) < 0.0)
            std::reverse(cf.raw_uv.begin(), cf.raw_uv.end());

        // Seat elevation under highest-priority supported member at v.
        if (framing.system == System::Slab) {
            cf.seat_w = framing.deck_flush ? -framing.deck : 0.0;
        } else {
            double lowest = 0.0;
            int best_tier = -1;
            for (size_t ei : adj[v]) {
                const EdgeSpec& es = edge_specs[ei];
                if (es.role == Role::None)
                    continue;
                if (framing.purlin == PurlinMode::Stacked && es.role == Role::Girder) {
                    lowest = std::min(lowest, es.w_bot);
                    best_tier = 100;
                } else if (best_tier < 100) {
                    if (es.tier > best_tier) {
                        best_tier = es.tier;
                        lowest = es.w_bot;
                    } else if (es.tier == best_tier) {
                        lowest = std::min(lowest, es.w_bot);
                    }
                }
            }
            cf.seat_w = lowest;
        }

        // Collect exterior boundary half-planes `(origin_uv, inward_normal)` at this node.
        std::vector<std::pair<Point, Vector>> bnd_halfplanes_uv;
        for (size_t ei : adj[v]) {
            const PlanEdge& e = plan.edges[ei];
            if (!e.boundary)
                continue;
            const auto key = std::minmax(e.a, e.b);
            if (!boundary_inward_uv.count(key))
                continue;
            const Vector n_in = boundary_inward_uv.at(key);
            const double half_w =
                edge_specs[ei].role != Role::None
                    ? 0.5 * edge_specs[ei].width
                    : wood_session::compute_size(framing.profiles.column).first * 0.5;
            bnd_halfplanes_uv.emplace_back(plan.nodes[v] - n_in * half_w, n_in);
        }

        // At true non-90-degree, non-180-degree boundary corners, clip column to exterior facade planes.
        if (bnd_dirs.size() >= 2) {
            const double d_abs = std::abs(bnd_dirs[0].dot(bnd_dirs[1]));
            if (d_abs > 0.02 && d_abs < 0.995) {
                for (const auto& [orig_p, n_in] : bnd_halfplanes_uv)
                    cf.facade_clips.push_back(lift_plane_uv(plan, orig_p, n_in));
            }
        }

        // Build chamfered IN-LINE head polygon (`[-beam_depth, 0]`) when `NodeMode::Head` frames beams!
        if (framing.node == NodeMode::Head && framing.system != System::Slab &&
            framing.head_reach > TOL) {
            const double col_hw = 0.5 * wood_session::compute_size(framing.profiles.column).first;
            double reach = col_hw + framing.head_reach;

            struct HeadRay {
                Vector d;
                double ang;
                double hw;
            };
            std::vector<HeadRay> hrays;
            for (size_t ei : adj[v]) {
                const EdgeSpec& es = edge_specs[ei];
                if (es.role == Role::None || es.w_bot > cf.seat_w + 0.5 * es.depth)
                    continue;
                const PlanEdge& e = plan.edges[ei];
                const size_t ov = (e.a == v) ? e.b : e.a;
                const Vector d = (plan.nodes[ov] - plan.nodes[v]).normalized();
                hrays.push_back({d, std::atan2(d[1], d[0]), 0.5 * es.width});
            }
            std::sort(hrays.begin(), hrays.end(), [](const HeadRay& a, const HeadRay& b) {
                return a.ang < b.ang;
            });
            for (size_t i = 0; i < hrays.size() && hrays.size() >= 2; ++i) {
                const HeadRay& a = hrays[i];
                const HeadRay& b = hrays[(i + 1) % hrays.size()];
                const double gap = wrap_ccw(b.ang - a.ang);
                if (gap > 0.20 && gap < Tolerance::PI - 0.20) {
                    const double clear_t = (b.hw + a.hw * std::cos(gap)) / std::sin(gap);
                    reach = std::max(reach, std::min(clear_t + 2.0, col_hw * 3.0));
                }
            }

            std::vector<Point> pts = cf.raw_uv;
            for (const HeadRay& hr : hrays) {
                const Vector s = left_2d(hr.d);
                pts.push_back(plan.nodes[v] + hr.d * reach + s * hr.hw);
                pts.push_back(plan.nodes[v] + hr.d * reach - s * hr.hw);
            }
            std::vector<Point> hull = convex_hull_2d(std::move(pts));
            for (const auto& [orig_p, n_in] : bnd_halfplanes_uv)
                hull = clip_polygon_halfplane_2d(hull, orig_p, n_in);
            if (hull.size() >= 3)
                cf.inline_head_uv = std::move(hull);
        }

        out[v] = std::move(cf);
    }

    return out;
}

std::optional<Plane> polygon_exit_plane_uv(
    const Plan& plan,
    const std::vector<Point>& poly_uv,
    const Point& origin_uv,
    const Vector& dir_uv,
    const std::optional<Point>& col_foot_w = std::nullopt
) {
    double best_t = std::numeric_limits<double>::max();
    std::optional<Plane> best;
    const Point top_node_w = plan.to_world(origin_uv[0], origin_uv[1], 0.0);
    for (size_t i = 0; i < poly_uv.size(); ++i) {
        const Point& a = poly_uv[i];
        const Point& b = poly_uv[(i + 1) % poly_uv.size()];
        const Vector edge = b - a;
        const double len = edge.magnitude();
        if (len < 1e-6)
            continue;
        const Vector n_out = right_2d(edge / len); // CCW loop -> right_2d is outward normal!
        const double denom = dir_uv.dot(n_out);
        if (denom <= 1e-6)
            continue;
        const double t = (a - origin_uv).dot(n_out) / denom;
        if (t > -TOL && t < best_t) {
            best_t = t;
            if (col_foot_w.has_value()) {
                const Vector down_w = *col_foot_w - top_node_w;
                const Point wa = plan.to_world(a[0], a[1], 0.0);
                const Point wb = plan.to_world(b[0], b[1], 0.0);
                const Vector out_horiz =
                    plan.frame.x_axis() * n_out[0] + plan.frame.y_axis() * n_out[1];
                Vector n_3d = down_w.cross(wb - wa).normalized();
                if (n_3d.dot(out_horiz) < 0.0)
                    n_3d = -n_3d;
                best = Plane::from_point_normal(wa, n_3d);
            } else {
                best = lift_plane_uv(plan, a, n_out);
            }
        }
    }
    return best;
}

// ═══════════════════════════════════════════════════════════════════════════
// Exact Multi-Tier Node Joint Solver in (u, v)
// ═══════════════════════════════════════════════════════════════════════════

struct EndTrim {
    double overrun = 0.0;
    std::vector<Plane> planes;
};

struct IncidentRay {
    size_t edge_idx = 0;
    Vector dir; // Outgoing unit vector in (u, v)
    double angle = 0.0;
    int tier = 0;
    double half_w = 0.0;
};

double plane_overrun_uv(const Vector& dir, double half_w, const Vector& keep_n) {
    const double speed = dir.dot(keep_n);
    if (speed <= 1e-4)
        return 0.0;
    return half_w * std::abs(left_2d(dir).dot(keep_n)) / speed;
}

/// Miter plane keeping ray `a` (`dir_a`, `hw_a`) on its CW side against CCW neighbour `b` (`dir_b`, `hw_b`).
std::pair<Plane, double> sector_miter_ccw(
    const Plan& plan,
    const Point& node_uv,
    const Vector& dir_a,
    double hw_a,
    const Vector& dir_b,
    double hw_b
) {
    const double delta = wrap_ccw(std::atan2(dir_b[1], dir_b[0]) - std::atan2(dir_a[1], dir_a[0]));
    // Near-180 collinear continuation -> clean bisector butt at node.
    if (std::abs(delta - Tolerance::PI) < 0.09) {
        const Vector diff = dir_a - dir_b;
        const Vector n = diff.magnitude() > 1e-6 ? diff.normalized() : dir_a;
        return {lift_plane_uv(plan, node_uv, n), plane_overrun_uv(dir_a, hw_a, n)};
    }

    const Vector s_a_ccw = left_2d(dir_a);
    const Vector s_b_cw = right_2d(dir_b);
    if (const auto corner = solve_2d_lines(s_a_ccw, hw_a, s_b_cw, hw_b); corner.has_value()) {
        const double mag = corner->magnitude();
        if (mag > 1e-6) {
            const Vector tangent = *corner / mag;
            const Vector keep_a = right_2d(tangent); // Points into ray A's sector CW of tangent!
            return {lift_plane_uv(plan, node_uv, keep_a), plane_overrun_uv(dir_a, hw_a, keep_a)};
        }
    }

    const Vector diff = dir_a - dir_b;
    const Vector n = diff.magnitude() > 1e-6 ? diff.normalized() : dir_a;
    return {lift_plane_uv(plan, node_uv, n), hw_a};
}

EndTrim compute_node_trim(
    const Plan& plan,
    const Framing& framing,
    const std::vector<EdgeSpec>& specs,
    const std::map<size_t, ColumnFootprint>& columns,
    const std::vector<std::vector<size_t>>& node_adj,
    size_t node_v,
    size_t target_ei
) {
    EndTrim trim;
    const PlanEdge& target_e = plan.edges[target_ei];
    const EdgeSpec& target_s = specs[target_ei];
    const size_t other_v = (target_e.a == node_v) ? target_e.b : target_e.a;
    const Point origin_uv = plan.nodes[node_v];
    const Vector target_dir = (plan.nodes[other_v] - origin_uv).normalized();
    const double span_len = origin_uv.distance(plan.nodes[other_v]);
    const double max_overrun = std::min(target_s.width * 2.5, span_len * 0.4);

    // 1. Continuous column (`NodeMode::Through`) or in-line head (`NodeMode::Head`) cuts beams at its exit face!
    const bool has_inline_head =
        framing.node == NodeMode::Head && columns.count(node_v) > 0 &&
        !columns.at(node_v).inline_head_uv.empty() &&
        target_s.w_bot <= columns.at(node_v).seat_w + 0.5 * target_s.depth;
    if (has_inline_head) {
        if (const auto hp = polygon_exit_plane_uv(
                plan, columns.at(node_v).inline_head_uv, origin_uv, target_dir
            );
            hp.has_value()) {
            add_unique_plane(trim.planes, *hp);
        }
        trim.overrun = 0.0;
        return trim;
    }

    const bool has_through_col =
        framing.node == NodeMode::Through && columns.count(node_v) > 0;
    double through_overrun = 0.0;
    if (has_through_col) {
        const std::optional<Point> col_foot_w =
            (node_v < plan.column_feet.size()) ? plan.column_feet[node_v] : std::nullopt;
        if (const auto col_plane = polygon_exit_plane_uv(
                plan, columns.at(node_v).raw_uv, origin_uv, target_dir, col_foot_w
            );
            col_plane.has_value()) {
            add_unique_plane(trim.planes, *col_plane);
            if (col_foot_w.has_value()) {
                const Vector w_dir =
                    (plan.frame.x_axis() * target_dir[0] + plan.frame.y_axis() * target_dir[1])
                        .normalized();
                const double speed = w_dir.dot(col_plane->z_axis());
                if (speed > 0.15) {
                    const Point soffit_orig =
                        plan.to_world(origin_uv[0], origin_uv[1], target_s.w_bot);
                    const double t_soffit =
                        (col_plane->origin() - soffit_orig).dot(col_plane->z_axis()) / speed;
                    through_overrun = std::max(0.0, -t_soffit);
                }
            }
        }
    }

    // Gather all active beams at `node_v` whose vertical `[w_bot, w_top]` layer overlaps `target_s`.
    std::vector<IncidentRay> rays;
    for (size_t ei : node_adj[node_v]) {
        const EdgeSpec& es = specs[ei];
        if (es.role == Role::None || !layers_overlap(target_s, es))
            continue;
        const PlanEdge& e = plan.edges[ei];
        const size_t ov = (e.a == node_v) ? e.b : e.a;
        const Vector d = (plan.nodes[ov] - origin_uv).normalized();
        int eff_tier = es.tier;
        rays.push_back(IncidentRay{ei, d, std::atan2(d[1], d[0]), eff_tier, 0.5 * es.width});
    }
    std::sort(rays.begin(), rays.end(), [](const IncidentRay& a, const IncidentRay& b) {
        return a.angle < b.angle;
    });

    // Promote straight-through collinear continuous members (perimeter walls, mullions passing window corners,
    // girders passing courtyard corners, or 3-ray T-junctions) so transverse sills/trimmers butt cleanly at 90 deg!
    if (rays.size() >= 3) {
        int max_t = rays[0].tier;
        for (const IncidentRay& r : rays)
            max_t = std::max(max_t, r.tier);

        int best_i = -1;
        int best_j = -1;
        std::pair<int, int> best_score{-1, -1};
        for (size_t i = 0; i < rays.size(); ++i) {
            for (size_t j = i + 1; j < rays.size(); ++j) {
                if (rays[i].dir.dot(rays[j].dir) >= -0.985)
                    continue;
                if (std::abs(rays[i].half_w - rays[j].half_w) > 1.0)
                    continue;
                const int hi = std::max(rays[i].tier, rays[j].tier);
                const int lo = std::min(rays[i].tier, rays[j].tier);
                if (hi < max_t)
                    continue;
                const bool both_bnd = plan.edges[rays[i].edge_idx].boundary &&
                                      plan.edges[rays[j].edge_idx].boundary;
                const bool mixed_cont = (hi != lo);
                if (!both_bnd && !mixed_cont && rays.size() > 3)
                    continue;
                const std::pair<int, int> sc{both_bnd ? hi + 1 : hi, lo};
                if (sc > best_score) {
                    best_score = sc;
                    best_i = static_cast<int>(i);
                    best_j = static_cast<int>(j);
                }
            }
        }
        if (best_i >= 0) {
            rays[best_i].tier = max_t + 1;
            rays[best_j].tier = max_t + 1;
        } else if (framing.span >= 0 && rays.size() == 3) {
            // Non-collinear 3-ray perimeter corner/apex (e.g. gable peak or polygon corner with interior beam):
            // promote the two boundary edges so the interior member T-butts cleanly into their inner faces!
            std::vector<size_t> bnds;
            for (size_t i = 0; i < rays.size(); ++i)
                if (plan.edges[rays[i].edge_idx].boundary)
                    bnds.push_back(i);
            if (bnds.size() == 2 &&
                std::max(rays[bnds[0]].tier, rays[bnds[1]].tier) == max_t) {
                const IncidentRay& a = rays[bnds[0]];
                const IncidentRay& b = rays[bnds[1]];
                const double turn = std::abs(Tolerance::PI - wrap_ccw(b.angle - a.angle));
                const bool sym_apex =
                    std::abs(std::abs(a.dir[0]) - std::abs(b.dir[0])) < 1e-3 &&
                    a.dir[1] * b.dir[1] > 0.0;
                if (turn < 0.68 || sym_apex) {
                    rays[bnds[0]].tier = max_t + 1;
                    rays[bnds[1]].tier = max_t + 1;
                } else {
                    const bool a_wins =
                        std::abs(a.half_w - b.half_w) > 1.0
                            ? (a.half_w > b.half_w)
                            : (std::abs(std::abs(a.dir[0]) - std::abs(b.dir[0])) > 1e-3
                                   ? (std::abs(a.dir[0]) > std::abs(b.dir[0]))
                                   : (a.edge_idx < b.edge_idx));
                    rays[bnds[a_wins ? 0 : 1]].tier = max_t + 2;
                    rays[bnds[a_wins ? 1 : 0]].tier = max_t + 1;
                }
            }
        }
    } else if (rays.size() == 2 && framing.span >= 0 && rays[0].tier == rays[1].tier) {
        // 2-ray corner in hierarchical framing (`span >= 0`): prefer a T-section bypass-and-butt joint
        // over a miter (except at gentle polygonal ring bends `< 39 deg` or mirror-symmetric gable peaks)!
        const double turn = std::abs(Tolerance::PI - wrap_ccw(rays[1].angle - rays[0].angle));
        const bool sym_apex =
            std::abs(std::abs(rays[0].dir[0]) - std::abs(rays[1].dir[0])) < 1e-3 &&
            rays[0].dir[1] * rays[1].dir[1] > 0.0;
        if (turn >= 0.68 && !sym_apex) {
            const bool r0_wins =
                std::abs(rays[0].half_w - rays[1].half_w) > 1.0
                    ? (rays[0].half_w > rays[1].half_w)
                    : (std::abs(std::abs(rays[0].dir[0]) - std::abs(rays[1].dir[0])) > 1e-3
                           ? (std::abs(rays[0].dir[0]) > std::abs(rays[1].dir[0]))
                           : (rays[0].edge_idx < rays[1].edge_idx));
            rays[r0_wins ? 0 : 1].tier += 1;
        }
    }

    size_t self_k = 0;
    for (size_t k = 0; k < rays.size(); ++k)
        if (rays[k].edge_idx == target_ei)
            self_k = k;
    const IncidentRay& self = rays[self_k];

    // Collect higher-tier rays at this node.
    std::vector<const IncidentRay*> higher;
    for (const IncidentRay& r : rays)
        if (r.tier > self.tier)
            higher.push_back(&r);

    if (!higher.empty()) {
        if (higher.size() == 1) {
            const IncidentRay& h = *higher[0];
            const double cross_z = h.dir[0] * self.dir[1] - h.dir[1] * self.dir[0];
            if (std::abs(cross_z) < 0.05 && h.dir.dot(self.dir) < 0.0) {
                // Collinear opposite to higher-tier beam: butt at node.
                add_unique_plane(trim.planes, lift_plane_uv(plan, origin_uv, self.dir));
            } else {
                const Vector side_n = cross_z >= 0.0 ? left_2d(h.dir) : right_2d(h.dir);
                add_unique_plane(
                    trim.planes, lift_plane_uv(plan, origin_uv + side_n * h.half_w, side_n)
                );
                const double sin_a = std::max(0.25, std::abs(cross_z));
                trim.overrun = std::max(trim.overrun, self.half_w / sin_a);
            }
        } else {
            // Find the enclosing higher-tier angular sector `[h_cw, h_ccw]` containing `self`.
            const IncidentRay* h_cw = higher.back();
            const IncidentRay* h_ccw = higher.front();
            for (size_t i = 0; i < higher.size(); ++i) {
                const IncidentRay* a = higher[i];
                const IncidentRay* b = higher[(i + 1) % higher.size()];
                const double span = wrap_ccw(b->angle - a->angle);
                const double rel = wrap_ccw(self.angle - a->angle);
                if (rel <= span + 1e-7) {
                    h_cw = a;
                    h_ccw = b;
                    break;
                }
            }
            const double sector_span = wrap_ccw(h_ccw->angle - h_cw->angle);
            const Vector n_cw = left_2d(h_cw->dir);
            const Vector n_ccw = right_2d(h_ccw->dir);
            const Plane p_cw = lift_plane_uv(plan, origin_uv + n_cw * h_cw->half_w, n_cw);
            const Plane p_ccw = lift_plane_uv(plan, origin_uv + n_ccw * h_ccw->half_w, n_ccw);

            if (sector_span <= Tolerance::PI + 0.05) {
                // Convex or straight enclosing sector: intersect both inner side half-spaces!
                add_unique_plane(trim.planes, p_cw);
                add_unique_plane(trim.planes, p_ccw);
            } else {
                // Reflex sector: butt against whichever higher-tier beam `self` faces more directly.
                const double d1 = self.dir.dot(n_cw);
                const double d2 = self.dir.dot(n_ccw);
                add_unique_plane(trim.planes, d1 >= d2 ? p_cw : p_ccw);
            }
            trim.overrun = std::max(trim.overrun, self.half_w * 1.5);
        }

        // Also miter against any same-tier neighbour sharing the same higher-tier sector!
        for (int step : {-1, 1}) {
            if (rays.size() < 2)
                continue;
            const IncidentRay& nb = rays[(self_k + rays.size() + step) % rays.size()];
            if (nb.edge_idx == self.edge_idx || nb.tier != self.tier)
                continue;
            const double gap = step > 0 ? wrap_ccw(nb.angle - self.angle)
                                        : wrap_ccw(self.angle - nb.angle);
            if (gap < Tolerance::PI - 0.05) {
                if (step > 0) {
                    const auto [mp, ov] =
                        sector_miter_ccw(plan, origin_uv, self.dir, self.half_w, nb.dir, nb.half_w);
                    add_unique_plane(trim.planes, mp);
                    trim.overrun = std::max(trim.overrun, ov);
                } else {
                    const auto [mp_nb, _] =
                        sector_miter_ccw(plan, origin_uv, nb.dir, nb.half_w, self.dir, self.half_w);
                    add_unique_plane(
                        trim.planes, Plane::from_point_normal(mp_nb.origin(), -mp_nb.z_axis())
                    );
                }
            }
        }

        trim.overrun =
            has_through_col ? std::min(through_overrun, max_overrun) : std::min(trim.overrun, max_overrun);
        return trim;
    }

    // `self` belongs to the highest beam tier at `node_v`.
    std::vector<const IncidentRay*> top;
    size_t top_self = 0;
    for (const IncidentRay& r : rays) {
        if (r.tier == self.tier) {
            if (r.edge_idx == self.edge_idx)
                top_self = top.size();
            top.push_back(&r);
        }
    }

    if (top.size() == 1) {
        if (has_through_col) {
            trim.overrun = std::min(through_overrun, max_overrun);
        } else {
            // Check if lower-tier boundary/corner rays meet `self`:
            // - Single facade plane (2-ray corner or straight oblique wall): bevel `self` coplanar with that outer face!
            // - Two angled ring facets on opposite sides (e.g. radial inner oculus & outer ring): cut `self` squarely
            //   where its side faces meet both ring beams' outer/oculus corners!
            struct FacadeHit {
                const IncidentRay* ray = nullptr;
                Vector n_in;
                double speed = 0.0;
            };
            std::vector<FacadeHit> hits;
            for (const IncidentRay& r : rays) {
                if (r.edge_idx == self.edge_idx || (!plan.edges[r.edge_idx].boundary && rays.size() > 2))
                    continue;
                const double sin_a = std::abs(self.dir[0] * r.dir[1] - self.dir[1] * r.dir[0]);
                if (sin_a <= 0.15)
                    continue;
                Vector n_in = left_2d(r.dir);
                if (self.dir.dot(n_in) < 0.0)
                    n_in = -n_in;
                const double speed = self.dir.dot(n_in);
                if (speed <= 0.15)
                    continue;
                hits.push_back({&r, n_in, speed});
            }

            bool single_facade_plane = !hits.empty();
            for (size_t i = 1; i < hits.size(); ++i)
                if (hits[i].n_in.dot(hits[0].n_in) < 0.999)
                    single_facade_plane = false;

            double ext = 0.0;
            if (!hits.empty() && single_facade_plane) {
                const Vector self_s = left_2d(self.dir);
                for (const FacadeHit& fh : hits) {
                    const double req =
                        (fh.ray->half_w + self.half_w * std::abs(self_s.dot(fh.n_in))) / fh.speed;
                    ext = std::max(ext, req);
                    add_unique_plane(
                        trim.planes,
                        lift_plane_uv(plan, origin_uv - fh.n_in * fh.ray->half_w, fh.n_in)
                    );
                }
                trim.overrun = std::min(ext, max_overrun);
            } else if (!hits.empty()) {
                for (const FacadeHit& fh : hits) {
                    const Vector s_toward =
                        fh.ray->dir.dot(left_2d(self.dir)) >= 0.0 ? left_2d(self.dir)
                                                                  : right_2d(self.dir);
                    const double req = std::max(
                        0.0, (fh.ray->half_w + self.half_w * s_toward.dot(fh.n_in)) / fh.speed
                    );
                    ext = std::max(ext, req);
                }
                trim.overrun = std::min(ext, max_overrun);
                add_unique_plane(
                    trim.planes, lift_plane_uv(plan, origin_uv - self.dir * trim.overrun, self.dir)
                );
            } else {
                if (rays.size() >= 2) {
                    for (const IncidentRay& r : rays) {
                        if (r.edge_idx == self.edge_idx)
                            continue;
                        const double sin_a =
                            std::abs(self.dir[0] * r.dir[1] - self.dir[1] * r.dir[0]);
                        if (sin_a > 0.25)
                            ext = std::max(ext, r.half_w / sin_a);
                    }
                } else if (columns.count(node_v)) {
                    for (const Point& cp : columns.at(node_v).raw_uv)
                        ext = std::max(ext, (cp - origin_uv).dot(-self.dir));
                }
                trim.overrun = std::min(ext, max_overrun);
                add_unique_plane(
                    trim.planes, lift_plane_uv(plan, origin_uv - self.dir * trim.overrun, self.dir)
                );
            }
        }
        return trim;
    }

    if (top.size() == 2) {
        const IncidentRay& other = *top[1 - top_self];
        const double ccw_gap = wrap_ccw(other.angle - self.angle);
        if (ccw_gap <= Tolerance::PI) {
            const auto [mp, ov] =
                sector_miter_ccw(plan, origin_uv, self.dir, self.half_w, other.dir, other.half_w);
            add_unique_plane(trim.planes, mp);
            trim.overrun = std::max(trim.overrun, ov);
        } else {
            const auto [mp_other, _] =
                sector_miter_ccw(plan, origin_uv, other.dir, other.half_w, self.dir, self.half_w);
            const Plane mp = Plane::from_point_normal(mp_other.origin(), -mp_other.z_axis());
            add_unique_plane(trim.planes, mp);
            const Vector keep_uv(
                mp.z_axis().dot(plan.frame.x_axis()), mp.z_axis().dot(plan.frame.y_axis()), 0.0
            );
            trim.overrun = std::max(trim.overrun, plane_overrun_uv(self.dir, self.half_w, keep_uv));
        }
        trim.overrun =
            has_through_col ? std::min(through_overrun, max_overrun) : std::min(trim.overrun, max_overrun);
        return trim;
    }

    // 3 or more equal-tier beams (e.g. 120 deg hexagonal Y, 4-way cross, 60 deg corner with diagonal, 6-way star):
    // Miter against both immediate CCW and CW angular neighbours around the full 360 deg ring!
    const IncidentRay& nb_ccw = *top[(top_self + 1) % top.size()];
    const IncidentRay& nb_cw = *top[(top_self + top.size() - 1) % top.size()];

    {
        const auto [mp, ov] =
            sector_miter_ccw(plan, origin_uv, self.dir, self.half_w, nb_ccw.dir, nb_ccw.half_w);
        add_unique_plane(trim.planes, mp);
        trim.overrun = std::max(trim.overrun, ov);
    }
    {
        const auto [mp_cw, _] =
            sector_miter_ccw(plan, origin_uv, nb_cw.dir, nb_cw.half_w, self.dir, self.half_w);
        const Plane mp = Plane::from_point_normal(mp_cw.origin(), -mp_cw.z_axis());
        add_unique_plane(trim.planes, mp);
        const Vector keep_uv(
            mp.z_axis().dot(plan.frame.x_axis()), mp.z_axis().dot(plan.frame.y_axis()), 0.0
        );
        trim.overrun = std::max(trim.overrun, plane_overrun_uv(self.dir, self.half_w, keep_uv));
    }

    trim.overrun =
        has_through_col ? std::min(through_overrun, max_overrun) : std::min(trim.overrun, max_overrun);
    return trim;
}

// ═══════════════════════════════════════════════════════════════════════════
// Element Builders on `plan.frame`
// ═══════════════════════════════════════════════════════════════════════════

double kept_axis_length(const Point& a, const Point& b, const std::vector<Plane>& cuts) {
    const Vector dir = (b - a).normalized();
    double low = 0.0;
    double high = a.distance(b);
    for (const Plane& p : cuts) {
        const double speed = dir.dot(p.z_axis());
        const double offset = (p.origin() - a).dot(p.z_axis());
        if (std::abs(speed) < 1e-8 && offset > 1e-4)
            return -1.0;
        if (speed > 1e-8)
            low = std::max(low, offset / speed);
        else if (speed < -1e-8)
            high = std::min(high, offset / speed);
    }
    return high - low;
}

std::shared_ptr<wood_session::EdgeStraightElement> make_plane_member(
    const Plan& plan,
    const Point& a_uv,
    const Point& b_uv,
    double w_mid,
    const std::vector<Polyline>& profile,
    const std::vector<Plane>& cuts,
    const std::string& name
) {
    const Point wa = plan.to_world(a_uv[0], a_uv[1], w_mid);
    const Point wb = plan.to_world(b_uv[0], b_uv[1], w_mid);
    const double min_len = std::max(TOL * 2.0, 0.6 * wood_session::compute_size(profile).first);
    if (kept_axis_length(wa, wb, cuts) <= min_len)
        return nullptr;

    auto elem = std::make_shared<wood_session::EdgeStraightElement>(
        Polyline({wa, wb}), profile, std::vector<Vector>{plan.frame.z_axis()}, name
    );
    elem->cuts = cuts;
    return elem;
}

/// Core wall ring expanded by `wall / 2` in (u, v) for trimming beams reaching a shear core.
std::vector<Polyline> expanded_cores_uv(const Plan& plan, double wall) {
    if (wall <= TOL)
        return plan.cores;
    std::vector<Polyline> rings;
    for (const Polyline& c : plan.cores) {
        Polyline copy = c;
        if (Intersection::offset_in_3d(copy, Plane::xy_plane(), 0.5 * wall))
            rings.push_back(copy);
        else
            rings.push_back(c);
    }
    return rings;
}

/// Clips a beam segment outside expanded core walls and adds core-face butt planes.
std::vector<std::pair<Line, std::vector<Plane>>> split_outside_cores_uv(
    const Plan& plan, const Line& line_uv, const std::vector<Polyline>& outer_cores
) {
    if (outer_cores.empty())
        return {{line_uv, {}}};

    struct Hit {
        double t = 0.0;
        int ring = -1;
        int side = -1;
    };
    std::vector<Hit> hits = {{0.0, -1, -1}, {1.0, -1, -1}};
    for (size_t r = 0; r < outer_cores.size(); ++r) {
        const std::vector<Point> pts = loop_points(outer_cores[r]);
        for (size_t i = 0; i < pts.size(); ++i) {
            const Line seg = Line::from_points(pts[i], pts[(i + 1) % pts.size()]);
            double t = 0.0;
            double s = 0.0;
            if (Intersection::line_line_parameters(line_uv, seg, t, s, TOL, true, false) &&
                line_uv.point_at(t).distance(seg.point_at(s)) <= TOL) {
                hits.push_back({std::clamp(t, 0.0, 1.0), static_cast<int>(r), static_cast<int>(i)});
            }
        }
    }
    std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) { return a.t < b.t; });

    auto inside_cores = [&](const Point& pt) {
        int c = 0;
        for (const Polyline& ring : outer_cores)
            if (ring.point_in_polygon_2d(pt))
                ++c;
        return (c % 2) == 1;
    };

    auto core_face_plane = [&](int r, int side_idx) {
        const std::vector<Point> pts = loop_points(outer_cores[r]);
        const Point& a = pts[side_idx];
        const Point& b = pts[(side_idx + 1) % pts.size()];
        const Vector dir = (b - a).normalized();
        const Vector n_out = area_2d(pts) >= 0.0 ? right_2d(dir) : left_2d(dir);
        return lift_plane_uv(plan, a, n_out);
    };

    std::vector<std::pair<Line, std::vector<Plane>>> out;
    const double len = line_uv.length();
    for (size_t i = 0; i + 1 < hits.size(); ++i) {
        if ((hits[i + 1].t - hits[i].t) * len <= TOL * 2.0)
            continue;
        const Point mid = line_uv.point_at(0.5 * (hits[i].t + hits[i + 1].t));
        if (inside_cores(mid))
            continue;
        std::vector<Plane> extra;
        if (hits[i].ring >= 0)
            extra.push_back(core_face_plane(hits[i].ring, hits[i].side));
        if (hits[i + 1].ring >= 0)
            extra.push_back(core_face_plane(hits[i + 1].ring, hits[i + 1].side));
        out.emplace_back(
            Line::from_points(line_uv.point_at(hits[i].t), line_uv.point_at(hits[i + 1].t)),
            std::move(extra)
        );
    }
    return out;
}

// ═══════════════════════════════════════════════════════════════════════════
// Subdivided Bay Purlins (`System::GirderPurlin`)
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::shared_ptr<Element>> build_bay_purlins(
    const Plan& plan,
    const Framing& framing,
    const std::vector<EdgeSpec>& edge_specs,
    const std::vector<Polyline>& outer_cores
) {
    std::vector<std::shared_ptr<Element>> purlins;
    if (framing.system != System::GirderPurlin || framing.spacing <= TOL)
        return purlins;

    const int primary_fam = std::max(0, framing.span);
    // Determine primary girder direction & purlin direction from the plan's edges.
    Vector girder_dir(1.0, 0.0, 0.0);
    Vector purlin_dir(0.0, 1.0, 0.0);
    bool found_g = false;
    bool found_p = false;
    for (const PlanEdge& e : plan.edges) {
        const Vector d = (plan.nodes[e.b] - plan.nodes[e.a]).normalized();
        if (!found_g && e.family == primary_fam) {
            girder_dir = d;
            found_g = true;
        } else if (!found_p && e.family >= 0 && e.family != primary_fam) {
            purlin_dir = d;
            found_p = true;
        }
    }
    if (!found_p)
        purlin_dir = left_2d(girder_dir);

    const Vector step_axis = left_2d(purlin_dir); // Perpendicular to purlins in (u, v)!
    const auto [pw, pd] = wood_session::compute_size(framing.profiles.purlin);
    const double w_mid = -0.5 * pd;

    std::map<std::pair<size_t, size_t>, size_t> edge_lookup;
    for (size_t ei = 0; ei < plan.edges.size(); ++ei)
        edge_lookup[std::minmax(plan.edges[ei].a, plan.edges[ei].b)] = ei;

    for (const PlanCell& cell : plan.cells) {
        if (cell.outer.size() < 3)
            continue;
        std::vector<Point> poly;
        double s_min = std::numeric_limits<double>::max();
        double s_max = -s_min;
        for (size_t vi : cell.outer) {
            const Point& p = plan.nodes[vi];
            poly.push_back(p);
            const double s = Vector(p[0], p[1], 0.0).dot(step_axis);
            s_min = std::min(s_min, s);
            s_max = std::max(s_max, s);
        }

        const double bay_span = s_max - s_min;
        const int divisions = std::max(1, static_cast<int>(std::round(bay_span / framing.spacing)));
        if (divisions <= 1)
            continue;

        // Precompute cell boundary sides and their trim planes for interior purlins.
        struct CellSide {
            Line seg;
            Plane flush_plane;
            double stacked_overhang = 0.0;
        };
        std::vector<CellSide> sides;
        auto collect_sides = [&](const std::vector<size_t>& loop) {
            for (size_t i = 0; i < loop.size(); ++i) {
                const size_t u = loop[i];
                const size_t v = loop[(i + 1) % loop.size()];
                const Point& a = plan.nodes[u];
                const Point& b = plan.nodes[v];
                const Vector d = (b - a).normalized();
                const Vector n_in = left_2d(d); // Points into cell interior!
                double hw = 0.0;
                if (auto it = edge_lookup.find(std::minmax(u, v)); it != edge_lookup.end())
                    hw = 0.5 * edge_specs[it->second].width;
                sides.push_back(CellSide{
                    Line::from_points(a, b),
                    lift_plane_uv(plan, a + n_in * hw, n_in),
                    hw,
                });
            }
        };
        collect_sides(cell.outer);
        for (const auto& h : cell.holes)
            collect_sides(h);

        std::vector<Polyline> cell_rings = {close_ring(poly)};
        for (const auto& h : cell.holes) {
            std::vector<Point> hp;
            for (size_t vi : h)
                hp.push_back(plan.nodes[vi]);
            cell_rings.push_back(close_ring(hp));
        }

        for (int k = 1; k < divisions; ++k) {
            const double s_val = s_min + bay_span * k / divisions;
            const Point base(step_axis[0] * s_val, step_axis[1] * s_val, 0.0);
            const Line inf_line = Line::from_points(base - purlin_dir * 1e6, base + purlin_dir * 1e6);

            struct Cross {
                double t = 0.0;
                size_t side_id = 0;
            };
            std::vector<Cross> crossings;
            for (size_t si = 0; si < sides.size(); ++si) {
                double t = 0.0;
                double u = 0.0;
                if (Intersection::line_line_parameters(inf_line, sides[si].seg, t, u, TOL, true, false) &&
                    inf_line.point_at(t).distance(sides[si].seg.point_at(u)) <= TOL) {
                    crossings.push_back({t, si});
                }
            }
            std::sort(crossings.begin(), crossings.end(), [](const Cross& a, const Cross& b) {
                return a.t < b.t;
            });

            for (size_t c = 0; c + 1 < crossings.size(); ++c) {
                const Point p0 = inf_line.point_at(crossings[c].t);
                const Point p1 = inf_line.point_at(crossings[c + 1].t);
                if (p0.distance(p1) <= pw)
                    continue;
                const Point mid = Point::mid_point(p0, p1);
                int in_cnt = 0;
                for (const Polyline& r : cell_rings)
                    if (r.point_in_polygon_2d(mid))
                        ++in_cnt;
                if ((in_cnt % 2) != 1)
                    continue;

                const CellSide& s0 = sides[crossings[c].side_id];
                const CellSide& s1 = sides[crossings[c + 1].side_id];
                Point ext0 = p0;
                Point ext1 = p1;
                std::vector<Plane> base_cuts;
                if (framing.purlin == PurlinMode::Stacked) {
                    ext0 = p0 - purlin_dir * s0.stacked_overhang;
                    ext1 = p1 + purlin_dir * s1.stacked_overhang;
                    base_cuts.push_back(lift_plane_uv(plan, ext0, purlin_dir));
                    base_cuts.push_back(lift_plane_uv(plan, ext1, -purlin_dir));
                } else {
                    ext0 = p0 - purlin_dir * pw;
                    ext1 = p1 + purlin_dir * pw;
                    base_cuts.push_back(s0.flush_plane);
                    base_cuts.push_back(s1.flush_plane);
                }

                for (const auto& [sub_seg, core_cuts] :
                     split_outside_cores_uv(plan, Line::from_points(ext0, ext1), outer_cores)) {
                    std::vector<Plane> cuts = base_cuts;
                    for (const Plane& cp : core_cuts)
                        add_unique_plane(cuts, cp);
                    if (auto elem = make_plane_member(
                            plan,
                            sub_seg.start(),
                            sub_seg.end(),
                            w_mid,
                            framing.profiles.purlin,
                            cuts,
                            "purlin"
                        )) {
                        purlins.push_back(elem);
                    }
                }
            }
        }
    }

    return purlins;
}

// ═══════════════════════════════════════════════════════════════════════════
// In-Plane Timber Braces (`BraceMode`: Diagonal, Cross, Chevron)
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::shared_ptr<Element>> build_bay_braces(
    const Plan& plan, const Framing& framing, const std::vector<EdgeSpec>& edge_specs
) {
    std::vector<std::shared_ptr<Element>> braces;
    if (framing.brace == BraceMode::None || framing.profiles.brace.empty())
        return braces;

    const auto [bw, bd] = wood_session::compute_size(framing.profiles.brace);
    // Center braces inside the primary framing depth (or centered in [-bd, 0] on Slab).
    const double frame_d = framing.system == System::Slab
                               ? bd
                               : wood_session::compute_size(framing.profiles.girder).second;
    const double w_mid = -0.5 * std::max(bd, frame_d);

    std::map<std::pair<size_t, size_t>, double> edge_hw;
    for (size_t ei = 0; ei < plan.edges.size(); ++ei)
        edge_hw[std::minmax(plan.edges[ei].a, plan.edges[ei].b)] = 0.5 * edge_specs[ei].width;

    std::set<size_t> filter(framing.braced_bays.begin(), framing.braced_bays.end());

    for (size_t ci = 0; ci < plan.cells.size(); ++ci) {
        if (!filter.empty() && !filter.count(ci))
            continue;
        const PlanCell& cell = plan.cells[ci];
        if (cell.outer.size() != 4 || !cell.holes.empty())
            continue;

        // Inset the 4 bay sides by the inner half-widths of their bounding frame members.
        std::array<Point, 4> raw;
        std::vector<double> insets;
        for (size_t i = 0; i < 4; ++i) {
            const size_t u = cell.outer[i];
            const size_t v = cell.outer[(i + 1) % 4];
            raw[i] = plan.nodes[u];
            insets.push_back(-edge_hw[std::minmax(u, v)]);
        }
        const Polyline shrunk = close_ring({raw[0], raw[1], raw[2], raw[3]}).offset_sides(insets);
        const std::vector<Point> q = loop_points(shrunk);
        if (q.size() != 4 || area_2d(q) <= bw * bw)
            continue;

        std::vector<Plane> box_planes;
        for (size_t i = 0; i < 4; ++i) {
            const Vector edge_dir = (q[(i + 1) % 4] - q[i]).normalized();
            box_planes.push_back(lift_plane_uv(plan, q[i], left_2d(edge_dir)));
        }

        auto add_brace_seg = [&](const Point& p0, const Point& p1, std::vector<Plane> extra) {
            const Vector d = (p1 - p0).normalized();
            for (const Plane& bp : box_planes)
                add_unique_plane(extra, bp);
            if (auto elem = make_plane_member(
                    plan,
                    p0 - d * (bw * 2.0),
                    p1 + d * (bw * 2.0),
                    w_mid,
                    framing.profiles.brace,
                    extra,
                    "brace"
                )) {
                braces.push_back(elem);
            }
        };

        if (framing.brace == BraceMode::Diagonal) {
            add_brace_seg(q[0], q[2], {});
        } else if (framing.brace == BraceMode::Cross) {
            // Primary continuous diagonal Q0 -> Q2; secondary Q1 -> Q3 split cleanly against its sides.
            add_brace_seg(q[0], q[2], {});
            const Vector d_main = (q[2] - q[0]).normalized();
            const Vector n_left = left_2d(d_main);
            const Point mid = Point::centroid(q);
            add_brace_seg(
                q[1], mid, {lift_plane_uv(plan, q[0] - n_left * (0.5 * bw), -n_left)}
            );
            add_brace_seg(
                mid, q[3], {lift_plane_uv(plan, q[0] + n_left * (0.5 * bw), n_left)}
            );
        } else if (framing.brace == BraceMode::Chevron) {
            // Order bottom edge as lowest v-coordinate edge of the quad.
            size_t bot_i = 0;
            double best_v = std::numeric_limits<double>::max();
            for (size_t i = 0; i < 4; ++i) {
                const double mid_v = 0.5 * (q[i][1] + q[(i + 1) % 4][1]);
                if (mid_v < best_v) {
                    best_v = mid_v;
                    bot_i = i;
                }
            }
            const Point& b0 = q[bot_i];
            const Point& b1 = q[(bot_i + 1) % 4];
            const Point apex = Point::mid_point(q[(bot_i + 2) % 4], q[(bot_i + 3) % 4]);
            const Vector bisector_n = (b1 - b0).normalized();
            add_brace_seg(b0, apex, {lift_plane_uv(plan, apex, -bisector_n)});
            add_brace_seg(b1, apex, {lift_plane_uv(plan, apex, bisector_n)});
        }
    }

    return braces;
}

// ═══════════════════════════════════════════════════════════════════════════
// Columns, Single Chamfered Timber Head, CLT Decks, and Core Shear Walls
// ═══════════════════════════════════════════════════════════════════════════

struct BuiltSupport {
    std::shared_ptr<Element> column;
    std::shared_ptr<Element> head;
};

BuiltSupport build_column_and_head(
    const Plan& plan, const Framing& framing, size_t v, const ColumnFootprint& cf
) {
    BuiltSupport out;
    const double w_foot = -framing.post_height;
    const bool has_inline_head =
        framing.node == NodeMode::Head && !cf.inline_head_uv.empty();
    const bool has_slab_head =
        framing.node == NodeMode::Head && framing.system == System::Slab &&
        framing.head_height > TOL && framing.head_reach > TOL;
    const double w_col_top = framing.node == NodeMode::Through
                                 ? 0.0
                                 : (has_slab_head ? cf.seat_w - framing.head_height : cf.seat_w);

    const bool custom_foot = v < plan.column_feet.size() && plan.column_feet[v].has_value();
    const Point top_datum_w = plan.to_world(plan.nodes[v][0], plan.nodes[v][1], 0.0);
    const Point foot_w = custom_foot
                             ? *plan.column_feet[v]
                             : plan.to_world(plan.nodes[v][0], plan.nodes[v][1], w_foot);
    const double dz_rem = (top_datum_w - foot_w).dot(plan.frame.z_axis());
    const Point top_w =
        (custom_foot && dz_rem > TOL)
            ? top_datum_w + (top_datum_w - foot_w) * (w_col_top / dz_rem)
            : plan.to_world(plan.nodes[v][0], plan.nodes[v][1], w_col_top);

    if ((top_w - foot_w).dot(plan.frame.z_axis()) > TOL * 2.0) {
        const Vector axis_dir = (top_w - foot_w).normalized();
        Vector up_w =
            (plan.frame.x_axis() * cf.up_uv[0] + plan.frame.y_axis() * cf.up_uv[1]).normalized();
        const bool is_inclined = std::abs(axis_dir.dot(plan.frame.z_axis())) < 0.9999;

        if (is_inclined) {
            const Point nominal_top = plan.to_world(plan.nodes[v][0], plan.nodes[v][1], w_col_top);
            const Polyline top_ring =
                plan.lift_polyline(close_ring(cf.raw_uv), w_col_top).translated(top_w - nominal_top);
            const Polyline bot_ring = top_ring.translated(foot_w - top_w);
            out.column = wood_session::EdgeVariableElement::between(bot_ring, top_ring, "column");
        } else {
            auto col = std::make_shared<wood_session::EdgeStraightElement>(
                Polyline({foot_w, top_w}),
                framing.profiles.column,
                std::vector<Vector>{up_w},
                "column"
            );
            col->cuts = cf.facade_clips;
            out.column = col;
        }
    }

    if (has_inline_head) {
        // In-line chamfered timber head block (`[cf.seat_w, 0.0]`): flush with beam tops & soffits!
        const Polyline ring_uv = close_ring(cf.inline_head_uv);
        auto head = std::make_shared<wood_session::NodeBlockElement>(
            std::vector<Polyline>{
                plan.lift_polyline(ring_uv, cf.seat_w),
                plan.lift_polyline(ring_uv, 0.0),
            },
            "head"
        );
        head->invalidate_geometry();
        out.head = head;
    } else if (has_slab_head) {
        // Under-slab chamfered timber head (`[w_col_top, cf.seat_w]`): flares outward on interior sides,
        // stays flush with boundary edges on perimeter/corner columns!
        std::vector<size_t> bnd_edges;
        for (size_t ei = 0; ei < plan.edges.size(); ++ei)
            if (plan.edges[ei].boundary && (plan.edges[ei].a == v || plan.edges[ei].b == v))
                bnd_edges.push_back(ei);

        std::vector<Plane> perim_limits = cf.facade_clips;
        if (!bnd_edges.empty() && perim_limits.empty()) {
            for (const PlanCell& cell : plan.cells) {
                const auto& loop = cell.outer;
                for (size_t i = 0; i < loop.size(); ++i) {
                    const size_t a = loop[i];
                    const size_t b = loop[(i + 1) % loop.size()];
                    if (a != v && b != v)
                        continue;
                    bool is_bnd = false;
                    for (size_t ei : bnd_edges)
                        if (std::minmax(plan.edges[ei].a, plan.edges[ei].b) == std::minmax(a, b))
                            is_bnd = true;
                    if (!is_bnd)
                        continue;
                    const Vector d = (plan.nodes[b] - plan.nodes[a]).normalized();
                    const Vector n_in = left_2d(d);
                    double col_ext = 0.0;
                    for (const Point& cp : cf.raw_uv)
                        col_ext = std::max(col_ext, (cp - plan.nodes[v]).dot(-n_in));
                    add_unique_plane(
                        perim_limits,
                        lift_plane_uv(plan, plan.nodes[v] - n_in * col_ext, n_in)
                    );
                }
            }
        }

        const Polyline bot_uv = close_ring(cf.raw_uv);
        const std::vector<double> flares(cf.raw_uv.size(), framing.head_reach);
        const Polyline top_uv = bot_uv.offset_sides(flares);

        auto head = std::make_shared<wood_session::NodeBlockElement>(
            std::vector<Polyline>{
                plan.lift_polyline(bot_uv, w_col_top),
                plan.lift_polyline(top_uv, cf.seat_w),
            },
            "head"
        );
        head->cuts = perim_limits;
        head->invalidate_geometry();
        out.head = head;
    }

    return out;
}

std::vector<std::vector<Polyline>> all_regions_with_holes(std::vector<Polyline> rings) {
    std::vector<Polyline> outers;
    std::vector<Polyline> holes;
    for (Polyline& r : rings) {
        const double a = area_2d(loop_points(r));
        if (a > TOL * TOL)
            outers.push_back(std::move(r));
        else if (a < -TOL * TOL)
            holes.push_back(std::move(r));
    }
    std::stable_sort(outers.begin(), outers.end(), [](const Polyline& a, const Polyline& b) {
        return std::abs(area_2d(loop_points(a))) < std::abs(area_2d(loop_points(b)));
    });
    std::vector<std::vector<Polyline>> groups(outers.size());
    for (size_t i = 0; i < outers.size(); ++i)
        groups[i].push_back(outers[i]);
    for (const Polyline& h : holes) {
        const std::vector<Point> pts = loop_points(h);
        if (pts.empty())
            continue;
        for (size_t i = 0; i < outers.size(); ++i) {
            if (outers[i].point_in_polygon_2d(pts[0])) {
                groups[i].push_back(h);
                break;
            }
        }
    }
    return groups;
}

std::vector<Polyline> largest_region_with_holes(std::vector<Polyline> rings) {
    auto groups = all_regions_with_holes(std::move(rings));
    if (groups.empty())
        return {};
    return std::move(groups.back());
}

std::vector<std::shared_ptr<Element>> build_decks(
    const Plan& plan,
    const Framing& framing,
    const std::vector<EdgeSpec>& edge_specs,
    const std::map<size_t, ColumnFootprint>& columns,
    const std::vector<Polyline>& outer_cores,
    const std::vector<Polyline>& rising_column_notches_uv
) {
    std::vector<std::shared_ptr<Element>> decks;
    if (framing.deck <= TOL)
        return decks;

    const double w0 = framing.deck_flush ? -framing.deck : 0.0;
    const double w1 = framing.deck_flush ? 0.0 : framing.deck;

    std::map<std::pair<size_t, size_t>, size_t> edge_lookup;
    for (size_t ei = 0; ei < plan.edges.size(); ++ei)
        edge_lookup[std::minmax(plan.edges[ei].a, plan.edges[ei].b)] = ei;

    // Determine CLT strip split axis (along primary span so strips span perpendicular to girders/purlins).
    Vector strip_along(1.0, 0.0, 0.0);
    const int primary_fam = std::max(0, framing.span);
    for (const PlanEdge& e : plan.edges) {
        if (e.family == primary_fam) {
            strip_along = (plan.nodes[e.b] - plan.nodes[e.a]).normalized();
            break;
        }
    }
    const Vector strip_across = left_2d(strip_along);

    std::vector<Polyline> global_cutters = outer_cores;
    for (const Polyline& notch : rising_column_notches_uv)
        global_cutters.push_back(notch);
    if (framing.deck_flush) {
        for (const auto& [_, cf] : columns) {
            if (!cf.inline_head_uv.empty())
                global_cutters.push_back(close_ring(cf.inline_head_uv));
            else if (framing.node == NodeMode::Through && !cf.raw_uv.empty())
                global_cutters.push_back(close_ring(cf.raw_uv));
        }
    }

    for (const PlanCell& cell : plan.cells) {
        if (cell.outer.size() < 3)
            continue;

        std::vector<Point> outer_pts;
        std::vector<double> offsets;
        for (size_t i = 0; i < cell.outer.size(); ++i) {
            const size_t u = cell.outer[i];
            const size_t v = cell.outer[(i + 1) % cell.outer.size()];
            outer_pts.push_back(plan.nodes[u]);
            double off = 0.0;
            if (auto it = edge_lookup.find(std::minmax(u, v)); it != edge_lookup.end()) {
                const EdgeSpec& es = edge_specs[it->second];
                const PlanEdge& pe = plan.edges[it->second];
                if (framing.deck_flush) {
                    off = -0.5 * es.width;
                } else if (pe.boundary) {
                    if (es.role != Role::None)
                        off = 0.5 * es.width;
                    else if (!framing.profiles.column.empty())
                        off = 0.5 * wood_session::compute_size(framing.profiles.column).first;
                }
            }
            offsets.push_back(off);
        }

        const Polyline base_outer = close_ring(outer_pts).offset_sides(offsets);
        std::vector<Polyline> cutters = global_cutters;
        for (const auto& h : cell.holes) {
            std::vector<Point> hp;
            for (size_t vi : h)
                hp.push_back(plan.nodes[vi]);
            if (area_2d(hp) < 0.0)
                std::reverse(hp.begin(), hp.end());
            cutters.push_back(close_ring(hp));
        }

        std::vector<Polyline> cell_loops = {base_outer};
        if (!cutters.empty()) {
            cell_loops = largest_region_with_holes(
                BooleanPolyline::compute_regions({base_outer}, cutters, 2)
            );
        }
        if (cell_loops.empty())
            continue;
        global_cutters.push_back(cell_loops[0]);

        // Split into CLT transport panels when `panel_width > 0`.
        std::vector<std::vector<Polyline>> panel_sets;
        if (framing.panel_width <= TOL) {
            panel_sets.push_back(cell_loops);
        } else {
            double lo = std::numeric_limits<double>::max();
            double hi = -lo;
            for (const Point& p : loop_points(cell_loops[0])) {
                const double s = Vector(p[0], p[1], 0.0).dot(strip_across);
                lo = std::min(lo, s);
                hi = std::max(hi, s);
            }
            const int n_strips =
                std::max(1, static_cast<int>(compute_bays(hi - lo, framing.panel_width).size()));
            const double sw = (hi - lo) / n_strips;
            for (int k = 0; k < n_strips; ++k) {
                const Point c_uv(
                    strip_across[0] * (lo + (k + 0.5) * sw),
                    strip_across[1] * (lo + (k + 0.5) * sw),
                    0.0
                );
                const Polyline band = close_ring({
                    c_uv - strip_along * 1e6 - strip_across * (0.5 * sw),
                    c_uv + strip_along * 1e6 - strip_across * (0.5 * sw),
                    c_uv + strip_along * 1e6 + strip_across * (0.5 * sw),
                    c_uv - strip_along * 1e6 + strip_across * (0.5 * sw),
                });
                for (std::vector<Polyline>& piece : all_regions_with_holes(
                         BooleanPolyline::compute_regions(cell_loops, {band}, 0)
                     )) {
                    panel_sets.push_back(std::move(piece));
                }
            }
        }

        for (const std::vector<Polyline>& loops : panel_sets) {
            const Polyline bot = plan.lift_polyline(loops[0], w0);
            const Polyline top = plan.lift_polyline(loops[0], w1);
            auto plate = std::make_shared<wood_session::FacePlanarElement>(bot, top, "deck");
            for (const Polyline& r : loops) {
                plate->features.bottom.push_back(plan.lift_polyline(r, w0));
                plate->features.top.push_back(plan.lift_polyline(r, w1));
            }
            plate->invalidate_geometry();
            decks.push_back(plate);
        }
    }

    return decks;
}

std::vector<std::shared_ptr<Element>> build_core_walls(const Plan& plan, const Framing& framing) {
    std::vector<std::shared_ptr<Element>> walls;
    if (framing.wall <= TOL || framing.post_height <= TOL || plan.cores.empty())
        return walls;

    const double hw = 0.5 * framing.wall;
    const double w_bot = -framing.post_height;
    const double w_top = (!framing.deck_flush && framing.deck > TOL) ? framing.deck : 0.0;

    for (const Polyline& ring : plan.cores) {
        std::vector<Point> corners = loop_points(ring);
        if (corners.size() < 3)
            continue;
        if (area_2d(corners) < 0.0)
            std::reverse(corners.begin(), corners.end());

        // Pinwheel butt joints around core corners: each wall runs from inner corner at start to outer corner at end!
        for (size_t i = 0; i < corners.size(); ++i) {
            const Point& a = corners[i];
            const Point& b = corners[(i + 1) % corners.size()];
            const Vector dir = (b - a).normalized();
            const Vector n_out = right_2d(dir);
            const Point start_uv = a + dir * hw;
            const Point end_uv = b + dir * hw;

            const Polyline inner_face({
                plan.to_world(start_uv[0] - n_out[0] * hw, start_uv[1] - n_out[1] * hw, w_bot),
                plan.to_world(end_uv[0] - n_out[0] * hw, end_uv[1] - n_out[1] * hw, w_bot),
                plan.to_world(end_uv[0] - n_out[0] * hw, end_uv[1] - n_out[1] * hw, w_top),
                plan.to_world(start_uv[0] - n_out[0] * hw, start_uv[1] - n_out[1] * hw, w_top),
                plan.to_world(start_uv[0] - n_out[0] * hw, start_uv[1] - n_out[1] * hw, w_bot),
            });
            const Polyline outer_face({
                plan.to_world(start_uv[0] + n_out[0] * hw, start_uv[1] + n_out[1] * hw, w_bot),
                plan.to_world(end_uv[0] + n_out[0] * hw, end_uv[1] + n_out[1] * hw, w_bot),
                plan.to_world(end_uv[0] + n_out[0] * hw, end_uv[1] + n_out[1] * hw, w_top),
                plan.to_world(start_uv[0] + n_out[0] * hw, start_uv[1] + n_out[1] * hw, w_top),
                plan.to_world(start_uv[0] + n_out[0] * hw, start_uv[1] + n_out[1] * hw, w_bot),
            });
            walls.push_back(
                std::make_shared<wood_session::FacePlanarElement>(inner_face, outer_face, "wall")
            );
        }
    }
    return walls;
}

// ═══════════════════════════════════════════════════════════════════════════
// Populate One Planar Unit into a Session
// ═══════════════════════════════════════════════════════════════════════════

void populate_plan_unit(
    wood_session::WoodSession& target,
    const std::shared_ptr<TreeNode>& parent_group,
    const Plan& plan,
    const Framing& framing,
    const std::string& suffix,
    const std::vector<Polyline>& rising_column_notches_uv = {},
    bool emit_plan_wireframe = true
) {
    auto group_name = [&](const std::string& base) {
        return suffix.empty() ? base : fmt::format("{}_{}", base, suffix);
    };

    // 1. Clean wireframe plan polylines at base/centreline (zero mesh triangulation diagonals!).
    if (emit_plan_wireframe) {
        const double plan_w =
            framing.post_height > TOL
                ? -framing.post_height
                : -0.5 * wood_session::compute_size(framing.profiles.girder).second;
        auto plan_grp = target.add_group(group_name("plan"), parent_group);
        for (const PlanEdge& e : plan.edges) {
            target.add_polyline(
                Polyline({
                    plan.to_world(plan.nodes[e.a][0], plan.nodes[e.a][1], plan_w),
                    plan.to_world(plan.nodes[e.b][0], plan.nodes[e.b][1], plan_w),
                }),
                plan_grp
            );
        }
    }

    // 2. Classify edges & standing column footprints.
    std::vector<EdgeSpec> edge_specs(plan.edges.size());
    for (size_t ei = 0; ei < plan.edges.size(); ++ei)
        edge_specs[ei] = classify_edge(plan.edges[ei], framing);

    const std::map<size_t, ColumnFootprint> columns =
        compute_column_footprints(plan, framing, edge_specs);

    std::vector<std::shared_ptr<Element>> col_elems;
    std::vector<std::shared_ptr<Element>> head_elems;
    for (const auto& [v, cf] : columns) {
        BuiltSupport sup = build_column_and_head(plan, framing, v, cf);
        if (sup.column)
            col_elems.push_back(sup.column);
        if (sup.head)
            head_elems.push_back(sup.head);
    }
    if (!col_elems.empty()) {
        auto grp = target.add_group(group_name("columns"), parent_group);
        for (const auto& el : col_elems)
            target.add_element(el, grp);
    }
    if (!head_elems.empty()) {
        auto grp = target.add_group(group_name("heads"), parent_group);
        for (const auto& el : head_elems)
            target.add_element(el, grp);
    }

    // 3. Core shear walls.
    const std::vector<Polyline> outer_cores = expanded_cores_uv(plan, framing.wall);
    const std::vector<std::shared_ptr<Element>> wall_elems = build_core_walls(plan, framing);
    if (!wall_elems.empty()) {
        auto grp = target.add_group(group_name("walls"), parent_group);
        for (const auto& el : wall_elems)
            target.add_element(el, grp);
    }

    // 4. Primary girders, secondary beams, and subdivided purlins.
    std::vector<std::vector<size_t>> node_adj(plan.nodes.size());
    for (size_t ei = 0; ei < plan.edges.size(); ++ei) {
        node_adj[plan.edges[ei].a].push_back(ei);
        node_adj[plan.edges[ei].b].push_back(ei);
    }

    std::vector<std::shared_ptr<Element>> girder_elems;
    std::vector<std::shared_ptr<Element>> beam_elems;
    std::vector<std::shared_ptr<Element>> purlin_elems;

    for (size_t ei = 0; ei < plan.edges.size(); ++ei) {
        const EdgeSpec& es = edge_specs[ei];
        if (es.role == Role::None)
            continue;
        const PlanEdge& e = plan.edges[ei];
        const Vector dir = (plan.nodes[e.b] - plan.nodes[e.a]).normalized();

        EndTrim trim_a = compute_node_trim(plan, framing, edge_specs, columns, node_adj, e.a, ei);
        EndTrim trim_b = compute_node_trim(plan, framing, edge_specs, columns, node_adj, e.b, ei);

        // In `Stacked` purlin mode, extend terminal upper-layer purlins across the full top width of girders!
        if (framing.system == System::GirderPurlin && framing.purlin == PurlinMode::Stacked &&
            es.role == Role::Purlin && framing.node != NodeMode::Through) {
            const double gw_half = 0.5 * wood_session::compute_size(framing.profiles.girder).first;
            auto is_terminal_over_girder = [&](size_t nv) {
                bool has_girder = false;
                for (size_t adj_ei : node_adj[nv]) {
                    if (adj_ei == ei)
                        continue;
                    if (edge_specs[adj_ei].role == Role::Girder)
                        has_girder = true;
                    if (edge_specs[adj_ei].role == Role::Purlin)
                        return false;
                }
                return has_girder;
            };
            if (trim_a.overrun < gw_half && is_terminal_over_girder(e.a)) {
                trim_a.overrun = gw_half;
                trim_a.planes = {lift_plane_uv(plan, plan.nodes[e.a] - dir * gw_half, dir)};
            }
            if (trim_b.overrun < gw_half && is_terminal_over_girder(e.b)) {
                trim_b.overrun = gw_half;
                trim_b.planes = {lift_plane_uv(plan, plan.nodes[e.b] + dir * gw_half, -dir)};
            }
        }

        const Point ext_a = plan.nodes[e.a] - dir * trim_a.overrun;
        const Point ext_b = plan.nodes[e.b] + dir * trim_b.overrun;
        const double w_mid = 0.5 * (es.w_bot + es.w_top);
        const std::string elem_name =
            es.role == Role::Girder ? "girder" : (es.role == Role::Beam ? "beam" : "purlin");

        for (const auto& [sub_line, core_cuts] :
             split_outside_cores_uv(plan, Line::from_points(ext_a, ext_b), outer_cores)) {
            std::vector<Plane> cuts = trim_a.planes;
            for (const Plane& p : trim_b.planes)
                add_unique_plane(cuts, p);
            for (const Plane& p : core_cuts)
                add_unique_plane(cuts, p);

            if (auto elem = make_plane_member(
                    plan, sub_line.start(), sub_line.end(), w_mid, es.profile, cuts, elem_name
                )) {
                if (es.role == Role::Girder)
                    girder_elems.push_back(elem);
                else if (es.role == Role::Beam)
                    beam_elems.push_back(elem);
                else
                    purlin_elems.push_back(elem);
            }
        }
    }

    for (const auto& p : build_bay_purlins(plan, framing, edge_specs, outer_cores))
        purlin_elems.push_back(p);

    if (framing.system == System::PostAndBeam) {
        std::vector<std::shared_ptr<Element>> all_beams = girder_elems;
        all_beams.insert(all_beams.end(), beam_elems.begin(), beam_elems.end());
        if (!all_beams.empty()) {
            auto grp = target.add_group(group_name("beams"), parent_group);
            for (const auto& el : all_beams)
                target.add_element(el, grp);
        }
    } else {
        std::vector<std::shared_ptr<Element>> all_girders = girder_elems;
        all_girders.insert(all_girders.end(), beam_elems.begin(), beam_elems.end());
        if (!all_girders.empty()) {
            auto grp = target.add_group(group_name("girders"), parent_group);
            for (const auto& el : all_girders)
                target.add_element(el, grp);
        }
        if (!purlin_elems.empty()) {
            auto grp = target.add_group(group_name("purlins"), parent_group);
            for (const auto& el : purlin_elems)
                target.add_element(el, grp);
        }
    }

    // 5. In-plane timber braces.
    const std::vector<std::shared_ptr<Element>> brace_elems =
        build_bay_braces(plan, framing, edge_specs);
    if (!brace_elems.empty()) {
        auto grp = target.add_group(group_name("braces"), parent_group);
        for (const auto& el : brace_elems)
            target.add_element(el, grp);
    }

    // 6. CLT decks / flush infill panels.
    const std::vector<std::shared_ptr<Element>> deck_elems =
        build_decks(plan, framing, edge_specs, columns, outer_cores, rising_column_notches_uv);
    if (!deck_elems.empty()) {
        auto grp = target.add_group(group_name("decks"), parent_group);
        for (const auto& el : deck_elems)
            target.add_element(el, grp);
    }
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Grid Constructors
// ═══════════════════════════════════════════════════════════════════════════

Grid::Grid(const Plan& plan, const Framing& framing, std::string name)
    : WoodSession(std::move(name)) {
    populate_plan_unit(*this, nullptr, plan, framing, "");
    compute_face_contacts();
}

Grid::Grid(const Stack& stack, std::string name) : WoodSession(std::move(name)) {
    for (size_t i = 0; i < stack.levels.size(); ++i) {
        const LevelSpec& lvl = stack.levels[i];
        Plan lvl_plan = lvl.plan;
        Framing lvl_framing = lvl.framing;

        // When the storey below has a top-mounted CLT deck (`!deck_flush && deck > 0`),
        // either notch that deck around continuous columns (`Through`) or stand upper columns on the deck!
        std::vector<Polyline> upper_col_notches_uv;
        if (i > 0) {
            const LevelSpec& below = stack.levels[i - 1];
            if (below.framing.deck > TOL && !below.framing.deck_flush &&
                lvl.framing.node != NodeMode::Through) {
                lvl_framing.post_height = std::max(0.0, lvl_framing.post_height - below.framing.deck);
                for (size_t v = 0; v < lvl_plan.column_feet.size(); ++v) {
                    if (!lvl_plan.column_feet[v].has_value())
                        continue;
                    const Point top_d =
                        lvl_plan.to_world(lvl_plan.nodes[v][0], lvl_plan.nodes[v][1], 0.0);
                    const Point foot_d = *lvl_plan.column_feet[v];
                    const double dz = (top_d - foot_d).dot(lvl_plan.frame.z_axis());
                    if (dz > TOL)
                        lvl_plan.column_feet[v] = foot_d + (top_d - foot_d) * (below.framing.deck / dz);
                }
            }
        }
        if (i + 1 < stack.levels.size() && lvl.framing.deck > TOL && !lvl.framing.deck_flush) {
            const LevelSpec& above = stack.levels[i + 1];
            if (above.framing.node == NodeMode::Through && above.framing.post_height > TOL) {
                std::vector<EdgeSpec> above_specs(above.plan.edges.size());
                for (size_t ei = 0; ei < above.plan.edges.size(); ++ei)
                    above_specs[ei] = classify_edge(above.plan.edges[ei], above.framing);
                for (const auto& [av, cf] :
                     compute_column_footprints(above.plan, above.framing, above_specs)) {
                    const Point top_d =
                        above.plan.to_world(above.plan.nodes[av][0], above.plan.nodes[av][1], 0.0);
                    const bool has_foot =
                        av < above.plan.column_feet.size() && above.plan.column_feet[av].has_value();
                    const Point foot_d =
                        has_foot ? *above.plan.column_feet[av] : top_d;
                    const double dz = std::max(TOL, (top_d - foot_d).dot(lvl.plan.frame.z_axis()));
                    const Vector shift_bot = foot_d - top_d;
                    const Vector shift_top =
                        shift_bot + (top_d - foot_d) * (lvl.framing.deck / dz);

                    std::vector<Point> swept_pts;
                    for (const Point& p : cf.raw_uv) {
                        const Point wp = above.plan.to_world(p[0], p[1], 0.0);
                        const Point lb = lvl.plan.to_local(wp + shift_bot);
                        const Point lt = lvl.plan.to_local(wp + shift_top);
                        swept_pts.emplace_back(lb[0], lb[1], 0.0);
                        if (has_foot)
                            swept_pts.emplace_back(lt[0], lt[1], 0.0);
                    }
                    const std::vector<Point> notch_hull =
                        has_foot ? convex_hull_2d(std::move(swept_pts)) : std::move(swept_pts);
                    if (notch_hull.size() >= 3)
                        upper_col_notches_uv.push_back(close_ring(notch_hull));
                }
            }
        }

        const std::string tag = lvl.name.empty() ? fmt::format("{}", i + 1) : lvl.name;
        auto lvl_grp = add_group(tag);
        populate_plan_unit(
            *this, lvl_grp, lvl_plan, lvl_framing, std::to_string(i + 1), upper_col_notches_uv, i == 0
        );
    }

    for (size_t i = 0; i < stack.facades.size(); ++i) {
        const LevelSpec& fac = stack.facades[i];
        const std::string tag = fac.name.empty() ? fmt::format("facade_{}", i) : fac.name;
        auto fac_grp = add_group(tag);
        populate_plan_unit(*this, fac_grp, fac.plan, fac.framing, tag, {}, false);
    }

    compute_face_contacts();
}

} // namespace wood_grid
