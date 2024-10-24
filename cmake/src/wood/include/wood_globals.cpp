
#include "wood_globals.h"

#include "../../../stdafx.h" //go up to the folder where the CMakeLists.txt is

namespace wood
{

// Clipper2 library mostly used in collider::clipper_util
int64_t GLOBALS::CLIPPER_SCALE = 1000000;
double GLOBALS::CLIPPER_AREA = 0.01; // default is 0.0001 but the tolerance is increased by purpose

// Tolerances for distance search
double GLOBALS::DISTANCE = 0.1;          // default is 0.01 but the tolerance is increased by purpose
double GLOBALS::DISTANCE_SQUARED = 0.01; // default is 0.0001 but the tolerance is increased by purpose
double GLOBALS::ANGLE = 0.11;            // default is 0.01 but the tolerance is increased by purpose

// File names
std::string GLOBALS::PATH_AND_FILE_FOR_JOINTS = "";
std::string GLOBALS::DATA_SET_INPUT_FOLDER = "C:\\IBOIS57\\_Code\\Software\\Python\\compas_wood\\backend\\src\\wood\\dataset\\";
std::string GLOBALS::DATA_SET_OUTPUT_FILE = GLOBALS::DATA_SET_INPUT_FOLDER + "out.xml";
std::string GLOBALS::DATA_SET_OUTPUT_DATABASE = GLOBALS::DATA_SET_INPUT_FOLDER + "out.db";

// Wood library
std::vector<double> GLOBALS::JOINT_VOLUME_EXTENSION = { // this property is used within local searches: plane_to_face and face_to_face
    0.0,                                                // x
    0.0,                                                // y
    0.0,                                                // z
    0, 0
};

int GLOBALS::OUTPUT_GEOMETRY_TYPE = 4;
bool GLOBALS::FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED = false;
bool GLOBALS::FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE = false;
double GLOBALS::FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE = 150.0;
double GLOBALS::LIMIT_MIN_JOINT_LENGTH = 0;

std::vector<std::string> GLOBALS::EXISTING_TYPES = {
    "JOINT_NAMES[1] = ss_e_ip_0;",     "JOINT_NAMES[2] = ss_e_ip_1;",  "JOINT_NAMES[3] = ss_e_ip_2;",     "JOINT_NAMES[8] = side_removal;",
    "JOINT_NAMES[9] = ss_e_ip_9;",     "JOINT_NAMES[10] = ss_e_op_0;", "JOINT_NAMES[11] = ss_e_op_1;",    "JOINT_NAMES[12] = ss_e_op_2;",
    "JOINT_NAMES[13] = ss_e_op_3;",    "JOINT_NAMES[14] = ss_e_op_4;", "JOINT_NAMES[15] = ss_e_op_5;",    "JOINT_NAMES[18] = side_removal;",
    "JOINT_NAMES[19] = ss_e_op_9;",    "JOINT_NAMES[20] = ts_e_p_0;",  "JOINT_NAMES[21] = ts_e_p_1;",     "JOINT_NAMES[22] = ts_e_p_2;",
    "JOINT_NAMES[23] = ts_e_p_3;",     "JOINT_NAMES[24] = ts_e_p_4;",  "JOINT_NAMES[25] = ts_e_p_5;",     "JOINT_NAMES[28] = side_removal;",
    "JOINT_NAMES[29] = ts_e_p_9;",     "JOINT_NAMES[30] = cr_c_ip_0;", "JOINT_NAMES[31] = cr_c_ip_1;",    "JOINT_NAMES[32] = cr_c_ip_2;",
    "JOINT_NAMES[38] = side_removal;", "JOINT_NAMES[39] = cr_c_ip_9;", "JOINT_NAMES[48] = side_removal;", "JOINT_NAMES[58] = side_removal_ss_e_r_1;",
    "JOINT_NAMES[59] = ss_e_r_9;",     "JOINT_NAMES[60] = b_0;",
};

std::vector<double> GLOBALS::JOINTS_PARAMETERS_AND_TYPES{
    300, 0.5,  3,  // 1-9 ss_e_ip (side-to-side edge in-plane)
    450, 0.64, 15, // 10-19 ss_e_op (side-to-side edge out-of-plane)
    450, 0.5,  20, // 20-29 ts_e_p (top-to-side edge plane)
    300, 0.5,  30, // 30-39 cr_c_ip (cross cutting in-plane)
    6,   0.95, 40, // 40-49 tt_e_p  (top-to-top edge plane)
    300, 0.5,  58, // 50-59 ss_e_r (side-to-side edge rotated)
    300, 1.0,  60  // 60-69 b (boundary)
};

// custom joint types
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_SS_E_IP_MALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_SS_E_IP_FEMALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_SS_E_OP_MALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_SS_E_OP_FEMALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_TS_E_P_MALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_TS_E_P_FEMALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_CR_C_IP_MALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_CR_C_IP_FEMALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_TT_E_P_MALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_TT_E_P_FEMALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_SS_E_R_MALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_SS_E_R_FEMALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_B_MALE{};
std::vector<CGAL_Polyline> GLOBALS::CUSTOM_JOINTS_B_FEMALE{};

// IMGUI
size_t GLOBALS::RUN_COUNT = 0;

} // namespace wood