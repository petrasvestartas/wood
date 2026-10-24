#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The corners of a regular polygon around the origin at height z, its edge at -y level.
inline std::vector<Point> regular_polygon(size_t sides, double radius, double z) {

    const double count = static_cast<double>(sides);
    const double first = -0.5 * Tolerance::PI + Tolerance::PI / count;
    std::vector<Point> corners;

    for (size_t i = 0; i < sides; i++) {
        const double angle = first + 2.0 * Tolerance::PI * static_cast<double>(i) / count;
        corners.push_back(Point(radius * std::cos(angle), radius * std::sin(angle), z));
    }

    return corners;
}

/// A folded surface gathered face by face, points within a hundredth of a millimetre welded to one vertex.
class FoldMesh {
private:
    std::vector<Point> _points; // Every point gathered, one per vertex.
    std::vector<std::vector<size_t>> _faces; // Every face as vertex indices, in the order given.
    std::map<std::array<long long, 3>, size_t>
        _welded; // Every point's rounded position to its vertex.

public:
    /// The index of point, the one already gathered when a point lies within a hundredth of a millimetre of it.
    size_t add(const Point& point);

    /// A face through points, in the order given.
    void face(const std::vector<Point>& points);

    /// The faces as one mesh, every face wound the same way and the sheet facing down.
    Mesh mesh() const;

    /// Every point gathered so far.
    const std::vector<Point>& points() const;

    /// Every face gathered so far, in the order given.
    const std::vector<std::vector<size_t>>& faces() const;
};

inline size_t FoldMesh::add(const Point& point) {

    const std::array<long long, 3> key = {
        std::llround(point[0] * 100.0),
        std::llround(point[1] * 100.0),
        std::llround(point[2] * 100.0),
    };
    const std::map<std::array<long long, 3>, size_t>::const_iterator found = _welded.find(key);

    if (found != _welded.end())
        return found->second;

    _welded[key] = _points.size();
    _points.push_back(point);
    return _points.size() - 1;
}

inline void FoldMesh::face(const std::vector<Point>& points) {

    std::vector<size_t> indices;

    for (const Point& point : points)
        indices.push_back(add(point));

    _faces.push_back(indices);
}

inline Mesh FoldMesh::mesh() const {

    // winding: every face turned the same way
    Mesh mesh = Mesh::from_vertices_and_faces(_points, _faces);
    mesh.unify_winding();
    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data =
        mesh.to_vertices_and_faces();
    double up = 0.0;

    // facing down: the sheet flipped when its projected area faces up
    for (const std::vector<size_t>& face : data.second)
        for (size_t i = 0; i < face.size(); i++) {
            const Point& a = data.first[face[i]];
            const Point& b = data.first[face[(i + 1) % face.size()]];
            up += (a[0] - b[0]) * (a[1] + b[1]);
        }

    if (up > 0.0)
        mesh.flip();

    return mesh;
}

inline const std::vector<Point>& FoldMesh::points() const {
    return _points;
}

inline const std::vector<std::vector<size_t>>& FoldMesh::faces() const {
    return _faces;
}
