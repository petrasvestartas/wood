#pragma once
#include "wood_session.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// Folded triangular surfaces over a regular polygon plan, a gable on every edge.
class TriangleFold : public WoodSession {
public:
    const size_t sides; // Sides of the regular polygon plan.
    const double
        radius; // mm, the plan's circumradius, its corners on the circle around the origin.
    const double middle; // mm, the edges' middles' height, the gables' peaks.
    const double
        centre; // mm, the centre's height; at the middles' height the polygon of the middles stays one flat face.
    const double thickness; // mm, every plate.

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.

public:
    /// The triangular fold over the polygon plan as the session named name.
    explicit TriangleFold(
        size_t sides = 4,
        double radius = 12000.0,
        double middle = 5000.0,
        double centre = 6500.0,
        double thickness = 120.0,
        const std::string& name = "triangle_fold"
    );

    /// Not copied, as a session is not.
    TriangleFold(const TriangleFold&) = delete;

    /// Not assigned, as it is not copied.
    TriangleFold& operator=(const TriangleFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;
};

inline TriangleFold::TriangleFold(
    size_t sides,
    double radius,
    double middle,
    double centre,
    double thickness,
    const std::string& name
)
    : WoodSession(name), sides(sides), radius(radius), middle(middle), centre(centre),
      thickness(thickness) {

    // points: the corners on the ground, the edges' middles raised, the centre
    const std::vector<Point> corners = regular_polygon(sides, radius, 0.0);
    std::vector<Point> middles;

    for (size_t i = 0; i < sides; i++) {
        const Point& start = corners[i];
        const Point& end = corners[(i + 1) % sides];
        const double x = 0.5 * (start[0] + end[0]);
        const double y = 0.5 * (start[1] + end[1]);
        middles.push_back(Point(x, y, middle));
    }

    // mesh: a gable per edge, a triangle per corner, the middles' polygon one flat face or a triangle per side to the centre
    FoldMesh fold;

    for (size_t i = 0; i < sides; i++) {
        fold.face({corners[i], corners[(i + 1) % sides], middles[i]});
        fold.face({corners[i], middles[i], middles[(i + sides - 1) % sides]});
    }

    const Point top(0.0, 0.0, centre);

    if (std::abs(centre - middle) < 1.0)
        fold.face(middles);
    else
        for (size_t i = 0; i < sides; i++)
            fold.face({middles[i], middles[(i + 1) % sides], top});

    _mesh = fold.mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh off the ground
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end && ((*start)[2] > 1.0 || (*end)[2] > 1.0))
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face by the exact offset, the edges on the ground cut flush with it
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : offset_plates(_mesh, thickness, {Plane::xy_plane()}))
        add(plate, plates);
}

inline const Mesh& TriangleFold::mesh() const {
    return _mesh;
}
