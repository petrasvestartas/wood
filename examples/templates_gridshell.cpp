#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/shells/lamella_gridshell.h"

using namespace session_cpp;
using namespace wood_session;

/// One gridshell of the row: its group name, curve family, plan side and lamellas per family.
struct Shell {
    std::string name; // Group name prefix.
    int curves; // 0 iso-curves, 1 asymptotic curves.
    double size; // Plan side of the saddle.
    int count; // Lamellas per family.
};

const std::vector<Shell> SHELLS = {{"asymptotic", 1, 10000.0, 9}, {"iso", 0, 6000.0, 5}};
const double RISE = 0.15; // corner lift and drop of a saddle over its side
const double SKEW = 0.5; // how much faster the saddle curves at one end, so the asymptotic curves bend in plan
const double GAP = 4000.0; // clear distance between the shells
const wood_gridshell::Lamella LAMELLA{.height = 140.0, .thickness = 20.0, .gap = 60.0, .spacing = 180.0, .overrun = 20.0, .step = 100.0};
const double CLEARANCE = 1.0; // faces closer than this count as touching, not overlapping

/// A convex part of an element with its box, what the clash check cuts.
struct Piece {
    size_t element; // Position of the element in the session.
    Mesh mesh; // Closed convex solid.
    std::pair<Point, Point> box; // Lowest and highest corner.
};

/// A cubic saddle over a square of side size from x, z = f(x) - y^2 with f convex, so the Gaussian curvature is negative everywhere; u along y and v along x so the normal points up.
NurbsSurface compute_saddle(double size, double x) {

    std::vector<Point> points;
    for (int u = 0; u < 4; u++)
        for (int v = 0; v < 4; v++) {
            const double across = v / 1.5 - 1.0;
            const double along = u / 1.5 - 1.0;
            points.emplace_back(x + (across + 1.0) * size / 2.0, along * size / 2.0, RISE * size / 2.0 * (across * across * (1.0 + SKEW * across) - along * along));
        }

    return NurbsSurface::create(false, false, 3, 3, 4, 4, points);
}

/// Convex pieces of an element: one loft per board segment, a stud whole.
std::vector<Mesh> compute_pieces(const Element& element) {

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

/// The largest overlap volume over every two convex pieces of different elements whose boxes overlap.
double compute_clash(const WoodSession& session) {

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

/// Largest angle in degrees between a board section's height edge and the surface normal at its station.
double compute_tilt(const wood_gridshell::Gridshell& gridshell) {

    const size_t tops = gridshell.top.size() / 2;
    double worst = 0.0;
    for (size_t i = 0; i < gridshell.frames.size(); i++)
        for (size_t side = 0; side < 2; side++) {
            const std::shared_ptr<Beam>& board = i < tops ? gridshell.top[2 * i + side] : gridshell.bottom[2 * (i - tops) + side];
            const std::vector<Polyline> sections = board->sections();
            for (size_t k = 0; k < sections.size(); k++) {
                const double cosine = (sections[k][3] - sections[k][0]).normalized().dot(gridshell.frames[i][k].z_axis());
                worst = std::max(worst, std::acos(std::clamp(std::abs(cosine), 0.0, 1.0)) * 180.0 / Tolerance::PI);
            }
        }

    return worst;
}

int main() {

    WoodSession wood_session("templates_gridshell");
    std::vector<wood_gridshell::Gridshell> gridshells;
    double offset = 0.0;

    for (const Shell& shell : SHELLS) {
        gridshells.push_back(wood_gridshell::Gridshell::from_surface(compute_saddle(shell.size, offset), shell.curves, shell.count, shell.count, LAMELLA));
        offset += shell.size + GAP;

        const std::shared_ptr<TreeNode> top = wood_session.add_group(shell.name + "_top");
        for (const std::shared_ptr<Beam>& board : gridshells.back().top)
            wood_session.add(board, top);

        const std::shared_ptr<TreeNode> bottom = wood_session.add_group(shell.name + "_bottom");
        for (const std::shared_ptr<Beam>& board : gridshells.back().bottom)
            wood_session.add(board, bottom);

        const std::shared_ptr<TreeNode> studs = wood_session.add_group(shell.name + "_studs");
        for (const std::shared_ptr<Column>& stud : gridshells.back().studs)
            wood_session.add(stud, studs);
    }

    wood_session.compute_contacts();
    wood_session.set_features_visible("section", false);
    wood_session.set_features_visible("axis", false);
    wood_session.pb_dump(pb_path("live"));

    bool touching = true;
    for (size_t i = 0; i < SHELLS.size(); i++) {
        size_t count = 0;
        for (const std::shared_ptr<Column>& stud : gridshells[i].studs)
            count += wood_session.get_neighbours(stud->guid()).size() == 4 ? 1 : 0;

        touching = touching && count == gridshells[i].studs.size();
        std::cout << fmt::format("{}: {} boards, {} of {} studs touch four boards, normal curvature {:.2e} 1/mm, unrolled deviation {:.3f} mm, section tilt {:.4f} deg\n", SHELLS[i].name, gridshells[i].top.size() + gridshells[i].bottom.size(), count, gridshells[i].studs.size(), compute_bending(gridshells[i]), compute_deviation(gridshells[i]), compute_tilt(gridshells[i]));
    }

    const double clash = compute_clash(wood_session);
    std::cout << fmt::format("{} contacts, largest overlap {} mm3\n", wood_session.get_contacts().size(), clash);

    return touching && clash <= CLEARANCE ? 0 : 1;
}

/*
|||||||| DESCRIPTION ||||||||
The lamella gridshell template twice, side by side: a 10 m saddle on its asymptotic curves and a 6 m saddle on its iso-curves. The first family of curves is the top layer a layer up the normal, the second the bottom layer a layer down, each lamella two upright boards with a gap between them; a hexagonal stud runs along the normal through both gaps at every crossing, its flats against the four boards. For each shell the example prints the largest normal curvature along the lamellas, how far an unrolled lamella strays from a straight line and how far a board section tilts from the normal: the first two are about zero on the asymptotic curves and large on the iso-curves. It fails unless every stud touches its four boards, then cuts every two convex pieces (a board segment, a stud) of different elements by each other and fails when any overlap is larger than the tolerance.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_gridshell --parallel 4 && ./build/templates_gridshell && ../bash/publish-scene.sh --target templates_gridshell

|||||||| WORKFLOW ||||||||
examples/templates_gridshell.cpp
 |
 |-- Gridshell::from_surface(surface, curves, count_top, count_bottom, lamella)  src/templates/shells/lamella_gridshell.h
 |    |-- compute_family: RK4 traces of the iso or asymptotic directions, seeded along a spine of the other family
 |    |-- compute_crossings: segment against segment in (u, v)
 |    |-- compute_stations: frames along each lamella, straight a gap either side of a crossing
 |    |-- compute_board: two Beams per lamella, profile_rectangle(thickness, height) on the local normal
 |    '-- compute_stud: a Column per crossing, a hexagon of three flat pairs gap apart
 |
 |-- WoodSession, add_group(<shell>_top, _bottom, _studs), add(element, group)
 |-- compute_contacts()                        face contacts, a stud against each of its four boards
 |-- set_features_visible("section" and "axis")   the section rings and the shared centreline of the boards off in the viewer
 '-- pb_dump(pb_path("live"))                  data/output/pb/live.pb, the file the viewer watches

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
