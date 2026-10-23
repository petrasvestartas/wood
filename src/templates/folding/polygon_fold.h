#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// Folded surfaces over a regular polygon plan after Engel, Structure Systems, Folded plate structures (pp. 228, 231-233): singly
/// folded surfaces, one per side of the plan, their folds parallel to that side, interpenetrating along the lines from the corners to
/// the centre. The folds are concentric copies of the plan, each ring at one height, so a band between two rings in one sector lies
/// between two parallel edges and is a planar trapezoid, whatever the heights: rings alternating ridge and valley fold the surface,
/// their heights rising, level or falling towards the centre set Engel's ridges rising toward the centre, horizontal or sloping to it.
/// The centre is an apex, the sectors' last triangles a pyramid (p. 231), or the innermost ring closed by a flat top, the truncated
/// pyramid (p. 233), its outer trapezoids folded into three triangles each from their outer edges' raised midpoints for a variation.
/// Every face is planar, so every plate is, whatever its thickness.
///
/// Fields: the plan, the ring heights, the centre and the thickness; the plan, the rings, the mesh and the plates are in the session.
class PolygonFold : public WoodSession {
public:
    const size_t sides; // Sides of the regular polygon plan.
    const double radius; // mm, the plan's circumradius, its corners on the circle around the origin.
    const std::vector<double> heights; // mm, every ring's height from the outer edge inwards, the rings evenly spaced to the centre.
    const double apex; // mm, the centre's height when the surface comes to a point.
    const bool truncated; // Whether the innermost ring is closed by a flat top instead of rising to the apex.
    const double thickness; // mm, every plate.
    const double edge_lift; // mm, the outer edges' midpoints raised so every outer trapezoid folds into three triangles (p. 233); 0 keeps it one.

    /// The fold over the polygon plan as the session named name.
    explicit PolygonFold(
        size_t sides = 4,
        double radius = 12000.0,
        const std::vector<double>& heights = {0.0, 1800.0, 900.0, 2700.0, 1800.0, 3600.0},
        double apex = 4500.0,
        bool truncated = false,
        double thickness = 240.0,
        double edge_lift = 0.0,
        const std::string& name = "polygon_fold"
    );

    /// Not copied, as a session is not.
    PolygonFold(const PolygonFold&) = delete;

    /// Not assigned, as it is not copied.
    PolygonFold& operator=(const PolygonFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh;

    /// Per ring its polygon, scaled to the ring's distance from the centre and lifted to its height.
    std::vector<Polyline> compute_rings() const;

    /// Per sector and band one trapezoid between two rings, the innermost band a triangle to the apex or the whole top one face.
    Mesh compute_mesh(const std::vector<Polyline>& rings) const;
};

inline PolygonFold::PolygonFold(
    size_t sides,
    double radius,
    const std::vector<double>& heights,
    double apex,
    bool truncated,
    double thickness,
    double edge_lift,
    const std::string& name
)
    : WoodSession(name),
      sides(sides),
      radius(radius),
      heights(heights),
      apex(apex),
      truncated(truncated),
      thickness(thickness),
      edge_lift(edge_lift) {

    // rings: a concentric copy of the plan at every ring's height, the outer one the plan
    const std::vector<Polyline> rings = compute_rings();
    const std::shared_ptr<TreeNode> folds = add_group("rings");

    for (const Polyline& ring : rings)
        add_polyline(ring, folds);

    // mesh: trapezoids between the rings in every sector, the centre an apex or a flat top
    _mesh = compute_mesh(rings);
    add_mesh(_mesh, add_group("mesh"));

    // plates: one plate per face, mitered at the folds, every corner cut back by the thickness on both faces, as the reflex fold
    // does, so the plates meeting at a vertex do not pass through each other
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : mitered_plates(_mesh, thickness, thickness, thickness, 180.0))
        add(plate, plates);
}

inline const Mesh& PolygonFold::mesh() const {
    return _mesh;
}

inline std::vector<Polyline> PolygonFold::compute_rings() const {

    const size_t count = heights.size();
    std::vector<Polyline> rings;

    for (size_t k = 0; k < count; k++) {
        const double scale = radius * (1.0 - static_cast<double>(k) / static_cast<double>(count));
        std::vector<Point> points;

        for (size_t i = 0; i <= sides; i++) {
            const double angle = 2.0 * Tolerance::PI * static_cast<double>(i % sides) / static_cast<double>(sides);
            points.push_back(Point(scale * std::cos(angle), scale * std::sin(angle), heights[k]));
        }

        rings.push_back(Polyline(points));
    }

    return rings;
}

inline Mesh PolygonFold::compute_mesh(const std::vector<Polyline>& rings) const {

    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    for (const Polyline& ring : rings)
        for (size_t i = 0; i < sides; i++)
            points.push_back(ring[i]);

    // bands: per sector a trapezoid between ring k and ring k + 1, facing down; with an edge lift the outer band's trapezoid a fan of
    // three triangles from its outer edge's raised midpoint, the doubly folded variations of the truncated pyramid (p. 233)
    for (size_t k = 0; k + 1 < rings.size(); k++)
        for (size_t i = 0; i < sides; i++) {
            const size_t next = (i + 1) % sides;
            const size_t outer_i = k * sides + i;
            const size_t inner_i = (k + 1) * sides + i;
            const size_t inner_next = (k + 1) * sides + next;
            const size_t outer_next = k * sides + next;

            if (k > 0 || edge_lift == 0.0) {
                faces.push_back({outer_i, inner_i, inner_next, outer_next});
                continue;
            }

            const Point& start = points[outer_i];
            const Point& end = points[outer_next];
            points.push_back(Point(0.5 * (start[0] + end[0]), 0.5 * (start[1] + end[1]), 0.5 * (start[2] + end[2]) + edge_lift));
            const size_t middle = points.size() - 1;
            faces.push_back({middle, outer_i, inner_i});
            faces.push_back({middle, inner_i, inner_next});
            faces.push_back({middle, inner_next, outer_next});
        }

    // centre: the innermost ring closed by a flat top, or a triangle per sector to the apex
    const size_t inner = (rings.size() - 1) * sides;

    if (truncated) {
        std::vector<size_t> top;

        for (size_t i = sides; i > 0; i--)
            top.push_back(inner + i - 1);

        faces.push_back(top);
    } else {
        points.push_back(Point(0.0, 0.0, apex));

        for (size_t i = 0; i < sides; i++)
            faces.push_back({inner + i, points.size() - 1, inner + (i + 1) % sides});
    }

    return Mesh::from_vertices_and_faces(points, faces);
}
