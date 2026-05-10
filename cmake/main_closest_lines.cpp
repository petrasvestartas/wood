#include "stdafx.h"
#include "wood_globals.h"
#include "wood_xml.h"
#include "session.h"

#include <chrono>
#include <iomanip>

namespace closest_lines_insertion
{

// ---------------------------------------------------------------------------
// Internal types
// ---------------------------------------------------------------------------

struct InsLine
{
    session_cpp::Point  from;
    session_cpp::Point  to;
    session_cpp::Vector dir; // unit vector: (to - from) / |to - from|
};

struct SegmentRecord
{
    int               elem_id;
    int               poly_id; // 0 = bottom polyline, 1 = top polyline
    int               edge_id; // edge index within polyline
    session_cpp::Line seg;
};

// ---------------------------------------------------------------------------
// Phase A — build per-segment spatial data (same as closest_points_joints)
// ---------------------------------------------------------------------------

static void
build_segments (const std::vector<Polyline> &flat, double dist, std::vector<SegmentRecord> &segs, std::vector<session_cpp::BvhAABB> &aabbs)
{
    const int n = static_cast<int> (flat.size () / 2);
    for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < 2; j++)
                {
                    const Polyline &poly = flat[static_cast<size_t> (i * 2 + j)];
                    const int       np   = static_cast<int> (poly.size ());
                    for (int k = 0; k < np - 1; k++)
                        {
                            const session_cpp::Point &p0 = poly[k];
                            const session_cpp::Point &p1 = poly[k + 1];

                            double xmin = std::min (p0[0], p1[0]) - dist;
                            double xmax = std::max (p0[0], p1[0]) + dist;
                            double ymin = std::min (p0[1], p1[1]) - dist;
                            double ymax = std::max (p0[1], p1[1]) + dist;
                            double zmin = std::min (p0[2], p1[2]) - dist;
                            double zmax = std::max (p0[2], p1[2]) + dist;

                            session_cpp::BvhAABB bb;
                            bb.cx = (xmin + xmax) * 0.5;
                            bb.cy = (ymin + ymax) * 0.5;
                            bb.cz = (zmin + zmax) * 0.5;
                            bb.hx = (xmax - xmin) * 0.5;
                            bb.hy = (ymax - ymin) * 0.5;
                            bb.hz = (zmax - zmin) * 0.5;

                            segs.push_back ({ i, j, k, session_cpp::Line::from_points (p0, p1) });
                            aabbs.push_back (bb);
                        }
                }
        }
}

// ---------------------------------------------------------------------------
// Precise check: squared distance from point to segment
// ---------------------------------------------------------------------------

static inline double
sq_dist_point_seg (const session_cpp::Point &p, const session_cpp::Line &seg)
{
    auto   cp = seg.closest_point (p);
    double dx = cp[0] - p[0], dy = cp[1] - p[1], dz = cp[2] - p[2];
    return dx * dx + dy * dy + dz * dz;
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void
run (session_cpp::Session &session)
{
    std::cerr << "\n=== closest_lines_insertion ===\n";

    // 1. Load hilti XML dataset
    wood::xml::path_and_file_for_input_polylines = wood::GLOBALS::DATA_SET_INPUT_FOLDER + "type_plates_name_side_to_side_edge_inplane_hilti.xml";
    std::vector<Polyline> flat;
    if (!wood::xml::read_xml_polylines (flat) || flat.empty ())
        {
            std::cerr << "closest_lines_insertion: failed to load hilti XML — skipping\n";
            return;
        }
    const int n_elements = static_cast<int> (flat.size () / 2);
    std::cerr << "Loaded " << n_elements << " elements\n";

    // 2. Build segment records + RTree
    const double dist    = wood::GLOBALS::DISTANCE;
    const double dist_sq = wood::GLOBALS::DISTANCE_SQUARED;

    std::vector<SegmentRecord>        segs;
    std::vector<session_cpp::BvhAABB> aabbs;
    build_segments (flat, dist, segs, aabbs);

    RTree<size_t, double, 3> tree;
    for (size_t s = 0; s < segs.size (); s++)
        {
            const auto &bb = aabbs[s];
            double      mn[3] = { bb.cx - bb.hx, bb.cy - bb.hy, bb.cz - bb.hz };
            double      mx[3] = { bb.cx + bb.hx, bb.cy + bb.hy, bb.cz + bb.hz };
            tree.Insert (mn, mx, s);
        }

    // 3. Generate synthetic test lines — one per segment, from midpoint along edge direction
    //    (guarantees From hits; To is 50 units away, won't hit anything)
    std::vector<InsLine> lines;
    lines.reserve (segs.size ());
    for (const auto &sr : segs)
        {
            const auto &s = sr.seg;
            session_cpp::Point from (
                (s.start ()[0] + s.end ()[0]) * 0.5,
                (s.start ()[1] + s.end ()[1]) * 0.5,
                (s.start ()[2] + s.end ()[2]) * 0.5);

            auto   d   = s.to_vector ();
            double len = std::sqrt (d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            if (len < 1e-10)
                continue;
            session_cpp::Vector dir = d * (1.0 / len);
            session_cpp::Point  to (
                from[0] + dir[0] * 50.0,
                from[1] + dir[1] * 50.0,
                from[2] + dir[2] * 50.0);

            lines.push_back ({ from, to, dir });
        }
    std::cerr << "Generated " << lines.size () << " test lines\n";

    // 4. Allocate insertion-vectors table (same dimensions as joint_types)
    std::vector<std::vector<session_cpp::Vector>> vectors (static_cast<size_t> (n_elements));
    for (int i = 0; i < n_elements; i++)
        {
            int sz = static_cast<int> (flat[static_cast<size_t> (i * 2)].size ()) + 1;
            vectors[static_cast<size_t> (i)].assign (static_cast<size_t> (sz), session_cpp::Vector (0, 0, 0));
        }

    // 5. RTree search for each line's From and To
    for (const auto &l : lines)
        {
            // From search → assign +dir
            {
                double mn[3] = { l.from[0] - dist, l.from[1] - dist, l.from[2] - dist };
                double mx[3] = { l.from[0] + dist, l.from[1] + dist, l.from[2] + dist };
                auto   cb    = [&] (size_t s) -> bool {
                    if (sq_dist_point_seg (l.from, segs[s].seg) < dist_sq)
                        {
                            int   col = segs[s].edge_id + 2;
                            auto &row = vectors[static_cast<size_t> (segs[s].elem_id)];
                            if (col < static_cast<int> (row.size ()))
                                row[static_cast<size_t> (col)] = l.dir;
                        }
                    return true;
                };
                tree.Search (mn, mx, cb);
            }

            // To search → assign -dir
            {
                double mn[3] = { l.to[0] - dist, l.to[1] - dist, l.to[2] - dist };
                double mx[3] = { l.to[0] + dist, l.to[1] + dist, l.to[2] + dist };
                auto   cb    = [&] (size_t s) -> bool {
                    if (sq_dist_point_seg (l.to, segs[s].seg) < dist_sq)
                        {
                            int   col = segs[s].edge_id + 2;
                            auto &row = vectors[static_cast<size_t> (segs[s].elem_id)];
                            if (col < static_cast<int> (row.size ()))
                                row[static_cast<size_t> (col)] = l.dir * -1.0;
                        }
                    return true;
                };
                tree.Search (mn, mx, cb);
            }
        }

    // 6. Session output — Line arrows at matched edges
    {
        // Build reverse map (elem_id * 10000 + edge_id) → first seg index with poly_id==0
        std::unordered_map<int, size_t> edge_to_seg;
        for (size_t s = 0; s < segs.size (); s++)
            {
                if (segs[s].poly_id != 0)
                    continue;
                int key = segs[s].elem_id * 10000 + segs[s].edge_id;
                if (edge_to_seg.find (key) == edge_to_seg.end ())
                    edge_to_seg[key] = s;
            }

        auto layer       = session.add_group ("closest_lines_insertion");
        int  count       = 0;
        const double scale = 50.0;

        for (int i = 0; i < n_elements; i++)
            {
                for (int j = 2; j < static_cast<int> (vectors[static_cast<size_t> (i)].size ()); j++)
                    {
                        const auto &v = vectors[static_cast<size_t> (i)][static_cast<size_t> (j)];
                        if (v[0] * v[0] + v[1] * v[1] + v[2] * v[2] < 1e-20)
                            continue;

                        int edge_id = j - 2;
                        int key     = i * 10000 + edge_id;
                        auto it     = edge_to_seg.find (key);
                        if (it == edge_to_seg.end ())
                            continue;

                        const auto &seg = segs[it->second].seg;
                        double      mx  = (seg.start ()[0] + seg.end ()[0]) * 0.5;
                        double      my  = (seg.start ()[1] + seg.end ()[1]) * 0.5;
                        double      mz  = (seg.start ()[2] + seg.end ()[2]) * 0.5;

                        auto lobj  = std::make_shared<session_cpp::Line> (
                            mx, my, mz,
                            mx + v[0] * scale,
                            my + v[1] * scale,
                            mz + v[2] * scale);
                        lobj->name = "e" + std::to_string (i) + "_edge" + std::to_string (edge_id);
                        session.add (session.add_line (lobj), layer);
                        ++count;
                    }
            }

        std::cerr << "Assigned " << count << " insertion vectors\n";
    }
}

} // namespace closest_lines_insertion
