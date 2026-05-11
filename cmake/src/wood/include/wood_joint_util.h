#pragma once

// ── wood_joint_util.h ─────────────────────────────────────────────────────────
// Wood-project-only geometry utilities that depend on collider::clipper_util
// (a wood dependency) and therefore cannot live in the session_cpp kernel.
// Included at the end of stdafx.h, after clipper_util.h is already in scope.

namespace session_cpp {

// polyline_plane_cross_joint — inlined using sc Intersection + sc are_points_inside
inline bool polyline_plane_cross_joint (std::vector<Point> &c0, std::vector<Point> &c1, Plane &p0, Plane &p1, Line &line_out, std::pair<int, int> &pair)
{
    std::vector<Point> pts0, pts1;
    std::vector<int> edge_ids_0, edge_ids_1;
    if (!Intersection::polyline_plane (c0, p1, pts0, edge_ids_0))
        return false;
    if (!Intersection::polyline_plane (c1, p0, pts1, edge_ids_1))
        return false;
    if (pts0.size () < 2 || pts1.size () < 2)
        return false;

    std::vector<int> ID1, ID0;
    int count0 = collider::clipper_util::are_points_inside (c0, p0, pts1, ID1);
    int count1 = collider::clipper_util::are_points_inside (c1, p1, pts0, ID0);

    if (count0 == 0 && count1 == 0)
        return false;
    else if (std::abs (count0 - count1) == 2)
        {
            if (count0 == 2)
                {
                    line_out = Line::from_points (pts0[0], pts0[1]);
                    pair = { edge_ids_0[0], edge_ids_0[1] };
                }
            else
                {
                    line_out = Line::from_points (pts1[0], pts1[1]);
                    pair = { edge_ids_1[0], edge_ids_1[1] };
                }
            return true;
        }
    else if (count0 == 1 && count1 == 1)
        {
            line_out = Line::from_points (pts0[ID0[0]], pts1[ID1[0]]);
            pair = { edge_ids_0[ID0[0]], edge_ids_1[ID1[0]] };
            return true;
        }
    else if (count0 > 1 || count1 > 1)
        {
            std::vector<Point> all_pts;
            all_pts.reserve (ID0.size () + ID1.size ());
            for (int i : ID0)
                all_pts.push_back (pts0[i]);
            for (int i : ID1)
                all_pts.push_back (pts1[i]);

            // Manual bounding box (replaces CGAL::bbox_3)
            double xmin = all_pts[0][0], ymin = all_pts[0][1], zmin = all_pts[0][2];
            double xmax = xmin, ymax = ymin, zmax = zmin;
            for (auto &p : all_pts)
                {
                    xmin = std::min (xmin, p[0]); ymin = std::min (ymin, p[1]); zmin = std::min (zmin, p[2]);
                    xmax = std::max (xmax, p[0]); ymax = std::max (ymax, p[1]); zmax = std::max (zmax, p[2]);
                }
            Point p0_bb = { xmin, ymin, zmin };
            Point p1_bb = { xmax, ymax, zmax };
            line_out = Line::from_points (p0_bb, p1_bb);

            int e0 = 0, e1 = 0;
            auto sq_dist = [] (const Point &a, const Point &b)
            {
                double dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
                return dx * dx + dy * dy + dz * dz;
            };
            for (int i : ID0)
                if (sq_dist (p0_bb, pts0[i]) < 0.001 || sq_dist (p1_bb, pts0[i]) < 0.001)
                    {
                        e0 = edge_ids_0[i];
                        break;
                    }
            for (int i : ID1)
                if (sq_dist (p0_bb, pts1[i]) < 0.001 || sq_dist (p1_bb, pts1[i]) < 0.001)
                    {
                        e1 = edge_ids_1[i];
                        break;
                    }
            pair = { e0, e1 };
            return true;
        }
    return false;
}

} // namespace session_cpp
