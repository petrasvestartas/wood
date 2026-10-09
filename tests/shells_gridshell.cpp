#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/shells/lamella_gridshell.h"

using namespace session_cpp;
using namespace wood_session;
using wood_gridshell::LamellaGridshell;

const double CLEARANCE = 1.0; // faces closer than this count as touching, not overlapping

/// A convex part of an element with its box, what the clash check cuts.
struct Piece {
    size_t element; // Position of the element in the session.
    Mesh mesh; // Closed convex solid.
    std::pair<Point, Point> box; // Lowest and highest corner.
};

static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// Convex pieces of an element: one loft per board segment, a stud whole.
static std::vector<Mesh> compute_pieces(const Element& element) {

    const Beam* board = dynamic_cast<const Beam*>(&element);

    if (!board)
        return {element.model_geometry_mesh()};

    const std::vector<Polyline> sections = board->sections();
    std::vector<Mesh> pieces;

    for (size_t i = 0; i + 1 < sections.size(); i++)
        pieces.push_back(Mesh::loft({sections[i]}, {sections[i + 1]}, true));

    return pieces;
}

/// The face planes of a closed mesh, normals out.
static std::vector<Plane> compute_planes(const Mesh& mesh) {

    const Point centre = mesh.centroid();
    std::vector<Plane> planes;

    for (const size_t face : mesh.faces()) {
        const std::vector<Point> points = mesh.face_polygon(face)->get_points();
        const Vector normal = compute_newell(points).normalized();
        const Point origin = Point::centroid(points);
        const Vector outward = (origin - centre).dot(normal) < 0.0 ? -normal : normal;
        planes.push_back(Plane::from_point_normal(origin, outward));
    }

    return planes;
}

/// Volume of other inside a convex mesh, its face planes pulled in by the tolerance.
static double compute_overlap(const Mesh& convex, const Mesh& other) {

    Mesh part = other;

    for (const Plane& plane : compute_planes(convex)) {
        const Point origin = plane.origin() - plane.z_axis() * CLEARANCE;
        part = part.cut_by_plane(Plane::from_point_normal(origin, -plane.z_axis()));

        if (part.faces().empty())
            return 0.0;
    }

    return std::abs(part.volume());
}

/// The lowest and highest corner of a mesh.
static std::pair<Point, Point> compute_box(const Mesh& mesh) {

    std::pair<Point, Point> box(Point(1e18, 1e18, 1e18), Point(-1e18, -1e18, -1e18));

    for (const size_t vertex : mesh.vertices())
        for (int axis = 0; axis < 3; axis++) {
            const double value = (*mesh.vertex_point(vertex))[axis];
            box.first[axis] = std::min(box.first[axis], value);
            box.second[axis] = std::max(box.second[axis], value);
        }

    return box;
}

/// True when two boxes overlap by more than the tolerance on every axis.
static bool is_near(const std::pair<Point, Point>& a, const std::pair<Point, Point>& b) {

    for (int axis = 0; axis < 3; axis++)
        if (a.second[axis] <= b.first[axis] + CLEARANCE || b.second[axis] <= a.first[axis] + CLEARANCE)
            return false;

    return true;
}

/// The largest overlap volume over every two convex pieces of different elements whose boxes overlap.
static double compute_clash(const WoodSession& session) {

    std::vector<Piece> pieces;
    const Collection<std::shared_ptr<Element>>& elements = session.elements();

    for (size_t i = 0; i < elements.size(); i++)
        for (const Mesh& mesh : compute_pieces(*elements[i]))
            pieces.push_back(Piece{i, mesh, compute_box(mesh)});

    double worst = 0.0;

    for (size_t i = 0; i < pieces.size(); i++)
        for (size_t j = i + 1; j < pieces.size(); j++)
            if (pieces[i].element != pieces[j].element && is_near(pieces[i].box, pieces[j].box))
                worst = std::max(worst, compute_overlap(pieces[i].mesh, pieces[j].mesh));

    return worst;
}

/// Largest normal curvature along the lamella centrelines in 1/mm.
static double compute_bending(const LamellaGridshell& gridshell) {

    double worst = 0.0;

    for (const std::vector<Plane>& frames : gridshell.stations())
        for (size_t k = 1; k + 1 < frames.size(); k++) {
            const Vector before = frames[k].origin() - frames[k - 1].origin();
            const Vector after = frames[k + 1].origin() - frames[k].origin();
            const double turn = (after.normalized() - before.normalized()).dot(frames[k].z_axis());
            worst = std::max(worst, std::abs(turn) * 2.0 / (before.magnitude() + after.magnitude()));
        }

    return worst;
}

/// Largest distance in mm of an unrolled lamella centreline from the line through its ends.
static double compute_deviation(const LamellaGridshell& gridshell) {

    double worst = 0.0;

    for (const std::vector<Plane>& frames : gridshell.stations()) {
        std::vector<Point> flat{Point(0.0, 0.0, 0.0)};
        double heading = 0.0;

        for (size_t k = 0; k + 1 < frames.size(); k++) {
            const Vector after = frames[k + 1].origin() - frames[k].origin();

            if (k > 0) {
                const Vector before = (frames[k].origin() - frames[k - 1].origin()).normalized();
                const double turn = (after.normalized() - before).dot(frames[k].z_axis());
                heading += std::asin(std::clamp(turn, -1.0, 1.0));
            }

            flat.push_back(flat.back() + Vector(std::cos(heading), std::sin(heading), 0.0) * after.magnitude());
        }

        const Vector chord = (flat.back() - flat.front()).normalized();

        for (const Point& point : flat)
            worst = std::max(worst, (point - flat.front()).cross(chord).magnitude());
    }

    return worst;
}

/// Largest angle in degrees between a board section's height edge and the surface normal at its station.
static double compute_tilt(const LamellaGridshell& gridshell) {

    const std::vector<std::vector<Plane>>& stations = gridshell.stations();
    const size_t tops = static_cast<size_t>(gridshell.count_top);
    double worst = 0.0;

    for (size_t i = 0; i < stations.size(); i++)
        for (size_t side = 0; side < 2; side++) {
            const std::string layer = i < tops ? "top" : "bottom";
            const size_t index = i < tops ? i : i - tops;
            const std::string name = fmt::format("board_{}_{}_{}", layer, index, side);
            const std::shared_ptr<Beam> board = gridshell.get_element_by_name<Beam>(name);
            const std::vector<Polyline> sections = board->sections();

            for (size_t k = 0; k < sections.size(); k++) {
                const Vector height = (sections[k][3] - sections[k][0]).normalized();
                const double cosine = height.dot(stations[i][k].z_axis());
                worst = std::max(worst, std::acos(std::clamp(std::abs(cosine), 0.0, 1.0)) * 180.0 / Tolerance::PI);
            }
        }

    return worst;
}

/// Studs that touch exactly four boards.
static size_t count_touching(LamellaGridshell& gridshell) {

    size_t count = 0;

    for (const std::shared_ptr<Column>& stud : gridshell.get_elements<Column>())
        if (gridshell.get_neighbours(stud->guid()).size() == 4)
            count++;

    return count;
}

/// The asymptotic shell unrolls straight, the iso shell does not; every stud touches four boards and nothing overlaps.
int main() {

    LamellaGridshell asymptotic;
    const NurbsSurface saddle = LamellaGridshell::default_surface(6000.0);
    LamellaGridshell iso(saddle, 0, 5, 5);

    for (LamellaGridshell* gridshell : {&asymptotic, &iso}) {
        const size_t boards = gridshell->get_elements<Beam>().size();
        const size_t studs = gridshell->get_elements<Column>().size();
        const size_t touching = count_touching(*gridshell);
        const double clash = compute_clash(*gridshell);
        std::cout << fmt::format("{}: {} boards, {} of {} studs touch four boards, normal curvature {:.2e} 1/mm, unrolled deviation {:.3f} mm, section tilt {:.4f} deg, largest overlap {} mm3\n", gridshell->curves == 1 ? "asymptotic" : "iso", boards, touching, studs, compute_bending(*gridshell), compute_deviation(*gridshell), compute_tilt(*gridshell), clash);
        check(touching == studs, "every stud touches its four boards");
        check(clash <= CLEARANCE, "no board or stud overlaps another");
    }

    check(compute_bending(asymptotic) < 1e-6, "the asymptotic lamellas have no normal curvature");
    check(compute_deviation(asymptotic) < 0.1, "the asymptotic lamellas unroll within 0.1 mm of straight");
    check(compute_deviation(iso) > 100.0, "the iso lamellas unroll more than 100 mm off straight");
    std::cout << "shells_gridshell: ok\n";
    return 0;
}
