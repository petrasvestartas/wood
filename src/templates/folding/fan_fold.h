#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// A cross-folded surface over a square plan with its ridges rising toward the centre after Engel, Structure Systems, Folded plate
/// structures (p. 229): the cross from the edges' midpoints to the centre divides the square into four quarters, and in every quarter
/// straight fold lines fan out from its corner, on the supports, to points along the two half cross lines and to the centre, the
/// diagonal. Every face between two fold lines from one corner is a planar triangle, whatever the heights; the points along the cross
/// rise from the midpoints to the centre, every second one raised by the fold depth, so the fan's lines alternate ridge and valley and
/// the cross itself folds, shared by the two quarters beside it.
///
/// Fields: the plan, the heights and the thickness; the folds, the mesh and the plates are in the session.
class FanFold : public WoodSession {
public:
    const double size; // mm, the square plan's side, centred on the origin.
    const size_t divisions; // Points along every half cross line between its midpoint and the centre, each a fan line's end.
    const double corner; // mm, the corners' height, on the supports.
    const double middle; // mm, the edges' midpoints' height.
    const double centre; // mm, the centre's height, the cross rising to it.
    const double depth; // mm, how far every second point along the cross is raised, the fold of the fan.
    const double thickness; // mm, every plate.

    /// The fan fold over the square as the session named name.
    explicit FanFold(
        double size = 18000.0,
        size_t divisions = 3,
        double corner = 0.0,
        double middle = 600.0,
        double centre = 3000.0,
        double depth = 1800.0,
        double thickness = 120.0,
        const std::string& name = "fan_fold"
    );

    /// Not copied, as a session is not.
    FanFold(const FanFold&) = delete;

    /// Not assigned, as it is not copied.
    FanFold& operator=(const FanFold&) = delete;

    /// The folded triangle mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh;

    /// The points along the half cross line from the midpoint of side side to the centre, both included: side 0 the edge at +y,
    /// then -x, -y and +x, so corner q lies between the half cross lines of sides q - 1 and q.
    std::vector<Point> compute_half_cross(size_t side) const;
};

inline FanFold::FanFold(
    double size,
    size_t divisions,
    double corner,
    double middle,
    double centre,
    double depth,
    double thickness,
    const std::string& name
)
    : WoodSession(name),
      size(size),
      divisions(divisions),
      corner(corner),
      middle(middle),
      centre(centre),
      depth(depth),
      thickness(thickness) {

    const double half = 0.5 * size;
    const std::array<Point, 4> corners = {Point(half, half, corner), Point(-half, half, corner), Point(-half, -half, corner), Point(half, -half, corner)};
    const std::shared_ptr<TreeNode> folds = add_group("folds");
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;
    std::map<std::pair<long long, long long>, size_t> welded;

    // the index of a point, shared by the quarters beside it
    const std::function<size_t(const Point&)> index = [&](const Point& point) {
        const std::pair<long long, long long> key = {std::llround(point[0]), std::llround(point[1])};

        if (welded.find(key) == welded.end()) {
            welded[key] = points.size();
            points.push_back(point);
        }

        return welded[key];
    };

    // quarters: per corner a fan of fold lines to the chain along the half cross before it, the centre and the half cross after it,
    // a triangle between every two neighbouring lines, facing down
    for (size_t q = 0; q < 4; q++) {
        std::vector<Point> chain = compute_half_cross((q + 3) % 4);
        const std::vector<Point> after = compute_half_cross(q);

        for (size_t j = after.size() - 1; j > 0; j--)
            chain.push_back(after[j - 1]);

        const size_t apex = index(corners[q]);

        for (const Point& point : chain)
            add_polyline(Polyline({corners[q], point}), folds);

        for (size_t j = 0; j + 1 < chain.size(); j++) {
            const size_t a = index(chain[j]);
            const size_t b = index(chain[j + 1]);
            const Vector turn = (chain[j] - corners[q]).cross(chain[j + 1] - corners[q]);
            faces.push_back(turn[2] > 0.0 ? std::vector<size_t>{apex, b, a} : std::vector<size_t>{apex, a, b});
        }
    }

    _mesh = Mesh::from_vertices_and_faces(points, faces);
    add_mesh(_mesh, add_group("mesh"));

    // plates: one plate per face, mitered at the folds; only at a node of three or more plates whose inner corners spread apart
    // the corners are cut back, by just that spread, so they do not pass through each other
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : node_cut_plates(_mesh, thickness))
        add(plate, plates);
}

inline const Mesh& FanFold::mesh() const {
    return _mesh;
}

inline std::vector<Point> FanFold::compute_half_cross(size_t side) const {

    // the midpoint's direction from the centre, side 0 along +y, a quarter turn per side
    const double angle = 0.5 * Tolerance::PI * static_cast<double>(side) + 0.5 * Tolerance::PI;
    const double half = 0.5 * size;
    const Vector out(std::round(std::cos(angle)), std::round(std::sin(angle)), 0.0);
    std::vector<Point> points;

    for (size_t j = 0; j <= divisions + 1; j++) {
        const double t = static_cast<double>(j) / static_cast<double>(divisions + 1);
        const double height = middle + (centre - middle) * t + (j % 2 == 1 && j <= divisions ? depth : 0.0);
        points.push_back(Point(out[0] * half * (1.0 - t), out[1] * half * (1.0 - t), height));
    }

    return points;
}
