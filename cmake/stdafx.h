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

#include <boost/exception/diagnostic_information.hpp>
// https://github.com/CGAL/cgal/discussions/6946
//  CGAL
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/intersections.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Bbox_3.h>
#include <CGAL/Plane_3.h>
#include <CGAL/Boolean_set_operations_2.h>

// CGAL meshing 2D
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Polygon_2.h>

// CGAL closest polylines
#include <CGAL/box_intersection_d.h>
#include <CGAL/Iterator_range.h>
#include <CGAL/tuple.h>
#include <CGAL/boost/iterator/counting_iterator.hpp>

// CGAL mesh boolean
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/IO/polygon_mesh_io.h>
#include <boost/container/flat_map.hpp>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>

// CGAL skeleton
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/extract_mean_curvature_flow_skeleton.h>
#include <CGAL/boost/graph/split_graph_into_polylines.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits_3.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>
#include <CGAL/Search_traits_3.h>
#include <CGAL/Search_traits_adapter.h>
#include <CGAL/Orthogonal_k_neighbor_search.h>

// BOOST
#include <boost/range/const_iterator.hpp>
#include <boost/range/value_type.hpp>
#include <boost/foreach.hpp>
#include <boost/iterator/function_output_iterator.hpp>
#include <boost/graph/properties.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

// RTREE
#include "src/wood/include/rtree.h"

using IK = CGAL::Exact_predicates_inexact_constructions_kernel;
using EK = CGAL::Exact_predicates_exact_constructions_kernel;
typedef CGAL::Cartesian_converter<IK, EK> IK_to_EK;
typedef CGAL::Cartesian_converter<EK, IK> EK_to_IK;
using CGAL_Polyline = std::vector<IK::Point_3>;
using CGAL_Polylines = std::list<CGAL_Polyline>;
typedef typename CGAL::Box_intersection_d::Box_with_info_d<double, 3, std::pair<std::size_t, std::size_t>> Box;
typedef CGAL::Surface_mesh<IK::Point_3> Mesh;
namespace PMP = CGAL::Polygon_mesh_processing;

struct FaceInfo2
{
    FaceInfo2() {}
    int nesting_level;
    bool in_domain()
    {
        return nesting_level % 2 == 1;
    }
};

typedef CGAL::Triangulation_vertex_base_2<IK> Vb;
typedef CGAL::Triangulation_face_base_with_info_2<FaceInfo2, IK> Fbb;
typedef CGAL::Constrained_triangulation_face_base_2<IK, Fbb> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb> TDS;
typedef CGAL::Exact_predicates_tag Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<IK, TDS, Itag> CGALCDT;
typedef CGALCDT::Point Point;
typedef CGAL::Polygon_2<IK> Polygon_2;
typedef CGALCDT::Face_handle Face_handle;

typedef CGAL::Simple_cartesian<double>                                                  CK;
typedef CGAL::Polyhedron_3<CK>                                                          Polyhedron;
typedef boost::graph_traits<Polyhedron>::vertex_descriptor                              vertex_descriptor;
typedef CGAL::Mean_curvature_flow_skeletonization<Polyhedron>                           Skeletonization;
typedef Skeletonization::Skeleton                                                       Skeleton;
typedef Skeleton::vertex_descriptor                                                     Skeleton_vertex;
typedef Skeleton::edge_descriptor                                                       Skeleton_edge;
typedef Polyhedron::HalfedgeDS                                                          HalfedgeDS;


// Wood Library Utilities
#include "wood_globals.h"

// Order Matters
// #include "cgal_print.h"

#include "cgal_box_search.h"
#include "cgal_inscribe_util.h"
#include "cgal_vector_util.h"
#include "cgal_intersection_util.h"
#include "cgal_xform_util.h"

// Order does not matter
#include "cgal_box_util.h"
// #include "cgal_data_set.h"
#include "cgal_math_util.h"
#include "cgal_polyline_mesh_util.h"
#include "cgal_plane_util.h"
#include "clipper_util.h"
#include "cgal_polyline_util.h"
#include "cgal_rectangle_util.h"
#include "rtree_util.h"

#include "cgal_skeleton.h"

// Display
static std::vector<CGAL_Polyline> viewer_polylines;
