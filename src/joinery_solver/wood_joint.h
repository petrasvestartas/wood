#pragma once

#include "../src/element.h"
#include "../src/line.h"
#include "../src/point.h"
#include "../src/polyline.h"
#include "wood_element_plate.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════

/// Topology class of a face contact from the two face indices alone (index < 2 outer, >= 2 side).
/// Not WoodJoint::joint_type: that is the solver's refined code (11/12/13/20/30/40) and needs geometry.
enum class ContactType : int {
    unknown   = -1,  ///< no plate face convention - every BlockElement contact
    side_side = 0,   ///< both faces are sides     (refines to 11 / 12 / 13)
    side_top  = 1,   ///< one side, one outer face (refines to 20)
    top_top   = 2,   ///< both outer faces         (refines to 40)
    cross     = 3,   ///< elements pass through each other - plane_to_face / CrossJoint
    line      = 4,   ///< two elements' boundary polylines cross within tolerance
};

/// One face pair in contact: the two faces, the class, the overlap region (closed, in the first face's plane).
struct FaceContact {
    int face_a = 0;
    int face_b = 0;
    ContactType type = ContactType::unknown;
    session_cpp::Polyline area;

    nlohmann::ordered_json jsondump() const;
    static FaceContact jsonload(const nlohmann::json& data);
};

/// face_contacts() output: one element pair as positions in the vector it was given, and every overlap between them.
struct ContactPair {
    int element_a = -1;
    int element_b = -1;
    std::vector<FaceContact> faces;
};

struct WoodJoint {
    WoodJoint();

    /// The two elements, by guid - a male, b female; swapped by the solver, so not ordered. index_of() gives a position.
    std::string element_a;
    std::string element_b;
    /// Which faces touched, and where.
    FaceContact contact;
    /// Type-30 (cross) joints only: the second side face of each element in the crossing; {-1,-1} otherwise.
    std::array<int, 2> cross_faces{-1, -1};
    /// Refined solver code: 11/12/13 side-side, 20 top-side, 30 cross, 40 top-top.
    int joint_type;
    std::string name;
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

    /// The joint as each host element carries it: [0] male (element_a, face_a), [1] female; bodies only current after sync_features().
    std::array<session_cpp::ElementFeature, 2> element_features;
    /// Identity of the two sides, minted on first read; kept here because an ElementFeature copy drops its guid.
    mutable std::array<std::string, 2> feature_guids;
    const std::string& feature_guid(int side) const;
    void sync_features();
    /// sync_features() applied to copies: identity preserved, the joint itself untouched.
    std::array<session_cpp::ElementFeature, 2> to_features() const;

    /// The whole joint, solver fields included; the two features travel as their guids alone.
    nlohmann::ordered_json jsondump() const;
    static WoodJoint jsonload(const nlohmann::json& data);
    std::string file_json_dumps() const;
    static WoodJoint file_json_loads(const std::string& json_string);
    void file_json_dump(const std::string& filename) const;
    static WoodJoint file_json_load(const std::string& filename);

    std::string str() const;
    friend std::ostream& operator<<(std::ostream& os, const WoodJoint& j);
};

// ═══════════════════════════════════════════════════════════════════════════
// Joint construction
// ═══════════════════════════════════════════════════════════════════════════

void apply_unit_scale(WoodJoint& joint);
void joint_orient_to_connection_area(WoodJoint& joint);
void merge_linked_joints(WoodJoint& joint, std::vector<WoodJoint>& all_joints);
void joint_get_divisions(WoodJoint& joint, double division_distance);

void side_removal_ss_e_r_1_port(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);
void side_removal(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements, bool merge_with_joint = false);

void tt_e_p_0(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);
void tt_e_p_1(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);
void tt_e_p_2(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);
void tt_e_p_3(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);
void tt_e_p_4(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);
void tt_e_p_5(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);

/// The [width, height, length] extension for a joint type: side-side (11/12/13) reads triple 0, top-side (20) triple 1, top-top (40) triple 2, cross (30) triple 3; a 3-entry list serves every type.
std::array<double, 3> joint_volume_extension(const std::vector<double>& extension, int joint_type);

/// Position of the plate with this guid, or -1.
int index_of(const std::vector<std::shared_ptr<Plate>>& elements, const std::string& guid);

} // namespace wood_session
