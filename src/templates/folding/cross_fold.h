#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// A cross-folded surface over a square plan after Engel, Structure Systems, Folded plate structures (p. 229), the waterbomb fold of
/// a square sheet: fold lines from the centre to the four corners, the diagonals, and to the edges' midpoints, the cross, each half
/// edge between a corner and a midpoint divided again into divisions segments with a fold line to every division point. Every fold
/// line is straight from the centre to the edge, so every face between two of them is a planar triangle, whatever the heights; the
/// edge points alternate low and high from the corners, so the fold lines alternate valley and ridge, the diagonals valleys and, with
/// an odd division, the cross ridges. The centre's height sets Engel's ridges folded upwards, rising toward or sloping to the centre.
///
/// Units side by side in columns and rows compose a roof over a larger plan (p. 230), their shared edge points welded, so the plates
/// of two units meet mitered along their common edge.
///
/// Fields: the plan, the heights, the grid and the thickness; the fold lines, the mesh and the plates are in the session.
class CrossFold : public WoodSession {
public:
    const double size; // mm, the square plan's side, centred on the origin.
    const size_t divisions; // Segments of every half edge between a corner and a midpoint, 1 for the waterbomb's eight triangles.
    const double low; // mm, the height of the corners and every second division point, the valleys' ends.
    const double high; // mm, the height of the other division points, the ridges' ends.
    const double centre; // mm, the height of the centre where every fold line meets.
    const double thickness; // mm, every plate.
    const size_t columns; // Units along x.
    const size_t rows; // Units along y.

    /// The cross fold over the square as the session named name.
    explicit CrossFold(
        double size = 18000.0,
        size_t divisions = 1,
        double low = 0.0,
        double high = 3000.0,
        double centre = 6000.0,
        double thickness = 120.0,
        size_t columns = 1,
        size_t rows = 1,
        const std::string& name = "cross_fold"
    );

    /// Not copied, as a session is not.
    CrossFold(const CrossFold&) = delete;

    /// Not assigned, as it is not copied.
    CrossFold& operator=(const CrossFold&) = delete;

    /// The folded triangle mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh;

    /// The edge points around the unit square centred on centre, from a corner, every half edge divided into divisions segments,
    /// each at its height.
    std::vector<Point> compute_edge_points(const Point& centre_point) const;
};

inline CrossFold::CrossFold(
    double size,
    size_t divisions,
    double low,
    double high,
    double centre,
    double thickness,
    size_t columns,
    size_t rows,
    const std::string& name
)
    : WoodSession(name),
      size(size),
      divisions(std::max<size_t>(divisions, 1)),
      low(low),
      high(high),
      centre(centre),
      thickness(thickness),
      columns(std::max<size_t>(columns, 1)),
      rows(std::max<size_t>(rows, 1)) {

    // folds and mesh: per unit a straight fold line from its centre to every edge point and a triangle between every two neighbouring
    // ones, the points shared by two units welded
    const std::shared_ptr<TreeNode> folds = add_group("folds");
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;
    std::map<std::pair<long long, long long>, size_t> welded;

    for (size_t column = 0; column < columns; column++)
        for (size_t row = 0; row < rows; row++) {
            const Point middle(size * (static_cast<double>(column) - 0.5 * static_cast<double>(columns - 1)), size * (static_cast<double>(row) - 0.5 * static_cast<double>(rows - 1)), centre);
            const std::vector<Point> edge = compute_edge_points(middle);
            std::vector<size_t> keys;

            for (const Point& point : edge) {
                add_polyline(Polyline({middle, point}), folds);
                const std::pair<long long, long long> key = {std::llround(point[0]), std::llround(point[1])};

                if (welded.find(key) == welded.end()) {
                    welded[key] = points.size();
                    points.push_back(point);
                }

                keys.push_back(welded[key]);
            }

            points.push_back(middle);

            for (size_t i = 0; i < keys.size(); i++)
                faces.push_back({keys[i], points.size() - 1, keys[(i + 1) % keys.size()]});
        }

    _mesh = Mesh::from_vertices_and_faces(points, faces);
    add_mesh(_mesh, add_group("mesh"));

    // plates: one plate per face, mitered at the folds; only at a node of three or more plates whose inner corners spread apart
    // the corners are cut back, by just that spread, so they do not pass through each other
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : node_cut_plates(_mesh, thickness))
        add(plate, plates);
}

inline const Mesh& CrossFold::mesh() const {
    return _mesh;
}

inline std::vector<Point> CrossFold::compute_edge_points(const Point& centre_point) const {

    const double half = 0.5 * size;
    const double x = centre_point[0];
    const double y = centre_point[1];
    const std::array<Point, 4> corners = {Point(x - half, y - half, 0.0), Point(x + half, y - half, 0.0), Point(x + half, y + half, 0.0), Point(x - half, y + half, 0.0)};
    const size_t per_side = 2 * divisions;
    std::vector<Point> points;

    // along every side from its corner, the point j steps on: low on even steps from the nearer corner, high on odd ones
    for (size_t side = 0; side < 4; side++)
        for (size_t j = 0; j < per_side; j++) {
            const Point start = corners[side];
            const Point end = corners[(side + 1) % 4];
            const double t = static_cast<double>(j) / static_cast<double>(per_side);
            const size_t from_corner = std::min(j, per_side - j);
            const double height = from_corner % 2 == 0 ? low : high;
            points.push_back(Point(start[0] + (end[0] - start[0]) * t, start[1] + (end[1] - start[1]) * t, height));
        }

    return points;
}
