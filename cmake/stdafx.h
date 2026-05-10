#pragma once

// std library
#include <stdlib.h>
#include <vector>
#include <array>
#include <map>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <numeric>
#include <limits>
#include <chrono>
#include <float.h>
#include <inttypes.h>
#include <cstring>
#include <set>
#include <unordered_set>
#include <list>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>


// clipper2
#include <clipper2/clipper.h>

// tinyply
#include <tinyply/tinyply.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BOOST
// CGAL
// EIGEN
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// https://github.com/CGAL/cgal/discussions/6946
//  CGAL
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/intersections.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Bbox_3.h>
#include <CGAL/Plane_3.h>
#include <CGAL/Boolean_set_operations_2.h>

// CDT, Surface_mesh/PMP, skeleton — moved to their respective headers:
//   cgal_mesh_boolean.h, database_writer.h, cgal_skeleton.h

// RTREE
#include "src/wood/include/rtree.h"

// session_cpp — lightweight geometry types replacing IK:: in the public API
#include "point.h"
#include "vector.h"
#include "plane.h"
#include "boundingbox.h"
#include "xform.h"
#include "line.h"
#include "polyline.h"
#include "intersection.h"

// session_cpp utility headers (replaces deleted cgal_xxx_util files)
#include "vector_util.h"
#include "polyline_util.h"
#include "box_util.h"
#include "intersection_util.h"

using IK = CGAL::Exact_predicates_inexact_constructions_kernel;
using EK = CGAL::Exact_predicates_exact_constructions_kernel;
typedef CGAL::Cartesian_converter<IK, EK> IK_to_EK;
typedef CGAL::Cartesian_converter<EK, IK> EK_to_IK;
using Polyline = std::vector<session_cpp::Point>;
using Polylines = std::vector<Polyline>;

// ── CGAL ↔ session_cpp bridge (used inside cgal_*.cpp and wood_*.cpp) ────────
inline IK::Point_3         to_cgal_pt   (const session_cpp::Point&  p) { return {p[0], p[1], p[2]}; }
inline session_cpp::Point  to_sc_pt     (const IK::Point_3&          p) { return {p.x(), p.y(), p.z()}; }
// Plane: IK::Plane_3(a,b,c,d) means normal=(a,b,c), dist=-d  ↔  session_cpp::Plane z_axis=normal
inline IK::Plane_3   to_cgal_plane(const session_cpp::Plane& p) { return {p.a(), p.b(), p.c(), p.d()}; }
inline session_cpp::BoundingBox sc_bbox_polyline(const std::vector<session_cpp::Point>& poly) {
    return session_cpp::BoundingBox::from_points(poly);
}
// Mesh, PMP, FaceInfo2/CDT typedefs, Skeleton typedefs — moved to their respective headers


// Wood Library Utilities
#include "wood_globals.h"

// Order Matters
// #include "cgal_print.h"

#include "cgal_box_search.h"
#include "cgal_inscribe_util.h"

// cgal_xform_util removed — all functionality lives in session_cpp::Xform static methods
// cgal_vector_util removed — all functionality lives in session_cpp vector_util.h
// cgal_box_util removed — OBB functions live in session_cpp::obb (box_util.h)
// cgal_plane_util removed — all functionality lives in session_cpp::Plane methods
// cgal_polyline_util removed — all functions live in session_cpp free functions (polyline_util.h)
// cgal_rectangle_util removed — quick_hull/bounding_rectangle/grid in session_cpp::Polyline
// cgal_math_util ported to session_cpp::tolerance.h (unique_from_two_int, triangle_edge_by_angle, etc.)

#include "cgal_polyline_mesh_util.h"

#include "clipper_util.h"

// wood_joint_util: polyline_plane_cross_joint — depends on collider::clipper_util
#include "wood_joint_util.h"

#include "rtree_util.h"

// Deleted cgal_* utility files (migrated to session_cpp or inlined in stdafx.h):
//   cgal_math_util, cgal_vector_util, cgal_xform_util, cgal_plane_util,
//   cgal_polyline_util, cgal_box_util, cgal_rectangle_util, cgal_intersection_util
// Ported cgal_* files (now session_cpp-based, no CGAL in their public API):
//   cgal_inscribe_util (polylabel + grid rect approx), cgal_polyline_mesh_util (session_cpp CDT),
//   cgal_box_search (session_cpp BVH broadphase)
// Remaining cgal_* files (intentionally CGAL, no change):
//   cgal_mesh_boolean, cgal_skeleton

#include "cgal_skeleton.h"

// Display
static std::vector<std::vector<session_cpp::Point>> viewer_polylines;
