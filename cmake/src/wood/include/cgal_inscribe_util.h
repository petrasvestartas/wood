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
// Get center with radius in a polygon: https://github.com/mapbox/polylabel
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CGAL_INSCRIBE_UTIL_H
#define CGAL_INSCRIBE_UTIL_H

#include <polylabel/polylabel.h>
#include "../../../ext/session_cpp/src/point.h"
#include "../../../ext/session_cpp/src/vector.h"
#include "../../../ext/session_cpp/src/plane.h"
#include "../../../ext/session_cpp/src/line.h"

namespace cgal
{
namespace inscribe_util
{

/**
 * inscribe circle in a polygon (with or without holes)
 * the polyline is oriented to XY plane using the input plane
 * then oriented back to 3D
 *
 * @param polylines planar polylines rotated in space
 * @param precision precision, the smaller the number the less precise the result, default 1.0
 * @return a tuple that represents the circle (center, plane, radius)
 */
std::tuple<session_cpp::Point, session_cpp::Plane, double> get_polylabel (const std::vector<Polyline> &polylines, double precision = 1.0);

/**
 * get points inscribed in a polylabel circle
 * @param [in] division_direction_in_3d division direction
 * @param [in] polylines list of polylines that represent the input for the polylabel algorithm
 * @param [out] points division points
 * @param [in] division number of points
 * @param [in] scale scale of the circle
 * @param [in] precision tolerance for the polylabel algorithm
 * @param [in] orient_to_closest_edge if true, the points are oriented to the closest edge
 */
void get_polylabel_circle_division_points (const session_cpp::Vector &division_direction_in_3d, const std::vector<Polyline> &polylines, std::vector<session_cpp::Point> &points, int division = 4, double scale = 0.75,
                                           double precision = 1.0, bool orient_to_closest_edge = true);

/**
 * inscribe rectangle in a convex polygon and divide its edges into points or create rectangle grid
 *
 * @param [in] polylines input polylines (with holes)
 * @param [out] result output rectangle inscribed between division points of the polygon
 * @param [out] points output division points
 * @param [in] direction segment in 3D that represents the orientation of the rectangle
 * @param [in] scale the rectangle scale
 * @param [in] precision numeric parameter that subdivides more or less points of the polygon
 * @param [in] rectangle_division_distance when division distance is below 0, the rectangle grid is
 * created, if above, the rectangle's edges are subdivided
 * @return bool flag if the result is valid
 */
bool inscribe_rectangle_in_convex_polygon (const std::vector<Polyline> &polylines, Polyline &result, std::vector<session_cpp::Point> &points, session_cpp::Line &direction, double scale = 0, double precision = 1.0,
                                           double rectangle_division_distance = 10);

} // namespace inscribe_util
} // namespace cgal
#endif
