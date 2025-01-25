
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

        void run(std::vector<float>& v, std::vector<int>& f, std::vector<CGAL_Polyline>& output)
        {
            CGAL::Polyhedron_3<CK> mesh;
            from_vertices_and_faces(v, f, mesh);
            
            Skeleton skeleton;
            CGAL::extract_mean_curvature_flow_skeleton(mesh, skeleton);
            
            internal::SkeletonConversion skeleton_conversion (skeleton, output, mesh);
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

        std::vector<IK::Point_3> generate_equally_spaced_points(const std::vector<std::vector<IK::Point_3>>& polylines, int numPoints) {
            
            std::vector<IK::Point_3> polyline = polylines[0];
            for (int i = 1; i < polylines.size(); ++i) {
                polyline.insert(polyline.end(), polylines[i].begin(), polylines[i].end());
            }


            // Compute cumulative lengths
            std::vector<double> lengths = computeCumulativeLengths(polyline);
            double totalLength = lengths.back();
            double segmentLength = totalLength / (numPoints - 1);

            std::vector<IK::Point_3> result;
            result.push_back(polyline[0]);  // First point
            // Generate points at equal intervals
            double currentTarget = segmentLength;
            size_t currentIdx = 0;

            for (int i = 1; i < numPoints - 1; ++i) {
                // Find the segment where the target falls
                while (currentIdx < lengths.size() - 1 && lengths[currentIdx + 1] < currentTarget) {
                    ++currentIdx;
                }

                // Interpolate within the segment
                double t = (currentTarget - lengths[currentIdx]) /
                        (lengths[currentIdx + 1] - lengths[currentIdx]);
                result.push_back(interpolate(polyline[currentIdx], polyline[currentIdx + 1], t));
                currentTarget += segmentLength;
            }

            result.push_back(polyline.back());  // Last point
            return result;
        }

    }

} // namespace cgal