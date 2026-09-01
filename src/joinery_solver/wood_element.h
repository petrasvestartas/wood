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

/// A minimal element for CONTACT DETECTION only: closed outlines plus one
/// plane per outline, and nothing else.
///
/// WoodElement carries the plate convention the joint classifier depends on -
/// polylines[0] is the top face, [1] the bottom, [2..] the sides, in that
/// order, plus thickness, insertion vectors and merged features. That
/// convention is exactly what loose geometry does NOT have: a list of closed
/// loops off a brep says nothing about which loop is which.
///
/// BlockElement drops all of it. It is enough for adjacency_search,
/// faces_coplanar and face_overlap_area - which only ever read `polylines` and
/// `planes` - and deliberately not enough for face_to_face_wood, which needs
/// the ordering to tell a side joint from a top joint.
struct BlockElement {
    BlockElement();

    /// One plane per loop: origin at the loop centroid, normal from
    /// Vector::average_normal (Newell). Loops with fewer than 3 points are
    /// dropped, matching WoodElement's degrade-rather-than-throw behaviour.
    explicit BlockElement(const std::vector<session_cpp::Polyline>& loops);

    std::vector<session_cpp::Polyline> polylines;
    std::vector<session_cpp::Plane>    planes;

    std::string str() const;
};

} // namespace wood_session
