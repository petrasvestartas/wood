#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/shells/lamella_gridshell.h"

using namespace session_cpp;
using namespace wood_session;

/// One gridshell of the row: its group name, carrier, plan side and lamellas per family.
struct Scene {
    std::string name; // Group name prefix.
    int carrier; // 0 surface iso-curves, 1 surface asymptotic curves, 2 minimal saddle mesh, 3 catenoid mesh, 4 Enneper mesh.
    double size; // Plan side of the carrier.
    int count; // Lamellas per family.
};

const std::vector<Scene> SCENES = {
    {"asymptotic", 1, 10000.0, 9},
    {"iso", 0, 6000.0, 5},
    {"saddle_mesh", 2, 20000.0, 12},
    {"catenoid_mesh", 3, 20000.0, 14},
    {"enneper_mesh", 4, 30000.0, 14},
};
const double RISE = 0.15; // corner lift and drop of a saddle over its side
const double SKEW = 0.5; // how much faster the saddle curves at one end, so the asymptotic curves bend in plan
const double GAP = 4000.0; // clear distance between the shells
const wood_gridshell::Lamella LAMELLA{.height = 140.0, .thickness = 20.0, .gap = 60.0, .spacing = 180.0, .overrun = 20.0, .step = 100.0};
const double CLEARANCE = 1.0; // faces closer than this count as touching, not overlapping
const double FIT = 1.0; // largest gap or penetration in mm between a stud flat and the twisting board it holds
const int SWEEPS = 400; // cotangent Laplacian sweeps that relax the saddle mesh to a minimal one

/// A convex part of an element with its box, what the clash check cuts.
struct Piece {
    size_t element; // Position of the element in the gridshell.
    Mesh mesh; // Closed convex solid.
    std::pair<Point, Point> box; // Lowest and highest corner.
};

// ═══════════════════════════════════════════════════════════════════════════
// Carriers
// ═══════════════════════════════════════════════════════════════════════════

/// The height of the skew saddle over across and along in [-1, 1], z = f(x) - y^2 with f convex, so the Gaussian curvature is negative everywhere.
double compute_height(double size, double across, double along) {
    return RISE * size / 2.0 * (across * across * (1.0 + SKEW * across) - along * along);
}

/// A cubic saddle over a square of side size from x; u along y and v along x so the normal points up.
NurbsSurface compute_saddle(double size, double x) {

    std::vector<Point> points;
    for (int u = 0; u < 4; u++)
        for (int v = 0; v < 4; v++)
            points.emplace_back(x + v / 1.5 * size / 2.0, (u / 1.5 - 1.0) * size / 2.0, compute_height(size, v / 1.5 - 1.0, u / 1.5 - 1.0));

    return NurbsSurface::create(false, false, 3, 3, 4, 4, points);
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

    const size_t top = gridshell.nodes[stud].first;
    const size_t bottom = gridshell.nodes[stud].second;

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

/// Largest normal curvature along the lamella centrelines in 1/mm: each station's turn against its normal.
double compute_bending(const wood_gridshell::Gridshell& gridshell) {

    double worst = 0.0;
    for (const std::vector<Plane>& frames : gridshell.frames)
        for (size_t k = 1; k + 1 < frames.size(); k++) {
            const Vector before = frames[k].origin() - frames[k - 1].origin();
            const Vector after = frames[k + 1].origin() - frames[k].origin();
            const double turn = (after.normalized() - before.normalized()).dot(frames[k].z_axis());
            worst = std::max(worst, std::abs(turn) * 2.0 / (before.magnitude() + after.magnitude()));
        }

    return worst;
}

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

/// The parameter of every section corner on its rail, chord length along the corners mapped onto the rail's domain.
std::vector<double> compute_parameters(const std::vector<Polyline>& rings, const NurbsCurve& rail, size_t corner) {

    std::vector<double> lengths{0.0};
    for (size_t k = 0; k + 1 < rings.size(); k++)
        lengths.push_back(lengths.back() + (rings[k + 1][corner] - rings[k][corner]).magnitude());

    std::vector<double> parameters;
    for (const double length : lengths)
        parameters.push_back(rail.domain().first + (rail.domain().second - rail.domain().first) * length / lengths.back());

    return parameters;
}

/// Largest angle in degrees between the ruling of a board face halfway between two sections and the section edge it spans there, the two sections' edges averaged: how far a face twists off its sections.
double compute_tilt(const wood_gridshell::Gridshell& gridshell) {

    double worst = 0.0;
    for (const std::vector<std::shared_ptr<BeamCurved>>& layer : {gridshell.top, gridshell.bottom})
        for (const std::shared_ptr<BeamCurved>& board : layer) {
            const std::vector<Polyline> rings = board->sections();
            const std::vector<NurbsCurve> rails = board->rails();
            for (size_t i = 0; i < rails.size(); i++) {
                const size_t j = (i + 1) % rails.size();
                const std::vector<double> first = compute_parameters(rings, rails[i], i);
                const std::vector<double> second = compute_parameters(rings, rails[j], j);
                for (size_t k = 0; k + 1 < rings.size(); k++) {
                    const Vector ruling = rails[j].point_at((second[k] + second[k + 1]) / 2.0) - rails[i].point_at((first[k] + first[k + 1]) / 2.0);
                    const Vector edge = (rings[k][j] - rings[k][i]) + (rings[k + 1][j] - rings[k + 1][i]);
                    worst = std::max(worst, std::acos(std::clamp(ruling.normalized().dot(edge.normalized()), -1.0, 1.0)) * 180.0 / Tolerance::PI);
                }
            }
        }

    return worst;
}

/// Largest penetration and largest gap in mm between each stud and the four boards it holds: the board rails sampled every 2 mm for 150 mm either side of the node, each point's depth inside the stud prism, positive inside; the gap of a board is its least distance outside.
std::pair<double, double> compute_fit(const wood_gridshell::Gridshell& gridshell) {

    std::vector<std::shared_ptr<BeamCurved>> boards = gridshell.top;
    boards.insert(boards.end(), gridshell.bottom.begin(), gridshell.bottom.end());
    std::vector<std::vector<NurbsCurve>> rails;
    for (const std::shared_ptr<BeamCurved>& board : boards)
        rails.push_back(board->rails());

    double penetration = 0.0;
    double gap = 0.0;
    for (size_t s = 0; s < gridshell.studs.size(); s++) {
        const Column& stud = *gridshell.studs[s];
        const Point base = stud.axis.start();
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

        for (const size_t b : compute_held(gridshell, s)) {
            double nearest = 1e300;
            for (const NurbsCurve& rail : rails[b]) {
                const double t = rail.closest_parameter(stud.axis.point_at(0.5));
                const double h = (rail.domain().second - rail.domain().first) * 1e-4;
                const double speed = (rail.point_at(std::min(t + h, rail.domain().second)) - rail.point_at(std::max(t - h, rail.domain().first))).magnitude() / (2.0 * h);
                for (int k = -75; k <= 75; k++) {
                    const double u = std::clamp(t + k * 2.0 / speed, rail.domain().first, rail.domain().second);
                    const Point p = rail.point_at(u);
                    const double axial = (p - base).dot(along) / along.dot(along);
                    if (axial < 0.0 || axial > 1.0)
                        continue;

                    double depth = 1e300;
                    for (const Plane& flat : flats)
                        depth = std::min(depth, -(p - flat.origin()).dot(flat.z_axis()));

                    penetration = std::max(penetration, depth);
                    nearest = std::min(nearest, -depth);
                }
            }

            gap = std::max(gap, std::max(nearest, 0.0));
        }
    }

    return {penetration, gap};
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

int main() {

    WoodSession wood_session("templates_gridshell");
    std::vector<wood_gridshell::Gridshell> gridshells;
    std::vector<double> residuals;
    double offset = 0.0;

    for (const Scene& scene : SCENES) {
        if (scene.carrier < 2) {
            gridshells.push_back(wood_gridshell::Gridshell::from_surface(compute_saddle(scene.size, offset), scene.carrier, scene.count, scene.count, LAMELLA));
            residuals.push_back(-1.0);
        } else {
            const Mesh mesh = scene.carrier == 2 ? compute_saddle_mesh(scene.size, offset) : scene.carrier == 3 ? compute_catenoid_mesh(scene.size, offset) : compute_enneper_mesh(scene.size, offset);
            gridshells.push_back(wood_gridshell::Gridshell::from_mesh(mesh, scene.count, scene.count, LAMELLA));
            residuals.push_back(wood_gridshell::MeshField(mesh).compute_mean_curvature());
        }

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
        const std::pair<double, double> fit = compute_fit(gridshells[i]);
        const bool smooth = is_smooth(gridshells[i]);
        passed = passed && fit.first <= FIT && fit.second <= FIT && clash <= CLEARANCE && smooth;
        const std::string residual = residuals[i] < 0.0 ? "" : fmt::format(", mean curvature share {:.4f}", residuals[i]);
        std::cout << fmt::format("{}: net asymptotic residual {:.1e} traced, {:.1e} optimised, {} {} boards{}, twist up to {:.1f} deg/m, normal curvature {:.2e} 1/mm, unrolled deviation {:.3f} mm, face tilt {:.4f} deg, {} studs fit their boards within {:.3f} mm (gap {:.3f} mm), {} with kernel face contacts to all four, largest overlap {} mm3\n", SCENES[i].name, gridshells[i].traced, wood_gridshell::compute_residual(gridshells[i].net), gridshells[i].top.size() + gridshells[i].bottom.size(), smooth ? "BRep" : "BROKEN", residual, compute_twist(gridshells[i]), compute_bending(gridshells[i]), compute_deviation(gridshells[i]), compute_tilt(gridshells[i]), gridshells[i].studs.size(), fit.first, fit.second, count, clash);
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
The lamella gridshell template five times in a row: a 10 m saddle surface on its asymptotic curves, a 6 m saddle surface on its iso-curves, then three minimal meshes on their asymptotic curves, one per topology - a 10 m disk relaxed to a minimal surface inside the skew saddle's boundary by cotangent Laplacian sweeps, a catenoid annulus between two 6 m rings and a 9 m Enneper disk. The first family of curves is the top layer a layer up the normal, the second the bottom layer a layer down, each lamella two upright boards with a gap between them; a hexagonal stud runs along the normal through both gaps at every crossing, its flats against the four boards. Every board is a BeamCurved, a rectangle section swept along the lamella's central axis into one closed BRep: four cubic rails, one through each corner of the board's sections, a ruled face between each two neighbouring rails and a planar cap at each end, so it is smooth along its length and kinks only at its four long edges and its two ends; the viewer draws those faces, not a ladder of section rings. Contacts and the clash check read the same solid sampled at its sections. For each scene the example prints how far a minimal mesh is from minimal, the largest normal curvature along the lamellas, how far an unrolled lamella strays from a straight line, how far a side face's ruling tilts from the normal and the largest overlap between two elements: normal curvature and unrolled deviation are about zero on asymptotic curves and large on the iso-curves. It fails unless every board is a valid six-face BRep solid, every stud touches its four boards, no overlap is larger than the tolerance and every board loads back from the file as a BeamCurved.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_gridshell --parallel 4 && ./build/templates_gridshell && ../bash/publish-scene.sh --target templates_gridshell

|||||||| WORKFLOW ||||||||
examples/templates_gridshell.cpp
 |
 |-- compute_saddle / compute_saddle_mesh / compute_catenoid_mesh / compute_enneper_mesh   the five carriers
 |
 |-- Gridshell::from_surface(surface, curves, count_top, count_bottom, lamella)  src/templates/shells/lamella_gridshell.h
 |-- Gridshell::from_mesh(mesh, count_top, count_bottom, lamella)
 |    |-- SurfaceField / MeshField: the iso or asymptotic directions, in (u, v) or on the mesh from vertex shape operators
 |    |-- compute_family: RK4 traces seeded along a spine of the other family
 |    |-- compute_crossings: segment against segment in the local tangent plane
 |    |-- compute_stations: frames along each lamella, straight a gap either side of a crossing
 |    |-- compute_board: two BeamCurved per lamella on its central axis, the rectangle section, the local normal as up
 |    |    '-- BeamCurved::element_geometry_brep: the section swept to four rails, ruled faces between them, two planar caps, one closed solid
 |    '-- compute_stud: a Column per crossing, a hexagon of three flat pairs gap apart
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
