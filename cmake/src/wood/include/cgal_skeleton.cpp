
#include "../../../stdafx.h" //go up to the folder where the CMakeLists.txt is

#include "cgal_skeleton.h"

namespace cgal
{
    namespace skeleton
    {
       void from_vertices_and_faces(std::vector<float>& v, std::vector<int>& f, CGAL::Polyhedron_3<CK>& mesh){
            cgal::skeleton::internal::polyhedron_builder<HalfedgeDS> builder (v, f);
            mesh.delegate (builder);

            if (!CGAL::is_triangle_mesh(mesh))
            {
                std::cout << "Input geometry is not triangulated." << std::endl;
                return;
            }
        }

        void run(std::vector<float>& v, std::vector<int>& f, CGAL::Polyhedron_3<CK>& output_mesh, std::vector<CGAL_Polyline>& output_polylines)
        {
            from_vertices_and_faces(v, f, output_mesh);
            
            Skeleton skeleton;
            CGAL::extract_mean_curvature_flow_skeleton(output_mesh, skeleton);
            
            internal::SkeletonConversion skeleton_conversion (skeleton, output_polylines, output_mesh);
            CGAL::split_graph_into_polylines (skeleton, skeleton_conversion);

        }

        std::vector<IK::Point_3> orderPoints(const std::vector<IK::Point_3>& points) {
            std::vector<IK::Point_3> ordered;
            std::vector<bool> visited(points.size(), false);

            // Start from the first point
            ordered.push_back(points[0]);
            visited[0] = true;

            for (size_t i = 1; i < points.size(); ++i) {
                double minDist = std::numeric_limits<double>::max();
                int nextIdx = -1;

                // Find the nearest unused point
                for (size_t j = 0; j < points.size(); ++j) {
                    if (!visited[j]) {
                        double dist =  std::sqrt(CGAL::squared_distance(ordered.back(), points[j]));
                        if (dist < minDist) {
                            minDist = dist;
                            nextIdx = j;
                        }
                    }
                }

                // Add the nearest point to the path
                if (nextIdx != -1) {
                    ordered.push_back(points[nextIdx]);
                    visited[nextIdx] = true;
                }
            }
            return ordered;
        }

        // Function to compute the cumulative arc lengths of the polyline
        std::vector<double> computeCumulativeLengths(const std::vector<IK::Point_3>& polyline) {
            std::vector<double> lengths(polyline.size(), 0.0);
            for (size_t i = 1; i < polyline.size(); ++i) {
                lengths[i] = lengths[i - 1] + std::sqrt(CGAL::squared_distance(polyline[i - 1], polyline[i]));
            }
            return lengths;
        }

        // Linear interpolation between two points
        IK::Point_3 interpolate(const IK::Point_3& p1, const IK::Point_3& p2, double t) {
            return {
                p1.x() + t * (p2.x() - p1.x()),
                p1.y() + t * (p2.y() - p1.y()),
                p1.z() + t * (p2.z() - p1.z())
            };
        }


        void divide_polyline(const std::vector<std::vector<IK::Point_3>>& polylines, int divisions, std::vector<IK::Point_3>& output_polyline) {
            
            std::vector<IK::Point_3> polyline = polylines[0];
            for (int i = 1; i < polylines.size(); ++i) {
                polyline.insert(polyline.end(), polylines[i].begin(), polylines[i].end());
            }


            // Compute cumulative lengths
            std::vector<double> lengths = computeCumulativeLengths(polyline);
            double totalLength = lengths.back();
            double segmentLength = totalLength / (divisions - 1);

            output_polyline.push_back(polyline[0]);  // First point
            // Generate points at equal intervals
            double currentTarget = segmentLength;
            size_t currentIdx = 0;

            for (int i = 1; i < divisions - 1; ++i) {
                // Find the segment where the target falls
                while (currentIdx < lengths.size() - 1 && lengths[currentIdx + 1] < currentTarget) {
                    ++currentIdx;
                }

                // Interpolate within the segment
                double t = (currentTarget - lengths[currentIdx]) /
                        (lengths[currentIdx + 1] - lengths[currentIdx]);
                output_polyline.push_back(interpolate(polyline[currentIdx], polyline[currentIdx + 1], t));
                currentTarget += segmentLength;
            }

            output_polyline.push_back(polyline.back());  // Last point
        }


        void find_nearest_mesh_distances(CGAL::Polyhedron_3<CK>& mesh, CGAL_Polyline& polyline, int neighbors, std::vector<float>& output_distances) {
            using Point = boost::graph_traits<CGAL::Polyhedron_3<CK>>::vertex_descriptor;
            using Vertex_point_pmap = boost::property_map<CGAL::Polyhedron_3<CK>, CGAL::vertex_point_t>::type;
            
            using Traits_base = CGAL::Search_traits_3<CK>;
            using Traits = CGAL::Search_traits_adapter<Point, Vertex_point_pmap, Traits_base>;
            using Tree = CGAL::Orthogonal_k_neighbor_search<Traits>::Tree;

            using K_neighbor_search = CGAL::Orthogonal_k_neighbor_search<Traits>;
            using Splitter = Tree::Splitter;
            using Distance = K_neighbor_search::Distance;
            
            Vertex_point_pmap vppmap = get(CGAL::vertex_point, mesh);

            // Insert number_of_data_points in the tree
            Tree tree(vertices(mesh).begin(), vertices(mesh).end(), Splitter(), Traits(vppmap));

            for (auto& p : polyline) {
                // search K nearest neighbors
                CK::Point_3 query(p.x(), p.y(), p.z());
                Distance tr_dist(vppmap);

                const unsigned int K = neighbors;
                K_neighbor_search search(tree, query, K, 0, true, tr_dist);
                double total_distance = 0.0;
                int count = 0;

                for (K_neighbor_search::iterator it = search.begin(); it != search.end(); ++it) {
                    double distance = tr_dist.inverse_of_transformed_distance(it->second);
                    total_distance += distance;
                    count++;
                }

                if (count > 0) {
                    double average_distance = total_distance / count;
                    output_distances.push_back(static_cast<float>(average_distance));
                } else {
                    output_distances.push_back(0.0f); // or some other default value
                }
            }
        }


        void extend_polyline_to_mesh(CGAL::Polyhedron_3<CK>& mesh, CGAL_Polyline& polyline, std::vector<float>& output_distances) {
            //https://doc.cgal.org/latest/AABB_tree/index.html#Chapter_Fast_Intersection_and_Distance_Computation
            
            using Plane = CK::Plane_3 ;
            using Vector = CK::Vector_3 ;
            using Segment = CK::Segment_3 ;
            using Ray = CK::Ray_3 ;
            using Primitive = CGAL::AABB_face_graph_triangle_primitive<CGAL::Polyhedron_3<CK>> ;
            using Traits = CGAL::AABB_traits_3<CK, Primitive> ;
            using Tree = CGAL::AABB_tree<Traits> ;
            using Segment_intersection = std::optional< Tree::Intersection_and_primitive_id<Segment>::Type > ;
            using Plane_intersection = std::optional< Tree::Intersection_and_primitive_id<Plane>::Type > ;
            using Primitive_id = Tree::Primitive_id ;

            CK::Point_3 p0(polyline[0].x(), polyline[0].y(), polyline[0].z());
            CK::Point_3 p1(polyline[1].x(), polyline[1].y(), polyline[1].z());
            CK::Point_3 p2(polyline[polyline.size()-2].x(), polyline[polyline.size()-2].y(), polyline[polyline.size()-2].z());
            CK::Point_3 p3(polyline[polyline.size()-1].x(), polyline[polyline.size()-1].y(), polyline[polyline.size()-1].z());

            // constructs AABB tree
            Tree tree(faces(mesh).first, faces(mesh).second, mesh);
           // constructs segment queries
            auto vector0 = p0 - p1;
            vector0 *= 1000;
            Segment segment_query0(p1, p0 + vector0);

            auto vector1 = p3 - p2;
            vector1 *= 1000;
            Segment segment_query1(p3, p2 + vector1);

            // tests intersections with segment queries
            if (!tree.do_intersect(segment_query0) || !tree.do_intersect(segment_query1)) {
                std::cout << "no intersection" << std::endl;
                return;
            }


            // computes first encountered intersection with segment_query0
            auto intersection0 = tree.any_intersection(segment_query0);
            if (intersection0) {
                // gets intersection object
                if (const CK::Point_3* p = std::get_if<CK::Point_3>(&intersection0->first)) {
                    polyline.insert(polyline.begin(), IK::Point_3(p->x(), p->y(), p->z()));
                    output_distances.insert(output_distances.begin(), output_distances[0]);
                }
            }

            // computes first encountered intersection with segment_query1
            auto intersection1 = tree.any_intersection(segment_query1);
            if (intersection1) {
                // gets intersection object
                if (const CK::Point_3* p = std::get_if<CK::Point_3>(&intersection1->first)) {
                    polyline.push_back(IK::Point_3(p->x(), p->y(), p->z()));
                    output_distances.push_back(output_distances[output_distances.size()-1]);
                }
            }




        }

    }

} // namespace cgal