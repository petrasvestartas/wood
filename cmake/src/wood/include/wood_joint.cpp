
#include "wood_joint.h"

#include "../../../stdafx.h" //go up to the folder where the CMakeLists.txt is
namespace wood
{

joint::joint ()
{
    // Empty constructor
}

joint::joint (int _id, int _v0, int _v1, int _f0_0, int _f1_0, int _f0_1, int _f1_1, std::array<CGAL_Polyline, 4> &_joint_volumes)
    : id (_id), v0 (_v0), v1 (_v1), f0_0 (_f0_0), f1_0 (_f1_0), f0_1 (_f0_1), f1_1 (_f1_1), type (-1)
{
    for (int i = 0; i < 4; i++)
        this->joint_volumes[i] = _joint_volumes[i];
}

joint::joint (int _id, int _v0, int _v1, int _f0_0, int _f1_0, int _f0_1, int _f1_1, CGAL_Polyline (&_joint_area), std::array<CGAL_Polyline, 2> &_joint_lines, std::array<CGAL_Polyline, 4> &_joint_volumes, int _type)
    : id (_id), v0 (_v0), v1 (_v1), f0_0 (_f0_0), f1_0 (_f1_0), f0_1 (_f0_1), f1_1 (_f1_1), type (_type)
{
    for (int i = 0; i < 4; i++)
        this->joint_volumes[i] = _joint_volumes[i];

    for (int i = 0; i < 2; i++)
        this->joint_lines[i] = _joint_lines[i];

    this->joint_area = _joint_area;
}

std::vector<CGAL_Polyline> &
joint::operator() (bool male_or_female, bool first_or_second)
{
    if (male_or_female)
        {
            return first_or_second ? m[0] : m[1];
        }
    else
        {
            return first_or_second ? f[0] : f[1];
        }
}

void
joint::get_edge_ids (bool male_or_female, int &fA, int &fB)
{
    if (male_or_female)
        {
            fA = f0_0;
            fB = f0_1;
        }
    else
        {
            fA = f1_0;
            fB = f1_1;
        }
}

wood::cut::cut_type &
joint::get_first_cutting_type (bool male_or_female)
{
    return male_or_female ? m_boolean_type[0] : f_boolean_type[0];
}

std::vector<wood::cut::cut_type> &
joint::operator() (bool male_or_female)
{
    return male_or_female ? m_boolean_type : f_boolean_type;
}

void
joint::reverse (bool male_or_female)
{
    if (male_or_female)
        {
            std::swap (m[0], m[1]);
        }
    else
        {
            std::swap (f[0], f[1]);
        }
}

std::string
joint::get_key ()
{
    if (this->key.empty ()) // Check if key is already generated
        {
            std::string key_0 = name;                                                                                                             // Use joint name as part of the key
            std::string key_1 = std::to_string ((shift + 0.000000001)).substr (0, std::to_string ((shift + 0.000000001)).find (".") + 3);         // Format shift to 2 decimal places
            std::string key_2 = std::to_string ((divisions + 0.000000001)).substr (0, std::to_string ((divisions + 0.000000001)).find (".") + 3); // Format divisions to 2 decimal places
            std::string joining = ";";                                                                                                            // Separator

            // Construct the full key by joining the parts
            this->key = key_0 + joining + key_1 + joining + key_2;
        }

    return key;
}

bool
joint::change_basis (CGAL_Polyline &rect0, CGAL_Polyline &rect1, CGAL::Aff_transformation_3<IK> &xform)
{
    IK::Point_3 O1 (-0.5, -0.5, -0.5);
    IK::Vector_3 X1 (1, 0, 0); // final frame X (X,Y,Z = arbitrary basis)
    IK::Vector_3 Y1 (0, 1, 0); // final frame Y
    IK::Vector_3 Z1 (0, 0, 1); // final frame Z

    IK::Point_3 O0 = rect0[0];
    IK::Vector_3 X0 = (rect0[1] - rect0[0]); // initial frame X (X,Y,Z = arbitrary basis)
    IK::Vector_3 Y0 = (rect0[3] - rect0[0]); // initial frame Y
    IK::Vector_3 Z0 = (rect1[0] - rect0[0]); // initial frame Z

    // Q = a0*X0 + b0*Y0 + c0*Z0 = a1*X1 + b1*Y1 + c1*Z1
    // then this transform will map the point (a0,b0,c0) to (a1,b1,c1)

    //*this = ON_Xform::ZeroTransformation;

    double a, b, c, d;
    a = X1 * Y1;
    b = X1 * Z1;
    c = Y1 * Z1;
    double R[3][6] = { { X1 * X1, a, b, X1 * X0, X1 * Y0, X1 * Z0 }, { a, Y1 * Y1, c, Y1 * X0, Y1 * Y0, Y1 * Z0 }, { b, c, Z1 * Z1, Z1 * X0, Z1 * Y0, Z1 * Z0 } };

    // row reduce R
    int i0 = (R[0][0] >= R[1][1]) ? 0 : 1;
    if (R[2][2] > R[i0][i0])
        i0 = 2;
    int i1 = (i0 + 1) % 3;
    int i2 = (i1 + 1) % 3;
    if (R[i0][i0] == 0.0)
        return false;
    d = 1.0 / R[i0][i0];
    R[i0][0] *= d;
    R[i0][1] *= d;
    R[i0][2] *= d;
    R[i0][3] *= d;
    R[i0][4] *= d;
    R[i0][5] *= d;
    R[i0][i0] = 1.0;
    if (R[i1][i0] != 0.0)
        {
            d = -R[i1][i0];
            R[i1][0] += d * R[i0][0];
            R[i1][1] += d * R[i0][1];
            R[i1][2] += d * R[i0][2];
            R[i1][3] += d * R[i0][3];
            R[i1][4] += d * R[i0][4];
            R[i1][5] += d * R[i0][5];
            R[i1][i0] = 0.0;
        }
    if (R[i2][i0] != 0.0)
        {
            d = -R[i2][i0];
            R[i2][0] += d * R[i0][0];
            R[i2][1] += d * R[i0][1];
            R[i2][2] += d * R[i0][2];
            R[i2][3] += d * R[i0][3];
            R[i2][4] += d * R[i0][4];
            R[i2][5] += d * R[i0][5];
            R[i2][i0] = 0.0;
        }

    if (fabs (R[i1][i1]) < fabs (R[i2][i2]))
        {
            int i = i1;
            i1 = i2;
            i2 = i;
        }
    if (R[i1][i1] == 0.0)
        return false;
    d = 1.0 / R[i1][i1];
    R[i1][0] *= d;
    R[i1][1] *= d;
    R[i1][2] *= d;
    R[i1][3] *= d;
    R[i1][4] *= d;
    R[i1][5] *= d;
    R[i1][i1] = 1.0;
    if (R[i0][i1] != 0.0)
        {
            d = -R[i0][i1];
            R[i0][0] += d * R[i1][0];
            R[i0][1] += d * R[i1][1];
            R[i0][2] += d * R[i1][2];
            R[i0][3] += d * R[i1][3];
            R[i0][4] += d * R[i1][4];
            R[i0][5] += d * R[i1][5];
            R[i0][i1] = 0.0;
        }
    if (R[i2][i1] != 0.0)
        {
            d = -R[i2][i1];
            R[i2][0] += d * R[i1][0];
            R[i2][1] += d * R[i1][1];
            R[i2][2] += d * R[i1][2];
            R[i2][3] += d * R[i1][3];
            R[i2][4] += d * R[i1][4];
            R[i2][5] += d * R[i1][5];
            R[i2][i1] = 0.0;
        }

    if (R[i2][i2] == 0.0)
        return false;
    d = 1.0 / R[i2][i2];
    R[i2][0] *= d;
    R[i2][1] *= d;
    R[i2][2] *= d;
    R[i2][3] *= d;
    R[i2][4] *= d;
    R[i2][5] *= d;
    R[i2][i2] = 1.0;
    if (R[i0][i2] != 0.0)
        {
            d = -R[i0][i2];
            R[i0][0] += d * R[i2][0];
            R[i0][1] += d * R[i2][1];
            R[i0][2] += d * R[i2][2];
            R[i0][3] += d * R[i2][3];
            R[i0][4] += d * R[i2][4];
            R[i0][5] += d * R[i2][5];
            R[i0][i2] = 0.0;
        }
    if (R[i1][i2] != 0.0)
        {
            d = -R[i1][i2];
            R[i1][0] += d * R[i2][0];
            R[i1][1] += d * R[i2][1];
            R[i1][2] += d * R[i2][2];
            R[i1][3] += d * R[i2][3];
            R[i1][4] += d * R[i2][4];
            R[i1][5] += d * R[i2][5];
            R[i1][i2] = 0.0;
        }

    CGAL::Aff_transformation_3<IK> m_xform (R[0][3], R[0][4], R[0][5], R[1][3], R[1][4], R[1][5], R[2][3], R[2][4], R[2][5]);

    CGAL::Aff_transformation_3<IK> T0 (CGAL::TRANSLATION, IK::Vector_3 (0 - O1.x (), 0 - O1.y (), 0 - O1.z ()));
    CGAL::Aff_transformation_3<IK> T2 (CGAL::TRANSLATION, IK::Vector_3 (O0.x (), O0.y (), O0.z ()));

    xform = (T2 * m_xform * T0);

    return true;
}

void
joint::transform (CGAL::Aff_transformation_3<IK> &xform0, CGAL::Aff_transformation_3<IK> &xform1)
{
    // Apply transformation xform0 to all polylines in the male part
    for (auto &polyline : m[0])
        for (auto &point : polyline)
            point = point.transform (xform0);

    for (auto &polyline : m[1])
        for (auto &point : polyline)
            point = point.transform (xform0);

    // Apply transformation xform1 to all polylines in the female part
    for (auto &polyline : f[0])
        for (auto &point : polyline)
            point = point.transform (xform1);

    for (auto &polyline : f[1])
        for (auto &point : polyline)
            point = point.transform (xform1);
}

bool
joint::orient_to_connection_area ()
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Unit scale to avoid z-axis stretching
    // The strech factor is equal to the 2nd joint volume edge length
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (unit_scale)
        {
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Distance between two rectangles is equal to the joint volume second edge length
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            unit_scale_distance = unit_scale_distance == 0 ? std::floor (std::sqrt (CGAL::squared_distance (joint_volumes[0][1], joint_volumes[0][2]))) : unit_scale_distance;
            // std::cout << std::sqrt(CGAL::squared_distance(joint_volumes[0][0], joint_volumes[0][1]))
            // << std::endl; std::cout << std::sqrt(CGAL::squared_distance(joint_volumes[0][1],
            // joint_volumes[0][2])) << std::endl;
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // First pair of joint volumes
            // The joint volumes are move in z-axis, so that the joint geometry would not be stretched
            // The distance of the movement is equal to the joint-volume rectangle edge length
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            IK::Segment_3 volume_segment (joint_volumes[0][0], joint_volumes[1][0]);
            IK::Vector_3 vec = volume_segment.to_vector () * 0.5;
            IK::Vector_3 vec_unit = volume_segment.to_vector ();
            cgal::vector_util::unitize (vec_unit);
            vec_unit *= (unit_scale_distance * 0.5);

            cgal::polyline_util::move (joint_volumes[0], vec);
            cgal::polyline_util::move (joint_volumes[1], -vec);
            cgal::polyline_util::move (joint_volumes[0], -vec_unit);
            cgal::polyline_util::move (joint_volumes[1], vec_unit);

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Second pair of joint volumes
            // If two pairs of joint volumes are given, then two transformations must be calculated, the
            // code is the same as above
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            if (joint_volumes[2].size () > 0)
                {
                    IK::Segment_3 volume_segment (joint_volumes[2][0], joint_volumes[3][0]);
                    vec = volume_segment.to_vector () * 0.5;
                    vec_unit = volume_segment.to_vector ();
                    cgal::vector_util::unitize (vec_unit);
                    vec_unit *= (unit_scale_distance * 0.5);

                    cgal::polyline_util::move (joint_volumes[2], vec);
                    cgal::polyline_util::move (joint_volumes[3], -vec);
                    cgal::polyline_util::move (joint_volumes[2], -vec_unit);
                    cgal::polyline_util::move (joint_volumes[3], vec_unit);
                }
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Scale transformation
    // To increase the joint for cases like cross joints or raw timber
    // This proption was implemented first time for round wood cross joint intersection
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (scale[0] != 1.0 || scale[1] != 1.0 || scale[2] != 1.0)
        {
            auto xform_scale = cgal::xform_util::scale (scale[0], scale[1], scale[2]);

            // std::cout << "Scale transformation: " << scale[0] << " " << scale[1] << " " << scale[2]
            // << std::endl;
            transform (xform_scale, xform_scale);
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create 2 change-of-basis transformations for the joint volume of the female and male parts
    // There can be two cases:
    // 1. The joint volume is made from 1 pair of rectangles e.g. side-to-side edge in-plane joint
    // 2. The joint volume is made from 2 pairs of rectangles e.g. side-to-side edge out-of-plane
    // joint
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CGAL::Aff_transformation_3<IK> xform0;
    bool flag0 = change_basis (joint_volumes[0], joint_volumes[1], xform0);

    CGAL::Aff_transformation_3<IK> xform1;
    bool flag1 = joint_volumes[2].size () > 0 ? change_basis (joint_volumes[2], joint_volumes[3], xform1) : change_basis (joint_volumes[0], joint_volumes[1], xform1);

    transform (xform0, xform1);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // output, true if the two transformations are successful
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    return flag0 && flag1;
}

void
joint::duplicate_geometry (joint &new_joint)
{
    new_joint.name = this->name;           // Duplicate name
    new_joint.shift = this->shift;         // Duplicate shift value
    new_joint.divisions = this->divisions; // Duplicate divisions value
    new_joint.length = this->length;       // Duplicate length value

    // Duplicate geometric data
    new_joint.m = this->m;
    new_joint.f = this->f;
    new_joint.m_boolean_type = this->m_boolean_type;
    new_joint.f_boolean_type = this->f_boolean_type;

    // Duplicate additional properties
    new_joint.unit_scale = this->unit_scale;
    new_joint.orient = this->orient;
}

void
joint::transfer_geometry (joint &geo_joint)
{
    this->name = geo_joint.name;           // Transfer name
    this->shift = geo_joint.shift;         // Transfer shift value
    this->divisions = geo_joint.divisions; // Transfer divisions value
    this->length = geo_joint.length;       // Transfer length value

    // Transfer geometric data
    this->m = geo_joint.m;
    this->f = geo_joint.f;
    this->m_boolean_type = geo_joint.m_boolean_type;
    this->f_boolean_type = geo_joint.f_boolean_type;

    // Transfer additional properties
    this->unit_scale = geo_joint.unit_scale;
    this->orient = geo_joint.orient;
}

void
joint::get_divisions (double &division_distance)
{
    division_length = division_distance; // Update division_length with the given division distance

    if (compute_geometrical_divisions)
        {
            // Check if a joint line exists to use its length for joint divisions calculation
            if (!joint_lines[0].empty ())
                {
                    // Calculate joint length as the distance between two points of the joint line
                    length = CGAL::squared_distance (joint_lines[0][0], joint_lines[0][1]);

                    // Calculate the number of divisions based on the joint length and division distance
                    divisions = (int)std::ceil (std::sqrt (length) / division_distance);

                    // Ensure divisions are within a reasonable range (1 to 100)
                    divisions = std::max (1, std::min (100, divisions));
                }
        }
}

void
joint::remove_geo_from_linked_joint_and_merge_with_current_joint (std::vector<joint> &all_joints)
{
    // return;
    if (linked_joints_seq.size () != linked_joints.size ()) // check if number of sequences is equa to linked joints
        {
            std::cout << "ERROR A in wood_join.cpp -> "
                         "remove_geo_from_linked_joint_and_merge_with_current_joint: "
                         "linked_joints_seq.size() != linked_joints.size() "
                      << linked_joints_seq.size () << " " << linked_joints.size () << "\n";
            return;
        }

    for (int i = 0; i < linked_joints.size (); i++) //
        {
            // check indices of linked and current joints to know which id to merge and remove -> a0-b0
            // | a0-b1
            bool m_f_curr = v0 == all_joints[linked_joints[i]].v0; // is it the item to merge, for both joints the
                                                                   // first==first or second==second must match
            bool m_f_next = m_f_curr;
            m_f_next = i == 0 ? m_f_next : !m_f_next; // if it is the second joint, we need to invert the
                                                      // result | does it mean it only works for 2 links only?

            if (linked_joints_seq[i].size () * 2 != (*this) (m_f_curr, true).size ()) // check number of sequences in the linked joints is equal to | multipled
                                                                                      // by 2 because the second line is just for merging
                {
                    std::cout << "ERROR B in wood_join.cpp -> "
                                 "remove_geo_from_linked_joint_and_merge_with_current_joint: "
                                 "linked_joints_seq[i].size() != (*this)(m_f_curr, true).size()) "
                              << linked_joints_seq[i].size () * 2 << " " << (*this) (m_f_curr, true).size () << "\n";
                    continue;
                }

            // merge joint using "linked_joints_seq"
            for (int j = 0; j < (*this) (m_f_curr, true).size (); j += 2)
                {
                    // sequence
                    int start_curr = linked_joints_seq[i][(int)(j * 0.5)][0];
                    int step_curr = linked_joints_seq[i][(int)(j * 0.5)][1];
                    int start_next = linked_joints_seq[i][(int)(j * 0.5)][2];
                    int step_next = linked_joints_seq[i][(int)(j * 0.5)][3];

                    if (start_curr == 0 && step_curr == 0 && start_next == 0 && step_next == 0)
                        continue;

                    if (step_curr == 0 || step_next == 0)
                        continue;

                    // create outlines
                    CGAL_Polyline merged_polyline_0;
                    CGAL_Polyline merged_polyline_1;
                    merged_polyline_0.reserve ((*this) (m_f_curr, true).size () + all_joints[linked_joints[i]](m_f_next, true).size ());
                    merged_polyline_1.reserve ((*this) (m_f_curr, false).size () + all_joints[linked_joints[i]](m_f_next, false).size ());

                    // begin shift -> start_curr, assumed that there is the same shift from two ends
                    merged_polyline_0.insert (merged_polyline_0.end (), (*this) (m_f_curr, true)[0].begin (), (*this) (m_f_curr, true)[0].begin () + start_curr);
                    merged_polyline_1.insert (merged_polyline_1.end (), (*this) (m_f_curr, false)[0].begin (), (*this) (m_f_curr, false)[0].begin () + start_curr);

                    for (int k = start_curr, it = 0; k < (*this) (m_f_curr, true)[j].size () - start_curr; k += step_curr,
                             it++) // start+step*i | this loops assumes that start and end are the same
                        {
                            // current 1st half
                            merged_polyline_0.insert (merged_polyline_0.end (), (*this) (m_f_curr, true)[0].begin () + start_curr + it * step_curr,
                                                      (*this) (m_f_curr, true)[0].begin () + start_curr + (it + 1) * step_curr - step_curr * 0.5);
                            merged_polyline_1.insert (merged_polyline_1.end (), (*this) (m_f_curr, false)[0].begin () + start_curr + it * step_curr,
                                                      (*this) (m_f_curr, false)[0].begin () + start_curr + (it + 1) * step_curr - step_curr * 0.5);

                            // next link insertion
                            merged_polyline_0.insert (merged_polyline_0.end (), all_joints[linked_joints[i]](m_f_next, true)[0].begin () + start_next + it * step_next,
                                                      all_joints[linked_joints[i]](m_f_next, true)[0].begin () + start_next + (it + 1) * step_next);
                            merged_polyline_1.insert (merged_polyline_1.end (), all_joints[linked_joints[i]](m_f_next, false)[0].begin () + start_next + it * step_next,
                                                      all_joints[linked_joints[i]](m_f_next, false)[0].begin () + start_next + (it + 1) * step_next);

                            // current 2nd half
                            merged_polyline_0.insert (merged_polyline_0.end (), (*this) (m_f_curr, true)[0].begin () + start_curr + (it + 1) * step_curr - step_curr * 0.5,
                                                      (*this) (m_f_curr, true)[0].begin () + start_curr + (it + 1) * step_curr);
                            merged_polyline_1.insert (merged_polyline_1.end (), (*this) (m_f_curr, false)[0].begin () + start_curr + (it + 1) * step_curr - step_curr * 0.5,
                                                      (*this) (m_f_curr, false)[0].begin () + start_curr + (it + 1) * step_curr);
                        }

                    // end shift -> start_curr, assumed that there is the same shift from two ends
                    merged_polyline_0.insert (merged_polyline_0.end (), (*this) (m_f_curr, true)[0].end () - start_curr, (*this) (m_f_curr, true)[0].end ());
                    merged_polyline_1.insert (merged_polyline_1.end (), (*this) (m_f_curr, false)[0].end () - start_curr, (*this) (m_f_curr, false)[0].end ());

                    //  replace current joint geometry with the merged one
                    (*this) (m_f_curr, true)[0] = merged_polyline_0;
                    (*this) (m_f_curr, false)[0] = merged_polyline_1;
                }

            // remove geometry from linked joint using "v_remove_from_link" parameter
            all_joints[linked_joints[i]](m_f_next, true).clear ();
            all_joints[linked_joints[i]](m_f_next, false).clear ();
        }
}
} // namespace wood