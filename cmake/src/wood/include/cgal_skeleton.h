///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVELOPER:
// Petras Vestartas, petasvestartas@gmail.com
// Funding: NCCR Digital Fabrication and EPFL
//
// HISTORY:
// 1) The first version was written during the PhD 8928 thesis of Petras Vestartas called:
// Design-to-Fabrication Workflow for Raw-Sawn-Timber using Joinery Solver, 2017-2021
// 2) The translation from C# to C++ was started during the funding of NCCR in two steps
// A - standalone C++ version of the joinery solver and B - integration to COMPAS framework (Python Pybind11)
//
// RESTRICTIONS:
// The code cannot be used for commercial reasons
// If you would like to use or change the code for research or educational reasons,
// please contact the developer first
//
// 3RD PARTY LIBRARIES:
// CGAL: https://doc.cgal.org/latest/Surface_mesh_skeletonization/index.html
// BLOGPOST:  http://jamesgregson.blogspot.com/2012/05/example-code-for-building.html
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef CGAL_SKELETON_H
#define CGAL_SKELETON_H


namespace cgal
{


    namespace skeleton
    {


        namespace internal{

            /**
             * @brief A struct to convert a skeleton to polylines.
             */
            struct SkeletonConversion {
                const Skeleton& skeleton;
                std::vector<CGAL_Polyline>& out;
                CGAL::Polyhedron_3<CK>& mesh;
                CGAL_Polyline polyline = CGAL_Polyline ();

                /**
                 * @brief Constructor for SkeletonConversion.
                 * @param skeleton The input skeleton.
                 * @param out The output vector of polylines.
                 * @param mesh The input mesh.
                 */
                SkeletonConversion (const Skeleton& skeleton, std::vector<CGAL_Polyline>& out, CGAL::Polyhedron_3<CK>& mesh)
                    : skeleton (skeleton), out (out), mesh (mesh) {}
                
                /**
                 * @brief Start a new polyline.
                 */
                void start_new_polyline () {
                    polyline = CGAL_Polyline ();
                }

                /**
                 * @brief Add a node to the current polyline.
                 * @param v The skeleton vertex to add.
                 */
                void add_node (Skeleton_vertex v) {
                    polyline.push_back({skeleton[v].point.x(), skeleton[v].point.y(), skeleton[v].point.z()});
                }

                /**
                 * @brief End the current polyline and add it to the output vector.
                 */
                void end_polyline () {
                    out.push_back (polyline);
                }
            };


            /**
             * @brief A modifier creating a triangle with the incremental builder.
             * @tparam HDS The halfedge data structure.
             */
            template <class HDS>
            class polyhedron_builder : public CGAL::Modifier_base<HDS> {
            public:
                std::vector<double>& coords;
                std::vector<int>& tris;


                /**
                 * @brief Constructor for polyhedron_builder.
                 * @param _coords The coordinates of the vertices.
                 * @param _tris The indices of the triangles.
                 */
                polyhedron_builder (std::vector<double>& _coords, std::vector<int>& _tris)
                    : coords (_coords), tris (_tris) {}

                /**
                 * @brief Build the polyhedron.
                 * @param hds The halfedge data structure.
                 */
                void operator()(HDS& hds) {
                    typedef typename HDS::Vertex Vertex;
                    typedef typename Vertex::Point Point;

                    // create a cgal incremental builder
                    CGAL::Polyhedron_incremental_builder_3<HDS> B (hds, true);
                    B.begin_surface (coords.size () / 3, tris.size () / 3);

                    // add the polyhedron vertices
                    for ( int i = 0; i < (int)coords.size (); i += 3 ) {
                        B.add_vertex (Point (coords[i + 0], coords[i + 1], coords[i + 2]));
                    }

                    // add the polyhedron triangles
                    for ( int i = 0; i < (int)tris.size (); i += 3 ) {
                        B.begin_facet ();
                        B.add_vertex_to_facet (tris[i + 0]);
                        B.add_vertex_to_facet (tris[i + 1]);
                        B.add_vertex_to_facet (tris[i + 2]);
                        B.end_facet ();
                    }

                    // finish up the surface
                    B.end_surface ();
                }
            };


            /**
             * @brief Orders the points to form a continuous polyline.
             * 
             * @param points The input vector of points.
             * @return A vector of points ordered to form a continuous polyline.
             */
            std::vector<IK::Point_3> orderPoints(const std::vector<IK::Point_3>& points);

            /**
             * @brief Computes the cumulative lengths of the segments in the polyline.
             * 
             * @param polyline The input polyline.
             * @return A vector of cumulative lengths for each segment in the polyline.
             */
            std::vector<double> compute_cumulative_lengths(const std::vector<IK::Point_3>& polyline);

            /**
             * @brief Interpolates between two points.
             * 
             * @param p1 The first point.
             * @param p2 The second point.
             * @param t The interpolation parameter (0 <= t <= 1).
             * @return The interpolated point.
             */
            IK::Point_3 interpolate(const IK::Point_3& p1, const IK::Point_3& p2, double t);

            /**
             * @brief Constructs a CGAL polyhedron from vertices and faces.
             * 
             * @param v The vertices.
             * @param f The faces.
             * @param mesh The output CGAL polyhedron.
             */
            void from_vertices_and_faces(std::vector<double>& v, std::vector<int>& f, CGAL::Polyhedron_3<CK>& mesh);
        }

        /**
         * @brief Run the skeleton extraction algorithm.
         * @param v The vertices.
         * @param f The faces.
         * @param output_polylines The output vector of polylines.
         * @param output_mesh OPTIONAL: The output CGAL polyhedron.
         * 
         * @example
         * std::string filepath = "C:/Users/petras/Desktop/dev_wood/wood_log.ply"; 
         * std::vector <double> v;
         * std::vector <int> f;
         * tinyply::read(filepath, v, f, false);
         * std::vector<CGAL_Polyline> output_polylines;
         * CGAL::Polyhedron_3<CK> output_mesh;
         * cgal::skeleton::mesh_skeleton(v, f, output_polylines, &output_mesh);
         * // Run Skeleton > equally space points > get distances > extend skeleton
         * CGAL_Polyline output_polyline;
         * std::vector<double> output_distances;
         * cgal::skeleton::divide_polyline(output_polylines, 10, output_polyline);
         * cgal::skeleton::find_nearest_mesh_distances(output_mesh, output_polyline, 10, output_distances);
         * cgal::skeleton::extend_polyline_to_mesh(output_mesh, output_polyline, output_distances);
         */
        void mesh_skeleton(std::vector<double>& v, std::vector<int>& f, std::vector<CGAL_Polyline>& output_polylines, CGAL::Polyhedron_3<CK>* output_mesh);

        /**
         * @brief Run the skeleton extraction algorithm.
         * 
         * @param v The vertices.
         * @param f The faces.
         * @param output_polylines The output vector of polylines.
         * 
         * @example
         * std::string filepath = "C:/Users/petras/Desktop/dev_wood/wood_log.ply";  // icosahedron_ascii
         * std::vector<double> v;
         * std::vector<int> f;
         * tinyply::read(filepath, v, f, false);
         * 
         * std::vector<CGAL_Polyline> output_polylines;
         * cgal::skeleton::mesh_skeleton(v, f, output_polylines);
         */
        void mesh_skeleton(std::vector<double>& v, std::vector<int>& f, std::vector<CGAL_Polyline>& output_polylines);

        /**
         * @brief Generate equally spaced points along the polylines.
         * @param polylines The input polylines.
         * @param divisions The number of points to generate.
         * @param output_polyline The output polyline.
         */
        void divide_polyline(const std::vector<std::vector<IK::Point_3>>& polylines, int divisions, std::vector<IK::Point_3>& output_polyline);


        /**
         * @brief Computes the average distances from each point in the polyline to its nearest neighbors in the mesh.
         * 
         * @param mesh The input CGAL polyhedron mesh.
         * @param polyline The input polyline.
         * @param neighbors The number of nearest neighbors to consider for each point in the polyline.
         * @param output_distances The output vector to store the average distances.
         */
        void find_nearest_mesh_distances(CGAL::Polyhedron_3<CK>& mesh, CGAL_Polyline& polyline, int neighbors, std::vector<double>& output_distances) ;

        /**
         * @brief Extends the skeleton by computing intersections of segment queries with the mesh and updating the polyline and distances.
         * 
         * @param mesh The input CGAL polyhedron mesh.
         * @param polyline The input polyline to be extended.
         * @param output_distances The output vector to store the distances corresponding to the extended polyline.
         */
        void extend_polyline_to_mesh(CGAL::Polyhedron_3<CK>& mesh, CGAL_Polyline& polyline, std::vector<double>& output_distances);

        /**
         * @brief Run the beam skeleton extraction algorithm.
         * @param v The vertices.
         * @param f The faces.
         * @param output_polyline The output polyline.
         * @param output_distances The output vector to store the distances corresponding to the output polyline.
         * @param divisions The number of points to generate along the polylines.
         * @param nearest_neighbors The number of nearest neighbors to consider for each point in the polyline.
         * @param extend Whether to extend the polyline to the mesh.
         */
        void beam_skeleton(std::vector<double>& v, std::vector<int>& f, CGAL_Polyline& output_polyline, std::vector<double>& output_distances, int divisions=0, int nearest_neighbors=0, bool extend=false);
    }
} // namespace cgal

#endif // CGAL_SKELETON_H