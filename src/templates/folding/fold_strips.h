#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The diagonal a warped strip of a fold folds along (Engel, Folded plate structures, pp. 222-227).
enum class Folding {
    ridge_to_ridge, // Through its two high corners, a ridge running diagonally from the ridge of one station to the next's.
    ridge_to_valley, // Through its two low corners, a valley running diagonally between them.
    alternate, // Ridge to ridge in one strip, ridge to valley in the next.
};

/// The folded mesh through stations, all with one point count: per strip between two stations and two fold lines one quad when its
/// fourth corner lies on the plane of the other three, else two triangles on the folding's diagonal, the corners' heights out of the
/// fold telling high from low; faces ordered station, next point, next station, so a station sequence along +x with profiles along
/// +y faces -z.
inline Mesh fold_strips(const std::vector<Polyline>& stations, const std::vector<std::vector<double>>& heights, Folding folding) {

    const size_t row = stations[0].point_count();
    std::vector<Point> points;
    std::vector<double> height;
    std::vector<std::vector<size_t>> faces;

    for (size_t s = 0; s < stations.size(); s++)
        for (size_t i = 0; i < row; i++) {
            points.push_back(stations[s][i]);
            height.push_back(heights[s][i]);
        }

    for (size_t s = 0; s + 1 < stations.size(); s++)
        for (size_t i = 0; i + 1 < row; i++) {
            const size_t a = s * row + i;
            const size_t b = a + 1;
            const size_t c = a + row + 1;
            const size_t d = a + row;

            // planar strip: one quad
            const Vector normal = (points[b] - points[a]).cross(points[d] - points[a]).normalized();

            if (std::abs((points[c] - points[a]).dot(normal)) < 1.0) {
                faces.push_back({a, b, c, d});
                continue;
            }

            // warped strip: the diagonal through its high corners for a ridge, through its low ones for a valley
            const bool high_ac = height[a] + height[c] > height[b] + height[d];
            const bool ridge = folding == Folding::ridge_to_ridge || (folding == Folding::alternate && i % 2 == 0);

            if (high_ac == ridge) {
                faces.push_back({a, b, c});
                faces.push_back({a, c, d});
            } else {
                faces.push_back({a, b, d});
                faces.push_back({b, c, d});
            }
        }

    return Mesh::from_vertices_and_faces(points, faces);
}
