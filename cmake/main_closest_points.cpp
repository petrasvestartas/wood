#include "stdafx.h"
#include "wood_globals.h"
#include "wood_xml.h"
#include "bvh.h"
#include "aabb.h"
#include "session.h"

#include <chrono>
#include <iomanip>
#include <numeric>

namespace closest_points_joints
{

// ---------------------------------------------------------------------------
// Internal types
// ---------------------------------------------------------------------------

struct SegmentRecord
{
    int elem_id; // element index (0-based)
    int poly_id; // 0 = bottom polyline, 1 = top polyline
    int edge_id; // edge index within the polyline
    IK::Segment_3 seg;
};

using JointTypes = std::vector<std::vector<int>>;

// ---------------------------------------------------------------------------
// Phase A — build per-segment spatial data
// ---------------------------------------------------------------------------

static void
build_segments (const std::vector<CGAL_Polyline> &flat, double dist, std::vector<SegmentRecord> &segs, std::vector<session_cpp::BvhAABB> &aabbs)
{
    const int n = static_cast<int> (flat.size () / 2);
    for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < 2; j++)
                {
                    const CGAL_Polyline &poly = flat[static_cast<size_t> (i * 2 + j)];
                    const int np = static_cast<int> (poly.size ());
                    for (int k = 0; k < np - 1; k++)
                        {
                            const IK::Point_3 &p0 = poly[k];
                            const IK::Point_3 &p1 = poly[k + 1];

                            double xmin = std::min (p0.x (), p1.x ()) - dist;
                            double xmax = std::max (p0.x (), p1.x ()) + dist;
                            double ymin = std::min (p0.y (), p1.y ()) - dist;
                            double ymax = std::max (p0.y (), p1.y ()) + dist;
                            double zmin = std::min (p0.z (), p1.z ()) - dist;
                            double zmax = std::max (p0.z (), p1.z ()) + dist;

                            session_cpp::BvhAABB bb;
                            bb.cx = (xmin + xmax) * 0.5;
                            bb.cy = (ymin + ymax) * 0.5;
                            bb.cz = (zmin + zmax) * 0.5;
                            bb.hx = (xmax - xmin) * 0.5;
                            bb.hy = (ymax - ymin) * 0.5;
                            bb.hz = (zmax - zmin) * 0.5;

                            segs.push_back ({ i, j, k, IK::Segment_3 (p0, p1) });
                            aabbs.push_back (bb);
                        }
                }
        }
}

// ---------------------------------------------------------------------------
// Precise check: squared distance from point to segment
// ---------------------------------------------------------------------------

static inline double
sq_dist_point_seg (const IK::Point_3 &p, const IK::Segment_3 &seg)
{
    return CGAL::to_double (CGAL::squared_distance (p, seg));
}

// ---------------------------------------------------------------------------
// Allocate output joint_types table matching existing wood convention
// ---------------------------------------------------------------------------

static JointTypes
make_joint_types (const std::vector<CGAL_Polyline> &flat)
{
    const int n = static_cast<int> (flat.size () / 2);
    JointTypes jt (static_cast<size_t> (n));
    for (int i = 0; i < n; i++)
        {
            // size = n_points_in_bottom_polyline + 1
            // index 0 = face assignment bottom, index 1 = face assignment top,
            // index 2+ = edge assignments (edge_id + 2)
            jt[i].assign (flat[static_cast<size_t> (i * 2)].size () + 1, -1);
        }
    return jt;
}

// ---------------------------------------------------------------------------
// Implementation A — per-segment RTree
// ---------------------------------------------------------------------------

static JointTypes
search_rtree (const std::vector<SegmentRecord> &segs, const std::vector<session_cpp::BvhAABB> &aabbs, const std::vector<IK::Point_3> &pts, const std::vector<int> &types, const std::vector<CGAL_Polyline> &flat)
{
    JointTypes jt = make_joint_types (flat);
    const double dist = wood::GLOBALS::DISTANCE;
    const double dist_sq = wood::GLOBALS::DISTANCE_SQUARED;

    // Build RTree — one entry per segment
    RTree<size_t, double, 3> tree;
    for (size_t s = 0; s < segs.size (); s++)
        {
            const auto &bb = aabbs[s];
            double mn[3] = { bb.cx - bb.hx, bb.cy - bb.hy, bb.cz - bb.hz };
            double mx[3] = { bb.cx + bb.hx, bb.cy + bb.hy, bb.cz + bb.hz };
            tree.Insert (mn, mx, s);
        }

    for (size_t qi = 0; qi < pts.size (); qi++)
        {
            const IK::Point_3 &p = pts[qi];
            const int t = types[qi];

            double mn[3] = { p.x () - dist, p.y () - dist, p.z () - dist };
            double mx[3] = { p.x () + dist, p.y () + dist, p.z () + dist };

            auto cb = [&] (size_t s) -> bool {
                if (sq_dist_point_seg (p, segs[s].seg) < dist_sq)
                    {
                        int col = (t < 0) ? segs[s].poly_id : segs[s].edge_id + 2;
                        int val = (t < -100) ? 0 : std::abs (t);
                        auto &row = jt[static_cast<size_t> (segs[s].elem_id)];
                        if (col < static_cast<int> (row.size ()))
                            row[static_cast<size_t> (col)] = val;
                    }
                return true;
            };
            tree.Search (mn, mx, cb);
        }

    return jt;
}

// ---------------------------------------------------------------------------
// Implementation B — AABBTree (session_cpp)
// ---------------------------------------------------------------------------

static JointTypes
search_aabb (const std::vector<SegmentRecord> &segs, const std::vector<session_cpp::BvhAABB> &aabbs, const std::vector<IK::Point_3> &pts, const std::vector<int> &types, const std::vector<CGAL_Polyline> &flat)
{
    JointTypes jt = make_joint_types (flat);
    const double dist = wood::GLOBALS::DISTANCE;
    const double dist_sq = wood::GLOBALS::DISTANCE_SQUARED;

    session_cpp::AABBTree tree;
    tree.build (aabbs.data (), aabbs.size ());

    for (size_t qi = 0; qi < pts.size (); qi++)
        {
            const IK::Point_3 &p = pts[qi];
            const int t = types[qi];

            session_cpp::BvhAABB q;
            q.cx = p.x (); q.cy = p.y (); q.cz = p.z ();
            q.hx = dist;   q.hy = dist;   q.hz = dist;

            for (int s : tree.query_aabb (q))
                {
                    if (sq_dist_point_seg (p, segs[static_cast<size_t> (s)].seg) < dist_sq)
                        {
                            const auto &sr = segs[static_cast<size_t> (s)];
                            int col = (t < 0) ? sr.poly_id : sr.edge_id + 2;
                            int val = (t < -100) ? 0 : std::abs (t);
                            auto &row = jt[static_cast<size_t> (sr.elem_id)];
                            if (col < static_cast<int> (row.size ()))
                                row[static_cast<size_t> (col)] = val;
                        }
                }
        }

    return jt;
}

// ---------------------------------------------------------------------------
// Implementation C — BVH (session_cpp)
// ---------------------------------------------------------------------------

static JointTypes
search_bvh (const std::vector<SegmentRecord> &segs, const std::vector<session_cpp::BvhAABB> &aabbs, const std::vector<IK::Point_3> &pts, const std::vector<int> &types, const std::vector<CGAL_Polyline> &flat)
{
    JointTypes jt = make_joint_types (flat);
    const double dist = wood::GLOBALS::DISTANCE;
    const double dist_sq = wood::GLOBALS::DISTANCE_SQUARED;

    // Compute world size from segment centers
    double max_ext = 0.0;
    for (const auto &bb : aabbs)
        {
            max_ext = std::max ({ max_ext, std::abs (bb.cx + bb.hx), std::abs (bb.cx - bb.hx), std::abs (bb.cy + bb.hy), std::abs (bb.cy - bb.hy), std::abs (bb.cz + bb.hz), std::abs (bb.cz - bb.hz) });
        }
    double ws = std::max (max_ext * 2.2, 10.0);

    session_cpp::BVH bvh (ws);
    bvh.build_from_aabbs (aabbs.data (), aabbs.size (), ws);

    for (size_t qi = 0; qi < pts.size (); qi++)
        {
            const IK::Point_3 &p = pts[qi];
            const int t = types[qi];

            session_cpp::BvhAABB q;
            q.cx = p.x (); q.cy = p.y (); q.cz = p.z ();
            q.hx = dist;   q.hy = dist;   q.hz = dist;

            for (int s : bvh.query_aabb (q))
                {
                    if (sq_dist_point_seg (p, segs[static_cast<size_t> (s)].seg) < dist_sq)
                        {
                            const auto &sr = segs[static_cast<size_t> (s)];
                            int col = (t < 0) ? sr.poly_id : sr.edge_id + 2;
                            int val = (t < -100) ? 0 : std::abs (t);
                            auto &row = jt[static_cast<size_t> (sr.elem_id)];
                            if (col < static_cast<int> (row.size ()))
                                row[static_cast<size_t> (col)] = val;
                        }
                }
        }

    return jt;
}

// ---------------------------------------------------------------------------
// Correctness helpers
// ---------------------------------------------------------------------------

static bool
results_match (const JointTypes &a, const JointTypes &b, const char *label)
{
    if (a.size () != b.size ())
        {
            std::cerr << label << ": FAIL (element count differs: " << a.size () << " vs " << b.size () << ")\n";
            return false;
        }
    for (size_t i = 0; i < a.size (); i++)
        {
            if (a[i] != b[i])
                {
                    std::cerr << label << ": FAIL at element " << i << "\n";
                    return false;
                }
        }
    std::cout << label << " match: OK\n";
    return true;
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void
run_benchmark (session_cpp::Session &session)
{
    std::cerr << "\n=== closest_points_joints benchmark ===\n";

    // 1. Load hilti XML dataset
    wood::xml::path_and_file_for_input_polylines = wood::GLOBALS::DATA_SET_INPUT_FOLDER + "type_plates_name_side_to_side_edge_inplane_hilti.xml";
    std::vector<CGAL_Polyline> flat;
    if (!wood::xml::read_xml_polylines (flat) || flat.empty ())
        {
            std::cerr << "closest_points_joints: failed to load hilti XML — skipping benchmark\n";
            return;
        }
    const int n_elements = static_cast<int> (flat.size () / 2);
    std::cerr << "Loaded " << n_elements << " elements (" << flat.size () << " polylines)\n";

    // 2. Generate test query points
    //    a) midpoints of all edges (positive type → edge assignment)
    //    b) centroid of each element (negative type → face assignment)
    std::vector<IK::Point_3> pts;
    std::vector<int> types;

    for (int i = 0; i < n_elements; i++)
        {
            for (int j = 0; j < 2; j++)
                {
                    const CGAL_Polyline &poly = flat[static_cast<size_t> (i * 2 + j)];
                    for (size_t k = 0; k + 1 < poly.size (); k++)
                        {
                            pts.push_back (IK::Point_3 ((poly[k].x () + poly[k + 1].x ()) * 0.5, (poly[k].y () + poly[k + 1].y ()) * 0.5, (poly[k].z () + poly[k + 1].z ()) * 0.5));
                            types.push_back (1); // positive → edge assignment
                        }
                }
            // centroid of bottom polyline → face assignment
            if (!flat[static_cast<size_t> (i * 2)].empty ())
                {
                    double sx = 0, sy = 0, sz = 0;
                    for (const auto &p : flat[static_cast<size_t> (i * 2)])
                        {
                            sx += p.x ();
                            sy += p.y ();
                            sz += p.z ();
                        }
                    double np = static_cast<double> (flat[static_cast<size_t> (i * 2)].size ());
                    pts.push_back (IK::Point_3 (sx / np, sy / np, sz / np));
                    types.push_back (-1); // negative → face assignment
                }
        }
    std::cerr << "Generated " << pts.size () << " query points\n";

    // 3. Build shared segment dictionary
    std::vector<SegmentRecord> segs;
    std::vector<session_cpp::BvhAABB> aabbs;
    build_segments (flat, wood::GLOBALS::DISTANCE, segs, aabbs);
    std::cerr << "Built " << segs.size () << " segment records\n";

    // 4. Time each search method
    using Clock = std::chrono::high_resolution_clock;
    using Ms = std::chrono::duration<double, std::milli>;

    JointTypes r_rtree, r_aabb, r_bvh;
    double ms_rtree, ms_aabb, ms_bvh;

    {
        auto t0 = Clock::now ();
        r_rtree = search_rtree (segs, aabbs, pts, types, flat);
        ms_rtree = Ms (Clock::now () - t0).count ();
    }
    {
        auto t0 = Clock::now ();
        r_aabb = search_aabb (segs, aabbs, pts, types, flat);
        ms_aabb = Ms (Clock::now () - t0).count ();
    }
    {
        auto t0 = Clock::now ();
        r_bvh = search_bvh (segs, aabbs, pts, types, flat);
        ms_bvh = Ms (Clock::now () - t0).count ();
    }

    // 5. Correctness checks
    bool ok_aabb = results_match (r_rtree, r_aabb, "AABB");
    bool ok_bvh  = results_match (r_rtree, r_bvh, "BVH");

    // Sanity: at least some hits
    auto any_hit = [] (const JointTypes &jt) {
        for (const auto &row : jt)
            for (int v : row)
                if (v >= 0) return true;
        return false;
    };
    if (!any_hit (r_rtree)) std::cerr << "WARNING: RTree produced no hits\n";
    if (!any_hit (r_aabb))  std::cerr << "WARNING: AABB  produced no hits\n";
    if (!any_hit (r_bvh))   std::cerr << "WARNING: BVH   produced no hits\n";

    // 6. Print timing table
    std::cout << "\n";
    std::cout << "+------------------------+----------+----------+\n";
    std::cout << "| Method                 | Time(ms) | vs RTree |\n";
    std::cout << "+------------------------+----------+----------+\n";

    auto row = [&] (const char *name, double ms) {
        double ratio = (ms_rtree > 0) ? ms / ms_rtree : 1.0;
        std::cout << "| " << std::left << std::setw (22) << name << " | "
                  << std::right << std::fixed << std::setprecision (2) << std::setw (8) << ms << " | "
                  << std::setw (7) << std::setprecision (2) << ratio << "x |\n";
    };

    row ("RTree (per-segment)", ms_rtree);
    row ("AABBTree (session)",  ms_aabb);
    row ("BVH     (session)",   ms_bvh);

    std::cout << "+------------------------+----------+----------+\n";

    (void)ok_aabb;
    (void)ok_bvh;

    // 7. Session output — named points at edge midpoints from r_rtree hits
    {
        // Build reverse map (elem_id * 10000 + edge_id) → first seg index (poly_id=0 preferred)
        std::unordered_map<int, size_t> edge_to_seg;
        for (size_t s = 0; s < segs.size (); s++)
            {
                int key = segs[s].elem_id * 10000 + segs[s].edge_id;
                if (edge_to_seg.find (key) == edge_to_seg.end ())
                    edge_to_seg[key] = s;
            }

        auto layer = session.add_group ("joint_hits");
        int count  = 0;
        for (size_t i = 0; i < r_rtree.size (); i++)
            {
                for (size_t j = 2; j < r_rtree[i].size (); j++)
                    {
                        if (r_rtree[i][j] < 0)
                            continue;
                        int edge_id = static_cast<int> (j) - 2;
                        int key     = static_cast<int> (i) * 10000 + edge_id;
                        auto it     = edge_to_seg.find (key);
                        if (it == edge_to_seg.end ())
                            continue;
                        const auto &seg = segs[it->second].seg;
                        double mx       = (seg.source ().x () + seg.target ().x ()) * 0.5;
                        double my       = (seg.source ().y () + seg.target ().y ()) * 0.5;
                        double mz       = (seg.source ().z () + seg.target ().z ()) * 0.5;
                        auto pt         = std::make_shared<session_cpp::Point> (mx, my, mz);
                        pt->name        = "e" + std::to_string (i) + "_edge" + std::to_string (edge_id);
                        session.add (session.add_point (pt), layer);
                        ++count;
                    }
            }
        std::cerr << "Added " << count << " joint_hit points to session\n";
    }
}

} // namespace closest_points_joints
