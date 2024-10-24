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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef WOOD_GLOBALS_H
#define WOOD_GLOBALS_H

// #ifdef WOOD_WRAPPER
// #include "../../../src/compas_wood/include/stdafx_pybind11.h" //go up to the folder where the
// CMakeLists.txt is #endif

// Preprocessor stage
#define ON_IS_FINITE(x) (0x7FF0 != (*((unsigned short *)(&x) + 3) & 0x7FF0))
#define ON_DBL_MIN 2.22507385850720200e-308
#define ON_EPSILON 2.2204460492503131e-16
#define ON_SQRT_EPSILON 1.490116119385000000e-8
#define ON_ZERO_TOLERANCE 2.3283064365386962890625e-10
#define ON_DBL_MAX 1.7976931348623158e+308

namespace wood
{
// Compilation stage
struct GLOBALS
{
  public:
    // Clipper2 library mostly used in collider::clipper_util
    static int64_t CLIPPER_SCALE;
    static double CLIPPER_AREA; // default is 0.0001 but the tolerance is increased by purpose

    // Tolerances for distance search
    static double DISTANCE;         // default is 0.01 but the tolerance is increased by purpose
    static double DISTANCE_SQUARED; // default is 0.0001 but the tolerance is increased by purpose
    static double ANGLE;            // default is 0.01 but the tolerance is increased by purpose

    // File names
    static std::string PATH_AND_FILE_FOR_JOINTS;
    static std::string DATA_SET_INPUT_FOLDER;
    static std::string DATA_SET_OUTPUT_FILE;
    static std::string DATA_SET_OUTPUT_DATABASE;

    // Wood library
    static std::vector<double> JOINT_VOLUME_EXTENSION;

    static int OUTPUT_GEOMETRY_TYPE;
    static bool FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED;
    static bool FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE;
    static double FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE;
    static double LIMIT_MIN_JOINT_LENGTH;

    static std::vector<std::string> EXISTING_TYPES;

    static std::vector<double> JOINTS_PARAMETERS_AND_TYPES;

    // custom joint types
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_SS_E_IP_MALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_SS_E_IP_FEMALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_SS_E_OP_MALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_SS_E_OP_FEMALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_TS_E_P_MALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_TS_E_P_FEMALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_CR_C_IP_MALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_CR_C_IP_FEMALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_TT_E_P_MALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_TT_E_P_FEMALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_SS_E_R_MALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_SS_E_R_FEMALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_B_MALE;
    static std::vector<CGAL_Polyline> CUSTOM_JOINTS_B_FEMALE;

    // IMGUI
    static size_t RUN_COUNT;
};

} // namespace wood

#endif