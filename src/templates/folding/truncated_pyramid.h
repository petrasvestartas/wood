#pragma once
#include "wood_session.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// Where a variation folds a side of the truncated pyramid, as shares of a plan line and of the height.
struct FoldPoint {
    double along; // Share of the line's plan length from the base edge.
    double rise; // Share of the height.
};

/// The pyramid and truncated pyramid on a square plan, its sides folded at edge and corner points.
class TruncatedPyramid : public WoodSession {
public:
    const double half; // mm, half the base square's side, centred on the origin.
    const double top; // mm, half the top square's side, 0 for an apex.
    const double height; // mm, the top's height.
    const std::optional<FoldPoint>
        edge_fold; // Every base edge's middle moved in and raised, or none.
    const std::optional<FoldPoint> corner_fold; // Every corner line bent at this point, or none.
    const double thickness; // mm, every plate.

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.

public:
    /// The pyramid as the session named name.
    explicit TruncatedPyramid(
        double half = 9000.0,
        double top = 3000.0,
        double height = 6800.0,
        const std::optional<FoldPoint>& edge_fold = std::nullopt,
        const std::optional<FoldPoint>& corner_fold = std::nullopt,
        double thickness = 120.0,
        const std::string& name = "truncated_pyramid"
    );

    /// Not copied, as a session is not.
    TruncatedPyramid(const TruncatedPyramid&) = delete;

    /// Not assigned, as it is not copied.
    TruncatedPyramid& operator=(const TruncatedPyramid&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;
};

inline TruncatedPyramid::TruncatedPyramid(
    double half,
    double top,
    double height,
    const std::optional<FoldPoint>& edge_fold,
    const std::optional<FoldPoint>& corner_fold,
    double thickness,
    const std::string& name
)
    : WoodSession(name), half(half), top(top), height(height), edge_fold(edge_fold),
      corner_fold(corner_fold), thickness(thickness) {

    // points: the base corners on the ground, the top corners, per corner line its fold point, per side its edge fold point
    const std::vector<Point> base = regular_polygon(4, half * std::sqrt(2.0), 0.0);
    const std::vector<Point> tops = regular_polygon(4, top * std::sqrt(2.0), height);
    std::vector<Point> bends;
    std::vector<Point> middles;

    for (size_t i = 0; i < 4; i++) {
        const Point& start = base[i];
        const Point& end = base[(i + 1) % 4];
        const double along = corner_fold ? corner_fold->along : 0.0;
        const double bend_x = start[0] + (tops[i][0] - start[0]) * along;
        const double bend_y = start[1] + (tops[i][1] - start[1]) * along;
        const double bend_z = corner_fold ? corner_fold->rise * height : 0.0;
        bends.push_back(Point(bend_x, bend_y, bend_z));

        const Vector inward = Vector(start[1] - end[1], end[0] - start[0], 0.0).normalized();
        const double in = edge_fold ? edge_fold->along * (half - top) : 0.0;
        const double middle_x = 0.5 * (start[0] + end[0]) + inward[0] * in;
        const double middle_y = 0.5 * (start[1] + end[1]) + inward[1] * in;
        const double middle_z = edge_fold ? edge_fold->rise * height : 0.0;
        middles.push_back(Point(middle_x, middle_y, middle_z));
    }

    // mesh: per side its chain over the corner folds and the top edge, one face or a fan from the edge fold point; the flat top
    FoldMesh fold;

    for (size_t i = 0; i < 4; i++) {
        const size_t next = (i + 1) % 4;
        std::vector<Point> chain = {base[i]};

        if (corner_fold)
            chain.push_back(bends[i]);

        chain.push_back(tops[i]);

        if (top > 0.0)
            chain.push_back(tops[next]);

        if (corner_fold)
            chain.push_back(bends[next]);

        chain.push_back(base[next]);

        if (!edge_fold) {
            fold.face(chain);
            continue;
        }

        for (size_t k = 0; k + 1 < chain.size(); k++)
            fold.face({chain[k], chain[k + 1], middles[i]});

        if (middles[i][2] > 1.0)
            fold.face({base[next], base[i], middles[i]});
    }

    if (top > 0.0)
        fold.face({tops[3], tops[2], tops[1], tops[0]});

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

inline const Mesh& TruncatedPyramid::mesh() const {
    return _mesh;
}
