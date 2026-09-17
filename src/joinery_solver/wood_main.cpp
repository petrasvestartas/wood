#include "wood_pch.h"
#include "wood_element_plate.h"
#include "wood_face_to_face.h"
#include "wood_joint.h"
#include "wood_polyline_merge.h"
#include "wood_session.h"
#include "wood_three_valence.h"
using namespace session_cpp;

constexpr bool TRACE = false;

/// In-memory adjacency override for the ChevronJoineryData overload; the three-valence override lives in wood_three_valence.
static thread_local std::vector<std::pair<int, int>> tl_adjacency_override;

namespace {

using wood_session::WoodJoint;
using wood_session::Plate;
using wood_session::joint_orient_to_connection_area;
using wood_session::merge_linked_joints;
using wood_session::joint_get_divisions;
using wood_session::side_removal_ss_e_r_1_port;
using wood_session::side_removal;
using wood_session::tt_e_p_0;
using wood_session::tt_e_p_1;
using wood_session::tt_e_p_2;
using wood_session::tt_e_p_3;
using wood_session::tt_e_p_4;
using wood_session::tt_e_p_5;

/// The joint library is header-only and static; it lands in this TU's anonymous namespace.
#include "wood_joint_lib.h"

// ═══════════════════════════════════════════════════════════════════════════
// Joint geometry dispatch
// ═══════════════════════════════════════════════════════════════════════════

/// Whether a per-face joint id falls in the id range of the detected joint type; -1 always matches.
static bool id_matches(const int type, const int id) {
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

/// Unit joinery geometry for `id` (family by tens, variant by id); id -1 falls back to the type's default variant.
static void joint_create_geometry(
    WoodJoint& joint,
    double division_distance,
    double shift_param,
    int id,
    std::vector<WoodJoint>* all_joints = nullptr,
    const std::vector<std::shared_ptr<Plate>>* elements = nullptr) {
    joint_get_divisions(joint, division_distance);
    joint.shift = shift_param;

    if (id == 0)
        return;
    if (!id_matches(joint.joint_type, id))
        return;

    int group = -1;
    if (id >= 1 && id <= 9) {
        group = 0;
    } else if (id >= 10 && id <= 19) {
        group = 1;
    } else if (id >= 20 && id <= 29) {
        group = 2;
    } else if (id >= 30 && id <= 39) {
        group = 3;
    } else if (id >= 40 && id <= 49) {
        group = 4;
    } else if (id >= 50 && id <= 59) {
        group = 5;
    } else if (id >= 60 && id <= 69) {
        group = 6;
    } else {
        switch (joint.joint_type) {
            case 11: group = 1; break;
            case 12: group = 0; break;
            case 13: group = 5; break;
            case 20: group = 2; break;
            case 30: group = 3; break;
            case 40: group = 4; break;
            default: group = -1;
        }
    }

    static thread_local std::set<int> warned_ids;
    auto warn_unimplemented = [&](const char* family) {
        if (warned_ids.insert(id).second)
            fmt::print(stderr, "joint_create_geometry: id={} ({}) not ported, using family default\n", id, family);
    };

    switch (group) {
        case 0:
            switch (id) {
                case 1: ss_e_ip_1(joint); break;
                case 2: ss_e_ip_0(joint); break;
                case 3: ss_e_ip_2(joint); break;
                case 4: ss_e_ip_3(joint); break;
                case 5: ss_e_ip_4(joint); break;
                case 6: if (elements) ss_e_ip_5(joint, *elements); break;
                case 8: if (elements) side_removal(joint, *elements); break;
                case 9: ss_e_ip_custom(joint); break;
                default: warn_unimplemented("ss_e_ip"); ss_e_ip_1(joint); break;
            }
            break;

        case 1:
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
                        warn_unimplemented("ss_e_op");
                        ss_e_op_1(joint);
                    }
                    break;
            }
            break;

        case 2:
            switch (id) {
                case 20: ts_e_p_3(joint); break;
                case 21: ts_e_p_2(joint); break;
                case 22: ts_e_p_3(joint); break;
                case 23: ts_e_p_0(joint); break;
                case 25: ts_e_p_5(joint); break;
                case 28: if (elements) side_removal(joint, *elements); break;
                case 29: ts_e_p_custom(joint); break;
                default: warn_unimplemented("ts_e_p"); ts_e_p_3(joint); break;
            }
            break;

        case 3:
            switch (id) {
                case 30: cr_c_ip_0(joint); break;
                case 31: cr_c_ip_1(joint); break;
                case 32: cr_c_ip_2(joint); break;
                case 33: cr_c_ip_3(joint); break;
                case 34: cr_c_ip_4(joint); break;
                case 35: cr_c_ip_5(joint); break;
                case 38: if (elements) side_removal(joint, *elements); break;
                case 39: cr_c_ip_custom(joint); break;
                default: warn_unimplemented("cr_c_ip"); cr_c_ip_0(joint); break;
            }
            break;

        case 4:
            switch (id) {
                case 40: if (elements) tt_e_p_0(joint, *elements); break;
                case 41: if (elements) tt_e_p_1(joint, *elements); break;
                case 42: if (elements) tt_e_p_2(joint, *elements); break;
                case 43: if (elements) tt_e_p_3(joint, *elements); break;
                case 44: if (elements) tt_e_p_4(joint, *elements); break;
                case 45: if (elements) tt_e_p_5(joint, *elements); break;
                default: warn_unimplemented("tt_e_p"); break;
            }
            break;

        case 5:
            switch (id) {
                case 54: ss_e_r_3(joint); break;
                case 55: ss_e_r_2(joint); break;
                case 56: ss_e_r_0(joint); break;
                case 57: if (elements) side_removal(joint, *elements); break;
                case 58:
                    if (elements)
                        side_removal_ss_e_r_1_port(joint, *elements);
                    else
                        ss_e_r_0(joint);
                    break;
                case 59: ss_e_r_custom(joint); break;
                default: warn_unimplemented("ss_e_r"); ss_e_r_0(joint); break;
            }
            break;

        case 6:
            switch (id) {
                case 60: b_0(joint); break;
                case 69: b_custom(joint); break;
                default: warn_unimplemented("b"); b_0(joint); break;
            }
            break;

        default:
            warn_unimplemented("unwired-group");
            switch (joint.joint_type) {
                case 11: case 12: ss_e_op_1(joint); break;
                case 20: ts_e_p_3(joint); break;
                default: break;
            }
            break;
    }
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════
// Pipeline stages
// ═══════════════════════════════════════════════════════════════════════════

namespace {

using Clock = std::chrono::high_resolution_clock;

/// Detection parameters handed to face_to_face_wood for every adjacent pair.
struct DetectionParameters {
    std::vector<double> joint_volume_extension;
    double limit_min_joint_length;
    double distance_squared;
    double coplanar_tolerance;
    double dihedral_angle_threshold;
    bool all_treated_as_rotated;
    bool rotated_joint_as_average;
};

/// Detection counters: successes, failures and successes per joint type [11, 12, 13, 20, 30, 40].
struct DetectionStatistics {
    int counts[6] = {0, 0, 0, 0, 0, 0};
    int failed = 0;
    int succeeded = 0;
};

/// Family lookup for one joint: representing id, division length and shift.
struct FamilyParameters {
    int id;
    double division_distance;
    double shift;
};

/// Pre-orient unit-cube geometry shared by joints with an equal cache key.
struct CachedJointGeometry {
    std::string name;
    std::array<std::vector<Polyline>, 2> m_outlines;
    std::array<std::vector<Polyline>, 2> f_outlines;
    std::array<std::vector<int>, 2> m_cut_types;
    std::array<std::vector<int>, 2> f_cut_types;
    bool unit_scale;
    double unit_scale_distance;
};

using JointGeometryCache = std::map<std::string, CachedJointGeometry>;

/// Stage boundaries for the trace timing report.
struct StageTimes {
    Clock::time_point start;
    Clock::time_point before_adjacency;
    Clock::time_point after_adjacency;
    Clock::time_point after_detection;
    Clock::time_point after_three_valence;
    Clock::time_point after_geometry;
    Clock::time_point after_membership;
    Clock::time_point end;
};

/// Stage 1: candidate pairs from the adjacency sidecar, the thread-local override or the OBB+BVH search.
static std::vector<std::pair<int, int>> adjacent_pairs(
    const std::string& adjacency_name,
    const std::vector<std::shared_ptr<Plate>>& elements) {
    std::vector<std::pair<int, int>> adjacency_pairs;
    if (!adjacency_name.empty()) {
        std::ifstream adjacency_file(adjacency_name);
        int a;
        int b;
        while (adjacency_file >> a >> b)
            adjacency_pairs.emplace_back(a, b);
        if (TRACE)
            fmt::print("adjacency: {} pairs from {}\n", adjacency_pairs.size(), adjacency_name);
    }
    if (adjacency_pairs.empty() && !tl_adjacency_override.empty())
        adjacency_pairs = tl_adjacency_override;

    if (adjacency_pairs.empty()) {
        const double distance = wood_session::globals::DISTANCE;
        if (TRACE)
            fmt::print(stderr, "[GCZ] adjacency_search start  DISTANCE={}\n", distance);
        std::vector<wood_session::ContactElement> view;
        view.reserve(elements.size());
        for (const std::shared_ptr<Plate>& plate : elements)
            view.emplace_back(*plate);
        adjacency_pairs = wood_session::adjacency_search(view, distance);
        if (TRACE)
            fmt::print(stderr, "[GCZ] adjacency pairs={}\n", adjacency_pairs.size());
        if (TRACE)
            fmt::print("adjacency: {} pairs from OBB+BVH\n", adjacency_pairs.size());
    }
    return adjacency_pairs;
}

/// Stage 2: per-element insertion vectors from the sidecar (one element per line, `x y z ...`) into each Plate, reversed plates flipped.
static void load_insertion_vectors(
    const std::string& insertion_vectors_name,
    std::vector<std::shared_ptr<Plate>>& elements) {
    std::vector<std::vector<Vector>> per_element(elements.size(), std::vector<Vector>{});
    if (!insertion_vectors_name.empty()) {
        std::ifstream insertion_vectors_file(insertion_vectors_name);
        std::string line;
        size_t element_index = 0;
        size_t total_loaded = 0;
        while (std::getline(insertion_vectors_file, line) && element_index < per_element.size()) {
            std::istringstream stream(line);
            std::vector<Vector>& vectors = per_element[element_index];
            double x;
            double y;
            double z;
            while (stream >> x >> y >> z) {
                vectors.emplace_back(x, y, z);
                total_loaded++;
            }
            element_index++;
        }
        if (TRACE)
            fmt::print("insertion_vectors: {} vectors across {} elements from {}\n", total_loaded, element_index, insertion_vectors_name);
    }
    for (size_t element_index = 0; element_index < elements.size(); element_index++) {
        if (elements[element_index]->insertion_vectors().empty())
            elements[element_index]->insertion_vectors() = per_element[element_index];
        if (elements[element_index]->reversed) {
            std::vector<Vector>& vectors = elements[element_index]->insertion_vectors();
            if (vectors.size() > 2)
                std::reverse(vectors.begin() + 2, vectors.end());
        }
    }
}

/// Stage 3: run face_to_face_wood on every adjacent pair; joints stay in adjacency-pair order.
static std::vector<WoodJoint> detect_joints(
    std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<std::pair<int, int>>& adjacency_pairs,
    const DetectionParameters& parameters,
    const SearchType search_type,
    DetectionStatistics& statistics) {
    std::vector<WoodJoint> all_joints;
    all_joints.reserve(adjacency_pairs.size());
    if (TRACE)
        fmt::print(stderr, "[GCZ] joint detection loop  pairs={}\n", adjacency_pairs.size());
    const int element_count = static_cast<int>(elements.size());
    for (size_t k = 0; k < adjacency_pairs.size(); ++k) {
        const int index_a = adjacency_pairs[k].first;
        const int index_b = adjacency_pairs[k].second;
        if (TRACE)
            fmt::print(stderr, "[GCZ]   pair k={}  index_a={} index_b={}\n", k, index_a, index_b);

        if (index_a < 0 || index_b < 0 || index_a >= element_count || index_b >= element_count) {
            fmt::print(stderr, "  WARNING: adjacency pair {} references elements ({}, {}) but only {} were loaded - skipping.\n", k, index_a, index_b, element_count);
            continue;
        }

        WoodJoint joint;
        bool swap_planes_b = false;
        const bool ok = face_to_face_wood(
            *elements[index_a],
            *elements[index_b],
            {index_a, index_b},
            parameters.joint_volume_extension,
            parameters.limit_min_joint_length,
            parameters.distance_squared,
            parameters.coplanar_tolerance,
            parameters.dihedral_angle_threshold,
            parameters.all_treated_as_rotated,
            parameters.rotated_joint_as_average,
            search_type,
            joint,
            swap_planes_b);
        if (TRACE)
            fmt::print(stderr, "[GCZ]   face_to_face_wood done  ok={}  type={}\n", (int)ok, ok ? joint.joint_type : -1);
        if (swap_planes_b) {
            std::swap(elements[index_b]->planes[0], elements[index_b]->planes[1]);
            std::swap(elements[index_b]->polylines[0], elements[index_b]->polylines[1]);
        }
        if (!ok) {
            if (TRACE && !joint.dbg_fail_reason.empty())
                fmt::print("  FAIL pair ({},{}) coplanar={} boolean={} reason={}\n", index_a, index_b, joint.dbg_coplanar, joint.dbg_boolean, joint.dbg_fail_reason);
            ++statistics.failed;
            continue;
        }
        ++statistics.succeeded;
        switch (joint.joint_type) {
            case 11: ++statistics.counts[0]; break;
            case 12: ++statistics.counts[1]; break;
            case 13: ++statistics.counts[2]; break;
            case 20: ++statistics.counts[3]; break;
            case 30: ++statistics.counts[4]; break;
            case 40: ++statistics.counts[5]; break;
            default: break;
        }
        all_joints.push_back(std::move(joint));
    }
    return all_joints;
}

/// Stage 5: per-element per-face joint type ids (the wood JOINTS_TYPES filter); 0 = no joint, tens digit = family.
static std::vector<std::vector<int>> load_joint_types(
    const std::string& joint_types_name,
    const std::vector<std::shared_ptr<Plate>>& elements) {
    std::vector<std::vector<int>> per_element(elements.size());
    if (!joint_types_name.empty()) {
        std::ifstream joint_types_file(joint_types_name);
        std::string line;
        size_t element_index = 0;
        size_t total_loaded = 0;
        while (std::getline(joint_types_file, line) && element_index < per_element.size()) {
            std::istringstream stream(line);
            int value;
            while (stream >> value) {
                per_element[element_index].push_back(value);
                total_loaded++;
            }
            element_index++;
        }
        if (TRACE)
            fmt::print("joints_types: {} ids across {} elements from {}\n", total_loaded, element_index, joint_types_name);
    }
    for (size_t element_index = 0; element_index < elements.size(); ++element_index)
        if (per_element[element_index].empty() && !elements[element_index]->joint_types.empty())
            per_element[element_index] = elements[element_index]->joint_types;
    return per_element;
}

/// Wood's id_representing_joint_name: max of the two face ids in the JOINTS_TYPES table, -1 when the table says nothing.
static int joint_id_for(
    const WoodJoint& joint,
    const std::vector<std::vector<int>>& per_element_joints_types,
    const std::vector<std::shared_ptr<Plate>>& elements) {
    int id_representing_joint_name = -1;
    if (!per_element_joints_types.empty()) {
        const int element0 = index_of(elements, joint.element_a);
        const int element1 = index_of(elements, joint.element_b);
        const int face0 = joint.contact.face_a;
        const int face1 = joint.contact.face_b;
        // The table uses pre-reversal face indices; a reversed winding reorders the side planes.
        auto original_face = [&](int element_index, int face) -> int {
            if (element_index < 0 || element_index >= (int)elements.size())
                return face;
            if (!elements[element_index]->reversed)
                return face;
            if (face < 2)
                return 1 - face;
            const int side_count = (int)elements[element_index]->planes.size() - 2;
            return 2 + (side_count - 1 - (face - 2));
        };
        const int original_face0 = original_face(element0, face0);
        const int original_face1 = original_face(element1, face1);
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
static int parameter_row(const int joint_type) {
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

/// Per-family row of JOINTS_PARAMETERS_AND_TYPES: division length, shift and the default id when none was given.
static FamilyParameters family_parameters(const int joint_type, const int id_representing_joint_name) {
    static constexpr double PARAMETER_DEFAULTS[21] = {
        300, 0.5,  3,
        450, 0.64, 15,
        450, 0.5,  20,
        300, 0.5,  30,
          6, 0.95, 40,
        300, 0.5,  58,
        300, 1.0,  60,
    };
    const std::vector<double>& parameters_global = wood_session::globals::JOINTS_PARAMETERS_AND_TYPES;
    const bool parameters_ok = parameters_global.size() >= 21;
    static thread_local bool parameters_warned = false;
    if (!parameters_ok && !parameters_warned) {
        fmt::print(stderr, "  WARNING: JOINTS_PARAMETERS_AND_TYPES has {} entries, expected 21 - using built-in defaults.\n", parameters_global.size());
        parameters_warned = true;
    }
    auto parameter = [&](size_t idx) -> double {
        return parameters_ok ? parameters_global[idx] : PARAMETER_DEFAULTS[idx];
    };
    const int row = parameter_row(joint_type);

    FamilyParameters family;
    family.id = id_representing_joint_name;
    if (family.id == -1)
        family.id = (int)parameter(row * 3 + 2);
    family.division_distance = parameter(row * 3 + 0);
    family.shift = parameter(row * 3 + 1);
    return family;
}

/// Wood's get_key number format: std::to_string truncated at two decimals.
static std::string cache_key_number(double v) {
    v += 1e-9;
    const std::string s = std::to_string(v);
    const auto dot = s.find('.');
    if (dot != std::string::npos && dot + 3 <= s.size())
        return s.substr(0, dot + 3);
    return s;
}

/// Unit-geometry cache key: the id stands in for `name` (id->constructor is deterministic).
static std::string joint_cache_key(const int id_representing_joint_name, const WoodJoint& joint) {
    return std::to_string(id_representing_joint_name) + ";" + cache_key_number(joint.shift) + ";" + cache_key_number((double)joint.divisions);
}

/// Stage 6, one joint: unit geometry (cached for butterflies), orient to the connection area, merge linked shadows.
static void build_joint_geometry(
    WoodJoint& joint,
    const FamilyParameters& family,
    std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<WoodJoint>& all_joints,
    JointGeometryCache& unique_joints_cache) {
    joint.scale = {
        wood_session::globals::JOINT_SCALE[0],
        wood_session::globals::JOINT_SCALE[1],
        wood_session::globals::JOINT_SCALE[2]};

    // ss_e_r_2/3 and ss_e_ip_2 divide by the element thickness, not the 40 mm default.
    if (joint.joint_type == 13 || joint.joint_type == 12) {
        const int element_index = index_of(elements, joint.element_a);
        if (element_index >= 0 && element_index < (int)elements.size())
            joint.unit_scale_distance = elements[element_index]->thickness;
    }

    joint_get_divisions(joint, family.division_distance);
    joint.shift = family.shift;
    const std::string cache_key = joint_cache_key(family.id, joint);

    // Only type 12 (butterflies) is cached; caching the other types regressed top_to_side_box and vda_floor_0.
    const bool use_cache = (joint.joint_type == 12) && joint.linked_joints.empty();

    const auto cache_entry = use_cache ? unique_joints_cache.find(cache_key) : unique_joints_cache.end();
    if (!use_cache) {
        joint_create_geometry(joint, family.division_distance, family.shift, family.id, &all_joints, &elements);
    } else if (cache_entry != unique_joints_cache.end()) {
        const CachedJointGeometry& cached = cache_entry->second;
        joint.name = cached.name;
        joint.m_outlines = cached.m_outlines;
        joint.f_outlines = cached.f_outlines;
        joint.m_cut_types = cached.m_cut_types;
        joint.f_cut_types = cached.f_cut_types;
        joint.unit_scale = cached.unit_scale;
        joint.unit_scale_distance = cached.unit_scale_distance;
    } else {
        joint_create_geometry(joint, family.division_distance, family.shift, family.id, &all_joints, &elements);
        CachedJointGeometry cached;
        cached.name = joint.name;
        cached.m_outlines = joint.m_outlines;
        cached.f_outlines = joint.f_outlines;
        cached.m_cut_types = joint.m_cut_types;
        cached.f_cut_types = joint.f_cut_types;
        cached.unit_scale = joint.unit_scale;
        cached.unit_scale_distance = joint.unit_scale_distance;
        unique_joints_cache.emplace(cache_key, std::move(cached));
    }
    if (TRACE)
        fmt::print(stderr, "[GCZ]   after joint_create_geometry  no_orient={}\n", (int)joint.no_orient);
    if (!joint.no_orient) {
        if (TRACE)
            fmt::print(stderr, "[GCZ]   calling joint_orient_to_connection_area\n");
        joint_orient_to_connection_area(joint);
        if (TRACE)
            fmt::print(stderr, "[GCZ]   joint_orient done\n");
    }
    if (!joint.linked_joints.empty() && (family.id == 15 || family.id == 16)) {
        for (int shadow_index : joint.linked_joints)
            if (!all_joints[shadow_index].no_orient)
                joint_orient_to_connection_area(all_joints[shadow_index]);
        merge_linked_joints(joint, all_joints);
    }
}

/// Stages 6-7: unit joinery geometry and orientation for every detected joint, in detection order (the cache is order-dependent).
static void build_joints_geometry(
    std::vector<WoodJoint>& all_joints,
    std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<std::vector<int>>& per_element_joints_types) {
    JointGeometryCache unique_joints_cache;
    if (TRACE)
        fmt::print(stderr, "[GCZ] geometry loop start  all_joints={}\n", all_joints.size());
    for (WoodJoint& joint : all_joints) {
        if (TRACE)
            fmt::print(stderr, "[GCZ]   geom joint type={}  element0={} element1={}\n", joint.joint_type, joint.element_a, joint.element_b);
        const int id_representing_joint_name = joint_id_for(joint, per_element_joints_types, elements);
        const FamilyParameters family = family_parameters(joint.joint_type, id_representing_joint_name);
        if (joint.link)
            continue;
        build_joint_geometry(joint, family, elements, all_joints, unique_joints_cache);
    }
}

/// Stage 8: membership[element][face] = [(joint index, is_male)]; shadow joints go to the extra last slot.
static JointMembership joint_membership_per_face(
    const std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<WoodJoint>& all_joints) {
    const size_t element_count = elements.size();
    JointMembership membership(element_count);
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

/// Stage 9: merge joint cuts into each plate's polylines and de-interleave [holes..., outer_top, outer_bot] into `features`.
static void merge_joints_into_plates(
    std::vector<std::shared_ptr<Plate>>& elements,
    const JointMembership& membership,
    std::vector<WoodJoint>& all_joints) {
    const size_t element_count = elements.size();
    for (size_t element_index = 0; element_index < element_count; element_index++) {
        std::vector<Polyline> merged = wood_session::PolylineMerge::merge(*elements[element_index], membership[element_index], all_joints, (int)element_index);
        auto& features = elements[element_index]->features;
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
    }
}

/// Trace summary: counts per type and per-stage timings.
static void report_timings(
    const std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<std::pair<int, int>>& adjacency_pairs,
    const DetectionStatistics& statistics,
    const StageTimes& times) {
    auto milliseconds = [](auto a, auto b) {
        return std::chrono::duration<double, std::milli>(b - a).count();
    };
    fmt::print("{} elements -> {} adjacency pairs\n", elements.size(), adjacency_pairs.size());
    fmt::print("  joints: {} success / {} failed\n", statistics.succeeded, statistics.failed);
    fmt::print(
        "  by type: 11={} 12={} 13={} 20={} 30={} 40={}\n",
        statistics.counts[0], statistics.counts[1], statistics.counts[2], statistics.counts[3], statistics.counts[4], statistics.counts[5]);
    fmt::print("  time: {:.0f}milliseconds\n", milliseconds(times.start, times.end));
    fmt::print(
        "  stages(milliseconds): setup={:.1f} adjacency={:.1f} detect={:.1f} tv={:.1f} geom={:.1f} jmf={:.1f} merge={:.1f}\n",
        milliseconds(times.start, times.after_adjacency) - milliseconds(times.before_adjacency, times.after_adjacency), milliseconds(times.before_adjacency, times.after_adjacency), milliseconds(times.after_adjacency, times.after_detection), milliseconds(times.after_detection, times.after_three_valence), milliseconds(times.after_three_valence, times.after_geometry), milliseconds(times.after_geometry, times.after_membership), milliseconds(times.after_membership, times.end));
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════
// get_connection_zones
// ═══════════════════════════════════════════════════════════════════════════

std::vector<WoodJoint> get_connection_zones(
    std::vector<std::shared_ptr<Plate>>& elements,
    SearchType search_type) {

    if (TRACE)
        fmt::print(stderr, "[GCZ] enter  element_count={}  search_type={}\n", elements.size(), (int)search_type);

    using namespace wood_session::globals;
    const std::string dataset_name = DATA_SET_INPUT_NAME;
    const double dihedral_threshold = FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE;

    StageTimes times;
    times.start = Clock::now();

    const std::string adjacency_name = DATA_SET_ADJACENCY;
    const std::string three_valence_name = DATA_SET_THREE_VALENCE;
    const std::string insertion_vectors_name = DATA_SET_INSERTION_VECTORS;
    const std::string joint_types_name = DATA_SET_JOINTS_TYPES;
    const std::vector<double> volume_extension = JOINT_VOLUME_EXTENSION;

    if (TRACE)
        fmt::print("\n=== {}.obj ===\n", dataset_name);

    times.before_adjacency = Clock::now();

    const std::vector<std::pair<int, int>> adjacency_pairs = adjacent_pairs(adjacency_name, elements);
    times.after_adjacency = Clock::now();

    const DetectionParameters parameters{
        volume_extension,
        LIMIT_MIN_JOINT_LENGTH,
        1e-6,
        DISTANCE_SQUARED,
        dihedral_threshold,
        FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED,
        FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE};

    wood_session::set_cross_joint_distance_squared(DISTANCE_SQUARED);

    load_insertion_vectors(insertion_vectors_name, elements);

    DetectionStatistics statistics;
    std::vector<WoodJoint> all_joints = detect_joints(elements, adjacency_pairs, parameters, search_type, statistics);
    times.after_detection = Clock::now();

    wood_session::link_three_valence_joints(three_valence_name, elements, all_joints);

    const std::vector<std::vector<int>> per_element_joints_types = load_joint_types(joint_types_name, elements);

    times.after_three_valence = Clock::now();
    build_joints_geometry(all_joints, elements, per_element_joints_types);
    times.after_geometry = Clock::now();
    if (TRACE)
        fmt::print(stderr, "[GCZ] geometry dispatch done  all_joints={}\n", all_joints.size());

    const JointMembership membership = joint_membership_per_face(elements, all_joints);
    times.after_membership = Clock::now();
    if (TRACE)
        fmt::print(stderr, "[GCZ] membership built  starting merge\n");

    merge_joints_into_plates(elements, membership, all_joints);
    times.end = Clock::now();

    if (TRACE)
        report_timings(elements, adjacency_pairs, statistics, times);
    for (WoodJoint& joint : all_joints)
        joint.sync_features();
    return all_joints;
}

/// In-memory overload: insertion vectors and joint types land on the plates, adjacency and three-valence in thread-locals.
std::vector<wood_session::WoodJoint> get_connection_zones(
    std::vector<std::shared_ptr<wood_session::Plate>>& elements,
    SearchType search_type,
    const wood_session::ChevronJoineryData& joinery_data) {
    for (size_t element_index = 0; element_index < elements.size(); ++element_index) {
        if (elements[element_index]->insertion_vectors().empty() && element_index < joinery_data.insertion_vectors.size()) {
            const auto& flat_vectors = joinery_data.insertion_vectors[element_index];
            std::vector<Vector>& vectors = elements[element_index]->insertion_vectors();
            for (int side = 0; side < 6; ++side)
                vectors.emplace_back(flat_vectors[side * 3 + 0], flat_vectors[side * 3 + 1], flat_vectors[side * 3 + 2]);
        }
    }

    for (size_t element_index = 0; element_index < elements.size(); ++element_index) {
        if (elements[element_index]->joint_types.empty() && element_index < joinery_data.joints_per_face.size()) {
            const auto& face_types = joinery_data.joints_per_face[element_index];
            elements[element_index]->joint_types.assign(face_types.begin(), face_types.end());
        }
    }

    // RAII clear: a throw inside the pipeline must not leave this model's adjacency for the next solve on this thread.
    struct OverrideGuard {
        ~OverrideGuard() {
            std::vector<std::pair<int, int>>().swap(tl_adjacency_override);
            wood_session::clear_three_valence_override();
        }
    } tl_guard;

    tl_adjacency_override = joinery_data.adjacency;

    std::vector<std::vector<int>> three_valence_groups;
    three_valence_groups.push_back({0});
    for (const auto& group : joinery_data.three_valence)
        three_valence_groups.push_back({group[0], group[1], group[2], group[3]});
    wood_session::set_three_valence_override(std::move(three_valence_groups));

    return get_connection_zones(elements, search_type);
}
