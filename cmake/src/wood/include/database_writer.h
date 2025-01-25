#ifndef POLYLINES_DATABASE_HPP
#define POLYLINES_DATABASE_HPP

#include <sqlite3.h>

#include "../../../stdafx.h"
#include "cgal_mesh_boolean.h"
#include "database_writer.h"

namespace database_writer
{

//! \cond NO_DOXYGEN
namespace internal
{

/**
 * Get length of a 3D vector, taken from the OpenNURBS library: https://github.com/mcneel/opennurbs
 *
 * @param [in] x The first coordinate of the vector
 * @param [in] y The second coordinate of the vector
 * @param [in] z The third coordinate of the vector
 * @return The length of the vector
 */
double length (double x, double y, double z);

/**
 * Unitize the vector by dividing its coordinates by its length
 *
 * @param [in, out] vector 3D vector
 * @return true if the vector length is not equal to zero
 */
bool unitize (IK::Vector_3 &vector);

/**
 * this functuon is used to mark facets that are inside the domain bounded by the polygon
 * it is used by mark_domains()
 *
 * @param [in] CGALCDT cgal constrained delaunay triangulation
 * @param [in] start face handle to start from
 * @param [in] index nesting level
 * @param [in, out] border border edges
 */
void mark_domains (CGALCDT &ct, Face_handle start, int index, std::list<CGALCDT::Edge> &border);

/**
 * explore set of facets connected with non constrained edges,
 * and attribute to each such set a nesting level.
 * We start from facets incident to the infinite vertex, with a nesting
 * level of 0. Then we recursively consider the non-explored facets incident
 * to constrained edges bounding the former set and increase the nesting level by 1.
 * Facets in the domain are those with an odd nesting level.
 *
 * @param [in] CGALCDT cgal constrained delaunay triangulation
 */

void mark_domains (CGALCDT &CGALCDT);

/**
 * Get transformation matrix from plane to XY plane
 *
 * @param [in] origin plane point
 * @param [in] plane CGAL plane
 * @return CGAL transformation matrix 4_4
 */
CGAL::Aff_transformation_3<IK> plane_to_xy (const IK::Point_3 &origin, const IK::Plane_3 &plane);

struct Visitor : public PMP::Corefinement::Default_visitor<Mesh>
{
    typedef Mesh::Face_index face_descriptor;

    boost::container::flat_map<const Mesh *, Mesh::Property_map<Mesh::Face_index, int> > properties;
    int face_id;

    Visitor ()
    {
        properties.reserve (3);
        face_id = -1;
    }

    // visitor API overloaded
    void
    before_subface_creations (face_descriptor f_split, Mesh &tm)
    {
        face_id = properties[&tm][f_split];
    }

    void
    after_subface_created (face_descriptor f_new, Mesh &tm)
    {
        properties[&tm][f_new] = face_id;
    }

    void
    after_face_copy (face_descriptor f_src, Mesh &tm_src, face_descriptor f_tgt, Mesh &tm_tgt)
    {
        properties[&tm_tgt][f_tgt] = properties[&tm_src][f_src];
    }
};
} // namespace internal
//! \endcond

/////////////////////////////////////////////////////////////////////////////////////////
// Database Writer
/////////////////////////////////////////////////////////////////////////////////////////
extern double SCALE;
extern double LINE_THICKNESS;
extern std::string COLOR;

struct SQLPoint
{
    double x, y, z; // Coordinates of the point.
};

struct SQLFace
{
    int a, b, c; // Coordinates of the point.
};

struct SQLPolyline
{
    std::vector<double> vertices; // Flat vector of vertex coordinates: x, y, z, ...
    std::string color = COLOR;    // Hexadecimal color code, e.g., "#FF0000" for red.
    double thickness = 2.0f;      // Thickness of the polyline.
};

struct SQLMesh
{
    std::vector<double> vertices; // Flat vector of vertex coordinates.
    std::vector<double> normals;  // Flat vector of normal coordinates.
    std::vector<int> indices;     // Flat vector of triangle face indices.
    std::string color = COLOR;    // Hexadecimal color code.
};

struct SQLPointCloud
{
    std::vector<double> vertices; // Flat vector of vertex coordinates: x, y, z, ...
    std::vector<double> normals;  // Flat vector of normal coordinates: nx, ny, nz, ...
    std::vector<double> colors;   // Flat vector of colors: r, g, b, ...
};

std::string serialize_sql_polyline (const SQLPolyline &polyline);
std::string serialize_sql_mesh (const SQLMesh &mesh);
std::string serialize_sql_pointcloud (const SQLPointCloud &pointCloud);
void exec_sql (sqlite3 *db, const std::string &sql);
void insert_sql_polylines (sqlite3 *db, const std::vector<SQLPolyline> &polylines, bool clearDbFirst = false);
void insert_sql_meshes (sqlite3 *db, const std::vector<SQLMesh> &meshes, bool clearDbFirst = false);
void insert_sql_pointclouds (sqlite3 *db, const std::vector<SQLPointCloud> &pointClouds, bool clearDbFirst = false);
bool write_to_database ();

/////////////////////////////////////////////////////////////////////////////////////////
// WOOD Types to Database Conversion
/////////////////////////////////////////////////////////////////////////////////////////

extern std::string DATABASE_PATH;
extern std::vector<SQLPolyline> SQL_POLYLINES;
extern std::vector<SQLMesh> SQL_MESHES;
extern std::vector<SQLPointCloud> SQL_POINTCLOUDS;

// Function to add polylines from a range, specified by iterators
template <typename Iterator>
static void
add_polylines (Iterator begin, Iterator end)
{
    for (auto it = begin; it != end; ++it)
        {
            if constexpr (std::is_same_v<typename std::iterator_traits<Iterator>::value_type, std::vector<IK::Point_3> >)
                {
                    // Directly add the polyline
                    if (it->empty ())
                        return;

                    double scale_inv = 1.0f / SCALE;
                    SQL_POLYLINES.emplace_back (SQLPolyline ());
                    SQL_POLYLINES.back ().vertices.reserve (it->size () * 3);
                    SQL_POLYLINES.back ().color = COLOR;
                    SQL_POLYLINES.back ().thickness = LINE_THICKNESS;

                    for (auto &point : *it)
                        {
                            SQL_POLYLINES.back ().vertices.emplace_back (static_cast<double> (point[0] * scale_inv));
                            SQL_POLYLINES.back ().vertices.emplace_back (static_cast<double> (point[1] * scale_inv));
                            SQL_POLYLINES.back ().vertices.emplace_back (static_cast<double> (point[2] * scale_inv));
                        }
                }
            else
                {
                    // Assuming nested structure, recurse into each sub-container
                    add_polylines (it->begin (), it->end ());
                }
        }
}

void add_polygon_mesh (const std::vector<CGAL_Polyline> &polylines);

void add_loft (std::vector<std::vector<CGAL_Polyline> > &output_plines);

void closed_mesh_from_polylines_vnf (const std::vector<CGAL_Polyline> &polylines_with_holes_not_clean, std::vector<double> &out_vertices, std::vector<double> &out_normals, std::vector<int> &out_triangles,
                                     const double &scale);

void mesh_from_polylines (const std::vector<CGAL_Polyline> &polylines_with_holes, const IK::Plane_3 &base_plane, std::vector<int> &top_outline_face_vertex_indices, int &v_count, int &f_count);

void add_mesh_boolean_difference (std::vector<CGAL_Polyline> &input_plines, std::vector<std::vector<CGAL_Polyline> > &output_plines);

void mesh_boolean_difference_to_viewer (std::vector<Mesh> &mesh_list, size_t difference_union_intersection, std::vector<double> &out_vertices, std::vector<double> &out_normals, std::vector<int> &out_triangles);

void add_points (const std::vector<IK::Point_3> &points);

}; // namespace database_writer
#endif // POLYLINES_DATABASE_HPP
