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

            struct SkeletonConversion {
                const Skeleton& skeleton;
                std::vector<double>& out;
                std::vector<int>& outID;
                int id = 0;

                SkeletonConversion (const Skeleton& skeleton, std::vector<double>& out, std::vector<int>& outID)
                    : skeleton (skeleton), out (out), outID (outID) {}

                void start_new_polyline () {
                    id++;
                }
                void add_node (Skeleton_vertex v) {
                    out.push_back (skeleton[v].point.x ());
                    out.push_back (skeleton[v].point.y ());
                    out.push_back (skeleton[v].point.z ());
                    outID.push_back (id);
                }
                void end_polyline () {}
            };


            // A modifier creating a triangle with the incremental builder.
            template <class HDS>
            class polyhedron_builder : public CGAL::Modifier_base<HDS> {
            public:
                std::vector<double>& coords;
                std::vector<int>& tris;
                polyhedron_builder (std::vector<double>& _coords, std::vector<int>& _tris)
                    : coords (_coords), tris (_tris) {}
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

        }


        // This example extracts a medially centered skeleton from a given mesh.
        void main(std::vector<double>& coords, std::vector<int>& tris)
        {

            CGAL::Polyhedron_3<CK> tmesh;
            internal::polyhedron_builder<HalfedgeDS> builder (coords, tris);
            tmesh.delegate (builder);


            if (!CGAL::is_triangle_mesh(tmesh))
            {
                std::cout << "Input geometry is not triangulated." << std::endl;
                return;
            }
            
            Skeleton skeleton;
            CGAL::extract_mean_curvature_flow_skeleton(tmesh, skeleton);
            
            std::cout << "Number of vertices of the skeleton: " << boost::num_vertices(skeleton) << "\n";
            std::cout << "Number of edges of the skeleton: " << boost::num_edges(skeleton) << "\n";
            
            // Output all the edges of the skeleton.
            std::vector<double> output;
            std::vector<int> outputID;
            internal::SkeletonConversion skeleton_conversion (skeleton, output, outputID);
            CGAL::split_graph_into_polylines (skeleton, skeleton_conversion);
            
        }
 
    }
} // namespace cgal

#endif // CGAL_SKELETON_H