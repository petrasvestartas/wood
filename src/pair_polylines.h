#pragma once
// Pair up the polylines of an OBJ into (bottom, top) plate couples.
//
// This used to live in session_cpp as file_obj::pair_polylines. It was dropped
// there in 150fce85, "drop pair_polylines (joinery pairing belongs to the wood
// project); no caller remained" - the removal was right about the ownership but
// wood did still call it, from main_export_xml and main_annen_faces, so both
// stopped compiling. Same algorithm, now owned here.
//
// An OBJ of plate outlines carries each plate as two separate loops, its bottom
// and its top face, in no particular order. Pairing looks for the nearest
// unpaired partner whose plane is roughly parallel, which is what "the other
// face of the same plate" looks like geometrically.

#include "aabb.h"
#include "element.h"
#include "polyline.h"
#include "spatial_rtree.h"

#include <cmath>
#include <utility>
#include <vector>

namespace wood {

/// Returns index pairs into `polylines`; unpaired outlines are simply omitted.
/// `search_radius` inflates each outline's AABB to bound the partner search.
inline std::vector<std::pair<int, int>> pair_polylines(
    const std::vector<session_cpp::Polyline>& polylines,
    double search_radius = 500.0)
{
    using namespace session_cpp;

    size_t NP = polylines.size();
    std::vector<Point>  centroids(NP);
    std::vector<Vector> normals(NP);
    std::vector<AABB>   aabbs(NP);

    // A closed outline repeats its first point; that duplicate would drag the
    // centroid toward the seam, so drop it before averaging.
    auto open_pts = [](const Polyline& pl) {
        auto pts = pl.get_points();
        if (pts.size() > 3) {
            auto& f = pts.front();
            auto& l = pts.back();
            if (std::abs(f[0]-l[0]) < 1e-6 && std::abs(f[1]-l[1]) < 1e-6 && std::abs(f[2]-l[2]) < 1e-6)
                pts.pop_back();
        }
        return pts;
    };

    SpatialRTree<int, double, 3> tree;
    for (size_t i = 0; i < NP; i++) {
        auto pts = open_pts(polylines[i]);
        double cx = 0, cy = 0, cz = 0;
        for (auto& p : pts) { cx += p[0]; cy += p[1]; cz += p[2]; }
        centroids[i] = Point(cx/pts.size(), cy/pts.size(), cz/pts.size());
        normals[i]   = ElementPlate::polygon_normal(pts);
        aabbs[i]     = AABB::from_polyline(polylines[i], search_radius);
        double mn[3] = {aabbs[i].cx-aabbs[i].hx, aabbs[i].cy-aabbs[i].hy, aabbs[i].cz-aabbs[i].hz};
        double mx[3] = {aabbs[i].cx+aabbs[i].hx, aabbs[i].cy+aabbs[i].hy, aabbs[i].cz+aabbs[i].hz};
        tree.insert(mn, mx, (int)i);
    }

    std::vector<bool> paired(NP, false);
    std::vector<std::pair<int, int>> pairs;
    for (size_t i = 0; i < NP; i++) {
        if (paired[i]) continue;
        int    best   = -1;
        double best_d = 1e30;
        double mn[3] = {aabbs[i].cx-aabbs[i].hx, aabbs[i].cy-aabbs[i].hy, aabbs[i].cz-aabbs[i].hz};
        double mx[3] = {aabbs[i].cx+aabbs[i].hx, aabbs[i].cy+aabbs[i].hy, aabbs[i].cz+aabbs[i].hz};
        tree.search(mn, mx, [&](int j) {
            if (j <= (int)i || paired[j]) return true;
            // |dot| rather than dot: the two faces of a plate point away from
            // each other, so their normals are antiparallel as often as not.
            double dot = normals[i][0]*normals[j][0] + normals[i][1]*normals[j][1] + normals[i][2]*normals[j][2];
            if (std::abs(dot) < 0.7) return true;
            double dx = centroids[i][0]-centroids[j][0];
            double dy = centroids[i][1]-centroids[j][1];
            double dz = centroids[i][2]-centroids[j][2];
            double d  = dx*dx + dy*dy + dz*dz;
            if (d < best_d) { best_d = d; best = j; }
            return true;
        });
        if (best >= 0) { pairs.push_back({(int)i, best}); paired[i] = paired[best] = true; }
    }
    return pairs;
}

}  // namespace wood
