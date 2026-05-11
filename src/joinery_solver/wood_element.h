#pragma once

#include <array>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../src/line.h"
#include "../src/mesh.h"
#include "../src/plane.h"
#include "../src/point.h"
#include "../src/polyline.h"
#include "../src/vector.h"

namespace wood_session {

struct WoodJoint {
    WoodJoint();

    std::pair<int, int> el_ids;
    std::pair<std::array<int, 2>, std::array<int, 2>> face_ids;
    int joint_type;
    std::string name;
    session_cpp::Polyline joint_area;
    std::array<session_cpp::Line, 2> joint_lines;
    std::array<std::optional<session_cpp::Polyline>, 4> joint_volumes_pair_a_pair_b;
    std::array<std::vector<session_cpp::Polyline>, 2> m_outlines;
    std::array<std::vector<session_cpp::Polyline>, 2> f_outlines;
    std::array<std::vector<int>, 2> m_cut_types;
    std::array<std::vector<int>, 2> f_cut_types;
    int divisions;
    double shift;
    double length;
    double division_length;
    std::array<double, 3> scale;
    bool unit_scale;
    double unit_scale_distance;
    std::vector<int> linked_joints;
    std::vector<std::vector<std::array<int, 4>>> linked_joints_seq;
    bool link;
    bool no_orient;
    int dbg_coplanar;
    int dbg_boolean;
    std::string dbg_fail_reason;
};

struct Features {
    std::vector<session_cpp::Polyline> top;
    std::vector<session_cpp::Polyline> bottom;
};

struct WoodElement {
    WoodElement();
    WoodElement(const session_cpp::Polyline& bot, const session_cpp::Polyline& top);

    std::vector<session_cpp::Polyline> polylines;
    std::vector<session_cpp::Plane>    planes;
    std::vector<session_cpp::Vector>   insertion_vectors;
    std::vector<int>                   joint_types;   // per-face codes; empty = auto
    bool reversed;
    double thickness;
    Features features;

    /// Loft polylines[0] (bottom) and polylines[1] (top) into a solid plate mesh.
    session_cpp::Mesh loft_mesh() const;

    std::string str() const;
    std::string repr() const;
    friend std::ostream& operator<<(std::ostream& os, const WoodElement& e);
};

} // namespace wood_session
