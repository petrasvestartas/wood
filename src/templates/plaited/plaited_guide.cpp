#include "pch.h"
#include "src/templates/plaited/plaited_guide.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_plaited {

namespace {

const size_t NO_VERTEX = std::numeric_limits<size_t>::max();
const double WELD_SCALE = 10000.0;

/// Ordered undirected key of two vertex indices.
std::pair<size_t, size_t> edge_pair(size_t a, size_t b) {
    return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
}

/// Unit part of v perpendicular to unit_axis, falling back to any perpendicular when parallel.
Vector perpendicular_unit(const Vector& v, const Vector& unit_axis) {

    const Vector part = v - unit_axis * v.dot(unit_axis);
    if (!part.is_zero())
        return part.normalized();

    Vector fallback = unit_axis.cross({0.0, 0.0, 1.0});
    if (fallback.is_zero())
        fallback = unit_axis.cross({1.0, 0.0, 0.0});

    return fallback.normalized();
}

/// Outward Newell unit normal of a polygon of points.
Vector polygon_normal(const std::vector<Point>& ring) {

    if (ring.size() < 3)
        return {0.0, 0.0, 1.0};

    Vector normal(0.0, 0.0, 0.0);
    const Point& origin = ring[0];

    for (size_t i = 0; i < ring.size(); i++)
        normal += (ring[i] - origin).cross(ring[(i + 1) % ring.size()] - origin);

    return normal.is_zero() ? Vector(0.0, 0.0, 1.0) : normal.normalized();
}

/// Quantized 3D point key for welding shared polygon corners in procedural surface meshes.
struct QuantizedKey {
    int64_t x = 0; // Quantized x coordinate.
    int64_t y = 0; // Quantized y coordinate.
    int64_t z = 0; // Quantized z coordinate.

    bool operator<(const QuantizedKey& other) const {
        if (x != other.x)
            return x < other.x;
        if (y != other.y)
            return y < other.y;
        return z < other.z;
    }
};

/// Welds a polygon's corners into a shared vertex table and appends its face.
void append_welded_face(
    const std::vector<Point>& polygon,
    std::map<QuantizedKey, size_t>& table,
    std::vector<Point>& vertices,
    std::vector<std::vector<size_t>>& faces
) {

    std::vector<size_t> face;
    face.reserve(polygon.size());

    for (const Point& pt : polygon) {
        const QuantizedKey key{
            std::llround(pt[0] * WELD_SCALE),
            std::llround(pt[1] * WELD_SCALE),
            std::llround(pt[2] * WELD_SCALE)
        };
        const std::map<QuantizedKey, size_t>::iterator found = table.find(key);
        if (found != table.end()) {
            face.push_back(found->second);
        } else {
            const size_t idx = vertices.size();
            vertices.push_back(pt);
            table.emplace(key, idx);
            face.push_back(idx);
        }
    }

    faces.push_back(face);
}

/// Interns a unit sphere direction into primal_dirs using a quantized lookup table.
size_t intern_unit_direction(
    const Vector& raw,
    std::map<QuantizedKey, size_t>& table,
    std::vector<Vector>& dirs
) {

    const Vector unit = raw.normalized();
    const QuantizedKey key{
        std::llround(unit[0] * 100000.0),
        std::llround(unit[1] * 100000.0),
        std::llround(unit[2] * 100000.0)
    };
    const std::map<QuantizedKey, size_t>::iterator found = table.find(key);
    if (found != table.end())
        return found->second;

    const size_t idx = dirs.size();
    dirs.push_back(unit);
    table.emplace(key, idx);
    return idx;
}

/// Subdivides a zenith-aligned icosahedron by frequency into welded unit directions and triangles.
void build_geodesic_triangulation(
    int freq,
    std::vector<Vector>& primal_dirs,
    std::vector<std::array<size_t, 3>>& tris
) {

    const double golden = 0.5 * (1.0 + std::sqrt(5.0));
    const std::vector<Vector> raw_ico = {
        {-1.0, golden, 0.0}, {1.0, golden, 0.0}, {-1.0, -golden, 0.0}, {1.0, -golden, 0.0},
        {0.0, -1.0, golden}, {0.0, 1.0, golden}, {0.0, -1.0, -golden}, {0.0, 1.0, -golden},
        {golden, 0.0, -1.0}, {golden, 0.0, 1.0}, {-golden, 0.0, -1.0}, {-golden, 0.0, 1.0}
    };
    const Vector zenith_src = raw_ico[0].normalized();
    const Vector rot_axis = zenith_src.cross({0.0, 0.0, 1.0});
    const double rot_angle = std::acos(std::clamp(zenith_src[2], -1.0, 1.0));
    const Xform to_zenith =
        rot_axis.is_zero() ? Xform::identity() : Xform::rotation(rot_axis.normalized(), rot_angle);

    std::vector<Vector> ico_verts;
    ico_verts.reserve(12);
    for (const Vector& v : raw_ico)
        ico_verts.push_back(v.normalized().transformed(to_zenith));

    const std::vector<std::array<size_t, 3>> ico_tris = {
        {0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
        {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
        {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1}
    };

    std::map<QuantizedKey, size_t> primal_table;
    for (const std::array<size_t, 3>& tri : ico_tris) {
        std::vector<std::vector<size_t>> grid(static_cast<size_t>(freq + 1));
        for (int i = 0; i <= freq; i++) {
            grid[static_cast<size_t>(i)].resize(static_cast<size_t>(freq - i + 1));
            for (int j = 0; j <= freq - i; j++) {
                const int k = freq - i - j;
                const Vector interp =
                    ico_verts[tri[0]] * static_cast<double>(k) +
                    ico_verts[tri[1]] * static_cast<double>(i) +
                    ico_verts[tri[2]] * static_cast<double>(j);
                grid[static_cast<size_t>(i)][static_cast<size_t>(j)] =
                    intern_unit_direction(interp, primal_table, primal_dirs);
            }
        }
        for (int i = 0; i < freq; i++) {
            for (int j = 0; j < freq - i; j++) {
                const size_t p00 = grid[static_cast<size_t>(i)][static_cast<size_t>(j)];
                const size_t p10 = grid[static_cast<size_t>(i + 1)][static_cast<size_t>(j)];
                const size_t p01 = grid[static_cast<size_t>(i)][static_cast<size_t>(j + 1)];
                tris.push_back({p00, p10, p01});
                if (i + j + 1 < freq)
                    tris.push_back({p10, grid[static_cast<size_t>(i + 1)][static_cast<size_t>(j + 1)], p01});
            }
        }
    }
}

/// Clips a planar polygon against the horizontal half-space z >= z_cut.
std::vector<Point> clip_polygon_above_z(const std::vector<Point>& poly, double z_cut) {

    std::vector<Point> clipped;
    for (size_t i = 0; i < poly.size(); i++) {
        const Point& cur = poly[i];
        const Point& nxt = poly[(i + 1) % poly.size()];
        const double d_cur = cur[2] - z_cut;
        const double d_nxt = nxt[2] - z_cut;
        if (d_cur >= 0.0)
            clipped.push_back(cur);
        if ((d_cur >= 0.0 && d_nxt < 0.0) || (d_cur < 0.0 && d_nxt >= 0.0)) {
            const double t = d_cur / (d_cur - d_nxt);
            clipped.emplace_back(
                cur[0] + t * (nxt[0] - cur[0]),
                cur[1] + t * (nxt[1] - cur[1]),
                z_cut
            );
        }
    }

    return clipped;
}

/// Builds a canonical PlaitedCell from four corners, vertex keys and original mesh edge keys.
PlaitedCell build_plaited_cell(
    const std::array<Point, 4>& corners,
    const std::array<size_t, 4>& vertex_ids,
    const std::array<std::pair<size_t, size_t>, 4>& mesh_edges,
    int source_face,
    bool shift_by_one
) {

    PlaitedCell cell;
    for (size_t k = 0; k < 4; k++) {
        const size_t src = shift_by_one ? (k + 1) % 4 : k;
        cell.corners[k] = corners[src];
        cell.vertex_ids[k] = vertex_ids[src];
        cell.mesh_edges[k] = mesh_edges[src];
    }

    const std::vector<Point> ring = {
        cell.corners[0], cell.corners[1], cell.corners[2], cell.corners[3]
    };
    cell.centre = Point::centroid(ring);
    cell.normal = polygon_normal(ring);
    cell.source_face = source_face;
    return cell;
}

/// Ordered mesh faces and vertex point lookup of a Session Mesh, preserving native vertex keys.
void extract_mesh_topology(
    const Mesh& mesh,
    std::map<size_t, Point>& points,
    std::vector<std::vector<size_t>>& faces
) {

    const std::vector<size_t> fkeys = mesh.faces();
    faces.reserve(fkeys.size());

    for (const size_t fk : fkeys) {
        const std::vector<size_t> fverts = mesh.face_vertices(fk).value();
        faces.push_back(fverts);
        for (const size_t vk : fverts)
            points.emplace(vk, mesh.vertex_point(vk).value());
    }
}

/// Breadth-first parity assignment across polygon faces sharing undirected mesh edges.
std::vector<int> assign_face_parity(
    const std::vector<std::vector<size_t>>& faces,
    size_t target_sides,
    int seed_value
) {

    std::map<std::pair<size_t, size_t>, std::vector<std::pair<size_t, size_t>>> owners;
    for (size_t f = 0; f < faces.size(); f++) {
        if (faces[f].size() != target_sides)
            continue;
        for (size_t k = 0; k < target_sides; k++) {
            const std::pair<size_t, size_t> key =
                edge_pair(faces[f][k], faces[f][(k + 1) % target_sides]);
            owners[key].emplace_back(f, k);
        }
    }

    std::vector<int> parity(faces.size(), -1);
    for (size_t seed = 0; seed < faces.size(); seed++) {
        if (faces[seed].size() != target_sides || parity[seed] >= 0)
            continue;
        parity[seed] = seed_value;
        std::queue<size_t> queue;
        queue.push(seed);

        while (!queue.empty()) {
            const size_t cur = queue.front();
            queue.pop();
            for (size_t k = 0; k < target_sides; k++) {
                const std::pair<size_t, size_t> key =
                    edge_pair(faces[cur][k], faces[cur][(k + 1) % target_sides]);
                for (const std::pair<size_t, size_t>& nbr : owners[key]) {
                    if (nbr.first == cur || parity[nbr.first] >= 0)
                        continue;
                    parity[nbr.first] = static_cast<int>(
                        (nbr.second + 2 * target_sides - k + static_cast<size_t>(parity[cur]) + 1) % 2
                    );
                    queue.push(nbr.first);
                }
            }
        }
    }

    return parity;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Surface meshes for flat floors, arches and doubly curved shells
// ═══════════════════════════════════════════════════════════════════════════

Mesh flat_quad_mesh(
    double width,
    double depth,
    int nx,
    int ny,
    double skew,
    double z
) {

    const int cols = std::max(1, nx);
    const int rows = std::max(1, ny);
    const double shear = std::tan(skew);
    std::vector<Point> vertices;
    vertices.reserve(static_cast<size_t>(cols + 1) * static_cast<size_t>(rows + 1));

    for (int j = 0; j <= rows; j++) {
        const double v = static_cast<double>(j) / static_cast<double>(rows) - 0.5;
        const double y = v * depth;
        for (int i = 0; i <= cols; i++) {
            const double u = static_cast<double>(i) / static_cast<double>(cols) - 0.5;
            vertices.emplace_back(u * width + y * shear, y, z);
        }
    }

    std::vector<std::vector<size_t>> faces;
    faces.reserve(static_cast<size_t>(cols) * static_cast<size_t>(rows));
    const size_t stride = static_cast<size_t>(cols + 1);

    for (int j = 0; j < rows; j++) {
        for (int i = 0; i < cols; i++) {
            const size_t a = static_cast<size_t>(j) * stride + static_cast<size_t>(i);
            faces.push_back({a, a + 1, a + stride + 1, a + stride});
        }
    }

    return Mesh::from_vertices_and_faces(vertices, faces);
}

Mesh flat_hex_grid_mesh(
    int cols,
    int rows,
    double radius,
    double z
) {

    const int nx = std::max(1, cols);
    const int ny = std::max(1, rows);
    const double dx = 1.5 * radius;
    const double dy = std::sqrt(3.0) * radius;
    const double x_mid = 0.5 * static_cast<double>(nx - 1) * dx;
    const double y_mid = 0.5 * (static_cast<double>(ny - 1) * dy + (nx > 1 ? 0.5 * dy : 0.0));

    std::map<QuantizedKey, size_t> table;
    std::vector<Point> vertices;
    std::vector<std::vector<size_t>> faces;

    for (int c = 0; c < nx; c++) {
        const double cx = static_cast<double>(c) * dx - x_mid;
        const double y_shift = (c % 2 != 0) ? 0.5 * dy : 0.0;
        for (int r = 0; r < ny; r++) {
            const double cy = static_cast<double>(r) * dy + y_shift - y_mid;
            std::vector<Point> hex;
            hex.reserve(6);
            for (int k = 0; k < 6; k++) {
                const double angle = static_cast<double>(k) * (Tolerance::PI / 3.0);
                hex.emplace_back(cx + radius * std::cos(angle), cy + radius * std::sin(angle), z);
            }
            append_welded_face(hex, table, vertices, faces);
        }
    }

    return Mesh::from_vertices_and_faces(vertices, faces);
}

Mesh flat_hex_mesh(int rings, double radius, double z) {

    const int n = std::max(0, rings);
    const double root3 = std::sqrt(3.0);
    std::map<QuantizedKey, size_t> table;
    std::vector<Point> vertices;
    std::vector<std::vector<size_t>> faces;

    for (int q = -n; q <= n; q++) {
        const int r_min = std::max(-n, -q - n);
        const int r_max = std::min(n, -q + n);
        for (int r = r_min; r <= r_max; r++) {
            const double cx = radius * 1.5 * static_cast<double>(q);
            const double cy =
                radius * root3 * (static_cast<double>(r) + 0.5 * static_cast<double>(q));
            std::vector<Point> hex;
            hex.reserve(6);
            for (int k = 0; k < 6; k++) {
                const double angle = static_cast<double>(k) * (Tolerance::PI / 3.0);
                hex.emplace_back(cx + radius * std::cos(angle), cy + radius * std::sin(angle), z);
            }
            append_welded_face(hex, table, vertices, faces);
        }
    }

    return Mesh::from_vertices_and_faces(vertices, faces);
}

Mesh arch_quad_mesh(
    double span,
    double rise,
    double depth,
    int courses,
    int rings,
    double skew,
    double twist
) {

    const int nu = std::max(2, courses);
    const int nv = std::max(1, rings);
    const double half_span = 0.5 * span;
    const double radius = (half_span * half_span + rise * rise) / (2.0 * rise);
    const double z_centre = rise - radius;
    const double sector = std::atan2(half_span, radius - rise);
    const double shear = std::tan(skew);

    std::vector<Point> vertices;
    vertices.reserve(static_cast<size_t>(nu + 1) * static_cast<size_t>(nv + 1));

    for (int j = 0; j <= nv; j++) {
        const double v = static_cast<double>(j) / static_cast<double>(nv) - 0.5;
        const double y_local = v * depth;
        for (int i = 0; i <= nu; i++) {
            const double u = static_cast<double>(i) / static_cast<double>(nu);
            const double theta = -sector + 2.0 * sector * u;
            const double twist_angle = twist * (2.0 * u - 1.0);
            const Point base(
                radius * std::sin(theta) + y_local * shear,
                0.0,
                z_centre + radius * std::cos(theta)
            );
            const Vector tangent(std::cos(theta), 0.0, -std::sin(theta));
            const Xform twist_rot = Xform::rotation(tangent, twist_angle);
            const Vector across = Vector(0.0, 1.0, 0.0).transformed(twist_rot);
            vertices.push_back(base + across * y_local);
        }
    }

    std::vector<std::vector<size_t>> faces;
    faces.reserve(static_cast<size_t>(nu) * static_cast<size_t>(nv));
    const size_t stride = static_cast<size_t>(nu + 1);

    for (int j = 0; j < nv; j++) {
        for (int i = 0; i < nu; i++) {
            const size_t a = static_cast<size_t>(j) * stride + static_cast<size_t>(i);
            faces.push_back({a, a + 1, a + stride + 1, a + stride});
        }
    }

    return Mesh::from_vertices_and_faces(vertices, faces);
}

Mesh barrel_hex_mesh(
    double span,
    double rise,
    double depth,
    int courses,
    int rings
) {

    const int nx = std::max(2, courses);
    const int ny = std::max(1, rings);
    const double half_span = 0.5 * span;
    const double arc_radius = (half_span * half_span + rise * rise) / (2.0 * rise);
    const double z_centre = rise - arc_radius;
    const double sector = std::atan2(half_span, arc_radius - rise);
    const double arc_length = 2.0 * arc_radius * sector;
    const double root3 = std::sqrt(3.0);
    const double hex_radius = arc_length / (1.5 * static_cast<double>(nx - 1) + 2.0);
    const double dx = 1.5 * hex_radius;
    const double dy = root3 * hex_radius;
    const double y_scale =
        depth / ((static_cast<double>(ny - 1) + 0.5) * dy + root3 * hex_radius);
    const double s_mid = 0.5 * static_cast<double>(nx - 1) * dx;
    const double y_mid = 0.5 * (static_cast<double>(ny - 1) * dy + 0.5 * dy);

    std::map<QuantizedKey, size_t> table;
    std::vector<Point> vertices;
    std::vector<std::vector<size_t>> faces;

    for (int c = 0; c < nx; c++) {
        const double cs = static_cast<double>(c) * dx - s_mid;
        const double shift = (c % 2 != 0) ? 0.5 * dy : 0.0;
        for (int r = 0; r < ny; r++) {
            const double cy = static_cast<double>(r) * dy + shift - y_mid;
            std::vector<Point> hex;
            hex.reserve(6);
            for (int k = 0; k < 6; k++) {
                const double angle = static_cast<double>(k) * (Tolerance::PI / 3.0);
                const double s = cs + hex_radius * std::cos(angle);
                const double y = (cy + hex_radius * std::sin(angle)) * y_scale;
                const double theta = s / arc_radius;
                hex.emplace_back(
                    arc_radius * std::sin(theta),
                    y,
                    z_centre + arc_radius * std::cos(theta)
                );
            }
            append_welded_face(hex, table, vertices, faces);
        }
    }

    return Mesh::from_vertices_and_faces(vertices, faces);
}

Mesh spherical_cap_hex_mesh(double span, double rise, int rings, bool circular_rim) {

    const int n = std::max(1, rings);
    const int grid_extent = circular_rim ? (n + 2) : n;
    const double half_span = 0.5 * span;
    const double sphere_radius = (half_span * half_span + rise * rise) / (2.0 * rise);
    const double z_centre = rise - sphere_radius;
    const double theta_max = std::atan2(half_span, sphere_radius - rise);
    const double root3 = std::sqrt(3.0);
    const double circle_radial = 2.0 * static_cast<double>(n);

    std::vector<std::array<Point, 6>> planar_hexes;
    double max_radial = 0.0;

    for (int q = -grid_extent; q <= grid_extent; q++) {
        const int r_min = std::max(-grid_extent, -q - grid_extent);
        const int r_max = std::min(grid_extent, -q + grid_extent);
        for (int r = r_min; r <= r_max; r++) {
            const double cx = 1.5 * static_cast<double>(q);
            const double cy = root3 * (static_cast<double>(r) + 0.5 * static_cast<double>(q));
            if (circular_rim && std::hypot(cx, cy) > circle_radial + 1.35) {
                continue;
            }
            std::array<Point, 6> hex;
            for (int k = 0; k < 6; k++) {
                const double angle = static_cast<double>(k) * (Tolerance::PI / 3.0);
                const double px = cx + std::cos(angle);
                const double py = cy + std::sin(angle);
                max_radial = std::max(max_radial, std::hypot(px, py));
                hex[static_cast<size_t>(k)] = Point(px, py, 0.0);
            }
            planar_hexes.push_back(hex);
        }
    }

    const double ref_radial = circular_rim ? circle_radial : max_radial;
    std::map<QuantizedKey, size_t> table;
    std::vector<Point> vertices;
    std::vector<std::vector<size_t>> faces;

    for (const std::array<Point, 6>& hex : planar_hexes) {
        std::vector<Point> mapped;
        mapped.reserve(6);
        for (const Point& pt : hex) {
            const double radial = std::hypot(pt[0], pt[1]);
            const double theta = ref_radial > 0.0 ? theta_max * (radial / ref_radial) : 0.0;
            const double phi = std::atan2(pt[1], pt[0]);
            mapped.emplace_back(
                sphere_radius * std::sin(theta) * std::cos(phi),
                sphere_radius * std::sin(theta) * std::sin(phi),
                z_centre + sphere_radius * std::cos(theta)
            );
        }
        append_welded_face(mapped, table, vertices, faces);
    }

    return Mesh::from_vertices_and_faces(vertices, faces);
}

Mesh geodesic_dome_mesh(double radius, int frequency, double clip_ratio) {

    std::vector<Vector> primal_dirs;
    std::vector<std::array<size_t, 3>> tris;
    build_geodesic_triangulation(std::max(1, frequency), primal_dirs, tris);

    std::vector<Plane> tangent_planes;
    tangent_planes.reserve(primal_dirs.size());
    for (const Vector& dir : primal_dirs) {
        tangent_planes.push_back(
            Plane::from_point_normal({dir[0] * radius, dir[1] * radius, dir[2] * radius}, dir)
        );
    }

    std::vector<Point> dual_points(tris.size());
    std::vector<std::vector<size_t>> incident(primal_dirs.size());
    for (size_t t = 0; t < tris.size(); t++) {
        Point hit;
        if (!Intersection::plane_plane_plane(
                tangent_planes[tris[t][0]],
                tangent_planes[tris[t][1]],
                tangent_planes[tris[t][2]],
                hit
            )) {
            const Vector dir =
                (primal_dirs[tris[t][0]] + primal_dirs[tris[t][1]] + primal_dirs[tris[t][2]])
                    .normalized();
            hit = Point(dir[0] * radius, dir[1] * radius, dir[2] * radius);
        }
        dual_points[t] = hit;
        for (size_t c = 0; c < 3; c++)
            incident[tris[t][c]].push_back(t);
    }

    const double z_target = radius * (1.0 - 2.0 * std::clamp(clip_ratio, 0.18, 0.46));
    double z_cut = z_target;
    double best_clearance = -1.0;
    for (int step = -40; step <= 40; step++) {
        const double candidate = z_target + static_cast<double>(step) * (0.002 * radius);
        if (candidate < 0.05 * radius || candidate > 0.72 * radius)
            continue;
        double clearance = radius;
        for (const Point& dp : dual_points)
            clearance = std::min(clearance, std::abs(dp[2] - candidate));
        if (clearance > best_clearance) {
            best_clearance = clearance;
            z_cut = candidate;
        }
    }

    std::map<QuantizedKey, size_t> out_table;
    std::vector<Point> vertices;
    std::vector<std::vector<size_t>> faces;

    for (size_t v = 0; v < primal_dirs.size(); v++) {
        if (incident[v].size() < 5 || primal_dirs[v][2] <= -0.35) {
            continue;
        }
        const Vector& normal = primal_dirs[v];
        const Vector axis_x = perpendicular_unit({1.0, 0.0, 0.0}, normal);
        const Vector axis_y = normal.cross(axis_x).normalized();

        std::vector<std::pair<double, size_t>> ordered;
        for (const size_t t : incident[v]) {
            const Vector rel = dual_points[t] - Point(0.0, 0.0, 0.0);
            ordered.emplace_back(std::atan2(rel.dot(axis_y), rel.dot(axis_x)), t);
        }
        std::sort(ordered.begin(), ordered.end());

        std::vector<Point> poly;
        for (const std::pair<double, size_t>& item : ordered) {
            poly.push_back(dual_points[item.second]);
        }

        std::vector<Point> clipped = clip_polygon_above_z(poly, z_cut);
        if (clipped.size() < 3) {
            continue;
        }
        double max_above = 0.0;
        for (Point& pt : clipped) {
            max_above = std::max(max_above, pt[2] - z_cut);
            pt = Point(pt[0], pt[1], pt[2] - z_cut);
        }
        if (max_above <= 1.0) {
            continue;
        }
        append_welded_face(clipped, out_table, vertices, faces);
    }

    return Mesh::from_vertices_and_faces(vertices, faces);
}

Mesh doubly_curved_quad_mesh(
    double width,
    double depth,
    double rise,
    int nx,
    int ny,
    double saddle
) {

    const int cols = std::max(2, nx);
    const int rows = std::max(2, ny);
    std::vector<Point> vertices;
    vertices.reserve(static_cast<size_t>(cols + 1) * static_cast<size_t>(rows + 1));

    double z_min = std::numeric_limits<double>::max();
    for (int j = 0; j <= rows; j++) {
        const double v = 2.0 * static_cast<double>(j) / static_cast<double>(rows) - 1.0;
        for (int i = 0; i <= cols; i++) {
            const double u = 2.0 * static_cast<double>(i) / static_cast<double>(cols) - 1.0;
            const double dome_z = rise * (1.0 - u * u) * (1.0 - v * v);
            const double hypar_z = 0.5 * rise * (1.0 - u * u + v * v);
            const double z = (1.0 - saddle) * dome_z + saddle * hypar_z;
            z_min = std::min(z_min, z);
            vertices.emplace_back(0.5 * width * u, 0.5 * depth * v, z);
        }
    }

    for (Point& pt : vertices)
        pt = Point(pt[0], pt[1], pt[2] - z_min);

    std::vector<std::vector<size_t>> faces;
    faces.reserve(static_cast<size_t>(cols) * static_cast<size_t>(rows));
    const size_t stride = static_cast<size_t>(cols + 1);

    for (int j = 0; j < rows; j++) {
        for (int i = 0; i < cols; i++) {
            const size_t a = static_cast<size_t>(j) * stride + static_cast<size_t>(i);
            faces.push_back({a, a + 1, a + stride + 1, a + stride});
        }
    }

    return Mesh::from_vertices_and_faces(vertices, faces);
}

// ═══════════════════════════════════════════════════════════════════════════
// PlaitedGuide
// ═══════════════════════════════════════════════════════════════════════════

PlaitedGuide::PlaitedGuide(
    const Mesh& mesh,
    PlaitedPattern pattern,
    double thickness,
    double bevel,
    double relief_depth,
    bool flip_chirality,
    bool bevel_naked_edges,
    double opening,
    bool flat_base
)
    : WoodSession("plaited_guide"),
      mesh(mesh),
      pattern(pattern),
      thickness(std::max(1.0, thickness)),
      bevel(std::clamp(bevel, -1.2, 1.2)),
      relief_depth(std::clamp(relief_depth, 0.0, std::max(0.0, thickness - 1e-3))),
      flip_chirality(flip_chirality),
      bevel_naked_edges(bevel_naked_edges),
      opening(std::clamp(opening, 0.0, 0.85)),
      flat_base(flat_base) {

    // horizontal springing plane when flat_base is requested on a curved mesh
    _base_plane = compute_base_plane();

    // extrados cells and shared bevel seams
    std::vector<PlaitedCell> built_cells = compute_cells();
    const std::vector<PlaitedSeam> built_seams = compute_seams(built_cells);
    _cells = std::move(built_cells);
    _seams = built_seams;

    // reciprocal bar axes and optional open hot-wire cheek planes
    _axes = compute_axes();
    if (this->opening > 0.0)
        _open_planes = compute_open_planes();

    draw();
}

const std::vector<PlaitedCell>& PlaitedGuide::cells() const {
    return _cells;
}

const std::vector<PlaitedSeam>& PlaitedGuide::seams() const {
    return _seams;
}

const std::vector<PlaitedAxis>& PlaitedGuide::axes() const {
    return _axes;
}

const Plane& PlaitedGuide::side_plane(size_t cell_index, size_t edge_index) const {

    if (!_open_planes.empty())
        return _open_planes.at(cell_index).at(edge_index);

    return seam_plane(cell_index, edge_index);
}

const Plane& PlaitedGuide::seam_plane(size_t cell_index, size_t edge_index) const {

    const int seam_id = _cells.at(cell_index).seam_ids.at(edge_index);
    const PlaitedSeam& seam = _seams.at(static_cast<size_t>(seam_id));
    return seam.cell_a == static_cast<int>(cell_index) ? seam.plane_for_a : seam.plane_for_b;
}

const Plane& PlaitedGuide::relief_plane(size_t cell_index, size_t edge_index) const {

    const int seam_id = _cells.at(cell_index).seam_ids.at(edge_index);
    const PlaitedSeam& seam = _seams.at(static_cast<size_t>(seam_id));
    return seam.cell_a == static_cast<int>(cell_index) ? seam.relief_for_a : seam.relief_for_b;
}

const std::optional<Plane>& PlaitedGuide::base_plane() const {
    return _base_plane;
}

int PlaitedGuide::neighbor_cell(size_t cell_index, size_t edge_index) const {

    const int seam_id = _cells.at(cell_index).seam_ids.at(edge_index);
    const PlaitedSeam& seam = _seams.at(static_cast<size_t>(seam_id));
    return seam.cell_a == static_cast<int>(cell_index) ? seam.cell_b : seam.cell_a;
}

std::optional<Plane> PlaitedGuide::compute_base_plane() const {

    if (!flat_base)
        return std::nullopt;

    double z_min = std::numeric_limits<double>::max();
    double z_max = std::numeric_limits<double>::lowest();

    for (const size_t fk : mesh.faces()) {
        for (const size_t vk : mesh.face_vertices(fk).value()) {
            const Point pt = mesh.vertex_point(vk).value();
            z_min = std::min(z_min, pt[2]);
            z_max = std::max(z_max, pt[2]);
        }
    }

    if (z_max - z_min <= 1e-2) {
        return std::nullopt;
    }

    const double z_level = (z_min < -1e-2 && z_max > 1e-2) ? 0.0 : z_min;
    return Plane::from_point_normal({0.0, 0.0, z_level}, {0.0, 0.0, 1.0});
}

// ═══════════════════════════════════════════════════════════════════════════
// Cell decompositions
// ═══════════════════════════════════════════════════════════════════════════

std::vector<PlaitedCell> PlaitedGuide::compute_cells() const {

    if (pattern == PlaitedPattern::Abeille)
        return compute_abeille_cells();
    if (pattern == PlaitedPattern::Sphera)
        return compute_sphera_cells();
    return compute_star_cells();
}

std::vector<PlaitedCell> PlaitedGuide::compute_abeille_cells() const {

    std::map<size_t, Point> points;
    std::vector<std::vector<size_t>> faces;
    extract_mesh_topology(mesh, points, faces);

    const std::vector<int> parity = assign_face_parity(faces, 4, flip_chirality ? 1 : 0);
    std::vector<PlaitedCell> cells;
    cells.reserve(faces.size());

    for (size_t f = 0; f < faces.size(); f++) {
        if (faces[f].size() != 4)
            continue;
        const std::array<Point, 4> quad_pts = {
            points.at(faces[f][0]),
            points.at(faces[f][1]),
            points.at(faces[f][2]),
            points.at(faces[f][3])
        };
        const std::array<size_t, 4> quad_ids = {
            faces[f][0], faces[f][1], faces[f][2], faces[f][3]
        };
        const std::array<std::pair<size_t, size_t>, 4> mesh_edges = {
            edge_pair(faces[f][0], faces[f][1]),
            edge_pair(faces[f][1], faces[f][2]),
            edge_pair(faces[f][2], faces[f][3]),
            edge_pair(faces[f][3], faces[f][0])
        };
        cells.push_back(
            build_plaited_cell(quad_pts, quad_ids, mesh_edges, static_cast<int>(f), parity[f] == 1)
        );
    }

    return cells;
}

std::vector<PlaitedCell> PlaitedGuide::compute_sphera_cells() const {

    std::map<size_t, Point> points;
    std::vector<std::vector<size_t>> faces;
    extract_mesh_topology(mesh, points, faces);

    size_t next_vid = points.empty() ? 0 : (points.rbegin()->first + 1);
    const std::vector<int> spoke_offset = assign_face_parity(faces, 6, 0);
    const std::pair<size_t, size_t> interior_spoke{NO_VERTEX, NO_VERTEX};
    std::vector<PlaitedCell> cells;

    for (size_t f = 0; f < faces.size(); f++) {
        if (faces[f].size() != 6)
            continue;
        std::vector<Point> ring;
        ring.reserve(6);
        for (const size_t vid : faces[f])
            ring.push_back(points.at(vid));

        const Point centre = Point::centroid(ring);
        const size_t centre_vid = next_vid++;
        const size_t offset = static_cast<size_t>(spoke_offset[f]);

        for (size_t m = 0; m < 3; m++) {
            const size_t i0 = (offset + 2 * m) % 6;
            const size_t i1 = (i0 + 1) % 6;
            const size_t i2 = (i0 + 2) % 6;
            const std::array<Point, 4> corners = {ring[i0], ring[i1], ring[i2], centre};
            const std::array<size_t, 4> vids = {
                faces[f][i0], faces[f][i1], faces[f][i2], centre_vid
            };
            const std::array<std::pair<size_t, size_t>, 4> mesh_edges = {
                edge_pair(faces[f][i0], faces[f][i1]),
                edge_pair(faces[f][i1], faces[f][i2]),
                interior_spoke,
                interior_spoke
            };
            cells.push_back(
                build_plaited_cell(corners, vids, mesh_edges, static_cast<int>(f), flip_chirality)
            );
        }
    }

    return cells;
}

std::vector<PlaitedCell> PlaitedGuide::compute_star_cells() const {

    std::map<size_t, Point> points;
    std::vector<std::vector<size_t>> faces;
    extract_mesh_topology(mesh, points, faces);

    size_t next_vid = points.empty() ? 0 : (points.rbegin()->first + 1);
    std::map<std::pair<size_t, size_t>, std::pair<size_t, Point>> edge_midpoints;

    for (const std::vector<size_t>& face : faces) {
        const size_t n = face.size();
        for (size_t k = 0; k < n; k++) {
            const std::pair<size_t, size_t> key = edge_pair(face[k], face[(k + 1) % n]);
            if (edge_midpoints.find(key) == edge_midpoints.end()) {
                const Point mid = Point::mid_point(points.at(key.first), points.at(key.second));
                edge_midpoints.emplace(key, std::make_pair(next_vid++, mid));
            }
        }
    }

    const std::pair<size_t, size_t> interior_spoke{NO_VERTEX, NO_VERTEX};
    std::vector<PlaitedCell> cells;

    for (size_t f = 0; f < faces.size(); f++) {
        const std::vector<size_t>& face = faces[f];
        const size_t n = face.size();
        if (n < 3)
            continue;

        std::vector<Point> ring;
        ring.reserve(n);
        for (const size_t vid : face)
            ring.push_back(points.at(vid));

        const Point centre = Point::centroid(ring);
        const size_t centre_vid = next_vid++;

        for (size_t k = 0; k < n; k++) {
            const size_t prev = (k + n - 1) % n;
            const std::pair<size_t, size_t> prev_edge = edge_pair(face[prev], face[k]);
            const std::pair<size_t, size_t> next_edge = edge_pair(face[k], face[(k + 1) % n]);
            const std::pair<size_t, Point>& mid_prev = edge_midpoints.at(prev_edge);
            const std::pair<size_t, Point>& mid_next = edge_midpoints.at(next_edge);

            const std::array<Point, 4> corners = {
                centre, mid_prev.second, ring[k], mid_next.second
            };
            const std::array<size_t, 4> vids = {
                centre_vid, mid_prev.first, face[k], mid_next.first
            };
            const std::array<std::pair<size_t, size_t>, 4> mesh_edges = {
                interior_spoke, prev_edge, next_edge, interior_spoke
            };
            cells.push_back(
                build_plaited_cell(corners, vids, mesh_edges, static_cast<int>(f), flip_chirality)
            );
        }
    }

    return cells;
}

// ═══════════════════════════════════════════════════════════════════════════
// Shared seam planes and reciprocal axes
// ═══════════════════════════════════════════════════════════════════════════

std::vector<PlaitedSeam> PlaitedGuide::compute_seams(std::vector<PlaitedCell>& cells) const {

    std::map<std::pair<size_t, size_t>, size_t> seam_lookup;
    std::vector<PlaitedSeam> seams;

    for (size_t c = 0; c < cells.size(); c++) {
        for (size_t k = 0; k < 4; k++) {
            const std::pair<size_t, size_t> key =
                edge_pair(cells[c].vertex_ids[k], cells[c].vertex_ids[(k + 1) % 4]);
            const int edge_sign = (k % 2 == 0) ? -1 : 1;
            const std::map<std::pair<size_t, size_t>, size_t>::iterator found =
                seam_lookup.find(key);

            if (found == seam_lookup.end()) {
                const size_t seam_idx = seams.size();
                PlaitedSeam seam;
                seam.start = cells[c].corners[k];
                seam.end = cells[c].corners[(k + 1) % 4];
                seam.midpoint = Point::mid_point(seam.start, seam.end);
                seam.cell_a = static_cast<int>(c);
                seam.edge_a = static_cast<int>(k);
                seam.sign_a = edge_sign;
                seams.push_back(seam);
                seam_lookup.emplace(key, seam_idx);
                cells[c].seam_ids[k] = static_cast<int>(seam_idx);
            } else {
                const size_t seam_idx = found->second;
                seams[seam_idx].cell_b = static_cast<int>(c);
                seams[seam_idx].edge_b = static_cast<int>(k);
                seams[seam_idx].sign_b = edge_sign;
                cells[c].seam_ids[k] = static_cast<int>(seam_idx);
            }
        }
    }

    const double wedge_height = std::clamp(thickness - relief_depth, 0.0, thickness);

    for (PlaitedSeam& seam : seams) {
        const Vector edge_vec = seam.end - seam.start;
        seam.tangent = edge_vec.is_zero() ? Vector(1.0, 0.0, 0.0) : edge_vec.normalized();

        const PlaitedCell& ca = cells[static_cast<size_t>(seam.cell_a)];
        const Vector sum_normal =
            seam.cell_b >= 0
                ? (ca.normal + cells[static_cast<size_t>(seam.cell_b)].normal)
                : ca.normal;
        seam.normal = perpendicular_unit(sum_normal, seam.tangent);

        Vector inward_a = seam.normal.cross(seam.tangent).normalized();
        if ((ca.centre - seam.midpoint).dot(inward_a) < 0.0)
            inward_a = -inward_a;

        const bool interior_matched = seam.cell_b >= 0 && seam.sign_a == -seam.sign_b;
        const double alpha =
            (interior_matched || (seam.cell_b < 0 && bevel_naked_edges)) ? bevel : 0.0;

        Vector bevel_normal_a =
            (inward_a * std::cos(alpha) -
             seam.normal * (static_cast<double>(seam.sign_a) * std::sin(alpha)))
                .normalized();

        if (_base_plane.has_value() && seam.cell_b < 0 && !bevel_naked_edges) {
            const double z_base = _base_plane->origin()[2];
            if (opening > 0.0) {
                const Vector horiz_inward(inward_a[0], inward_a[1], 0.0);
                if (horiz_inward.magnitude() > 0.5)
                    bevel_normal_a = horiz_inward.normalized();
            } else if (
                std::abs(seam.start[2] - z_base) <= 1e-2 &&
                std::abs(seam.end[2] - z_base) <= 1e-2 &&
                ca.centre[2] > z_base + 1e-2
            ) {
                bevel_normal_a = {0.0, 0.0, 1.0};
            }
        }

        seam.plane_for_a = Plane::from_point_normal(seam.midpoint, bevel_normal_a);
        seam.plane_for_b = Plane::from_point_normal(seam.midpoint, -bevel_normal_a);

        const Point waist =
            seam.midpoint - seam.normal * wedge_height -
            inward_a * (static_cast<double>(seam.sign_a) * wedge_height * std::tan(alpha));
        seam.relief_for_a = Plane::from_point_normal(waist, inward_a);
        seam.relief_for_b = Plane::from_point_normal(waist, -inward_a);
    }

    return seams;
}

std::vector<PlaitedAxis> PlaitedGuide::compute_axes() const {

    std::vector<PlaitedAxis> axes;
    axes.reserve(_cells.size());

    for (const PlaitedCell& cell : _cells) {
        const Point start = Point::mid_point(cell.corners[3], cell.corners[0]);
        const Point end = Point::mid_point(cell.corners[1], cell.corners[2]);
        const Vector span = end - start;
        const Vector dir = span.is_zero() ? Vector(1.0, 0.0, 0.0) : span.normalized();
        const Vector up = perpendicular_unit(cell.normal, dir);
        axes.push_back(PlaitedAxis{
            Line::from_points(start, end),
            dir,
            up,
            Point::mid_point(start, end)
        });
    }

    return axes;
}

std::vector<std::array<Plane, 4>> PlaitedGuide::compute_open_planes() const {

    const size_t count = _cells.size();
    std::vector<std::array<Plane, 4>> planes(count);
    std::vector<Point> mid_centres(count);

    for (size_t c = 0; c < count; c++) {
        const PlaitedCell& cell = _cells[c];
        const PlaitedAxis& axis = _axes[c];
        const Vector depth_shift = -axis.up * (0.5 * thickness);
        const Point m0 = Point::mid_point(cell.corners[0], cell.corners[1]);
        const Point m2 = Point::mid_point(cell.corners[2], cell.corners[3]);
        const Point m0_mid = m0 + depth_shift;
        const Point m2_mid = m2 + depth_shift;

        Vector inward_0 = axis.up.cross(axis.direction).normalized();
        if ((axis.centre - m0).dot(inward_0) < 0.0)
            inward_0 = -inward_0;

        const double cell_width = std::max(1.0, std::abs((m2 - m0).dot(inward_0)));
        const double half_mid = 0.5 * (1.0 - opening) * cell_width;
        const double max_abs_alpha = std::atan2(0.75 * half_mid, 0.5 * thickness);
        const double alpha = std::clamp(bevel, -max_abs_alpha, max_abs_alpha);

        const Vector normal_0 =
            (inward_0 * std::cos(alpha) - axis.up * std::sin(alpha)).normalized();
        const Vector normal_2 =
            (-inward_0 * std::cos(alpha) - axis.up * std::sin(alpha)).normalized();

        const bool naked_0 = neighbor_cell(c, 0) < 0;
        const bool naked_2 = neighbor_cell(c, 2) < 0;

        if (naked_0 && !naked_2) {
            planes[c][0] = seam_plane(c, 0);
            planes[c][2] = Plane::from_point_normal(m0_mid + inward_0 * (2.0 * half_mid), normal_2);
            mid_centres[c] = m0_mid + inward_0 * half_mid;
        } else if (!naked_0 && naked_2) {
            planes[c][0] = Plane::from_point_normal(m2_mid - inward_0 * (2.0 * half_mid), normal_0);
            planes[c][2] = seam_plane(c, 2);
            mid_centres[c] = m2_mid - inward_0 * half_mid;
        } else {
            mid_centres[c] = axis.centre + depth_shift;
            planes[c][0] = Plane::from_point_normal(mid_centres[c] - inward_0 * half_mid, normal_0);
            planes[c][2] = Plane::from_point_normal(mid_centres[c] + inward_0 * half_mid, normal_2);
        }
    }

    for (size_t c = 0; c < count; c++) {
        for (const size_t k : {size_t{1}, size_t{3}}) {
            const int nbr = neighbor_cell(c, k);
            if (nbr >= 0) {
                const size_t n_idx = static_cast<size_t>(nbr);
                const Vector delta = mid_centres[c] - mid_centres[n_idx];
                const Plane& cheek_0 = planes[n_idx][0];
                const Plane& cheek_2 = planes[n_idx][2];
                const Plane& near_cheek =
                    delta.dot(cheek_0.z_axis()) < delta.dot(cheek_2.z_axis()) ? cheek_0 : cheek_2;
                planes[c][k] = Plane::from_point_normal(near_cheek.origin(), -near_cheek.z_axis());
            } else {
                planes[c][k] = seam_plane(c, k);
            }
        }
    }

    return planes;
}

void PlaitedGuide::draw() {

    add_mesh(mesh, group_named("mesh"));

    const std::shared_ptr<TreeNode> cell_group = group_named("cells");
    for (const PlaitedCell& cell : _cells) {
        const Polyline outline({
            cell.corners[0],
            cell.corners[1],
            cell.corners[2],
            cell.corners[3],
            cell.corners[0]
        });
        add_polyline(outline, cell_group);
    }

    const std::shared_ptr<TreeNode> axis_group = group_named("axes");
    for (const PlaitedAxis& axis : _axes)
        add_line(axis.line, axis_group);
}

} // namespace wood_plaited
