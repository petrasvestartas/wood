#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// The diagonal a warped strip of a fold folds along (Engel, Folded plate structures, pp. 222-224).
enum class Folding {
    ridge_to_ridge, // Through its two high corners, a ridge running diagonally from the ridge of one station to the next's.
    ridge_to_valley, // Through its two low corners, a valley running diagonally between them.
    alternate, // Fold by fold, the first ridge to valley, the next ridge to ridge.
};

/// The folded mesh through stations: per strip one planar quad, else two triangles on the folding's diagonal, facing -z.
inline Mesh fold_strips(const std::vector<Polyline>& stations, const std::vector<std::vector<double>>& heights, Folding folding) {

    const size_t row = stations[0].point_count();
    std::vector<Point> points;
    std::vector<double> height;
    std::vector<std::vector<size_t>> faces;

    // points: every station's points with their heights
    for (size_t s = 0; s < stations.size(); s++)
        for (size_t i = 0; i < row; i++) {
            points.push_back(stations[s][i]);
            height.push_back(heights[s][i]);
        }

    // faces: every strip between two stations and two fold lines
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

            // warped strip: two triangles on the diagonal through its high corners for a ridge, its low ones for a valley
            const bool high_ac = height[a] + height[c] > height[b] + height[d];
            const bool alternate_ridge = folding == Folding::alternate && ((i + 1) / 2) % 2 == 1;
            const bool ridge = folding == Folding::ridge_to_ridge || alternate_ridge;

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

/// Faces f and g joined across f's edge i into one face, its straight corners dropped.
inline std::vector<size_t> joined_across_edge(
    const std::vector<Point>& points,
    const std::vector<size_t>& f,
    const std::vector<size_t>& g,
    size_t i
) {

    // joined: f from v round to u, then g's corners strictly between u and v
    const size_t nf = f.size();
    const size_t ng = g.size();
    const size_t at_v = (i + 1) % nf;
    const size_t at_u = static_cast<size_t>(std::find(g.begin(), g.end(), f[i]) - g.begin());
    std::vector<size_t> joined;

    for (size_t k = 0; k < nf; k++)
        joined.push_back(f[(at_v + k) % nf]);

    for (size_t k = 1; k + 1 < ng; k++)
        joined.push_back(g[(at_u + k) % ng]);

    // corners: the straight ones dropped
    const size_t m = joined.size();
    std::vector<size_t> corners;

    for (size_t k = 0; k < m; k++) {
        const Vector in = points[joined[k]] - points[joined[(k + m - 1) % m]];
        const Vector out = points[joined[(k + 1) % m]] - points[joined[k]];

        if (in.cross(out).magnitude() > 1e-6 * in.magnitude() * out.magnitude())
            corners.push_back(joined[k]);
    }

    return corners;
}

/// The mesh with every two coplanar faces sharing an edge joined, again and again, and the points no face uses left out.
inline Mesh merge_coplanar(const Mesh& mesh) {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = mesh.to_vertices_and_faces();
    const std::vector<Point>& points = data.first;
    std::vector<std::vector<size_t>> faces = data.second;
    bool merged = true;

    // merge: one pair of coplanar neighbours joined at a time until none is left
    while (merged) {
        merged = false;
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
                const Vector normal = newell_normal(points, faces[f]);

                if (normal.dot(newell_normal(points, faces[g])) < 1.0 - 1e-9)
                    continue;

                bool coplanar = true;

                for (const size_t k : faces[g])
                    coplanar = coplanar && std::abs((points[k] - points[u]).dot(normal)) < 0.01;

                if (!coplanar)
                    continue;

                faces[f] = joined_across_edge(points, faces[f], faces[g], i);
                faces.erase(faces.begin() + static_cast<std::ptrdiff_t>(g));
                merged = true;
            }
    }

    // used: the points some face still uses, renumbered
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
