
#include "cgal_polyline_mesh_util.h"

#include "../../../stdafx.h" //go up to the folder where the CMakeLists.txt is
#include "../../../ext/session_cpp/src/trimesh_cdt.h"

namespace cgal
{
namespace polyline_mesh_util
{

// ── session_cpp CDT-based implementation ────────────────────────────────────

void
mesh_from_polylines (const std::vector<Polyline> &polylines_with_holes, const session_cpp::Plane &base_plane, std::vector<int> &top_outline_face_vertex_indices, int &v_count, int &f_count)
{
    //////////////////////////////////////////////////////////////////////////////
    // Create Transformation | Orient to 2D
    //////////////////////////////////////////////////////////////////////////////
    session_cpp::Point origin = base_plane.origin ();
    session_cpp::Vector x_axis = base_plane.x_axis ();
    session_cpp::Vector y_axis = base_plane.y_axis ();
    session_cpp::Vector z_axis = base_plane.z_axis ();
    session_cpp::Xform xform_toXY = session_cpp::Xform::plane_to_xy (origin, x_axis, y_axis, z_axis);

    //////////////////////////////////////////////////////////////////////////////
    // Project each even-indexed polyline to 2D
    //////////////////////////////////////////////////////////////////////////////
    std::vector<std::pair<double, double>> border_2d;
    std::vector<std::vector<std::pair<double, double>>> holes_2d;
    size_t total_vertex_count = 0;

    for (int i = 0; i < (int)polylines_with_holes.size (); i += 2)
        {
            const auto &poly = polylines_with_holes[i];
            int n = (int)poly.size () - 1; // skip closing point
            total_vertex_count += n;

            std::vector<std::pair<double, double>> pts_2d;
            pts_2d.reserve (n);
            for (int j = 0; j < n; j++)
                {
                    session_cpp::Point p = poly[j];
                    xform_toXY.transform_point (p);
                    pts_2d.push_back ({ p[0], p[1] });
                }

            if (i == 0)
                border_2d = std::move (pts_2d);
            else
                holes_2d.push_back (std::move (pts_2d));
        }

    //////////////////////////////////////////////////////////////////////////////
    // Constrained Delaunay Triangulation via session_cpp
    //////////////////////////////////////////////////////////////////////////////
    auto triangles = session_cpp::cdt_triangulate (border_2d, holes_2d);

    v_count = (int)total_vertex_count;
    f_count = (int)triangles.size ();

    top_outline_face_vertex_indices.reserve (triangles.size () * 3);
    for (auto &tri : triangles)
        {
            top_outline_face_vertex_indices.push_back (tri[0]);
            top_outline_face_vertex_indices.push_back (tri[1]);
            top_outline_face_vertex_indices.push_back (tri[2]);
        }
}

void
closed_mesh_from_polylines_vnf (const std::vector<Polyline> &polylines_with_holes_not_clean, std::vector<double> &out_vertices, std::vector<double> &out_normals, std::vector<int> &out_triangles, const double &scale)
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sanity Check
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<Polyline> polylines_with_holes;
    polylines_with_holes.reserve (polylines_with_holes_not_clean.size ());

    for (auto &polyline : polylines_with_holes_not_clean)
        if (polyline.size () > 2)
            polylines_with_holes.emplace_back (polyline);

    if (polylines_with_holes_not_clean.size () % 2 == 1)
        return;

    for (auto i = 0; i < polylines_with_holes.size (); i += 2)
        {
            auto a = polylines_with_holes[i].size ();
            auto b = polylines_with_holes[i + 1].size ();
            if (a != b)
                return;
            if (a < 2 || b < 2)
                return;
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Clean duplicate points
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<Polyline> polylines (polylines_with_holes.size ());

    for (auto i = 0; i < polylines_with_holes.size (); i += 2)
        {
            polylines[i + 0].reserve (polylines_with_holes[i + 0].size ());
            polylines[i + 1].reserve (polylines_with_holes[i + 1].size ());
            polylines[i + 0].emplace_back (polylines_with_holes[i + 0][0]);
            polylines[i + 1].emplace_back (polylines_with_holes[i + 1][0]);
            for (auto j = 1; j < polylines_with_holes[i + 0].size (); j++)
                {
                    if (polylines_with_holes[i + 0][j - 1].squared_distance (polylines_with_holes[i + 0][j]) > wood::GLOBALS::DISTANCE_SQUARED)
                        {
                            polylines[i + 0].emplace_back (polylines_with_holes[i + 0][j]);
                            polylines[i + 1].emplace_back (polylines_with_holes[i + 1][j]);
                        }
                }
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Compute average normal and create a plane
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    auto lastID = polylines.size () - 2;
    auto len = polylines[lastID].size () - 1;
    session_cpp::Vector average_normal (0, 0, 0);

    for (auto i = 0; i < len; i++)
        {
            auto prev = ((i - 1) + len) % len;
            auto next = ((i + 1) + len) % len;
            auto v1 = polylines[lastID][i] - polylines[lastID][prev];
            auto v2 = polylines[lastID][next] - polylines[lastID][i];
            average_normal = average_normal + v1.cross (v2);
        }
    average_normal.normalize_self ();

    // flip if needed: check if bottom polyline point is on positive side of plane
    auto &origin_pt = polylines[lastID][0];
    auto &test_pt = polylines[polylines.size () - 1][0];
    double dot = (test_pt[0] - origin_pt[0]) * average_normal[0] + (test_pt[1] - origin_pt[1]) * average_normal[1] + (test_pt[2] - origin_pt[2]) * average_normal[2];
    if (dot > 0)
        average_normal = average_normal * -1.0;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create a mesh for top outlines
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    session_cpp::Vector norm_for_plane = average_normal;
    session_cpp::Point orig_for_plane = origin_pt;
    session_cpp::Plane base_plane = session_cpp::Plane::from_point_normal (orig_for_plane, norm_for_plane);

    std::vector<int> top_outline_face_vertex_indices;
    int v, f;
    mesh_from_polylines (polylines, base_plane, top_outline_face_vertex_indices, v, f);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Count vertices
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    size_t vertex_count = 0;
    for (int i = 0; i < polylines.size (); i += 2)
        vertex_count += polylines[i].size () - 1;

    if (v != (int)vertex_count)
        return;

    auto face_count = top_outline_face_vertex_indices.size () / 3;

    std::vector<double> out_vertices_temp;
    out_vertices_temp.reserve (vertex_count * 2 * 3);
    out_vertices.reserve (face_count * 2 * 3);
    out_normals.reserve (face_count * 2 * 3);
    out_triangles.reserve (face_count * 2 * 3);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // -> Top vertices
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int vid = 0;
    std::vector<std::array<int, 4>> sides;
    sides.reserve (vertex_count);

    bool holes = polylines.size () > 2;

    for (auto i = 0; i < polylines.size (); i += 2)
        {
            for (auto j = 0; j < polylines[i].size () - 1; j++)
                {
                    out_vertices_temp.emplace_back ((double)polylines[i][j][0] / scale);
                    out_vertices_temp.emplace_back ((double)polylines[i][j][1] / scale);
                    out_vertices_temp.emplace_back ((double)polylines[i][j][2] / scale);

                    if (j == polylines[i].size () - 2)
                        {
                            auto n = polylines[i].size () - 2;
                            std::array<int, 4> side{ vid, vid - (int)n, vid - (int)n + (int)vertex_count, vid + 0 + (int)vertex_count };
                            sides.emplace_back (side);
                        }
                    else
                        {
                            std::array<int, 4> side = {
                                vid,
                                vid + 1,
                                vid + 1 + (int)vertex_count,
                                vid + 0 + (int)vertex_count,
                            };
                            sides.emplace_back (side);
                        }

                    vid++;
                }
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // -> Bottom vertices
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for (auto i = 0; i < polylines.size (); i += 2)
        {
            for (auto j = 0; j < polylines[i].size () - 1; j++)
                {
                    out_vertices_temp.emplace_back ((double)polylines[i + 1][j][0] / scale);
                    out_vertices_temp.emplace_back ((double)polylines[i + 1][j][1] / scale);
                    out_vertices_temp.emplace_back ((double)polylines[i + 1][j][2] / scale);
                }
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // -> Top face indices
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < top_outline_face_vertex_indices.size (); i += 3)
        {
            int a = top_outline_face_vertex_indices[i + 0];
            int b = top_outline_face_vertex_indices[i + 1];
            int c = top_outline_face_vertex_indices[i + 2];

            out_triangles.emplace_back (out_triangles.size ());
            out_triangles.emplace_back (out_triangles.size ());
            out_triangles.emplace_back (out_triangles.size ());

            out_vertices.emplace_back (out_vertices_temp[a * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[a * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[a * 3 + 2]);

            out_vertices.emplace_back (out_vertices_temp[b * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[b * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[b * 3 + 2]);

            out_vertices.emplace_back (out_vertices_temp[c * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[c * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[c * 3 + 2]);

            out_normals.emplace_back (average_normal[0]);
            out_normals.emplace_back (average_normal[1]);
            out_normals.emplace_back (average_normal[2]);

            out_normals.emplace_back (average_normal[0]);
            out_normals.emplace_back (average_normal[1]);
            out_normals.emplace_back (average_normal[2]);

            out_normals.emplace_back (average_normal[0]);
            out_normals.emplace_back (average_normal[1]);
            out_normals.emplace_back (average_normal[2]);
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // -> Bottom face indices
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < top_outline_face_vertex_indices.size (); i += 3)
        {
            int a = (int)vertex_count + top_outline_face_vertex_indices[i + 2];
            int b = (int)vertex_count + top_outline_face_vertex_indices[i + 1];
            int c = (int)vertex_count + top_outline_face_vertex_indices[i + 0];

            out_triangles.emplace_back (out_triangles.size ());
            out_triangles.emplace_back (out_triangles.size ());
            out_triangles.emplace_back (out_triangles.size ());

            out_vertices.emplace_back (out_vertices_temp[a * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[a * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[a * 3 + 2]);

            out_vertices.emplace_back (out_vertices_temp[b * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[b * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[b * 3 + 2]);

            out_vertices.emplace_back (out_vertices_temp[c * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[c * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[c * 3 + 2]);

            out_normals.emplace_back (-average_normal[0]);
            out_normals.emplace_back (-average_normal[1]);
            out_normals.emplace_back (-average_normal[2]);

            out_normals.emplace_back (-average_normal[0]);
            out_normals.emplace_back (-average_normal[1]);
            out_normals.emplace_back (-average_normal[2]);

            out_normals.emplace_back (-average_normal[0]);
            out_normals.emplace_back (-average_normal[1]);
            out_normals.emplace_back (-average_normal[2]);
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // -> Side face indices
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < sides.size (); i++)
        {
            int a0 = sides[i][3];
            int b0 = sides[i][2];
            int c0 = sides[i][1];

            int a1 = sides[i][3];
            int b1 = sides[i][1];
            int c1 = sides[i][0];

            out_triangles.emplace_back (out_triangles.size ());
            out_triangles.emplace_back (out_triangles.size ());
            out_triangles.emplace_back (out_triangles.size ());

            out_triangles.emplace_back (out_triangles.size ());
            out_triangles.emplace_back (out_triangles.size ());
            out_triangles.emplace_back (out_triangles.size ());

            out_vertices.emplace_back (out_vertices_temp[a0 * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[a0 * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[a0 * 3 + 2]);

            out_vertices.emplace_back (out_vertices_temp[b0 * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[b0 * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[b0 * 3 + 2]);

            out_vertices.emplace_back (out_vertices_temp[c0 * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[c0 * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[c0 * 3 + 2]);

            out_vertices.emplace_back (out_vertices_temp[a1 * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[a1 * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[a1 * 3 + 2]);

            out_vertices.emplace_back (out_vertices_temp[b1 * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[b1 * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[b1 * 3 + 2]);

            out_vertices.emplace_back (out_vertices_temp[c1 * 3 + 0]);
            out_vertices.emplace_back (out_vertices_temp[c1 * 3 + 1]);
            out_vertices.emplace_back (out_vertices_temp[c1 * 3 + 2]);

            // Side face normal from cross product
            double ax0 = out_vertices_temp[a0 * 3 + 0], ay0 = out_vertices_temp[a0 * 3 + 1], az0 = out_vertices_temp[a0 * 3 + 2];
            double bx0 = out_vertices_temp[b0 * 3 + 0], by0 = out_vertices_temp[b0 * 3 + 1], bz0 = out_vertices_temp[b0 * 3 + 2];
            double cx0 = out_vertices_temp[c0 * 3 + 0], cy0 = out_vertices_temp[c0 * 3 + 1], cz0 = out_vertices_temp[c0 * 3 + 2];
            session_cpp::Vector va (ax0 - bx0, ay0 - by0, az0 - bz0);
            session_cpp::Vector vb (cx0 - bx0, cy0 - by0, cz0 - bz0);
            session_cpp::Vector normal_0 = (va.cross (vb)) * -1.0;
            normal_0.normalize_self ();

            for (int k = 0; k < 6; k++)
                {
                    out_normals.emplace_back (normal_0[0]);
                    out_normals.emplace_back (normal_0[1]);
                    out_normals.emplace_back (normal_0[2]);
                }
        }
}

void
closed_mesh_from_polylines (const std::vector<Polyline> &polylines_with_holes_not_clean, CGAL::Surface_mesh<CGAL::Exact_predicates_inexact_constructions_kernel::Point_3> &mesh, const double &scale)
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sanity Check
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<Polyline> polylines_with_holes;
    polylines_with_holes.reserve (polylines_with_holes_not_clean.size ());

    for (auto &polyline : polylines_with_holes_not_clean)
        if (polyline.size () > 2)
            polylines_with_holes.emplace_back (polyline);

    if (polylines_with_holes_not_clean.size () % 2 == 1)
        return;

    for (auto i = 0; i < polylines_with_holes.size (); i += 2)
        {
            auto a = polylines_with_holes[i].size ();
            auto b = polylines_with_holes[i + 1].size ();
            if (a != b)
                return;
            if (a < 2 || b < 2)
                return;
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Clean duplicate points
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<Polyline> polylines (polylines_with_holes.size ());

    for (auto i = 0; i < polylines_with_holes.size (); i += 2)
        {
            polylines[i + 0].reserve (polylines_with_holes[i + 0].size ());
            polylines[i + 1].reserve (polylines_with_holes[i + 1].size ());
            polylines[i + 0].emplace_back (polylines_with_holes[i + 0][0]);
            polylines[i + 1].emplace_back (polylines_with_holes[i + 1][0]);
            for (auto j = 1; j < polylines_with_holes[i + 0].size (); j++)
                {
                    if (polylines_with_holes[i + 0][j - 1].squared_distance (polylines_with_holes[i + 0][j]) > wood::GLOBALS::DISTANCE_SQUARED)
                        {
                            polylines[i + 0].emplace_back (polylines_with_holes[i + 0][j]);
                            polylines[i + 1].emplace_back (polylines_with_holes[i + 1][j]);
                        }
                }
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Compute average normal
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    auto lastID = polylines.size () - 2;
    auto len = polylines[lastID].size () - 1;
    session_cpp::Vector average_normal (0, 0, 0);

    for (auto i = 0; i < len; i++)
        {
            auto prev = ((i - 1) + len) % len;
            auto next = ((i + 1) + len) % len;
            auto v1 = polylines[lastID][i] - polylines[lastID][prev];
            auto v2 = polylines[lastID][next] - polylines[lastID][i];
            average_normal = average_normal + v1.cross (v2);
        }
    average_normal.normalize_self ();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create a mesh for top outlines
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    session_cpp::Point orig_pt = polylines[lastID][0];
    session_cpp::Vector norm_vec = average_normal;
    session_cpp::Plane sc_base_plane = session_cpp::Plane::from_point_normal (orig_pt, norm_vec);

    std::vector<int> top_outline_face_vertex_indices;
    int v, f;
    mesh_from_polylines (polylines, sc_base_plane, top_outline_face_vertex_indices, v, f);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // add vertices to the mesh from the top and bottom outlines
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < polylines.size (); i += 2)
        {
            for (int j = 0; j < polylines[i].size () - 1; j++)
                mesh.add_vertex (CGAL::Exact_predicates_inexact_constructions_kernel::Point_3 (polylines[i][j][0] / scale, polylines[i][j][1] / scale, polylines[i][j][2] / scale));

            for (int j = 0; j < polylines[i + 1].size () - 1; j++)
                mesh.add_vertex (CGAL::Exact_predicates_inexact_constructions_kernel::Point_3 (polylines[i + 1][j][0] / scale, polylines[i + 1][j][1] / scale, polylines[i + 1][j][2] / scale));
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // add faces to the mesh from the top and bottom outlines
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < top_outline_face_vertex_indices.size (); i += 3)
        {
            mesh.add_face (CGAL::SM_Vertex_index (top_outline_face_vertex_indices[i + 0]), CGAL::SM_Vertex_index (top_outline_face_vertex_indices[i + 1]), CGAL::SM_Vertex_index (top_outline_face_vertex_indices[i + 2]));
            mesh.add_face (CGAL::SM_Vertex_index (top_outline_face_vertex_indices[i + 2] + v), CGAL::SM_Vertex_index (top_outline_face_vertex_indices[i + 1] + v),
                           CGAL::SM_Vertex_index (top_outline_face_vertex_indices[i + 0] + v));
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // add vertical faces to the mesh
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < polylines.size (); i += 2)
        {
            for (int j = 0; j < polylines[i].size () - 1; j++)
                {
                    int a0 = j;
                    int b0 = (j + 1) % (polylines[i].size () - 1);
                    int c0 = j + v;
                    int a1 = j + v;
                    int b1 = (j + 1) % (polylines[i].size () - 1) + v;
                    int c1 = (j + 1) % (polylines[i].size () - 1);

                    mesh.add_face (CGAL::SM_Vertex_index (c0), CGAL::SM_Vertex_index (b0), CGAL::SM_Vertex_index (a0));
                    mesh.add_face (CGAL::SM_Vertex_index (a1), CGAL::SM_Vertex_index (b1), CGAL::SM_Vertex_index (c1));
                }
        }
}

} // namespace polyline_mesh_util
} // namespace cgal
