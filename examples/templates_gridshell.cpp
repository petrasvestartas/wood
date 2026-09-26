#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/shells/lamella_gridshell_net.h"

using namespace session_cpp;
using namespace wood_session;

/// One gridshell of the row: its group name, carrier, path, plan side, lamellas per family and how they are seeded.
struct Scene {
    std::string name; // Group name prefix.
    int carrier; // 0 the cubic saddle, 1 the hyperbolic paraboloid, 2 the half catenoid; meshes: 3 the relaxed saddle disk, 4 the catenoid annulus, 5 the Enneper disk.
    wood_gridshell::Path path; // What the lamellas follow.
    double size; // Plan side of the carrier.
    int count; // Lamellas per family.
    double edge; // Negative: seeds along the seed line of each family through the middle; else both families seeded along the parameter line u = edge, the catenoid's bottom rim at 0.
};

const double CONSTANT = 2e-5; // normal curvature in 1/mm the lamellas of the constant scene keep, inside [k2, k1] all over the saddle
const double WAIST = 2000.0; // waist radius c of the catenoid r = c cosh(z / c), between rims 1.3 c above and below

const std::vector<Scene> SCENES = {
    {"asymptotic", 0, wood_gridshell::Path::normal_curvature(0.0), 10000.0, 9, -1.0},
    {"iso", 0, wood_gridshell::Path::isocurves(), 6000.0, 5, -1.0},
    {"paraboloid", 1, wood_gridshell::Path::normal_curvature(0.0), 8000.0, 7, -1.0},
    {"catenoid", 2, wood_gridshell::Path::normal_curvature(0.0), 8000.0, 8, 0.02},
    {"constant", 0, wood_gridshell::Path::normal_curvature(CONSTANT), 8000.0, 7, -1.0},
    {"saddle_mesh", 3, wood_gridshell::Path::normal_curvature(0.0), 12000.0, 14, -1.0},
    {"catenoid_mesh", 4, wood_gridshell::Path::normal_curvature(0.0), 12000.0, 12, -1.0},
    {"enneper_mesh", 5, wood_gridshell::Path::normal_curvature(0.0), 16000.0, 16, -1.0},
};
const int SWEEPS = 400; // cotangent Laplacian sweeps that relax the saddle mesh to a minimal one
const int ROUNDS = 40; // rounds of guided projection before the final six
const double RISE = 0.15; // corner lift and drop of a saddle over its side
const double SKEW = 0.5; // how much faster the saddle curves across at one end than at the other, so the asymptotic curves bend in plan
const double GAP = 4000.0; // clear distance between the shells
const wood_gridshell::Lamella LAMELLA{.height = 140.0, .thickness = 20.0, .gap = 60.0, .spacing = 180.0, .overrun = 20.0, .step = 50.0, .sample = 50.0};
const double CLEARANCE = 1.0; // faces closer than this count as touching, not overlapping
const double FIT = 0.1; // largest gap, and largest mismatch at the node section, in mm between a stud flat and a board it holds
const double RESIDUAL = 1e-8; // largest sum of squares of the A-net constraints on the unit net after optimisation, the weakest of Schling's Table 2 (1e-8 to 1e-22)
const double STRAIGHT = 5e-3; // largest departure in 1/m of a lamella's normal curvature from its path's value, a bending radius of 200 m
const int SAMPLES = 200; // curvature samples per lamella

/// A convex part of an element with its box, what the clash check cuts.
struct Piece {
    size_t element; // Position of the element in the gridshell.
    Mesh mesh; // Closed convex solid.
    std::pair<Point, Point> box; // Lowest and highest corner.
};

// ═══════════════════════════════════════════════════════════════════════════
// Carriers
// ═══════════════════════════════════════════════════════════════════════════

/// The height of the skew saddle over x and y in [-1, 1], z = x^2 - y^2 (1 + s x): the curvature across grows along x, and z_xx z_yy - z_xy^2 = -4 (1 + s x) - 4 s^2 y^2 < 0, so the Gaussian curvature is negative everywhere.
double compute_height(double size, double x, double y) {
    return RISE * size / 2.0 * (x * x - y * y * (1.0 + SKEW * x));
}

/// The inverse of a 4 x 4 matrix by Gauss-Jordan elimination.
std::array<std::array<double, 4>, 4> compute_inverse(std::array<std::array<double, 4>, 4> m) {

    std::array<std::array<double, 4>, 4> inverse{};
    for (int i = 0; i < 4; i++)
        inverse[i][i] = 1.0;

    for (int c = 0; c < 4; c++) {
        const double pivot = m[c][c];
        for (int j = 0; j < 4; j++) {
            m[c][j] /= pivot;
            inverse[c][j] /= pivot;
        }

        for (int r = 0; r < 4; r++) {
            if (r == c)
                continue;

            const double factor = m[r][c];
            for (int j = 0; j < 4; j++) {
                m[r][j] -= factor * m[c][j];
                inverse[r][j] -= factor * inverse[c][j];
            }
        }
    }

    return inverse;
}

/// The bicubic Bezier patch through f at the 4 x 4 nodes i / 3 of the unit square: exact for any polynomial of degree at most 3 in each parameter, u the first parameter.
NurbsSurface compute_bezier(const std::function<Point(double, double)>& f) {

    std::array<std::array<double, 4>, 4> bernstein{};
    for (int i = 0; i < 4; i++) {
        const double s = i / 3.0;
        const double binomials[4] = {1.0, 3.0, 3.0, 1.0};
        for (int j = 0; j < 4; j++)
            bernstein[i][j] = binomials[j] * std::pow(s, j) * std::pow(1.0 - s, 3 - j);
    }

    const std::array<std::array<double, 4>, 4> inverse = compute_inverse(bernstein);
    std::vector<Point> points(16, Point(0.0, 0.0, 0.0));
    for (int i = 0; i < 4; i++)
        for (int k = 0; k < 4; k++) {
            Point cv(0.0, 0.0, 0.0);
            for (int a = 0; a < 4; a++)
                for (int b = 0; b < 4; b++)
                    cv = cv + (f(a / 3.0, b / 3.0) - Point(0.0, 0.0, 0.0)) * (inverse[i][a] * inverse[k][b]);

            points[i * 4 + k] = cv;
        }

    return NurbsSurface::create(false, false, 3, 3, 4, 4, points);
}

/// A cubic saddle over a square of side size from x, as the Bezier patch through its heights; u along x and v along y so the normal points up.
NurbsSurface compute_saddle(double size, double x) {
    return compute_bezier([&](double u, double v) {
        return Point(x + u * size, (v - 0.5) * size, compute_height(size, 2.0 * u - 1.0, 2.0 * v - 1.0));
    });
}

/// A hyperbolic paraboloid z = x y over a square of side size from x, turned 45 degrees so its straight asymptotic lines run along the sides; a doubly ruled surface, so every asymptotic curve is a straight line.
NurbsSurface compute_paraboloid(double size, double x) {
    return compute_bezier([&](double u, double v) {
        return Point(x + u * size, (v - 0.5) * size, RISE * size / 2.0 * (2.0 * u - 1.0) * (2.0 * v - 1.0));
    });
}

/// Half of the catenoid r = WAIST cosh(z / WAIST) between z = -1.3 and 1.3 WAIST, its axis vertical through the middle of a square of side size from x: u the cubic B-spline profile through 9 samples of cosh, v two exact rational quadratic arcs of 90 degrees, so the surface is a NURBS fit of a minimal surface.
NurbsSurface compute_catenoid(double size, double x) {

    std::vector<double> radii;
    std::vector<double> heights;
    for (int k = 0; k < 9; k++) {
        heights.push_back(WAIST * (-1.3 + 2.6 * k / 8.0));
        radii.push_back(WAIST * std::cosh(heights.back() / WAIST));
    }

    const double w = std::sqrt(0.5);
    const double arcs[5][2] = {{1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {-1.0, 1.0}, {-1.0, 0.0}};
    const double weights_v[5] = {1.0, w, 1.0, w, 1.0};
    std::vector<std::vector<Point>> points(5, std::vector<Point>(9, Point(0.0, 0.0, 0.0)));
    std::vector<std::vector<double>> weights(5, std::vector<double>(9, 1.0));
    for (int iv = 0; iv < 5; iv++)
        for (int iu = 0; iu < 9; iu++) {
            points[iv][iu] = Point(x + size / 2.0 + radii[iu] * arcs[iv][0], radii[iu] * arcs[iv][1], heights[iu]);
            weights[iv][iu] = weights_v[iv];
        }

    return NurbsSurface::create_from_parameters(points, weights, {0.0, 1.0 / 6.0, 2.0 / 6.0, 0.5, 4.0 / 6.0, 5.0 / 6.0, 1.0}, {0.0, 0.5, 1.0}, {4, 1, 1, 1, 1, 1, 4}, {3, 2, 3}, 3, 2);
}

/// Triangles over a grid of rows x columns vertices numbered row by row, each quad split on its diagonal, the last column joined to the first when closed; wound up the rows after along the columns.
std::vector<std::vector<size_t>> compute_grid(int rows, int columns, bool closed) {

    std::vector<std::vector<size_t>> faces;
    for (int r = 0; r + 1 < rows; r++)
        for (int c = 0; c + 1 < columns + (closed ? 1 : 0); c++) {
            const size_t a = r * columns + c;
            const size_t b = r * columns + (c + 1) % columns;
            faces.push_back({a, b, b + columns});
            faces.push_back({a, b + columns, a + columns});
        }

    return faces;
}

/// The points relaxed towards a minimal surface with the boundary fixed: every interior vertex moved to the cotangent-weighted mean of its neighbours, sweeps times.
std::vector<Point> compute_minimal(std::vector<Point> points, const std::vector<std::vector<size_t>>& faces) {

    std::map<std::pair<size_t, size_t>, int> uses;
    for (const std::vector<size_t>& face : faces)
        for (int k = 0; k < 3; k++)
            uses[{std::min(face[k], face[(k + 1) % 3]), std::max(face[k], face[(k + 1) % 3])}]++;

    std::vector<bool> fixed(points.size(), false);
    for (const std::pair<const std::pair<size_t, size_t>, int>& edge : uses)
        if (edge.second == 1) {
            fixed[edge.first.first] = true;
            fixed[edge.first.second] = true;
        }

    for (int sweep = 0; sweep < SWEEPS; sweep++) {
        std::vector<Vector> sums(points.size(), Vector(0.0, 0.0, 0.0));
        std::vector<double> weights(points.size(), 0.0);
        for (const std::vector<size_t>& face : faces)
            for (int k = 0; k < 3; k++) {
                const size_t i = face[(k + 1) % 3];
                const size_t j = face[(k + 2) % 3];
                const Vector a = points[i] - points[face[k]];
                const Vector b = points[j] - points[face[k]];
                const double cotangent = a.dot(b) / a.cross(b).magnitude() / 2.0;
                sums[i] = sums[i] + (points[j] - Point(0.0, 0.0, 0.0)) * cotangent;
                sums[j] = sums[j] + (points[i] - Point(0.0, 0.0, 0.0)) * cotangent;
                weights[i] += cotangent;
                weights[j] += cotangent;
            }

        for (size_t i = 0; i < points.size(); i++)
            if (!fixed[i])
                points[i] = Point(0.0, 0.0, 0.0) + sums[i] / weights[i];
    }

    return points;
}

/// A disk: the skew saddle's boundary over a square of side size from x on a 33 x 33 grid, the inside relaxed to a minimal surface from the saddle itself.
Mesh compute_saddle_mesh(double size, double x) {

    const int n = 33;
    std::vector<Point> points;
    for (int r = 0; r < n; r++)
        for (int c = 0; c < n; c++) {
            const double across = 2.0 * c / (n - 1) - 1.0;
            const double along = 2.0 * r / (n - 1) - 1.0;
            points.emplace_back(x + (across + 1.0) * size / 2.0, along * size / 2.0, compute_height(size, across, along));
        }

    const std::vector<std::vector<size_t>> faces = compute_grid(n, n, false);

    return Mesh::from_vertices_and_faces(compute_minimal(points, faces), faces);
}

/// An annulus: the catenoid r = c cosh(z / c) between two rings 1.3 c above and below its waist, 3.9 c tall, the rings size wide, from x.
Mesh compute_catenoid_mesh(double size, double x) {

    const int rows = 31;
    const int columns = 72;
    const double c = size / 2.0 / std::cosh(1.3);
    std::vector<Point> points;
    for (int r = 0; r < rows; r++)
        for (int k = 0; k < columns; k++) {
            const double w = 2.6 * r / (rows - 1) - 1.3;
            const double angle = 2.0 * Tolerance::PI * k / columns;
            points.emplace_back(x + size / 2.0 + c * std::cosh(w) * std::cos(angle), c * std::cosh(w) * std::sin(angle), c * w);
        }

    return Mesh::from_vertices_and_faces(points, compute_grid(rows, columns, true));
}

/// A disk: Enneper's surface over the parameter disk of radius 1.2, below its self-intersection, scaled to size wide from x on 24 rings of 72.
Mesh compute_enneper_mesh(double size, double x) {

    const int rings = 24;
    const int columns = 72;
    const double radius = 1.2;
    const double scale = size / 2.0 / (radius + radius * radius * radius / 3.0);
    std::vector<Point> points{Point(x + size / 2.0, 0.0, 0.0)};
    for (int r = 1; r <= rings; r++)
        for (int k = 0; k < columns; k++) {
            const double u = radius * r / rings * std::cos(2.0 * Tolerance::PI * k / columns);
            const double v = radius * r / rings * std::sin(2.0 * Tolerance::PI * k / columns);
            points.emplace_back(x + size / 2.0 + scale * (u - u * u * u / 3.0 + u * v * v), scale * (v - v * v * v / 3.0 + v * u * u), scale * (u * u - v * v));
        }

    std::vector<std::vector<size_t>> faces;
    for (int k = 0; k < columns; k++)
        faces.push_back({0, static_cast<size_t>(1 + k), static_cast<size_t>(1 + (k + 1) % columns)});

    for (const std::vector<size_t>& face : compute_grid(rings, columns, true)) {
        const std::vector<size_t> shifted{face[0] + 1, face[2] + 1, face[1] + 1};
        faces.push_back(shifted);
    }

    return Mesh::from_vertices_and_faces(points, faces);
}

// ═══════════════════════════════════════════════════════════════════════════
// Clash
// ═══════════════════════════════════════════════════════════════════════════


/// The carrier of a surface scene.
NurbsSurface compute_carrier(const Scene& scene, double offset) {

    if (scene.carrier == 0)
        return compute_saddle(scene.size, offset);

    if (scene.carrier == 1)
        return compute_paraboloid(scene.size, offset);

    return compute_catenoid(scene.size, offset);
}

/// The gridshell of a mesh scene: the catenoid as a rotational net from one traced curve about its vertical axis, the disks from both families seeded along seed lines through the mesh point nearest the middle; the carrier's largest mean curvature share is written into minimal.
wood_gridshell::Gridshell compute_mesh_gridshell(const Scene& scene, double offset, double& minimal) {

    const Mesh mesh = scene.carrier == 3 ? compute_saddle_mesh(scene.size, offset) : scene.carrier == 4 ? compute_catenoid_mesh(scene.size, offset) : compute_enneper_mesh(scene.size, offset);
    const wood_gridshell::Carrier carrier(mesh);
    minimal = carrier.compute_mean_curvature();
    const Point centre(offset + scene.size / 2.0, 0.0, 0.0);
    if (scene.carrier == 4) {
        const Point seed = carrier.compute_point(carrier.compute_foot(Point(offset + scene.size / 2.0 + scene.size / 4.0, 0.0, 0.0)));
        return wood_gridshell::compute_rotational_gridshell(carrier, seed, centre, Vector(0.0, 0.0, 1.0), scene.count, LAMELLA, ROUNDS);
    }

    const std::pair<Point, Point> top = wood_gridshell::compute_mesh_seed_line(carrier, centre, true);
    const std::pair<Point, Point> bottom = wood_gridshell::compute_mesh_seed_line(carrier, centre, false);

    return wood_gridshell::compute_traced_gridshell(carrier, wood_gridshell::compute_mesh_seeds(carrier, top.first, top.second, scene.count), wood_gridshell::compute_mesh_seeds(carrier, bottom.first, bottom.second, scene.count), centre, LAMELLA, ROUNDS);
}

/// The gridshell of a surface scene: both families seeded along their seed lines through the middle of the domain, where they are told apart, or both along the parameter line u = edge, told apart at the first seed.
wood_gridshell::Gridshell compute_gridshell(const Scene& scene, double offset) {

    const NurbsSurface surface = compute_carrier(scene, offset);
    if (scene.edge < 0.0) {
        const Vector centre = wood_gridshell::compute_centre(surface);
        const std::pair<Vector, Vector> top = wood_gridshell::compute_seed_line(surface, scene.path, true, centre);
        const std::pair<Vector, Vector> bottom = wood_gridshell::compute_seed_line(surface, scene.path, false, centre);
        return wood_gridshell::Gridshell::from_surface(surface, scene.path, wood_gridshell::compute_seeds(top.first, top.second, scene.count), wood_gridshell::compute_seeds(bottom.first, bottom.second, scene.count), LAMELLA);
    }

    const wood_gridshell::Boundary boundary = wood_gridshell::Boundary::of(surface);
    const double u = boundary.u0 + (boundary.u1 - boundary.u0) * scene.edge;
    const std::vector<Vector> seeds = wood_gridshell::compute_seeds(Vector(u, boundary.v0, 0.0), Vector(u, boundary.v1, 0.0), scene.count);

    return wood_gridshell::Gridshell::from_surface(surface, scene.path, seeds, seeds, LAMELLA, seeds.front());
}

// ═══════════════════════════════════════════════════════════════════════════
// Clash
// ═══════════════════════════════════════════════════════════════════════════

/// The face planes of a closed mesh, normals out.
std::vector<Plane> compute_planes(const Mesh& mesh) {

    const Point centre = mesh.centroid();
    std::vector<Plane> planes;
    for (const size_t face : mesh.faces()) {
        const std::vector<Point> points = mesh.face_polygon(face)->get_points();
        const Vector normal = compute_newell(points).normalized();
        const Point origin = Point::centroid(points);
        planes.push_back(Plane::from_point_normal(origin, (origin - centre).dot(normal) < 0.0 ? -normal : normal));
    }

    return planes;
}

/// Volume of other inside a convex mesh, its face planes pulled in by the tolerance so touching faces count for nothing.
double compute_overlap(const Mesh& convex, const Mesh& other) {

    Mesh part = other;
    for (const Plane& plane : compute_planes(convex)) {
        part = part.cut_by_plane(Plane::from_point_normal(plane.origin() - plane.z_axis() * CLEARANCE, -plane.z_axis()));
        if (part.faces().empty())
            return 0.0;
    }

    return std::abs(part.volume());
}

/// The lowest and highest corner of a mesh.
std::pair<Point, Point> compute_box(const Mesh& mesh) {

    std::pair<Point, Point> box(Point(1e18, 1e18, 1e18), Point(-1e18, -1e18, -1e18));
    for (const size_t vertex : mesh.vertices())
        for (int axis = 0; axis < 3; axis++) {
            box.first[axis] = std::min(box.first[axis], (*mesh.vertex_point(vertex))[axis]);
            box.second[axis] = std::max(box.second[axis], (*mesh.vertex_point(vertex))[axis]);
        }

    return box;
}

/// True when two boxes overlap by more than the tolerance on every axis.
bool is_near(const std::pair<Point, Point>& a, const std::pair<Point, Point>& b) {

    for (int axis = 0; axis < 3; axis++)
        if (a.second[axis] <= b.first[axis] + CLEARANCE || b.second[axis] <= a.first[axis] + CLEARANCE)
            return false;

    return true;
}


/// The four boards a stud holds, as positions in the top boards followed by the bottom boards.
std::vector<size_t> compute_held(const wood_gridshell::Gridshell& gridshell, size_t stud) {

    const size_t top = gridshell.pairs[stud].first;
    const size_t bottom = gridshell.pairs[stud].second;

    return {2 * top, 2 * top + 1, gridshell.top.size() + 2 * bottom, gridshell.top.size() + 2 * bottom + 1};
}

/// True when one of two element positions is a stud and the other a board it holds; boards come first, then the studs.
bool is_held(const wood_gridshell::Gridshell& gridshell, size_t a, size_t b) {

    const size_t boards = gridshell.top.size() + gridshell.bottom.size();
    if ((a < boards) == (b < boards))
        return false;

    const std::vector<size_t> held = compute_held(gridshell, std::max(a, b) - boards);

    return std::find(held.begin(), held.end(), std::min(a, b)) != held.end();
}

/// The largest overlap volume over every two convex pieces of different elements whose boxes overlap, a stud and the four boards it holds left to compute_fit: a board cut into one loft per pair of neighbouring rings, a stud whole.
double compute_clash(const wood_gridshell::Gridshell& gridshell) {

    std::vector<Piece> pieces;
    std::vector<std::shared_ptr<BeamCurved>> boards = gridshell.top;
    boards.insert(boards.end(), gridshell.bottom.begin(), gridshell.bottom.end());
    for (size_t i = 0; i < boards.size(); i++) {
        const std::vector<Polyline> rings = boards[i]->sections();
        for (size_t k = 0; k + 1 < rings.size(); k++) {
            const Mesh mesh = Mesh::loft({rings[k]}, {rings[k + 1]}, true);
            pieces.push_back(Piece{i, mesh, compute_box(mesh)});
        }
    }

    for (size_t i = 0; i < gridshell.studs.size(); i++)
        pieces.push_back(Piece{boards.size() + i, gridshell.studs[i]->model_geometry_mesh(), compute_box(gridshell.studs[i]->model_geometry_mesh())});

    double worst = 0.0;
    for (size_t i = 0; i < pieces.size(); i++)
        for (size_t j = i + 1; j < pieces.size(); j++)
            if (pieces[i].element != pieces[j].element && is_near(pieces[i].box, pieces[j].box) && !is_held(gridshell, pieces[i].element, pieces[j].element))
                worst = std::max(worst, compute_overlap(pieces[i].mesh, pieces[j].mesh));

    return worst;
}

// ═══════════════════════════════════════════════════════════════════════════
// Checks
// ═══════════════════════════════════════════════════════════════════════════


// ═══════════════════════════════════════════════════════════════════════════
// Checks
// ═══════════════════════════════════════════════════════════════════════════

/// Largest distance in mm of an unrolled lamella centreline from the line through its ends: every board laid flat by its turns about the normal plane.
double compute_deviation(const wood_gridshell::Gridshell& gridshell) {

    double worst = 0.0;
    for (const std::vector<Plane>& frames : gridshell.frames) {
        std::vector<Point> flat{Point(0.0, 0.0, 0.0)};
        double heading = 0.0;
        for (size_t k = 0; k + 1 < frames.size(); k++) {
            const Vector after = frames[k + 1].origin() - frames[k].origin();
            if (k > 0)
                heading += std::asin(std::clamp((after.normalized() - (frames[k].origin() - frames[k - 1].origin()).normalized()).dot(frames[k].z_axis()), -1.0, 1.0));

            flat.push_back(flat.back() + Vector(std::cos(heading), std::sin(heading), 0.0) * after.magnitude());
        }

        const Vector chord = (flat.back() - flat.front()).normalized();
        for (const Point& point : flat)
            worst = std::max(worst, (point - flat.front()).cross(chord).magnitude());
    }

    return worst;
}

/// Largest angle in degrees between the ruling of a board face halfway between two stations and the station edges it spans, the two edges averaged: how far the BRep's ruled faces twist off the sections; and the largest distance in mm of a rail at a station parameter from its section corner there.
std::pair<double, double> compute_tilt(const wood_gridshell::Gridshell& gridshell) {

    double worst = 0.0;
    double error = 0.0;
    for (const std::vector<std::shared_ptr<BeamCurved>>& layer : {gridshell.top, gridshell.bottom})
        for (const std::shared_ptr<BeamCurved>& board : layer) {
            const std::vector<Polyline> rings = board->sections();
            const std::vector<NurbsCurve> rails = board->rails();
            const std::vector<double>& parameters = board->parameters;
            for (size_t i = 0; i < rails.size(); i++) {
                const size_t j = (i + 1) % rails.size();
                for (size_t k = 0; k < rings.size(); k++)
                    error = std::max(error, (rails[i].point_at(parameters[k]) - rings[k][i]).magnitude());

                for (size_t k = 0; k + 1 < rings.size(); k++) {
                    const double middle = (parameters[k] + parameters[k + 1]) / 2.0;
                    const Vector ruling = rails[j].point_at(middle) - rails[i].point_at(middle);
                    const Vector edge = (rings[k][j] - rings[k][i]) + (rings[k + 1][j] - rings[k + 1][i]);
                    worst = std::max(worst, std::acos(std::clamp(ruling.normalized().dot(edge.normalized()), -1.0, 1.0)) * 180.0 / Tolerance::PI);
                }
            }
        }

    return {worst, error};
}

/// The flats of a stud: one plane per side of its hexagon along its axis, normal out.
std::vector<Plane> compute_flats(const Column& stud) {

    const Vector along = stud.axis.to_vector();
    const std::vector<Point> corners = stud.section.get_points();
    const Point middle = Point::centroid(std::vector<Point>(corners.begin(), corners.end() - 1));
    std::vector<Plane> flats;
    for (size_t k = 0; k + 1 < corners.size(); k++) {
        Vector normal = (corners[k + 1] - corners[k]).cross(along).normalized();
        if (normal.dot(corners[k] - middle) < 0.0)
            normal = -normal;

        flats.push_back(Plane::from_point_normal(corners[k], normal));
    }

    return flats;
}

/// The depth of p inside the prism the flats bound, positive inside: its least signed distance behind the flats.
double compute_depth(const std::vector<Plane>& flats, const Point& p) {

    double depth = 1e300;
    for (const Plane& flat : flats)
        depth = std::min(depth, -(p - flat.origin()).dot(flat.z_axis()));

    return depth;
}

/// The board section nearest the node, the station its ruling through the node stands on.
Polyline compute_node_ring(const BeamCurved& board, const std::vector<Polyline>& rings, const Point& node) {

    size_t nearest = 0;
    double least = 1e300;
    for (size_t k = 0; k < board.parameters.size(); k++) {
        const double distance = (board.axis.point_at(board.parameters[k]) - node).magnitude();
        if (distance < least) {
            least = distance;
            nearest = k;
        }
    }

    return rings[nearest];
}

/// The parameter of the curve nearest to p: the best of 2000 samples refined by bisection on the neighbouring samples.
double compute_nearest(const NurbsCurve& curve, const Point& p) {

    const double t0 = curve.domain().first;
    const double dt = (curve.domain().second - t0) / 2000.0;
    double best = t0;
    double least = 1e300;
    for (int k = 0; k <= 2000; k++) {
        const double distance = (curve.point_at(t0 + k * dt) - p).magnitude();
        if (distance < least) {
            least = distance;
            best = t0 + k * dt;
        }
    }

    double lo = std::max(best - dt, t0);
    double hi = std::min(best + dt, curve.domain().second);
    for (int k = 0; k < 40; k++) {
        const double a = lo + (hi - lo) / 3.0;
        const double b = hi - (hi - lo) / 3.0;
        if ((curve.point_at(a) - p).magnitude() < (curve.point_at(b) - p).magnitude())
            hi = b;
        else
            lo = a;
    }

    return (lo + hi) / 2.0;
}

/// How each stud meets the four boards it holds, in mm: the largest distance from a stud flat to the nearest corner of each board's section at the node, where the construction puts the ruling through the node in the flat; the largest penetration of a board rail anywhere along the stud, the twist of the strip against the straight node axis (Schling et al. 2022, Sec. 3.4); and the largest gap, each board's least distance outside the stud. The rails are sampled every 2 mm for 150 mm either side of the node.
std::array<double, 3> compute_fit(const wood_gridshell::Gridshell& gridshell) {

    std::vector<std::shared_ptr<BeamCurved>> boards = gridshell.top;
    boards.insert(boards.end(), gridshell.bottom.begin(), gridshell.bottom.end());
    std::vector<std::vector<NurbsCurve>> rails;
    std::vector<std::vector<Polyline>> rings;
    for (const std::shared_ptr<BeamCurved>& board : boards) {
        rails.push_back(board->rails());
        rings.push_back(board->sections());
    }

    double section = 0.0;
    double penetration = 0.0;
    double gap = 0.0;
    for (size_t s = 0; s < gridshell.studs.size(); s++) {
        const Column& stud = *gridshell.studs[s];
        const Point node = stud.axis.point_at(0.5);
        const std::vector<Plane> flats = compute_flats(stud);
        for (const size_t b : compute_held(gridshell, s)) {
            double touch = 1e300;
            for (const Point& corner : compute_node_ring(*boards[b], rings[b], node).get_points())
                touch = std::min(touch, std::abs(compute_depth(flats, corner)));

            section = std::max(section, touch);
            double nearest = 1e300;
            for (const NurbsCurve& rail : rails[b]) {
                const double t = compute_nearest(rail, node);
                const double h = (rail.domain().second - rail.domain().first) * 1e-4;
                const double speed = (rail.point_at(std::min(t + h, rail.domain().second)) - rail.point_at(std::max(t - h, rail.domain().first))).magnitude() / (2.0 * h);
                for (int k = -75; k <= 75; k++) {
                    const Point p = rail.point_at(std::clamp(t + k * 2.0 / speed, rail.domain().first, rail.domain().second));
                    const double axial = (p - stud.axis.start()).dot(stud.axis.to_vector()) / stud.axis.to_vector().dot(stud.axis.to_vector());
                    if (axial < 0.0 || axial > 1.0)
                        continue;

                    const double depth = compute_depth(flats, p);
                    penetration = std::max(penetration, depth);
                    nearest = std::min(nearest, -depth);
                }
            }

            gap = std::max(gap, std::max(nearest, 0.0));
        }
    }

    return {section, penetration, gap};
}

/// Largest twist of the lamellas in degrees per metre: the turn of the normal about the tangent from one station to the next.
double compute_twist(const wood_gridshell::Gridshell& gridshell) {

    double worst = 0.0;
    for (const std::vector<Plane>& frames : gridshell.frames)
        for (size_t k = 1; k + 2 < frames.size(); k++) {
            const Vector chord = frames[k + 1].origin() - frames[k].origin();
            const Vector tangent = chord.normalized();
            const Vector a = (frames[k].z_axis() - tangent * frames[k].z_axis().dot(tangent)).normalized();
            const Vector b = (frames[k + 1].z_axis() - tangent * frames[k + 1].z_axis().dot(tangent)).normalized();
            worst = std::max(worst, std::acos(std::clamp(a.dot(b), -1.0, 1.0)) / chord.magnitude());
        }

    return worst * 180.0 / Tolerance::PI * 1000.0;
}

/// True when every board's BRep is one valid closed solid of six faces.
bool is_smooth(const wood_gridshell::Gridshell& gridshell) {

    bool smooth = true;
    for (const std::vector<std::shared_ptr<BeamCurved>>& layer : {gridshell.top, gridshell.bottom})
        for (const std::shared_ptr<BeamCurved>& board : layer) {
            const BRep& brep = board->element_geometry_brep();
            smooth = smooth && brep.is_valid() && brep.is_solid() && brep.face_count() == 6;
        }

    return smooth;
}


/// The least and largest normal curvature, then the largest geodesic curvature and geodesic torsion by magnitude, over every lamella of the gridshell, in 1/m, SAMPLES per lamella.
std::array<double, 4> compute_curvatures(const wood_gridshell::Gridshell& gridshell) {

    std::array<double, 4> worst{1e300, -1e300, 0.0, 0.0};
    for (const NurbsCurve& curve : gridshell.curves) {
        if (!curve.is_valid())
            continue;

        for (int k = 0; k <= SAMPLES; k++) {
            const double t = curve.domain().first + (curve.domain().second - curve.domain().first) * k / SAMPLES;
            const std::array<double, 3> metrics = wood_gridshell::compute_metrics(gridshell.surface, curve, t);
            worst[0] = std::min(worst[0], metrics[0] * 1000.0);
            worst[1] = std::max(worst[1], metrics[0] * 1000.0);
            worst[2] = std::max(worst[2], std::abs(metrics[1]) * 1000.0);
            worst[3] = std::max(worst[3], std::abs(metrics[2]) * 1000.0);
        }
    }

    return worst;
}

int main() {

    WoodSession wood_session("templates_gridshell");
    std::vector<wood_gridshell::Gridshell> gridshells;
    double offset = 0.0;

    std::vector<double> minimals;
    for (const Scene& scene : SCENES) {
        double minimal = -1.0;
        gridshells.push_back(scene.carrier < 3 ? compute_gridshell(scene, offset) : compute_mesh_gridshell(scene, offset, minimal));
        minimals.push_back(minimal);
        offset += scene.size + GAP;

        const std::shared_ptr<TreeNode> top = wood_session.add_group(scene.name + "_top");
        for (const std::shared_ptr<BeamCurved>& board : gridshells.back().top)
            wood_session.add(board, top);

        const std::shared_ptr<TreeNode> bottom = wood_session.add_group(scene.name + "_bottom");
        for (const std::shared_ptr<BeamCurved>& board : gridshells.back().bottom)
            wood_session.add(board, bottom);

        const std::shared_ptr<TreeNode> studs = wood_session.add_group(scene.name + "_studs");
        for (const std::shared_ptr<Column>& stud : gridshells.back().studs)
            wood_session.add(stud, studs);
    }

    wood_session.compute_contacts();

    bool passed = true;
    for (size_t i = 0; i < SCENES.size(); i++) {
        size_t count = 0;
        for (const std::shared_ptr<Column>& stud : gridshells[i].studs)
            count += wood_session.get_neighbours(stud->guid()).size() == 4 ? 1 : 0;

        const double clash = compute_clash(gridshells[i]);
        const std::array<double, 3> fit = compute_fit(gridshells[i]);
        const std::array<double, 4> curvatures = compute_curvatures(gridshells[i]);
        const bool smooth = is_smooth(gridshells[i]);
        const bool mesh = SCENES[i].carrier >= 3;
        const bool straight = mesh ? gridshells[i].residual <= RESIDUAL : SCENES[i].path.iso || std::max(std::abs(curvatures[0] - SCENES[i].path.value * 1000.0), std::abs(curvatures[1] - SCENES[i].path.value * 1000.0)) <= STRAIGHT;
        passed = passed && fit[0] <= FIT && fit[2] <= FIT && clash <= CLEARANCE && smooth && straight && !gridshells[i].studs.empty();
        if (mesh)
            std::cout << fmt::format("{}: mean curvature share {:.4f}, net residual {:.2e} traced, {:.2e} optimised and refined to {} vertices in {} rounds (constraints {:.2e} on the unit net), ", SCENES[i].name, minimals[i], gridshells[i].traced, gridshells[i].optimised, gridshells[i].vertices, gridshells[i].rounds, gridshells[i].residual);
        else
            std::cout << SCENES[i].name << ": ";

        if (!mesh)
            std::cout << fmt::format("normal curvature {:.2e} to {:.2e} 1/m, geodesic curvature {:.3f} 1/m, geodesic torsion {:.3f} 1/m, ", curvatures[0], curvatures[1], curvatures[2], curvatures[3]);

        std::cout << fmt::format("{} {} boards, {} nodes, rulings lean up to {:.1f} deg from the normal ({} stations past 45 deg regularised, the normal within 50 mm of a node), twist up to {:.1f} deg/m, unrolled deviation {:.3f} mm, face tilt {:.4f} deg (rails off their corners by {:.2e} mm), {} studs fit their boards within {:.4f} mm at the node (gap {:.3f} mm, twist mismatch at the stud ends {:.3f} mm), {} with kernel face contacts to all four, largest overlap {} mm3\n", gridshells[i].top.size() + gridshells[i].bottom.size(), smooth ? "BRep" : "BROKEN", gridshells[i].nodes.size(), gridshells[i].lean, gridshells[i].capped, compute_twist(gridshells[i]), compute_deviation(gridshells[i]), compute_tilt(gridshells[i]).first, compute_tilt(gridshells[i]).second, gridshells[i].studs.size(), fit[0], fit[2], fit[1], count, clash);
    }

    std::cout << fmt::format("{} contacts\n", wood_session.get_contacts().size());

    for (const wood_gridshell::Gridshell& gridshell : gridshells) {
        for (const std::vector<std::shared_ptr<BeamCurved>>& layer : {gridshell.top, gridshell.bottom})
            for (const std::shared_ptr<BeamCurved>& board : layer)
                board->compute_geometry_brep();

        for (const std::shared_ptr<Column>& stud : gridshell.studs)
            stud->compute_geometry_brep();
    }

    wood_session.set_features_visible("section", false);
    wood_session.set_features_visible("axis", false);
    wood_session.pb_dump(pb_path("live"));

    const WoodSession loaded = WoodSession::pb_load(pb_path("live"));
    size_t curved = 0;
    size_t reloaded = 0;
    for (const std::shared_ptr<Element>& element : loaded.elements()) {
        const std::shared_ptr<BeamCurved> beam = std::dynamic_pointer_cast<BeamCurved>(element);
        curved += beam ? 1 : 0;
        reloaded += beam && beam->element_geometry_brep().is_solid() && beam->sections().size() == beam->parameters.size() ? 1 : 0;
    }

    std::cout << fmt::format("{} of {} BeamCurved reloaded from the file as BRep solids\n", reloaded, curved);

    return passed && curved > 0 && reloaded == curved ? 0 : 1;
}

/*
|||||||| DESCRIPTION ||||||||
The lamella gridshell template eight times in a row. Five NURBS surfaces: a 10 m cubic saddle on its asymptotic curves, a 6 m saddle on its iso-curves, an 8 m hyperbolic paraboloid (straight asymptotic lines), a half catenoid fitted as a rational NURBS with both families seeded along its rim, and the saddle on curves of constant normal curvature 0.02 1/m. Each lamella is a curve on the surface traced as in Bowerbird (Oberbichler): fourth-order Runge-Kutta steps in the parameter plane along the direction of the wanted normal curvature (Euler's formula from the principal curvatures) that lies closest to the last step, both ways from a seed until the domain boundary; the traced parameters become a cubic curve in the parameter plane, evaluated on the surface, and the nodes are the crossings of the two families refined by Newton. Then three minimal meshes as discrete A-nets after Schling, Wang, Hoyer, Pottmann (CAD 2022): a 12 m disk relaxed to a minimal surface, a catenoid annulus as a rotational net from one traced curve turned and mirrored around its axis, and a 16 m Enneper disk; the crossing net of the traced families is optimised by guided projection (planar vertex stars with one unit normal per node, fairness, proximity to the mesh, damping) to a residual of 1e-8 or better, refined once by bilinear subdivision and optimised again. Everywhere a lamella is two upright boards a gap apart, continuous through every node: a BeamCurved swept through exact stations every 50 mm plus one at every node, its section carried along the ruling of the lamella's rectifying developable, tg t + kg n in the Darboux frame (Schling Sec. 2.2), regularised where the geodesic curvature vanishes and held to the straight node axis within a spacer block of every node, so the strip unrolls straight except where the node forces it (Sec. 3.4); one closed BRep with four rails interpolated at the station parameters, ruled faces and planar caps. A hexagonal stud runs along the exact surface normal, on a mesh the shared A-net normal, through both gaps at every crossing, its flats against the four boards along their rulings. Per scene the example prints Bowerbird's curve-on-surface measures (normal curvature range, geodesic curvature and torsion), the ruling lean, the twist, the unrolled straightness, the face tilt and rail error, the stud fit at the node and the twist of the strips against the straight studs, the overlap, and for a mesh the mean curvature share, the net residual traced, optimised and refined and the rounds taken. It fails unless every board is a valid six-face BRep solid, every lamella keeps its path's normal curvature within 5e-3 1/m, every net's constraint residual is at most 1e-8, every stud fits its four boards within 0.1 mm at the node, no overlap is larger than the tolerance and every board loads back from the file as a BeamCurved.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_gridshell --parallel 4 && ./build/templates_gridshell && ../bash/publish-scene.sh --target templates_gridshell

|||||||| WORKFLOW ||||||||
examples/templates_gridshell.cpp
 |
 |-- compute_saddle / compute_paraboloid / compute_catenoid          NURBS carriers: exact bicubic Bezier patches, a rational surface of revolution
 |-- compute_saddle_mesh / compute_catenoid_mesh / compute_enneper_mesh   minimal meshes
 |-- compute_seed_line, compute_seeds                                 seeds of each family along a line across it
 |
 |-- Gridshell::from_surface(surface, path, seeds_top, seeds_bottom, lamella)  src/templates/shells/lamella_gridshell.h
 |    |-- compute_curvature, compute_directions: principal curvatures, iso or normal-curvature directions (Bowerbird PrincipalCurvature, FindNormalCurvature)
 |    |-- compute_trace: Runge-Kutta 4 through the parameter plane both ways from the seed, clipped at the boundary (Pathfinder)
 |    |-- compute_curve, compute_crossings: the cubic through the traced parameters, Newton onto the curves at the crossings
 |    |-- compute_darboux, compute_ruling: Darboux frame and invariants, the rectifying ruling regularised and blended to the node axis
 |    |-- compute_board: two BeamCurved per lamella through its stations along the rulings
 |    '-- compute_stud: a Column per node on the surface normal, a hexagon of three flat pairs gap apart
 |
 |-- compute_traced_gridshell / compute_rotational_gridshell         src/templates/shells/lamella_gridshell_net.h
 |    |-- Carrier: vertex normals and shape operators, closest point, asymptotic directions of a mesh
 |    |-- compute_mesh_trace, compute_rotational: curves on the mesh, a rotational net from one curve
 |    |-- compute_net: the crossing net, one vertex and normal per crossing
 |    |-- compute_optimised: guided projection (Schling Eq. 14) by Gauss-Newton and conjugate gradients
 |    |-- compute_refined: bilinear subdivision, optimised again
 |    '-- compute_net_gridshell: boards through the refined lamella lines, studs on the shared normals
 |
 |-- WoodSession, add_group(<scene>_top, _bottom, _studs), add(element, group)
 |-- compute_contacts()                        face contacts, a stud against each of its four boards
 |-- compute_geometry_brep()                   every board and stud written as its BRep
 |-- set_features_visible("section" and "axis")   the section rings and the shared centreline of the boards off in the viewer
 |-- pb_dump(pb_path("live"))                  data/output/pb/live.pb, the file the viewer watches
 '-- WoodSession::pb_load                      every board back as a BeamCurved with its BRep

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
