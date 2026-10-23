#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// A structure system of folded triangular surfaces after Engel, Structure Systems, Folded plate structures (p. 232): a regular
/// polygon plan cut by the lines joining its edges' midpoints into a triangle at every corner, and the polygon of the midpoints
/// either one face, the triangular plan's middle triangle, or split into triangles to a raised centre. Every face is a triangle or a
/// flat polygon of points at one height, so every plate is planar, whatever its thickness: the corners low on the supports, the
/// midpoints high, the corner triangles folding down to the corners.
///
/// Fields: the plan, the heights and the thickness; the folds, the mesh and the plates are in the session.
class TriangleFold : public WoodSession {
public:
    const size_t sides; // Sides of the regular polygon plan.
    const double radius; // mm, the plan's circumradius, its corners on the circle around the origin.
    const double corner; // mm, the corners' height, on the supports.
    const double middle; // mm, the edges' midpoints' height.
    const double centre; // mm, the centre's height; at the midpoints' height the polygon of the midpoints stays one flat face.
    const double thickness; // mm, every plate.

    /// The triangular fold over the polygon plan as the session named name.
    explicit TriangleFold(
        size_t sides = 4,
        double radius = 12000.0,
        double corner = 0.0,
        double middle = 4000.0,
        double centre = 7000.0,
        double thickness = 240.0,
        const std::string& name = "triangle_fold"
    );

    /// Not copied, as a session is not.
    TriangleFold(const TriangleFold&) = delete;

    /// Not assigned, as it is not copied.
    TriangleFold& operator=(const TriangleFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh;
};

inline TriangleFold::TriangleFold(
    size_t sides,
    double radius,
    double corner,
    double middle,
    double centre,
    double thickness,
    const std::string& name
)
    : WoodSession(name),
      sides(sides),
      radius(radius),
      corner(corner),
      middle(middle),
      centre(centre),
      thickness(thickness) {

    // points: the corners on the circle, the edges' midpoints, the centre
    std::vector<Point> points;

    for (size_t i = 0; i < sides; i++) {
        const double angle = 2.0 * Tolerance::PI * static_cast<double>(i) / static_cast<double>(sides);
        points.push_back(Point(radius * std::cos(angle), radius * std::sin(angle), corner));
    }

    for (size_t i = 0; i < sides; i++) {
        const Point& start = points[i];
        const Point& end = points[(i + 1) % sides];
        points.push_back(Point(0.5 * (start[0] + end[0]), 0.5 * (start[1] + end[1]), middle));
    }

    points.push_back(Point(0.0, 0.0, centre));
    const size_t mid = sides;
    const size_t top = 2 * sides;

    // mesh: a triangle at every corner between its two midpoints, the midpoints' polygon one flat face or a triangle per side to
    // the centre, all facing down
    std::vector<std::vector<size_t>> faces;

    for (size_t i = 0; i < sides; i++)
        faces.push_back({i, mid + (i + sides - 1) % sides, mid + i});

    if (std::abs(centre - middle) < 1.0) {
        std::vector<size_t> inner;

        for (size_t i = sides; i > 0; i--)
            inner.push_back(mid + i - 1);

        faces.push_back(inner);
    } else {
        for (size_t i = 0; i < sides; i++)
            faces.push_back({top, mid + i, mid + (i + sides - 1) % sides});
    }

    _mesh = Mesh::from_vertices_and_faces(points, faces);
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end)
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face, mitered at the folds, every corner cut back by the thickness so the plates meeting at a vertex do
    // not pass through each other
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : mitered_plates(_mesh, thickness, thickness, thickness, 180.0))
        add(plate, plates);
}

inline const Mesh& TriangleFold::mesh() const {
    return _mesh;
}
