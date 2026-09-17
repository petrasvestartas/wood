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

/// Topology class of a face contact, derived from the two face indices alone.
/// Available on any element type, because it needs no geometry beyond the plate
/// face convention (index < 2 = outer face, >= 2 = side face).
///
/// NOT the same vocabulary as WoodJoint::joint_type. That one is the refined
/// solver code (11/12/13/20/30/40) and needs plate geometry - dihedral angle,
/// alignment chords, thickness - to compute. ContactType is what a contact can
/// say about itself; joint_type is what the solver decided afterwards. The two
/// spaces do not even agree numerically: ContactType::side_top is 1, the
/// top-to-side joint code is 20.
enum class ContactType : int {
    unknown   = -1,  ///< no plate face convention - every BlockElement contact
    side_side = 0,   ///< both faces are sides     (refines to 11 / 12 / 13)
    side_top  = 1,   ///< one side, one outer face (refines to 20)
    top_top   = 2,   ///< both outer faces         (refines to 40)
    cross     = 3,   ///< elements pass through each other - plane_to_face / CrossJoint
    line      = 4,   ///< two elements' boundary polylines cross within tolerance
};

/// One face pair in contact: the two faces, the class, the overlap region (closed, in the
/// first face's plane). Which ELEMENTS is the graph edge this rides on, not this.
///
/// Lives here rather than in wood_face_to_face.h because WoodJoint embeds one
/// by value, and that header includes wood_session.h, which includes this one.
struct FaceContact {
    int face_a = 0;
    int face_b = 0;
    ContactType type = ContactType::unknown;
    session_cpp::Polyline area{std::vector<session_cpp::Point>{}};

    nlohmann::ordered_json jsondump() const;
    static FaceContact jsonload(const nlohmann::json& data);
};

/// face_contacts() output: one element pair as positions in the vector it was given, and
/// every overlap polygon between them. A scene turns this into one graph edge.
struct ContactPair {
    int element_a = -1;
    int element_b = -1;
    std::vector<FaceContact> faces;
};

struct WoodJoint {
    WoodJoint();

    /// The two elements, by guid - a male, b female; swapped by the solver, so not ordered.
    /// index_of() turns one into a position.
    std::string element_a;
    std::string element_b;
    /// Which faces touched, and where.
    FaceContact contact;
    /// Type-30 (cross) joints only: the SECOND side face of each element that
    /// the crossing involves, from CrossJoint::face_ids_a/.face_ids_b. Every
    /// other joint has one face per element and leaves this at {-1,-1}.
    std::array<int, 2> cross_faces{-1, -1};
    /// Refined solver code: 11/12/13 side-side, 20 top-side, 30 cross, 40
    /// top-top. See ContactType above - a different vocabulary, not this one.
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

    // ── Kernel view, by composition ────────────────────────────────────────
    //
    // The joint as each of its two host elements carries it: [0] is the male side
    // (element_a, detected on face contact.face_a), [1] the female side
    // (element_b, contact.face_b). Identity lives here - element_features[k].guid()
    // is the handle a Session consumer uses to name this side of the joint again. Copying
    // an ElementFeature mints a fresh guid, so copying a joint copies its geometry, not its
    // identity, exactly as the kernel does.
    //
    // feature_type is "joint", name is the joint-library variant when the solver set one
    // ("tt_e_p_3", "side_removal") and "joint_<type>" otherwise, face_index is the face the
    // contact was detected on, and outlines are that side's cut outlines on both plate faces
    // (m_outlines / f_outlines, face 0 then face 1, flattened - ElementFeature has one list).
    // The solver keeps its two-face split because the merge stage needs it; the feature is
    // the shape every other consumer reads. get_connection_zones syncs these before it
    // returns, so a joint it hands back is always current.
    std::array<session_cpp::ElementFeature, 2> element_features;
    /// The identity of the two sides, one guid each, minted on first read. Plain strings and
    /// not the features' own guids, because an ElementFeature copy deliberately drops its
    /// guid and a joint is copied constantly - through the solver, onto its edge and back -
    /// so this is the only place a side's identity survives. Read them through
    /// feature_guid() and the features through to_features(); element_features below is the
    /// bodies, and its guids are only current right after sync_features().
    mutable std::array<std::string, 2> feature_guids;
    const std::string& feature_guid(int side) const;
    void sync_features();
    /// sync_features() applied to copies: identity preserved, the joint itself untouched.
    std::array<session_cpp::ElementFeature, 2> to_features() const;

    /// The whole joint, solver fields included, as JSON - what a graph edge carries. The two
    /// features travel as their guids alone: sync_features() re-derives everything else from
    /// the solver fields, so writing them out in full would store every cut outline twice.
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

/// Position of the plate with this guid, or -1.
int index_of(const std::vector<std::shared_ptr<Plate>>& elements, const std::string& guid);

} // namespace wood_session
