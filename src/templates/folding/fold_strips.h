#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The diagonal a warped strip of a fold folds along (Engel, Folded plate structures, pp. 222-224).
enum class Folding {
    ridge_to_ridge, // Through its two high corners, a ridge running diagonally from the ridge of one station to the next's.
    ridge_to_valley, // Through its two low corners, a valley running diagonally between them.
    alternate, // Fold by fold, each fold the two strips about an even fold line: the first fold ridge to valley, the next ridge to ridge.
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

            // warped strip: the diagonal through its high corners for a ridge, through its low ones for a valley; alternate folding
            // turns fold by fold, strip i lying in the fold about the even line (i + 1) / 2 * 2
            const bool high_ac = height[a] + height[c] > height[b] + height[d];
            const bool ridge = folding == Folding::ridge_to_ridge || (folding == Folding::alternate && ((i + 1) / 2) % 2 == 1);

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

/// The mesh with every two faces that share an edge and lie in one plane joined into one face, again and again until none are left,
/// a corner on a straight edge dropped and a point no face uses left out: a crease the folding does not bend, as the straight
/// fold lines of Engel's elevated folds (p. 223), is no fold, so its two sides are one plate.
inline Mesh merge_coplanar(const Mesh& mesh) {

    std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = mesh.to_vertices_and_faces();
    const std::vector<Point>& points = data.first;
    std::vector<std::vector<size_t>> faces = data.second;

    // the plane of a face: its unit normal by Newell's method, so a polygon of any corner count counts
    const std::function<Vector(const std::vector<size_t>&)> normal_of = [&points](const std::vector<size_t>& face) {
        Vector normal(0.0, 0.0, 0.0);

        for (size_t i = 0; i < face.size(); i++) {
            const Point& p = points[face[i]];
            const Point& q = points[face[(i + 1) % face.size()]];
            normal = normal + Vector((p[1] - q[1]) * (p[2] + q[2]), (p[2] - q[2]) * (p[0] + q[0]), (p[0] - q[0]) * (p[1] + q[1]));
        }

        return normal.normalized();
    };

    bool merged = true;

    while (merged) {
        merged = false;

        // every directed edge to its face: the face across an edge u v holds v u
        std::map<std::pair<size_t, size_t>, size_t> across;

        for (size_t f = 0; f < faces.size(); f++)
            for (size_t i = 0; i < faces[f].size(); i++)
                across[{faces[f][i], faces[f][(i + 1) % faces[f].size()]}] = f;

        for (size_t f = 0; f < faces.size() && !merged; f++)
            for (size_t i = 0; i < faces[f].size() && !merged; i++) {
                const size_t u = faces[f][i];
                const size_t v = faces[f][(i + 1) % faces[f].size()];
                const std::map<std::pair<size_t, size_t>, size_t>::const_iterator other = across.find({v, u});

                if (other == across.end() || other->second == f)
                    continue;

                const size_t g = other->second;
                const Vector normal = normal_of(faces[f]);

                if (normal.dot(normal_of(faces[g])) < 1.0 - 1e-9)
                    continue;

                bool coplanar = true;

                for (const size_t k : faces[g])
                    coplanar = coplanar && std::abs((points[k] - points[u]).dot(normal)) < 0.01;

                if (!coplanar)
                    continue;

                // joined: f from v round to u, then g's corners strictly between u and v
                std::vector<size_t> joined;
                const size_t nf = faces[f].size();
                const size_t ng = faces[g].size();
                const size_t at_v = (i + 1) % nf;
                const size_t at_u = static_cast<size_t>(std::find(faces[g].begin(), faces[g].end(), u) - faces[g].begin());

                for (size_t k = 0; k < nf; k++)
                    joined.push_back(faces[f][(at_v + k) % nf]);

                for (size_t k = 1; k + 1 < ng; k++)
                    joined.push_back(faces[g][(at_u + k) % ng]);

                // straight corners dropped
                std::vector<size_t> corners;

                for (size_t k = 0; k < joined.size(); k++) {
                    const Point& before = points[joined[(k + joined.size() - 1) % joined.size()]];
                    const Point& here = points[joined[k]];
                    const Point& after = points[joined[(k + 1) % joined.size()]];

                    if ((here - before).cross(after - here).magnitude() > 1e-6 * (here - before).magnitude() * (after - here).magnitude())
                        corners.push_back(joined[k]);
                }

                faces[f] = corners;
                faces.erase(faces.begin() + static_cast<std::ptrdiff_t>(g));
                merged = true;
            }
    }

    // the points some face still uses, renumbered
    std::map<size_t, size_t> renumbered;
    std::vector<Point> used;

    for (std::vector<size_t>& face : faces)
        for (size_t& k : face) {
            if (renumbered.find(k) == renumbered.end()) {
                renumbered[k] = used.size();
                used.push_back(points[k]);
            }

            k = renumbered[k];
        }

    return Mesh::from_vertices_and_faces(used, faces);
}
