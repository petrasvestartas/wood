// Notes:
// Serialization: The serialize_sql_polyline function converts a SQLPolyline object into a
// serialized string format, ready for storage in the SQLite database. Database Initialization: The
// program starts by opening (and creating if necessary) the SQLite database polylines.db. It then
// ensures the SQLPolylines table exists by executing the CREATE TABLE IF NOT EXISTS SQL statement.
// Data Insertion: The insert_sql_polylines function iterates over a vector of SQLPolyline objects,
// serializes each one, and inserts the serialized data into the database along with the polyline
// color. Program Execution: Running this program will create the polylines.db SQLite database file
// in the current working directory (if it doesn't already exist), create the SQLPolylines table
// within that database, and insert two sample polyline records. cmake -S . -B build -G "Visual
// Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release
// --parallel 8 && build\Release\opengl_viewer

#include "database_writer.h"

namespace database_writer
{

//! \cond NO_DOXYGEN
namespace internal
{

double
length (double x, double y, double z)
{
    double len;
    x = fabs (x);
    y = fabs (y);
    z = fabs (z);
    if (y >= x && y >= z)
        {
            len = x;
            x = y;
            y = len;
        }
    else if (z >= x && z >= y)
        {
            len = x;
            x = z;
            z = len;
        }

    // 15 September 2003 Dale Lear
    //     For small denormalized doubles (positive but smaller
    //     than DBL_MIN), some compilers/FPUs set 1.0/x to +INF.
    //     Without the ON_DBL_MIN test we end up with
    //     microscopic vectors that have infinte length!
    //
    //     This code is absolutely necessary.  It is a critical
    //     part of the bug fix for RR 11217.
    if (x > ON_DBL_MIN)
        {
            y /= x;
            z /= x;
            len = x * sqrt (1.0 + y * y + z * z);
        }
    else if (x > 0.0 && ON_IS_FINITE (x))
        len = x;
    else
        len = 0.0;

    return len;
}

bool
unitize (IK::Vector_3 &vector)
{
    bool rc = false;
    // Since x,y,z are doubles, d will not be denormalized and the
    // ON_DBL_MIN tests in ON_2dVector::Unitize() are not needed.

    double d = length (vector.hx (), vector.hy (), vector.hz ());
    if (d > 0.0)
        {
            double dx = vector.hx ();
            double dy = vector.hy ();
            double dz = vector.hz ();
            vector = IK::Vector_3 ((dx / d), (dy / d), (dz / d));
            rc = true;
        }
    return rc;
}

void
mark_domains (CGALCDT &ct, Face_handle start, int index, std::list<CGALCDT::Edge> &border)
{
    if (start->info ().nesting_level != -1)
        return;

    std::list<Face_handle> queue;
    queue.push_back (start);
    while (!queue.empty ())
        {
            Face_handle fh = queue.front ();
            queue.pop_front ();
            if (fh->info ().nesting_level == -1)
                {
                    fh->info ().nesting_level = index;
                    for (int i = 0; i < 3; i++)
                        {
                            CGALCDT::Edge e (fh, i);
                            Face_handle n = fh->neighbor (i);
                            if (n->info ().nesting_level == -1)
                                {
                                    if (ct.is_constrained (e))
                                        border.push_back (e);
                                    else
                                        queue.push_back (n);
                                }
                        }
                }
        }
}

void
mark_domains (CGALCDT &CGALCDT)
{
    for (CGALCDT::Face_handle f : CGALCDT.all_face_handles ())
        {
            f->info ().nesting_level = -1;
        }
    std::list<CGALCDT::Edge> border;
    mark_domains (CGALCDT, CGALCDT.infinite_face (), 0, border);
    while (!border.empty ())
        {
            CGALCDT::Edge e = border.front ();
            border.pop_front ();
            Face_handle n = e.first->neighbor (e.second);
            if (n->info ().nesting_level == -1)
                {
                    mark_domains (CGALCDT, n, e.first->info ().nesting_level + 1, border);
                }
        }
}

CGAL::Aff_transformation_3<IK>
plane_to_xy (const IK::Point_3 &origin, const IK::Plane_3 &plane)
{
    auto x_axis = plane.base1 ();
    auto y_axis = plane.base2 ();
    auto z_axis = plane.orthogonal_vector ();
    internal::unitize (x_axis);
    internal::unitize (y_axis);
    internal::unitize (z_axis);

    // transformation maps P0 to P1, P0+X0 to P1+X1, ...

    // Move to origin -> T0 translates point P0 to (0,0,0)
    CGAL::Aff_transformation_3<IK> t (CGAL::TRANSLATION, IK::Vector_3 (-origin.x (), -origin.y (), -origin.z ()));

    // Rotate ->
    CGAL::Aff_transformation_3<IK> f (x_axis.x (), x_axis.y (), x_axis.z (), y_axis.x (), y_axis.y (), y_axis.z (), z_axis.x (), z_axis.y (), z_axis.z ());

    return f * t;
}

} // namespace internal
//! \endcond

/////////////////////////////////////////////////////////////////////////////////////////
// Database Writer
/////////////////////////////////////////////////////////////////////////////////////////
double SCALE = 1000.0f;
double LINE_THICKNESS = 3;
std::string COLOR = "#000000";

std::string
serialize_sql_polyline (const SQLPolyline &polyline)
{
    std::ostringstream ss;
    for (size_t i = 0; i < polyline.vertices.size (); i += 3)
        {
            ss << std::fixed << std::setprecision (6) << polyline.vertices[i] << "," << polyline.vertices[i + 1] << "," << polyline.vertices[i + 2] << ";";
        }
    return ss.str ();
}

std::string
serialize_sql_mesh (const SQLMesh &mesh)
{
    std::ostringstream ss;
    for (size_t i = 0; i < mesh.vertices.size (); i += 3)
        {
            ss << std::fixed << std::setprecision (6) << mesh.vertices[i] << "," << mesh.vertices[i + 1] << "," << mesh.vertices[i + 2] << ";";
        }
    ss << "|";
    for (size_t i = 0; i < mesh.normals.size (); i += 3)
        {
            ss << std::fixed << std::setprecision (6) << mesh.normals[i] << "," << mesh.normals[i + 1] << "," << mesh.normals[i + 2] << ";";
        }
    ss << "|";
    for (size_t i = 0; i < mesh.indices.size (); i += 3)
        {
            ss << mesh.indices[i] << "," << mesh.indices[i + 1] << "," << mesh.indices[i + 2] << ";";
        }
    return ss.str ();
}

std::string
serialize_sql_pointcloud (const SQLPointCloud &pointCloud)
{
    std::ostringstream ss;
    for (size_t i = 0; i < pointCloud.vertices.size (); i += 3)
        {
            ss << std::fixed << std::setprecision (6) << pointCloud.vertices[i] << "," << pointCloud.vertices[i + 1] << "," << pointCloud.vertices[i + 2] << ";";
        }
    ss << "|";
    for (size_t i = 0; i < pointCloud.normals.size (); i += 3)
        {
            ss << std::fixed << std::setprecision (6) << pointCloud.normals[i] << "," << pointCloud.normals[i + 1] << "," << pointCloud.normals[i + 2] << ";";
        }
    ss << "|";
    for (size_t i = 0; i < pointCloud.colors.size (); i += 3)
        {
            ss << std::fixed << std::setprecision (6) << pointCloud.colors[i] << "," << pointCloud.colors[i + 1] << "," << pointCloud.colors[i + 2] << ";";
        }
    return ss.str ();
}

void
exec_sql (sqlite3 *db, const std::string &sql)
{
    char *errMsg = nullptr;
    int rc = sqlite3_exec (db, sql.c_str (), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
        {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free (errMsg);
        }
}

void
insert_sql_polylines (sqlite3 *db, const std::vector<SQLPolyline> &polylines, bool clearDbFirst)
{
    if (clearDbFirst)
        {
            exec_sql (db, "DELETE FROM SQLPolylines;");
        }

    const char *sql = "INSERT INTO SQLPolylines (Data, Color, Thickness) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2 (db, sql, -1, &stmt, NULL) != SQLITE_OK)
        {
            std::cerr << "SQL error: " << sqlite3_errmsg (db) << std::endl;
            return;
        }

    for (const auto &polyline : polylines)
        {
            std::string data = serialize_sql_polyline (polyline);

            // Bind the serialized data
            sqlite3_bind_text (stmt, 1, data.c_str (), -1, SQLITE_TRANSIENT);

            // Bind the color
            sqlite3_bind_text (stmt, 2, polyline.color.c_str (), -1, SQLITE_TRANSIENT);

            // Bind the thickness as a REAL
            sqlite3_bind_double (stmt, 3, static_cast<double> (polyline.thickness));

            if (sqlite3_step (stmt) != SQLITE_DONE)
                {
                    std::cerr << "Error inserting data: " << sqlite3_errmsg (db) << std::endl;
                }

            // Reset the statement to be reused
            sqlite3_reset (stmt);
        }

    // Finalize the statement to free resources
    sqlite3_finalize (stmt);
}

void
insert_sql_meshes (sqlite3 *db, const std::vector<SQLMesh> &meshes, bool clearDbFirst)
{
    if (clearDbFirst)
        {
            exec_sql (db, "DELETE FROM SQLMeshes;");
        }

    for (const auto &mesh : meshes)
        {
            std::string data = serialize_sql_mesh (mesh);
            exec_sql (db, "INSERT INTO SQLMeshes (Data, Color) VALUES ('" + data + "', '" + mesh.color + "');");
        }
}

void
insert_sql_pointclouds (sqlite3 *db, const std::vector<SQLPointCloud> &pointClouds, bool clearDbFirst)
{
    if (clearDbFirst)
        {
            exec_sql (db, "DELETE FROM SQLPointClouds;");
        }

    for (const auto &pointCloud : pointClouds)
        {
            std::string data = serialize_sql_pointcloud (pointCloud);
            exec_sql (db, "INSERT INTO SQLPointClouds (Data) VALUES ('" + data + "');");
        }
}

bool
write_to_database ()
{
    sqlite3 *db;
    std::cout << wood::GLOBALS::DATA_SET_OUTPUT_DATABASE.c_str () << std::endl;

    if (sqlite3_open (wood::GLOBALS::DATA_SET_OUTPUT_DATABASE.c_str (), &db) == SQLITE_OK)
        {
            // Enable WAL mode
            sqlite3_exec (db, "PRAGMA journal_mode=WAL;", 0, 0, 0);

            // Ensure the tables exist and are properly structured.
            // Continue with the rest of your database operations...
        }
    else
        {
            std::cerr << "Can't open database: " << sqlite3_errmsg (db) << std::endl;
            return false;
        }

    // Ensure the tables exist and are properly structured.
    exec_sql (db, "CREATE TABLE IF NOT EXISTS SQLPolylines (ID INTEGER PRIMARY KEY AUTOINCREMENT, Data "
                  "TEXT NOT NULL, Color TEXT NOT NULL, Thickness REAL NOT NULL);");
    exec_sql (db, "CREATE TABLE IF NOT EXISTS SQLMeshes (ID INTEGER PRIMARY KEY AUTOINCREMENT, Data "
                  "TEXT NOT NULL, Color TEXT NOT NULL);");
    exec_sql (db, "CREATE TABLE IF NOT EXISTS SQLPointClouds (ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "Data TEXT NOT NULL);");

    // clear the database
    exec_sql (db, "DELETE FROM SQLPolylines;");
    exec_sql (db, "DELETE FROM SQLMeshes;");
    exec_sql (db, "DELETE FROM SQLPointClouds;");

    // add geometry to the database
    insert_sql_polylines (db, SQL_POLYLINES, false);
    insert_sql_meshes (db, SQL_MESHES, false);
    insert_sql_pointclouds (db, SQL_POINTCLOUDS, false);
    sqlite3_close (db);

    // Clear the geometry
    SQL_POLYLINES.clear ();
    SQL_MESHES.clear ();
    SQL_POINTCLOUDS.clear ();

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// WOOD Types to Database Conversion
/////////////////////////////////////////////////////////////////////////////////////////

std::vector<SQLPolyline> SQL_POLYLINES{};
std::vector<SQLMesh> SQL_MESHES{};
std::vector<SQLPointCloud> SQL_POINTCLOUDS{};

// done
void
add_polygon_mesh (const std::vector<CGAL_Polyline> &polylines)
{
    SQLMesh sqlmesh;
    IK::Plane_3 base_plane = IK::Plane_3 (polylines[0][0], polylines[0][1], polylines[0][2]);
    IK::Vector_3 normal = base_plane.orthogonal_vector ();
    internal::unitize (normal);

    for (const auto &polyline : polylines)
        {
            for (int i = 0; i < polyline.size () - 1; ++i)
                {
                    for (int axis : { 0, 1, 2 })
                        { // Loop over x, y, z coordinates
                            sqlmesh.vertices.emplace_back (static_cast<double> (polyline[i].cartesian (axis) / SCALE));
                            sqlmesh.normals.emplace_back (static_cast<double> (normal.cartesian (axis)));
                        }
                }
        }

    int v_count = 0, f_count = 0;
    mesh_from_polylines (polylines, base_plane, sqlmesh.indices, v_count, f_count);
    sqlmesh.color = COLOR;
    SQL_MESHES.emplace_back (sqlmesh);
}

// done
void
add_loft (std::vector<std::vector<CGAL_Polyline> > &output_plines)
{
    std::vector<double> out_vertices;
    std::vector<double> out_normals;
    std::vector<int> out_triangles;

    for (auto &polylines : output_plines)
        {
            SQLMesh sqlmesh;
            closed_mesh_from_polylines_vnf (polylines, sqlmesh.vertices, sqlmesh.normals, sqlmesh.indices, SCALE);
            sqlmesh.color = COLOR;
            SQL_MESHES.emplace_back (sqlmesh);
        }
}

void
closed_mesh_from_polylines_vnf (const std::vector<CGAL_Polyline> &polylines_with_holes_not_clean, std::vector<double> &out_vertices, std::vector<double> &out_normals, std::vector<int> &out_triangles, const double &scale)
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sanity Check
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // RowMatrixXd v_empty(0, 0);
    // RowMatrixXi f_empty(0, 0);
    // auto empty_tuple = std::make_tuple(v_empty, f_empty);

    std::vector<CGAL_Polyline> polylines_with_holes;
    polylines_with_holes.reserve (polylines_with_holes_not_clean.size ());

    // ignore line segments with less than 2 points
    for (auto &polyline : polylines_with_holes_not_clean)
        if (polyline.size () > 2)
            polylines_with_holes.emplace_back (polyline);

    // if there polyline pairs are not even, return empty mesh
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
    std::vector<CGAL_Polyline> polylines (polylines_with_holes.size ());

    for (auto i = 0; i < polylines_with_holes.size (); i += 2)
        {
            polylines[i + 0].reserve (polylines_with_holes[i + 0].size ());
            polylines[i + 1].reserve (polylines_with_holes[i + 1].size ());
            polylines[i + 0].emplace_back (polylines_with_holes[i + 0][0]);
            polylines[i + 1].emplace_back (polylines_with_holes[i + 1][0]);
            for (auto j = 1; j < polylines_with_holes[i + 0].size (); j++)
                {
                    if (CGAL::squared_distance (polylines_with_holes[i + 0][j - 1], polylines_with_holes[i + 0][j]) > wood::GLOBALS::DISTANCE_SQUARED)
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
    IK::Vector_3 average_normal = IK::Vector_3 (0, 0, 0);

    for (auto i = 0; i < len; i++)
        {
            auto prev = ((i - 1) + len) % len;
            auto next = ((i + 1) + len) % len;
            average_normal = average_normal + CGAL::cross_product (polylines[lastID][i] - polylines[lastID][prev], polylines[lastID][next] - polylines[lastID][i]);
        }
    internal::unitize (average_normal);

    // flip if needed
    IK::Plane_3 base_plane (polylines[lastID][0], average_normal);
    if (base_plane.has_on_positive_side (polylines[polylines.size () - 1][0]))
        average_normal *= -1;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create a mesh for top outlines
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<int> top_outline_face_vertex_indices;
    int v, f;
    mesh_from_polylines (polylines, base_plane, top_outline_face_vertex_indices, v, f);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create mesh for the full plate
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // -> Count vertices and faces
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    size_t vertex_count = 0;
    for (int i = 0; i < polylines.size (); i += 2)
        vertex_count += polylines[i].size () - 1;

    if (v != vertex_count)
        {
            return;
        }

    auto face_count = top_outline_face_vertex_indices.size () / 3;

    std::vector<double> out_vertices_temp;
    out_vertices_temp.reserve (vertex_count * 2 * 3);
    out_vertices.reserve (face_count * 2 * 3);
    out_normals.reserve (face_count * 2 * 3);
    out_triangles.reserve (face_count * 2 * 3);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // -> Top vertices
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Set vertex coordinates from all polylines a-b a-b
    int vid = 0;
    std::vector<std::array<int, 4> > sides;
    sides.reserve (vertex_count);

    bool holes = polylines.size () > 2;

    for (auto i = 0; i < polylines.size (); i += 2)
        {
            bool last = i == (polylines.size () - 2);

            for (auto j = 0; j < polylines[i].size () - 1; j++)
                {
                    // vertices
                    out_vertices_temp.emplace_back ((double)polylines[i][j].hx () / scale);
                    out_vertices_temp.emplace_back ((double)polylines[i][j].hy () / scale);
                    out_vertices_temp.emplace_back ((double)polylines[i][j].hz () / scale);

                    // last faces
                    if (j == polylines[i].size () - 2)
                        { // take vertices from beggining
                            auto n = polylines[i].size () - 2;
                            std::array<int, 4> side{ vid, vid - (int)n, vid - (int)n + (int)vertex_count, vid + 0 + (int)vertex_count };

                            if (holes)
                                {
                                    sides.emplace_back (side);
                                }
                            else
                                {
                                    sides.emplace_back (side);
                                }
                        }
                    else
                        { // take next vertices
                            std::array<int, 4> side = {
                                vid + 0 + (int)vertex_count,
                                vid + 1 + (int)vertex_count,
                                vid + 1,
                                vid,
                            };

                            if (holes)
                                {
                                    side = {
                                        vid,
                                        vid + 1,
                                        vid + 1 + (int)vertex_count,
                                        vid + 0 + (int)vertex_count,
                                    };
                                    sides.emplace_back (side);
                                }
                            else
                                {
                                    side = {
                                        vid,
                                        vid + 1,
                                        vid + 1 + (int)vertex_count,
                                        vid + 0 + (int)vertex_count,
                                    };
                                    sides.emplace_back (side);
                                }
                        }

                    vid++;
                }
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // -> Bottom vertices
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    vid = 0;
    for (auto i = 0; i < polylines.size (); i += 2)
        {
            for (auto j = 0; j < polylines[i].size () - 1; j++)
                {
                    // vertices
                    out_vertices_temp.emplace_back ((double)polylines[i + 1][j].hx () / scale);
                    out_vertices_temp.emplace_back ((double)polylines[i + 1][j].hy () / scale);
                    out_vertices_temp.emplace_back ((double)polylines[i + 1][j].hz () / scale);
                    vid++;
                }
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // -> Top face indices
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    for (int i = 0; i < top_outline_face_vertex_indices.size (); i += 3)
        {
            int fid = i / 3;
            int a = top_outline_face_vertex_indices[i + 0];
            int b = top_outline_face_vertex_indices[i + 1];
            int c = top_outline_face_vertex_indices[i + 2];

            int v_count = out_triangles.size ();
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

            out_normals.emplace_back (average_normal.hx ());
            out_normals.emplace_back (average_normal.hy ());
            out_normals.emplace_back (average_normal.hz ());

            out_normals.emplace_back (average_normal.hx ());
            out_normals.emplace_back (average_normal.hy ());
            out_normals.emplace_back (average_normal.hz ());

            out_normals.emplace_back (average_normal.hx ());
            out_normals.emplace_back (average_normal.hy ());
            out_normals.emplace_back (average_normal.hz ());
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // -> Bottom face indices
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    for (int i = 0; i < top_outline_face_vertex_indices.size (); i += 3)
        {
            int fid = i / 3;
            int a = (int)vertex_count + top_outline_face_vertex_indices[i + 2];
            int b = (int)vertex_count + top_outline_face_vertex_indices[i + 1];
            int c = (int)vertex_count + top_outline_face_vertex_indices[i + 0];

            int v_count = out_triangles.size ();
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

            out_normals.emplace_back (-average_normal.hx ());
            out_normals.emplace_back (-average_normal.hy ());
            out_normals.emplace_back (-average_normal.hz ());

            out_normals.emplace_back (-average_normal.hx ());
            out_normals.emplace_back (-average_normal.hy ());
            out_normals.emplace_back (-average_normal.hz ());

            out_normals.emplace_back (-average_normal.hx ());
            out_normals.emplace_back (-average_normal.hy ());
            out_normals.emplace_back (-average_normal.hz ());
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

            IK::Point_3 coord_0 (out_vertices_temp[a0 * 3 + 0], out_vertices_temp[a0 * 3 + 1], out_vertices_temp[a0 * 3 + 2]);
            IK::Point_3 coord_1 (out_vertices_temp[b0 * 3 + 0], out_vertices_temp[b0 * 3 + 1], out_vertices_temp[b0 * 3 + 2]);
            IK::Point_3 coord_2 (out_vertices_temp[c0 * 3 + 0], out_vertices_temp[c0 * 3 + 1], out_vertices_temp[c0 * 3 + 2]);
            IK::Vector_3 normal_0 = -CGAL::cross_product (coord_0 - coord_1, coord_2 - coord_1);
            internal::unitize (normal_0);

            out_normals.emplace_back (normal_0.hx ());
            out_normals.emplace_back (normal_0.hy ());
            out_normals.emplace_back (normal_0.hz ());

            out_normals.emplace_back (normal_0.hx ());
            out_normals.emplace_back (normal_0.hy ());
            out_normals.emplace_back (normal_0.hz ());

            out_normals.emplace_back (normal_0.hx ());
            out_normals.emplace_back (normal_0.hy ());
            out_normals.emplace_back (normal_0.hz ());

            IK::Point_3 coord_3 (out_vertices_temp[a1 * 3 + 0], out_vertices_temp[a1 * 3 + 1], out_vertices_temp[a1 * 3 + 2]);
            IK::Point_3 coord_4 (out_vertices_temp[b1 * 3 + 0], out_vertices_temp[b1 * 3 + 1], out_vertices_temp[b1 * 3 + 2]);
            IK::Point_3 coord_5 (out_vertices_temp[c1 * 3 + 0], out_vertices_temp[c1 * 3 + 1], out_vertices_temp[c1 * 3 + 2]);

            out_normals.emplace_back (normal_0.hx ());
            out_normals.emplace_back (normal_0.hy ());
            out_normals.emplace_back (normal_0.hz ());

            out_normals.emplace_back (normal_0.hx ());
            out_normals.emplace_back (normal_0.hy ());
            out_normals.emplace_back (normal_0.hz ());

            out_normals.emplace_back (normal_0.hx ());
            out_normals.emplace_back (normal_0.hy ());
            out_normals.emplace_back (normal_0.hz ());
        }

    return;
}

void
mesh_from_polylines (const std::vector<CGAL_Polyline> &polylines_with_holes, const IK::Plane_3 &base_plane, std::vector<int> &top_outline_face_vertex_indices, int &v_count, int &f_count)
{
    //////////////////////////////////////////////////////////////////////////////
    // Create Transformation | Orient to 2D
    //////////////////////////////////////////////////////////////////////////////
    CGAL::Aff_transformation_3<IK> xform_toXY = internal::plane_to_xy (polylines_with_holes[0][0], base_plane);
    CGAL::Aff_transformation_3<IK> xform_toXY_Inv = xform_toXY.inverse ();

    CGALCDT CGALCDT;
    for (int i = 0; i < polylines_with_holes.size (); i += 2)
        {
            Polygon_2 polygon_2d;
            for (int j = 0; j < polylines_with_holes[i].size () - 1; j++)
                {
                    IK::Point_3 p = polylines_with_holes[i][j].transform (xform_toXY);
                    auto pt_2d = Point (p.hx (), p.hy ());

                    CGALCDT::Locate_type l_t;
                    int l_i;
                    CGALCDT.locate (pt_2d, l_t, l_i);

                    if (l_t == CGALCDT::VERTEX)
                        {
                            printf ("CPP CGALCDT::VERTEX \n");

                            return;
                        }

                    polygon_2d.push_back (pt_2d);
                }

            if (!polygon_2d.is_simple ())
                {
                    printf ("CPP Not simple \n");
                    return;
                }

            //////////////////////////////////////////////////////////////////////////////
            // Insert the polygons into a constrained triangulation
            //////////////////////////////////////////////////////////////////////////////
            CGALCDT.insert_constraint (polygon_2d.vertices_begin (), polygon_2d.vertices_end (), true);
        }

    //////////////////////////////////////////////////////////////////////////////
    // Mark facets that are inside the domain bounded by the polygon
    //////////////////////////////////////////////////////////////////////////////
    internal::mark_domains (CGALCDT);

    int count = 0;
    for (Face_handle f : CGALCDT.finite_face_handles ())
        {
            if (f->info ().in_domain ())
                {
                    ++count;
                }
        }

    std::map<CGALCDT::Vertex_handle, int> vertex_index;
    int k = 0;
    for (auto it = CGALCDT.vertices_begin (); it != CGALCDT.vertices_end (); ++it)
        {
            auto p = it->point ();
            vertex_index[it] = k;
            k++;
        }
    v_count = k;

    // count vertices to check if there are same number of points as in polyline
    size_t vertex_count = 0;
    for (int i = 0; i < polylines_with_holes.size (); i += 2)
        vertex_count += polylines_with_holes[i].size () - 1;

    if (v_count != vertex_count)
        {
            top_outline_face_vertex_indices = std::vector<int> (0);
            return;
        }

    int number_of_faces = 0;
    for (Face_handle f : CGALCDT.finite_face_handles ())
        if (f->info ().in_domain ())
            number_of_faces += 3;

    f_count = number_of_faces / 3;

    top_outline_face_vertex_indices.reserve (number_of_faces);
    for (Face_handle f : CGALCDT.finite_face_handles ())
        {
            if (f->info ().in_domain ())
                {
                    top_outline_face_vertex_indices.emplace_back (vertex_index[f->vertex (0)]);
                    top_outline_face_vertex_indices.emplace_back (vertex_index[f->vertex (1)]);
                    top_outline_face_vertex_indices.emplace_back (vertex_index[f->vertex (2)]);
                }
        }
}

// done
void
add_mesh_boolean_difference (std::vector<CGAL_Polyline> &input_plines, std::vector<std::vector<CGAL_Polyline> > &output_plines)
{
    for (int i = 0; i < input_plines.size (); i += 2)
        {
            std::vector<CGAL_Polyline> input_plines_pair = { input_plines[i], input_plines[i + 1] };

            // create mesh list for boolean difference
            std::vector<CGAL::Surface_mesh<CGAL::Exact_predicates_inexact_constructions_kernel::Point_3> > mesh_list;

            // create mesh from outlines
            mesh_list.emplace_back (CGAL::Surface_mesh<CGAL::Exact_predicates_inexact_constructions_kernel::Point_3> ());
            cgal::polyline_mesh_util::closed_mesh_from_polylines (input_plines_pair, mesh_list[0], 1000);

            // create mesh from genereated joints - output_plines
            for (int j = 0; j < output_plines[(size_t)(i * 0.5)].size (); j += 2)
                {
                    mesh_list.emplace_back (CGAL::Surface_mesh<CGAL::Exact_predicates_inexact_constructions_kernel::Point_3> ());
                    std::vector<CGAL_Polyline> polylines_single = { output_plines[(size_t)(i * 0.5)][j], output_plines[(size_t)(i * 0.5)][j + 1] };
                    cgal::polyline_mesh_util::closed_mesh_from_polylines (polylines_single, mesh_list.back (), 1000);
                }

            for (size_t i = 0; i < mesh_list.size (); i++)
                {
                    if (!CGAL::is_closed (mesh_list[i]))
                        std::cerr << "mesh is not closed." << i << std::endl;

                    if (CGAL::Polygon_mesh_processing::does_self_intersect (mesh_list[i]))
                        std::cerr << "mesh has self-intersections." << i << std::endl;
                }

            // create boolean difference
            SQLMesh sqlmesh;
            mesh_boolean_difference_to_viewer (mesh_list, 0, sqlmesh.vertices, sqlmesh.normals, sqlmesh.indices);

            // add to viewer
            if (sqlmesh.vertices.size () > 2 && sqlmesh.normals.size () > 2 && sqlmesh.indices.size () > 2)
                SQL_MESHES.emplace_back (sqlmesh);
        }
}

void
mesh_boolean_difference_to_viewer (std::vector<Mesh> &mesh_list, size_t difference_union_intersection, std::vector<double> &out_vertices, std::vector<double> &out_normals, std::vector<int> &out_triangles)
{
    ////////////////////////////////////////////////////////////////
    // Perform CGAL Boolean
    ////////////////////////////////////////////////////////////////

    CGAL::Surface_mesh<CGAL::Exact_predicates_inexact_constructions_kernel::Point_3> &lastMesh = mesh_list[0];
    Mesh::Property_map<Mesh::Face_index, int> lastMesh_id = lastMesh.add_property_map<Mesh::Face_index, int> ("f:id", -1).first;
    for (Mesh::Face_index f : faces (lastMesh))
        lastMesh_id[f] = 1;

    for (int i = 1; i < mesh_list.size (); i++)
        { // mesh_list.size ()

            internal::Visitor visitor;

            // Properties last mesh
            // Mesh::Property_map<Mesh::Face_index, int>  lastMesh_id =
            // lastMesh.add_property_map<Mesh::Face_index, int> ("f:id", -1).first; for (
            // Mesh::Face_index f : faces (lastMesh) )
            //     lastMesh_id[f] = 1;
            visitor.properties[&lastMesh] = lastMesh_id; // From previous iteration must or must not contain property map?

            ////Properties current mesh
            Mesh::Property_map<Mesh::Face_index, int> mesh_id = mesh_list[i].add_property_map<Mesh::Face_index, int> ("f:id", -1).first;
            for (Mesh::Face_index f : faces (mesh_list[i]))
                mesh_id[f] = (i + 1);
            visitor.properties[&mesh_list[i]] = mesh_id;

            ////Properties Out
            Mesh out;
            Mesh::Property_map<Mesh::Face_index, int> out_id = out.add_property_map<Mesh::Face_index, int> ("f:id", -1).first;
            visitor.properties[&out] = out_id;

            bool valid_union = false;
            const bool throw_on_self_intersection = true;

            try
                {
                    if (difference_union_intersection == 1)
                        {
                            valid_union = CGAL::Polygon_mesh_processing::corefine_and_compute_union (
                                lastMesh, mesh_list[i], out, CGAL::Polygon_mesh_processing::parameters::visitor (visitor),
                                CGAL::Polygon_mesh_processing::parameters::throw_on_self_intersection (true)); //, ,
                                                                                                               // CGAL::Polygon_mesh_processing::parameters::throw_on_self_intersection
                                                                                                               //(true) , CGAL::Polygon_mesh_processing::parameters::visitor
                                                                                                               //(visitor)
                        }
                    else if (difference_union_intersection == 2)
                        {
                            valid_union = CGAL::Polygon_mesh_processing::corefine_and_compute_intersection (
                                lastMesh, mesh_list[i], out, CGAL::Polygon_mesh_processing::parameters::visitor (visitor),
                                CGAL::Polygon_mesh_processing::parameters::throw_on_self_intersection (true)); //, CGAL::Polygon_mesh_processing::parameters::visitor (visitor)
                        }
                    else
                        {
                            valid_union = CGAL::Polygon_mesh_processing::corefine_and_compute_difference (
                                lastMesh, mesh_list[i], out, CGAL::Polygon_mesh_processing::parameters::visitor (visitor),
                                CGAL::Polygon_mesh_processing::parameters::throw_on_self_intersection (true)); //, CGAL::Polygon_mesh_processing::parameters::visitor (visitor)
                        }

                    lastMesh = valid_union ? out : mesh_list[i];

                    lastMesh_id = lastMesh.add_property_map<Mesh::Face_index, int> ("f:id", -1).first;
                    for (Mesh::Face_index f : faces (out))
                        {
                            auto faceID = out_id[f];
                            lastMesh_id[f] = faceID;
                        }
                }
            catch (const std::exception &e)
                {
                    std::cout << "Exception: " << e.what () << "\n";
                    lastMesh = mesh_list[i];
                }
        }

    ////////////////////////////////////////////////////////////////
    // Return vertex and normals and faces
    ////////////////////////////////////////////////////////////////

    // Compute mesh vertex normals
    Mesh::Property_map<boost::graph_traits<Mesh>::vertex_descriptor, IK::Vector_3> vnormals
        = lastMesh.template add_property_map<boost::graph_traits<Mesh>::vertex_descriptor, IK::Vector_3> ("v:normals", CGAL::NULL_VECTOR).first;
    CGAL::Polygon_mesh_processing::compute_vertex_normals (lastMesh, vnormals);

    const int vc = lastMesh.number_of_vertices () * 3;

    out_vertices = std::vector<double> (vc);
    out_normals = std::vector<double> (vc);

    int i = 0;
    int c = 0;
    for (auto &ni : vnormals)
        {
            out_normals[i + 0] = (double)ni.hx ();
            out_normals[i + 1] = (double)ni.hy ();
            out_normals[i + 2] = (double)ni.hz ();
            i += 3;
            c++;
        }

    // Output
    //  Get vertices coordinates

    i = 0;
    c = 0;
    for (const auto &vi : lastMesh.vertices ())
        {
            const auto &pt = lastMesh.point (vi);
            out_vertices[i + 0] = (double)pt.x ();
            out_vertices[i + 1] = (double)pt.y ();
            out_vertices[i + 2] = (double)pt.z ();
            i += 3;
            c++;
        }

    // Get face indices
    const int fc = lastMesh.number_of_faces () * 3;
    out_triangles = std::vector<int> (fc);

    i = 0;
    c = 0;
    for (auto face_index : lastMesh.faces ())
        {
            std::vector<uint32_t> indices;
            CGAL::Vertex_around_face_circulator<CGAL::Surface_mesh<CGAL::Exact_predicates_inexact_constructions_kernel::Point_3> > vcirc (lastMesh.halfedge (face_index), lastMesh), done (vcirc);
            do
                indices.push_back (*vcirc++);
            while (vcirc != done);

            out_triangles[i++] = (int)indices[0];
            out_triangles[i++] = (int)indices[1];
            out_triangles[i++] = (int)indices[2];
            c++;
        }

    return;
}

void
add_points (const std::vector<IK::Point_3> &points)
{
    SQLPointCloud sqlpointcloud;

    // points
    for (const auto &point : points)
        {
            sqlpointcloud.vertices.emplace_back (static_cast<double> (point.hx () / SCALE));
            sqlpointcloud.vertices.emplace_back (static_cast<double> (point.hy () / SCALE));
            sqlpointcloud.vertices.emplace_back (static_cast<double> (point.hz () / SCALE));
        }

    // normals
    for (const auto &point : points)
        {
            sqlpointcloud.normals.emplace_back (0.0f);
            sqlpointcloud.normals.emplace_back (0.0f);
            sqlpointcloud.normals.emplace_back (1.0f);
        }

    // colors convert the color hex to rgb
    std::string hex = COLOR;
    unsigned int rgb;
    std::stringstream ss;
    ss << std::hex << hex.substr (1); // Remove '#' and convert
    ss >> rgb;

    double r = ((rgb >> 16) & 0xFF) / 255.0f;
    double g = ((rgb >> 8) & 0xFF) / 255.0f;
    double b = (rgb & 0xFF) / 255.0f;
    double a = 1.0f; // Assuming no alpha in the string, default to 1

    // If the string contains alpha value (e.g., #RRGGBBAA)
    if (hex.size () == 9)
        {
            unsigned int alpha;
            ss.clear ();
            ss << std::hex << hex.substr (7, 2);
            ss >> alpha;
            a = alpha / 255.0f;
        }

    for (const auto &point : points)
        {
            sqlpointcloud.colors.emplace_back (r);
            sqlpointcloud.colors.emplace_back (g);
            sqlpointcloud.colors.emplace_back (b);
        }

    SQL_POINTCLOUDS.emplace_back (sqlpointcloud);
}

} // namespace database_writer