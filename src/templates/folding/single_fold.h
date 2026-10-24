#pragma once
#include "wood_session.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// Singly folded surfaces over a special plan geometry after Engel, Structure Systems, Folded plate structures (p. 228): a regular
/// polygon plan standing on its corners, every sector between two corners and the centre one singly folded surface, folded once along
/// its ridge from the middle of its edge to the centre, so it is two planar triangles meeting on the ridge and meeting its neighbours on
/// the valleys from the corners to the centre. The ridge's two ends set Engel's three cases: the edge middles as high as the centre,
/// the ridges horizontal (the triangular plan); lower, the ridges rising toward the centre (the square); higher, sloping to it (the
/// hexagon). The edge middle may stand off the edge: pushed out, the eave triangle from the edge to it leans outward and the plan's
/// outline is a star (the triangle); in line, it is a vertical gable (the square); pulled in, it leans inward and is seen in the plan
/// (the hexagon). Every face is a triangle, so every plate is planar, whatever its thickness.
///
/// Fields: the plan, the heights, the overhang and the thickness; the folds, the mesh and the plates are in the session.
class SingleFold : public WoodSession {
public:
    const size_t sides; // Sides of the regular polygon plan.
    const double radius; // mm, the plan's circumradius, its corners on the circle around the origin, on the ground.
    const double overhang; // mm, how far every edge's middle stands off the edge, outward when positive, inward when negative.
    const double gable; // mm, the height of every edge's middle, the ridges' outer ends.
    const double apex; // mm, the centre's height, the ridges' inner end.
    const double thickness; // mm, every plate.

    /// The fold over the polygon plan as the session named name.
    explicit SingleFold(
        size_t sides = 4,
        double radius = 12728.0,
        double overhang = 0.0,
        double gable = 5400.0,
        double apex = 6900.0,
        double thickness = 120.0,
        const std::string& name = "single_fold"
    );

    /// Not copied, as a session is not.
    SingleFold(const SingleFold&) = delete;

    /// Not assigned, as it is not copied.
    SingleFold& operator=(const SingleFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh;
};

inline SingleFold::SingleFold(
    size_t sides,
    double radius,
    double overhang,
    double gable,
    double apex,
    double thickness,
    const std::string& name
)
    : WoodSession(name),
      sides(sides),
      radius(radius),
      overhang(overhang),
      gable(gable),
      apex(apex),
      thickness(thickness) {

    // points: the corners on the ground, every edge's middle stood off it and raised, the centre
    const std::vector<Point> corners = regular_polygon(sides, radius, 0.0);
    const Point centre(0.0, 0.0, apex);
    std::vector<Point> middles;

    for (size_t i = 0; i < sides; i++) {
        const Point& start = corners[i];
        const Point& end = corners[(i + 1) % sides];
        const Vector outward = Vector(end[1] - start[1], start[0] - end[0], 0.0).normalized();
        middles.push_back(Point(0.5 * (start[0] + end[0]) + outward[0] * overhang, 0.5 * (start[1] + end[1]) + outward[1] * overhang, gable));
    }

    // mesh: per edge its eave triangle from the edge to its middle, per sector two triangles folded on the ridge from that middle to the centre
    FoldMesh fold;

    for (size_t i = 0; i < sides; i++) {
        const Point& start = corners[i];
        const Point& end = corners[(i + 1) % sides];
        fold.face({start, end, middles[i]});
        fold.face({start, middles[i], centre});
        fold.face({middles[i], end, centre});
    }

    _mesh = fold.mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: the ridges and the valleys, every edge of the mesh off the ground
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end && ((*start)[2] > 1.0 || (*end)[2] > 1.0))
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face by the exact offset, its inner face trimmed by its neighbours', every node of more than three plates
    // split so they meet on whole planar faces; the edges on the ground cut flush with it
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : offset_plates(_mesh, thickness, {Plane::xy_plane()}))
        add(plate, plates);
}

inline const Mesh& SingleFold::mesh() const {
    return _mesh;
}
