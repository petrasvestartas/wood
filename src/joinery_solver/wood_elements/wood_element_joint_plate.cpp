#include "pch.h"
#include "wood_session.h"
#include "wood_three_valence.h"
#include "element_joint.pb.h"
#include "wood_element_geometry.h"

using namespace session_cpp;
namespace {

using namespace wood_session;

#include "wood_interaction_feature_plate_joints.h"

// ═══════════════════════════════════════════════════════════════════════════
// Joint library
// ═══════════════════════════════════════════════════════════════════════════

/// The id of every design 2024's tables reach, by name and by its 2024 name: the inverse of the build_ switches below, a side removal by its family.
const std::unordered_map<std::string, int> LIBRARY_IDS = {
    {"", 0},
    {"side_side_inplane_dovetails", 1}, {"side_side_inplane_three_fingers", 2}, {"side_side_inplane_butterfly_keys", 3}, {"side_side_inplane_splayed_scarf", 4}, {"side_side_inplane_cross_splayed_scarf", 5}, {"side_side_inplane_tooth_keys", 6}, {"jupiter_scarf", 7}, {"side_side_inplane/side_removal", 8}, {"side_side_inplane_custom", 9},
    {"side_side_outplane_fingers", 10}, {"side_side_outplane_graded_fingers", 11}, {"side_side_outplane_three_fingers", 12}, {"side_side_outplane_miter_tenon", 13}, {"side_side_outplane_tenons", 14}, {"side_side_outplane_linked_tenons", 15}, {"side_side_outplane_linked_tenons_wall", 16}, {"side_side_outplane/side_removal", 18}, {"side_side_outplane_custom", 19},
    {"top_side_tenons", 20}, {"top_side_dense_tenons", 21}, {"top_side_milled_wedge", 24}, {"top_side_snap_fit", 25}, {"top_side/side_removal", 28}, {"top_side_custom", 29},
    {"cross_slot", 30}, {"cross_sliced_half_lap", 31}, {"cross_half_lap", 32}, {"cross_half_lap_diagonal_drills", 33}, {"cross_half_lap_drill", 34}, {"cross_brussels_half_lap", 35}, {"cross/side_removal", 38}, {"cross_custom", 39},
    {"top_top_centre_pin", 40}, {"top_top_polylabel_pin", 41}, {"top_top_circle_pins", 42}, {"top_top_ring_pins", 43}, {"top_top_grid_pins", 44}, {"top_top_rectangle_pins", 45}, {"top_top/side_removal", 48}, {"top_top_custom", 49},
    {"side_side_rotated_keys", 54}, {"side_side_rotated_hooks", 55}, {"side_side_rotated_miter", 56}, {"side_side_rotated/side_removal", 57}, {"side_side_rotated/side_removal_merged", 58}, {"side_side_rotated_custom", 59},
    {"border", 60}, {"border_custom", 69},
    // the same designs by their 2024 names
    {"ss_e_ip_1", 1}, {"ss_e_ip_0", 2}, {"ss_e_ip_2", 3}, {"ss_e_ip_3", 4}, {"ss_e_ip_4", 5}, {"ss_e_ip_5", 6}, {"ss_e_ip_custom", 9},
    {"ss_e_op_1", 10}, {"ss_e_op_2", 11}, {"ss_e_op_0", 12}, {"ss_e_op_3", 13}, {"ss_e_op_4", 14}, {"ss_e_op_5", 15}, {"ss_e_op_6", 16}, {"ss_e_op_custom", 19},
    {"ts_e_p_3", 20}, {"ts_e_p_2", 21}, {"ts_e_p_4", 24}, {"ts_e_p_5", 25}, {"ts_e_p_custom", 29},
    {"cr_c_ip_0", 30}, {"cr_c_ip_1", 31}, {"cr_c_ip_2", 32}, {"cr_c_ip_3", 33}, {"cr_c_ip_4", 34}, {"cr_c_ip_5", 35}, {"cr_c_ip_custom", 39},
    {"tt_e_p_0", 40}, {"tt_e_p_1", 41}, {"tt_e_p_2", 42}, {"tt_e_p_3", 43}, {"tt_e_p_4", 44}, {"tt_e_p_5", 45}, {"tt_e_p_custom", 49},
    {"ss_e_r_3", 54}, {"ss_e_r_2", 55}, {"ss_e_r_0", 56}, {"side_removal_ss_e_r_1", 58}, {"ss_e_r_custom", 59},
    {"b_0", 60}, {"b_custom", 69},
};

/// What a builder may read while it fills a InteractionFeaturePlate.
struct BuildContext {
    const Settings& settings;
    std::vector<std::shared_ptr<Plate>>& elements;
    std::vector<InteractionFeaturePlate>& all_joints;
};

/// Family of an id by tens; -1 outside 1..69.
int family_of(const int id) {

    if (id < 1 || id > 69)
        return -1;

    return id < 10 ? 0 : id / 10;
}

/// The family a detected joint type falls in: 11 -> 1, 12 -> 0, 13 -> 5, 20 -> 2, 30 -> 3, 40 -> 4, 60 -> 6; -1 otherwise.
int family_of_type(const int joint_type) {
    switch (joint_type) {
    case 11:
        return 1;
    case 12:
        return 0;
    case 13:
        return 5;
    case 20:
        return 2;
    case 30:
        return 3;
    case 40:
        return 4;
    case 60:
        return 6;
    default:
        return -1;
    }
}

/// Whether a per-face joint id falls in the id range of the detected joint type; -1 always matches.
bool id_matches(const int joint_type, const int id) {
    return id == -1 || family_of(id) == family_of_type(joint_type);
}

/// The library: the builder of every id the JOINTS_TYPES table names; false for an id without one, which takes its family's default below.
static bool build_inplane(const int id, InteractionFeaturePlate& joint, BuildContext& context) {

    switch (id) {
    case 1:
        side_side_inplane_dovetails(joint);
        return true;
    case 2:
        side_side_inplane_three_fingers(joint);
        return true;
    case 3:
        side_side_inplane_butterfly_keys(joint, context.elements);
        return true;
    case 4:
        side_side_inplane_splayed_scarf(joint);
        return true;
    case 5:
        side_side_inplane_cross_splayed_scarf(joint);
        return true;
    case 6:
        side_side_inplane_tooth_keys(joint, context.elements);
        return true;
    case 7:
        jupiter_scarf(joint);
        return true;
    case 8:
        side_removal(joint, context.elements);
        return true;
    case 9:
        side_side_inplane_custom(joint, context.settings);
        return true;
    default:
        return false;
    }
}

static bool build_outofplane(const int id, InteractionFeaturePlate& joint, BuildContext& context) {

    switch (id) {
    case 10:
        side_side_outplane_fingers(joint);
        return true;
    case 11:
        side_side_outplane_graded_fingers(joint);
        return true;
    case 12:
        side_side_outplane_three_fingers(joint);
        return true;
    case 13:
        side_side_outplane_miter_tenon(joint);
        return true;
    case 14:
        side_side_outplane_tenons(joint);
        return true;
    case 15:
        side_side_outplane_linked_tenons(joint, context.all_joints, false);
        return true;
    case 16:
        side_side_outplane_linked_tenons_wall(joint, context.all_joints);
        return true;
    case 18:
        side_removal(joint, context.elements);
        return true;
    case 19:
        side_side_outplane_custom(joint, context.settings);
        return true;
    default:
        return false;
    }
}

static bool build_topside(const int id, InteractionFeaturePlate& joint, BuildContext& context) {

    switch (id) {
    case 20:
        top_side_tenons(joint);
        return true;
    case 21:
        top_side_dense_tenons(joint);
        return true;
    case 22:
        top_side_tenons(joint);
        return true;
    case 23: // 2024 ran top_side_three_tenons here and fell through into top_side_milled_wedge, which overwrote all of it
    case 24:
        top_side_milled_wedge(joint);
        return true;
    case 25:
        top_side_snap_fit(joint);
        return true;
    case 28:
        side_removal(joint, context.elements);
        return true;
    case 29:
        top_side_custom(joint, context.settings);
        return true;
    default:
        return false;
    }
}

static bool build_cross(const int id, InteractionFeaturePlate& joint, BuildContext& context) {

    switch (id) {
    case 30:
        cross_slot(joint);
        return true;
    case 31:
        cross_sliced_half_lap(joint);
        return true;
    case 32:
        cross_half_lap(joint);
        return true;
    case 33:
        cross_half_lap_diagonal_drills(joint);
        return true;
    case 34:
        cross_half_lap_drill(joint);
        return true;
    case 35:
        cross_brussels_half_lap(joint);
        return true;
    case 38:
        side_removal(joint, context.elements);
        return true;
    case 39:
        cross_custom(joint, context.settings);
        return true;
    default:
        return false;
    }
}

static bool build_toptop(const int id, InteractionFeaturePlate& joint, BuildContext& context) {

    switch (id) {
    case 40:
        top_top_centre_pin(joint, context.elements);
        return true;
    case 41:
        top_top_polylabel_pin(joint, context.elements);
        return true;
    case 42:
        top_top_circle_pins(joint, context.elements);
        return true;
    case 43:
        top_top_ring_pins(joint, context.elements, context.settings, 0.0);
        return true;
    case 44:
        top_top_grid_pins(joint, context.elements);
        return true;
    case 45:
        top_top_rectangle_pins(joint, context.elements, context.settings);
        return true;
    case 48:
        side_removal(joint, context.elements);
        return true;
    case 49:
        top_top_custom(joint, context.settings);
        return true;
    default:
        return false;
    }
}

static bool build_rotated(const int id, InteractionFeaturePlate& joint, BuildContext& context) {

    switch (id) {
    case 54:
        side_side_rotated_keys(joint, context.elements);
        return true;
    case 55:
        side_side_rotated_hooks(joint, context.elements);
        return true;
    case 56:
        side_side_rotated_miter(joint);
        return true;
    case 57:
        side_removal(joint, context.elements);
        return true;
    case 58:
        side_removal(joint, context.elements, true);
        return true;
    case 59:
        side_side_rotated_custom(joint, context.settings);
        return true;
    default:
        return false;
    }
}

static bool build_beam(const int id, InteractionFeaturePlate& joint, BuildContext& context) {

    switch (id) {
    case 60:
        border(joint);
        return true;
    case 69:
        border_custom(joint, context.settings);
        return true;
    default:
        return false;
    }
}

bool build_joint(const int id, InteractionFeaturePlate& joint, BuildContext& context) {

    switch (family_of(id)) {
    case 0:
        return build_inplane(id, joint, context);
    case 1:
        return build_outofplane(id, joint, context);
    case 2:
        return build_topside(id, joint, context);
    case 3:
        return build_cross(id, joint, context);
    case 4:
        return build_toptop(id, joint, context);
    case 5:
        return build_rotated(id, joint, context);
    case 6:
        return build_beam(id, joint, context);
    default:
        return false;
    }
}

/// The builder a family falls back to for an id it has no entry for, as 2024 fell back.
void build_family_default(const int family, InteractionFeaturePlate& joint, BuildContext& context) {
    switch (family) {
    case 0:
        side_side_inplane_dovetails(joint);
        return;
    case 1:
        side_side_outplane_fingers(joint);
        return;
    case 2:
        top_side_tenons(joint);
        return;
    case 3:
        cross_slot(joint);
        return;
    case 4:
        top_top_centre_pin(joint, context.elements);
        return;
    case 5:
        side_removal(joint, context.elements);
        return;
    case 6:
        border(joint);
        return;
    default:
        return;
    }
}

/// Unit joinery geometry for `id`: the library entry, else the family default; a negative id in the out-of-plane family is the linked variant. Nothing for id 0 or an id outside the detected type's family.
void joint_create_geometry(
    InteractionFeaturePlate& joint,
    const double division_distance,
    const double shift_param,
    const int id,
    BuildContext& context
) {

    joint_get_divisions(joint, division_distance);
    joint.shift = shift_param;

    if (id == 0 || !id_matches(joint.joint_type, id))
        return;

    if (build_joint(id, joint, context))
        return;

    const int family = family_of(id) >= 0 ? family_of(id) : family_of_type(joint.joint_type);
    if (family == 1 && id < 0) {
        side_side_outplane_linked_tenons(joint, context.all_joints, false);
        return;
    }

    if (family < 0) {
        if (joint.joint_type == 11 || joint.joint_type == 12)
            side_side_outplane_fingers(joint);
        else if (joint.joint_type == 20)
            top_side_tenons(joint);
        return;
    }

    build_family_default(family, joint, context);
}

// ═══════════════════════════════════════════════════════════════════════════
// Joint types and parameters
// ═══════════════════════════════════════════════════════════════════════════

/// Family lookup for one joint: representing id, division length and shift.
struct FamilyParameters {
    int id;                   // The id the JOINTS_TYPES table gives the joint, or the family default.
    double division_distance; // Division length of the family.
    double shift;             // Shift of the family.
};

/// Wood's id_representing_joint_name: max of the two face ids in the JOINTS_TYPES table, -1 when the table says nothing.
int joint_id_for(
    const InteractionFeaturePlate& joint,
    const std::vector<std::vector<int>>& per_element_joints_types,
    const std::vector<std::shared_ptr<Plate>>& elements) {

    int id_representing_joint_name = -1;
    if (!per_element_joints_types.empty()) {

        const int element0 = index_of_plate(elements, joint.element_a);
        const int element1 = index_of_plate(elements, joint.element_b);
        const int face0 = joint.contact.face_a;
        const int face1 = joint.contact.face_b;
        const int original_face0 = element0 >= 0 && element0 < (int)elements.size() ? elements[element0]->given_face(face0) : face0;
        const int original_face1 = element1 >= 0 && element1 < (int)elements.size() ? elements[element1]->given_face(face1) : face1;
        const int id0 = (element0 >= 0 && element0 < (int)per_element_joints_types.size() && original_face0 >= 0 && original_face0 < (int)per_element_joints_types[element0].size())
                            ? std::abs(per_element_joints_types[element0][original_face0])
                            : 0;
        const int id1 = (element1 >= 0 && element1 < (int)per_element_joints_types.size() && original_face1 >= 0 && original_face1 < (int)per_element_joints_types[element1].size())
                            ? std::abs(per_element_joints_types[element1][original_face1])
                            : 0;

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
    case 11:
        return 1;
    case 12:
        return 0;
    case 13:
        return 5;
    case 20:
        return 2;
    case 30:
        return 3;
    case 40:
        return 4;
    case 60:
        return 6;
    default:
        return 1;
    }
}

/// Built-in joint parameters: division length, shift and joint id per family row.
constexpr double PARAMETER_DEFAULTS[21] = {
    300,
    0.5,
    3,
    450,
    0.64,
    15,
    450,
    0.5,
    20,
    300,
    0.5,
    30,
    6,
    0.95,
    40,
    300,
    0.5,
    58,
    300,
    1.0,
    60,
};

/// One joint_parameters entry, from the settings when they are complete and from the built-in defaults otherwise.
double joint_parameter(const std::vector<double>& parameters_global, bool parameters_ok, size_t index) {
    return parameters_ok ? parameters_global[index] : PARAMETER_DEFAULTS[index];
}

/// Per-family row of settings.joint_parameters: division length, shift and the default id when none was given.
FamilyParameters family_parameters(const Settings& settings, const int joint_type, const int id_representing_joint_name) {

    const std::vector<double>& parameters_global = settings.joint_parameters;
    const bool parameters_ok = parameters_global.size() >= 21;

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
    std::string name;                                         // Joint name the constructor gave.
    std::array<std::vector<Polyline>, 2> male_outlines;       // Male outlines, top and bottom.
    std::array<std::vector<Polyline>, 2> female_outlines;     // Female outlines, top and bottom.
    std::array<std::vector<int>, 2> male_fabrication_types;   // Male cut types, top and bottom.
    std::array<std::vector<int>, 2> female_fabrication_types; // Female cut types, top and bottom.
    bool unit_scale;                                          // Whether the constructor scales the unit cube.
};

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
std::string joint_cache_key(const int id_representing_joint_name, const InteractionFeaturePlate& joint) {
    return std::to_string(id_representing_joint_name) + ";" + cache_key_number(joint.shift) + ";" + cache_key_number((double)joint.divisions);
}

/// Unit geometry from the cache when the key is known, else from the constructor and cached afterwards, as 2024 reused it: every joint without linked joints reads the cache, every joint that is oriented writes it (a side removal is not), and what is transferred is the geometry, the types and the unit scale flag, never the unit scale distance, which stays the joint's own. The reuse is part of the reference: a unit-scale design built on the first joint's volume serves every joint of its key, as top_to_side_snap_fit shows.
void reuse_or_create_geometry(
    InteractionFeaturePlate& joint,
    const FamilyParameters& family,
    BuildContext& context,
    std::map<std::string, CachedJointGeometry>& unique_joints_cache) {

    const std::string cache_key = joint_cache_key(family.id, joint);
    const bool use_cache = joint.linked_joints.empty();

    const auto cache_entry = use_cache ? unique_joints_cache.find(cache_key) : unique_joints_cache.end();
    if (cache_entry != unique_joints_cache.end()) {
        const CachedJointGeometry& cached = cache_entry->second;
        joint.name = cached.name;
        joint.male_outlines = cached.male_outlines;
        joint.female_outlines = cached.female_outlines;
        joint.male_fabrication_types = cached.male_fabrication_types;
        joint.female_fabrication_types = cached.female_fabrication_types;
        joint.unit_scale = cached.unit_scale;
        return;
    }

    joint_create_geometry(
        joint,
        family.division_distance,
        family.shift,
        family.id,
        context
    );

    if (!use_cache || joint.no_orient)
        return;

    CachedJointGeometry cached;
    cached.name = joint.name;
    cached.male_outlines = joint.male_outlines;
    cached.female_outlines = joint.female_outlines;
    cached.male_fabrication_types = joint.male_fabrication_types;
    cached.female_fabrication_types = joint.female_fabrication_types;
    cached.unit_scale = joint.unit_scale;
    unique_joints_cache.emplace(cache_key, std::move(cached));
}

/// One joint: scale and thickness, unit geometry, orientation to the connection area, merge of the linked shadows.
void build_feature_geometry(
    InteractionFeaturePlate& joint,
    const FamilyParameters& family,
    BuildContext& context,
    std::map<std::string, CachedJointGeometry>& unique_joints_cache) {

    std::vector<std::shared_ptr<Plate>>& elements = context.elements;
    std::vector<InteractionFeaturePlate>& all_joints = context.all_joints;
    joint.scale = context.settings.joint_scale;

    // the distance a unit-scale design keeps along the joint line, as 2024 pinned every joint: the first plate's thickness by scale[2]
    const int element_index = index_of_plate(elements, joint.element_a);
    if (element_index >= 0 && element_index < (int)elements.size())
        joint.unit_scale_distance = elements[element_index]->thickness * joint.scale[2];

    joint_get_divisions(joint, family.division_distance);
    joint.shift = family.shift;
    reuse_or_create_geometry(
        joint,
        family,
        context,
        unique_joints_cache
    );

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
    std::vector<InteractionFeaturePlate>& all_joints,
    std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<std::vector<int>>& per_element_joints_types,
    const Settings& settings) {

    std::map<std::string, CachedJointGeometry> unique_joints_cache;
    BuildContext context{settings, elements, all_joints};

    for (InteractionFeaturePlate& joint : all_joints) {

        const int id_representing_joint_name = joint_id_for(joint, per_element_joints_types, elements);
        const FamilyParameters family = family_parameters(settings, joint.joint_type, id_representing_joint_name);
        if (joint.link)
            continue;

        build_feature_geometry(
            joint,
            family,
            context,
            unique_joints_cache
        );
    }
}

}

namespace wood_session {

JointPlate::JointPlate() {
    name = "JointPlate";
}
JointPlate::JointPlate(const InteractionFeaturePlate& connection) : connections{connection} {
    name = connection.name;
}
JointPlate::JointPlate(int variant, double division_distance, double shift)
    : variant(variant), division_distance(division_distance), shift(shift) {
    name = "JointPlate";
}
JointPlate::JointPlate(const std::shared_ptr<Plate>& source, const std::shared_ptr<Plate>& target,
                       const InteractionContactFace& contact, int variant, double division_distance,
                       double shift, const Settings& settings)
    : JointPlate(variant, division_distance, shift) {
    if (!source || !target)
        throw std::invalid_argument("Missing joint plates");

    orient(std::make_shared<InteractionContactFace>(contact), {source, target}, settings);
}
int JointPlate::library_id(const std::string& name) {

    const auto found = LIBRARY_IDS.find(name);
    if (found != LIBRARY_IDS.end())
        return found->second;

    if (name == "top_side_three_tenons" || name == "top_side_annen_tenons" || name == "side_side_outplane_capped_fingers" || name == "side_side_outplane_notch")
        throw std::invalid_argument(fmt::format("{} has no id in 2024's tables: it is reached by name only, through its JointPlate constructor", name));
    throw std::invalid_argument(fmt::format("no joint design is named {}", name));
}

std::shared_ptr<JointPlate> JointPlate::side_to_top(int variant, double division_distance, double shift) {
    if (variant < 20 || variant > 29)
        throw std::invalid_argument("Expected a side-to-top joint variant (20..29)");
    return std::make_shared<JointPlate>(variant, division_distance, shift);
}
void JointPlate::construct(InteractionFeaturePlate connection, const std::function<void(InteractionFeaturePlate&)>& builder) {
    if (!builder)
        throw std::invalid_argument("A custom joint needs a builder");
    joint_get_divisions(connection, division_distance);
    connection.shift = shift;
    builder(connection);
    if (!connection.no_orient)
        joint_orient_to_connection_area(connection);
    name = connection.name;
    connections = {std::move(connection)};
    invalidate_geometry();
}
/// The side-side family the detector gives a pair, for a joint of no family of its own: 13 rotated when the scene reads every
/// side-side contact so or the two alignment lines are not parallel within settings.angle, else by the dihedral angle between the
/// plates at their averaged line, 11 out of plane up to settings.dihedral_angle, 12 in plane beyond it; 11 without the plates.
static int side_side_family(const InteractionContactFace& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) {

    if (settings.all_treated_as_rotated)
        return 13;

    const Vector v0 = contact.lines[0].to_vector();
    const Vector v1 = contact.lines[1].to_vector();
    const double lengths = v0.magnitude() * v1.magnitude();
    if (lengths <= 0.0 || std::abs(v0.dot(v1)) / lengths < std::cos(settings.angle))
        return 13;

    if (elements.size() != 2)
        return 11;

    Line average = contact.lines[0];
    contact.lines[0].overlap_average(contact.lines[1], average);
    const Point centre_0 = Point::mid_point(elements[0]->polylines[0].center(), elements[0]->polylines[1].center());
    const Point centre_1 = Point::mid_point(elements[1]->polylines[0].center(), elements[1]->polylines[1].center());
    const double dihedral = Point::dihedral_angle_deg(average.start(), average.end(), centre_0, centre_1);

    return dihedral <= settings.dihedral_angle ? 11 : 12;
}

/// Resolves the library family from the selected joint and checks the contact topology; a joint of no family on a side-side
/// contact takes the family the detector would give the pair.
static int plate_contact_family(const JointPlate& joint, const InteractionContactFace& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) {

    const ContactType type = contact.type;
    int family = joint.parameters.contact_type;
    if (family == 0 && joint.parameters.library.empty()) {
        const int index = family_of(joint.variant);
        const std::array<int, 7> families = {12, 11, 20, 30, 40, 13, 60};
        if (index >= 0)
            family = families[index];
    }
    if (family == 0) {
        if (type == ContactType::side_side)
            family = side_side_family(contact, elements, settings);
        else if (type == ContactType::side_top)
            family = 20;
        else if (type == ContactType::top_top)
            family = 40;
        else if (type == ContactType::border)
            family = 60;
    }

    const bool compatible = ((family == 11 || family == 12 || family == 13) && type == ContactType::side_side)
        || (family == 20 && type == ContactType::side_top)
        || (family == 40 && type == ContactType::top_top)
        || (family == 60 && type == ContactType::border);
    if (!compatible)
        throw std::invalid_argument("The selected joint does not support this contact type");

    return family;
}

void JointPlate::orient(const std::shared_ptr<InteractionContactFace>& contact, const Settings& settings) {

    orient(contact, std::vector<std::shared_ptr<Plate>>(), settings);
}

void JointPlate::orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) {
    if (!contact)
        throw std::invalid_argument("JointPlate needs a face contact");
    // a border contact has its one plate, every other face contact its two in contact order
    const size_t plates = contact->type == ContactType::border ? 1 : 2;
    if (!elements.empty() && (elements.size() != plates || !elements[0] || !elements.back()))
        throw std::invalid_argument(plates == 1 ? "A border joint needs its one plate" : "A face joint needs two plates in contact order");

    // the male is the plate the detector designates: the second of an out-of-plane pair, the side-face one of a side-top pair, else the first
    const int family = plate_contact_family(*this, *contact, elements, settings);
    const bool reverse = family == 11 || (contact->type == ContactType::side_top && contact->face_a < 2);
    InteractionFeaturePlate connection;
    connection.contact = reverse ? *std::dynamic_pointer_cast<InteractionContactFace>(contact->flipped()) : *contact;
    connection.joint_type = family;
    connection.joint_lines = connection.contact.lines;
    for (int k = 0; k < 4; ++k)
        if (connection.contact.volumes[k].point_count() >= 4)
            connection.joint_volumes[k] = connection.contact.volumes[k];
    if (elements.size() == 2) {
        connection.element_a = elements[reverse ? 1 : 0]->guid();
        connection.element_b = elements[reverse ? 0 : 1]->guid();
    }
    // a border joint has one plate, both ends of its edge
    if (elements.size() == 1 && contact->type == ContactType::border) {
        connection.element_a = elements[0]->guid();
        connection.element_b = elements[0]->guid();
    }
    construct(std::move(connection), settings, elements);
}
void JointPlate::orient(const std::shared_ptr<InteractionContactCross>& contact, const Settings& settings) {

    orient(contact, std::vector<std::shared_ptr<Plate>>(), settings);
}

void JointPlate::orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) {
    if (!contact)
        throw std::invalid_argument("JointPlate needs a cross contact");
    InteractionFeaturePlate connection;
    connection.contact = InteractionContactFace(
        contact->faces_a[0],
        contact->faces_b[0],
        ContactType::unknown,
        contact->polygon
    );
    connection.cross_faces = {contact->faces_a[1], contact->faces_b[1]};
    connection.joint_type = 30;
    for (int k = 0; k < 2; ++k) {
        if (contact->lines[k].point_count() < 2)
            throw std::invalid_argument("Missing cross contact line");
        connection.joint_lines[k] = Line::from_points(contact->lines[k][0], contact->lines[k][1]);
        connection.joint_volumes[k] = contact->volumes[k];
    }
    if (elements.size() == 2) {
        connection.element_a = elements[0]->guid();
        connection.element_b = elements[1]->guid();
    }
    construct(std::move(connection), settings, elements);
}
std::shared_ptr<InteractionFeaturePlate> JointPlate::interaction_feature(int side, size_t index) const {
    if (side < 0 || side > 1)
        throw std::out_of_range("Joint side must be 0 or 1");
    if (connections.empty())
        throw std::logic_error("JointPlate has no connection: call orient(contact) before interaction_feature()");
    if (index >= connections.size())
        throw std::out_of_range("JointPlate connection index is out of range");

    const InteractionFeaturePlate& connection = connections[index];
    const std::string id = connection.feature_guid(side);
    const std::shared_ptr<InteractionFeaturePlate> feature = std::make_shared<InteractionFeaturePlate>(connection);
    feature->target_side = side + 1;
    feature->guid() = id;
    return feature;
}

std::shared_ptr<Interaction> JointPlate::interaction(size_t target) const {
    return interaction_feature(static_cast<int>(target % 2), target / 2);
}

size_t JointPlate::interaction_count() const {
    return 2 * connections.size();
}

const std::string& JointPlate::interaction_target(size_t target) const {

    if (target >= interaction_count())
        throw std::out_of_range("JointPlate interaction target is out of range");

    const InteractionFeaturePlate& connection = connections[target / 2];
    return target % 2 == 0 ? connection.element_a : connection.element_b;
}

/// The direction across a seam within a run's face, the run's normal crossed with the seam: taken from one run of a design, it measures every run of it on one scale.
static Vector seam_across(const Polyline& run, const Polyline& marker) {

    const Vector along = (marker[1] - marker[0]).normalized();
    return compute_newell(run.get_points()).cross(along);
}

/// The signed offsets of a run's points from its seam marker along a direction across it: zero on the seam, one sign per side of it.
static std::vector<double> seam_offsets(const std::vector<Point>& points, const Polyline& marker, const Vector& across) {

    std::vector<double> offsets;
    offsets.reserve(points.size());

    for (const Point& point : points)
        offsets.push_back((point - marker[0]).dot(across));

    return offsets;
}

/// The side of the seam a run keeps to, the sign of its farthest offset: zero for a run that crosses the seam or lies on it.
static int seam_side(const std::vector<double>& offsets) {

    double lowest = 0.0;
    double highest = 0.0;

    for (const double offset : offsets) {
        lowest = std::min(lowest, offset);
        highest = std::max(highest, offset);
    }

    if (lowest < -Tolerance::APPROXIMATION && highest > Tolerance::APPROXIMATION)
        return 0;
    if (highest > Tolerance::APPROXIMATION)
        return 1;
    if (lowest < -Tolerance::APPROXIMATION)
        return -1;
    return 0;
}

/// The pockets of a run that keeps to one side of its seam, one closed loop per stretch between two touches of the seam: a tiled key touches it between its teeth, and one loop around them all would run back along the seam over its own edges. Empty for a run that crosses the seam, the fingers of a plate.
static std::vector<Polyline> seam_pockets(const Polyline& run, const Polyline& marker) {

    const std::vector<Point> points = run.get_points();
    const std::vector<double> offsets = seam_offsets(points, marker, seam_across(run, marker));

    if (seam_side(offsets) == 0)
        return {};

    std::vector<Polyline> pockets;
    std::vector<Point> group;
    bool along_seam = true;

    for (size_t i = 0; i < points.size(); i++) {
        const bool on_seam = std::abs(offsets[i]) < Tolerance::APPROXIMATION;

        if (on_seam && along_seam) {
            group = {points[i]};
            continue;
        }

        group.push_back(points[i]);
        along_seam = false;

        if (on_seam && group.size() >= 3) {
            pockets.push_back(Polyline(group).closed());
            group = {points[i]};
            along_seam = true;
        }
    }

    return pockets;
}

/// The pocket pairs, bottom and top, of one side of an in-plane design: its edge insertion run on both faces split at the seam; empty when the side is not a key pocket, its run crossing the seam or the two faces not pairing up.
static std::vector<std::array<Polyline, 2>> side_pockets(const std::array<std::vector<Polyline>, 2>& outlines, const std::array<std::vector<int>, 2>& types) {

    if (types[0].empty() || (types[0][0] != FabricationType::edge_insertion && types[0][0] != FabricationType::insert_between_multiple_edges))
        return {};

    for (int face = 0; face < 2; face++)
        if (outlines[face].size() < 2 || outlines[face][0].point_count() < 3 || outlines[face][1].point_count() != 2)
            return {};

    const std::vector<Polyline> bottom = seam_pockets(outlines[0][0], outlines[0][1]);
    const std::vector<Polyline> top = seam_pockets(outlines[1][0], outlines[1][1]);

    if (bottom.empty() || bottom.size() != top.size())
        return {};

    std::vector<std::array<Polyline, 2>> pairs;

    for (size_t i = 0; i < bottom.size(); i++)
        pairs.push_back({bottom[i], top[i]});

    return pairs;
}

/// The keys: an in-plane design whose male and female runs each stay on their own side of the seam leaves pockets in each plate that neither fills, so the joint is those loose pieces, every pocket closed along the seam. Two runs on the same side are one plate's tongue in the other's notch, no loose piece; fingers, tenons and holes belong to the plates, the solid cuts and drills to the session; none is drawn here.
std::vector<std::array<Polyline, 2>> JointPlate::bodies() const {

    std::vector<std::array<Polyline, 2>> result;
    for (const InteractionFeaturePlate& connection : connections) {

        if (connection.joint_type != 12 && connection.joint_type != 13)
            continue;

        const std::vector<std::array<Polyline, 2>> male = side_pockets(connection.male_outlines, connection.male_fabrication_types);
        const std::vector<std::array<Polyline, 2>> female = side_pockets(connection.female_outlines, connection.female_fabrication_types);

        if (male.empty() || female.empty())
            continue;

        // both bottom runs on the male run's scale: a key's pockets lie on opposite sides of the seam
        const Polyline& male_run = connection.male_outlines[0][0];
        const Polyline& female_run = connection.female_outlines[0][0];
        const Vector across = seam_across(male_run, connection.male_outlines[0][1]);
        const int male_side = seam_side(seam_offsets(male_run.get_points(), connection.male_outlines[0][1], across));
        const int female_side = seam_side(seam_offsets(female_run.get_points(), connection.female_outlines[0][1], across));

        if (male_side == female_side)
            continue;

        result.insert(result.end(), male.begin(), male.end());
        result.insert(result.end(), female.begin(), female.end());
    }

    return result;
}
std::vector<Line> JointPlate::drill_axes() const {
    std::vector<Line> result = drill_lines;
    for (const InteractionFeaturePlate& connection : connections) {
        for (int side = 0; side < 2; ++side) {
            const std::array<std::vector<Polyline>, 2>& outlines = side == 0 ? connection.male_outlines : connection.female_outlines;
            const std::array<std::vector<int>, 2>& types = side == 0 ? connection.male_fabrication_types : connection.female_fabrication_types;
            for (size_t i = 0; i < std::min(outlines[0].size(), types[0].size()); ++i) {
                const Polyline& line = outlines[0][i];
                if (!is_drill(types[0][i]) || line.point_count() != 2 || (line[1] - line[0]).magnitude_squared() <= 1e-12)
                    continue;
                // 2024 doubled every pair and both sides list a pin through both plates: each axis once, either way round
                bool listed = false;
                for (const Line& axis : result)
                    listed = listed || (axis.start() == line[0] && axis.end() == line[1]) || (axis.start() == line[1] && axis.end() == line[0]);
                if (!listed)
                    result.push_back(Line::from_points(line[0], line[1]));
            }
        }
    }
    return result;
}
/// True when the pair k of the side's outlines equals an earlier pair point for point: the second copy a builder pushes, as the 2024 library doubled every pair.
static bool outline_pair_repeated(const std::array<std::vector<Polyline>, 2>& outlines, size_t k) {

    for (size_t j = 0; j < k; j++)
        if (outlines[0][k].get_points() == outlines[0][j].get_points() && outlines[1][k].get_points() == outlines[1][j].get_points())
            return true;

    return false;
}

std::vector<Mesh> JointPlate::side_solids(const InteractionFeaturePlate& connection, int side) {

    const std::array<std::vector<Polyline>, 2>& outlines = side == 0 ? connection.male_outlines : connection.female_outlines;
    const std::array<std::vector<int>, 2>& types = side == 0 ? connection.male_fabrication_types : connection.female_fabrication_types;
    std::vector<Mesh> pieces;

    for (size_t k = 0; k < std::min({outlines[0].size(), outlines[1].size(), types[0].size()}); k++) {
        const Polyline& bottom = outlines[0][k];
        const Polyline& top = outlines[1][k];

        // the solid types are the contiguous run from slice to conic_reverse; the plate types merge into the outline instead
        const bool solid = types[0][k] >= FabricationType::slice && types[0][k] <= FabricationType::conic_reverse;
        if (!solid || outline_pair_repeated(outlines, k) || bottom.point_count() < 3 || bottom.point_count() != top.point_count())
            continue;

        const Mesh piece = loft_stations({bottom.closed(), top.closed()});
        if (manifold_solid(piece) && compute_volume(piece) > 0.0)
            pieces.push_back(piece);
    }

    return pieces;
}

std::vector<Line> JointPlate::side_drills(const InteractionFeaturePlate& connection, int side) {

    const std::array<std::vector<Polyline>, 2>& outlines = side == 0 ? connection.male_outlines : connection.female_outlines;
    const std::array<std::vector<int>, 2>& types = side == 0 ? connection.male_fabrication_types : connection.female_fabrication_types;
    std::vector<Line> drills;

    for (size_t k = 0; k < std::min({outlines[0].size(), outlines[1].size(), types[0].size()}); k++) {
        const Polyline& bottom = outlines[0][k];
        if (!is_drill(types[0][k]) || outline_pair_repeated(outlines, k))
            continue;
        if (bottom.point_count() == 2 && (bottom[1] - bottom[0]).magnitude_squared() > 1e-12)
            drills.push_back(Line::from_points(bottom[0], bottom[1]));
    }

    return drills;
}

void JointPlate::compute_key(const std::vector<std::shared_ptr<Plate>>& elements) {

    key = Mesh();
    for (const InteractionFeaturePlate& connection : connections) {
        if (connection.name != "side_side_rotated_hooks" && connection.name != "side_side_rotated_keys" && connection.name != "side_side_inplane_splayed_scarf" && connection.name != "side_side_inplane_cross_splayed_scarf" && connection.name != "top_top_custom" && connection.name != "jupiter_scarf")
            continue;

        // the Jupiter's key fills the slot both plates are notched around: the prism of its outline pair
        if (connection.name == "jupiter_scarf") {
            if (connection.male_outlines[0].size() > 2 && connection.male_outlines[1].size() > 2)
                key = Mesh::loft({connection.male_outlines[0][2]}, {connection.male_outlines[1][2]}, true);
            continue;
        }

        // the pieces the key fills: each side's milled pockets, and the in-plane notches its runs leave along the seam
        std::vector<Mesh> pieces;
        for (const std::array<Polyline, 2>& body : bodies())
            pieces.push_back(Mesh::loft({body[0]}, {body[1]}, true));

        // the rotated keys are the straight prisms 2024 milled, both sides' pockets united, not trimmed by the folded plates; a custom
        // top-top joint's milled pockets make its key the same way, the loose piece hidden between the stacked plates
        if (connection.name == "side_side_rotated_hooks" || connection.name == "side_side_rotated_keys" || connection.name == "top_top_custom") {
            for (int side = 0; side < 2; side++)
                for (const Mesh& piece : side_solids(connection, side))
                    key = key.number_of_faces() == 0 ? piece : solid_boolean(key, piece, SolidOperation::add);
            continue;
        }

        // the in-plane keys are what each plate loses to them, united into the one key, flush with the plates
        for (int side = 0; side < 2; side++) {
            const int index = index_of_plate(elements, side == 0 ? connection.element_a : connection.element_b);
            if (index < 0)
                continue;

            const Mesh& stock = elements[index]->element_geometry_mesh();
            std::vector<Mesh> side_pieces = side_solids(connection, side);
            side_pieces.insert(side_pieces.end(), pieces.begin(), pieces.end());
            for (const Mesh& piece : side_pieces) {
                const Mesh pocket = solid_boolean(stock, piece, SolidOperation::intersect);
                if (pocket.number_of_faces() > 0)
                    key = key.number_of_faces() == 0 ? pocket : solid_boolean(key, pocket, SolidOperation::add);
            }
        }
    }

    // a joint with a key shows it: the key is the joint's own piece
    if (key.number_of_faces() > 0)
        is_visible = true;
    invalidate_geometry();
}

/// A loop in a slab's frame (origin on the seam's top edge, x across the seam into the slab, y along it, z the top's normal), at bottom
/// and at top: its outline points in the x-y plane, moved down or up along z.
static std::array<Polyline, 2> hilti_prism(const Xform& frame, const std::vector<Point>& outline, double bottom, double top) {

    // an outline that starts at the neck's end repeats its first corner: each corner once
    std::vector<Point> corners;
    for (const Point& point : outline)
        if (corners.empty() || point.distance(corners.back()) > Tolerance::APPROXIMATION)
            corners.push_back(point);
    const Polyline loop = Polyline(corners).closed();

    return {loop.transformed(frame * Xform::translation(0.0, 0.0, bottom)), loop.transformed(frame * Xform::translation(0.0, 0.0, top))};
}

/// The half bow-tie of the 2024 Hilti joint (side_side_rotated_hooks, id 55) in a slab's frame, from start across the seam to end: the neck neck_width
/// wide to neck_end, the wing widening to wing_width at taper_end, straight on to end; its two far corners rounded by the router's
/// radius when one is given, quarters of the kernel's circle of sides.
static std::vector<Point> hilti_outline(double start, double end, double neck_end, double taper_end, double neck_width, double wing_width, double router_radius, int sides) {

    std::vector<Point> outline = {
        Point(start, -0.5 * neck_width, 0.0),
        Point(neck_end, -0.5 * neck_width, 0.0),
        Point(taper_end, -0.5 * wing_width, 0.0),
    };

    // the far side: a sharp corner, or a quarter of the router's circle at each corner from the wing's side to the end's
    if (router_radius <= 0.0) {
        outline.emplace_back(end, -0.5 * wing_width, 0.0);
        outline.emplace_back(end, 0.5 * wing_width, 0.0);
    } else {
        const int quarter = std::max(sides / 4, 1);
        const std::vector<Point> circle = Polyline::from_sides(4 * quarter, router_radius, false).get_points();
        for (int corner = 0; corner < 2; corner++) {
            const Point centre(end - router_radius, (corner == 0 ? -1.0 : 1.0) * (0.5 * wing_width - router_radius), 0.0);
            // the circle starts on +x and turns to +y: its fourth quarter runs from -y to +x, its first from +x to +y
            const int first = corner == 0 ? 3 * quarter : 0;
            for (int k = 0; k <= quarter; k++) {
                const Point& point = circle[(first + k) % (4 * quarter)];
                outline.emplace_back(centre[0] + point[0], centre[1] + point[1], 0.0);
            }
        }
    }

    outline.emplace_back(taper_end, 0.5 * wing_width, 0.0);
    outline.emplace_back(neck_end, 0.5 * neck_width, 0.0);
    outline.emplace_back(start, 0.5 * neck_width, 0.0);

    return outline;
}

/// A disc's two loops: the kernel's polygon of sides around the rod, from start to end along x, its centre height z.
static std::array<Polyline, 2> hilti_disc(const Xform& frame, double start, double end, double z, double radius, int sides) {

    // the circle's plane across the rod: its x along the seam, its y up, its normal along the rod
    const Xform across = Xform::from_axes(Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0));
    const Polyline circle = Polyline::from_sides(sides, radius, true);

    return {circle.transformed(frame * Xform::translation(start, 0.0, z) * across), circle.transformed(frame * Xform::translation(end, 0.0, z) * across)};
}

std::shared_ptr<JointPlate> JointPlate::side_side_inplane_hilti(
    const Plate& a,
    const Plate& b,
    const InteractionContactFace& contact,
    double half_length,
    double neck_length,
    double neck_width,
    double taper_end,
    double wing_width,
    double depth,
    double height,
    double rod_diameter,
    double disc_diameter,
    double disc_thickness,
    double router_radius,
    int sides
) {

    // the seam, of two slabs side to side: its face's normal from a into b, its direction along the longest edge of the contact
    const std::vector<Point> points = merge_collinear(contact.polygon);
    if (contact.type != ContactType::side_side || points.size() < 4)
        return nullptr;

    Vector x = compute_newell(points).normalized();
    const Point centre = Point::centroid(points);
    if ((b.element_geometry_mesh().centroid() - centre).dot(x) < 0.0)
        x = x * -1.0;

    Vector along;
    double longest = 0.0;
    for (size_t i = 0; i < points.size(); i++) {
        const Vector edge = points[(i + 1) % points.size()] - points[i];
        if (edge.magnitude() > longest) {
            longest = edge.magnitude();
            along = edge.normalized();
        }
    }
    if (longest <= 0.0)
        return nullptr;

    // the bow-tie's frame on the seam's top edge, the middle of the contact's two corners highest up, its z up between the two slabs' tops
    Vector z = x.cross(along).normalized();
    if (z.dot(Vector(0.0, 0.0, 1.0)) < 0.0)
        z = z * -1.0;
    std::vector<Point> corners = points;
    std::sort(corners.begin(), corners.end(), [&](const Point& p, const Point& q) { return (p - centre).dot(z) > (q - centre).dot(z); });
    const Point origin = corners[0] + (corners[1] - corners[0]) * 0.5;

    // the joint on the pair, its outlines in world space
    InteractionFeaturePlate connection;
    connection.name = "hilti";
    connection.element_a = a.guid();
    connection.element_b = b.guid();
    connection.contact = contact;
    connection.joint_type = 13;
    connection.no_orient = true;

    // per slab: its pocket milled from the top, and its half of the key with its disc, in the half's frame (x across the seam into the slab)
    const double part_top = height - depth;
    const double rod_z = part_top - 0.5 * height;
    const double neck_end = 0.5 * neck_length;
    std::array<Point, 2> rod_ends;
    std::vector<Mesh> key_pieces;
    for (int side = 0; side < 2; side++) {
        const Vector into = side == 0 ? x * -1.0 : x;
        const Xform frame = Xform::frame_to_world(origin, into, z.cross(into), z);

        // the pocket: the 2024 half bow-tie milled from the top, 1 mm past the seam so the cut shares no face with the seam face, its far
        // corners the router's, depth under the seam's top edge and up through the slab's top, which a fold only lowers
        const std::array<Polyline, 2> pocket = hilti_prism(frame, hilti_outline(-1.0, half_length, neck_end, taper_end, neck_width, wing_width, router_radius, sides), -depth, 1.0);
        std::array<std::vector<Polyline>, 2>& outlines = side == 0 ? connection.male_outlines : connection.female_outlines;
        std::array<std::vector<int>, 2>& types = side == 0 ? connection.male_fabrication_types : connection.female_fabrication_types;
        outlines[0].push_back(pocket[0]);
        outlines[1].push_back(pocket[1]);
        types[0].push_back(FabricationType::mill);
        types[1].push_back(FabricationType::mill);

        // the half: its neck from the seam and its wing, rounded like its pocket's corners, its top under the seam's top edge by what the
        // pocket is deeper than the half; the disc recessed in the wing's end
        const std::array<Polyline, 2> half = hilti_prism(frame, hilti_outline(0.0, half_length - disc_thickness, neck_end, taper_end, neck_width, wing_width, router_radius, sides), part_top - height, part_top);
        const std::array<Polyline, 2> disc = hilti_disc(frame, half_length - disc_thickness, half_length, rod_z, 0.5 * disc_diameter, sides);
        key_pieces.push_back(loft_stations({half[0], half[1]}));
        key_pieces.push_back(loft_stations({disc[0], disc[1]}));
        rod_ends[side] = Point(half_length, 0.0, rod_z).transformed(frame);
    }

    // the key: both halves and both discs, one solid, the bolt from disc to disc through it, inside the pockets
    const std::shared_ptr<JointPlate> joint = std::make_shared<JointPlate>();
    joint->name = "side_side_inplane_hilti";
    joint->is_visible = true;
    joint->line_radius = 0.5 * rod_diameter;
    joint->chord_tolerance = sides_tolerance(joint->line_radius, sides);
    joint->connections = {std::move(connection)};
    joint->drill_lines = {Line::from_points(rod_ends[0], rod_ends[1])};
    for (const Mesh& piece : key_pieces)
        joint->key = joint->key.number_of_faces() == 0 ? piece : solid_boolean(joint->key, piece, SolidOperation::add);
    joint->invalidate_geometry();

    return joint;
}

Mesh JointPlate::key_mesh() const {
    return key;
}

void JointPlate::place(const Xform& xform) {
    Joint::place(xform);
    key = key.transformed(xform);
    for (InteractionFeaturePlate& connection : connections) {
        connection.contact.polygon = connection.contact.polygon.transformed(xform);
        for (Line& line : connection.contact.lines)
            line = line.transformed(xform);
        for (Polyline& volume : connection.contact.volumes)
            volume = volume.transformed(xform);
        for (Line& line : connection.joint_lines)
            line = line.transformed(xform);
        for (std::optional<Polyline>& volume : connection.joint_volumes)
            if (volume)
                volume = volume->transformed(xform);
        for (std::array<std::vector<Polyline>, 2>* outlines : {&connection.male_outlines, &connection.female_outlines})
            for (std::vector<Polyline>& face : *outlines)
                face = transformed_list(face, xform);
        connection.sync_features();
    }
}

void JointPlate::build_geometry(std::vector<InteractionFeaturePlate>& connections,
                                std::vector<std::shared_ptr<Plate>>& elements,
                                const std::vector<std::vector<int>>& types, const Settings& settings) {
    build_features_geometry(
        connections,
        elements,
        types,
        settings
    );
}
void JointPlate::construct(InteractionFeaturePlate connection, const Settings& settings, std::vector<std::shared_ptr<Plate>> elements) {

    if (!parameters.library.empty()) {
        construct(std::vector<InteractionFeaturePlate>{std::move(connection)}, elements, settings);
        return;
    }

    if (!id_matches(connection.joint_type, variant))
        throw std::invalid_argument("Joint variant does not match the contact type");
    if (!std::isfinite(division_distance) || !std::isfinite(shift))
        throw std::invalid_argument("Joint parameters must be finite");
    std::vector<InteractionFeaturePlate> joints{std::move(connection)};
    BuildContext context{settings, elements, joints};
    std::map<std::string, CachedJointGeometry> cache;
    ::build_feature_geometry(
        joints[0],
        FamilyParameters{variant, division_distance, shift},
        context,
        cache
    );
    connections = std::move(joints);
    name = connections[0].name;
    compute_key(elements);
    invalidate_geometry();
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<JointPlate> JointPlate::from_library(const std::string& library, int contact_type) {

    const std::shared_ptr<JointPlate> joint = std::make_shared<JointPlate>();
    joint->name = library;
    joint->parameters.library = library;
    joint->parameters.contact_type = contact_type;

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

void JointPlate::compute_parameters(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) const {

    if (parameters.contact_type && connection.joint_type != parameters.contact_type)
        throw std::invalid_argument("The joint library constructor does not match the contact family");

    joint_get_divisions(connection, division_distance);
    connection.name = parameters.library;
    connection.scale = settings.joint_scale;
    connection.shift = shift;

    if (parameters.divisions > 0)
        connection.divisions = parameters.divisions;

    // the distance a unit-scale design keeps along the joint line, as 2024 pinned every joint: the first plate's thickness by scale[2]
    const int index = index_of_plate(elements, connection.element_a);
    if (index >= 0)
        connection.unit_scale_distance = elements[index]->thickness * connection.scale[2];
}

void JointPlate::compute_library(
    InteractionFeaturePlate& connection,
    const std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<InteractionFeaturePlate>& connections,
    const Settings& settings
) const {

    if (parameters.library.starts_with("side_side_inplane_")) {
        if (compute_side_side_inplane(connection, elements, settings))
            return;
    } else if (parameters.library.starts_with("side_side_outplane_")) {
        if (compute_side_side_outplane(connection, connections, settings))
            return;
    } else if (parameters.library.starts_with("top_side_")) {
        if (compute_top_side(connection, settings))
            return;
    } else if (parameters.library.starts_with("top_top_")) {
        if (compute_top_top(connection, elements, settings))
            return;
    } else if (parameters.library.starts_with("side_side_rotated_")) {
        if (compute_side_side_rotated(connection, elements, settings))
            return;
    } else if (parameters.library.starts_with("cross_")) {
        if (compute_cross(connection, settings))
            return;
    } else if (parameters.library.starts_with("border")) {
        if (compute_b(connection, settings))
            return;
    } else if (parameters.library.starts_with("side_removal") || parameters.library == "jupiter_scarf") {
        if (elements.size() < 2)
            throw std::invalid_argument("Side removal requires the participating plates in orient(contact, elements)");

        if (compute_side_removal(connection, elements))
            return;
    }

    throw std::invalid_argument("Unknown plate joint library constructor: " + parameters.library);
}

void JointPlate::construct(std::vector<InteractionFeaturePlate> joints, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) {

    Settings configuration = settings;

    if (parameters.library.ends_with("_custom"))
        configuration.custom_joints[parameters.library.substr(0, parameters.library.size() - 7)] = parameters.outlines;

    for (InteractionFeaturePlate& connection : joints)
        compute_parameters(connection, elements, configuration);

    for (InteractionFeaturePlate& connection : joints)
        if (!connection.link)
            compute_library(
                connection,
                elements,
                joints,
                configuration
            );

    for (InteractionFeaturePlate& connection : joints)
        if (!connection.no_orient)
            joint_orient_to_connection_area(connection);

    if (parameters.library == "side_side_outplane_linked_tenons" || parameters.library == "side_side_outplane_linked_tenons_wall")
        for (InteractionFeaturePlate& connection : joints)
            if (!connection.linked_joints.empty())
                merge_linked_joints(connection, joints);

    connections = std::move(joints);
    name = parameters.library;
    compute_key(elements);
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

/// An in-plane design on its 2024 family defaults: geometric divisions every 300 mm of the joint line, shift 0.5.
std::shared_ptr<JointPlate> JointPlate::side_side_inplane(const std::string& library) {

    const std::shared_ptr<JointPlate> joint = from_library(library, 12);
    joint->division_distance = 300.0;
    joint->shift = 0.5;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_inplane_three_fingers() {

    return side_side_inplane("side_side_inplane_three_fingers");
}

std::shared_ptr<JointPlate> JointPlate::side_side_inplane_dovetails(int divisions, double shift) {

    const std::shared_ptr<JointPlate> joint = side_side_inplane("side_side_inplane_dovetails");
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_inplane_butterfly_keys(int divisions) {

    const std::shared_ptr<JointPlate> joint = side_side_inplane("side_side_inplane_butterfly_keys");
    joint->parameters.divisions = divisions;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_inplane_splayed_scarf() {

    return side_side_inplane("side_side_inplane_splayed_scarf");
}

std::shared_ptr<JointPlate> JointPlate::side_side_inplane_cross_splayed_scarf() {

    return side_side_inplane("side_side_inplane_cross_splayed_scarf");
}

std::shared_ptr<JointPlate> JointPlate::side_side_inplane_tooth_keys(int divisions) {

    const std::shared_ptr<JointPlate> joint = side_side_inplane("side_side_inplane_tooth_keys");
    joint->parameters.divisions = divisions;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_inplane_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female) {

    const std::shared_ptr<JointPlate> joint = side_side_inplane("side_side_inplane_custom");
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool JointPlate::compute_side_side_inplane(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) const {

    if (parameters.library == "side_side_inplane_three_fingers")
        ::side_side_inplane_three_fingers(connection);
    else if (parameters.library == "side_side_inplane_dovetails")
        ::side_side_inplane_dovetails(connection);
    else if (parameters.library == "side_side_inplane_butterfly_keys")
        ::side_side_inplane_butterfly_keys(connection, elements);
    else if (parameters.library == "side_side_inplane_splayed_scarf")
        ::side_side_inplane_splayed_scarf(connection);
    else if (parameters.library == "side_side_inplane_cross_splayed_scarf")
        ::side_side_inplane_cross_splayed_scarf(connection);
    else if (parameters.library == "side_side_inplane_tooth_keys")
        ::side_side_inplane_tooth_keys(connection, elements);
    else if (parameters.library == "side_side_inplane_custom")
        ::side_side_inplane_custom(connection, settings);
    else
        return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

/// An out-of-plane design on its 2024 family defaults: geometric divisions every 450 mm of the joint line, shift 0.64.
std::shared_ptr<JointPlate> JointPlate::side_side_outplane(const std::string& library) {

    const std::shared_ptr<JointPlate> joint = from_library(library, 11);
    joint->division_distance = 450.0;
    joint->shift = 0.64;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_outplane_three_fingers() {

    return side_side_outplane("side_side_outplane_three_fingers");
}

std::shared_ptr<JointPlate> JointPlate::side_side_outplane_fingers(int divisions, double shift) {

    const std::shared_ptr<JointPlate> joint = side_side_outplane("side_side_outplane_fingers");
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_outplane_graded_fingers(int divisions, double shift) {

    const std::shared_ptr<JointPlate> joint = side_side_outplane("side_side_outplane_graded_fingers");
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_outplane_miter_tenon() {

    return side_side_outplane("side_side_outplane_miter_tenon");
}

std::shared_ptr<JointPlate> JointPlate::side_side_outplane_tenons(
    int divisions,
    double taper,
    bool chamfer,
    bool modify_outline,
    const std::array<double, 2>& x,
    const std::array<double, 2>& y,
    const std::array<double, 2>& z
) {

    const std::shared_ptr<JointPlate> joint = side_side_outplane("side_side_outplane_tenons");
    joint->parameters.divisions = divisions;
    joint->parameters.taper = taper;
    joint->parameters.chamfer = chamfer;
    joint->parameters.modify_outline = modify_outline;
    joint->parameters.x = x;
    joint->parameters.y = y;
    joint->parameters.z = z;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_outplane_linked_tenons(int divisions, bool disable_divisions) {

    const std::shared_ptr<JointPlate> joint = side_side_outplane("side_side_outplane_linked_tenons");
    joint->parameters.divisions = divisions;
    joint->parameters.disable_divisions = disable_divisions;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_outplane_linked_tenons_wall(int divisions) {

    const std::shared_ptr<JointPlate> joint = side_side_outplane("side_side_outplane_linked_tenons_wall");
    joint->parameters.divisions = divisions;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_outplane_capped_fingers(int divisions) {

    const std::shared_ptr<JointPlate> joint = side_side_outplane("side_side_outplane_capped_fingers");
    joint->parameters.divisions = divisions;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_outplane_notch() {

    return side_side_outplane("side_side_outplane_notch");
}

std::shared_ptr<JointPlate> JointPlate::side_side_outplane_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female) {

    const std::shared_ptr<JointPlate> joint = side_side_outplane("side_side_outplane_custom");
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool JointPlate::compute_side_side_outplane(InteractionFeaturePlate& connection, std::vector<InteractionFeaturePlate>& connections, const Settings& settings) const {

    if (parameters.library == "side_side_outplane_three_fingers")
        ::side_side_outplane_three_fingers(connection);
    else if (parameters.library == "side_side_outplane_fingers")
        ::side_side_outplane_fingers(connection);
    else if (parameters.library == "side_side_outplane_graded_fingers")
        ::side_side_outplane_graded_fingers(connection);
    else if (parameters.library == "side_side_outplane_miter_tenon")
        ::side_side_outplane_miter_tenon(connection);
    else if (parameters.library == "side_side_outplane_tenons")
        ::side_side_outplane_tenons(
            connection,
            parameters.taper,
            parameters.chamfer,
            parameters.modify_outline,
            parameters.x[0],
            parameters.x[1],
            parameters.y[0],
            parameters.y[1],
            parameters.z[0],
            parameters.z[1]
        );
    else if (parameters.library == "side_side_outplane_linked_tenons")
        ::side_side_outplane_linked_tenons(connection, connections, parameters.disable_divisions);
    else if (parameters.library == "side_side_outplane_linked_tenons_wall")
        ::side_side_outplane_linked_tenons_wall(connection, connections);
    else if (parameters.library == "side_side_outplane_capped_fingers")
        ::side_side_outplane_capped_fingers(connection);
    else if (parameters.library == "side_side_outplane_notch")
        ::side_side_outplane_notch(connection);
    else if (parameters.library == "side_side_outplane_custom")
        ::side_side_outplane_custom(connection, settings);
    else
        return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

// A top-side design on its 2024 family defaults, the class defaults: geometric divisions every 450 mm of the joint line, shift 0.5.

std::shared_ptr<JointPlate> JointPlate::top_side_three_tenons() {

    return from_library("top_side_three_tenons", 20);
}

std::shared_ptr<JointPlate> JointPlate::top_side_annen_tenons() {

    return from_library("top_side_annen_tenons", 20);
}

std::shared_ptr<JointPlate> JointPlate::top_side_dense_tenons(int divisions, double shift) {

    const std::shared_ptr<JointPlate> joint = from_library("top_side_dense_tenons", 20);
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::top_side_tenons(int divisions, double shift) {

    const std::shared_ptr<JointPlate> joint = from_library("top_side_tenons", 20);
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::top_side_milled_wedge() {

    return from_library("top_side_milled_wedge", 20);
}

std::shared_ptr<JointPlate> JointPlate::top_side_snap_fit(int divisions) {

    const std::shared_ptr<JointPlate> joint = from_library("top_side_snap_fit", 20);
    joint->parameters.divisions = divisions;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::top_side_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female) {

    const std::shared_ptr<JointPlate> joint = from_library("top_side_custom", 20);
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool JointPlate::compute_top_side(InteractionFeaturePlate& connection, const Settings& settings) const {

    if (parameters.library == "top_side_three_tenons")
        ::top_side_three_tenons(connection);
    else if (parameters.library == "top_side_annen_tenons")
        ::top_side_annen_tenons(connection);
    else if (parameters.library == "top_side_dense_tenons")
        ::top_side_dense_tenons(connection);
    else if (parameters.library == "top_side_tenons")
        ::top_side_tenons(connection);
    else if (parameters.library == "top_side_milled_wedge")
        ::top_side_milled_wedge(connection);
    else if (parameters.library == "top_side_snap_fit")
        ::top_side_snap_fit(connection);
    else if (parameters.library == "top_side_custom")
        ::top_side_custom(connection, settings);
    else
        return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

// A top-top design on its 2024 family defaults: division length 6 and shift 0.95, each read the design's own way; the radius is the pin's.

std::shared_ptr<JointPlate> JointPlate::top_top_centre_pin(double radius) {

    const std::shared_ptr<JointPlate> joint = from_library("top_top_centre_pin", 40);
    joint->line_radius = radius;
    joint->division_distance = 6.0;
    joint->shift = 0.95;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::top_top_polylabel_pin(double radius) {

    const std::shared_ptr<JointPlate> joint = from_library("top_top_polylabel_pin", 40);
    joint->line_radius = radius;
    joint->division_distance = 6.0;
    joint->shift = 0.95;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::top_top_circle_pins(int divisions, double shift, double radius) {

    const std::shared_ptr<JointPlate> joint = from_library("top_top_circle_pins", 40);
    joint->line_radius = radius;
    joint->division_distance = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::top_top_ring_pins(double division_length, double shift, double radius) {

    const std::shared_ptr<JointPlate> joint = from_library("top_top_ring_pins", 40);
    joint->line_radius = radius;
    joint->division_distance = division_length;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::top_top_grid_pins(double division_length, double shift, double radius) {

    const std::shared_ptr<JointPlate> joint = from_library("top_top_grid_pins", 40);
    joint->line_radius = radius;
    joint->division_distance = division_length;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::top_top_rectangle_pins(double division_length, double shift, double radius) {

    const std::shared_ptr<JointPlate> joint = from_library("top_top_rectangle_pins", 40);
    joint->line_radius = radius;
    joint->division_distance = division_length;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::top_top_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female) {

    const std::shared_ptr<JointPlate> joint = from_library("top_top_custom", 40);
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool JointPlate::compute_top_top(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) const {

    if (parameters.library == "top_top_custom") {
        ::top_top_custom(connection, settings);
        return true;
    }

    if (elements.size() < 2)
        throw std::invalid_argument("Drill joints require the participating plates in orient(contact, elements)");

    if (parameters.library == "top_top_centre_pin") {
        ::top_top_centre_pin(connection, elements);
        return true;
    }
    if (parameters.library == "top_top_polylabel_pin") {
        ::top_top_polylabel_pin(connection, elements);
        return true;
    }

    // a drill pattern is laid on the contact less a border of three drill radii, so one diameter of wood stays between every hole and
    // the plate's edge; its shift is measured inside that border
    const Polyline contact = connection.contact.polygon;
    if (line_radius > 0.0)
        connection.contact.polygon = offset_contact(connection, 3.0 * line_radius);

    bool known = true;
    if (parameters.library == "top_top_circle_pins")
        ::top_top_circle_pins(connection, elements);
    else if (parameters.library == "top_top_ring_pins")
        ::top_top_ring_pins(connection, elements, settings, line_radius);
    else if (parameters.library == "top_top_grid_pins")
        ::top_top_grid_pins(connection, elements);
    else if (parameters.library == "top_top_rectangle_pins")
        ::top_top_rectangle_pins(connection, elements, settings);
    else
        known = false;

    connection.contact.polygon = contact;
    return known;
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<JointPlate> JointPlate::cross_slot() {

    return from_library("cross_slot", 30);
}

std::shared_ptr<JointPlate> JointPlate::cross_sliced_half_lap(double shift) {

    const std::shared_ptr<JointPlate> joint = from_library("cross_sliced_half_lap", 30);
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::cross_half_lap(double shift) {

    const std::shared_ptr<JointPlate> joint = from_library("cross_half_lap", 30);
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::cross_half_lap_diagonal_drills(double shift) {

    const std::shared_ptr<JointPlate> joint = from_library("cross_half_lap_diagonal_drills", 30);
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::cross_half_lap_drill(double shift) {

    const std::shared_ptr<JointPlate> joint = from_library("cross_half_lap_drill", 30);
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::cross_brussels_half_lap(double shift) {

    const std::shared_ptr<JointPlate> joint = from_library("cross_brussels_half_lap", 30);
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::cross_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female) {

    const std::shared_ptr<JointPlate> joint = from_library("cross_custom", 30);
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool JointPlate::compute_cross(InteractionFeaturePlate& connection, const Settings& settings) const {

    if (parameters.library == "cross_slot")
        ::cross_slot(connection);
    else if (parameters.library == "cross_sliced_half_lap")
        ::cross_sliced_half_lap(connection);
    else if (parameters.library == "cross_half_lap")
        ::cross_half_lap(connection);
    else if (parameters.library == "cross_half_lap_diagonal_drills")
        ::cross_half_lap_diagonal_drills(connection);
    else if (parameters.library == "cross_half_lap_drill")
        ::cross_half_lap_drill(connection);
    else if (parameters.library == "cross_brussels_half_lap")
        ::cross_brussels_half_lap(connection);
    else if (parameters.library == "cross_custom")
        ::cross_custom(connection, settings);
    else
        return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

/// A rotated design on its 2024 family defaults: geometric divisions every 300 mm of the joint line, shift 0.5.
std::shared_ptr<JointPlate> JointPlate::side_side_rotated(const std::string& library) {

    const std::shared_ptr<JointPlate> joint = from_library(library, 13);
    joint->division_distance = 300.0;
    joint->shift = 0.5;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_rotated_miter() {

    return side_side_rotated("side_side_rotated_miter");
}

std::shared_ptr<JointPlate> JointPlate::side_side_rotated_hooks(int divisions, double shift) {

    const std::shared_ptr<JointPlate> joint = side_side_rotated("side_side_rotated_hooks");
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_rotated_keys(int divisions, double shift) {

    const std::shared_ptr<JointPlate> joint = side_side_rotated("side_side_rotated_keys");
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::side_side_rotated_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female) {

    const std::shared_ptr<JointPlate> joint = side_side_rotated("side_side_rotated_custom");
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool JointPlate::compute_side_side_rotated(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) const {

    if (parameters.library == "side_side_rotated_miter")
        ::side_side_rotated_miter(connection);
    else if (parameters.library == "side_side_rotated_hooks")
        ::side_side_rotated_hooks(connection, elements);
    else if (parameters.library == "side_side_rotated_keys")
        ::side_side_rotated_keys(connection, elements);
    else if (parameters.library == "side_side_rotated_custom")
        ::side_side_rotated_custom(connection, settings);
    else
        return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<JointPlate> JointPlate::side_removal(bool merge_with_joint, double shift) {

    const std::shared_ptr<JointPlate> joint = from_library("side_removal", 0);
    joint->parameters.merge_with_joint = merge_with_joint;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlate> JointPlate::jupiter_scarf() {

    return from_library("jupiter_scarf", 12);
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool JointPlate::compute_side_removal(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements) const {

    if (parameters.library == "side_removal")
        ::side_removal(connection, elements, parameters.merge_with_joint, true);
    else if (parameters.library == "jupiter_scarf")
        ::jupiter_scarf(connection);
    else
        return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<JointPlate> JointPlate::border() {

    return from_library("border", 60);
}

std::shared_ptr<JointPlate> JointPlate::border_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female) {

    const std::shared_ptr<JointPlate> joint = from_library("border_custom", 60);
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool JointPlate::compute_b(InteractionFeaturePlate& connection, const Settings& settings) const {

    if (parameters.library == "border")
        ::border(connection);
    else if (parameters.library == "border_custom")
        ::border_custom(connection, settings);
    else
        return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string JointPlateParameters::pb_dumps() const {

    wood_proto::JointPlateParameters proto;
    proto.set_library(library);
    proto.set_contact_type(contact_type);
    proto.set_divisions(divisions);
    proto.set_taper(taper);
    proto.set_chamfer(chamfer);
    proto.set_modify_outline(modify_outline);
    proto.set_disable_divisions(disable_divisions);
    proto.set_distance_squared(distance_squared);
    proto.set_merge_with_joint(merge_with_joint);

    for (int i = 0; i < 2; ++i) {
        proto.add_x(x[i]);
        proto.add_y(y[i]);
        proto.add_z(z[i]);
    }

    for (const Polyline& outline : outlines[0])
        if (!proto.add_male()->ParseFromString(outline.pb_dumps()))
            throw std::runtime_error("Invalid male joint outline");

    for (const Polyline& outline : outlines[1])
        if (!proto.add_female()->ParseFromString(outline.pb_dumps()))
            throw std::runtime_error("Invalid female joint outline");

    return proto.SerializeAsString();
}

JointPlateParameters JointPlateParameters::pb_loads(const std::string& data) {

    wood_proto::JointPlateParameters proto;

    if (!proto.ParseFromString(data))
        throw std::runtime_error("Invalid plate joint parameters");

    JointPlateParameters parameters;
    parameters.library = proto.library();
    parameters.contact_type = proto.contact_type();
    parameters.divisions = proto.divisions();
    parameters.taper = proto.taper();
    parameters.chamfer = proto.chamfer();
    parameters.disable_divisions = proto.disable_divisions();
    parameters.distance_squared = proto.distance_squared();
    parameters.merge_with_joint = proto.merge_with_joint();

    if (proto.has_modify_outline())
        parameters.modify_outline = proto.modify_outline();

    if (proto.x_size() != 2 || proto.y_size() != 2 || proto.z_size() != 2)
        throw std::runtime_error("Invalid finger extents");

    for (int i = 0; i < 2; ++i) {
        parameters.x[i] = proto.x(i);
        parameters.y[i] = proto.y(i);
        parameters.z[i] = proto.z(i);
    }

    for (const session_proto::Polyline& outline : proto.male())
        parameters.outlines[0].push_back(Polyline::pb_loads(outline.SerializeAsString()));

    for (const session_proto::Polyline& outline : proto.female())
        parameters.outlines[1].push_back(Polyline::pb_loads(outline.SerializeAsString()));

    return parameters;
}

JointAnnen::JointAnnen(std::vector<std::shared_ptr<Plate>> elements, std::vector<InteractionFeaturePlate> joints,
                       const std::vector<std::vector<int>>& groups, const Settings& settings) {
    align_annen_joints(groups, elements, joints);
    build_geometry(
        joints,
        elements,
        {},
        settings
    );
    connections = std::move(joints);
    name = "JointAnnen";
}

JointVidy::JointVidy(std::vector<std::shared_ptr<Plate>> elements, std::vector<InteractionFeaturePlate> joints,
                     const std::vector<std::vector<int>>& groups, const Settings& settings) {
    std::unordered_map<uint64_t, int> map = joints_by_element_pair(elements, joints);
    add_vidy_shadow_joints(
        groups,
        elements,
        joints,
        map,
        settings.angle
    );
    build_geometry(
        joints,
        elements,
        {},
        settings
    );
    connections = std::move(joints);
    name = "JointVidy";
}

void JointPlate::write_proto(wood_proto::Joint& proto) const {

    Joint::write_proto(proto);
    if (!proto.mutable_plate_parameters()->ParseFromString(parameters.pb_dumps()))
        throw std::runtime_error("Invalid plate joint parameters");

    proto.set_variant(variant);
    proto.set_division_distance(division_distance);
    proto.set_shift(shift);
    for (const InteractionFeaturePlate& connection : connections)
        if (!proto.add_connections()->ParseFromString(connection.pb_dumps()))
            throw std::runtime_error("Invalid joint connection");
}

void JointPlate::read_proto(const wood_proto::Joint& proto) {

    Joint::read_proto(proto);
    if (proto.has_plate_parameters())
        parameters = JointPlateParameters::pb_loads(proto.plate_parameters().SerializeAsString());

    variant = proto.variant();
    division_distance = proto.division_distance();
    shift = proto.shift();
    for (const session_proto::Interaction& item : proto.connections()) {
        const std::shared_ptr<InteractionFeaturePlate> connection = std::dynamic_pointer_cast<InteractionFeaturePlate>(Interaction::pb_loads(item.SerializeAsString()));
        if (!connection)
            throw std::runtime_error("Invalid plate joint connection");
        connections.push_back(*connection);
    }
}

}
