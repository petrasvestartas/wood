#include "pch.h"
#include "wood_session.h"
#include "wood_face_to_face.h"
#include "wood_merge_modifier.h"
#include "wood_three_valence.h"
using namespace session_cpp;

namespace {

using namespace wood_session;

/// The joint library is header-only and static; it lands in this TU's anonymous namespace.
#include "wood_joint_lib.h"

// ═══════════════════════════════════════════════════════════════════════════
// Joint geometry dispatch
// ═══════════════════════════════════════════════════════════════════════════

/// Whether a per-face joint id falls in the id range of the detected joint type; -1 always matches.
bool id_matches(const int type, const int id) {

    if (id == -1)
        return true;

    switch (type) {
        case 11: return id >= 10 && id <= 19;
        case 12: return id >= 1 && id <= 9;
        case 13: return id >= 50 && id <= 59;
        case 20: return id >= 20 && id <= 29;
        case 30: return id >= 30 && id <= 39;
        case 40: return id >= 40 && id <= 49;
        case 60: return id >= 60 && id <= 69;
    }

    return false;
}

/// Family of a joint id by tens (1-9 -> 0, 10-19 -> 1, ... 60-69 -> 6); outside those, the detected type's family, -1 when it has none.
int joint_family(const int id, const int joint_type) {

    if (id >= 1 && id <= 9)
        return 0;
    if (id >= 10 && id <= 19)
        return 1;
    if (id >= 20 && id <= 29)
        return 2;
    if (id >= 30 && id <= 39)
        return 3;
    if (id >= 40 && id <= 49)
        return 4;
    if (id >= 50 && id <= 59)
        return 5;
    if (id >= 60 && id <= 69)
        return 6;

    switch (joint_type) {
        case 11: return 1;
        case 12: return 0;
        case 13: return 5;
        case 20: return 2;
        case 30: return 3;
        case 40: return 4;
        default: return -1;
    }
}

/// Warns once per id, per thread, when an id has no constructor and the family default is used instead.
void warn_unimplemented(const int id, std::string_view family) {
    static thread_local std::set<int> warned_ids;
    if (warned_ids.insert(id).second)
        std::cerr << fmt::format("joint_create_geometry: id={} ({}) not ported, using family default\n", id, family);
}

/// Family 0, side-to-side in-plane: ss_e_ip_* by id.
void create_side_in_plane_joint(WoodJoint& joint, const int id, const std::vector<std::shared_ptr<Plate>>* elements) {
    switch (id) {
        case 1: ss_e_ip_1(joint); break;
        case 2: ss_e_ip_0(joint); break;
        case 3: ss_e_ip_2(joint); break;
        case 4: ss_e_ip_3(joint); break;
        case 5: ss_e_ip_4(joint); break;
        case 6:
            if (elements)
                ss_e_ip_5(joint, *elements);
            break;
        case 8:
            if (elements)
                side_removal(joint, *elements);
            break;
        case 9: ss_e_ip_custom(joint); break;
        default: warn_unimplemented(id, "ss_e_ip"); ss_e_ip_1(joint); break;
    }
}

/// Family 1, side-to-side out-of-plane: ss_e_op_* by id; 15, 16 and negative ids need the joint list for the linked shadows.
void create_side_out_of_plane_joint(WoodJoint& joint, const int id, std::vector<WoodJoint>* all_joints) {
    switch (id) {
        case 10: ss_e_op_1(joint); break;
        case 11: ss_e_op_2(joint); break;
        case 12: ss_e_op_0(joint); break;
        case 13: ss_e_op_3(joint); break;
        case 14: ss_e_op_4(joint, 0.0, true); break;
        case 15:
            if (all_joints)
                ss_e_op_5(joint, *all_joints, false);
            else
                ss_e_op_4(joint);
            break;
        case 16:
            if (all_joints)
                ss_e_op_5(joint, *all_joints, true);
            else
                ss_e_op_4(joint);
            break;
        case 17: ss_e_op_17(joint); break;
        case 18: ss_e_op_tutorial(joint); break;
        case 19: ss_e_op_custom(joint); break;
        default:
            if (id < 0) {
                if (all_joints)
                    ss_e_op_5(joint, *all_joints, false);
                else
                    ss_e_op_4(joint);
            } else {
                warn_unimplemented(id, "ss_e_op");
                ss_e_op_1(joint);
            }
            break;
    }
}

/// Family 2, top-to-side: ts_e_p_* by id.
void create_top_side_joint(WoodJoint& joint, const int id, const std::vector<std::shared_ptr<Plate>>* elements) {
    switch (id) {
        case 20: ts_e_p_3(joint); break;
        case 21: ts_e_p_2(joint); break;
        case 22: ts_e_p_3(joint); break;
        case 23: ts_e_p_0(joint); break;
        case 25: ts_e_p_5(joint); break;
        case 28:
            if (elements)
                side_removal(joint, *elements);
            break;
        case 29: ts_e_p_custom(joint); break;
        default: warn_unimplemented(id, "ts_e_p"); ts_e_p_3(joint); break;
    }
}

/// Family 3, cross in-plane: cr_c_ip_* by id.
void create_cross_joint(WoodJoint& joint, const int id, const std::vector<std::shared_ptr<Plate>>* elements) {
    switch (id) {
        case 30: cr_c_ip_0(joint); break;
        case 31: cr_c_ip_1(joint); break;
        case 32: cr_c_ip_2(joint); break;
        case 33: cr_c_ip_3(joint); break;
        case 34: cr_c_ip_4(joint); break;
        case 35: cr_c_ip_5(joint); break;
        case 38:
            if (elements)
                side_removal(joint, *elements);
            break;
        case 39: cr_c_ip_custom(joint); break;
        default: warn_unimplemented(id, "cr_c_ip"); cr_c_ip_0(joint); break;
    }
}

/// Family 4, top-to-top: tt_e_p_* by id; every constructor needs the elements.
void create_top_top_joint(WoodJoint& joint, const int id, const std::vector<std::shared_ptr<Plate>>* elements) {
    switch (id) {
        case 40:
            if (elements)
                tt_e_p_0(joint, *elements);
            break;
        case 41:
            if (elements)
                tt_e_p_1(joint, *elements);
            break;
        case 42:
            if (elements)
                tt_e_p_2(joint, *elements);
            break;
        case 43:
            if (elements)
                tt_e_p_3(joint, *elements);
            break;
        case 44:
            if (elements)
                tt_e_p_4(joint, *elements);
            break;
        case 45:
            if (elements)
                tt_e_p_5(joint, *elements);
            break;
        default: warn_unimplemented(id, "tt_e_p"); break;
    }
}

/// Family 5, side-to-side rotated: ss_e_r_* by id.
void create_rotated_side_joint(WoodJoint& joint, const int id, const std::vector<std::shared_ptr<Plate>>* elements) {
    switch (id) {
        case 54: ss_e_r_3(joint); break;
        case 55: ss_e_r_2(joint); break;
        case 56: ss_e_r_0(joint); break;
        case 57:
            if (elements)
                side_removal(joint, *elements);
            break;
        case 58:
            if (elements)
                side_removal_ss_e_r_1_port(joint, *elements);
            else
                ss_e_r_0(joint);
            break;
        case 59: ss_e_r_custom(joint); break;
        default: warn_unimplemented(id, "ss_e_r"); ss_e_r_0(joint); break;
    }
}

/// Family 6, boundary: b_* by id.
void create_boundary_joint(WoodJoint& joint, const int id) {
    switch (id) {
        case 60: b_0(joint); break;
        case 69: b_custom(joint); break;
        default: warn_unimplemented(id, "b"); b_0(joint); break;
    }
}

/// Unit joinery geometry for `id` (family by tens, variant by id); id -1 falls back to the type's default variant.
void joint_create_geometry(
    WoodJoint& joint,
    const double division_distance,
    const double shift_param,
    const int id,
    std::vector<WoodJoint>* all_joints,
    const std::vector<std::shared_ptr<Plate>>* elements) {

    joint_get_divisions(joint, division_distance);
    joint.shift = shift_param;

    if (id == 0)
        return;
    if (!id_matches(joint.joint_type, id))
        return;

    switch (joint_family(id, joint.joint_type)) {
        case 0: create_side_in_plane_joint(joint, id, elements); break;
        case 1: create_side_out_of_plane_joint(joint, id, all_joints); break;
        case 2: create_top_side_joint(joint, id, elements); break;
        case 3: create_cross_joint(joint, id, elements); break;
        case 4: create_top_top_joint(joint, id, elements); break;
        case 5: create_rotated_side_joint(joint, id, elements); break;
        case 6: create_boundary_joint(joint, id); break;
        default:
            warn_unimplemented(id, "unwired-group");
            switch (joint.joint_type) {
                case 11: case 12: ss_e_op_1(joint); break;
                case 20: ts_e_p_3(joint); break;
                default: break;
            }
            break;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Joint types and parameters
// ═══════════════════════════════════════════════════════════════════════════

/// Family lookup for one joint: representing id, division length and shift.
struct FamilyParameters {
    int id; // The id the JOINTS_TYPES table gives the joint, or the family default.
    double division_distance; // Division length of the family.
    double shift; // Shift of the family.
};

/// The face index before the plate was reversed: the JOINTS_TYPES table uses pre-reversal indices, and a reversed winding reorders the side planes.
int original_face_index(const std::vector<std::shared_ptr<Plate>>& elements, const int element_index, const int face) {

    if (element_index < 0 || element_index >= (int)elements.size())
        return face;
    if (!elements[element_index]->reversed)
        return face;
    if (face < 2)
        return 1 - face;

    const int side_count = (int)elements[element_index]->planes.size() - 2;
    return 2 + (side_count - 1 - (face - 2));
}

/// Wood's id_representing_joint_name: max of the two face ids in the JOINTS_TYPES table, -1 when the table says nothing.
int joint_id_for(
    const WoodJoint& joint,
    const std::vector<std::vector<int>>& per_element_joints_types,
    const std::vector<std::shared_ptr<Plate>>& elements) {

    int id_representing_joint_name = -1;
    if (!per_element_joints_types.empty()) {

        const int element0 = index_of(elements, joint.element_a);
        const int element1 = index_of(elements, joint.element_b);
        const int face0 = joint.contact.face_a;
        const int face1 = joint.contact.face_b;
        const int original_face0 = original_face_index(elements, element0, face0);
        const int original_face1 = original_face_index(elements, element1, face1);
        const int id0 = (element0 >= 0 && element0 < (int)per_element_joints_types.size() && original_face0 >= 0 && original_face0 < (int)per_element_joints_types[element0].size())
            ? std::abs(per_element_joints_types[element0][original_face0]) : 0;
        const int id1 = (element1 >= 0 && element1 < (int)per_element_joints_types.size() && original_face1 >= 0 && original_face1 < (int)per_element_joints_types[element1].size())
            ? std::abs(per_element_joints_types[element1][original_face1]) : 0;

        if (element0 >= 0 && element0 < (int)per_element_joints_types.size() &&
            element1 >= 0 && element1 < (int)per_element_joints_types.size() &&
            (per_element_joints_types[element0].size() > 0 || per_element_joints_types[element1].size() > 0)) {
            id_representing_joint_name = std::max(id0, id1);
            if (id_representing_joint_name == 0)
                id_representing_joint_name = -1;
        }
    }

    return id_representing_joint_name;
}

/// Row of JOINTS_PARAMETERS_AND_TYPES for a joint type: 11->1 12->0 13->5 20->2 30->3 40->4 60->6.
int parameter_row(const int joint_type) {
    switch (joint_type) {
        case 11: return 1;
        case 12: return 0;
        case 13: return 5;
        case 20: return 2;
        case 30: return 3;
        case 40: return 4;
        case 60: return 6;
        default: return 1;
    }
}

/// Built-in JOINTS_PARAMETERS_AND_TYPES: division length, shift and joint id per family row.
constexpr double PARAMETER_DEFAULTS[21] = {
    300, 0.5,  3,
    450, 0.64, 15,
    450, 0.5,  20,
    300, 0.5,  30,
      6, 0.95, 40,
    300, 0.5,  58,
    300, 1.0,  60,
};

/// One JOINTS_PARAMETERS_AND_TYPES entry, from the globals when they are complete and from the built-in defaults otherwise.
double joint_parameter(const std::vector<double>& parameters_global, bool parameters_ok, size_t index) {
    return parameters_ok ? parameters_global[index] : PARAMETER_DEFAULTS[index];
}

/// Per-family row of JOINTS_PARAMETERS_AND_TYPES: division length, shift and the default id when none was given.
FamilyParameters family_parameters(const int joint_type, const int id_representing_joint_name) {

    const std::vector<double>& parameters_global = wood_session::config::JOINTS_PARAMETERS_AND_TYPES;
    const bool parameters_ok = parameters_global.size() >= 21;

    static thread_local bool parameters_warned = false;
    if (!parameters_ok && !parameters_warned) {
        std::cerr << fmt::format("  WARNING: JOINTS_PARAMETERS_AND_TYPES has {} entries, expected 21 - using built-in defaults.\n", parameters_global.size());
        parameters_warned = true;
    }

    const int row = parameter_row(joint_type);

    FamilyParameters family;
    family.id = id_representing_joint_name;
    if (family.id == -1)
        family.id = (int)joint_parameter(parameters_global, parameters_ok, row * 3 + 2);
    family.division_distance = joint_parameter(parameters_global, parameters_ok, row * 3 + 0);
    family.shift = joint_parameter(parameters_global, parameters_ok, row * 3 + 1);

    return family;
}

// ═══════════════════════════════════════════════════════════════════════════
// Joint geometry
// ═══════════════════════════════════════════════════════════════════════════

/// Pre-orient unit-cube geometry shared by joints with an equal cache key.
struct CachedJointGeometry {
    std::string name; // Joint name the constructor gave.
    std::array<std::vector<Polyline>, 2> male_outlines; // Male outlines, top and bottom.
    std::array<std::vector<Polyline>, 2> female_outlines; // Female outlines, top and bottom.
    std::array<std::vector<int>, 2> male_cut_types; // Male cut types, top and bottom.
    std::array<std::vector<int>, 2> female_cut_types; // Female cut types, top and bottom.
    bool unit_scale; // Whether the constructor scales the unit cube.
    double unit_scale_distance; // The distance the unit cube is scaled by.
};

/// Unit geometry by cache key.

/// Wood's get_key number format: std::to_string truncated at two decimals.
std::string cache_key_number(double v) {

    v += 1e-9;
    const std::string s = std::to_string(v);
    const size_t dot = s.find('.');
    if (dot != std::string::npos && dot + 3 <= s.size())
        return s.substr(0, dot + 3);

    return s;
}

/// Unit-geometry cache key: the id stands in for `name` (id->constructor is deterministic).
std::string joint_cache_key(const int id_representing_joint_name, const WoodJoint& joint) {
    return std::to_string(id_representing_joint_name) + ";" + cache_key_number(joint.shift) + ";" + cache_key_number((double)joint.divisions);
}

/// Unit geometry from the cache when the key is known, else from the constructor and cached afterwards; only type 12 (butterflies) is cached, caching the other types regressed top_to_side_box and vda_floor_0.
void reuse_or_create_geometry(
    WoodJoint& joint,
    const FamilyParameters& family,
    std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<WoodJoint>& all_joints,
    std::map<std::string, CachedJointGeometry>& unique_joints_cache) {

    const std::string cache_key = joint_cache_key(family.id, joint);
    const bool use_cache = (joint.joint_type == 12) && joint.linked_joints.empty();

    const auto cache_entry = use_cache ? unique_joints_cache.find(cache_key) : unique_joints_cache.end();
    if (!use_cache) {
        joint_create_geometry(joint, family.division_distance, family.shift, family.id, &all_joints, &elements);
    } else if (cache_entry != unique_joints_cache.end()) {
        const CachedJointGeometry& cached = cache_entry->second;
        joint.name = cached.name;
        joint.male_outlines = cached.male_outlines;
        joint.female_outlines = cached.female_outlines;
        joint.male_cut_types = cached.male_cut_types;
        joint.female_cut_types = cached.female_cut_types;
        joint.unit_scale = cached.unit_scale;
        joint.unit_scale_distance = cached.unit_scale_distance;
    } else {
        joint_create_geometry(joint, family.division_distance, family.shift, family.id, &all_joints, &elements);

        CachedJointGeometry cached;
        cached.name = joint.name;
        cached.male_outlines = joint.male_outlines;
        cached.female_outlines = joint.female_outlines;
        cached.male_cut_types = joint.male_cut_types;
        cached.female_cut_types = joint.female_cut_types;
        cached.unit_scale = joint.unit_scale;
        cached.unit_scale_distance = joint.unit_scale_distance;
        unique_joints_cache.emplace(cache_key, std::move(cached));
    }
}

/// One joint: scale and thickness, unit geometry, orientation to the connection area, merge of the linked shadows.
void build_joint_geometry(
    WoodJoint& joint,
    const FamilyParameters& family,
    std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<WoodJoint>& all_joints,
    std::map<std::string, CachedJointGeometry>& unique_joints_cache) {

    joint.scale = {
        wood_session::config::JOINT_SCALE[0],
        wood_session::config::JOINT_SCALE[1],
        wood_session::config::JOINT_SCALE[2]};

    // ss_e_r_2/3 and ss_e_ip_2 divide by the element thickness, not the 40 mm default.
    if (joint.joint_type == 13 || joint.joint_type == 12) {
        const int element_index = index_of(elements, joint.element_a);
        if (element_index >= 0 && element_index < (int)elements.size())
            joint.unit_scale_distance = elements[element_index]->thickness;
    }

    joint_get_divisions(joint, family.division_distance);
    joint.shift = family.shift;
    reuse_or_create_geometry(joint, family, elements, all_joints, unique_joints_cache);

    if (!joint.no_orient) {
        joint_orient_to_connection_area(joint);
    }

    if (!joint.linked_joints.empty() && (family.id == 15 || family.id == 16)) {
        for (int shadow_index : joint.linked_joints)
            if (!all_joints[shadow_index].no_orient)
                joint_orient_to_connection_area(all_joints[shadow_index]);

        merge_linked_joints(joint, all_joints);
    }
}

/// Unit joinery geometry and orientation for every detected joint, in detection order (the cache is order-dependent).
void build_joints_geometry(
    std::vector<WoodJoint>& all_joints,
    std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<std::vector<int>>& per_element_joints_types) {

    std::map<std::string, CachedJointGeometry> unique_joints_cache;

    for (WoodJoint& joint : all_joints) {

        const int id_representing_joint_name = joint_id_for(joint, per_element_joints_types, elements);
        const FamilyParameters family = family_parameters(joint.joint_type, id_representing_joint_name);
        if (joint.link)
            continue;

        build_joint_geometry(joint, family, elements, all_joints, unique_joints_cache);
    }
}

/// membership[element][face] = [(joint index, is_male)]; shadow joints go to the extra last slot.
std::vector<std::vector<std::vector<std::pair<int, bool>>>> joint_membership_per_face(
    const std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<WoodJoint>& all_joints) {

    const size_t element_count = elements.size();
    std::vector<std::vector<std::vector<std::pair<int, bool>>>> membership(element_count);
    for (size_t element_index = 0; element_index < element_count; element_index++)
        membership[element_index].resize(elements[element_index]->planes.size() + 1);

    for (size_t joint_index = 0; joint_index < all_joints.size(); joint_index++) {

        const WoodJoint& joint = all_joints[joint_index];
        const int element0 = index_of(elements, joint.element_a);
        const int element1 = index_of(elements, joint.element_b);

        if (joint.link) {
            if (element0 >= 0 && element0 < (int)element_count)
                membership[element0].back().push_back({(int)joint_index, true});
            if (element1 >= 0 && element1 < (int)element_count)
                membership[element1].back().push_back({(int)joint_index, false});
        } else {
            const int face0 = joint.contact.face_a;
            const int face1 = joint.contact.face_b;
            if (element0 >= 0 && element0 < (int)element_count && face0 >= 0 && face0 < (int)membership[element0].size())
                membership[element0][face0].push_back({(int)joint_index, true});
            if (element1 >= 0 && element1 < (int)element_count && face1 >= 0 && face1 < (int)membership[element1].size())
                membership[element1][face1].push_back({(int)joint_index, false});
        }
    }
    return membership;
}

} // anonymous namespace

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Joints
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::pair<int, int>> WoodSession::adjacent_pairs(const std::vector<std::pair<int, int>>& adjacency) const {

    if (!adjacency.empty())
        return adjacency;

    std::vector<ContactElement> view;
    for (const std::shared_ptr<Plate>& plate : plates())
        view.emplace_back(*plate);

    return adjacency_search(view, config::DISTANCE);
}

std::vector<WoodJoint> WoodSession::detect_joints(const std::vector<std::pair<int, int>>& pairs, SearchType search_type) {

    using namespace config;

    const std::vector<std::shared_ptr<Plate>> elements = plates();
    const int element_count = static_cast<int>(elements.size());
    const double zero_length_squared = 1e-6; // A joint line no longer than 1 mm is degenerate.
    set_cross_joint_distance_squared(DISTANCE_SQUARED);

    std::vector<WoodJoint> joints;
    joints.reserve(pairs.size());
    for (size_t k = 0; k < pairs.size(); ++k) {

        const int index_a = pairs[k].first;
        const int index_b = pairs[k].second;
        if (index_a < 0 || index_b < 0 || index_a >= element_count || index_b >= element_count) {
            std::cerr << fmt::format("  WARNING: adjacency pair {} references elements ({}, {}) but only {} were loaded - skipping.\n", k, index_a, index_b, element_count);
            continue;
        }

        WoodJoint joint;
        bool swap_planes_b = false;
        const bool ok = face_to_face_wood(
            *elements[index_a],
            *elements[index_b],
            {index_a, index_b},
            JOINT_VOLUME_EXTENSION,
            LIMIT_MIN_JOINT_LENGTH,
            zero_length_squared,
            DISTANCE_SQUARED,
            FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE,
            FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED,
            FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE,
            search_type,
            joint,
            swap_planes_b);

        if (swap_planes_b) {
            std::swap(elements[index_b]->planes[0], elements[index_b]->planes[1]);
            std::swap(elements[index_b]->polylines[0], elements[index_b]->polylines[1]);
        }

        if (ok)
            joints.push_back(std::move(joint));
    }

    return joints;
}

void WoodSession::build_joint_geometry(std::vector<WoodJoint>& joints, const std::vector<std::vector<int>>& joint_types) {
    std::vector<std::shared_ptr<Plate>> elements = plates();
    build_joints_geometry(joints, elements, joint_types);
}

void WoodSession::merge_joints(std::vector<WoodJoint>& joints) {

    std::vector<std::shared_ptr<Plate>> elements = plates();
    const std::vector<std::vector<std::vector<std::pair<int, bool>>>> membership = joint_membership_per_face(elements, joints);
    const size_t element_count = elements.size();
    for (size_t element_index = 0; element_index < element_count; element_index++) {

        std::vector<Polyline> merged = wood_session::MergeModifier::apply(*elements[element_index], membership[element_index], joints, (int)element_index);
        wood_session::Features& features = elements[element_index]->features;
        features.top.clear();
        features.bottom.clear();

        if (merged.size() >= 2) {
            const size_t hole_count = merged.size() - 2;
            features.top.reserve(1 + hole_count / 2);
            features.bottom.reserve(1 + hole_count / 2);
            features.top.push_back(std::move(merged[merged.size() - 2]));
            features.bottom.push_back(std::move(merged[merged.size() - 1]));
            for (size_t hole_index = 0; hole_index + 2 <= hole_count; hole_index += 2) {
                features.top.push_back(std::move(merged[hole_index]));
                features.bottom.push_back(std::move(merged[hole_index + 1]));
            }
        }

        elements[element_index]->invalidate_geometry();
    }
}

std::vector<WoodJoint> WoodSession::compute_joints(SearchType search_type) {
    return compute_joints(search_type, config::load_joint_data(plates().size()));
}

/// A plate keeps the insertion vectors and joint types it already carries; `data` fills the ones it does not.
std::vector<WoodJoint> WoodSession::compute_joints(SearchType search_type, const JointData& data) {

    std::vector<std::shared_ptr<Plate>> elements = plates();
    if (elements.empty())
        return {};

    clear_joints();

    std::vector<std::vector<int>> joint_types(elements.size());
    for (size_t i = 0; i < elements.size(); ++i) {

        std::vector<Vector>& vectors = elements[i]->insertion_vectors();
        if (vectors.empty() && i < data.insertion_vectors.size())
            vectors = data.insertion_vectors[i];
        if (elements[i]->reversed && vectors.size() > 2)
            std::reverse(vectors.begin() + 2, vectors.end());

        joint_types[i] = i < data.joint_types.size() && !data.joint_types[i].empty() ? data.joint_types[i] : elements[i]->joint_types;
    }

    std::vector<WoodJoint> joints = detect_joints(adjacent_pairs(data.adjacency), search_type);
    link_three_valence_joints(data.three_valence, elements, joints);
    build_joint_geometry(joints, joint_types);
    merge_joints(joints);

    for (WoodJoint& joint : joints) {

        joint.sync_features();
        if (!get_element<Element>(joint.element_a) || !get_element<Element>(joint.element_b))
            continue;

        WoodInteraction interaction = get_interaction(joint.element_a, joint.element_b);
        interaction.joints.push_back(joint);
        set_interaction(joint.element_a, joint.element_b, interaction);
    }

    sync_joint_features();

    return joints;
}

} // namespace wood_session

std::vector<wood_session::WoodJoint> get_connection_zones(std::vector<std::shared_ptr<wood_session::Plate>>& elements, SearchType search_type) {
    return get_connection_zones(elements, search_type, wood_session::config::load_joint_data(elements.size()));
}

std::vector<wood_session::WoodJoint> get_connection_zones(
    std::vector<std::shared_ptr<wood_session::Plate>>& elements,
    SearchType search_type,
    const wood_session::JointData& data) {

    wood_session::WoodSession scene(wood_session::config::DATA_SET_INPUT_NAME);
    for (const std::shared_ptr<wood_session::Plate>& plate : elements)
        scene.add(plate);

    return scene.compute_joints(search_type, data);
}
