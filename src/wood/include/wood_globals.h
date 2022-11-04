#ifndef WOOD_GLOBALS_H
#define WOOD_GLOBALS_H

namespace wood_globals
{
    extern int output_geometry_type;
    extern bool force_side_to_side_joints_to_be_rotated;
    extern bool run;
        extern double joint_line_extension;
    extern std::array<std::string, 7> joint_names;
    extern std::vector<std::string> existing_types;
    extern std::vector<double> joint_types;

}

#endif