#include "pch.h"
#include "wood_session.h"
#include "wood_contact_detection.h"
#include "wood_feature_detection.h"
#include "wood_merge_modifier.h"
#include "wood_three_valence.h"
using namespace session_cpp;

namespace {

using namespace wood_session;

/// The joint library is header-only and static; it lands in this TU's anonymous namespace.
#include "wood_interaction_feature_plate_joints.h"

// ═══════════════════════════════════════════════════════════════════════════
// Joint library
// ═══════════════════════════════════════════════════════════════════════════

/// What a builder may read while it fills a FeaturePlate.
struct BuildContext {
    const Settings& settings;
    std::vector<std::shared_ptr<Plate>>& elements;
    std::vector<FeaturePlate>& all_joints;
};

/// Family names by family index: the id ranges 1-9, 10-19, 20-29, 30-39, 40-49, 50-59, 60-69.
constexpr std::array<std::string_view, 7> FAMILY_NAMES = {"ss_e_ip", "ss_e_op", "ts_e_p", "cr_c_ip", "tt_e_p", "ss_e_r", "b"};

/// Family of an id by tens; -1 outside 1..69.
int family_of(const int id) {

    if (id < 1 || id > 69)
        return -1;

    return id < 10 ? 0 : id / 10;
}

/// The family a detected joint type falls in: 11 -> 1, 12 -> 0, 13 -> 5, 20 -> 2, 30 -> 3, 40 -> 4, 60 -> 6; -1 otherwise.
int family_of_type(const int joint_type) {
    switch (joint_type) {
        case 11: return 1;
        case 12: return 0;
        case 13: return 5;
        case 20: return 2;
        case 30: return 3;
        case 40: return 4;
        case 60: return 6;
        default: return -1;
    }
}

/// Whether a per-face joint id falls in the id range of the detected joint type; -1 always matches.
bool id_matches(const int joint_type, const int id) {
    return id == -1 || family_of(id) == family_of_type(joint_type);
}

/// The library: the builder of every id the JOINTS_TYPES table names; false for an id without one, which takes its family's default below.
bool build_joint(const int id, FeaturePlate& joint, BuildContext& context) {
    switch (id) {
        case 1: ss_e_ip_1(joint); return true;
        case 2: ss_e_ip_0(joint); return true;
        case 3: ss_e_ip_2(joint); return true;
        case 4: ss_e_ip_3(joint); return true;
        case 5: ss_e_ip_4(joint); return true;
        case 6: ss_e_ip_5(joint, context.elements); return true;
        case 8: side_removal(joint, context.elements); return true;
        case 9: ss_e_ip_custom(joint, context.settings); return true;
        case 10: ss_e_op_1(joint); return true;
        case 11: ss_e_op_2(joint); return true;
        case 12: ss_e_op_0(joint); return true;
        case 13: ss_e_op_3(joint); return true;
        case 14: ss_e_op_4(joint, 0.0, true); return true;
        case 15: ss_e_op_5(joint, context.all_joints, false); return true;
        case 16: ss_e_op_5(joint, context.all_joints, true); return true;
        case 17: ss_e_op_17(joint); return true;
        case 18: ss_e_op_tutorial(joint); return true;
        case 19: ss_e_op_custom(joint, context.settings); return true;
        case 20: ts_e_p_3(joint); return true;
        case 21: ts_e_p_2(joint); return true;
        case 22: ts_e_p_3(joint); return true;
        case 23: ts_e_p_0(joint); return true;
        case 25: ts_e_p_5(joint); return true;
        case 28: side_removal(joint, context.elements); return true;
        case 29: ts_e_p_custom(joint, context.settings); return true;
        case 30: cr_c_ip_0(joint); return true;
        case 31: cr_c_ip_1(joint); return true;
        case 32: cr_c_ip_2(joint); return true;
        case 33: cr_c_ip_3(joint); return true;
        case 34: cr_c_ip_4(joint); return true;
        case 35: cr_c_ip_5(joint); return true;
        case 38: side_removal(joint, context.elements); return true;
        case 39: cr_c_ip_custom(joint, context.settings); return true;
        case 40: tt_e_p_0(joint, context.elements); return true;
        case 41: tt_e_p_1(joint, context.elements); return true;
        case 42: tt_e_p_2(joint, context.elements); return true;
        case 43: tt_e_p_3(joint, context.elements, context.settings.distance_squared); return true;
        case 44: tt_e_p_4(joint, context.elements); return true;
        case 45: tt_e_p_5(joint, context.elements); return true;
        case 54: ss_e_r_3(joint); return true;
        case 55: ss_e_r_2(joint); return true;
        case 56: ss_e_r_0(joint); return true;
        case 57: side_removal(joint, context.elements); return true;
        case 58: side_removal_ss_e_r_1_port(joint, context.elements); return true;
        case 59: ss_e_r_custom(joint, context.settings); return true;
        case 60: b_0(joint); return true;
        case 69: b_custom(joint, context.settings); return true;
        default: return false;
    }
}

/// The builder a family falls back to for an id it has no entry for; tt_e_p has none.
void build_family_default(const int family, FeaturePlate& joint) {
    switch (family) {
        case 0: ss_e_ip_1(joint); return;
        case 1: ss_e_op_1(joint); return;
        case 2: ts_e_p_3(joint); return;
        case 3: cr_c_ip_0(joint); return;
        case 5: ss_e_r_0(joint); return;
        case 6: b_0(joint); return;
        default: return;
    }
}

/// Warns once per id, per thread, when an id has no builder and the family default is used instead.
void warn_unimplemented(const int id, std::string_view family) {
    static thread_local std::set<int> warned_ids;
    if (warned_ids.insert(id).second)
        std::cerr << fmt::format("joint_create_geometry: id={} ({}) not ported, using family default\n", id, family);
}

/// Unit joinery geometry for `id`: the library entry, else the family default; a negative id in the out-of-plane family is the linked variant. Nothing for id 0 or an id outside the detected type's family.
void joint_create_geometry(FeaturePlate& joint, const double division_distance, const double shift_param, const int id, BuildContext& context) {

    joint_get_divisions(joint, division_distance);
    joint.shift = shift_param;

    if (id == 0 || !id_matches(joint.joint_type, id))
        return;

    if (build_joint(id, joint, context))
        return;

    const int family = family_of(id) >= 0 ? family_of(id) : family_of_type(joint.joint_type);
    if (family == 1 && id < 0) {
        ss_e_op_5(joint, context.all_joints, false);
        return;
    }

    if (family < 0) {
        warn_unimplemented(id, "unwired-group");
        if (joint.joint_type == 11 || joint.joint_type == 12)
            ss_e_op_1(joint);
        else if (joint.joint_type == 20)
            ts_e_p_3(joint);
        return;
    }

    warn_unimplemented(id, FAMILY_NAMES[family]);
    build_family_default(family, joint);
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
    const FeaturePlate& joint,
    const std::vector<std::vector<int>>& per_element_joints_types,
    const std::vector<std::shared_ptr<Plate>>& elements) {

    int id_representing_joint_name = -1;
    if (!per_element_joints_types.empty()) {

        const int element0 = index_of_plate(elements, joint.element_a);
        const int element1 = index_of_plate(elements, joint.element_b);
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

/// Row of joint_parameters for a joint type: 11->1 12->0 13->5 20->2 30->3 40->4 60->6.
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

/// Built-in joint parameters: division length, shift and joint id per family row.
constexpr double PARAMETER_DEFAULTS[21] = {
    300, 0.5,  3,
    450, 0.64, 15,
    450, 0.5,  20,
    300, 0.5,  30,
      6, 0.95, 40,
    300, 0.5,  58,
    300, 1.0,  60,
};

/// One joint_parameters entry, from the settings when they are complete and from the built-in defaults otherwise.
double joint_parameter(const std::vector<double>& parameters_global, bool parameters_ok, size_t index) {
    return parameters_ok ? parameters_global[index] : PARAMETER_DEFAULTS[index];
}

/// Per-family row of settings.joint_parameters: division length, shift and the default id when none was given.
FamilyParameters family_parameters(const Settings& settings, const int joint_type, const int id_representing_joint_name) {

    const std::vector<double>& parameters_global = settings.joint_parameters;
    const bool parameters_ok = parameters_global.size() >= 21;

    static thread_local bool parameters_warned = false;
    if (!parameters_ok && !parameters_warned) {
        std::cerr << fmt::format("  WARNING: joint_parameters has {} entries, expected 21 - using built-in defaults.\n", parameters_global.size());
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
    std::array<std::vector<int>, 2> male_fabrication_types; // Male cut types, top and bottom.
    std::array<std::vector<int>, 2> female_fabrication_types; // Female cut types, top and bottom.
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
std::string joint_cache_key(const int id_representing_joint_name, const FeaturePlate& joint) {
    return std::to_string(id_representing_joint_name) + ";" + cache_key_number(joint.shift) + ";" + cache_key_number((double)joint.divisions);
}

/// Unit geometry from the cache when the key is known, else from the constructor and cached afterwards; only type 12 (butterflies) is cached, caching the other types regressed top_to_side_box and vda_floor_0.
void reuse_or_create_geometry(
    FeaturePlate& joint,
    const FamilyParameters& family,
    BuildContext& context,
    std::map<std::string, CachedJointGeometry>& unique_joints_cache) {

    const std::string cache_key = joint_cache_key(family.id, joint);
    const bool use_cache = (joint.joint_type == 12) && joint.linked_joints.empty();

    const auto cache_entry = use_cache ? unique_joints_cache.find(cache_key) : unique_joints_cache.end();
    if (!use_cache) {
        joint_create_geometry(joint, family.division_distance, family.shift, family.id, context);
    } else if (cache_entry != unique_joints_cache.end()) {
        const CachedJointGeometry& cached = cache_entry->second;
        joint.name = cached.name;
        joint.male_outlines = cached.male_outlines;
        joint.female_outlines = cached.female_outlines;
        joint.male_fabrication_types = cached.male_fabrication_types;
        joint.female_fabrication_types = cached.female_fabrication_types;
        joint.unit_scale = cached.unit_scale;
        joint.unit_scale_distance = cached.unit_scale_distance;
    } else {
        joint_create_geometry(joint, family.division_distance, family.shift, family.id, context);

        CachedJointGeometry cached;
        cached.name = joint.name;
        cached.male_outlines = joint.male_outlines;
        cached.female_outlines = joint.female_outlines;
        cached.male_fabrication_types = joint.male_fabrication_types;
        cached.female_fabrication_types = joint.female_fabrication_types;
        cached.unit_scale = joint.unit_scale;
        cached.unit_scale_distance = joint.unit_scale_distance;
        unique_joints_cache.emplace(cache_key, std::move(cached));
    }
}

/// One joint: scale and thickness, unit geometry, orientation to the connection area, merge of the linked shadows.
void build_feature_geometry(
    FeaturePlate& joint,
    const FamilyParameters& family,
    BuildContext& context,
    std::map<std::string, CachedJointGeometry>& unique_joints_cache) {

    std::vector<std::shared_ptr<Plate>>& elements = context.elements;
    std::vector<FeaturePlate>& all_joints = context.all_joints;
    joint.scale = context.settings.joint_scale;

    if (joint.joint_type == 13 || joint.joint_type == 12) {
        const int element_index = index_of_plate(elements, joint.element_a);
        if (element_index >= 0 && element_index < (int)elements.size())
            joint.unit_scale_distance = elements[element_index]->thickness;
    }

    joint_get_divisions(joint, family.division_distance);
    joint.shift = family.shift;
    reuse_or_create_geometry(joint, family, context, unique_joints_cache);

    if (!joint.no_orient) {
        joint_orient_to_connection_area(joint);
    }

    if (!joint.linked_joints.empty() && (family.id == 15 || family.id == 16)) {
        for (const std::string& shadow : joint.linked_joints) {
            const int shadow_index = index_of_joint(all_joints, shadow);
            if (shadow_index >= 0 && !all_joints[shadow_index].no_orient)
                joint_orient_to_connection_area(all_joints[shadow_index]);
        }

        merge_linked_joints(joint, all_joints);
    }
}

/// Unit joinery geometry and orientation for every detected joint, in detection order (the cache is order-dependent).
void build_features_geometry(
    std::vector<FeaturePlate>& all_joints,
    std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<std::vector<int>>& per_element_joints_types,
    const Settings& settings) {

    std::map<std::string, CachedJointGeometry> unique_joints_cache;
    BuildContext context{settings, elements, all_joints};

    for (FeaturePlate& joint : all_joints) {

        const int id_representing_joint_name = joint_id_for(joint, per_element_joints_types, elements);
        const FamilyParameters family = family_parameters(settings, joint.joint_type, id_representing_joint_name);
        if (joint.link)
            continue;

        build_feature_geometry(joint, family, context, unique_joints_cache);
    }
}

/// membership[element][face] = [(joint index, is_male)]; shadow joints go to the extra last slot.
std::vector<std::vector<std::vector<std::pair<int, bool>>>> joint_membership_per_face(
    const std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<FeaturePlate>& all_joints) {

    const size_t element_count = elements.size();
    std::vector<std::vector<std::vector<std::pair<int, bool>>>> membership(element_count);
    for (size_t element_index = 0; element_index < element_count; element_index++)
        membership[element_index].resize(elements[element_index]->planes.size() + 1);

    for (size_t joint_index = 0; joint_index < all_joints.size(); joint_index++) {

        const FeaturePlate& joint = all_joints[joint_index];
        const int element0 = index_of_plate(elements, joint.element_a);
        const int element1 = index_of_plate(elements, joint.element_b);

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

std::vector<std::pair<int, int>> WoodSession::adjacent_pairs(const std::vector<std::shared_ptr<Plate>>& elements) const {

    if (!adjacency.empty())
        return adjacency;

    return adjacency_search(std::vector<std::shared_ptr<Element>>(elements.begin(), elements.end()), settings.distance);
}

std::vector<FeaturePlate> WoodSession::detect_features(const std::vector<std::shared_ptr<Plate>>& elements, const std::vector<std::pair<int, int>>& pairs, SearchType search_type) {

    const int element_count = static_cast<int>(elements.size());

    std::vector<FeaturePlate> joints;
    joints.reserve(pairs.size());
    for (size_t k = 0; k < pairs.size(); ++k) {

        const int index_a = pairs[k].first;
        const int index_b = pairs[k].second;
        if (index_a < 0 || index_b < 0 || index_a >= element_count || index_b >= element_count) {
            std::cerr << fmt::format("  WARNING: adjacency pair {} references elements ({}, {}) but only {} were loaded - skipping.\n", k, index_a, index_b, element_count);
            continue;
        }

        FeaturePlate joint;
        bool swap_planes_b = false;
        const bool ok = face_to_face_wood(*elements[index_a], *elements[index_b], {index_a, index_b}, settings, search_type, joint, swap_planes_b);

        if (swap_planes_b)
            elements[index_b]->flip();

        if (!ok)
            continue;

        joint.guid = ::guid();
        joints.push_back(std::move(joint));
    }

    return joints;
}

void WoodSession::build_feature_geometry(std::vector<std::shared_ptr<Plate>>& elements, std::vector<FeaturePlate>& joints, const std::vector<std::vector<int>>& feature_types) {
    build_features_geometry(joints, elements, feature_types, settings);
}

void WoodSession::merge_features(const std::vector<std::shared_ptr<Plate>>& elements, std::vector<FeaturePlate>& joints) {

    const std::vector<std::vector<std::vector<std::pair<int, bool>>>> membership = joint_membership_per_face(elements, joints);
    const size_t element_count = elements.size();
    for (size_t element_index = 0; element_index < element_count; element_index++) {

        std::vector<Polyline> merged = wood_session::MergeModifier::apply(*elements[element_index], membership[element_index], joints, (int)element_index, settings.distance_squared);
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

void WoodSession::load_sidecars(const std::vector<std::shared_ptr<Plate>>& elements) {

    if (adjacency.empty())
        adjacency = io::load_adjacency(config::DATA_SET_ADJACENCY);
    if (three_valence.empty())
        three_valence = io::load_three_valence(config::DATA_SET_THREE_VALENCE);

    const std::vector<std::vector<Vector>> vectors = io::load_insertion_vectors(config::DATA_SET_INSERTION_VECTORS, elements.size());
    const std::vector<std::vector<int>> types = io::load_feature_types(config::DATA_SET_JOINTS_TYPES, elements.size());
    for (size_t i = 0; i < elements.size(); ++i) {
        if (elements[i]->insertion_vectors().empty() && i < vectors.size())
            elements[i]->insertion_vectors() = vectors[i];
        if (elements[i]->feature_types.empty() && i < types.size())
            elements[i]->feature_types = types[i];
    }
}

/// A reversed plate lists its side slots backwards, so its insertion vectors are read in the same order.
std::vector<FeaturePlate> WoodSession::compute_features() {
    return compute_features(settings.search_type);
}

/// Instances take part as world views: one a joint lands on is promoted before the joint is stored, any other dropped unchanged; a stored plate placed off identity takes its view back.
std::vector<FeaturePlate> WoodSession::compute_features(SearchType search_type) {

    std::vector<std::shared_ptr<Plate>> elements = world_elements<Plate>();
    if (elements.empty())
        return {};

    clear_features();
    load_sidecars(elements);

    std::vector<std::vector<int>> feature_types(elements.size());
    for (size_t i = 0; i < elements.size(); ++i) {

        std::vector<Vector>& vectors = elements[i]->insertion_vectors();
        if (elements[i]->reversed && vectors.size() > 2)
            std::reverse(vectors.begin() + 2, vectors.end());

        feature_types[i] = elements[i]->feature_types;
    }

    std::vector<FeaturePlate> joints = detect_features(elements, adjacent_pairs(elements), search_type);
    link_three_valence_joints(three_valence, elements, joints, settings.angle);
    build_feature_geometry(elements, joints, feature_types);
    merge_features(elements, joints);

    std::unordered_set<std::string> jointed;
    for (const FeaturePlate& joint : joints) {
        jointed.insert(joint.element_a);
        jointed.insert(joint.element_b);
    }

    for (const std::shared_ptr<Plate>& plate : elements)
        if (jointed.count(plate->guid()) || !instance_lookup.count(plate->guid()))
            promote(plate);

    for (FeaturePlate& joint : joints) {

        joint.sync_features();
        if (get_element<Element>(joint.element_a) && get_element<Element>(joint.element_b))
            add_feature(joint);
    }

    return joints;
}

} // namespace wood_session

std::vector<wood_session::FeaturePlate> get_connection_zones(std::vector<std::shared_ptr<wood_session::Plate>>& elements, const wood_session::Settings& settings, SearchType search_type) {

    wood_session::WoodSession scene(wood_session::config::DATA_SET_INPUT_NAME);
    scene.settings = settings;
    for (const std::shared_ptr<wood_session::Plate>& plate : elements)
        scene.add(plate);

    return scene.compute_features(search_type);
}
