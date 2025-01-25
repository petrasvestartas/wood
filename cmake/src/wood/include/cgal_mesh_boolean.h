#ifndef CGAL_MESH_BOOLEAN_H
#define CGAL_MESH_BOOLEAN_H

namespace cgal
{
namespace mesh_boolean
{
void mesh_boolean_create_array_track_colors (

    // double *coord_mesh, int *n_coord_meshArray, // Flat array of coordinates / 0 256 512 flat
    // array of vertices array / number of meshes int *faces_mesh, int *n_faces_meshArray, size_t
    // n_mesh,

    std::vector<Mesh> mesh_list, size_t difference_union_intersection,

    std::vector<double> &coord_out, int &n_coord_out, std::vector<double> &normals_out, std::vector<int> &faces_out, int &n_faces_out, std::vector<int> &facesColors_out, int &n_facesColors_out, int &n_valid_meshes

);

void mesh_boolean_test ();

void mesh_boolean_difference_to_viewer (std::vector<Mesh> &mesh_list, size_t difference_union_intersection, std::vector<double> &out_vertices, std::vector<double> &out_normals, std::vector<int> &out_triangles);

void mesh_boolean_difference (std::vector<Mesh> &mesh_list, size_t difference_union_intersection, std::vector<IK::Point_3> &out_vertices, std::vector<IK::Vector_3> &out_normals,
                              std::vector<std::vector<int> > &out_triangles);

void mesh_boolean_difference_from_polylines (std::vector<CGAL_Polyline> &input_plines, std::vector<std::vector<CGAL_Polyline> > &output_plines, std::vector<std::vector<IK::Point_3> > &out_vertices,
                                             std::vector<std::vector<IK::Vector_3> > &out_normals, std::vector<std::vector<std::vector<int> > > &out_triangles);
} // namespace mesh_boolean
} // namespace cgal
#endif // CGAL_MESH_BOOLEAN_H
