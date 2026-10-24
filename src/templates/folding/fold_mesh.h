#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The corners of a regular polygon around the origin at one height, counter-clockwise from the right of its lowest edge, so the edge
/// at -y is level: the plan drawn the way Engel draws it, a square on its side, a triangle and a hexagon on an edge.
inline std::vector<Point> regular_polygon(size_t sides, double radius, double z) {

    std::vector<Point> corners;

    for (size_t i = 0; i < sides; i++) {
        const double angle = -0.5 * Tolerance::PI + Tolerance::PI / static_cast<double>(sides) + 2.0 * Tolerance::PI * static_cast<double>(i) / static_cast<double>(sides);
        corners.push_back(Point(radius * std::cos(angle), radius * std::sin(angle), z));
    }

    return corners;
}

/// A folded surface gathered face by face from points, a point used by several faces welded to one vertex by its position, so the
/// mesh is one connected sheet whose faces the winding unifier can turn the same way; the sheet faces down, as every folding template's
/// mesh does, so the plates grow from it in one direction.
class FoldMesh {
public:
    /// The index of point, the one already gathered when a point lies within a hundredth of a millimetre of it.
    size_t add(const Point& point) {

        const std::array<long long, 3> key = {std::llround(point[0] * 100.0), std::llround(point[1] * 100.0), std::llround(point[2] * 100.0)};
        const std::map<std::array<long long, 3>, size_t>::const_iterator found = _welded.find(key);

        if (found != _welded.end())
            return found->second;

        _welded[key] = _points.size();
        _points.push_back(point);
        return _points.size() - 1;
    }

    /// A face through points, in the order given.
    void face(const std::vector<Point>& points) {

        std::vector<size_t> indices;

        for (const Point& point : points)
            indices.push_back(add(point));

        _faces.push_back(indices);
    }

    /// The faces as one mesh, every face wound the same way and the sheet facing down.
    Mesh mesh() const {

        Mesh mesh = Mesh::from_vertices_and_faces(_points, _faces);
        mesh.unify_winding();
        const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = mesh.to_vertices_and_faces();
        double up = 0.0;

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

    /// Every point gathered so far.
    const std::vector<Point>& points() const {
        return _points;
    }

private:
    std::vector<Point> _points;
    std::vector<std::vector<size_t>> _faces;
    std::map<std::array<long long, 3>, size_t> _welded;
};
