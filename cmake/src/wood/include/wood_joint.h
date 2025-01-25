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
// None
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef WOOD_JOINT_H
#define WOOD_JOINT_H

#include "wood_cut.h"

namespace wood
{
// Container for cuts
class joint
{
    /**
     * Changes the basis of a joint geometry from one coordinate system to another.
     * This is particularly useful for aligning joint geometries in 3D space.
     *
     * @param rect0 The first rectangle defining the initial coordinate system.
     * @param rect1 The second rectangle defining the initial coordinate system, typically
     * perpendicular to the first.
     * @param xform The transformation object to be filled with the change of basis transformation.
     * @return True if the basis change was successful, false otherwise.
     */
    bool change_basis (CGAL_Polyline &rect0, CGAL_Polyline &rect1,
                       CGAL::Aff_transformation_3<IK> &xform); // first get 2x change_basis transformation matrices

  public:
    /////////////////////////////////////////////////////////////////////////////////////////
    // Parameters from Search method v-volume f-face
    /////////////////////////////////////////////////////////////////////////////////////////
    int id = 0, v0, v1, f0_0, f1_0, f0_1, f1_1,
        type; // 10 - SS Rotate 11 - SS OUT OF PLANE 12 - SS IN Plane,  20 Top-Side, 30 - Cross
    std::string key = "";
    CGAL_Polyline joint_area;     // delete
    CGAL_Polyline joint_lines[2]; // delete
    // CGAL_Polyline joint_quads[2];//delete
    CGAL_Polyline joint_volumes[4]; // mostly 2, but can be 4 e.g. in-plane side-side

    /////////////////////////////////////////////////////////////////////////////////////////
    // Detailed parameters for geometry transfer from library or custom made
    /////////////////////////////////////////////////////////////////////////////////////////
    std::string name = "";
    int id_of_global_joint_list = -1;    // Directs which joint applies where, -1 all cases
    std::vector<double> tile_parameters; // For rebuilding

    std::array<std::vector<CGAL_Polyline>, 2> m;
    std::vector<wood::cut::cut_type> m_boolean_type; // 0 - do not merge, 1 - edge insertion, 2 -
                                                     // hole 3 - insert between multiple edges hole

    std::array<std::vector<CGAL_Polyline>, 2> f;
    std::vector<wood::cut::cut_type> f_boolean_type; // 0 - do not merge, 1 - edge insertion, 2 -
                                                     // hole 3 - insert between multiple edges hole

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Joint geometrical parameters for
    // a) enable orientation
    // b) enable unit scale
    // c) parametric joint changes
    // d) scale of the joint volume
    // e) computed parameters for computed the key, while creating the cache of precomputed joints
    // and constucting initial joints
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Unit scale
    // if this property is enabled, joint volume rectangles are moved within unit_scale_distance,
    // this property is equal to first element thickness
    bool unit_scale = false;
    double unit_scale_distance = 0;

    // Enable orientation or the joint is compute in place e.g. screws are computed in place, while
    // other are pre-computed and oriented
    bool orient = true;

    // User parameter
    double division_length = 10;
    double shift = 0.5;

    // scale of the joint volume
    std::array<double, 3> scale = { 1, 1, 1 };

    // computed parameters
    int divisions = 1;
    double length = 1;
    bool compute_geometrical_divisions = true;

    /////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    /////////////////////////////////////////////////////////////////////////////////////////
    /**
     * Default constructor for the joint class.
     */
    joint ();

    /**
     * Constructs a joint object with basic geometric information.
     *
     * @param _id Unique identifier for the joint.
     * @param _v0 First vertex index of the joint.
     * @param _v1 Second vertex index of the joint.
     * @param _f0_0 First face index associated with the first vertex.
     * @param _f1_0 Second face index associated with the first vertex.
     * @param _f0_1 First face index associated with the second vertex.
     * @param _f1_1 Second face index associated with the second vertex.
     * @param _joint_volumes Array of 4 CGAL_Polyline objects representing the volumes of the joint.
     */
    joint (int, int, int, int, int, int, int, std::array<CGAL_Polyline, 4> &);

    /**
     * Constructs a joint object with extended geometric information including joint area and lines.
     *
     * @param _id Unique identifier for the joint.
     * @param _v0 First vertex index of the joint.
     * @param _v1 Second vertex index of the joint.
     * @param _f0_0 First face index associated with the first vertex.
     * @param _f1_0 Second face index associated with the first vertex.
     * @param _f0_1 First face index associated with the second vertex.
     * @param _f1_1 Second face index associated with the second vertex.
     * @param _joint_area CGAL_Polyline object representing the area of the joint.
     * @param _joint_lines Array of 2 CGAL_Polyline objects representing the lines of the joint.
     * @param _joint_volumes Array of 4 CGAL_Polyline objects representing the volumes of the joint.
     * @param _type Integer representing the type of the joint.
     */
    joint (int, int, int, int, int, int, int, CGAL_Polyline (&), std::array<CGAL_Polyline, 2> &, std::array<CGAL_Polyline, 4> &, int);

    // Operators
    /**
     * Overloaded function call operator to access specific joint polyline vectors based on gender
     * and order.
     *
     * @param male_or_female Boolean flag indicating male (true) or female (false) joint.
     * @param first_or_second Boolean flag indicating first (true) or second (false) joint.
     * @return Reference to a vector of CGAL_Polyline objects representing the joint's geometry.
     */
    std::vector<CGAL_Polyline> &operator() (bool male_or_female, bool first_or_second);

    /**
     * Retrieves the edge IDs associated with the male or female part of the joint.
     *
     * @param male_or_female Boolean flag indicating male (true) or female (false) joint.
     * @param fA Reference to an integer to store the first face index.
     * @param fB Reference to an integer to store the second face index.
     */
    void get_edge_ids (bool male_or_female, int &fA, int &fB);

    /**
     * Retrieves the first cutting type for the specified gender part of the joint.
     *
     * @param male_or_female Boolean flag indicating male (true) or female (false) joint.
     * @return Reference to the cut_type enum representing the cutting type for the first joint
     * part.
     */
    wood::cut::cut_type &get_first_cutting_type (bool male_or_female);

    /**
     * Overloaded function call operator to access cutting types based on gender.
     *
     * @param male_or_female Boolean flag indicating male (true) or female (false) joint.
     * @return Reference to a vector of cut_type enums representing the cutting types for the
     * specified gender.
     */
    std::vector<wood::cut::cut_type> &operator() (bool male_or_female);

    /**
     * Reverses the order of the polylines for the specified gender part of the joint.
     *
     * @param male_or_female Boolean flag indicating male (true) or female (false) joint.
     */
    void reverse (bool male_or_female);

    /**
     * Generates a unique key for the joint based on its name, shift, and divisions properties.
     * The shift and divisions values are formatted to a precision of 2 decimal places.
     *
     * @return A string representing the unique key of the joint.
     */
    std::string get_key ();

    /**
     * Applies transformation to the geometries of both male and female parts of the joint.
     *
     * @param xform0 Transformation to be applied to the male part of the joint.
     * @param xform1 Transformation to be applied to the female part of the joint.
     */
    void transform (CGAL::Aff_transformation_3<IK> &xform0,
                    CGAL::Aff_transformation_3<IK> &xform1); // Custom user transformation

    /**
     * Orients the joint to a specific connection area, adjusting for scale and position.
     * This involves moving and scaling the joint's geometry to align with the connection area.
     *
     * @return True if the orientation and scaling were successful, false otherwise.
     */
    bool orient_to_connection_area (); // Orient to connection area if rectangles are set

    /**
     * Duplicates the geometric information from the current joint object to a new joint object.
     * This method is intended for duplicating geometric data without adjacency information
     * to reduce the time required for object creation.
     *
     * @param new_joint Reference to the joint object where the geometric information will be
     * duplicated.
     */
    void duplicate_geometry (joint &);

    /**
     * Transfers geometric information from another joint object to this joint object.
     * This method is intended for updating the current joint's geometric data based on another
     * joint's geometry.
     *
     * @param geo_joint The joint object from which geometric information will be transferred.
     */
    void transfer_geometry (joint &);

    /**
     * Calculates the number of divisions for the joint based on a given division distance and the
     * joint's length. The method updates the 'divisions' property of the joint object based on the
     * computed value.
     *
     * @param division_distance The distance between each division.
     */
    void get_divisions (double &division_distance);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // joint linking with other joints
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool link = false;                                                // assigned in three_valence_joint_addition_vidy for the link joint
    std::vector<int> linked_joints;                                   // assigned in three_valence_joint_addition_vidy for the main joint
    std::vector<std::vector<std::array<int, 4> > > linked_joints_seq; // assigned on wood::joint_library | it is nested because there can be
                                                                      // umber of polylines | example {start_curr,step_curr} means that
                                                                      // "start_curr+step_curr*i" and {start_link,step_link} ->
                                                                      // "start_link+step_link*i"

    /**
     * Merges geometric information from linked joints into the current joint and then removes the
     * geometric data from the linked joints. This method is designed to consolidate geometric data
     * into a single joint object to simplify the representation of linked joints.
     *
     * @param all_joints A reference to a vector containing all joint objects, including the current
     * and linked joints.
     */
    void remove_geo_from_linked_joint_and_merge_with_current_joint (std::vector<joint> &all_joints); // it is called if linked_joints vector is not empty | also check
                                                                                                     // wood_joint -> joint linking with other joints
};
} // namespace wood
#endif
