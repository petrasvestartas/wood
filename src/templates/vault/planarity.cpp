#include "pch.h"
#include "src/templates/vault/planarity.h"
#include "intersection.h"
#include "line.h"
#include "plane.h"
#include "tolerance.h"

namespace wood_vault {

namespace {

// ═══════════════════════════════════════════════════════════════════════════
// Conjugate-Gradient Planarizer Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// Fitted plane frame, normal residuals, and orthonormalized affine basis of one polygon.
struct Fit {
    Vector normal; // Unit normal of the fitted PCA plane.
    std::vector<double> heights; // Signed heights orthogonal to affine functions.
    std::array<std::vector<double>, 3> affine; // Orthonormalized {1, x, y} corner basis.
};

/// Removes the affine components spanned by fit from values in place.
void remove_affine(const Fit& fit, std::vector<double>& values) {

    for (const std::vector<double>& function : fit.affine) {
        double proj = 0.0;
        for (size_t k = 0; k < values.size(); k++)
            proj += function[k] * values[k];

        for (size_t k = 0; k < values.size(); k++)
            values[k] -= proj * function[k];
    }
}

/// Builds the local plane fit and orthonormalized affine basis for one polygon.
Fit to_fit(const std::vector<Point>& points, const std::vector<size_t>& polygon) {

    std::vector<Point> corners;
    corners.reserve(polygon.size());
    for (const size_t corner : polygon)
        corners.push_back(points[corner]);

    const Plane plane = Plane::from_points_pca(corners);
    const Point centre = Point::centroid(corners);

    Fit fit{plane.z_axis(), {}, {}};
    for (const Point& corner : corners) {
        const Vector offset = corner - centre;
        fit.heights.push_back(offset.dot(plane.z_axis()));
        fit.affine[0].push_back(1.0);
        fit.affine[1].push_back(offset.dot(plane.x_axis()));
        fit.affine[2].push_back(offset.dot(plane.y_axis()));
    }

    for (size_t a = 0; a < 3; a++) {
        double before = 0.0;
        for (const double value : fit.affine[a])
            before += value * value;

        for (size_t b = 0; b < a; b++) {
            double proj = 0.0;
            for (size_t k = 0; k < corners.size(); k++)
                proj += fit.affine[a][k] * fit.affine[b][k];

            for (size_t k = 0; k < corners.size(); k++)
                fit.affine[a][k] -= proj * fit.affine[b][k];
        }

        double norm = 0.0;
        for (const double value : fit.affine[a])
            norm += value * value;

        const double scale = norm > 1e-12 * before ? 1.0 / std::sqrt(norm) : 0.0;
        for (double& value : fit.affine[a])
            value *= scale;
    }

    remove_affine(fit, fit.heights);

    return fit;
}

/// Builds the plane fits for every polygon.
std::vector<Fit> to_fits(
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& polygons
) {

    std::vector<Fit> fits;
    fits.reserve(polygons.size());
    for (const std::vector<size_t>& polygon : polygons)
        fits.push_back(to_fit(points, polygon));

    return fits;
}

/// Accumulates vertex displacements from Lagrange multipliers at polygon corners.
std::vector<Vector> to_moves(
    const std::vector<Fit>& fits,
    const std::vector<std::vector<size_t>>& polygons,
    const std::vector<std::vector<double>>& multipliers,
    size_t count,
    const std::vector<bool>& pinned,
    const std::vector<Vector>& tangent_normals
) {

    std::vector<Vector> moves(count, Vector(0.0, 0.0, 0.0));
    for (size_t f = 0; f < polygons.size(); f++) {
        for (size_t k = 0; k < polygons[f].size(); k++) {
            const size_t idx = polygons[f][k];
            if (idx < pinned.size() && pinned[idx])
                continue;

            moves[idx] = moves[idx] + fits[f].normal * multipliers[f][k];
        }
    }

    if (tangent_normals.size() == count) {
        for (size_t i = 0; i < count; i++) {
            const Vector& n = tangent_normals[i];
            moves[i] = moves[i] - n * moves[i].dot(n);
        }
    }

    return moves;
}

/// Evaluates linearized non-planar height residuals induced by vertex moves.
std::vector<std::vector<double>> to_heights(
    const std::vector<Fit>& fits,
    const std::vector<std::vector<size_t>>& polygons,
    const std::vector<Vector>& moves
) {

    std::vector<std::vector<double>> heights(polygons.size());
    for (size_t f = 0; f < polygons.size(); f++) {
        for (const size_t corner : polygons[f])
            heights[f].push_back(fits[f].normal.dot(moves[corner]));

        remove_affine(fits[f], heights[f]);
    }

    return heights;
}

/// Returns the inner product of two corner-value tables.
double dot_values(
    const std::vector<std::vector<double>>& a,
    const std::vector<std::vector<double>>& b
) {

    double sum = 0.0;
    for (size_t f = 0; f < a.size(); f++)
        for (size_t k = 0; k < a[f].size(); k++)
            sum += a[f][k] * b[f][k];

    return sum;
}

/// Adds scale * b into a in place.
void add_scaled(
    std::vector<std::vector<double>>& a,
    const std::vector<std::vector<double>>& b,
    double scale
) {

    for (size_t f = 0; f < a.size(); f++)
        for (size_t k = 0; k < a[f].size(); k++)
            a[f][k] += scale * b[f][k];
}

/// Returns the maximum absolute residual height across all polygon fits.
double largest_height(const std::vector<Fit>& fits) {

    double worst = 0.0;
    for (const Fit& fit : fits)
        for (const double value : fit.heights)
            worst = std::max(worst, std::abs(value));

    return worst;
}

const int NEWTON_STEPS = 80;
const int GRADIENT_STEPS = 600;

// ═══════════════════════════════════════════════════════════════════════════
// Extrusion & Projection Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// Extracts the unit vertex normal stored in vertex attributes or falls back to area-weighted face normals.
std::vector<Vector> vertex_normals_of(
    const Mesh& mesh,
    const std::vector<Point>& vertices,
    const std::vector<std::vector<size_t>>& faces,
    const std::vector<size_t>& keys
) {

    std::vector<Vector> normals(vertices.size(), Vector(0.0, 0.0, 0.0));
    for (size_t f = 0; f < faces.size(); f++) {
        const std::vector<size_t>& loop = faces[f];
        if (loop.size() < 3)
            continue;

        Point centre(0.0, 0.0, 0.0);
        for (const size_t idx : loop)
            centre = centre + Vector(vertices[idx][0], vertices[idx][1], vertices[idx][2]);
        centre = Point(
            centre[0] / static_cast<double>(loop.size()),
            centre[1] / static_cast<double>(loop.size()),
            centre[2] / static_cast<double>(loop.size())
        );

        Vector area_normal(0.0, 0.0, 0.0);
        for (size_t k = 0; k < loop.size(); k++) {
            const Vector u = vertices[loop[k]] - centre;
            const Vector w = vertices[loop[(k + 1) % loop.size()]] - centre;
            area_normal = area_normal + u.cross(w);
        }
        for (const size_t idx : loop)
            normals[idx] = normals[idx] + area_normal;
    }

    for (size_t i = 0; i < vertices.size(); i++) {
        const std::optional<std::array<double, 3>> attr = mesh.vertex.at(keys[i]).normal();
        if (attr) {
            Vector stored((*attr)[0], (*attr)[1], (*attr)[2]);
            if (stored.normalize_self()) {
                if (normals[i].magnitude_squared() > 1e-12 && stored.dot(normals[i]) < 0.0)
                    stored = -stored;
                normals[i] = stored;
                continue;
            }
        }
        if (!normals[i].normalize_self())
            normals[i] = Vector(0.0, 0.0, 1.0);
    }

    return normals;
}

/// Returns the fitted plane of corners oriented so its z-axis agrees with reference_up.
Plane oriented_plane(const std::vector<Point>& corners, const Vector& reference_up) {

    const Plane raw = Plane::from_points_pca(corners);
    const Point centre = Point::centroid(corners);
    if (raw.z_axis().dot(reference_up) >= 0.0)
        return Plane(centre, raw.x_axis(), raw.y_axis());

    return Plane(centre, raw.y_axis(), raw.x_axis());
}

/// Projects the point along the ruling line through bot_pt and top_pt onto target_plane, clamped against grazing hits.
Point project_along_ruling(
    const Point& bot_pt,
    const Point& top_pt,
    const Plane& target_plane,
    double base_param,
    double flatten_caps,
    bool& clean_hit
) {

    const Line ruling = Line::from_points(bot_pt, top_pt);
    const std::optional<Point> hit = Intersection::line_plane(ruling, target_plane, false);
    const Point raw = base_param < 0.5 ? bot_pt : top_pt;
    if (!hit) {
        clean_hit = false;
        return raw;
    }

    const double span = std::max(1e-6, bot_pt.distance(top_pt));
    if (hit->distance(raw) > 4.0 * span) {
        clean_hit = false;
        return raw;
    }

    if (flatten_caps >= 1.0 - 1e-9)
        return *hit;

    clean_hit = false;

    return raw + (*hit - raw) * std::clamp(flatten_caps, 0.0, 1.0);
}

/// Rotates vec around unit_axis by angle_radians (Rodrigues formula).
Vector rotate_vector(const Vector& vec, const Vector& unit_axis, double angle_radians) {

    const double c = std::cos(angle_radians);
    const double s = std::sin(angle_radians);

    return vec * c + unit_axis.cross(vec) * s + unit_axis * (unit_axis.dot(vec) * (1.0 - c));
}

/// Builds a closed NodeBlockElement from matched intrados and extrados rings, splitting non-planar caps cleanly.
std::shared_ptr<wood_session::NodeBlockElement> build_stone_element(
    std::vector<Point> intrados,
    std::vector<Point> extrados,
    bool planar_cap,
    bool trim_base,
    double base_z,
    const std::string& name
) {

    if (intrados.size() < 3 || intrados.size() != extrados.size())
        return nullptr;

    const Point centre = Point::centroid(intrados);
    Vector normal(0.0, 0.0, 0.0);
    for (size_t i = 0; i < intrados.size(); i++) {
        const Vector a = intrados[i] - centre;
        const Vector b = intrados[(i + 1) % intrados.size()] - centre;
        normal = normal + a.cross(b);
    }
    if (normal.dot(Point::centroid(extrados) - centre) < 0.0) {
        std::reverse(intrados.begin(), intrados.end());
        std::reverse(extrados.begin(), extrados.end());
    }

    bool has_reflex_or_collinear = false;
    if (planar_cap && intrados.size() > 3) {
        const Vector cap_dir = (Point::centroid(extrados) - centre).normalized();
        const size_t m = intrados.size();
        for (size_t i = 0; i < m; i++) {
            const Vector e_prev = intrados[i] - intrados[(i + m - 1) % m];
            const Vector e_next = intrados[(i + 1) % m] - intrados[i];
            const double denom = e_prev.magnitude() * e_next.magnitude();
            if (denom <= 1e-8 || e_prev.cross(e_next).dot(cap_dir) <= 0.05 * denom) {
                has_reflex_or_collinear = true;
                break;
            }
        }
    }

    std::vector<std::vector<Point>> faces;
    if ((planar_cap && !has_reflex_or_collinear) || intrados.size() == 3) {
        faces.emplace_back(intrados.rbegin(), intrados.rend());
        faces.push_back(extrados);
    } else {
        const Point bot_mid = Point::centroid(intrados);
        const Point top_mid = Point::centroid(extrados);
        for (size_t i = 0; i < intrados.size(); i++) {
            const size_t j = (i + 1) % intrados.size();
            faces.push_back({bot_mid, intrados[j], intrados[i]});
            faces.push_back({top_mid, extrados[i], extrados[j]});
        }
    }

    for (size_t i = 0; i < intrados.size(); i++) {
        const size_t j = (i + 1) % intrados.size();
        faces.push_back({intrados[i], intrados[j], extrados[j], extrados[i]});
    }

    std::vector<std::vector<Point>> kept;
    for (const std::vector<Point>& face : faces) {
        std::vector<Point> corners;
        for (size_t i = 0; i < face.size(); i++)
            if (face[i].distance(face[(i + 1) % face.size()]) > 1e-5)
                corners.push_back(face[i]);

        if (corners.size() >= 3)
            kept.push_back(corners);
    }
    if (kept.size() < 4)
        return nullptr;

    Mesh solid = Mesh::from_polylines(kept, 1e-4);
    if (planar_cap && !has_reflex_or_collinear && solid.number_of_vertices() != 2 * intrados.size())
        return nullptr;
    solid.orient_outward();
    std::shared_ptr<wood_session::NodeBlockElement> block =
        std::make_shared<wood_session::NodeBlockElement>(solid, name);

    if (trim_base) {
        double min_z = 1e30;
        double max_z = -1e30;
        for (const Point& pt : intrados) {
            min_z = std::min(min_z, pt[2]);
            max_z = std::max(max_z, pt[2]);
        }
        for (const Point& pt : extrados) {
            min_z = std::min(min_z, pt[2]);
            max_z = std::max(max_z, pt[2]);
        }
        if (max_z <= base_z + 2.0)
            return nullptr;

        if (min_z < base_z - 0.5) {
            const double whole = std::abs(block->element_geometry_mesh().volume());
            block->cuts = {Plane(Point(0.0, 0.0, base_z), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0))};
            block->invalidate_geometry();
            if (std::abs(block->model_geometry_mesh().volume()) <= 1e-3 * std::max(1.0, whole))
                return nullptr;
        }
    }

    return block;
}

/// Assigns consistent alternating +1/-1 bevel signs to undirected edges via BFS over even-sided cells.
std::map<std::pair<size_t, size_t>, double> assign_bevel_signs(
    const std::vector<std::vector<size_t>>& faces
) {

    std::map<std::pair<size_t, size_t>, std::vector<std::pair<size_t, size_t>>> edge_uses;
    for (size_t f = 0; f < faces.size(); f++) {
        const size_t m = faces[f].size();
        for (size_t k = 0; k < m; k++) {
            const size_t a = faces[f][k];
            const size_t b = faces[f][(k + 1) % m];
            const std::pair<size_t, size_t> key = a < b ? std::make_pair(a, b) : std::make_pair(b, a);
            edge_uses[key].emplace_back(f, k);
        }
    }

    // Canonical direction of key = (min, max): +1 means plane tilts outward on the (min->max) left face at top.
    std::map<std::pair<size_t, size_t>, double> signs;
    std::vector<bool> visited(faces.size(), false);
    for (size_t start = 0; start < faces.size(); start++) {
        if (visited[start])
            continue;

        std::vector<std::pair<size_t, int>> queue = {{start, 0}};
        visited[start] = true;
        size_t head = 0;
        while (head < queue.size()) {
            const size_t f = queue[head].first;
            const int phase = queue[head].second;
            head++;

            const size_t m = faces[f].size();
            for (size_t k = 0; k < m; k++) {
                const size_t a = faces[f][k];
                const size_t b = faces[f][(k + 1) % m];
                const std::pair<size_t, size_t> key = a < b ? std::make_pair(a, b) : std::make_pair(b, a);
                const double local_outward = ((static_cast<int>(k) + phase) % 2 == 0) ? 1.0 : -1.0;
                const double canonical = (a < b) ? local_outward : -local_outward;
                if (!signs.count(key))
                    signs[key] = canonical;

                for (const std::pair<size_t, size_t>& use : edge_uses[key]) {
                    if (visited[use.first])
                        continue;

                    const size_t ua = faces[use.first][use.second];
                    const size_t ub = faces[use.first][(use.second + 1) % faces[use.first].size()];
                    // Neighbor across shared edge sees the opposite outward sign (-local_outward).
                    const double desired_neighbor = (ua < ub) ? signs[key] : -signs[key];
                    const int neighbor_phase =
                        ((static_cast<int>(use.second) % 2 == 0) == (desired_neighbor > 0.0)) ? 0 : 1;
                    visited[use.first] = true;
                    queue.emplace_back(use.first, neighbor_phase);
                }
            }
        }
    }

    return signs;
}

/// Builds interlocking bevelled blocks by intersecting tilted shared edge planes with planar top and bottom caps.
std::vector<std::shared_ptr<wood_session::NodeBlockElement>> build_bevelled_solids(
    const std::vector<Point>& vertices,
    const std::vector<Vector>& normals,
    const std::vector<std::vector<size_t>>& faces,
    double thickness,
    double bevel_degrees,
    bool trim_base,
    double base_z,
    NgonSolidMetrics* metrics
) {

    const double bevel = bevel_degrees * Tolerance::TO_RADIANS;
    const std::map<std::pair<size_t, size_t>, double> signs = assign_bevel_signs(faces);

    std::map<std::pair<size_t, size_t>, Plane> edge_planes;
    for (const std::pair<const std::pair<size_t, size_t>, double>& entry : signs) {
        const size_t a = entry.first.first;
        const size_t b = entry.first.second;
        const Point mid = Point::mid_point(vertices[a], vertices[b]);
        Vector axis = vertices[b] - vertices[a];
        if (!axis.normalize_self())
            continue;

        Vector avg_up = normals[a] + normals[b];
        avg_up = avg_up - axis * avg_up.dot(axis);
        if (!avg_up.normalize_self())
            avg_up = Vector(0.0, 0.0, 1.0);

        const Vector tilted_up = rotate_vector(avg_up, axis, entry.second * bevel);
        const Vector side_normal = axis.cross(tilted_up).normalized();
        const Vector y_in_plane = side_normal.cross(axis).normalized();
        edge_planes[entry.first] = Plane(mid, axis, y_in_plane);
    }

    std::vector<std::shared_ptr<wood_session::NodeBlockElement>> blocks;
    std::vector<Point> metric_pts;
    std::vector<std::vector<size_t>> side_polys;
    std::vector<std::vector<size_t>> cap_polys;

    for (size_t f = 0; f < faces.size(); f++) {
        const std::vector<size_t>& loop = faces[f];
        if (loop.size() < 3)
            continue;

        std::vector<Point> raw_corners;
        Vector up_ref(0.0, 0.0, 0.0);
        for (const size_t idx : loop) {
            raw_corners.push_back(vertices[idx]);
            up_ref = up_ref + normals[idx];
        }
        const Plane mid_plane = oriented_plane(raw_corners, up_ref);
        const Vector z_up = mid_plane.z_axis();
        const Plane bot_plane(
            mid_plane.origin() - z_up * (0.5 * thickness), mid_plane.x_axis(), mid_plane.y_axis()
        );
        const Plane top_plane(
            mid_plane.origin() + z_up * (0.5 * thickness), mid_plane.x_axis(), mid_plane.y_axis()
        );

        std::vector<Plane> sides;
        for (size_t k = 0; k < loop.size(); k++) {
            const size_t a = loop[k];
            const size_t b = loop[(k + 1) % loop.size()];
            const std::pair<size_t, size_t> key = a < b ? std::make_pair(a, b) : std::make_pair(b, a);
            sides.push_back(edge_planes.at(key));
        }

        std::vector<Point> intrados;
        std::vector<Point> extrados;
        bool valid = true;
        for (size_t k = 0; k < loop.size(); k++) {
            const Plane& prev_side = sides[(k + loop.size() - 1) % loop.size()];
            const Plane& curr_side = sides[k];
            const std::optional<Point> p_bot = Intersection::plane_plane_plane(prev_side, curr_side, bot_plane);
            const std::optional<Point> p_top = Intersection::plane_plane_plane(prev_side, curr_side, top_plane);
            if (!p_bot || !p_top || p_bot->distance(vertices[loop[k]]) > 4.0 * thickness ||
                p_top->distance(vertices[loop[k]]) > 4.0 * thickness) {
                valid = false;
                break;
            }
            intrados.push_back(*p_bot);
            extrados.push_back(*p_top);
        }
    for (size_t k = 0; k < loop.size(); k++) {
            const size_t nk = (k + 1) % loop.size();
            if ((extrados[nk] - extrados[k]).dot(intrados[nk] - intrados[k]) <= 1e-4) {
                valid = false;
                break;
            }
        }
        if (!valid)
            continue;

        const size_t base_idx = metric_pts.size();
        std::vector<size_t> bot_ring;
        std::vector<size_t> top_ring;
        for (size_t k = 0; k < intrados.size(); k++) {
            bot_ring.push_back(base_idx + k);
            metric_pts.push_back(intrados[k]);
        }
        for (size_t k = 0; k < extrados.size(); k++) {
            top_ring.push_back(base_idx + intrados.size() + k);
            metric_pts.push_back(extrados[k]);
        }
        cap_polys.push_back(bot_ring);
        cap_polys.push_back(top_ring);
        for (size_t k = 0; k < intrados.size(); k++) {
            const size_t nk = (k + 1) % intrados.size();
            side_polys.push_back({bot_ring[k], bot_ring[nk], top_ring[nk], top_ring[k]});
        }

        const std::shared_ptr<wood_session::NodeBlockElement> block = build_stone_element(
            intrados, extrados, true, trim_base, base_z, fmt::format("voussoir_{}", blocks.size())
        );
        if (block)
            blocks.push_back(block);
    }

    if (metrics) {
        metrics->side_planarity = side_polys.empty() ? 0.0 : planarity(metric_pts, side_polys);
        metrics->cap_planarity = cap_polys.empty() ? 0.0 : planarity(metric_pts, cap_polys);
    }

    return blocks;
}

/// Solves minimum-norm Gauss-Newton polygon planarization with optional per-vertex tangent-plane constraints.
std::vector<Point> planarized_impl(
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& polygons,
    double tolerance,
    const std::vector<bool>& pinned,
    const std::vector<Vector>& tangent_normals
) {

    std::vector<Point> moved = points;
    for (int step = 0; step < NEWTON_STEPS; step++) {
        const std::vector<Fit> fits = to_fits(moved, polygons);
        if (largest_height(fits) < tolerance)
            break;

        std::vector<std::vector<double>> residual;
        std::vector<std::vector<double>> multipliers;
        residual.reserve(fits.size());
        multipliers.reserve(fits.size());
        for (const Fit& fit : fits) {
            residual.push_back(fit.heights);
            multipliers.emplace_back(fit.heights.size(), 0.0);
        }

        std::vector<std::vector<double>> direction = residual;
        const double start = dot_values(residual, residual);
        double norm = start;

        for (int gradient = 0; gradient < GRADIENT_STEPS && norm > 1e-18 * start; gradient++) {
            const std::vector<Vector> moves =
                to_moves(fits, polygons, direction, moved.size(), pinned, tangent_normals);
            const std::vector<Vector>& step_moves = moves;
            const std::vector<std::vector<double>> image = to_heights(fits, polygons, step_moves);
            const double curvature = dot_values(direction, image);
            if (curvature <= 1e-30)
                break;

            const double alpha = norm / curvature;
            add_scaled(multipliers, direction, alpha);
            add_scaled(residual, image, -alpha);
            const double next = dot_values(residual, residual);
            std::vector<std::vector<double>> turned = residual;
            add_scaled(turned, direction, next / norm);
            direction = std::move(turned);
            norm = next;
        }

        const std::vector<Vector> moves =
            to_moves(fits, polygons, multipliers, moved.size(), pinned, tangent_normals);
        for (size_t v = 0; v < moved.size(); v++)
            moved[v] = moved[v] - moves[v];
    }

    return moved;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Public Planarization API
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Point> planarized(
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& polygons,
    double tolerance,
    const std::vector<bool>& pinned
) {
    return planarized_impl(points, polygons, tolerance, pinned, {});
}

double planarity(
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& polygons
) {
    return largest_height(to_fits(points, polygons));
}

// ═══════════════════════════════════════════════════════════════════════════
// Public NGon Solids Extrusion API
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::shared_ptr<wood_session::NodeBlockElement>> ngon_solids(
    const Mesh& mesh,
    double thickness,
    bool planar_sides,
    double flatten_caps,
    const Vector& facet_shift,
    double bevel_degrees,
    bool trim_base,
    double base_z,
    NgonSolidMetrics* metrics
) {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> vf =
        mesh.to_vertices_and_faces();
    const std::vector<Point>& vertices = vf.first;
    const std::vector<std::vector<size_t>>& faces = vf.second;
    const std::vector<size_t> keys = mesh.vertices();
    if (vertices.empty() || faces.empty())
        return {};

    const std::vector<Vector> normals = vertex_normals_of(mesh, vertices, faces, keys);

    if (std::abs(bevel_degrees) > 1e-6) {
        return build_bevelled_solids(
            vertices, normals, faces, thickness, bevel_degrees, trim_base, base_z, metrics
        );
    }

    const size_t v_cnt = vertices.size();
    std::vector<Point> offset_pts(2 * v_cnt);
    for (size_t i = 0; i < v_cnt; i++) {
        const Vector tangent_shift = facet_shift - normals[i] * facet_shift.dot(normals[i]);
        offset_pts[i] = vertices[i] - normals[i] * (0.5 * thickness) - tangent_shift * 0.5;
        offset_pts[v_cnt + i] =
            vertices[i] + normals[i] * (0.5 * thickness) + tangent_shift * 0.5;
    }

    // Collect every undirected mesh edge as a 4-vertex side quad between intrados and extrados.
    std::set<std::pair<size_t, size_t>> seen_edges;
    std::vector<std::vector<size_t>> side_quads;
    for (const std::vector<size_t>& loop : faces) {
        for (size_t k = 0; k < loop.size(); k++) {
            const size_t a = loop[k];
            const size_t b = loop[(k + 1) % loop.size()];
            const std::pair<size_t, size_t> edge = a < b ? std::make_pair(a, b) : std::make_pair(b, a);
            if (seen_edges.insert(edge).second)
                side_quads.push_back({edge.first, edge.second, v_cnt + edge.second, v_cnt + edge.first});
        }
    }

    if (planar_sides && !side_quads.empty()) {
        std::vector<Vector> sheet_normals(2 * v_cnt);
        for (size_t i = 0; i < v_cnt; i++) {
            sheet_normals[i] = normals[i];
            sheet_normals[v_cnt + i] = normals[i];
        }
        offset_pts = planarized_impl(offset_pts, side_quads, 1e-8, {}, sheet_normals);
    }

    std::vector<std::shared_ptr<wood_session::NodeBlockElement>> blocks;
    std::vector<Point> metric_pts;
    std::vector<std::vector<size_t>> final_sides;
    std::vector<std::vector<size_t>> final_caps;
    const bool make_caps_planar = flatten_caps >= 1.0 - 1e-6;

    for (size_t f = 0; f < faces.size(); f++) {
        const std::vector<size_t>& loop = faces[f];
        if (loop.size() < 3)
            continue;

        std::vector<Point> raw_bot;
        std::vector<Point> raw_top;
        Vector up_ref(0.0, 0.0, 0.0);
        for (const size_t idx : loop) {
            raw_bot.push_back(offset_pts[idx]);
            raw_top.push_back(offset_pts[v_cnt + idx]);
            up_ref = up_ref + normals[idx];
        }

        std::vector<Point> intrados = raw_bot;
        std::vector<Point> extrados = raw_top;
        bool block_planar_cap = make_caps_planar;
        if (flatten_caps > 1e-6) {
            const Plane bot_plane = oriented_plane(raw_bot, up_ref);
            const Plane top_plane = oriented_plane(raw_top, up_ref);
            for (size_t k = 0; k < loop.size(); k++) {
                intrados[k] = project_along_ruling(
                    raw_bot[k], raw_top[k], bot_plane, 0.0, flatten_caps, block_planar_cap
                );
                extrados[k] = project_along_ruling(
                    raw_bot[k], raw_top[k], top_plane, 1.0, flatten_caps, block_planar_cap
                );
            }
        }

        const std::shared_ptr<wood_session::NodeBlockElement> block = build_stone_element(
            intrados,
            extrados,
            block_planar_cap,
            trim_base,
            base_z,
            fmt::format("voussoir_{}", blocks.size())
        );
        if (!block)
            continue;

        const size_t base_idx = metric_pts.size();
        std::vector<size_t> bot_ring;
        std::vector<size_t> top_ring;
        for (size_t k = 0; k < intrados.size(); k++) {
            bot_ring.push_back(base_idx + k);
            metric_pts.push_back(intrados[k]);
        }
        for (size_t k = 0; k < extrados.size(); k++) {
            top_ring.push_back(base_idx + intrados.size() + k);
            metric_pts.push_back(extrados[k]);
        }
        final_caps.push_back(bot_ring);
        final_caps.push_back(top_ring);
        for (size_t k = 0; k < intrados.size(); k++) {
            const size_t nk = (k + 1) % intrados.size();
            final_sides.push_back({bot_ring[k], bot_ring[nk], top_ring[nk], top_ring[k]});
        }
        blocks.push_back(block);
    }

    if (metrics) {
        metrics->side_planarity = final_sides.empty() ? 0.0 : planarity(metric_pts, final_sides);
        metrics->cap_planarity = final_caps.empty() ? 0.0 : planarity(metric_pts, final_caps);
    }

    return blocks;
}

}
