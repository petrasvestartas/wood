#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// A folded plate with conical folding after Engel, Structure Systems, Folded plate structures (p. 224): one continuous fold
/// profile along the span, its valleys level and its ridges depth high, with its upper edge cut by two sloping planes, rising from
/// end_depth at either end to the ridges at mid-span. Below the cut every flank keeps its plane, a pentagon from the valley line up
/// to its cut edge, which rises from the end to the ridge point at mid-span and falls again; above it the cut itself is a thin
/// triangle in the sloping plane per fold and half span, from the flat end of the fold to the ridge point, so the ends are
/// trapezoids and the middle a sharp zigzag. Every face is planar, so every plate is, whatever its thickness.
///
/// Fields: the folds, the sizes and the thickness; the end profiles, the mesh, the folds and the plates are in the session.
class ConicalFold : public WoodSession {
public:
    const size_t folds; // Folds across, a ridge each between two valleys.
    const double fold_width; // mm, every flank across in plan, half a fold from valley to ridge.
    const double depth; // mm, the ridges over the valleys at mid-span.
    const double end_depth; // mm, the cut over the valleys at either end.
    const double span; // mm, along x.
    const double thickness; // mm, every plate.

    /// The conical fold of folds folds over span as the session named name.
    explicit ConicalFold(
        size_t folds = 4,
        double fold_width = 1500.0,
        double depth = 1500.0,
        double end_depth = 600.0,
        double span = 14400.0,
        double thickness = 120.0,
        const std::string& name = "conical_fold"
    );

    /// Not copied, as a session is not.
    ConicalFold(const ConicalFold&) = delete;

    /// Not assigned, as it is not copied.
    ConicalFold& operator=(const ConicalFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh;

    /// The profile at x: a trapezoid per fold at the ends, cut height high and in from the ridge line by the flank's run to that
    /// height, the full zigzag at mid-span.
    Polyline compute_profile(double x, double height) const;

    /// The mesh: per fold its two flank pentagons and its two cut triangles, every face turned to face -z as the folds of the other
    /// templates.
    Mesh compute_mesh() const;
};

inline ConicalFold::ConicalFold(
    size_t folds,
    double fold_width,
    double depth,
    double end_depth,
    double span,
    double thickness,
    const std::string& name
)
    : WoodSession(name),
      folds(folds),
      fold_width(fold_width),
      depth(depth),
      end_depth(end_depth),
      span(span),
      thickness(thickness) {

    // stations: the cut profile at both ends and the sharp one at mid-span
    const std::shared_ptr<TreeNode> profiles = add_group("stations");
    add_polyline(compute_profile(0.0, end_depth), profiles);
    add_polyline(compute_profile(0.5 * span, depth), profiles);
    add_polyline(compute_profile(span, end_depth), profiles);

    // mesh: the flank pentagons and the cut triangles
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh, the valleys, the cut edges and the ridge points' creases
    const std::shared_ptr<TreeNode> fold_lines = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end)
            add_polyline(Polyline({*start, *end}), fold_lines);
    }

    // plates: one plate per face by the exact offset, its inner face trimmed by its neighbours', every node of more than three plates
    // split so they meet on whole planar faces; the free edges along the span cut flush by the
    // side planes y 0 and y width
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    const double width = 2.0 * fold_width * static_cast<double>(folds);
    const std::vector<Plane> sides = {Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0)), Plane::from_point_normal(Point(0.0, width, 0.0), Vector(0.0, 1.0, 0.0))};

    for (const std::shared_ptr<Plate>& plate : offset_plates(_mesh, thickness, sides))
        add(plate, plates);
}

inline const Mesh& ConicalFold::mesh() const {
    return _mesh;
}

inline Polyline ConicalFold::compute_profile(double x, double height) const {

    const double run = fold_width * height / depth;
    std::vector<Point> points;

    for (size_t k = 0; k < folds; k++) {
        const double valley = 2.0 * fold_width * static_cast<double>(k);
        points.push_back(Point(x, valley, 0.0));
        points.push_back(Point(x, valley + run, height));

        if (run < fold_width)
            points.push_back(Point(x, valley + 2.0 * fold_width - run, height));
    }

    points.push_back(Point(x, 2.0 * fold_width * static_cast<double>(folds), 0.0));
    return Polyline(points);
}

inline Mesh ConicalFold::compute_mesh() const {

    const double run = fold_width * end_depth / depth;
    const double middle = 0.5 * span;
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    // a face from its corners, turned to face -z, its corners shared with the faces around it
    const std::function<std::array<long long, 3>(const Point&)> key = [](const Point& point) {
        return std::array<long long, 3>{std::llround(point[0] * 100.0), std::llround(point[1] * 100.0), std::llround(point[2] * 100.0)};
    };
    std::map<std::array<long long, 3>, size_t> indices;
    const std::function<void(std::vector<Point>)> face = [&points, &faces, &indices, &key](std::vector<Point> corners) {
        const Vector normal = (corners[1] - corners[0]).cross(corners[2] - corners[0]);

        if (normal[2] > 0.0)
            std::reverse(corners.begin(), corners.end());

        std::vector<size_t> face_indices;

        for (const Point& corner : corners) {
            const std::array<long long, 3> id = key(corner);

            if (indices.find(id) == indices.end()) {
                indices[id] = points.size();
                points.push_back(corner);
            }

            face_indices.push_back(indices[id]);
        }

        faces.push_back(face_indices);
    };

    for (size_t k = 0; k < folds; k++) {
        const double valley = 2.0 * fold_width * static_cast<double>(k);
        const double ridge = valley + fold_width;
        const double next = valley + 2.0 * fold_width;
        const Point peak(middle, ridge, depth);

        // the flank pentagons, from the valley line up to the cut edge through the ridge point
        face({Point(0.0, valley, 0.0), Point(0.0, valley + run, end_depth), peak, Point(span, valley + run, end_depth), Point(span, valley, 0.0)});
        face({Point(0.0, next, 0.0), Point(0.0, next - run, end_depth), peak, Point(span, next - run, end_depth), Point(span, next, 0.0)});

        // the cut triangles in the sloping planes, from the flat end of the fold to the ridge point
        face({Point(0.0, valley + run, end_depth), Point(0.0, next - run, end_depth), peak});
        face({Point(span, valley + run, end_depth), Point(span, next - run, end_depth), peak});
    }

    return Mesh::from_vertices_and_faces(points, faces);
}
