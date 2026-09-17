#pragma once

#include "wood_pch.h"

#include "wood_element_plate.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════

/// Topology class of a face contact from the two face indices alone (index < 2 outer, >= 2 side).
/// Not WoodJoint::joint_type: that is the solver's refined code (11/12/13/20/30/40) and needs geometry.
enum class ContactType : int {
    /// No plate face convention: every Block contact.
    unknown = -1,

    /// Both faces are sides; refines to 11, 12 or 13.
    side_side = 0,

    /// One side face and one outer face; refines to 20.
    side_top = 1,

    /// Both outer faces; refines to 40.
    top_top = 2,

    /// The elements pass through each other: plane_to_face, CrossJoint.
    cross = 3,

    /// The two boundary polylines cross within tolerance.
    line = 4,
};

/// One face pair in contact: the two faces, the class, the overlap region (closed, in the first face's plane).
struct FaceContact {
    /// Face index on the first element.
    int face_a = 0;

    /// Face index on the second element.
    int face_b = 0;

    /// Topology class of the pair.
    ContactType type = ContactType::unknown;

    /// The overlap region, closed, in face_a's plane.
    session_cpp::Polyline area;

    /// The contact as JSON: area, face_a, face_b, type.
    nlohmann::ordered_json jsondump() const;

    /// A contact from its JSON.
    static FaceContact jsonload(const nlohmann::json& data);
};

/// face_contacts() output: one element pair as positions in the vector it was given, and every overlap between them.
struct ContactPair {
    /// Position of the first element.
    int element_a = -1;

    /// Position of the second element.
    int element_b = -1;

    /// Every face pair in contact between the two.
    std::vector<FaceContact> faces;
};

/// One connection between two plates: what was detected, how it was classified, and the cut outlines the joint library made of it.
struct WoodJoint {
    /// An empty joint: type 0, one division, shift 0.5, unit scale off.
    WoodJoint();

    /// The male element, by guid; swapped with element_b by the solver, so not ordered. index_of() gives a position.
    std::string element_a;

    /// The female element, by guid.
    std::string element_b;

    /// Which faces touched, and where.
    FaceContact contact;

    /// Cross joints only: the second side face of each element in the crossing; {-1, -1} otherwise.
    std::array<int, 2> cross_faces{-1, -1};

    /// Refined solver code: 11/12/13 side-side, 20 top-side, 30 cross, 40 top-top.
    int joint_type;

    /// The joint library variant that built the outlines ("ss_e_ip_2", "side_removal"), empty before construction.
    std::string name;

    /// The two alignment lines, one per element, along the shared edge.
    std::array<session_cpp::Line, 2> joint_lines;

    /// The volume rectangles: [0] and [1] bound the male side, [2] and [3] the female side when it differs.
    std::array<std::optional<session_cpp::Polyline>, 4> joint_volumes_pair_a_pair_b;

    /// Male cut outlines per face, [0] bottom and [1] top; the last entry of each face is a 2-point endpoint marker.
    std::array<std::vector<session_cpp::Polyline>, 2> m_outlines;

    /// Female cut outlines per face, laid out like m_outlines.
    std::array<std::vector<session_cpp::Polyline>, 2> f_outlines;

    /// One cut_type per male outline.
    std::array<std::vector<int>, 2> m_cut_types;

    /// One cut_type per female outline.
    std::array<std::vector<int>, 2> f_cut_types;

    /// Number of teeth or notches along the joint line.
    int divisions;

    /// Lateral offset of the pattern along the joint line, 0..1.
    double shift;

    /// Length of the joint line.
    double length;

    /// Spacing between divisions along the joint line.
    double division_length;

    /// Multiplicative scale of the unit-box geometry, x, y, z.
    std::array<double, 3> scale;

    /// True when the variant pins its axial size to unit_scale_distance.
    bool unit_scale;

    /// The axial size a unit-scale variant is pinned to; 0 reads it off the volume rectangle.
    double unit_scale_distance;

    /// Indices of the joints this one is linked with (three-valence).
    std::vector<int> linked_joints;

    /// Per linked joint, the vertex ranges merge_linked_joints interleaves.
    std::vector<std::vector<std::array<int, 4>>> linked_joints_seq;

    /// True when this joint is the link of a three-valence group.
    bool link;

    /// True when the outlines are already in world space and must not be oriented.
    bool no_orient;

    /// Face pairs that passed the coplanarity test in detection.
    int dbg_coplanar;

    /// Face pairs with a real overlap area in detection.
    int dbg_boolean;

    /// Why detection rejected the pair, filled only under TRACE.
    std::string dbg_fail_reason;

    /// The joint as each host element carries it: [0] male (element_a, face_a), [1] female; bodies current only after sync_features().
    std::array<session_cpp::ElementFeature, 2> element_features;

    /// Identity of the two sides, minted on first read; kept here because an ElementFeature copy drops its guid.
    mutable std::array<std::string, 2> feature_guids;

    /// The guid of one side, minted on first read.
    const std::string& feature_guid(int side) const;

    /// Rebuilds element_features from the solver fields.
    void sync_features();

    /// sync_features() applied to copies: identity preserved, the joint itself untouched.
    std::array<session_cpp::ElementFeature, 2> to_features() const;

    /// The whole joint, solver fields included; the two features travel as their guids alone.
    nlohmann::ordered_json jsondump() const;

    /// A joint from its JSON, features re-derived from the solver fields.
    static WoodJoint jsonload(const nlohmann::json& data);

    /// jsondump() as a string.
    std::string file_json_dumps() const;

    /// jsonload() from a string.
    static WoodJoint file_json_loads(const std::string& json_string);

    /// jsondump() to a file.
    void file_json_dump(const std::string& filename) const;

    /// jsonload() from a file.
    static WoodJoint file_json_load(const std::string& filename);

    /// "WoodJoint(type, elements, faces, name)".
    std::string str() const;

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const WoodJoint& j);
};

// ═══════════════════════════════════════════════════════════════════════════
// Joint construction
// ═══════════════════════════════════════════════════════════════════════════

/// Moves the volume rectangles of a unit-scale joint to unit_scale_distance apart along the joint line.
void apply_unit_scale(WoodJoint& joint);

/// Maps the unit-box outlines onto the joint volumes by change of basis, after apply_unit_scale.
void joint_orient_to_connection_area(WoodJoint& joint);

/// Interleaves the outlines of the joints in linked_joints into this one, following linked_joints_seq.
void merge_linked_joints(WoodJoint& joint, std::vector<WoodJoint>& all_joints);

/// Sets divisions from the joint length and division_distance, at least one.
void joint_get_divisions(WoodJoint& joint, double division_distance);

/// Side removal (id 58): four side-face rectangles widened at convex corners and pushed along the face normals; no orient.
void side_removal_ss_e_r_1_port(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);

/// side_removal_ss_e_r_1_port with the merge branch forced off unless merge_with_joint (ids 8, 28, 38, 57).
void side_removal(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements, bool merge_with_joint = false);

/// Top-to-top: a single drill at the area centroid (id 40).
void tt_e_p_0(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);

/// Top-to-top: a single drill at the visual centre, approximated by the centroid (id 41).
void tt_e_p_1(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);

/// Top-to-top: division_length drills on a circle of radius shift around the centroid (id 42).
void tt_e_p_2(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);

/// Top-to-top: drills along the offset area boundary, open rings by DISTANCE_SQUARED (id 43).
void tt_e_p_3(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);

/// Top-to-top: drills along the offset area boundary, open rings by a fixed 0.01 (id 44).
void tt_e_p_4(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);

/// Top-to-top: drills along the offset area boundary with the division length taken absolute (id 45).
void tt_e_p_5(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements);

/// The [width, height, length] extension for a joint type: side-side (11/12/13) reads triple 0, top-side (20) triple 1, top-top (40) triple 2, cross (30) triple 3; a 3-entry list serves every type.
std::array<double, 3> joint_volume_extension(const std::vector<double>& extension, int joint_type);

/// Position of the plate with this guid, or -1.
int index_of(const std::vector<std::shared_ptr<Plate>>& elements, const std::string& guid);

} // namespace wood_session
