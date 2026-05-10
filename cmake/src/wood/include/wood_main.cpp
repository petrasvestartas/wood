
#include "wood_main.h"

#include "../../../stdafx.h" //go up to the folder where the CMakeLists.txt is

#pragma region GET ELEMENTS

namespace wood
{
namespace main
{

void
get_elements (std::vector<Polyline> &pp, std::vector<std::vector<session_cpp::Vector> > &insertion_vectors, std::vector<std::vector<int> > &JOINTS_TYPES, std::vector<wood::element> &elements)
{
    int n = (int)(pp.size () * 0.5);
    // elements = std::vector<wood::element>(n);
    elements.reserve (n);

    int count = 0;
    for (int i = 0; i < pp.size (); i += 2)
        {
            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Safety Check
            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            if (pp[i].size () != pp[i + 1].size ())
                continue;
            if (session_cpp::Point::squared_distance(pp[i][0], pp[i + 1][0]) < wood::GLOBALS::DISTANCE_SQUARED)
                continue;

            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Create Empty Element
            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // elements[id].id = i * 0.5;
            elements.emplace_back ((int)(i * 0.5)); // Create empty wood::element just with id

            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Get BoundingBox - AABB
            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Create copy of a polyline and transform points
            // Polyline twoPolylines;
            // twoPolylines.resize(pp[i].size() + pp[i + 1].size());
            // std::copy(pp[i].begin(), pp[i].end(), twoPolylines.begin());
            // std::copy(pp[i + 1].begin(), pp[i + 1].end(),
            // twoPolylines.begin() + pp[i].size());

            Polyline twoPolylines;
            twoPolylines.reserve (pp[i].size () + pp[i + 1].size ());

            for (auto &p : pp[i])
                twoPolylines.emplace_back (p);

            for (auto &p : pp[i + 1])
                twoPolylines.emplace_back (p);

            session_cpp::BoundingBox AABB = session_cpp::BoundingBox::from_points (twoPolylines, wood::GLOBALS::DISTANCE);

            elements[count].aabb = AABB;

            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Get Object Oriented BoundingBox - OOB -> transform to XY and
            // back to 3D
            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Create Planes
            session_cpp::Vector planeAxes[4];
            session_cpp::get_average_plane (pp[i], planeAxes);

            // Create Transformation
            session_cpp::Point  _o76  (planeAxes[0][0], planeAxes[0][1], planeAxes[0][2]);
            session_cpp::Vector _x76 = planeAxes[1], _y76 = planeAxes[2], _z76 = planeAxes[3];
            session_cpp::Xform xform_toXY = session_cpp::Xform::plane_to_xy (_o76, _x76, _y76, _z76);
            session_cpp::Xform xform_toXY_Inv = xform_toXY.inverse ().value ();

            // transform the merged polyline to xy and compute xyBounding
            // Box
            session_cpp::transform (twoPolylines, xform_toXY);

            // Compute bounding box, get center point, and construct 5 size
            // vector, where 5th dimension is half site,  then transform
            // box back to 3D by an inverse matrix
            session_cpp::BoundingBox AABBXY = session_cpp::BoundingBox::from_points (twoPolylines);
            double scale = 10;
            session_cpp::Vector box[5]
                = { {(AABBXY.min_point()[0] + AABBXY.max_point()[0]) * 0.5, (AABBXY.min_point()[1] + AABBXY.max_point()[1]) * 0.5, (AABBXY.min_point()[2] + AABBXY.max_point()[2]) * 0.5}, {1, 0, 0}, {0, 1, 0},
                    {0, 0, 1},
                    {fabs ((1.0 + wood::GLOBALS::DISTANCE * 1) * (AABBXY.max_point()[0] - AABBXY.min_point()[0]) * 0.5), // 0.00001
                     fabs ((1.0 + wood::GLOBALS::DISTANCE * 1) * (AABBXY.max_point()[1] - AABBXY.min_point()[1]) * 0.5), fabs ((1.0 + wood::GLOBALS::DISTANCE * 1) * (AABBXY.max_point()[2] - AABBXY.min_point()[2]) * 0.5)} };
            // CGAL_Debug(1.0 + wood::GLOBALS::DISTANCE * 0.00001);

            session_cpp::obb::transform_plane_as_vector_array (box, xform_toXY_Inv);
            session_cpp::obb::assign (box, elements[(int)(i * 0.5)].oob, 5);

            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Check orientation of polylines and reverse if needed
            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            bool reverse_poylines = twoPolylines.back ()[2] > 0;
            if (reverse_poylines)
                {
                    std::reverse (pp[i].begin (), pp[i].end ());
                    std::reverse (pp[i + 1].begin (), pp[i + 1].end ());
                }

            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Get Side Polylines and Planes
            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elements[count].polylines = std::vector<Polyline> (1 + pp[i].size ());
            elements[count].polylines[0] = pp[i];
            elements[count].polylines[1] = pp[i + 1];

            elements[count].planes = std::vector<session_cpp::Plane> (1 + pp[i].size ());

            // session_cpp::Point origin = session_cpp::center(pp[i]);
            session_cpp::Vector normal_sc;
            session_cpp::average_normal (pp[i], normal_sc);
            session_cpp::Vector normal = normal_sc;
            { session_cpp::Point _c0 = session_cpp::center (pp[i]); elements[count].planes[0] = session_cpp::Plane::from_point_normal (_c0, normal); }
            { session_cpp::Point _c1 = session_cpp::center (pp[i + 1]); session_cpp::Vector _neg_normal = normal * -1.0; elements[count].planes[1] = session_cpp::Plane::from_point_normal (_c1, _neg_normal); }
            elements[count].thickness = std::sqrt (session_cpp::Point::squared_distance (pp[i][0], elements[count].planes[1].project (pp[i][0])));
            // CGAL_Debug(elements[count].thickness);

            for (int j = 0; j < pp[i].size () - 1; j++)
                {
                    { std::vector<session_cpp::Point> _pts3 = {pp[i][j + 1], pp[i][j], pp[i + 1][j + 1]}; elements[count].planes[2 + j] = session_cpp::Plane::from_points (_pts3); }
                    elements[count].polylines[2 + j] = { pp[i][j], pp[i][j + 1], pp[i + 1][j + 1], pp[i + 1][j], pp[i][j] };
                }

            // Edge initialization, total number of edge all sides +
            // top,bottom  + undefined not lying on face for beams
            elements[count].j_mf = std::vector<std::vector<std::tuple<int, bool, double> > > ((pp[i].size () - 1) + 2 + 1); //(side id, false, parameter on edge)

            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // User given properties, Other properties such as insertion
            // vectors or wood::joint tapes be aware of "-1" in "
            // elements[count].j_mf.size() - 1" | custom wood::joint type
            // is skipped
            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            if (insertion_vectors.size () != 0)
                {
                    if (insertion_vectors.size () == pp.size () * 0.5)
                        if (insertion_vectors[count].size () == elements[count].j_mf.size () - 1)
                            {
                                elements[count].edge_vectors = insertion_vectors[count];
                                // This was very nasty bug, because
                                // polylines are reverse based on
                                // orientation fix | also only shift +2
                                // must be reversed (edges, skip top and
                                // bottom)
                                if (reverse_poylines)
                                    std::reverse (elements[count].edge_vectors.begin () + 2, elements[count].edge_vectors.end ());
                            }
                        else
                            std::cout << "ERROR - wood_main.cpp -> get_elements "
                                         "-> "
                                         "insertion_vectors[count].size() == "
                                         "elements[count].j_mf.size()-1, "
                                         "currently is: "
                                      << insertion_vectors[count].size () << " must be: " << elements[count].j_mf.size () - 1 << "\n";
                    else
                        std::cout << "ERROR - wood_main.cpp -> get_elements -> "
                                     "insertion_vectors.size() != "
                                     "pp.size() * 0.5, currently is: "
                                  << insertion_vectors.size () << " must be: " << 0.5 * pp.size () << "\n";
                }

            if (JOINTS_TYPES.size () != 0)
                {
                    if (JOINTS_TYPES.size () == pp.size () * 0.5)
                        if (JOINTS_TYPES[count].size () == elements[count].j_mf.size () - 1)
                            {
                                elements[count].JOINTS_TYPES = JOINTS_TYPES[count];
                                // This was very nasty bug, because
                                // polylines are reverse based on
                                // orientation fix | also only shift +2
                                // must be reversed (edges, skip top and
                                // bottom)
                                if (reverse_poylines)
                                    std::reverse (elements[count].JOINTS_TYPES.begin () + 2, elements[count].JOINTS_TYPES.end ());
                            }
                        else
                            std::cout << "ERROR - wood_main.cpp -> "
                                         "get_elements -> "
                                         "JOINTS_TYPES[count].size() == "
                                         "pp.size() * 0.5, currently is: "
                                      << JOINTS_TYPES[count].size () << " must be: " << 0.5 * pp.size () << "\n";
                    else
                        std::cout << "ERROR - wood_main.cpp -> get_elements -> "
                                     "JOINTS_TYPES.size() == "
                                     "pp.size() * 0.5, currently is: "
                                  << JOINTS_TYPES.size () << " must be: " << 0.5 * pp.size () << "\n";
                }
            count++;
        }
}

#pragma endregion

#pragma region SEARCH_LOCAL_METHODS

bool
border_to_face (const size_t &joint_id, std::vector<Polyline> &Polyline0, std::vector<session_cpp::Plane> &Plane0, std::vector<session_cpp::Vector> &insertion_vectors0, int f, double &thickness, Polyline &joint_area,
                std::array<Polyline, 2> &joint_lines, std::array<Polyline, 4> &joint_volumes_pairA_pairB)
{
    size_t extension_variables_count = size_t (std::floor (wood::GLOBALS::JOINT_VOLUME_EXTENSION.size () / 3.0) - 1);
    size_t extension_id = extension_variables_count == 0 ? 0 : std::min (joint_id, extension_variables_count) * 3;

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // The wifth and offset bretween two rectangles must be changed by user
    // given scale values
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    joint_area = Polyline0[f];

    if (f > 1)
        {
            /////////////////////////////////////////////////////////////////////////////////////////////////////
            // Get average line
            /////////////////////////////////////////////////////////////////////////////////////////////////////

            session_cpp::Line edge_0 = session_cpp::Line::from_points (Polyline0[0][f - 2], Polyline0[1][f - 2]);
            session_cpp::Line edge_1 = session_cpp::Line::from_points (Polyline0[0][f - 1], Polyline0[1][f - 1]);
            Polyline projection_points = { Polyline0[0][f - 2], Polyline0[1][f - 2], Polyline0[0][f - 1], Polyline0[1][f - 1] };

            session_cpp::Line average_line;
            session_cpp::line_line_average (edge_0, edge_1, average_line);
            session_cpp::line_from_projected_points (average_line, projection_points, average_line);

            joint_lines[0] = { average_line.start (), average_line.end () };
            joint_lines[1] = joint_lines[0];
            double line_length = session_cpp::polyline_length (joint_lines[0]);

            // Get average thickness
            double y_offset = 0.5; // thickness / 2.0;
            double x_offset = 0.5; // thickness / 2.0;
            double half_dist = thickness / 2.0;

            // Move points up and down using cross product
            session_cpp::Vector z_axis = Plane0[f].z_axis ();
            session_cpp::unitize (z_axis);

            // set x-axis
            auto x_axis = average_line.to_vector ();
            session_cpp::unitize (x_axis);

            // set y-axis
            auto y_axis = (z_axis).cross(x_axis);
            session_cpp::unitize (y_axis);

            session_cpp::Point p0 = average_line.start ();
            session_cpp::Point p1 = average_line.end ();
            if (session_cpp::Point::squared_distance(session_cpp::Point::mid_point(Polyline0[f][0], Polyline0[f][1]), p0) < session_cpp::Point::squared_distance(session_cpp::Point::mid_point(Polyline0[f][0], Polyline0[f][1]), p1))
                std::swap (p0, p1);

            /////////////////////////////////////////////////////////////////////////////////////////////////////
            // wood::joint valumes
            /////////////////////////////////////////////////////////////////////////////////////////////////////
            Polyline rect0 = {
                p0 - y_axis * half_dist * 1 - z_axis * half_dist * 0.25, p0 - y_axis * half_dist * 1 + z_axis * half_dist * 0.25, p1 - y_axis * half_dist * 1 + z_axis * half_dist * 0.25,
                p1 - y_axis * half_dist * 1 - z_axis * half_dist * 0.25, p0 - y_axis * half_dist * 1 - z_axis * half_dist * 0.25,
            };

            Polyline rect1 = {
                p0 - y_axis * half_dist * -1 - z_axis * half_dist * 0.25, p0 - y_axis * half_dist * -1 + z_axis * half_dist * 0.25, p1 - y_axis * half_dist * -1 + z_axis * half_dist * 0.25,
                p1 - y_axis * half_dist * -1 - z_axis * half_dist * 0.25, p0 - y_axis * half_dist * -1 - z_axis * half_dist * 0.25,
            };
            joint_volumes_pairA_pairB = { rect0, rect1, rect0, rect1 };

            return true;
        }

    return false;
}

bool
plane_to_face (const size_t &joint_id, std::vector<Polyline> &Polyline0, std::vector<Polyline> &Polyline1, std::vector<session_cpp::Plane> &Plane0, std::vector<session_cpp::Plane> &Plane1,
               std::vector<session_cpp::Vector> &insertion_vectors0, std::vector<session_cpp::Vector> &insertion_vectors1, std::pair<int, int> &el_ids, std::pair<std::array<int, 2>, std::array<int, 2> > &face_ids, int &type,
               Polyline &joint_area, std::array<Polyline, 2> &joint_lines, std::array<Polyline, 4> &joint_volumes_pairA_pairB,

               double angleTol, bool checkOverlap

)
{
    face_ids.first[0] = -1;
    face_ids.first[1] = -1;
    face_ids.second[0] = -1;
    face_ids.second[1] = -1;
    type = 30;
    size_t extension_variables_count = size_t (std::floor (wood::GLOBALS::JOINT_VOLUME_EXTENSION.size () / 3.0) - 1);
    size_t extension_id = extension_variables_count == 0 ? 0 : std::min (joint_id, extension_variables_count) * 3;

    //////////////////////////////////////////////////////////////////////////////
    // Check
    // 1. Vector angles if they are not close to parallel
    // 2. Are polylines overlapping (usually skipped within first except if
    // structure is planar) 3. Check overlay between center lines
    //////////////////////////////////////////////////////////////////////////////

    session_cpp::Vector _za0 = Plane0[0].z_axis (); session_cpp::Vector _za1 = Plane1[0].z_axis ();
    double angle = 90.0 - fabs (_za0.angle (_za1, false, true) - 90);

    if (angle < angleTol)
        return false;

    session_cpp::Line centerIntersectionLine;
    session_cpp::Line centerIntersectionLineMax;

    //////////////////////////////////////////////////////////////////////////////
    // Inputs for intersection
    //////////////////////////////////////////////////////////////////////////////
    auto cx0 = &Polyline0[0];
    auto cx1 = &Polyline0[1];
    auto cy0 = &Polyline1[0];
    auto cy1 = &Polyline1[1];

    auto px0 = &Plane0[0];
    auto px1 = &Plane0[1];
    auto py0 = &Plane1[0];
    auto py1 = &Plane1[1];

    //////////////////////////////////////////////////////////////////////////////
    // Perform intersection
    //////////////////////////////////////////////////////////////////////////////

    session_cpp::Line cx0_py0__cy0_px0;
    std::pair<int, int> edge_pair_e0_0__e1_0; //

    if (!session_cpp::polyline_plane_cross_joint (*cx0, *cy0, *px0, *py0, cx0_py0__cy0_px0, edge_pair_e0_0__e1_0))
        return false;

    session_cpp::Line cx0_py1__cy1_px0;
    std::pair<int, int> edge_pair_e0_0__e1_1;
    if (!session_cpp::polyline_plane_cross_joint (*cx0, *cy1, *px0, *py1, cx0_py1__cy1_px0, edge_pair_e0_0__e1_1))
        return false;

    session_cpp::Line cx1_py0__cy0_px1;
    std::pair<int, int> edge_pair_e0_1__e1_0;
    if (!session_cpp::polyline_plane_cross_joint (*cx1, *cy0, *px1, *py0, cx1_py0__cy0_px1, edge_pair_e0_1__e1_0))
        return false;

    session_cpp::Line cx1_py1__cy1_px1;
    std::pair<int, int> edge_pair_e0_1__e1_1;
    if (!session_cpp::polyline_plane_cross_joint (*cx1, *cy1, *px1, *py1, cx1_py1__cy1_px1, edge_pair_e0_1__e1_1))
        return false;

    face_ids.first[0] = edge_pair_e0_0__e1_0.first + 2;
    face_ids.second[0] = edge_pair_e0_0__e1_0.second + 2;
    face_ids.first[1] = edge_pair_e0_1__e1_1.first + 2;
    face_ids.second[1] = edge_pair_e0_1__e1_1.second + 2;

    //////////////////////////////////////////////////////////////////////////////
    // Sort Lines
    //////////////////////////////////////////////////////////////////////////////
    { session_cpp::Vector _ref = cx0_py0__cy0_px0.to_vector (); session_cpp::Vector _v1 = cx0_py1__cy1_px0.to_vector (); session_cpp::Vector _v1n = _v1 * -1.0;
      if (_ref.angle (_v1, false, true) > _ref.angle (_v1n, false, true))
          cx0_py1__cy1_px0 = session_cpp::Line::from_points (cx0_py1__cy1_px0.end (), cx0_py1__cy1_px0.start ()); }
    { session_cpp::Vector _ref = cx0_py0__cy0_px0.to_vector (); session_cpp::Vector _v2 = cx1_py0__cy0_px1.to_vector (); session_cpp::Vector _v2n = _v2 * -1.0;
      if (_ref.angle (_v2, false, true) > _ref.angle (_v2n, false, true))
          cx1_py0__cy0_px1 = session_cpp::Line::from_points (cx1_py0__cy0_px1.end (), cx1_py0__cy0_px1.start ()); }
    { session_cpp::Vector _ref = cx0_py0__cy0_px0.to_vector (); session_cpp::Vector _v3 = cx1_py1__cy1_px1.to_vector (); session_cpp::Vector _v3n = _v3 * -1.0;
      if (_ref.angle (_v3, false, true) > _ref.angle (_v3n, false, true))
          cx1_py1__cy1_px1 = session_cpp::Line::from_points (cx1_py1__cy1_px1.end (), cx1_py1__cy1_px1.start ()); }

    //////////////////////////////////////////////////////////////////////////////
    // Define wood::joint area from lines
    //////////////////////////////////////////////////////////////////////////////
    joint_volumes_pairA_pairB[0] = { cx0_py0__cy0_px0.start (), cx0_py1__cy1_px0.start (), cx1_py1__cy1_px1.start (), cx1_py0__cy0_px1.start (), cx0_py0__cy0_px0.start () };
    joint_volumes_pairA_pairB[1] = { cx0_py0__cy0_px0.end (), cx0_py1__cy1_px0.end (), cx1_py1__cy1_px1.end (), cx1_py0__cy0_px1.end (), cx0_py0__cy0_px0.end () };

    //////////////////////////////////////////////////////////////////////////////
    // Find shortest and longest curve based on closest point
    //-----*-----*--
    //*--------------*
    //////////////////////////////////////////////////////////////////////////////
    session_cpp::Line c;
    session_cpp::get_middle_line (cx0_py1__cy1_px0, cx1_py0__cy0_px1, c);
    session_cpp::scale_line (c, 10);

    // Find Max gap
    double cpt0[4];
    { session_cpp::Point _p = cx0_py0__cy0_px0.start (); session_cpp::closest_point_to (_p, c, cpt0[0]); }
    { session_cpp::Point _p = cx0_py1__cy1_px0.start (); session_cpp::closest_point_to (_p, c, cpt0[1]); }
    { session_cpp::Point _p = cx1_py0__cy0_px1.start (); session_cpp::closest_point_to (_p, c, cpt0[2]); }
    { session_cpp::Point _p = cx1_py1__cy1_px1.start (); session_cpp::closest_point_to (_p, c, cpt0[3]); }
    std::sort (cpt0, cpt0 + 4);

    double cpt1[4];
    { session_cpp::Point _p = cx0_py0__cy0_px0.end (); session_cpp::closest_point_to (_p, c, cpt1[0]); }
    { session_cpp::Point _p = cx0_py1__cy1_px0.end (); session_cpp::closest_point_to (_p, c, cpt1[1]); }
    { session_cpp::Point _p = cx1_py0__cy0_px1.end (); session_cpp::closest_point_to (_p, c, cpt1[2]); }
    { session_cpp::Point _p = cx1_py1__cy1_px1.end (); session_cpp::closest_point_to (_p, c, cpt1[3]); }
    std::sort (cpt1, cpt1 + 4);

    double cpt[8] = { cpt0[0], cpt0[1], cpt0[2], cpt0[3], cpt1[0], cpt1[1], cpt1[2], cpt1[3] };
    std::sort (cpt, cpt + 8);

    session_cpp::Line lMin = session_cpp::Line::from_points (session_cpp::point_at (c, cpt0[3]), session_cpp::point_at (c, cpt1[0]));
    session_cpp::Line lMax = session_cpp::Line::from_points (session_cpp::point_at (c, cpt[0]), session_cpp::point_at (c, cpt[7]));

    //////////////////////////////////////////////////////////////////////////////
    // RE-Define wood::joint area from lines
    //////////////////////////////////////////////////////////////////////////////
    // lMin mid plane
    session_cpp::Point lMinMid = session_cpp::Point::mid_point (lMin.start (), lMin.end ());
    session_cpp::Plane midPlane;
    { session_cpp::Vector _lmin_dir = lMin.to_direction (); midPlane = session_cpp::Plane::from_point_normal (lMinMid, _lmin_dir); } // Min line mid-plane

    // Intersection lMax with midPlane
    session_cpp::Point midPlane_lMax;
    session_cpp::line_plane (lMax, midPlane, midPlane_lMax);
    // session_cpp::PlaneLineIntersection(midPlane, lMax,
    // midPlane_lMax);

    // Get max distance from middle point of min line
    int maxID = session_cpp::Point::squared_distance(lMax.start (), midPlane_lMax) > session_cpp::Point::squared_distance(lMax.end (), midPlane_lMax) ? 0 : 1;
    session_cpp::Vector v = maxID == 1 ? lMax.end () - midPlane_lMax : (lMax.start () - midPlane_lMax) * -1.0;

    // exten only when user gives positive values

    if (wood::GLOBALS::JOINT_VOLUME_EXTENSION[2 + extension_id] > 0)
        {
            double length = session_cpp::length (v[0], v[1], v[2]);
            double target_length = length + wood::GLOBALS::JOINT_VOLUME_EXTENSION[2 + extension_id];
            v *= (target_length / length);
        }

    // intersection mid plane with four lines and move it in both
    // directions Polyline joint_area;
    session_cpp::plane_4lines (midPlane, cx0_py0__cy0_px0, cx0_py1__cy1_px0, cx1_py1__cy1_px1, cx1_py0__cy0_px1, joint_area);

    //////////////////////////////////////////////////////////////////////////////
    // Move rectangles in opposite direction
    //////////////////////////////////////////////////////////////////////////////

    joint_volumes_pairA_pairB[0] = { joint_area[0] + v, joint_area[1] + v, joint_area[2] + v, joint_area[3] + v, joint_area[4] + v };
    joint_volumes_pairA_pairB[1] = { joint_area[0] - v, joint_area[1] - v, joint_area[2] - v, joint_area[3] - v, joint_area[4] - v };

    // does not work
    if (wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id] + wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id] > 0)
        {
            session_cpp::extend (joint_volumes_pairA_pairB[0], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id], wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
            session_cpp::extend (joint_volumes_pairA_pairB[0], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id], wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
            session_cpp::extend (joint_volumes_pairA_pairB[0], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id], wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
            session_cpp::extend (joint_volumes_pairA_pairB[0], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id], wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);

            session_cpp::extend (joint_volumes_pairA_pairB[1], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id], wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
            session_cpp::extend (joint_volumes_pairA_pairB[1], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id], wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
            session_cpp::extend (joint_volumes_pairA_pairB[1], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id], wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
            session_cpp::extend (joint_volumes_pairA_pairB[1], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id], wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
        }

    ////////////////////////////////////////////////////////////////////////////////
    ////return plate joints
    ////////////////////////////////////////////////////////////////////////////////

    return true;
}

// ToDo: Currently one connection can be made with another object, but one
// multiple shared edges can be possible e.g. |_>-<_|
bool
face_to_face (const size_t &joint_id, std::vector<Polyline> &Polyline0, std::vector<Polyline> &Polyline1, std::vector<session_cpp::Plane> &Plane0, std::vector<session_cpp::Plane> &Plane1,
              std::vector<session_cpp::Vector> &insertion_vectors0, std::vector<session_cpp::Vector> &insertion_vectors1, std::pair<int, int> &el_ids, std::pair<std::array<int, 2>, std::array<int, 2> > &face_ids, int &type,
              Polyline &joint_area, std::array<Polyline, 2> &joint_lines, std::array<Polyline, 4> &joint_volumes_pairA_pairB)
{
    size_t extension_variables_count = size_t (std::floor (wood::GLOBALS::JOINT_VOLUME_EXTENSION.size () / 3.0) - 1);
    size_t extension_id = extension_variables_count == 0 ? 0 : std::min (joint_id, extension_variables_count) * 3;

    for (int i = 0; i < Plane0.size (); i++)
        {
            for (int j = 0; j < Plane1.size (); j++)
                {
                    // Check if polygons are co-planar using a geometric tolerance (0.1 mm).
                    // session_cpp::Plane::is_coplanar uses ZERO_TOLERANCE=1e-12 (too tight for
                    // floating-point geometry at mm scale where sub-micron noise is normal).
                    bool coplanar = false;
                    {
                        session_cpp::Vector n0 = Plane0[i].z_axis (), n1 = Plane1[j].z_axis ();
                        double dot = n0[0]*n1[0] + n0[1]*n1[1] + n0[2]*n1[2];
                        bool dirs_parallel = std::abs (std::abs (dot) - 1.0) < 1e-4;
                        if (dirs_parallel) {
                            auto p0 = Plane0[i].origin (), p1 = Plane1[j].origin ();
                            double dist = std::abs (n0[0]*(p1[0]-p0[0]) + n0[1]*(p1[1]-p0[1]) + n0[2]*(p1[2]-p0[2]));
                            coplanar = dist < 0.1; // 0.1 mm geometric tolerance
                        }
                    }

                    if (coplanar)
                        {
                            // Perform 2D Intersection 20 ms
                            bool include_triangles = i < 2 && j < 2;
                            bool hasIntersection = collider::clipper_util::get_intersection_between_two_polylines (Polyline0[i], Polyline1[j], Plane0[i], joint_area, 0,
                                                                                                                   include_triangles); // +20 ms 10000.0;
                                                                                                                                       // GlobalClipperScale

                            //////////////////////////////////////////////////////////////////////////////////////////////////
                            // Intersection lines and rectangles
                            //////////////////////////////////////////////////////////////////////////////////////////////////
                            if (hasIntersection)
                                {
                                    // std::cout << "t\n";
                                    face_ids.first[0] = i;  // Do not add +2, because
                                                            // planes are iterated
                                    face_ids.second[0] = j; // Do not add +2, because
                                                            // planes are iterated
                                    face_ids.first[1] = i;  // Do not add +2, because
                                                            // planes are iterated
                                    face_ids.second[1] = j; // Do not add +2, because
                                                            // planes are iterated

                                    int type0 = i > 1 ? 0 : 1;
                                    int type1 = j > 1 ? 0 : 1;
                                    type = type0 + type1;

                                    // Get Line: Option A(0) - Side-Side
                                    // i>2 && i > 2, Option B(1) - Side-Top
                                    // i>2, C(2) Top-Top

                                    // jointArea1 = jointArea0;//default
                                    // case (Top-top)
                                    session_cpp::Line joint_line0 = session_cpp::Line::from_points (session_cpp::Point (0, 0, 0), session_cpp::Point (0, 0, 0));
                                    session_cpp::Plane averagePlane0;
                                    { session_cpp::Point _ap0 = session_cpp::Point::mid_point(Polyline0[0][0], Polyline0[1][0]); session_cpp::Vector _az0 = Plane0[0].z_axis (); averagePlane0 = session_cpp::Plane::from_point_normal (_ap0, _az0); } // center Plane
                                    Polyline joint_quads0;
                                    if (i > 1)
                                        { // Side-Top  or Side-Side
                                          // Middle line for alignment

                                            session_cpp::Line alignmentSegment = session_cpp::Line::from_points (session_cpp::Point::mid_point(Polyline0[0][i - 2], Polyline0[1][i - 2]), session_cpp::Point::mid_point(Polyline0[0][i - 1], Polyline0[1][i - 1]));

                                            // Intersect: a) clipper
                                            // region, b) center plane

                                            bool isLine = session_cpp::polyline_plane_to_line (joint_area, averagePlane0, alignmentSegment, joint_line0);

                                            // Planes to get a quad
                                            if (isLine && joint_line0.squared_length () > wood::GLOBALS::DISTANCE_SQUARED)
                                                { //
                                                    bool isQuad = session_cpp::get_quad_from_line_topbottomplanes (Plane0[i], joint_line0, Plane0[0], Plane0[1], joint_quads0);
                                                }
                                            else
                                                {
                                                    std::cerr << "[FTF] return false: isLine0 failed (i=" << i << " j=" << j << " isLine=" << isLine << " sq=" << joint_line0.squared_length() << ")\n";
                                                    return false;
                                                }
                                        }

                                    // Intersect second time for the
                                    // side-side connection

                                    session_cpp::Line joint_line1 = session_cpp::Line::from_points (session_cpp::Point (0, 0, 0), session_cpp::Point (0, 0, 0));
                                    session_cpp::Plane averagePlane1;
                                    { session_cpp::Point _ap1 = session_cpp::Point::mid_point(Polyline1[0][0], Polyline1[1][0]); session_cpp::Vector _az1 = Plane1[0].z_axis (); averagePlane1 = session_cpp::Plane::from_point_normal (_ap1, _az1); } // center Plane
                                    Polyline joint_quads1;

                                    if (j > 1)
                                        { // Side-Side
                                          // Middle line for alignment

                                            session_cpp::Line alignmentSegment = session_cpp::Line::from_points (session_cpp::Point::mid_point(Polyline1[0][j - 2], Polyline1[1][j - 2]), session_cpp::Point::mid_point(Polyline1[0][j - 1], Polyline1[1][j - 1]));

                                            bool isLine = session_cpp::polyline_plane_to_line (joint_area, averagePlane1, alignmentSegment, joint_line1);

                                            // Planes to get a quad
                                            if (isLine && joint_line1.squared_length () > wood::GLOBALS::DISTANCE_SQUARED)
                                                { //
                                                    bool isQuad = session_cpp::get_quad_from_line_topbottomplanes (Plane1[j], joint_line1, Plane1[0], Plane1[1], joint_quads1);
                                                }
                                            else
                                                {
                                                    std::cerr << "[FTF] return false: isLine1 failed (i=" << i << " j=" << j << " isLine=" << isLine << " sq=" << joint_line1.squared_length() << ")\n";
                                                    return false;
                                                    continue;
                                                }
                                        }

                                    ////////////////////////////////////////////////////////////////////////////////
                                    // extend wood::joint line, for plates
                                    // it is negative, for beam positive
                                    // e.g.
                                    // wood::GLOBALS::JOINT_VOLUME_EXTENSION[2]
                                    // = -20; check the limit so that the
                                    // lines would not be 0 or inverse
                                    ////////////////////////////////////////////////////////////////////////////////

                                    double JOINT_LINE_EXTENSION_limit = (wood::GLOBALS::JOINT_VOLUME_EXTENSION[2 + extension_id] * 2) * (wood::GLOBALS::JOINT_VOLUME_EXTENSION[2 + extension_id] * 2);
                                    double LIMIT_MIN_JOINT_LENGTH_squared = std::pow (wood::GLOBALS::LIMIT_MIN_JOINT_LENGTH, 2);

                                    // Extension check only applies when the joint line was actually computed (i>1 / j>1 = side face)
                                    if (i > 1)
                                        {
                                            if (JOINT_LINE_EXTENSION_limit > joint_line0.squared_length () - LIMIT_MIN_JOINT_LENGTH_squared)
                                                { std::cerr << "[FTF] return false: line0 too short (ext_limit=" << JOINT_LINE_EXTENSION_limit << " sq=" << joint_line0.squared_length() << ")\n"; return false; }
                                            session_cpp::extend_equally (joint_line0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[2 + extension_id]);
                                        }

                                    if (j > 1)
                                        {
                                            if (JOINT_LINE_EXTENSION_limit > joint_line1.squared_length () - LIMIT_MIN_JOINT_LENGTH_squared)
                                                { std::cerr << "[FTF] return false: line1 too short (ext_limit=" << JOINT_LINE_EXTENSION_limit << " sq=" << joint_line1.squared_length() << ")\n"; return false; }
                                            session_cpp::extend_equally (joint_line1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[2 + extension_id]);
                                        }

                                    ////////////////////////////////////////////////////////////////////////////////
                                    // ToDo set edge direction - Check
                                    // Insertion angle if edge axis is
                                    // assigned Applies for both elements
                                    ////////////////////////////////////////////////////////////////////////////////
                                    session_cpp::Vector dir (0, 0, 0);
                                    bool dirSet = false;
                                    if (insertion_vectors0.size () > 0)
                                        {
                                            //////Take priority for male
                                            dir = i > j ? insertion_vectors0[i] : insertion_vectors1[j];
                                            dirSet = (std::abs (dir[0]) + std::abs (dir[1]) + std::abs (dir[2])) > 0.01;
                                        }

                                    //////////////////////////////////////////////////////////////////////////////////////////////////
                                    // Identify connection volumes
                                    //////////////////////////////////////////////////////////////////////////////////////////////////
                                    // CGAL_Debug(type);
                                    if (type == 0)
                                        { // side-side

                                            joint_lines[0] = { joint_line0.start (), joint_line0.end () };
                                            joint_lines[1] = { joint_line1.start (), joint_line1.end () };

                                            // return true;

                                            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                                            // Elements are rotated
                                            ///////////////////////////////////////////////////§/////////////////////////////////////////////////////////////////

                                            auto v0 = joint_line0.start () - joint_line0.end ();
                                            auto v1 = joint_line1.start () - joint_line1.end ();

                                            if ((v0.is_parallel_to (v1) == 0 || wood::GLOBALS::FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED))
                                                {
                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    // Get average
                                                    // intersection line
                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    session_cpp::Line average_segment = session_cpp::Point::squared_distance(joint_line0.start (), joint_line1.start ()) < session_cpp::Point::squared_distance(joint_line0.start (), joint_line1.end ())
                                                                                        ? session_cpp::Line::from_points (session_cpp::Point::mid_point(joint_line0.start (), joint_line1.start ()), session_cpp::Point::mid_point(joint_line0.end (), joint_line1.end ()))
                                                                                        : session_cpp::Line::from_points (session_cpp::Point::mid_point(joint_line0.start (), joint_line1.end ()), session_cpp::Point::mid_point(joint_line0.end (), joint_line1.start ()));

                                                    if (!wood::GLOBALS::FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE)
                                                        session_cpp::Line average_segment = joint_line0;
                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    // Create Plane to XY
                                                    // transformation
                                                    // matrix
                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    session_cpp::Vector o (average_segment.start ()[0], average_segment.start ()[1], average_segment.start ()[2]);
                                                    session_cpp::Vector x = average_segment.to_vector ();   // session_cpp::unitize(x);
                                                    session_cpp::Vector z = Plane0[i].z_axis (); // session_cpp::unitize(z);
                                                    session_cpp::Vector y = (x).cross(z); // session_cpp::unitize(y);
                                                    session_cpp::unitize (y);

                                                    if (!wood::GLOBALS::FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE)
                                                        {
                                                            y = Plane0[0].z_axis ();
                                                            z = (x).cross(y);
                                                        }

                                                    // Reorient axis using
                                                    // first wood::element
                                                    // orientation - Plane0
                                                    // and Plane1
                                                    session_cpp::Point center = session_cpp::center (Polyline0[i]);
                                                    double thickness = std::max (std::sqrt (session_cpp::Point::squared_distance (Plane0[0].origin (), Plane0[1].project (Plane0[0].origin ()))),
                                                                                 std::sqrt (session_cpp::Point::squared_distance (Plane1[0].origin (), Plane1[1].project (Plane1[0].origin ()))));
                                                    y *= (thickness) * 2;
                                                    session_cpp::Line y_line;
                                                    { session_cpp::Point _yl_s = center + y; session_cpp::Point _yl_e = center + (y * -1.0); y_line = session_cpp::Line::from_points (_yl_s, _yl_e); }
                                                    session_cpp::line_two_planes (y_line, Plane0[0], Plane1[1]);
                                                    y = y_line.end () - y_line.start ();
                                                    x = (y).cross(z);
                                                    // viewer_polylines.emplace_back(Polyline{y_line[0],
                                                    // y_line[1]});

                                                    session_cpp::Point  _o667 (o[0], o[1], o[2]);
                                                    session_cpp::Vector _x667 = x, _y667 = y, _z667 = z;
                                                    session_cpp::Xform xform = session_cpp::Xform::plane_to_xy (_o667, _x667, _y667, _z667);

                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    // Decide min or max
                                                    // rectangle
                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    Polyline joint_area_copy;
                                                    bool min = true;
                                                    if (min)
                                                        joint_area_copy = joint_area;
                                                    else
                                                        {
                                                            joint_area_copy.insert (joint_area_copy.end (), std::begin (Polyline0[i]), std::end (Polyline0[i]));
                                                            joint_area_copy.insert (joint_area_copy.end (), std::begin (Polyline1[j]), std::end (Polyline1[j]));
                                                        }

                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    // 2D - Get boundary
                                                    // ractangle
                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    session_cpp::transform (joint_area_copy, xform);
                                                    auto AABB = session_cpp::BoundingBox::from_points (joint_area_copy);
                                                    auto mn = AABB.min_point ();
                                                    auto mx = AABB.max_point ();
                                                    session_cpp::Line segmentX (mn[0], mn[1], mn[2], mx[0], mn[1], mn[2]);
                                                    session_cpp::Line segmentY (mn[0], mn[1], mn[2], mn[0], mx[1], mn[2]);
                                                    Polyline average_rectangle = {
                                                        segmentX.start () + segmentX.to_vector () + segmentY.to_vector (),
                                                        segmentY.end (),
                                                        segmentX.start (),
                                                        segmentX.end (),
                                                    };

                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    // 2D Orient to 3D
                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    session_cpp::Xform xformInv = xform.inverse ().value ();
                                                    session_cpp::transform (average_rectangle, xformInv);

                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    // Align the
                                                    // average_Rectangle to
                                                    // first plate
                                                    ////////////////////////////////////////////////////////////////////////////////

                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    // Create Joint
                                                    // rectangles by an
                                                    // offset of
                                                    // wood::element
                                                    // thickness
                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    session_cpp::Vector offset_vector = z;

                                                    if (dirSet)
                                                        {
                                                            offset_vector = dir;
                                                        }

                                                    session_cpp::unitize (offset_vector);
                                                    double d0 = 0.5 * std::sqrt (session_cpp::Point::squared_distance (Plane0[0].origin (), Plane0[1].project (Plane0[0].origin ())));
                                                    offset_vector *= d0;

                                                    joint_volumes_pairA_pairB[0] = { average_rectangle[3] + offset_vector, average_rectangle[3] - offset_vector, average_rectangle[0] - offset_vector,
                                                                                     average_rectangle[0] + offset_vector, average_rectangle[3] + offset_vector };
                                                    joint_volumes_pairA_pairB[1] = { average_rectangle[2] + offset_vector, average_rectangle[2] - offset_vector, average_rectangle[1] - offset_vector,
                                                                                     average_rectangle[1] + offset_vector, average_rectangle[2] + offset_vector };

                                                    // extend the polygons
                                                    // to scale the joint
                                                    // if the values are
                                                    // not set to zero
                                                    session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                    session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                    session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                    session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);

                                                    session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                    session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                    session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                    session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);

                                                    // set type for the
                                                    // wood::joint_lib
                                                    type = 13;

                                                    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                                                    // Elements are
                                                    // parallel
                                                    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                                                }
                                            else
                                                {
                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    // Get Overlap-Line //
                                                    // scale it down ?
                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    session_cpp::Line lJ;
                                                    session_cpp::line_line_overlap_average (joint_line0, joint_line1, lJ);
                                                    // CGAL_Debug(std::sqrt(joint_line0.squared_length()),
                                                    // std::sqrt(joint_line1.squared_length()),
                                                    // std::sqrt(lJ.squared_length()));
                                                    joint_lines[0] = { lJ.start (), lJ.end () };
                                                    joint_lines[1] = joint_lines[0];

                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    // Get Plane
                                                    // perpendicular to
                                                    // overlap-axis //with
                                                    // or without insertion
                                                    // vector |-----------|
                                                    ////////////////////////////////////////////////////////////////////////////////

                                                    session_cpp::Plane plEnd0;
                                                    { session_cpp::Point _pe0p = lJ.start (); session_cpp::Vector _pe0v = lJ.to_vector (); plEnd0 = session_cpp::Plane::from_point_normal (_pe0p, _pe0v); } // lJ.to_vector().cross(averagePlane0.z_axis())

                                                    if (dirSet)
                                                        { session_cpp::Point _pe0p2 = lJ.start (); plEnd0 = session_cpp::Plane::from_point_normal (_pe0p2, dir); }
                                                    // CGAL_Debug(dir);

                                                    session_cpp::Plane plEnd1;
                                                    { session_cpp::Point _pe1p = lJ.end (); session_cpp::Vector _pe1v = plEnd0.z_axis (); plEnd1 = session_cpp::Plane::from_point_normal (_pe1p, _pe1v); }
                                                    session_cpp::Plane pl_mid;
                                                    { session_cpp::Point _pmid_p = session_cpp::Point::mid_point(lJ.start (), lJ.end ()); session_cpp::Vector _pmid_v = plEnd0.z_axis (); pl_mid = session_cpp::Plane::from_point_normal (_pmid_p, _pmid_v); }

                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    // Get dihedral angle
                                                    // to a) in-plane b)
                                                    // out-of-plane c)
                                                    // not-valid sharper
                                                    // than 20 deg returns
                                                    // an approximation of
                                                    // the signed dihedral
                                                    // angle in the
                                                    // tetrahedron pqrs of
                                                    // edge pq. The sign is
                                                    // negative if
                                                    // orientation(p, q, r,
                                                    // s) is
                                                    // CGAL::NEGATIVEand
                                                    // positive
                                                    // otherwise.The angle
                                                    // is given in degrees.
                                                    ////////////////////////////////////////////////////////////////////////////////
                                                    session_cpp::Point centerPoint0 = averagePlane0.project (session_cpp::center (Polyline0[0]));
                                                    session_cpp::Point centerPoint1 = averagePlane1.project (session_cpp::center (Polyline1[0]));
                                                    // Fixed dihedral angle: project vectors perp to joint edge (matches CGAL::approximate_dihedral_angle)
                                                    double dihedralAngle;
                                                    {
                                                        session_cpp::Vector edge = lJ.to_vector();
                                                        double edge_sq_len = edge[0]*edge[0] + edge[1]*edge[1] + edge[2]*edge[2];
                                                        if (edge_sq_len < 1e-14) { std::cerr << "[FTF] return false: zero-length joint edge\n"; return false; }
                                                        auto v0_raw = session_cpp::Vector(centerPoint0[0]-lJ.start()[0], centerPoint0[1]-lJ.start()[1], centerPoint0[2]-lJ.start()[2]);
                                                        double t0 = (v0_raw[0]*edge[0]+v0_raw[1]*edge[1]+v0_raw[2]*edge[2]) / edge_sq_len;
                                                        session_cpp::Vector v0_perp(v0_raw[0]-t0*edge[0], v0_raw[1]-t0*edge[1], v0_raw[2]-t0*edge[2]);
                                                        auto v1_raw = session_cpp::Vector(centerPoint1[0]-lJ.start()[0], centerPoint1[1]-lJ.start()[1], centerPoint1[2]-lJ.start()[2]);
                                                        double t1 = (v1_raw[0]*edge[0]+v1_raw[1]*edge[1]+v1_raw[2]*edge[2]) / edge_sq_len;
                                                        session_cpp::Vector v1_perp(v1_raw[0]-t1*edge[0], v1_raw[1]-t1*edge[1], v1_raw[2]-t1*edge[2]);
                                                        dihedralAngle = std::abs(v0_perp.angle(v1_perp, false, true));
                                                    }
                                                    std::cerr << "[FTF] dihedralAngle=" << dihedralAngle << " (i=" << i << " j=" << j << ")\n";

                                                    if (dihedralAngle < 20)
                                                        { // 160
                                                            std::cerr << "[FTF] return false: dihedral < 20\n";
                                                            return false;
                                                        }
                                                    else if (dihedralAngle <= wood::GLOBALS::FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE)
                                                        { // OUT-OF-PLANE
                                                          // // &&
                                                          // jointArea0.size()>0

                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            // Rotate
                                                            // line-wood::joint
                                                            // 90 degrees
                                                            // and
                                                            // intersect
                                                            // with
                                                            // adjacent
                                                            // wood::element
                                                            // top and
                                                            // bottom
                                                            // planes This
                                                            // is needed to
                                                            // check if
                                                            // adjacent
                                                            // wood::element
                                                            // top and
                                                            // bottom
                                                            // planes are
                                                            // in the same
                                                            // or opposite
                                                            // order
                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            session_cpp::Vector connectionNormal = Plane0[i].z_axis ();
                                                            session_cpp::Vector lJ_normal = lJ.to_vector ();
                                                            session_cpp::Vector lJ_v_90 = (lJ_normal).cross(connectionNormal) * 0.5;
                                                            session_cpp::Line lj_l_90 = session_cpp::Line::from_points (lJ.start (), lJ.start () + lJ_v_90);

                                                            session_cpp::Point pl0_0_p;
                                                            session_cpp::line_plane (lj_l_90, Plane0[0], pl0_0_p);
                                                            session_cpp::Point pl1_0_p;
                                                            session_cpp::line_plane (lj_l_90, Plane1[0], pl1_0_p);
                                                            session_cpp::Point pl1_1_p;
                                                            session_cpp::line_plane (lj_l_90, Plane1[1], pl1_1_p);

                                                            session_cpp::Plane planes[4];
                                                            planes[1] = Plane0[0];

                                                            if (session_cpp::Point::squared_distance(pl0_0_p, pl1_0_p) > session_cpp::Point::squared_distance(pl0_0_p, pl1_1_p))
                                                                {
                                                                    planes[2] = Plane1[0];
                                                                    planes[3] = Plane0[1];
                                                                    planes[0] = Plane1[1];
                                                                }
                                                            else
                                                                {
                                                                    planes[2] = Plane1[1];
                                                                    planes[3] = Plane0[1];
                                                                    planes[0] = Plane1[0];
                                                                }

                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            // Intersect
                                                            // End plane
                                                            // |-----------------------|
                                                            // with top and
                                                            // bottom
                                                            // planes
                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            session_cpp::plane_4planes_open (plEnd0, planes, joint_volumes_pairA_pairB[0]);
                                                            session_cpp::plane_4planes_open (plEnd1, planes, joint_volumes_pairA_pairB[1]);

                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            // Check the
                                                            // orientation
                                                            // of the
                                                            // volume, must
                                                            // consistent
                                                            // for
                                                            // non-simetrical
                                                            // joints
                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            if (Plane0[i].z_axis ().dot (session_cpp::Vector (joint_volumes_pairA_pairB[0][1][0]-Plane0[i].origin ()[0], joint_volumes_pairA_pairB[0][1][1]-Plane0[i].origin ()[1], joint_volumes_pairA_pairB[0][1][2]-Plane0[i].origin ()[2])) >= 0)
                                                                {
                                                                    std::rotate (joint_volumes_pairA_pairB[0].begin (), joint_volumes_pairA_pairB[0].begin () + 2, joint_volumes_pairA_pairB[0].end ());
                                                                    std::rotate (joint_volumes_pairA_pairB[1].begin (), joint_volumes_pairA_pairB[1].begin () + 2, joint_volumes_pairA_pairB[1].end ());
                                                                }

                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            // Reverse if
                                                            // female and
                                                            // male order
                                                            // ERROR CHECK
                                                            // OTHER DATA
                                                            // SETS BEOFRE
                                                            // DELETING
                                                            // THIS: you
                                                            // need to find
                                                            // other
                                                            // solution
                                                            // than
                                                            // reversing
                                                            // the order,
                                                            // because use
                                                            // might need
                                                            // to preserve
                                                            // the order
                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            std::reverse (joint_volumes_pairA_pairB[0].begin (), joint_volumes_pairA_pairB[0].end ());
                                                            std::reverse (joint_volumes_pairA_pairB[1].begin (), joint_volumes_pairA_pairB[1].end ());
                                                            std::rotate (joint_volumes_pairA_pairB[0].begin (), joint_volumes_pairA_pairB[0].begin () + 3, joint_volumes_pairA_pairB[0].end ());
                                                            std::rotate (joint_volumes_pairA_pairB[1].begin (), joint_volumes_pairA_pairB[1].begin () + 3, joint_volumes_pairA_pairB[1].end ());
                                                            el_ids = std::pair<int, int> (el_ids.second, el_ids.first);
                                                            face_ids = std::pair<std::array<int, 2>, std::array<int, 2> > (face_ids.second, face_ids.first);
                                                            std::reverse (joint_lines.begin (), joint_lines.end ());

                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            // Close
                                                            // rectangle
                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            joint_volumes_pairA_pairB[0].emplace_back (joint_volumes_pairA_pairB[0][0]);
                                                            joint_volumes_pairA_pairB[1].emplace_back (joint_volumes_pairA_pairB[1][0]);
                                                            // extend the
                                                            // polygons to
                                                            // scale the
                                                            // joint if the
                                                            // values are
                                                            // not set to
                                                            // zero
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);

                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);

                                                            type = 11;
                                                        }
                                                    else
                                                        { // IN-PLANE

                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            // Intersect
                                                            // current top
                                                            // and bottom
                                                            // wood::element
                                                            // planes,
                                                            // including
                                                            // the offseted
                                                            // wood::joint
                                                            // face planes
                                                            // with
                                                            // |......................|
                                                            // end planes
                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            double d0 = 0.5 * std::sqrt (session_cpp::Point::squared_distance (Plane0[0].origin (), Plane0[1].project (Plane0[0].origin ())));

                                                            session_cpp::Plane offset_plane_0 = Plane0[i].translate_by_normal (-d0);
                                                            session_cpp::Plane offset_plane_1 = Plane0[i].translate_by_normal (d0);

                                                            // Check the winding for correct orientation of joinery tiles:
                                                            auto point0 = Plane0[0].origin ();
                                                            auto projection0 = Plane1[0].project (point0);
                                                            auto point1 = Plane0[0].origin ();
                                                            auto projection1 = Plane1[1].project (point1);

                                                            double w0 = session_cpp::Point::squared_distance(point0, projection0);
                                                            double w1 = session_cpp::Point::squared_distance(point1, projection1);
                                                           
                                                            if (w0 > w1)
                                                                std::swap (Plane1[0], Plane1[1]);


                                                            session_cpp::Plane loopOfPlanes0[4] = {
                                                                offset_plane_0,
                                                                Plane0[0],
                                                                offset_plane_1,
                                                                Plane0[1],
                                                            };
                                                            session_cpp::Plane loopOfPlanes1[4] = {
                                                                offset_plane_0,
                                                                Plane1[0],
                                                                offset_plane_1,
                                                                Plane1[1],
                                                            };

                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            // Intersect
                                                            // End plane
                                                            // |-----------------------|
                                                            // with top and
                                                            // bottom
                                                            // planes
                                                            ////////////////////////////////////////////////////////////////////////////////
                                                            session_cpp::plane_4planes (plEnd0, loopOfPlanes0, joint_volumes_pairA_pairB[0]);
                                                            session_cpp::plane_4planes (plEnd1, loopOfPlanes0, joint_volumes_pairA_pairB[1]);
                                                            session_cpp::plane_4planes (plEnd0, loopOfPlanes1, joint_volumes_pairA_pairB[2]);
                                                            session_cpp::plane_4planes (plEnd1, loopOfPlanes1, joint_volumes_pairA_pairB[3]);

                                                            // extend the
                                                            // polygons to
                                                            // scale the
                                                            // joint if the
                                                            // values are
                                                            // not set to
                                                            // zero
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);

                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);

                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[2], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[2], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[3], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[3], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);

                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[2], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[2], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[3], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[3], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);

                                                            // set type for
                                                            // the
                                                            // wood::joint_lib
                                                            type = 12;
                                                        }
                                                }
                                        }
                                    else if (type == 1)
                                        { // top-side

                                            //////////////////////////////////////////////////////////////////////////////////
                                            // Which wood::element is male
                                            // or female?
                                            //////////////////////////////////////////////////////////////////////////////////
                                            bool male_or_female = i > j;

                                            joint_lines[0] = male_or_female ? Polyline ({ joint_line0.start (), joint_line0.end () }) : Polyline ({ joint_line1.start (), joint_line1.end () });
                                            joint_lines[1] = joint_lines[0];

                                            session_cpp::Plane *plane0_0 = male_or_female ? &Plane0[0] : &Plane1[0];
                                            session_cpp::Plane *plane1_0 = !male_or_female ? &Plane0[i] : &Plane1[j]; // female
                                                                                                               // collision
                                                                                                               // plane
                                            session_cpp::Plane *plane1_1 = !male_or_female ? &Plane0[std::abs (i - 1)] : &Plane1[std::abs (j - 1)];

                                            Polyline *quad_0 = male_or_female ? &joint_quads0 : &joint_quads1; // male, female does not exist int top-side

                                            //////////////////////////////////////////////////////////////////////////////////////
                                            // Two possibilities: if dir is
                                            // unset move it
                                            // perpendicularly else move by
                                            // wood::element direction This
                                            // case will only work for
                                            // top-side connection when
                                            // elements are parallel For
                                            // other cases you need to find
                                            // a way to get opposite plane
                                            // i.e. mesh intersection
                                            //////////////////////////////////////////////////////////////////////////////////////
                                            session_cpp::Vector offset_vector;
                                            session_cpp::get_orthogonal_vector_between_two_plane_pairs (*plane0_0, *plane1_0, *plane1_1, offset_vector);

                                            // dir = i > j ?
                                            // insertion_vectors0[i]:
                                            // insertion_vectors1[j];
                                            // dirSet = true;
                                            if (dirSet)
                                                {
                                                    session_cpp::Vector offset_vector_;
                                                    // CGAL::cross_product(dir,
                                                    // plane0_0->orthogonal_vector())
                                                    bool flag = session_cpp::scale_vector_to_distance_of_2planes (dir, *plane1_0, *plane1_1, offset_vector_);
                                                    if (flag)
                                                        offset_vector = offset_vector_;
                                                }

                                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                                            // OUTPUT: IMPORTANT Switch if
                                            // first one is not side
                                            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                                            if (!male_or_female)
                                                {
                                                    // el_ids =
                                                    // std::pair(0, 0);

                                                    el_ids = std::make_pair (el_ids.second, el_ids.first);
                                                    face_ids = std::make_pair (std::array<int, 2>{ face_ids.second[0], face_ids.second[1] }, std::array<int, 2>{ face_ids.first[0], face_ids.first[1] });
                                                }

                                            //////////////////////////////////////////////////////////////////////////////////
                                            // OUTPUT: Create Connection
                                            // rectangles, IMPORTANT based
                                            // on order
                                            //////////////////////////////////////////////////////////////////////////////////
                                            int m_id = male_or_female ? 0 : 1;
                                            int f_id = male_or_female ? 1 : 0;
                                            joint_volumes_pairA_pairB[m_id] = { (*quad_0)[0], (*quad_0)[1], ((*quad_0)[1] + offset_vector), ((*quad_0)[0] + offset_vector), (*quad_0)[0] };
                                            joint_volumes_pairA_pairB[f_id] = { (*quad_0)[3], (*quad_0)[2], ((*quad_0)[2] + offset_vector), ((*quad_0)[3] + offset_vector), (*quad_0)[3] };

                                            // extend the polygons to scale
                                            // the joint if the values are
                                            // not set to zero
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[m_id], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[m_id], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[f_id], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[f_id], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);

                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[m_id], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[m_id], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[f_id], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[f_id], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);

                                            // set type for the
                                            // wood::joint_lib
                                            type = 20;

                                            return true;
                                        }
                                    else
                                        {
                                            // top-to-top:
                                            // compute bounding-rectangle
                                            // around the "joint_area" move
                                            // the bounding-rectangle up
                                            // and down by the element
                                            // thickness contruct
                                            // joint_volumes_pairA_pairB
                                            // from the rectangles and
                                            // assign them std::cout <<
                                            // "wood::main -> top-to-top
                                            // not implemented \n";

                                            // this has to become a
                                            // function
                                            Polyline result;
                                            collider::clipper_util::bounding_rectangle (joint_area, Plane0[i], result);

                                            // assign output that are in
                                            // the same position but will
                                            // be moved in the next step
                                            joint_volumes_pairA_pairB[0] = result;
                                            joint_volumes_pairA_pairB[1] = result;

                                            // get movement direction.
                                            // WARNING this will fail if
                                            // the insertion direction is
                                            // pointing to the opposite
                                            // direction from the element's
                                            // plane
                                            session_cpp::Vector dir0 = dirSet ? insertion_vectors0[i] : Plane0[i].z_axis (); // Plane0[i].orthogonal_vector();
                                            session_cpp::unitize (dir0);
                                            session_cpp::Vector dir1 = dir0 * -1.0;
                                            dir0 *= -1;
                                            dir1 *= -1;

                                            // get thickness
                                            int next_plane_0 = i == 0 ? 1 : 0;
                                            int next_plane_1 = j == 0 ? 1 : 0;
                                            double distance_0 = std::sqrt (session_cpp::Point::squared_distance(Plane0[i].origin(), Plane0[next_plane_0].project(Plane0[i].origin())));
                                            double distance_1 = std::sqrt (session_cpp::Point::squared_distance(Plane1[j].origin(), Plane1[next_plane_1].project(Plane1[j].origin())));
                                            dir0 *= distance_0;
                                            dir1 *= distance_1;

                                            // move the bounding boxes
                                            // rectangles
                                            for (int k = 0; k < 5; k++)
                                                {
                                                    joint_volumes_pairA_pairB[0][k] = joint_volumes_pairA_pairB[0][k] + dir0;
                                                    joint_volumes_pairA_pairB[1][k] = joint_volumes_pairA_pairB[1][k] + dir1;
                                                }

                                            // to follow the same notation
                                            // as other types, take two
                                            // edge and reconstruct two
                                            // rectangles
                                            Polyline joint_volumes_pairA_pairB_temp0 = {
                                                joint_volumes_pairA_pairB[0][0], joint_volumes_pairA_pairB[0][1], joint_volumes_pairA_pairB[1][1], joint_volumes_pairA_pairB[1][0], joint_volumes_pairA_pairB[0][0],
                                            };

                                            Polyline joint_volumes_pairA_pairB_temp1 = {
                                                joint_volumes_pairA_pairB[0][3], joint_volumes_pairA_pairB[0][2], joint_volumes_pairA_pairB[1][2], joint_volumes_pairA_pairB[1][3], joint_volumes_pairA_pairB[0][3],
                                            };

                                            // reassign
                                            joint_volumes_pairA_pairB[0] = joint_volumes_pairA_pairB_temp0;
                                            joint_volumes_pairA_pairB[1] = joint_volumes_pairA_pairB_temp1;

                                            // extend the polygons to scale
                                            // the joint if the values are
                                            // not set to zero
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 0, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 2, wood::GLOBALS::JOINT_VOLUME_EXTENSION[0 + extension_id]);

                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[0], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 1, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);
                                            session_cpp::extend_equally (joint_volumes_pairA_pairB[1], 3, wood::GLOBALS::JOINT_VOLUME_EXTENSION[1 + extension_id]);

                                            // set type for the
                                            // wood::joint_lib
                                            type = 40;

                                            return true;
                                        }
                                    return true;
                                } // Has Intersection
                        } // Co-planar
                } // for j
        } // for i

    std::cerr << "[FTF] return false: no coplanar+intersection found\n";
    return false;
}

#pragma endregion

#pragma region SEARCH_GLOBAL_METHODS

bool
pair_search (

    // Input
    std::vector<wood::element> &elements,     // real wood::element
    std::vector<Polyline> &beam_volumes, // wood::joint volumes
    const int &polyline_id_0, const int &polyline_id_1,

    int search_type,

    // Output
    std::vector<wood::joint> &joints, std::unordered_map<uint64_t, int> &joints_map

)
{
    //////////////////////////////////////////////////////////////////////////////
    // construct elements, with planes and side polylines
    //////////////////////////////////////////////////////////////////////////////
    std::vector<Polyline> input_polyline_pairs (std::begin (beam_volumes), std::end (beam_volumes));
    std::vector<std::vector<session_cpp::Vector> > input_insertion_vectors;
    std::vector<std::vector<int> > input_JOINTS_TYPES;
    std::vector<wood::element> beam_volumes_elements;

    get_elements (input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, beam_volumes_elements);

    //////////////////////////////////////////////////////////////////////////////
    // search
    //////////////////////////////////////////////////////////////////////////////

    Polyline joint_area;
    std::array<Polyline, 2> joint_quads;
    std::array<Polyline, 2> joint_lines;
    std::array<Polyline, 4> joint_volumes_pairA_pairB;

    std::pair<int, int> el_ids (polyline_id_0, polyline_id_1);
    std::pair<std::array<int, 2>, std::array<int, 2> > face_ids;
    int type;

    int found_type = 0;
    switch (search_type)
        {
        case (0):

            found_type = face_to_face (joints.size (), beam_volumes_elements[0].polylines, beam_volumes_elements[1].polylines, beam_volumes_elements[0].planes, beam_volumes_elements[1].planes,
                                       beam_volumes_elements[0].edge_vectors, beam_volumes_elements[1].edge_vectors, el_ids, face_ids, type, joint_area, joint_lines, joint_volumes_pairA_pairB)
                             ? 1
                             : 0;

            break;

        case (1):

            found_type = plane_to_face (joints.size (), beam_volumes_elements[0].polylines, beam_volumes_elements[1].polylines, beam_volumes_elements[0].planes, beam_volumes_elements[1].planes,
                                        beam_volumes_elements[0].edge_vectors, beam_volumes_elements[1].edge_vectors, el_ids, face_ids, type, joint_area, joint_lines, joint_volumes_pairA_pairB)
                             ? 2
                             : 0;

            break;
        }

    if (found_type == 0)
        return false;

    //////////////////////////////////////////////////////////////////////////////
    // create and joints
    //////////////////////////////////////////////////////////////////////////////
    int joint_id = (int)joints.size ();
    if (joint_area.size () > 0)
        {
            joints.emplace_back (joint_id, el_ids.first, el_ids.second, face_ids.first[0], face_ids.second[0], face_ids.first[1], face_ids.second[1], joint_area, joint_lines, joint_volumes_pairA_pairB, type);

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            // Map element0-element1 to joint_id
            //////////////////////////////////////////////////////////////////////////////////////////////////////
            joints_map.emplace (session_cpp::unique_from_two_int (el_ids.first, el_ids.second), joint_id);

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            // Place wood::joint ids and male or female flags to
            // wood::joint list
            //////////////////////////////////////////////////////////////////////////////////////////////////////

            elements[el_ids.first].j_mf.back ().push_back (std::tuple<int, bool, double> (joint_id, true, 0));
            elements[el_ids.second].j_mf.back ().push_back (std::tuple<int, bool, double> (joint_id, false, 0));
            return true;
        }

    return false;
}

void
rtree_search (

    // Input
    std::vector<wood::element> &elements, int search_type,

    // Output
    std::vector<int> &result)
{
    //////////////////////////////////////////////////////////////////////////////
    // Create RTree
    //////////////////////////////////////////////////////////////////////////////

    RTree<int, double, 3> tree;

    //////////////////////////////////////////////////////////////////////////////
    // Insert AABB
    //////////////////////////////////////////////////////////////////////////////

    for (int i = 0; i < elements.size (); i++)
        {
            double min[3] = { elements[i].aabb.min_point ()[0], elements[i].aabb.min_point ()[1], elements[i].aabb.min_point ()[2] };
            double max[3] = { elements[i].aabb.max_point ()[0], elements[i].aabb.max_point ()[1], elements[i].aabb.max_point ()[2] };
            tree.Insert (min, max, i);
        }

    //////////////////////////////////////////////////////////////////////////////
    // Search Closest Boxes | Skip duplicates pairs | Perform callback with
    // OBB
    //////////////////////////////////////////////////////////////////////////////

    for (int i = 0; i < elements.size (); i++)
        {
            auto callback = [&result, i, &elements] (int foundValue) -> bool {
                if (i < foundValue && session_cpp::obb::get_collision (elements[i].oob, elements[foundValue].oob))
                    {
                        result.emplace_back (i);
                        result.emplace_back (foundValue);
                        result.emplace_back (-1);
                        result.emplace_back (-1);
                    }
                return true;
            };

            double min[3] = { elements[i].aabb.min_point ()[0], elements[i].aabb.min_point ()[1], elements[i].aabb.min_point ()[2] };
            double max[3] = { elements[i].aabb.max_point ()[0], elements[i].aabb.max_point ()[1], elements[i].aabb.max_point ()[2] };
            int nhits = tree.Search (min, max,
                                     callback); // callback in this method call callback above
        }
}

void
adjacency_search (
    // Input
    std::vector<wood::element> &elements, int search_type, std::vector<int> &input_adjacency,
    // Output
    std::vector<wood::joint> &joints, std::unordered_map<uint64_t, int> &joints_map)
{
    //////////////////////////////////////////////////////////////////////////////
    // Split the adjacency list into two lists:
    // a) one that does not have v0=v1 and f0 = f1 (border conditions
    // usually) b) one that has valid adjacency
    //////////////////////////////////////////////////////////////////////////////
    std::vector<int> adjacency_border;
    std::vector<int> adjacency_valid;
    adjacency_border.reserve (input_adjacency.size ());
    adjacency_valid.reserve (input_adjacency.size ());

    int adjacency_item_count = 4; // v0, v1, f0, f1
    for (int i = 0; i < input_adjacency.size (); i += adjacency_item_count)
        {
            // Sanity check
            if (input_adjacency[i] > (elements.size () - 1) || input_adjacency[i + 1] > (elements.size () - 1))
                continue;

            if (input_adjacency[i + 0] == input_adjacency[i + 1])
                {
                    for (int j = 0; j < adjacency_item_count; j++)
                        {
                            adjacency_border.emplace_back (input_adjacency[i + j]);
                        }
                }
            else
                {
                    for (int j = 0; j < adjacency_item_count; j++)
                        {
                            adjacency_valid.emplace_back (input_adjacency[i + j]);
                        }
                }
        }

    //////////////////////////////////////////////////////////////////////////////
    // Perform Adjacency Search in result is empty
    //////////////////////////////////////////////////////////////////////////////
    if (adjacency_valid.size () == 0)
        rtree_search (elements, search_type, adjacency_valid);

    joints.reserve (adjacency_valid.size ());
    joints_map.reserve (adjacency_valid.size ());

    //////////////////////////////////////////////////////////////////////////////
    // Search Contact zones
    //////////////////////////////////////////////////////////////////////////////
    int joint_id = 0;
    for (int i = 0; i < adjacency_valid.size (); i += adjacency_item_count)
        { // because v0, f0 and v1,f1 are adjacent

            Polyline joint_area;
            std::array<Polyline, 2> joint_lines;
            std::array<Polyline, 4> joint_volumes_pairA_pairB;

            std::pair<int, int> el_ids (adjacency_valid[i], adjacency_valid[i + 1]);

            std::pair<std::array<int, 2>, std::array<int, 2> > face_ids;
            int type;
            // std::cout << el_ids.first << " " << el_ids.second <<
            // std::endl;
            int found_type = 0;
            switch (search_type)
                {
                case (0):
                    found_type = face_to_face (joints.size (), elements[el_ids.first].polylines, elements[el_ids.second].polylines, elements[el_ids.first].planes, elements[el_ids.second].planes,
                                               elements[el_ids.first].edge_vectors, elements[el_ids.second].edge_vectors, el_ids, face_ids, type, joint_area, joint_lines, joint_volumes_pairA_pairB

                                               )
                                     ? 1
                                     : 0;

                    break;

                case (1):

                    found_type
                        = plane_to_face (joints.size (), elements[adjacency_valid[i]].polylines, elements[adjacency_valid[i + 1]].polylines, elements[adjacency_valid[i]].planes, elements[adjacency_valid[i + 1]].planes,
                                         elements[adjacency_valid[i]].edge_vectors, elements[adjacency_valid[i + 1]].edge_vectors, el_ids, face_ids, type, joint_area, joint_lines, joint_volumes_pairA_pairB)
                              ? 2
                              : 0;

                    break;

                case (2):

                    bool found_type_temp = face_to_face (joints.size (), elements[el_ids.first].polylines, elements[el_ids.second].polylines, elements[el_ids.first].planes, elements[el_ids.second].planes,
                                                         elements[el_ids.first].edge_vectors, elements[el_ids.second].edge_vectors, el_ids, face_ids, type, joint_area, joint_lines, joint_volumes_pairA_pairB);

                    if (found_type_temp)
                        {
                            found_type = 3;
                            break;
                        }

                    found_type = plane_to_face (joints.size (), elements[el_ids.first].polylines, elements[el_ids.second].polylines, elements[el_ids.first].planes, elements[el_ids.second].planes,
                                                elements[el_ids.first].edge_vectors, elements[el_ids.second].edge_vectors, el_ids, face_ids, type, joint_area, joint_lines, joint_volumes_pairA_pairB)
                                     ? 3
                                     : 0;
                    break;
                }

            if (!found_type)
                continue;

            if (joint_area.size () > 0)
                {
                    // int joint_id = joints.size();

                    joints.emplace_back (joint_id, el_ids.first, el_ids.second, face_ids.first[0], face_ids.second[0], face_ids.first[1], face_ids.second[1], joint_area, joint_lines, joint_volumes_pairA_pairB, type);

                    joints_map.emplace (session_cpp::unique_from_two_int (el_ids.first, el_ids.second), joint_id);

                    //////////////////////////////////////////////////////////////////////////////////////////////////////
                    // Place wood::joint ids and male or female flags to
                    // wood::joint list
                    //////////////////////////////////////////////////////////////////////////////////////////////////////
                    elements[el_ids.first].j_mf[face_ids.first[0]].emplace_back (std::tuple<int, bool, double> (joint_id, true, 0));
                    elements[el_ids.second].j_mf[face_ids.second[0]].emplace_back (std::tuple<int, bool, double> (joint_id, false, 0));
                    joint_id++;
                }
        }

    //////////////////////////////////////////////////////////////////////////////
    // Define border if adjacency was given
    //////////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < adjacency_border.size (); i += adjacency_item_count)
        {
            Polyline joint_area;
            std::array<Polyline, 2> joint_lines;
            std::array<Polyline, 4> joint_volumes_pairA_pairB;

            border_to_face (joints.size (), elements[adjacency_border[i]].polylines, elements[adjacency_border[i]].planes, elements[adjacency_border[i]].edge_vectors, adjacency_border[i + 2],
                            elements[adjacency_border[i]].thickness, joint_area, joint_lines, joint_volumes_pairA_pairB

            );

            joints.emplace_back (joint_id, adjacency_border[i], adjacency_border[i], adjacency_border[i + 2], adjacency_border[i + 2], adjacency_border[i + 2], adjacency_border[i + 2], joint_area, joint_lines,
                                 joint_volumes_pairA_pairB, 60);

            joints_map.emplace (session_cpp::unique_from_two_int (adjacency_border[i], adjacency_border[i]), joint_id);
            //////////////////////////////////////////////////////////////////////////////////////////////////////
            // Place wood::joint ids and male or female flags to
            // wood::joint list
            //////////////////////////////////////////////////////////////////////////////////////////////////////
            elements[adjacency_border[i]].j_mf[adjacency_border[i + 2]].emplace_back (std::tuple<int, bool, double> (joint_id, true, 0));

            joint_id++;
        }
}

#pragma endregion

#pragma region CUSTOM IMPLEMENTATIONS : 3 - VALENCE VIDY
// 0. create a parametric joints whose number of tenons and chamfers could
// be controlled, this wood::joint must be adapted to the length
// 1. compute two additional connection volumes, meaning creating two
// additional wood::joint with index(id0, id_tenon) and (id1, id_tenon)
// 2. assign wood::joint type
// 2.1 orient it to the connection volumes(this is probably done in
// wood::joint computatio step) 2.2. remove male geometry and merge it with
// the id_tenon, this has to be done in the preprocessing step before merge
// joints in the wood_main.cpp in the switch statements
void
three_valence_joint_addition_vidy (std::vector<std::vector<int> > &in_s0_s1_e20_e31, // 0 - side wood::element, 1 - side
                                                                                     // wood::element, 2 - connecting wood::element
                                                                                     // for 0, 3 - connecting wood::element for 1
                                   std::vector<wood::element> &elements, std::vector<wood::joint> &joints, std::unordered_map<uint64_t, int> &joints_map, bool align_joints)
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // check if the user gave the input
    // check if the 3 valence number is within the range
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (in_s0_s1_e20_e31.size () < 2)
        return;

    for (int i = 1; i < in_s0_s1_e20_e31.size (); i++)
        {
            auto size = elements.size () - 1;

            if (in_s0_s1_e20_e31[i].size () != 4)
                {
                    std::cout << "wood::main -> "
                                 "three_valence_joint_addition_vidy -> "
                                 "four elements must be "
                                 "present \n";
                    std::cout << in_s0_s1_e20_e31[i].size () << "\n";
                    return;
                }

            if (in_s0_s1_e20_e31[i][0] < 0 || in_s0_s1_e20_e31[i][1] < 0 || in_s0_s1_e20_e31[i][2] < 0 || in_s0_s1_e20_e31[i][3] < 0)
                {
                    std::cout << in_s0_s1_e20_e31[i][0] << " " << in_s0_s1_e20_e31[i][1] << " " << in_s0_s1_e20_e31[i][2] << " " << in_s0_s1_e20_e31[i][3] << "\n";
                    std::cout << "wood::main -> "
                                 "three_valence_joint_addition_vidy -> id "
                                 "is negative \n";
                    return;
                }

            if (in_s0_s1_e20_e31[i][0] > size || in_s0_s1_e20_e31[i][1] > size || in_s0_s1_e20_e31[i][2] > size || in_s0_s1_e20_e31[i][3] > size)
                {
                    std::cout << in_s0_s1_e20_e31[i][0] << " " << in_s0_s1_e20_e31[i][1] << " " << in_s0_s1_e20_e31[i][2] << " " << in_s0_s1_e20_e31[i][3] << " " << size << "\n";
                    std::cout << "wood::main -> "
                                 "three_valence_joint_addition_vidy -> id "
                                 "is bigger than the "
                                 "total number of elements \n";
                    return;
                }
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 1. compute two additional connection volumes, meaning creating two
    // additional wood::joint with index(id0, id_tenon) and (id1, id_tenon)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    for (int i = 1; i < in_s0_s1_e20_e31.size (); i++) // skip first item because it is indicator which custom
                                                       // method to run
        {
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // check if the two plates in the double layer system are
            // parallel
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            if (in_s0_s1_e20_e31[i][2] != in_s0_s1_e20_e31[i][3])
                {
                    if (!session_cpp::Plane::is_same_direction (elements[in_s0_s1_e20_e31[i][0]].planes[0], elements[in_s0_s1_e20_e31[i][3]].planes[0])
                        || !session_cpp::Plane::is_same_direction (elements[in_s0_s1_e20_e31[i][1]].planes[0], elements[in_s0_s1_e20_e31[i][2]].planes[0]))
                        {
                            std::cout << "wood_main.cpp -> "
                                         "three_valence_joint_addition_"
                                         "vidy -> planes are not "
                                         "parallel \n";
                            std::cout << "wood_main.cpp -> "
                                         "three_valence_joint_addition_vidy ->"
                                      << in_s0_s1_e20_e31[i][0] << " " << in_s0_s1_e20_e31[i][1] << " " << in_s0_s1_e20_e31[i][2] << " " << in_s0_s1_e20_e31[i][3] << "\n";
                            continue;
                        }
                }

            //////////////////////////////////////////////////////////////////////////////////////////////////
            // get unique key, if key does not exist throw out of range
            // exception
            //////////////////////////////////////////////////////////////////////////////////////////////////
            int id = -1;
            // int id_alignment_joint = -1;
            try
                {
                    id = joints_map.at (session_cpp::unique_from_two_int (in_s0_s1_e20_e31[i][0], in_s0_s1_e20_e31[i][1]));

                    // id_alignment_joint =
                    // joints_map.at(session_cpp::unique_from_two_int(in_s0_s1_e20_e31[i][2],
                    // in_s0_s1_e20_e31[i][3])); std::cout << id << "
                    // three_valence_joint_addition_vidy \n";
                }
            catch (const std::out_of_range &oor)
                {
                    printf ("\nCPP   FILE %s    METHOD %s   LINE %i     "
                            "WHAT %s  %s ",
                            __FILE__, __FUNCTION__, __LINE__, "wood::main -> vidy -> out-of-range error:", oor.what ());
                    continue;
                }

            //////////////////////////////////////////////////////////////////////////////////////////////////
            // if plates are parallel, then it would be enough to move the
            // wood::joint volume, without performing intersection
            //////////////////////////////////////////////////////////////////////////////////////////////////

            // find two nearst planes
            double d00 = session_cpp::Point::squared_distance(elements[in_s0_s1_e20_e31[i][0]].planes[0].origin (), elements[in_s0_s1_e20_e31[i][3]].planes[0].origin ());
            double d01 = session_cpp::Point::squared_distance(elements[in_s0_s1_e20_e31[i][0]].planes[0].origin (), elements[in_s0_s1_e20_e31[i][3]].planes[1].origin ());

            session_cpp::Plane plane00_far = d00 < d01 ? elements[in_s0_s1_e20_e31[i][3]].planes[0] : elements[in_s0_s1_e20_e31[i][3]].planes[1];
            // std::cout << "dist: " << std::sqrt(d00) << " " <<
            // std::sqrt(d01) << "\n";

            d00 = session_cpp::Point::squared_distance(plane00_far.origin (), elements[in_s0_s1_e20_e31[i][0]].planes[0].origin ());
            d01 = session_cpp::Point::squared_distance(plane00_far.origin (), elements[in_s0_s1_e20_e31[i][0]].planes[1].origin ());

            session_cpp::Plane plane01_near = d00 < d01 ? elements[in_s0_s1_e20_e31[i][0]].planes[1] : elements[in_s0_s1_e20_e31[i][0]].planes[0];
            // std::cout << "dist: " << std::sqrt(d00) << " " <<
            // std::sqrt(d01) << "\n";

            double d10 = session_cpp::Point::squared_distance(elements[in_s0_s1_e20_e31[i][1]].planes[0].origin (), elements[in_s0_s1_e20_e31[i][2]].planes[0].origin ());
            double d11 = session_cpp::Point::squared_distance(elements[in_s0_s1_e20_e31[i][1]].planes[0].origin (), elements[in_s0_s1_e20_e31[i][2]].planes[1].origin ());

            session_cpp::Plane plane10_far = d10 < d11 ? elements[in_s0_s1_e20_e31[i][2]].planes[0] : elements[in_s0_s1_e20_e31[i][2]].planes[1];
            // std::cout << "dist: " << std::sqrt(d10) << " " <<
            // std::sqrt(d11) << "\n";

            d10 = session_cpp::Point::squared_distance(plane10_far.origin (), elements[in_s0_s1_e20_e31[i][1]].planes[0].origin ());
            d11 = session_cpp::Point::squared_distance(plane10_far.origin (), elements[in_s0_s1_e20_e31[i][1]].planes[1].origin ());

            session_cpp::Plane plane11_near = d10 < d11 ? elements[in_s0_s1_e20_e31[i][1]].planes[1] : elements[in_s0_s1_e20_e31[i][1]].planes[0];
            // std::cout << "dist: " << std::sqrt(d10) << " " <<
            // std::sqrt(d11) << "\n";

            // find movement direction

            session_cpp::Line l0 = session_cpp::Line::from_points (joints[id].joint_volumes[0][0], joints[id].joint_volumes[0][1]);
            session_cpp::Line l1 = session_cpp::Line::from_points (joints[id].joint_volumes[0][1], joints[id].joint_volumes[0][2]);
            // std::cout << "dist of lines: " <<
            // std::sqrt(session_cpp::Point::squared_distance(joints[id].joint_volumes[0][0], // joints[id].joint_volumes[0][1])) << "\n"; std::cout << "dist
            // of lines: " <<
            // std::sqrt(session_cpp::Point::squared_distance(joints[id].joint_volumes[0][1], // joints[id].joint_volumes[0][2])) << "\n";

            // double d_plane0_near =
            // std::abs(session_cpp::Point::squared_distance(joints[id].joint_volumes[0][0], // plane01_near) -
            // session_cpp::Point::squared_distance(joints[id].joint_volumes[0][1], // plane01_near)); double d_plane1_near =
            // std::abs(session_cpp::Point::squared_distance(joints[id].joint_volumes[0][1], // plane11_near) -
            // session_cpp::Point::squared_distance(joints[id].joint_volumes[0][2], // plane11_near));
            session_cpp::Line projection_line_0 = session_cpp::Line::from_points (plane01_near.project (joints[id].joint_volumes[0][0]), plane01_near.project (joints[id].joint_volumes[0][1]));
            session_cpp::Line projection_line_1 = session_cpp::Line::from_points (plane01_near.project (joints[id].joint_volumes[0][1]), plane01_near.project (joints[id].joint_volumes[0][2]));

            session_cpp::Vector _pl0v = projection_line_0.to_vector (), _l0v = l0.to_vector ();
            bool is_parallel_00 = _pl0v.is_parallel_to (_l0v) == 0;
            session_cpp::Vector _pl1v = projection_line_1.to_vector (), _l1v = l1.to_vector ();
            bool is_parallel_01 = _pl1v.is_parallel_to (_l1v) == 0;

            // std::cout << "is parallel: " << is_parallel_00 << " " <<
            // is_parallel_01 << "\n";

            session_cpp::Point midpoint_0 = session_cpp::Point::mid_point(joints[id].joint_volumes[0][0], joints[id].joint_volumes[0][1]);
            session_cpp::Point midpoint_1 = session_cpp::Point::mid_point(joints[id].joint_volumes[0][1], joints[id].joint_volumes[0][2]);
            session_cpp::Point midpoint_2 = session_cpp::Point::mid_point(joints[id].joint_volumes[0][2], joints[id].joint_volumes[0][3]);
            session_cpp::Point midpoint_3 = session_cpp::Point::mid_point(joints[id].joint_volumes[0][3], joints[id].joint_volumes[0][0]);

            // double d_plane0_near =
            // std::abs(session_cpp::Point::squared_distance(joints[id].joint_volumes[0][0], // plane01_near) -
            // session_cpp::Point::squared_distance(joints[id].joint_volumes[0][1], // plane01_near)); double d_plane1_near =
            // std::abs(session_cpp::Point::squared_distance(joints[id].joint_volumes[0][1], // plane11_near) -
            // session_cpp::Point::squared_distance(joints[id].joint_volumes[0][2], // plane11_near));

            // std::array<session_cpp::Line, 2> l = d_plane0_near < d_plane1_near
            // ? std::array<session_cpp::Line, 2>{l1, l0} : std::array<session_cpp::Line,
            // 2>{l0, l1};
            std::array<session_cpp::Line, 2> l = is_parallel_01 ? std::array<session_cpp::Line, 2>{ l1, l0 } : std::array<session_cpp::Line, 2>{ l0, l1 };

            session_cpp::Point p00;
            session_cpp::line_plane (l[0], plane00_far, p00);

            session_cpp::Point p01;
            session_cpp::line_plane (l[0], plane01_near, p01);

            session_cpp::Point p10;
            session_cpp::line_plane (l[1], plane10_far, p10);

            session_cpp::Point p11;
            session_cpp::line_plane (l[1], plane11_near, p11);

            // when only one neighbor is given
            if (in_s0_s1_e20_e31[i][2] == in_s0_s1_e20_e31[i][3])
                {
                    p10 = p00;
                    p11 = p01;
                }

            auto _tv0 = p00 - p01; auto _tv1 = p10 - p11;
            std::array<session_cpp::Xform, 2> translation
                = std::array<session_cpp::Xform, 2>{ session_cpp::Xform::translation (_tv0[0], _tv0[1], _tv0[2]), session_cpp::Xform::translation (_tv1[0], _tv1[1], _tv1[2]) };

            std::array<session_cpp::Vector, 2> translation_vectors = std::array<session_cpp::Vector, 2>{ p00 - p01, p10 - p11 };

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            // Translates the wood::joint volume and lines | change the
            // orientation of the second volume to rotate the same
            // wood::joint
            //////////////////////////////////////////////////////////////////////////////////////////////////////
            std::array<Polyline, 4> joint_volumes_0 = { joints[id].joint_volumes[0], joints[id].joint_volumes[1], joints[id].joint_volumes[2], joints[id].joint_volumes[3] };
            std::array<Polyline, 4> joint_volumes_1 = { joints[id].joint_volumes[0], joints[id].joint_volumes[1], joints[id].joint_volumes[2], joints[id].joint_volumes[3] };

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            // Check if translation vectors are valid
            //////////////////////////////////////////////////////////////////////////////////////////////////////
            double vector_coord_sum
                = translation_vectors[0][0] + translation_vectors[0][1] + translation_vectors[0][2] + translation_vectors[1][0] + translation_vectors[1][1] + translation_vectors[1][2];
            bool a = vector_coord_sum < -1.0e8;
            bool b = vector_coord_sum > 1.0e8;

            if (vector_coord_sum < -1.0e8 || vector_coord_sum > 1.0e8)
                {
                    std::cout << "ERROR -> wood_main.cpp -> "
                                 "three_valence_joint_addition_vidy "
                              << vector_coord_sum << " translation vector is huge or super small \n";
                    return;
                }

            // orient volumes to translation direction for correct
            // wood::joint orientation
            int shift = 0;
            for (int j = 0; j < 4; j++)
                {
                    session_cpp::Vector v = joint_volumes_1[0][j] - joint_volumes_1[0][j + 1];
                    session_cpp::Vector _tv1 = translation_vectors[1]; if (_tv1.is_parallel_to (v) == 1)
                        {
                            shift = j;
                            break;
                        }
                }

            // std::cout << "shift: " << shift << std::endl;
            for (int j = 0; j < joint_volumes_1.size (); j++)
                if (joint_volumes_1[j].size () == 5)
                    session_cpp::shift (joint_volumes_1[j], shift);

            std::array<Polyline, 2> joint_lines_0 = { joints[id].joint_lines[0], joints[id].joint_lines[1] };
            std::array<Polyline, 2> joint_lines_1 = { joints[id].joint_lines[0], joints[id].joint_lines[1] };
            for (int j = 0; j < 2; j++)
                {
                    session_cpp::transform (joint_volumes_0[j], translation[0]);
                    session_cpp::transform (joint_volumes_1[j], translation[1]);
                    session_cpp::transform (joint_lines_0[j], translation[0]);
                    session_cpp::transform (joint_lines_1[j], translation[1]);
                }

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // if wood::joint order was reversed, reverse the neighbors
            // here:
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            if (joints[id].v0 == in_s0_s1_e20_e31[i][1])
                {
                    std::swap (in_s0_s1_e20_e31[i][2], in_s0_s1_e20_e31[i][3]);
                    std::swap (in_s0_s1_e20_e31[i][0], in_s0_s1_e20_e31[i][1]);
                }

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            //  2. Add joints | Map element0-element1 to joint_id | Add
            //  wood::element indexing for display of volumes
            //////////////////////////////////////////////////////////////////////////////////////////////////////

            joints.emplace_back ((int)joints.size (), in_s0_s1_e20_e31[i][0], in_s0_s1_e20_e31[i][2], -1, -1, -1, -1, joints[id].joint_area, joint_lines_0, joint_volumes_0, joints[id].type);

            joints_map.emplace (session_cpp::unique_from_two_int (in_s0_s1_e20_e31[i][0], in_s0_s1_e20_e31[i][2]), joints[joints.size () - 1].id);
            elements[in_s0_s1_e20_e31[i][0]].j_mf.back ().push_back (std::tuple<int, bool, double> (joints[joints.size () - 1].id, true, 0));
            elements[in_s0_s1_e20_e31[i][2]].j_mf.back ().push_back (std::tuple<int, bool, double> (joints[joints.size () - 1].id, false, 0));
            joints[joints.size () - 1].link = true;

            if (in_s0_s1_e20_e31[i][2] != in_s0_s1_e20_e31[i][3])
                {
                    joints.emplace_back ((int)joints.size (), in_s0_s1_e20_e31[i][1], in_s0_s1_e20_e31[i][3], -1, -1, -1, -1, joints[id].joint_area, joint_lines_1, joint_volumes_1, joints[id].type);

                    joints_map.emplace (session_cpp::unique_from_two_int (in_s0_s1_e20_e31[i][1], in_s0_s1_e20_e31[i][3]), joints[joints.size () - 1].id);
                    elements[in_s0_s1_e20_e31[i][1]].j_mf.back ().push_back (std::tuple<int, bool, double> (joints[joints.size () - 1].id, true, 0));
                    elements[in_s0_s1_e20_e31[i][3]].j_mf.back ().push_back (std::tuple<int, bool, double> (joints[joints.size () - 1].id, false, 0));
                    joints[joints.size () - 1].link = true;
                }

            joints[id].linked_joints = (in_s0_s1_e20_e31[i][2] != in_s0_s1_e20_e31[i][3]) ? std::vector<int>{ (int)joints.size () - 2, (int)joints.size () - 1 } : std::vector<int>{ (int)joints.size () - 1 };

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // perepare for linking
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        }
}
#pragma endregion

#pragma region CUSTOM IMPLEMENTATIONS : 3 - VALENCE ANNEN
// the function aligns to joints so that tenons would no collide and be
// distributed in an equal manner
void
three_valence_joint_alignment_annen (std::vector<std::vector<int> > &in_s0_s1_e20_e31, // 0 - side wood::element, 1 - top wood::element,
                                                                                       // 2 - side wood::element, 3 - top wood::element
                                     std::vector<wood::element> &elements, std::vector<wood::joint> &joints, std::unordered_map<uint64_t, int> &joints_map

)
{
    if (in_s0_s1_e20_e31.size () < 2)
        return;

    for (int i = 1; i < in_s0_s1_e20_e31.size (); i++)
        {
            auto size = elements.size () - 1;

            if (in_s0_s1_e20_e31[i].size () != 4)
                {
                    std::cout << "wood::main -> "
                                 "three_valence_joint_addition_vidy -> "
                                 "four elements must be "
                                 "present \n";
                    std::cout << in_s0_s1_e20_e31[i].size () << "\n";
                    return;
                }

            if (in_s0_s1_e20_e31[i][0] < 0 || in_s0_s1_e20_e31[i][1] < 0 || in_s0_s1_e20_e31[i][2] < 0 || in_s0_s1_e20_e31[i][3] < 0)
                {
                    std::cout << in_s0_s1_e20_e31[i][0] << " " << in_s0_s1_e20_e31[i][1] << " " << in_s0_s1_e20_e31[i][2] << " " << in_s0_s1_e20_e31[i][3] << "\n";
                    std::cout << "wood::main -> "
                                 "three_valence_joint_addition_vidy -> id "
                                 "is negative \n";
                    return;
                }

            if (in_s0_s1_e20_e31[i][0] > size || in_s0_s1_e20_e31[i][1] > size || in_s0_s1_e20_e31[i][2] > size || in_s0_s1_e20_e31[i][3] > size)
                {
                    std::cout << in_s0_s1_e20_e31[i][0] << " " << in_s0_s1_e20_e31[i][1] << " " << in_s0_s1_e20_e31[i][2] << " " << in_s0_s1_e20_e31[i][3] << " " << size << "\n";
                    std::cout << "wood::main -> "
                                 "three_valence_joint_addition_vidy -> id "
                                 "is bigger than the "
                                 "total number of elements \n";
                    return;
                }
        }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // For solving multiple valences (Specific case Annen), only works when
    // only one wood::joint is possible between two unique plates (wont
    // work for plates with subdivided edges)
    //////////////////////////////////////////////////////////////////////////////////////////////////
    if (in_s0_s1_e20_e31.size () < 2)
        return;

    for (int i = 1; i < in_s0_s1_e20_e31.size (); i++)
        {
            //////////////////////////////////////////////////////////////////////////////////////////////////
            // get unique key, if key does not exist throw out of range
            // exception
            //////////////////////////////////////////////////////////////////////////////////////////////////
            int id_0, id_1;
            try
                {
                    id_0 = joints_map.at (session_cpp::unique_from_two_int (in_s0_s1_e20_e31[i][0], in_s0_s1_e20_e31[i][1]));
                    id_1 = joints_map.at (session_cpp::unique_from_two_int (in_s0_s1_e20_e31[i][2], in_s0_s1_e20_e31[i][3]));
                }
            catch (const std::out_of_range &oor)
                {
                    std::cout << "\n" << in_s0_s1_e20_e31[i][0] << " " << in_s0_s1_e20_e31[i][1];
                    std::cout << "\n" << in_s0_s1_e20_e31[i][2] << " " << in_s0_s1_e20_e31[i][3];
                    printf ("\nCPP   FILE %s    METHOD %s   LINE %i     "
                            "WHAT %s  %s ",
                            __FILE__, __FUNCTION__, __LINE__, "Out of Range error:", oor.what ());
                    continue;
                }

            //////////////////////////////////////////////////////////////////////////////////////////////////
            // Get overlap segment and plane within its normal
            //////////////////////////////////////////////////////////////////////////////////////////////////
            session_cpp::Line l0 = session_cpp::Line::from_points (joints[id_0].joint_lines[0][0], joints[id_0].joint_lines[0][1]);

            session_cpp::Line l1 = session_cpp::Point::squared_distance(joints[id_0].joint_lines[0][0], joints[id_1].joint_lines[0][0]) < session_cpp::Point::squared_distance(joints[id_0].joint_lines[0][0], joints[id_1].joint_lines[0][1])
                                   ? session_cpp::Line::from_points (joints[id_1].joint_lines[0][0], joints[id_1].joint_lines[0][1])
                                   : session_cpp::Line::from_points (joints[id_1].joint_lines[0][1], joints[id_1].joint_lines[0][0]);

            session_cpp::Line l;
            session_cpp::line_line_overlap_average (l0, l1, l);
            double thickness = elements[joints[id_0].v0].thickness;
            session_cpp::extend_line (l, -thickness, -thickness);

            //////////////////////////////////////////////////////////////////////////////////////////////////
            // Both sides of joints must have the same alignment else there
            // will be a collision
            //////////////////////////////////////////////////////////////////////////////////////////////////
            if (joints[id_0].joint_lines->size () > 0)
                {
                    joints[id_0].joint_lines[0] = Polyline{ l.start (), l.end () };
                    joints[id_1].joint_lines[0] = Polyline{ l.start (), l.end () };
                }

            //////////////////////////////////////////////////////////////////////////////////////////////////
            // Construct plane from exisiting wood::joint volume edges
            //////////////////////////////////////////////////////////////////////////////////////////////////

            session_cpp::Vector cross0 = (joints[id_0].joint_volumes[0][2] - joints[id_0].joint_volumes[0][1]).cross (joints[id_0].joint_volumes[0][0] - joints[id_0].joint_volumes[0][1]);
            session_cpp::Vector cross1 = (joints[id_1].joint_volumes[0][2] - joints[id_1].joint_volumes[0][1]).cross (joints[id_1].joint_volumes[0][0] - joints[id_1].joint_volumes[0][1]);

            //////////////////////////////////////////////////////////////////////////////////////////////////
            // Intersect lines with planes
            //////////////////////////////////////////////////////////////////////////////////////////////////

            session_cpp::Point _ls = l.start (), _le = l.end ();
            session_cpp::Plane plane0_0 = session_cpp::Plane::from_point_normal (_ls, cross0);
            session_cpp::Plane plane0_1 = session_cpp::Plane::from_point_normal (_le, cross0);
            session_cpp::Plane plane1_0 = session_cpp::Plane::from_point_normal (_le, cross1);
            session_cpp::Plane plane1_1 = session_cpp::Plane::from_point_normal (_ls, cross1);

            for (int j = 0; j < 4; j += 2)
                {
                    if (joints[id_0].joint_volumes[j].size () == 0)
                        continue;

                    session_cpp::Line s0 = session_cpp::Line::from_points (joints[id_0].joint_volumes[j + 0][0], joints[id_0].joint_volumes[j + 1][0]);
                    session_cpp::Line s1 = session_cpp::Line::from_points (joints[id_0].joint_volumes[j + 0][1], joints[id_0].joint_volumes[j + 1][1]);
                    session_cpp::Line s2 = session_cpp::Line::from_points (joints[id_0].joint_volumes[j + 0][2], joints[id_0].joint_volumes[j + 1][2]);
                    session_cpp::Line s3 = session_cpp::Line::from_points (joints[id_0].joint_volumes[j + 0][3], joints[id_0].joint_volumes[j + 1][3]);

                    session_cpp::plane_4lines (plane0_0, s0, s1, s2, s3, joints[id_0].joint_volumes[j]);
                    session_cpp::plane_4lines (plane0_1, s0, s1, s2, s3, joints[id_0].joint_volumes[j + 1]);
                }

            for (int j = 0; j < 4; j += 2)
                {
                    if (joints[id_1].joint_volumes[j].size () == 0)
                        continue;

                    session_cpp::Line s0 = session_cpp::Line::from_points (joints[id_1].joint_volumes[j + 0][0], joints[id_1].joint_volumes[j + 1][0]);
                    session_cpp::Line s1 = session_cpp::Line::from_points (joints[id_1].joint_volumes[j + 0][1], joints[id_1].joint_volumes[j + 1][1]);
                    session_cpp::Line s2 = session_cpp::Line::from_points (joints[id_1].joint_volumes[j + 0][2], joints[id_1].joint_volumes[j + 1][2]);
                    session_cpp::Line s3 = session_cpp::Line::from_points (joints[id_1].joint_volumes[j + 0][3], joints[id_1].joint_volumes[j + 1][3]);

                    session_cpp::plane_4lines (plane1_0, s0, s1, s2, s3, joints[id_1].joint_volumes[j]);
                    session_cpp::plane_4lines (plane1_1, s0, s1, s2, s3, joints[id_1].joint_volumes[j + 1]);
                }
        }
}

#pragma endregion

#pragma region MAIN METHODS

void
get_connection_zones (std::vector<Polyline> &input_polyline_pairs, std::vector<std::vector<session_cpp::Vector> > &input_insertion_vectors, std::vector<std::vector<int> > &input_JOINTS_TYPES,
                      std::vector<std::vector<int> > &input_three_valence_element_indices_and_instruction, std::vector<int> &input_adjacency,

                      // output
                      std::vector<std::vector<Polyline> > &output_plines, std::vector<std::vector<wood::cut::cut_type> > &output_types, std::vector<std::vector<int> > &top_face_triangulation,

                      // Global Parameters
                      std::vector<double> &default_parameters_for_JOINTS_TYPES, std::vector<double> &scale, int search_type, int output_type, int triangulate

)
{
    //////////////////////////////////////////////////////////////////////////////
    // Main Properties: elements, joints, joints_map
    //////////////////////////////////////////////////////////////////////////////
    std::vector<wood::element> elements;
    auto joints = std::vector<wood::joint> ();
    auto joints_map = std::unordered_map<uint64_t, int> ();

    //////////////////////////////////////////////////////////////////////////////
    // Create elements, AABB, OBB, P, Pls, thickness
    //////////////////////////////////////////////////////////////////////////////
    try
        {
            get_elements (input_polyline_pairs, input_insertion_vectors, input_JOINTS_TYPES, elements);
        }
    catch (std::exception &e)
        {
            printf ("\nCPP   FILE %s    METHOD %s   LINE %i     WHAT %s ", __FILE__, __FUNCTION__, __LINE__, "Error in get_elements");
            return;
        }
    catch (...)
        {
            printf ("\nCPP   FILE %s    METHOD %s   LINE %i     WHAT %s ", __FILE__, __FUNCTION__, __LINE__, "Error in get_elements");
            return;
        }

    //////////////////////////////////////////////////////////////////////////////
    // Create joints, Perform Joint Area Search
    //////////////////////////////////////////////////////////////////////////////
    // std::vector<int> neighbours;
    try
        {
            adjacency_search (elements, search_type, input_adjacency, joints, joints_map);
        }
    catch (std::exception &e)
        {
            printf ("\nCPP   FILE %s    METHOD %s   LINE %i     WHAT %s ", __FILE__, __FUNCTION__, __LINE__, "Error in adjacency_search");
            return;
        }
    catch (...)
        {
            printf ("\nCPP   FILE %s    METHOD %s   LINE %i     WHAT %s ", __FILE__, __FUNCTION__, __LINE__, "Error in adjacency_search");
            return;
        }

    //////////////////////////////////////////////////////////////////////////////
    // custom cases, where the two valence rules breaks
    // the first vector in the in the parameter
    // "input_three_valence_element_indices_and_instruction" must be a
    // single number e.g. {0}, that points to the right method needed to be
    // applied {0} - Annen - 3-valence joints cases {1} - Vidy - extension
    // of the tenon mortise wood::joint to another panel
    //////////////////////////////////////////////////////////////////////////////
    if (input_three_valence_element_indices_and_instruction.size () > 0)
        {
            // sanity check give default parameter of the annen project
            if (input_three_valence_element_indices_and_instruction[0].size () != 1)
                input_three_valence_element_indices_and_instruction.insert (input_three_valence_element_indices_and_instruction.begin (), { 0 });

            // run the correct method
            switch (input_three_valence_element_indices_and_instruction[0][0])
                {
                case (0):
                    // Annen
                    three_valence_joint_alignment_annen (input_three_valence_element_indices_and_instruction, elements, joints, joints_map);
                    break;
                case (1):
                    // Vidy
                    three_valence_joint_addition_vidy (input_three_valence_element_indices_and_instruction, elements, joints, joints_map);
                    break;
                }
        }

    ////////////////////////////////////////////////////////////////////////////////
    // Create and Align Joints 1. Iterate type 2. Select wood::joint based
    // on not/given user joint_type
    ////////////////////////////////////////////////////////////////////////////////

    try
        {
            wood::joint_lib::construct_joint_by_index (elements, joints, default_parameters_for_JOINTS_TYPES,
                                                       scale); // division_distance, shift,  // CGAL_Debug(99999);
        }
    catch (std::exception &e)
        {
            printf ("\nCPP   FILE %s    METHOD %s   LINE %i     WHAT %s ", __FILE__, __FUNCTION__, __LINE__, "Error in construct_joint_by_index");
            return;
        }
    catch (...)
        {
            printf ("\nCPP   FILE %s    METHOD %s   LINE %i     WHAT %s ", __FILE__, __FUNCTION__, __LINE__, "Error in construct_joint_by_index");
            return;
        }

    //////////////////////////////////////////////////////////////////////////////
    // Iterate wood::joint address
    //////////////////////////////////////////////////////////////////////////////
    output_plines = std::vector<std::vector<Polyline> > (elements.size ());
    output_types = std::vector<std::vector<wood::cut::cut_type> > (elements.size ());

    try
        {
            for (auto i = 0; i < elements.size (); i++) // elements.size()
                {                                       // takes 30-50 ms just to copy-paste polyline geometry
                    switch (output_type)
                        {
                        case (0):
                            elements[i].get_joints_geometry (joints, output_plines, 0, output_types);
                            break;
                        case (1):
                            elements[i].get_joints_geometry (joints, output_plines, 1, output_types);
                            break;
                        case (2):
                            elements[i].get_joints_geometry (joints, output_plines, 2,
                                                             output_types); // Display error
                            break;
                        default:
                        case (3):
                            elements[i].get_joints_geometry (joints, output_plines, 3, output_types);
                            break;
                        case (4):

                            try
                                {
                                    elements[i].merge_joints (joints, output_plines);
                                }
                            catch (const std::overflow_error &e)
                                {
                                    (void)e;
                                    printf ("\nCPP   FILE %s    METHOD %s "
                                            "  LINE %i     WHAT %s  ",
                                            __FILE__, __FUNCTION__, __LINE__,
                                            "this executes if f() throws "
                                            "std::overflow_error(same "
                                            "type rule)");
                                } // this executes if f() throws
                                  // std::overflow_error (same type rule)
                            catch (const std::runtime_error &e)
                                {
                                    (void)e;
                                    printf ("\nCPP   FILE %s    METHOD %s "
                                            "  LINE %i     WHAT %s  ",
                                            __FILE__, __FUNCTION__, __LINE__,
                                            "this executes if f() throws "
                                            "std::underflow_error (base "
                                            "class rule)");
                                } // this executes if f() throws
                                  // std::underflow_error (base class rule)
                            catch (const std::exception &e)
                                {
                                    (void)e;
                                    printf ("\nCPP   FILE %s    METHOD %s "
                                            "  LINE %i     WHAT %s  ",
                                            __FILE__, __FUNCTION__, __LINE__,
                                            " this executes if f() throws "
                                            "std::logic_error (base class "
                                            "rule)");
                                } // this executes if f() throws
                                  // std::logic_error (base class rule)
                            catch (...)
                                {
                                    printf ("\nCPP   FILE %s    METHOD %s   "
                                            "LINE %i     WHAT %s  ",
                                            __FILE__, __FUNCTION__, __LINE__,
                                            " // this executes if f() throws "
                                            "std::string or int or any other "
                                            "unrelated type");
                                } // this executes if f() throws
                                  // std::string or int or any other
                                  // unrelated type

                            break;

                        case (5):
                            elements[i].get_joints_geometry (joints, output_plines, 5, output_types);
                            break;
                        }
                }
        }
    catch (...)
        {
            printf ("\nCPP   FILE %s    METHOD %s   LINE %i     WHAT %s ", __FILE__, __FUNCTION__, __LINE__, "Error in get_joints_geometry");
            return;
        }

    //////////////////////////////////////////////////////////////////////////////
    // Create Mesh Triangulation for top face
    //////////////////////////////////////////////////////////////////////////////
    if (triangulate)
        {
            top_face_triangulation = std::vector<std::vector<int> > (elements.size ());
            for (int i = 0; i < elements.size (); i++)
                {
                    int v, f;
                    cgal::polyline_mesh_util::mesh_from_polylines (output_plines[i], elements[i].planes[0], top_face_triangulation[i], v, f);
                }
        }
}

void
beam_volumes (std::vector<Polyline> &polylines, std::vector<std::vector<double> > &polylines_segment_radii, std::vector<std::vector<session_cpp::Vector> > &polylines_segment_direction,
              std::vector<int> &input_allowed_types_per_polyline, double &min_distance, double &volume_length, double &cross_or_side_to_end, int &flip_male,

              // output of wood::joint areas
              std::vector<std::vector<int> > &index_polylines, std::vector<std::vector<int> > &index_polylines_segment, std::vector<std::vector<double> > &distance, std::vector<std::vector<session_cpp::Point> > &point_pairs,
              std::vector<std::vector<Polyline> > &volume_pairs, std::vector<Polyline> &joints_areas, std::vector<int> &joints_types,

              // Global Parameters and output wood::joint selection and orientation
              std::vector<std::vector<Polyline> > &output_plines, std::vector<std::vector<wood::cut::cut_type> > &output_types, bool compute_joints, double division_distance, double shift, int output_type,
              bool use_eccentricities_to_scale_joints

)
{
    //////////////////////////////////////////////////////////////////////////////
    // Main Properties: elements, joints, joints_map
    //////////////////////////////////////////////////////////////////////////////

    std::vector<double> scale = { 1, 1, 1 };

    std::vector<wood::element> elements;
    elements.reserve (polylines.size ());
    for (int i = 0; i < polylines.size (); i++)
        {
            elements.emplace_back (i);
            elements.back ().central_polyline = polylines[i];
            elements.back ().j_mf = std::vector<std::vector<std::tuple<int, bool, double> > > (1);
        }

    std::vector<wood::joint> joints;
    std::unordered_map<uint64_t, int> joints_map;

    /////////////////////////////////////////////////////////////////////
    // Segment callback
    /////////////////////////////////////////////////////////////////////

    auto segment = [&polylines] (std::size_t pid, std::size_t sid) { return session_cpp::Line::from_points (polylines[pid][sid], polylines[pid][sid + 1]); };

    /////////////////////////////////////////////////////////////////////
    // Build inflated AABB bounding boxes for BVH broadphase
    /////////////////////////////////////////////////////////////////////
    std::vector<session_cpp::BoundingBox> boxes;
    std::vector<std::pair<std::size_t, std::size_t>> seg_refs;
    for (std::size_t pid = 0; pid < polylines.size (); ++pid)
        {
            for (std::size_t sid = 0; sid < polylines[pid].size () - 1; ++sid)
                {
                    session_cpp::Line seg = segment (pid, sid);
                    double radius = polylines_segment_radii[pid][sid];

                    session_cpp::Vector zaxis = seg.to_vector ();
                    session_cpp::Point _seg_start = seg.start ();
                    session_cpp::Plane plane = session_cpp::Plane::from_point_normal (_seg_start, zaxis);
                    session_cpp::Vector x_axis = plane.x_axis () * radius;
                    session_cpp::Vector y_axis = plane.y_axis () * radius;

                    std::array<session_cpp::Point, 8> pts = {
                        seg.start () + x_axis + y_axis, seg.start () - x_axis + y_axis, seg.start () - x_axis - y_axis, seg.start () + x_axis - y_axis,
                        seg.end () + x_axis + y_axis,   seg.end () - x_axis + y_axis,   seg.end () - x_axis - y_axis,   seg.end () + x_axis - y_axis,
                    };

                    double xmn = pts[0][0], xmx = xmn, ymn = pts[0][1], ymx = ymn, zmn = pts[0][2], zmx = zmn;
                    for (int k = 1; k < 8; k++)
                        {
                            xmn = std::min (xmn, pts[k][0]); xmx = std::max (xmx, pts[k][0]);
                            ymn = std::min (ymn, pts[k][1]); ymx = std::max (ymx, pts[k][1]);
                            zmn = std::min (zmn, pts[k][2]); zmx = std::max (zmx, pts[k][2]);
                        }
                    session_cpp::Point center ((xmn + xmx) * 0.5, (ymn + ymx) * 0.5, (zmn + zmx) * 0.5);
                    session_cpp::Vector half_size ((xmx - xmn) * 0.5, (ymx - ymn) * 0.5, (zmx - zmn) * 0.5);
                    boxes.emplace_back (center, session_cpp::Vector::x_axis (), session_cpp::Vector::y_axis (), session_cpp::Vector::z_axis (), half_size);
                    seg_refs.emplace_back (pid, sid);
                }
        }

    /////////////////////////////////////////////////////////////////////
    // BVH broadphase self-intersection
    /////////////////////////////////////////////////////////////////////
    std::map<uint64_t, std::tuple<double, int, int, int, int>> pair_collisions;

    if (!boxes.empty ())
        {
            double world_size = session_cpp::BVH::compute_world_size (boxes);
            session_cpp::BVH bvh = session_cpp::BVH::from_boxes (boxes, world_size);
            auto [collision_pairs, unused_col, unused_cnt] = bvh.check_all_collisions (boxes);

            for (auto &[bi, bj] : collision_pairs)
                {
                    auto [pid0, sid0] = seg_refs[bi];
                    auto [pid1, sid1] = seg_refs[bj];

                    if (pid0 == pid1)
                        continue;

                    session_cpp::Line s0 = segment (pid0, sid0);
                    session_cpp::Line s1 = segment (pid1, sid1);
                    double distance = cgal::box_search::segment_segment_sq_distance (s0, s1);

                    if (distance < min_distance * min_distance)
                        {
                            bool flipped = pid1 > pid0;
                            uint64_t id = flipped ? ((uint64_t)pid1 << 32 | pid0) : ((uint64_t)pid0 << 32 | pid1);

                            auto dist_ids = flipped ? std::make_tuple (distance, (int)pid0, (int)sid0, (int)pid1, (int)sid1) : std::make_tuple (distance, (int)pid1, (int)sid1, (int)pid0, (int)sid0);

                            if (pair_collisions.find (id) == pair_collisions.end ())
                                pair_collisions.insert ({ id, dist_ids });
                            else if (distance < std::get<0> (pair_collisions[id]))
                                pair_collisions[id] = dist_ids;
                        }
                }
        }

    /////////////////////////////////////////////////////////////////////
    // Iterate the result, get insertion points and parameter on the lines
    /////////////////////////////////////////////////////////////////////
    index_polylines.reserve (pair_collisions.size () * 2);
    index_polylines_segment.reserve (pair_collisions.size () * 2);
    distance.reserve (pair_collisions.size () * 2);
    point_pairs.reserve (point_pairs.size () * 2);

    for (auto const &x : pair_collisions)
        {
            auto &v = x.second;

            ///////////////////////////////////////////////////////////////////////
            // line line intersection and type detection 0-0 0-1 1-1
            ///////////////////////////////////////////////////////////////////////
            session_cpp::Line s0 = segment (std::get<1> (v), std::get<2> (v));
            session_cpp::Line s1 = segment (std::get<3> (v), std::get<4> (v));

            session_cpp::Point p0;
            session_cpp::Point p1;
            session_cpp::Vector v0;
            session_cpp::Vector v1;
            session_cpp::Vector normal;
            bool type0, type1;
            bool is_parallel = false;

            bool r = cgal::box_search::line_line_intersection_with_properties (s0, s1, (int)(polylines[std::get<1> (v)].size () - 1), (int)(polylines[std::get<3> (v)].size () - 1), std::get<2> (v), std::get<4> (v),
                                                                               cross_or_side_to_end, p0, p1, v0, v1, normal, type0, type1, is_parallel);

            if (!r)
                continue;

            ///////////////////////////////////////////////////////////////////////
            // Check the assigned types
            ///////////////////////////////////////////////////////////////////////

            auto is_valid_type = [] (int sum_of_type0_type1, int allowed_type) {
                switch (allowed_type)
                    {
                    case (0):
                        return sum_of_type0_type1 == 0;
                    case (1):
                        return sum_of_type0_type1 == 1 || sum_of_type0_type1 == 2;
                    case (-1):
                        return true;
                    default:
                        return false;
                    }
            };

            if (input_allowed_types_per_polyline.size () > 0)
                {
                    if (input_allowed_types_per_polyline.size () == 1)
                        {
                            if (!(is_valid_type (type0 + type1, input_allowed_types_per_polyline[0])))
                                continue;
                        }
                    else if (input_allowed_types_per_polyline.size () == polylines.size ())
                        {
                            bool allowed_0 = is_valid_type (type0 + type1, input_allowed_types_per_polyline[std::get<1> (v)]);
                            bool allowed_1 = is_valid_type (type0 + type1, input_allowed_types_per_polyline[std::get<3> (v)]);
                            if (!(allowed_0 && allowed_1))
                                continue;
                        }
                }

            ///////////////////////////////////////////////////////////////////////
            // Output
            ///////////////////////////////////////////////////////////////////////
            index_polylines.emplace_back (std::vector<int>{ std::get<1> (v), std::get<3> (v) });
            index_polylines_segment.emplace_back (std::vector<int>{ std::get<2> (v), std::get<4> (v) });
            distance.emplace_back (std::vector<double>{ (double)std::get<0> (v) });
            point_pairs.emplace_back (std::vector<session_cpp::Point>{ p0, p1 });

            ///////////////////////////////////////////////////////////////////////
            // draw rectangles volumes around intersection points
            ///////////////////////////////////////////////////////////////////////
            session_cpp::Vector segment_normal0 = polylines_segment_direction.size () == 0 ? normal : polylines_segment_direction[std::get<1> (v)][std::get<2> (v)];
            session_cpp::Vector segment_normal1 = polylines_segment_direction.size () == 0 ? normal : polylines_segment_direction[std::get<3> (v)][std::get<4> (v)];

            std::vector<Polyline> beam_volume (4);
            cgal::box_search::two_rect_from_point_vector_and_zaxis (p0, v0, segment_normal0, type0, polylines_segment_radii[std::get<1> (v)][std::get<2> (v)], volume_length, flip_male, beam_volume[0], beam_volume[1]);
            cgal::box_search::two_rect_from_point_vector_and_zaxis (p1, v1, segment_normal1, type1, polylines_segment_radii[std::get<3> (v)][std::get<4> (v)], volume_length, flip_male, beam_volume[2], beam_volume[3]);

            ///////////////////////////////////////////////////////////////////////
            // Cut rectangles by planes, 0-0 bisector, 0-1 or 1-0 be closer
            // female side orient cross wood::joint closer to female
            ///////////////////////////////////////////////////////////////////////
            std::array<session_cpp::Point, 4> intersection_points;
            if (type0 + type1 == 0)
                {
                    // if v0,v1 are oriented from intersection point the
                    // plane normal is v0+v1

                    // Get cutting planes

                    session_cpp::Point p = session_cpp::Point::mid_point(p0, p1);
                    session_cpp::Point p0_ = p + v0;
                    session_cpp::Point p1_ = p + v1;
                    session_cpp::Vector _diff_v = v0 - v1;
                    session_cpp::Plane cut_plane = is_parallel ? session_cpp::Plane::from_point_normal (p, v0) : session_cpp::Plane::from_point_normal (p, _diff_v);

                    bool oriented_towards_v0 = (cut_plane.z_axis ().dot (session_cpp::Vector (p0_[0]-cut_plane.origin ()[0], p0_[1]-cut_plane.origin ()[1], p0_[2]-cut_plane.origin ()[2])) > 0);
                    session_cpp::Vector _cpz = cut_plane.z_axis (); session_cpp::Vector _cpzn = _cpz * -1.0;
                    session_cpp::Plane cut_plane0 = oriented_towards_v0 ? session_cpp::Plane::from_point_normal (p, _cpz) : session_cpp::Plane::from_point_normal (p, _cpzn);
                    session_cpp::Plane cut_plane1 = !oriented_towards_v0 ? session_cpp::Plane::from_point_normal (p, _cpz) : session_cpp::Plane::from_point_normal (p, _cpzn);

                    for (size_t lid = 0; lid < 2; lid++)
                        {
                            int shift = lid == 0 ? 0 : 2;
                            session_cpp::Plane cut_plane_ = lid == 0 ? cut_plane0 : cut_plane1;

                            // Intersect segments with plane
                            session_cpp::Line s0 = session_cpp::Line::from_points (beam_volume[0 + shift][0], beam_volume[0 + shift][1]);
                            session_cpp::Line s1 = session_cpp::Line::from_points (beam_volume[0 + shift][3], beam_volume[0 + shift][2]);
                            bool result = cgal::box_search::line_plane (s0, cut_plane_, intersection_points[0]);
                            result = cgal::box_search::line_plane (s1, cut_plane_, intersection_points[1]);
                            s0 = session_cpp::Line::from_points (beam_volume[1 + shift][0], beam_volume[1 + shift][1]);
                            s1 = session_cpp::Line::from_points (beam_volume[1 + shift][3], beam_volume[1 + shift][2]);
                            result = cgal::box_search::line_plane (s0, cut_plane_, intersection_points[2]);
                            result = cgal::box_search::line_plane (s1, cut_plane_, intersection_points[3]);

                            // points on positive side must be moved
                            if ((cut_plane_.z_axis ().dot (session_cpp::Vector (beam_volume[0 + shift][0][0]-cut_plane_.origin ()[0], beam_volume[0 + shift][0][1]-cut_plane_.origin ()[1], beam_volume[0 + shift][0][2]-cut_plane_.origin ()[2])) < 0))
                                {
                                    beam_volume[0 + shift][0] = intersection_points[0];
                                    beam_volume[0 + shift][3] = intersection_points[1];
                                    beam_volume[0 + shift][4] = beam_volume[0 + shift][0];

                                    beam_volume[1 + shift][0] = intersection_points[2];
                                    beam_volume[1 + shift][3] = intersection_points[3];
                                    beam_volume[1 + shift][4] = beam_volume[1 + shift][0];
                                }
                            else
                                {
                                    beam_volume[0 + shift][1] = intersection_points[0];
                                    beam_volume[0 + shift][2] = intersection_points[1];

                                    beam_volume[1 + shift][1] = intersection_points[2];
                                    beam_volume[1 + shift][2] = intersection_points[3];
                                }
                        }
                }
            else if (type0 + type1 == 1)
                {
                    // CGAL_Debug(1000);
                    // if male vector is oriented from intersection point,
                    // the p+v closer rectangle is the cutting plane
                    int closer_rect;
                    int farrer_rect;
                    if (type0 == 0)
                        {
                            bool closer = session_cpp::Point::squared_distance(p0 + v0, beam_volume[2][0]) < session_cpp::Point::squared_distance(p0 + v0, beam_volume[3][0]);
                            closer_rect = closer ? 2 : 3;
                            farrer_rect = closer ? 3 : 2;
                        }
                    else
                        {
                            bool closer = session_cpp::Point::squared_distance(p1 + v1, beam_volume[0][0]) < session_cpp::Point::squared_distance(p1 + v1, beam_volume[1][0]);
                            closer_rect = closer ? 0 : 1;
                            farrer_rect = closer ? 1 : 0;
                        }

                    session_cpp::Vector rect_v0 = beam_volume[closer_rect][1] - beam_volume[closer_rect][0];
                    session_cpp::Vector rect_v1 = beam_volume[closer_rect][2] - beam_volume[closer_rect][0];
                    session_cpp::Vector rect_normal = (rect_v0).cross(rect_v1);

                    session_cpp::Plane cut_plane = session_cpp::Plane::from_point_normal (beam_volume[closer_rect][0], rect_normal);
                    if ((cut_plane.z_axis ().dot (session_cpp::Vector (beam_volume[farrer_rect][0][0]-cut_plane.origin ()[0], beam_volume[farrer_rect][0][1]-cut_plane.origin ()[1], beam_volume[farrer_rect][0][2]-cut_plane.origin ()[2])) > 0))
                        { session_cpp::Vector _neg_rn = rect_normal * -1.0; cut_plane = session_cpp::Plane::from_point_normal (beam_volume[closer_rect][0], _neg_rn); }

                    // Get intersection points

                    int shift = type0 == 0 ? 0 : 2;
                    session_cpp::Line s0 = session_cpp::Line::from_points (beam_volume[0 + shift][0], beam_volume[0 + shift][1]);
                    session_cpp::Line s1 = session_cpp::Line::from_points (beam_volume[0 + shift][3], beam_volume[0 + shift][2]);
                    bool result = cgal::box_search::line_plane (s0, cut_plane, intersection_points[0]);
                    result = cgal::box_search::line_plane (s1, cut_plane, intersection_points[1]);
                    s0 = session_cpp::Line::from_points (beam_volume[1 + shift][0], beam_volume[1 + shift][1]);
                    s1 = session_cpp::Line::from_points (beam_volume[1 + shift][3], beam_volume[1 + shift][2]);
                    result = cgal::box_search::line_plane (s0, cut_plane, intersection_points[2]);
                    result = cgal::box_search::line_plane (s1, cut_plane, intersection_points[3]);

                    if ((cut_plane.z_axis ().dot (session_cpp::Vector (beam_volume[0 + shift][0][0]-cut_plane.origin ()[0], beam_volume[0 + shift][0][1]-cut_plane.origin ()[1], beam_volume[0 + shift][0][2]-cut_plane.origin ()[2])) < 0))
                        {
                            beam_volume[0 + shift][0] = intersection_points[0];
                            beam_volume[0 + shift][3] = intersection_points[1];
                            beam_volume[0 + shift][4] = beam_volume[0 + shift][0];

                            beam_volume[1 + shift][0] = intersection_points[2];
                            beam_volume[1 + shift][3] = intersection_points[3];
                            beam_volume[1 + shift][4] = beam_volume[1 + shift][0];
                        }
                    else
                        {
                            beam_volume[0 + shift][1] = intersection_points[0];
                            beam_volume[0 + shift][2] = intersection_points[1];

                            beam_volume[1 + shift][1] = intersection_points[2];
                            beam_volume[1 + shift][2] = intersection_points[3];
                        }
                }

            //////////////////////////////////////////////////////////////////////////////
            // Construct wood::element properties, side polylines and
            // planes
            //////////////////////////////////////////////////////////////////////////////

            bool found_joint = pair_search (elements, beam_volume, std::get<1> (v), std::get<3> (v), (type0 + type1 == 2), joints, joints_map);

            if (found_joint)
                {
                    joints_areas.emplace_back (joints[joints.size () - 1].joint_area);
                    joints_types.emplace_back (joints[joints.size () - 1].type);
                }

            //////////////////////////////////////////////////////////////////////////////
            // Search Contact zones
            //////////////////////////////////////////////////////////////////////////////

            // this is wrong because after joints must be distributed to
            // elements that is why you need joints list
            // volume_pairs.emplace_back(beam_volume);

            volume_pairs.emplace_back (beam_volume);
        }

    if (!compute_joints)
        return;

    //////////////////////////////////////////////////////////////////////////////////
    ////Create and Align Joints 1. Iterate type 2. Select wood::joint based
    /// on not/given user joint_type
    //////////////////////////////////////////////////////////////////////////////////

    // Experimental scale joints by value of intersection
    // This works only when two beams are the same radius
    if (use_eccentricities_to_scale_joints && joints.size () == point_pairs.size ())
        {
            scale.clear ();
            for (int i = 0; i < point_pairs.size (); i++)
                {
                    double L = std::sqrt (session_cpp::Point::squared_distance(point_pairs[i][0], point_pairs[i][1]));
                    double L_ = L;
                    L *= 0.5;

                    double scale_value = 1;
                    if (L > 0.01)
                        {
                            // double max_r =
                            // std::max(polylines_segment_radii[joints[i].v0][0],
                            // polylines_segment_radii[joints[i].v1][0]);
                            double max_r = (polylines_segment_radii[joints[i].v0][0] + polylines_segment_radii[joints[i].v1][0]) * 0.5;

                            // L == 0 will be scale 1
                            //
                            L = (max_r - L) / max_r;

                            // L /=
                            // (polylines_segment_radii[joints[i].v0][0] +
                            // polylines_segment_radii[joints[i].v1][0]) *
                            // 0.5;
                            scale_value = std::cos (std::asin (1 - std::min (L,
                                                                             1.0))); // Math.Cos(Math.Asin(1 - x));
                            // scale_value = std::sin(std::acos(1 -
                            // std::min(L, 1.0)));//Math.Cos(Math.Asin(1 -
                            // x));
                        }

                    // std::cout << L_ << " " << L << " " << std::endl;

                    joints[i].scale[0] = scale_value;
                    joints[i].scale[1] = scale_value;
                    joints[i].scale[2] = 1;
                }
            scale.clear ();
        }

    wood::joint_lib::construct_joint_by_index (elements, joints, wood::GLOBALS::JOINTS_PARAMETERS_AND_TYPES,
                                               scale); // division_distance, shift,
    //////////////////////////////////////////////////////////////////////////////
    // Iterate wood::joint address
    //////////////////////////////////////////////////////////////////////////////

    output_plines = std::vector<std::vector<Polyline> > (elements.size ());
    output_types = std::vector<std::vector<wood::cut::cut_type> > (elements.size ());
    for (int i = 0; i < elements.size (); i++)
        { // takes 30-50 ms just to copy past polyline geometry
            switch (output_type)
                {
                case (0):
                    elements[i].get_joints_geometry (joints, output_plines, 0, output_types);
                    break;
                case (1):
                    elements[i].get_joints_geometry (joints, output_plines, 1, output_types);
                    break;
                case (2):
                    elements[i].get_joints_geometry (joints, output_plines, 2, output_types);
                    break;
                default:
                case (3):
                    elements[i].get_joints_geometry (joints, output_plines, 3, output_types);
                    break;
                case (4):
                    break;
                }
        }
}

#pragma endregion

} // namespace main
} // namespace wood
