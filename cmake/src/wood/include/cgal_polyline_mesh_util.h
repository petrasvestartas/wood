
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVELOPER:
// Petras Vestartas, petasvestartas@gmail.com
// Funding: NCCR Digital Fabrication and EPFL
//
// HISTORY:
// 1) The first version was written during the PhD 8928 thesis of Petras Vestartas called:
// Design-to-Fabrication Workflow for Raw-Sawn-Timber using Joinery Solver, 2017-2021
// 2) The translation from C# to C++ was started during the funding of NCCR in two steps
// A - standalone C++ version of the joinery solver and B - integration to COMPAS framework (Python
// Pybind11)
//
// RESTRICTIONS:
// The code cannot be used for commercial reasons
// If you would like to use or change the code for research or educational reasons,
// please contact the developer first
//
// 3RD PARTY LIBRARIES:
// session_cpp CDT (trimesh_cdt.h)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CGAL_POLYLINE_MESH_UTIL_H
#define CGAL_POLYLINE_MESH_UTIL_H

#include "../../../ext/session_cpp/src/plane.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

namespace cgal
{
namespace polyline_mesh_util
{

/**
 * Create a mesh from a set of polylines with holes
 * WARNING: polylines_with_holes input contains top and bottom polylines, example top, bottom, top,
 * bottom, top, bottom, ..., but only top polylines are used to create the mesh
 *
 * @param [in] polylines_with_holes polylines in the order: top, bottom, top, bottom, top, bottom,
 * ...
 * @param [in] base_plane plane to orient to the first polyline
 * @param [out] top_outline_face_vertex_indices vector of vertex indices for top outline face
 * @param [out] v_count vertex count
 * @param [out] f_count face count
 */
void mesh_from_polylines (const std::vector<Polyline> &polylines_with_holes, const session_cpp::Plane &base_plane, std::vector<int> &top_outline_face_vertex_indices, int &v_count, int &f_count);

/**
 * Create a mesh loft from a set of polylines with holes
 * "VNF" stands for "vertices, normals, faces
 *
 * @param [in] polylines_with_holes polylines in the order: top, bottom, top, bottom, top, bottom,
 * ...
 * @param [out] out_vertices vertex coordinates: x,y,z,x,y,z,x,y,z,...
 * @param [out] out_normals vertex normals: x,y,z,x,y,z,x,y,z,...
 * @param [out] out_triangles triangle indices: i,j,k,i,j,k,i,j,k,...
 * @param [in] scale scale factor to convert scale the output geometry for viewer::viewer_wood
 * @return mesh vertices and faces in a form of Eigen matrices to be compatible with pybind11
 */
void closed_mesh_from_polylines_vnf (const std::vector<Polyline> &polylines_with_holes, std::vector<double> &out_vertices, std::vector<double> &out_normals, std::vector<int> &out_triangles,
                                     const double &scale = 1000);

/**
 * Create a mesh loft from a set of polylines with holes
 * Outputs a CGAL::Surface_mesh (used by cgal_mesh_boolean)
 *
 * @param [in] polylines_with_holes_not_clean polylines in the order: top, bottom, top, bottom, ...
 * @param [out] mesh CGAL Surface_mesh
 * @param [in] scale scale factor
 */
void closed_mesh_from_polylines (const std::vector<Polyline> &polylines_with_holes_not_clean, CGAL::Surface_mesh<CGAL::Exact_predicates_inexact_constructions_kernel::Point_3> &mesh, const double &scale = 1000);

} // namespace polyline_mesh_util
} // namespace cgal
#endif
