#include "pch.h"
#include "wood_session.h"
#include "wood_three_valence.h"
#include "element_joint.pb.h"
#include "wood_element_geometry.h"
#include "wood_joint_library.h"
#include "src/templates/cavity_border.h"
#include "src/templates/vidy/vidy_joint_kernel.h"

using namespace session_cpp;

namespace {

using namespace wood_session;

static bool plate_from_library(
    const std::string& design,
    InteractionFeaturePlate& joint,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const std::map<std::string, double>& given
);

#include "wood_interaction_feature_plate_joints.h"

// ═══════════════════════════════════════════════════════════════════════════
// Joint library
// ═══════════════════════════════════════════════════════════════════════════

/// The id of every design 2024's tables reach, by name and by its 2024 name.
const std::unordered_map<std::string, int> LIBRARY_IDS = {
    {"", 0},
    {"side_side_inplane_dovetails", 1},
    {"side_side_inplane_three_fingers", 2},
    {"side_side_inplane_butterfly_keys", 3},
    {"side_side_inplane_splayed_scarf", 4},
    {"side_side_inplane_cross_splayed_scarf", 5},
    {"side_side_inplane_tooth_keys", 6},
    {"jupiter_scarf", 7},
    {"side_side_inplane/side_removal", 8},
    {"side_side_inplane_custom", 9},
    {"side_side_outplane_fingers", 10},
    {"side_side_outplane_graded_fingers", 11},
    {"side_side_outplane_three_fingers", 12},
    {"side_side_outplane_miter_tenon", 13},
    {"side_side_outplane_tenons", 14},
    {"side_side_outplane_linked_tenons", 15},
    {"side_side_outplane_linked_tenons_wall", 16},
    {"side_side_outplane_annen_fingers", 17},
    {"side_side_outplane/side_removal", 18},
    {"side_side_outplane_custom", 19},
    {"top_side_tenons", 20},
    {"top_side_dense_tenons", 21},
    {"top_side_annen_tenons", 22},
    {"top_side_annen_shifted_tenons", 23},
    {"top_side_milled_wedge", 24},
    {"top_side_snap_fit", 25},
    {"top_side_bridle", 26},
    {"top_side/side_removal", 28},
    {"top_side_custom", 29},
    {"cross_slot", 30},
    {"cross_sliced_half_lap", 31},
    {"cross_half_lap", 32},
    {"cross_brussels_half_lap", 35},
    {"cross/side_removal", 38},
    {"cross_custom", 39},
    {"top_top_centre_pin", 40},
    {"top_top_polylabel_pin", 41},
    {"top_top_circle_pins", 42},
    {"top_top_ring_pins", 43},
    {"top_top_grid_pins", 44},
    {"top_top_rectangle_pins", 45},
    {"top_top/side_removal", 48},
    {"top_top_custom", 49},
    {"side_side_rotated_keys", 54},
    {"side_side_rotated_hooks", 55},
    {"side_side_rotated_miter", 56},
    {"side_side_rotated/side_removal", 57},
    {"side_side_rotated/side_removal_merged", 58},
    {"side_side_rotated_custom", 59},
    {"border", 60},
    {"border_custom", 69},
    {"ss_e_ip_1", 1},
    {"ss_e_ip_0", 2},
    {"ss_e_ip_2", 3},
    {"ss_e_ip_3", 4},
    {"ss_e_ip_4", 5},
    {"ss_e_ip_5", 6},
    {"ss_e_ip_custom", 9},
    {"ss_e_op_1", 10},
    {"ss_e_op_2", 11},
    {"ss_e_op_0", 12},
    {"ss_e_op_3", 13},
    {"ss_e_op_4", 14},
    {"ss_e_op_5", 15},
    {"ss_e_op_6", 16},
    {"ss_e_op_custom", 19},
    {"ts_e_p_3", 20},
    {"ts_e_p_2", 21},
    {"ts_e_p_4", 24},
    {"ts_e_p_5", 25},
    {"ts_e_p_custom", 29},
    {"cr_c_ip_0", 30},
    {"cr_c_ip_1", 31},
    {"cr_c_ip_2", 32},
    {"cr_c_ip_3", 33},
    {"cr_c_ip_4", 34},
    {"cr_c_ip_5", 35},
    {"cr_c_ip_custom", 39},
    {"tt_e_p_0", 40},
    {"tt_e_p_1", 41},
    {"tt_e_p_2", 42},
    {"tt_e_p_3", 43},
    {"tt_e_p_4", 44},
    {"tt_e_p_5", 45},
    {"tt_e_p_custom", 49},
    {"ss_e_r_3", 54},
    {"ss_e_r_2", 55},
    {"ss_e_r_0", 56},
    {"side_removal_ss_e_r_1", 58},
    {"ss_e_r_custom", 59},
    {"b_0", 60},
    {"b_custom", 69},
};

/// What a builder may read while it fills a InteractionFeaturePlate.
/// A fabrication type by its name: "edge_insertion", "mill_project", ...; edge_insertion for an unknown name.
static FabricationType fabrication_named(const std::string& name) {

    static const std::map<std::string, FabricationType> NAMES = {
        {"nothing", FabricationType::nothing}, {"hole", FabricationType::hole}, {"edge_insertion", FabricationType::edge_insertion},
        {"insert_between_multiple_edges", FabricationType::insert_between_multiple_edges}, {"slice", FabricationType::slice},
        {"slice_projectsheer", FabricationType::slice_projectsheer}, {"mill", FabricationType::mill},
        {"mill_project", FabricationType::mill_project}, {"mill_projectsheer", FabricationType::mill_projectsheer},
        {"cut", FabricationType::cut}, {"cut_project", FabricationType::cut_project}, {"cut_projectsheer", FabricationType::cut_projectsheer},
        {"cut_reverse", FabricationType::cut_reverse}, {"conic", FabricationType::conic}, {"conic_reverse", FabricationType::conic_reverse},
        {"drill", FabricationType::drill}, {"drill_50", FabricationType::drill_50}, {"drill_10", FabricationType::drill_10},
    };
    const std::map<std::string, FabricationType>::const_iterator found = NAMES.find(name);
    return found == NAMES.end() ? FabricationType::edge_insertion : found->second;
}

/// The text between top-level separators of text, parentheses kept whole, each part trimmed.
static std::vector<std::string> top_parts(const std::string& text, char separator) {

    std::vector<std::string> parts(1);
    int depth = 0;
    for (const char c : text) {
        depth += c == '(' ? 1 : c == ')' ? -1 : 0;
        if (c == separator && depth == 0)
            parts.emplace_back();
        else
            parts.back() += c;
    }
    for (std::string& part : parts) {
        part.erase(0, part.find_first_not_of(' '));
        part.erase(part.find_last_not_of(' ') + 1);
    }

    return parts;
}

/// The points of an outline: parts joined by "|", each "x, y, z; x, y, z" with every coordinate an expression of names, or
/// "repeat j < n: points" with the points written once for j = 0 ... n - 1; a point "if c: x, y, z" only where c is not zero;
/// a point may be one vector expression.
static std::vector<Point> outline_points(const std::string& text, std::map<std::string, LibraryValue>& names) {

    std::vector<Point> points;
    for (const std::string& part : top_parts(text, '|')) {
        std::string list = part;
        std::string index;
        int times = 1;
        if (part.rfind("repeat ", 0) == 0) {
            const size_t less = part.find('<');
            const size_t colon = part.find(':');
            index = top_parts(part.substr(7, less - 7), ' ')[0];
            times = static_cast<int>(std::llround(evaluate(part.substr(less + 1, colon - less - 1), names).number));
            list = part.substr(colon + 1);
        }
        for (int k = 0; k < times; k++) {
            if (!index.empty())
                names[index] = {static_cast<double>(k)};
            for (const std::string& written : top_parts(list, ';')) {
                // "if c: x, y, z" a point only where c is not zero (a chamfer, a middle point a design may leave out)
                std::string point = written;
                if (written.rfind("if ", 0) == 0) {
                    const size_t colon = written.find(':');
                    if (evaluate(written.substr(3, colon - 3), names).number == 0.0)
                        continue;
                    point = written.substr(colon + 1);
                }
                const std::vector<std::string> xyz = top_parts(point, ',');
                // a point as one vector expression (a corner moved along a normal)
                if (xyz.size() == 1) {
                    const Vector at = evaluate(xyz[0], names).vector;
                    points.emplace_back(at[0], at[1], at[2]);
                    continue;
                }
                if (xyz.size() != 3)
                    throw std::invalid_argument(fmt::format("plate outline point \"{}\" needs x, y and z", point));
                points.emplace_back(evaluate(xyz[0], names).number, evaluate(xyz[1], names).number, evaluate(xyz[2], names).number);
            }
        }
    }

    return points;
}

/// Every joint volume of joint made a size wide square about the middle of its first edge, reaching twice half the size along
/// its second: the rotated joints' volumes, as 2024 drew them.
static void square_volumes(InteractionFeaturePlate& joint, double size) {

    for (int vi = 0; vi < 4; vi++) {
        std::optional<Polyline>& volume = joint.joint_volumes[vi];
        if (!volume || volume->point_count() != 5)
            continue;
        const Point p0 = volume->get_point(0);
        const Point p1 = volume->get_point(1);
        const Point p2 = volume->get_point(2);
        const Point c = Point::mid_point(p0, p1);
        Vector xd = p1 - p0;
        const double xl = std::sqrt(xd.magnitude_squared());
        if (xl < 1e-12)
            continue;
        xd = xd * (size * 0.5 / xl);
        Vector yd = p2 - p1;
        const double yl = std::sqrt(yd.magnitude_squared());
        if (yl < 1e-12)
            continue;
        yd = yd * (size * 0.5 / yl);
        *volume = Polyline({c + xd + 2 * yd, c - xd + 2 * yd, c - xd, c + xd, c + xd + 2 * yd});
    }
}

/// The outlines of design from its record of data/joints/plate/*.yaml, in the unit box (z along the joint line): its parameters
/// read with the joint's divisions, shift, length (the joint line scaled) and unit (the plate thickness the box is scaled to
/// along the line, when the record has unit_scale); each "role face = points" an outline, "role face = ends" the line between
/// the ends of the face's first outline, "role face = other face" a copy, "role face = each k < n: points" an outline per k;
/// each outline of the fabrication type the record names for its role (male_fabrication, female_fabrication, else fabrication,
/// else edge_insertion) unless "types" names it. False when no record names the design.
static bool plate_from_library(
    const std::string& design,
    InteractionFeaturePlate& joint,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const std::map<std::string, double>& given
) {

    LibraryRecord record = plate_record(design);
    if (record.lists.empty() && record.fields.empty())
        return false;

    // a design called by another (the linked tenons) keeps the caller's name
    if (!(record.fields.count("named") && record.fields.at("named") == "false"))
        joint.name = record.fields.count("name") ? record.fields.at("name") : design;
    const bool scaled = record.fields.count("unit_scale") && record.fields.at("unit_scale") == "true";
    if (scaled)
        joint.unit_scale_distance = unit_scale_distance(joint, elements);
    std::map<std::string, LibraryValue> names = {
        {"divisions", {static_cast<double>(joint.divisions)}},
        {"shift", {joint.shift}},
        {"length", {joint.joint_lines[0].length() * joint.scale[2]}},
        {"unit", {joint.unit_scale_distance}},
        {"joint_length", {joint.length * joint.scale[2]}},
    };
    for (const std::string& line : record.lists["parameters"]) {
        const std::vector<std::string> pair = top_parts(line, '=');
        names[pair[0]] = given.count(pair[0]) ? LibraryValue{given.at(pair[0])} : evaluate(pair[1], names);
    }

    // the outlines, slot by slot: male 0, male 1, female 0, female 1, each with its fabrication type: the role's default
    // (male_fabrication, female_fabrication, else fabrication, else edge_insertion) or the one "... as type" names
    const std::string common = record.fields.count("fabrication") ? record.fields.at("fabrication") : "edge_insertion";
    const FabricationType male = fabrication_named(record.fields.count("male_fabrication") ? record.fields.at("male_fabrication") : common);
    const FabricationType female = fabrication_named(record.fields.count("female_fabrication") ? record.fields.at("female_fabrication") : common);
    std::map<std::string, std::vector<Polyline>*> slots = {
        {"male 0", &joint.male_outlines[0]}, {"male 1", &joint.male_outlines[1]},
        {"female 0", &joint.female_outlines[0]}, {"female 1", &joint.female_outlines[1]},
    };
    std::map<std::string, std::vector<int>*> types = {
        {"male 0", &joint.male_fabrication_types[0]}, {"male 1", &joint.male_fabrication_types[1]},
        {"female 0", &joint.female_fabrication_types[0]}, {"female 1", &joint.female_fabrication_types[1]},
    };
    for (std::pair<const std::string, std::vector<Polyline>*>& slot : slots)
        slot.second->clear();
    for (std::pair<const std::string, std::vector<int>*>& slot : types)
        slot.second->clear();
    for (const std::string& line : record.lists["outlines"]) {
        const size_t equals = line.find('=');
        const std::string slot = top_parts(line.substr(0, equals), ';')[0];
        std::string trimmed = top_parts(line.substr(equals + 1), '\n')[0];
        if (!slots.count(slot))
            throw std::invalid_argument(fmt::format("{}: no outline slot \"{}\"", design, slot));
        int type = slot.rfind("male", 0) == 0 ? male : female;
        const size_t as = trimmed.rfind(" as ");
        if (as != std::string::npos && trimmed.find(';', as) == std::string::npos && trimmed.find(',', as) == std::string::npos) {
            type = fabrication_named(top_parts(trimmed.substr(as + 4), ' ')[0]);
            trimmed = top_parts(trimmed.substr(0, as), '\n')[0];
        }
        std::vector<Polyline>& outlines = *slots.at(slot);
        const size_t before = outlines.size();
        if (trimmed == "ends")
            outlines.push_back(Polyline({outlines.front().get_point(0), outlines.front().get_point(outlines.front().point_count() - 1)}));
        else if (slots.count(trimmed)) {
            const std::vector<Polyline> copy = *slots.at(trimmed);
            outlines.insert(outlines.end(), copy.begin(), copy.end());
            const std::vector<int> copied = *types.at(trimmed);
            types.at(slot)->insert(types.at(slot)->end(), copied.begin(), copied.end());
            continue;
        }
        // "each k < n: points" an outline per k (a hole per tenon), else one outline
        else if (trimmed.rfind("each ", 0) == 0) {
            const size_t less = trimmed.find('<');
            const size_t colon = trimmed.find(':');
            const std::string index = top_parts(trimmed.substr(5, less - 5), ' ')[0];
            const int times = static_cast<int>(std::llround(evaluate(trimmed.substr(less + 1, colon - less - 1), names).number));
            for (int k = 0; k < times; k++) {
                names[index] = {static_cast<double>(k)};
                outlines.push_back(Polyline(outline_points(trimmed.substr(colon + 1), names)));
            }
        }
        else
            outlines.push_back(Polyline(outline_points(trimmed, names)));
        types.at(slot)->insert(types.at(slot)->end(), outlines.size() - before, type);
    }

    // "types" names a face's types outright
    for (const std::string& line : record.lists["types"]) {
        const size_t equals = line.find('=');
        std::vector<int>& slot = *types.at(top_parts(line.substr(0, equals), ';')[0]);
        slot.clear();
        for (const std::string& name : top_parts(line.substr(equals + 1), ','))
            slot.push_back(fabrication_named(name));
    }

    if (scaled)
        joint.unit_scale = true;

    // volume_size squares the joint volumes about the seam, the size the profiles are then scaled to
    if (record.fields.count("volume_size")) {
        const double size = evaluate(record.fields.at("volume_size"), names).number;
        square_volumes(joint, size);
        joint.unit_scale_distance = size;
    }

    return true;
}

struct BuildContext {
    const Settings& settings;
    std::vector<std::shared_ptr<FacePlanarElement>>& elements;
    std::vector<InteractionFeaturePlate>& all_joints;
};

/// Family of an id by tens; -1 outside 1..69.
int family_of(const int id) {

    if (id < 1 || id > 69)
        return -1;

    return id < 10 ? 0 : id / 10;
}

/// The family a detected joint type falls in.
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

/// The library.
static bool build_inplane(const int id, InteractionFeaturePlate& joint, BuildContext& context) {

    switch (id) {
    case 1:
        plate_from_library("side_side_inplane_dovetails", joint, context.elements, {});
        return true;
    case 2:
        plate_from_library("side_side_inplane_three_fingers", joint, context.elements, {});
        return true;
    case 3:
        plate_from_library("side_side_inplane_butterfly_keys", joint, context.elements, {});
        return true;
    case 4:
        plate_from_library("side_side_inplane_splayed_scarf", joint, context.elements, {});
        return true;
    case 5:
        plate_from_library("side_side_inplane_cross_splayed_scarf", joint, context.elements, {});
        return true;
    case 6:
        plate_from_library("side_side_inplane_tooth_keys", joint, context.elements, {});
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
        plate_from_library("side_side_outplane_fingers", joint, context.elements, {});
        return true;
    case 11:
        plate_from_library("side_side_outplane_graded_fingers", joint, context.elements, {});
        return true;
    case 12:
        plate_from_library("side_side_outplane_three_fingers", joint, context.elements, {});
        return true;
    case 13:
        plate_from_library("side_side_outplane_miter_tenon", joint, context.elements, {});
        return true;
    case 14:
        plate_from_library("side_side_outplane_tenons", joint, context.elements, {});
        return true;
    case 15:
        side_side_outplane_linked_tenons(joint, context.all_joints, false);
        return true;
    case 16:
        side_side_outplane_linked_tenons_wall(joint, context.all_joints);
        return true;
    case 17:
        side_side_outplane_annen_fingers(joint, context.elements, 17, 15.0);
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
        plate_from_library("top_side_tenons", joint, context.elements, {});
        return true;
    case 21:
        plate_from_library("top_side_dense_tenons", joint, context.elements, {});
        return true;
    case 22:
        top_side_annen_tenons(joint, context.elements, context.all_joints);
        return true;
    case 23:
        top_side_annen_shifted_tenons(
            joint, context.elements, context.all_joints, 0, context.settings.limit_min_joint_length
        );
        return true;
    case 24:
        top_side_milled_wedge(joint);
        return true;
    case 25:
        top_side_snap_fit(joint);
        return true;
    case 26:
        top_side_bridle(joint, context.elements, 1.0 / 3.0);
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
        plate_from_library("cross_slot", joint, context.elements, {});
        return true;
    case 31:
        plate_from_library("cross_sliced_half_lap", joint, context.elements, {});
        return true;
    case 32:
    case 33:
    case 34:
        plate_from_library("cross_half_lap", joint, context.elements, {{"drills", static_cast<double>(id - 32)}});
        return true;
    case 35:
        plate_from_library("cross_brussels_half_lap", joint, context.elements, {});
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
        plate_from_library("side_side_rotated_keys", joint, context.elements, {});
        return true;
    case 55:
        plate_from_library("side_side_rotated_hooks", joint, context.elements, {});
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
        plate_from_library("side_side_inplane_dovetails", joint, context.elements, {});
        return;
    case 1:
        plate_from_library("side_side_outplane_fingers", joint, context.elements, {});
        return;
    case 2:
        plate_from_library("top_side_tenons", joint, context.elements, {});
        return;
    case 3:
        plate_from_library("cross_slot", joint, context.elements, {});
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

/// Unit joinery geometry for `id`.
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
            plate_from_library("side_side_outplane_fingers", joint, context.elements, {});
        else if (joint.joint_type == 20)
            plate_from_library("top_side_tenons", joint, context.elements, {});
        return;
    }

    build_family_default(family, joint, context);
}

// ═══════════════════════════════════════════════════════════════════════════
// Joint types and parameters
// ═══════════════════════════════════════════════════════════════════════════

/// Family lookup for one joint: representing id, division length and shift.
struct FamilyParameters {
    int id;
    double division_distance;
    double shift;
};

/// Wood's id_representing_joint_name: max of the two face ids in the JOINTS_TYPES table, -1 when the table says nothing.
int joint_id_for(
    const InteractionFeaturePlate& joint,
    const std::vector<std::vector<int>>& per_element_joints_types,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements
) {

    int id_representing_joint_name = -1;
    if (!per_element_joints_types.empty()) {

        const int element0 = index_of_plate(elements, joint.element_a);
        const int element1 = index_of_plate(elements, joint.element_b);
        const int face0 = joint.contact.face_a;
        const int face1 = joint.contact.face_b;
        const int original_face0 = element0 >= 0 && element0 < (int)elements.size()
                                       ? elements[element0]->given_face(face0)
                                       : face0;
        const int original_face1 = element1 >= 0 && element1 < (int)elements.size()
                                       ? elements[element1]->given_face(face1)
                                       : face1;
        const int id0 =
            (element0 >= 0 && element0 < (int)per_element_joints_types.size() &&
             original_face0 >= 0 && original_face0 < (int)per_element_joints_types[element0].size())
                ? std::abs(per_element_joints_types[element0][original_face0])
                : 0;
        const int id1 =
            (element1 >= 0 && element1 < (int)per_element_joints_types.size() &&
             original_face1 >= 0 && original_face1 < (int)per_element_joints_types[element1].size())
                ? std::abs(per_element_joints_types[element1][original_face1])
                : 0;

        if (element0 >= 0 && element0 < (int)per_element_joints_types.size() && element1 >= 0 &&
            element1 < (int)per_element_joints_types.size() &&
            (per_element_joints_types[element0].size() > 0 ||
             per_element_joints_types[element1].size() > 0)) {
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
    300, 0.5, 3, 450, 0.64, 15, 450, 0.5, 20, 300, 0.5, 30, 6, 0.95, 40, 300, 0.5, 58, 300, 1.0, 60,
};

/// One joint_parameters entry, from the settings when they are complete and from the built-in defaults otherwise.
double
joint_parameter(const std::vector<double>& parameters_global, bool parameters_ok, size_t index) {
    return parameters_ok ? parameters_global[index] : PARAMETER_DEFAULTS[index];
}

/// Per-family row of settings.joint_parameters: division length, shift and the default id when none was given.
FamilyParameters family_parameters(
    const Settings& settings, const int joint_type, const int id_representing_joint_name
) {

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
    std::string name;
    std::array<std::vector<Polyline>, 2> male_outlines;
    std::array<std::vector<Polyline>, 2> female_outlines;
    std::array<std::vector<int>, 2> male_fabrication_types;
    std::array<std::vector<int>, 2> female_fabrication_types;
    bool unit_scale;
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
std::string
joint_cache_key(const int id_representing_joint_name, const InteractionFeaturePlate& joint) {
    return std::to_string(id_representing_joint_name) + ";" + cache_key_number(joint.shift) + ";" +
           cache_key_number((double)joint.divisions);
}

/// Unit geometry from the cache when the key is known, else from the constructor and cached afterwards, as 2024 reused it.
void reuse_or_create_geometry(
    InteractionFeaturePlate& joint,
    const FamilyParameters& family,
    BuildContext& context,
    std::map<std::string, CachedJointGeometry>& unique_joints_cache
) {

    const std::string cache_key = joint_cache_key(family.id, joint);
    // the bridle's tile depends on the lean of each joint's own plate
    const bool use_cache = joint.linked_joints.empty() && family.id != 26;

    const std::map<std::string, CachedJointGeometry>::iterator cache_entry =
        use_cache ? unique_joints_cache.find(cache_key) : unique_joints_cache.end();
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

    joint_create_geometry(joint, family.division_distance, family.shift, family.id, context);

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
    std::map<std::string, CachedJointGeometry>& unique_joints_cache
) {

    std::vector<std::shared_ptr<FacePlanarElement>>& elements = context.elements;
    std::vector<InteractionFeaturePlate>& all_joints = context.all_joints;
    joint.scale = context.settings.joint_scale;

    // the distance a unit-scale design keeps along the joint line, as 2024 pinned every joint: the first plate's thickness by scale[2]
    const int element_index = index_of_plate(elements, joint.element_a);
    if (element_index >= 0 && element_index < (int)elements.size())
        joint.unit_scale_distance = elements[element_index]->thickness * joint.scale[2];

    joint_get_divisions(joint, family.division_distance);
    joint.shift = family.shift;
    reuse_or_create_geometry(joint, family, context, unique_joints_cache);

    if (!joint.no_orient) {
        scale_joint_depth(joint, context.settings.joint_depth_scale);
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
    std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const std::vector<std::vector<int>>& per_element_joints_types,
    const Settings& settings
) {

    std::map<std::string, CachedJointGeometry> unique_joints_cache;
    BuildContext context{settings, elements, all_joints};

    for (InteractionFeaturePlate& joint : all_joints) {

        const int id_representing_joint_name =
            joint_id_for(joint, per_element_joints_types, elements);
        const FamilyParameters family =
            family_parameters(settings, joint.joint_type, id_representing_joint_name);
        if (joint.link)
            continue;

        build_feature_geometry(joint, family, context, unique_joints_cache);
    }
}

}

namespace wood_session {

JointPlateElement::JointPlateElement() {
    name = "JointPlateElement";
}

JointPlateElement::JointPlateElement(const InteractionFeaturePlate& connection)
    : connections{connection} {
    name = connection.name;
}

JointPlateElement::JointPlateElement(int variant, double division_distance, double shift)
    : variant(variant), division_distance(division_distance), shift(shift) {
    name = "JointPlateElement";
}

JointPlateElement::JointPlateElement(
    const std::shared_ptr<FacePlanarElement>& source,
    const std::shared_ptr<FacePlanarElement>& target,
    const InteractionContactFace& contact,
    int variant,
    double division_distance,
    double shift,
    const Settings& settings
)
    : JointPlateElement(variant, division_distance, shift) {
    if (!source || !target)
        throw std::invalid_argument("Missing joint plates");

    orient(std::make_shared<InteractionContactFace>(contact), {source, target}, settings);
}

int JointPlateElement::library_id(const std::string& name) {

    const auto found = LIBRARY_IDS.find(name);
    if (found != LIBRARY_IDS.end())
        return found->second;

    if (name == "top_side_three_tenons" || name == "top_side_annen_tenons" ||
        name == "side_side_outplane_capped_fingers" || name == "side_side_outplane_notch")
        throw std::invalid_argument(
            fmt::format(
                "{} has no id in 2024's tables: it is reached by name only, through its JointPlateElement "
                "constructor",
                name
            )
        );
    throw std::invalid_argument(fmt::format("no joint design is named {}", name));
}

std::shared_ptr<JointPlateElement>
JointPlateElement::side_to_top(int variant, double division_distance, double shift) {
    if (variant < 20 || variant > 29)
        throw std::invalid_argument("Expected a side-to-top joint variant (20..29)");
    return std::make_shared<JointPlateElement>(variant, division_distance, shift);
}

void JointPlateElement::construct(
    InteractionFeaturePlate connection, const std::function<void(InteractionFeaturePlate&)>& builder
) {
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

/// The side-side family the detector gives a pair, for a joint of no family of its own.
static int side_side_family(
    const InteractionContactFace& contact,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const Settings& settings
) {

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
    const Point centre_0 =
        Point::mid_point(elements[0]->polylines[0].center(), elements[0]->polylines[1].center());
    const Point centre_1 =
        Point::mid_point(elements[1]->polylines[0].center(), elements[1]->polylines[1].center());
    const double dihedral =
        Point::dihedral_angle_deg(average.start(), average.end(), centre_0, centre_1);

    return dihedral <= settings.dihedral_angle ? 11 : 12;
}

/// Resolves the library family from the selected joint and checks the contact topology.
static int plate_contact_family(
    const JointPlateElement& joint,
    const InteractionContactFace& contact,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const Settings& settings
) {

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

    const bool compatible =
        ((family == 11 || family == 12 || family == 13) && type == ContactType::side_side) ||
        (family == 20 && type == ContactType::side_top) ||
        (family == 40 && type == ContactType::top_top) ||
        (family == 60 && type == ContactType::border);
    if (!compatible)
        throw std::invalid_argument("The selected joint does not support this contact type");

    return family;
}

void JointPlateElement::orient(
    const std::shared_ptr<InteractionContactFace>& contact, const Settings& settings
) {

    orient(contact, std::vector<std::shared_ptr<FacePlanarElement>>(), settings);
}

void JointPlateElement::orient(
    const std::shared_ptr<InteractionContactFace>& contact,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const Settings& settings
) {
    if (!contact)
        throw std::invalid_argument("JointPlateElement needs a face contact");
    const size_t plates = contact->type == ContactType::border ? 1 : 2;
    if (!elements.empty() && (elements.size() != plates || !elements[0] || !elements.back()))
        throw std::invalid_argument(
            plates == 1 ? "A border joint needs its one plate"
                        : "A face joint needs two plates in contact order"
        );

    // the male is the plate the detector designates: the second of an out-of-plane pair, the side-face one of a side-top pair, else the first
    const int family = plate_contact_family(*this, *contact, elements, settings);
    const bool reverse =
        family == 11 || (contact->type == ContactType::side_top && contact->face_a < 2);
    InteractionFeaturePlate connection;
    connection.contact =
        reverse ? *std::dynamic_pointer_cast<InteractionContactFace>(contact->flipped()) : *contact;
    connection.joint_type = family;
    connection.joint_lines = connection.contact.lines;
    for (int k = 0; k < 4; ++k)
        if (connection.contact.volumes[k].point_count() >= 4)
            connection.joint_volumes[k] = connection.contact.volumes[k];
    if (elements.size() == 2) {
        connection.element_a = elements[reverse ? 1 : 0]->guid();
        connection.element_b = elements[reverse ? 0 : 1]->guid();
    }
    if (elements.size() == 1 && contact->type == ContactType::border) {
        connection.element_a = elements[0]->guid();
        connection.element_b = elements[0]->guid();
    }
    construct(std::move(connection), settings, elements);
}

void JointPlateElement::orient(
    const std::shared_ptr<InteractionContactCross>& contact, const Settings& settings
) {

    orient(contact, std::vector<std::shared_ptr<FacePlanarElement>>(), settings);
}

void JointPlateElement::orient(
    const std::shared_ptr<InteractionContactCross>& contact,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const Settings& settings
) {
    if (!contact)
        throw std::invalid_argument("JointPlateElement needs a cross contact");
    InteractionFeaturePlate connection;
    connection.contact = InteractionContactFace(
        contact->faces_a[0], contact->faces_b[0], ContactType::unknown, contact->polygon
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

std::shared_ptr<InteractionFeaturePlate>
JointPlateElement::interaction_feature(int side, size_t index) const {
    if (side < 0 || side > 1)
        throw std::out_of_range("Joint side must be 0 or 1");
    if (connections.empty())
        throw std::logic_error(
            "JointPlateElement has no connection: call orient(contact) before interaction_feature()"
        );
    if (index >= connections.size())
        throw std::out_of_range("JointPlateElement connection index is out of range");

    const InteractionFeaturePlate& connection = connections[index];
    const std::string id = connection.feature_guid(side);
    const std::shared_ptr<InteractionFeaturePlate> feature =
        std::make_shared<InteractionFeaturePlate>(connection);
    feature->target_side = side + 1;
    feature->guid() = id;
    return feature;
}

std::shared_ptr<Interaction> JointPlateElement::interaction(size_t target) const {
    return interaction_feature(static_cast<int>(target % 2), target / 2);
}

size_t JointPlateElement::interaction_count() const {
    return 2 * connections.size();
}

const std::string& JointPlateElement::interaction_target(size_t target) const {

    if (target >= interaction_count())
        throw std::out_of_range("JointPlateElement interaction target is out of range");

    const InteractionFeaturePlate& connection = connections[target / 2];
    return target % 2 == 0 ? connection.element_a : connection.element_b;
}

/// The direction across a seam within a run's face.
static Vector seam_across(const Polyline& run, const Polyline& marker) {

    const Vector along = (marker[1] - marker[0]).normalized();
    return compute_newell(run.get_points()).cross(along);
}

/// The signed offsets of a run's points from its seam marker along a direction across it.
static std::vector<double>
seam_offsets(const std::vector<Point>& points, const Polyline& marker, const Vector& across) {

    std::vector<double> offsets;
    offsets.reserve(points.size());

    for (const Point& point : points)
        offsets.push_back((point - marker[0]).dot(across));

    return offsets;
}

/// The side of the seam a run keeps to.
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

/// The pockets of a run that keeps to one side of its seam.
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

/// The pocket pairs, bottom and top, of one side of an in-plane design.
static std::vector<std::array<Polyline, 2>> side_pockets(
    const std::array<std::vector<Polyline>, 2>& outlines,
    const std::array<std::vector<int>, 2>& types
) {

    if (types[0].empty() || (types[0][0] != FabricationType::edge_insertion &&
                             types[0][0] != FabricationType::insert_between_multiple_edges))
        return {};

    for (int face = 0; face < 2; face++)
        if (outlines[face].size() < 2 || outlines[face][0].point_count() < 3 ||
            outlines[face][1].point_count() != 2)
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

/// The keys.
std::vector<std::array<Polyline, 2>> JointPlateElement::bodies() const {

    std::vector<std::array<Polyline, 2>> result;
    for (const InteractionFeaturePlate& connection : connections) {

        if (connection.joint_type != 12 && connection.joint_type != 13)
            continue;

        const std::vector<std::array<Polyline, 2>> male =
            side_pockets(connection.male_outlines, connection.male_fabrication_types);
        const std::vector<std::array<Polyline, 2>> female =
            side_pockets(connection.female_outlines, connection.female_fabrication_types);

        if (male.empty() || female.empty())
            continue;

        const Polyline& male_run = connection.male_outlines[0][0];
        const Polyline& female_run = connection.female_outlines[0][0];
        const Vector across = seam_across(male_run, connection.male_outlines[0][1]);
        const int male_side =
            seam_side(seam_offsets(male_run.get_points(), connection.male_outlines[0][1], across));
        const int female_side = seam_side(
            seam_offsets(female_run.get_points(), connection.female_outlines[0][1], across)
        );

        if (male_side == female_side)
            continue;

        result.insert(result.end(), male.begin(), male.end());
        result.insert(result.end(), female.begin(), female.end());
    }

    return result;
}

std::vector<Line> JointPlateElement::drill_axes() const {
    std::vector<Line> result = drill_lines;
    for (const InteractionFeaturePlate& connection : connections) {
        for (int side = 0; side < 2; ++side) {
            const std::array<std::vector<Polyline>, 2>& outlines =
                side == 0 ? connection.male_outlines : connection.female_outlines;
            const std::array<std::vector<int>, 2>& types =
                side == 0 ? connection.male_fabrication_types : connection.female_fabrication_types;
            for (size_t i = 0; i < std::min(outlines[0].size(), types[0].size()); ++i) {
                const Polyline& line = outlines[0][i];
                if (!is_drill(types[0][i]) || line.point_count() != 2 ||
                    (line[1] - line[0]).magnitude_squared() <= 1e-12)
                    continue;
                // 2024 doubled every pair and both sides list a pin through both plates: each axis once, either way round
                bool listed = false;
                for (const Line& axis : result)
                    listed = listed || (axis.start() == line[0] && axis.end() == line[1]) ||
                             (axis.start() == line[1] && axis.end() == line[0]);
                if (!listed)
                    result.push_back(Line::from_points(line[0], line[1]));
            }
        }
    }
    return result;
}

/// True when the pair k of the side's outlines equals an earlier pair point for point.
static bool outline_pair_repeated(const std::array<std::vector<Polyline>, 2>& outlines, size_t k) {

    for (size_t j = 0; j < k; j++)
        if (outlines[0][k].get_points() == outlines[0][j].get_points() &&
            outlines[1][k].get_points() == outlines[1][j].get_points())
            return true;

    return false;
}

std::vector<Mesh>
JointPlateElement::side_solids(const InteractionFeaturePlate& connection, int side) {

    const std::array<std::vector<Polyline>, 2>& outlines =
        side == 0 ? connection.male_outlines : connection.female_outlines;
    const std::array<std::vector<int>, 2>& types =
        side == 0 ? connection.male_fabrication_types : connection.female_fabrication_types;
    std::vector<Mesh> pieces;

    for (size_t k = 0; k < std::min({outlines[0].size(), outlines[1].size(), types[0].size()});
         k++) {
        const Polyline& bottom = outlines[0][k];
        const Polyline& top = outlines[1][k];

        const bool solid =
            types[0][k] >= FabricationType::slice && types[0][k] <= FabricationType::conic_reverse;
        if (!solid || outline_pair_repeated(outlines, k) || bottom.point_count() < 3 ||
            bottom.point_count() != top.point_count())
            continue;

        const Mesh piece = loft_stations({bottom.closed(), top.closed()});
        if (manifold_solid(piece) && compute_volume(piece) > 0.0)
            pieces.push_back(piece);
    }

    return pieces;
}

std::vector<Line>
JointPlateElement::side_drills(const InteractionFeaturePlate& connection, int side) {

    const std::array<std::vector<Polyline>, 2>& outlines =
        side == 0 ? connection.male_outlines : connection.female_outlines;
    const std::array<std::vector<int>, 2>& types =
        side == 0 ? connection.male_fabrication_types : connection.female_fabrication_types;
    std::vector<Line> drills;

    for (size_t k = 0; k < std::min({outlines[0].size(), outlines[1].size(), types[0].size()});
         k++) {
        const Polyline& bottom = outlines[0][k];
        if (!is_drill(types[0][k]) || outline_pair_repeated(outlines, k))
            continue;
        if (bottom.point_count() == 2 && (bottom[1] - bottom[0]).magnitude_squared() > 1e-12)
            drills.push_back(Line::from_points(bottom[0], bottom[1]));
    }

    return drills;
}

void JointPlateElement::compute_key(
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements
) {

    key = Mesh();
    for (const InteractionFeaturePlate& connection : connections) {
        if (connection.name != "side_side_rotated_hooks" &&
            connection.name != "side_side_rotated_keys" &&
            connection.name != "side_side_inplane_splayed_scarf" &&
            connection.name != "side_side_inplane_cross_splayed_scarf" &&
            connection.name != "top_top_custom" && connection.name != "jupiter_scarf" &&
            connection.name != "border" && connection.name != "border_custom")
            continue;

        if (connection.name == "jupiter_scarf") {
            if (connection.male_outlines[0].size() > 2 && connection.male_outlines[1].size() > 2)
                key = Mesh::loft(
                    {connection.male_outlines[0][2]}, {connection.male_outlines[1][2]}, true
                );
            continue;
        }

        // the pieces the key fills: each side's milled pockets, and the in-plane notches its runs leave along the seam
        std::vector<Mesh> pieces;
        for (const std::array<Polyline, 2>& body : bodies())
            pieces.push_back(Mesh::loft({body[0]}, {body[1]}, true));

        if (connection.name == "side_side_rotated_hooks" ||
            connection.name == "side_side_rotated_keys" || connection.name == "top_top_custom" ||
            connection.name == "border" || connection.name == "border_custom") {
            for (int side = 0; side < 2; side++)
                for (const Mesh& piece : side_solids(connection, side))
                    key = key.number_of_faces() == 0
                              ? piece
                              : solid_boolean(key, piece, SolidOperation::add);
            continue;
        }

        for (int side = 0; side < 2; side++) {
            const int index =
                index_of_plate(elements, side == 0 ? connection.element_a : connection.element_b);
            if (index < 0)
                continue;

            const Mesh& stock = elements[index]->element_geometry_mesh();
            std::vector<Mesh> side_pieces = side_solids(connection, side);
            side_pieces.insert(side_pieces.end(), pieces.begin(), pieces.end());
            for (const Mesh& piece : side_pieces) {
                const Mesh pocket = solid_boolean(stock, piece, SolidOperation::intersect);
                if (pocket.number_of_faces() > 0)
                    key = key.number_of_faces() == 0
                              ? pocket
                              : solid_boolean(key, pocket, SolidOperation::add);
            }
        }
    }

    // a joint with a key shows it: the key is the joint's own piece
    if (key.number_of_faces() > 0)
        is_visible = true;
    invalidate_geometry();
}

/// Returns the slab loop at the requested height in its seam frame.
static std::array<Polyline, 2>
hilti_prism(const Xform& frame, const std::vector<Point>& outline, double bottom, double top) {

    std::vector<Point> corners;
    for (const Point& point : outline)
        if (corners.empty() || point.distance(corners.back()) > Tolerance::APPROXIMATION)
            corners.push_back(point);
    const Polyline loop = Polyline(corners).closed();

    return {
        loop.transformed(frame * Xform::translation(0.0, 0.0, bottom)),
        loop.transformed(frame * Xform::translation(0.0, 0.0, top))
    };
}

/// The half bow-tie of the 2024 Hilti joint (side_side_rotated_hooks, id 55) in a slab's frame, from start across the seam to end.
static std::vector<Point> hilti_outline(
    double start,
    double end,
    double neck_end,
    double taper_end,
    double neck_width,
    double wing_width,
    double router_radius,
    int sides
) {

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
        const std::vector<Point> circle =
            Polyline::from_sides(4 * quarter, router_radius, false).get_points();
        for (int corner = 0; corner < 2; corner++) {
            const Point centre(
                end - router_radius,
                (corner == 0 ? -1.0 : 1.0) * (0.5 * wing_width - router_radius),
                0.0
            );
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
static std::array<Polyline, 2>
hilti_disc(const Xform& frame, double start, double end, double z, double radius, int sides) {

    const Xform across =
        Xform::from_axes(Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0));
    const Polyline circle = Polyline::from_sides(sides, radius, true);

    return {
        circle.transformed(frame * Xform::translation(start, 0.0, z) * across),
        circle.transformed(frame * Xform::translation(end, 0.0, z) * across)
    };
}

namespace {

/// Orders contact corners from highest to lowest along the seam up axis.
struct HiltiCornerHigher {
    Point centre; // Contact centroid.
    Vector axis; // Seam upward axis.

    /// Return whether p lies higher than q along axis.
    bool operator()(const Point& p, const Point& q) const {
        return (p - centre).dot(axis) > (q - centre).dot(axis);
    }
};

}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_inplane_hilti(
    const FacePlanarElement& a,
    const FacePlanarElement& b,
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

    Vector z = x.cross(along).normalized();
    if (z.dot(Vector(0.0, 0.0, 1.0)) < 0.0)
        z = z * -1.0;
    std::vector<Point> corners = points;
    std::sort(corners.begin(), corners.end(), HiltiCornerHigher{centre, z});
    const Point origin = corners[0] + (corners[1] - corners[0]) * 0.5;

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
        const std::array<Polyline, 2> pocket = hilti_prism(
            frame,
            hilti_outline(
                -1.0, half_length, neck_end, taper_end, neck_width, wing_width, router_radius, sides
            ),
            -depth,
            1.0
        );
        std::array<std::vector<Polyline>, 2>& outlines =
            side == 0 ? connection.male_outlines : connection.female_outlines;
        std::array<std::vector<int>, 2>& types =
            side == 0 ? connection.male_fabrication_types : connection.female_fabrication_types;
        outlines[0].push_back(pocket[0]);
        outlines[1].push_back(pocket[1]);
        types[0].push_back(FabricationType::mill);
        types[1].push_back(FabricationType::mill);

        // the half: its neck from the seam and its wing, rounded like its pocket's corners, its top under the seam's top edge by what the
        const std::array<Polyline, 2> half = hilti_prism(
            frame,
            hilti_outline(
                0.0,
                half_length - disc_thickness,
                neck_end,
                taper_end,
                neck_width,
                wing_width,
                router_radius,
                sides
            ),
            part_top - height,
            part_top
        );
        const std::array<Polyline, 2> disc = hilti_disc(
            frame, half_length - disc_thickness, half_length, rod_z, 0.5 * disc_diameter, sides
        );
        key_pieces.push_back(loft_stations({half[0], half[1]}));
        key_pieces.push_back(loft_stations({disc[0], disc[1]}));
        rod_ends[side] = Point(half_length, 0.0, rod_z).transformed(frame);
    }

    // the key: both halves and both discs, one solid, the bolt from disc to disc through it, inside the pockets
    const std::shared_ptr<JointPlateElement> joint = std::make_shared<JointPlateElement>();
    joint->name = "side_side_inplane_hilti";
    joint->is_visible = true;
    joint->line_radius = 0.5 * rod_diameter;
    joint->chord_tolerance = sides_tolerance(joint->line_radius, sides);
    joint->connections = {std::move(connection)};
    joint->drill_lines = {Line::from_points(rod_ends[0], rod_ends[1])};
    for (const Mesh& piece : key_pieces)
        joint->key = joint->key.number_of_faces() == 0
                         ? piece
                         : solid_boolean(joint->key, piece, SolidOperation::add);
    joint->invalidate_geometry();

    return joint;
}

Mesh JointPlateElement::key_mesh() const {
    return key;
}

void JointPlateElement::place(const Xform& xform) {
    JointElement::place(xform);
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
        for (std::array<std::vector<Polyline>, 2>* outlines :
             {&connection.male_outlines, &connection.female_outlines})
            for (std::vector<Polyline>& face : *outlines)
                face = transformed_list(face, xform);
        connection.sync_features();
    }
}

void JointPlateElement::build_geometry(
    std::vector<InteractionFeaturePlate>& connections,
    std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const std::vector<std::vector<int>>& types,
    const Settings& settings
) {
    build_features_geometry(connections, elements, types, settings);
}

void JointPlateElement::construct(
    InteractionFeaturePlate connection,
    const Settings& settings,
    std::vector<std::shared_ptr<FacePlanarElement>> elements
) {

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
        joints[0], FamilyParameters{variant, division_distance, shift}, context, cache
    );
    connections = std::move(joints);
    name = connections[0].name;
    compute_key(elements);
    invalidate_geometry();
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════
std::shared_ptr<JointPlateElement>
JointPlateElement::from_library(const std::string& library, int contact_type) {

    const std::shared_ptr<JointPlateElement> joint = std::make_shared<JointPlateElement>();
    joint->name = library;
    joint->parameters.library = library;
    joint->parameters.contact_type = contact_type;

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
void JointPlateElement::compute_parameters(
    InteractionFeaturePlate& connection,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const Settings& settings
) const {

    if (parameters.contact_type && connection.joint_type != parameters.contact_type)
        throw std::invalid_argument(
            "The joint library constructor does not match the contact family"
        );

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

void JointPlateElement::compute_library(
    InteractionFeaturePlate& connection,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    std::vector<InteractionFeaturePlate>& connections,
    const Settings& settings
) const {

    if (parameters.library.starts_with("side_side_inplane_")) {
        if (compute_side_side_inplane(connection, elements, settings))
            return;
    } else if (parameters.library.starts_with("side_side_outplane_")) {
        if (compute_side_side_outplane(connection, elements, connections, settings))
            return;
    } else if (parameters.library.starts_with("top_side_")) {
        if (compute_top_side(connection, elements, connections, settings))
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
    } else if (
        parameters.library.starts_with("side_removal") || parameters.library == "jupiter_scarf"
    ) {
        if (elements.size() < 2)
            throw std::invalid_argument(
                "Side removal requires the participating plates in orient(contact, elements)"
            );

        if (compute_side_removal(connection, elements))
            return;
    }

    throw std::invalid_argument("Unknown plate joint library constructor: " + parameters.library);
}

void JointPlateElement::construct(
    std::vector<InteractionFeaturePlate> joints,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const Settings& settings
) {

    Settings configuration = settings;

    if (parameters.library.ends_with("_custom"))
        configuration.custom_joints[parameters.library.substr(0, parameters.library.size() - 7)] =
            parameters.outlines;

    for (InteractionFeaturePlate& connection : joints)
        compute_parameters(connection, elements, configuration);

    for (InteractionFeaturePlate& connection : joints)
        if (!connection.link)
            compute_library(connection, elements, joints, configuration);

    for (InteractionFeaturePlate& connection : joints)
        if (!connection.no_orient) {
            scale_joint_depth(connection, configuration.joint_depth_scale);
            joint_orient_to_connection_area(connection);
        }

    if (parameters.library == "side_side_outplane_linked_tenons" ||
        parameters.library == "side_side_outplane_linked_tenons_wall")
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
std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_inplane(const std::string& library) {

    const std::shared_ptr<JointPlateElement> joint = from_library(library, 12);
    joint->division_distance = 300.0;
    joint->shift = 0.5;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_inplane_three_fingers() {

    return side_side_inplane("side_side_inplane_three_fingers");
}

std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_inplane_dovetails(int divisions, double shift) {

    const std::shared_ptr<JointPlateElement> joint =
        side_side_inplane("side_side_inplane_dovetails");
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_inplane_butterfly_keys(int divisions) {

    const std::shared_ptr<JointPlateElement> joint =
        side_side_inplane("side_side_inplane_butterfly_keys");
    joint->parameters.divisions = divisions;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_inplane_splayed_scarf() {

    return side_side_inplane("side_side_inplane_splayed_scarf");
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_inplane_cross_splayed_scarf() {

    return side_side_inplane("side_side_inplane_cross_splayed_scarf");
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_inplane_tooth_keys(int divisions) {

    const std::shared_ptr<JointPlateElement> joint =
        side_side_inplane("side_side_inplane_tooth_keys");
    joint->parameters.divisions = divisions;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_inplane_custom(
    const std::vector<Polyline>& male, const std::vector<Polyline>& female
) {

    const std::shared_ptr<JointPlateElement> joint = side_side_inplane("side_side_inplane_custom");
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
bool JointPlateElement::compute_side_side_inplane(
    InteractionFeaturePlate& connection,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const Settings& settings
) const {

    if (parameters.library == "side_side_inplane_three_fingers")
        plate_from_library("side_side_inplane_three_fingers", connection, elements, {});
    else if (parameters.library == "side_side_inplane_dovetails")
        plate_from_library("side_side_inplane_dovetails", connection, elements, {});
    else if (parameters.library == "side_side_inplane_butterfly_keys")
        plate_from_library("side_side_inplane_butterfly_keys", connection, elements, {});
    else if (parameters.library == "side_side_inplane_splayed_scarf")
        plate_from_library("side_side_inplane_splayed_scarf", connection, elements, {});
    else if (parameters.library == "side_side_inplane_cross_splayed_scarf")
        plate_from_library("side_side_inplane_cross_splayed_scarf", connection, elements, {});
    else if (parameters.library == "side_side_inplane_tooth_keys")
        plate_from_library("side_side_inplane_tooth_keys", connection, elements, {});
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
std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_outplane(const std::string& library) {

    const std::shared_ptr<JointPlateElement> joint = from_library(library, 11);
    joint->division_distance = 450.0;
    joint->shift = 0.64;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_outplane_three_fingers() {

    return side_side_outplane("side_side_outplane_three_fingers");
}

std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_outplane_fingers(int divisions, double shift) {

    const std::shared_ptr<JointPlateElement> joint =
        side_side_outplane("side_side_outplane_fingers");
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_outplane_graded_fingers(int divisions, double shift) {

    const std::shared_ptr<JointPlateElement> joint =
        side_side_outplane("side_side_outplane_graded_fingers");
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_outplane_miter_tenon() {

    return side_side_outplane("side_side_outplane_miter_tenon");
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_outplane_tenons(
    int divisions,
    double taper,
    bool chamfer,
    bool modify_outline,
    const std::array<double, 2>& x,
    const std::array<double, 2>& y,
    const std::array<double, 2>& z
) {

    const std::shared_ptr<JointPlateElement> joint =
        side_side_outplane("side_side_outplane_tenons");
    joint->parameters.divisions = divisions;
    joint->parameters.taper = taper;
    joint->parameters.chamfer = chamfer;
    joint->parameters.modify_outline = modify_outline;
    joint->parameters.x = x;
    joint->parameters.y = y;
    joint->parameters.z = z;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_outplane_linked_tenons(int divisions, bool disable_divisions) {

    const std::shared_ptr<JointPlateElement> joint =
        side_side_outplane("side_side_outplane_linked_tenons");
    joint->parameters.divisions = divisions;
    joint->parameters.disable_divisions = disable_divisions;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_outplane_linked_tenons_wall(int divisions) {

    const std::shared_ptr<JointPlateElement> joint =
        side_side_outplane("side_side_outplane_linked_tenons_wall");
    joint->parameters.divisions = divisions;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_outplane_capped_fingers(int divisions) {

    const std::shared_ptr<JointPlateElement> joint =
        side_side_outplane("side_side_outplane_capped_fingers");
    joint->parameters.divisions = divisions;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_outplane_annen_fingers(int divisions, double angle) {

    const std::shared_ptr<JointPlateElement> joint =
        side_side_outplane("side_side_outplane_annen_fingers");
    joint->parameters.divisions = divisions;
    joint->parameters.taper = angle;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_outplane_notch() {

    return side_side_outplane("side_side_outplane_notch");
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_outplane_custom(
    const std::vector<Polyline>& male, const std::vector<Polyline>& female
) {

    const std::shared_ptr<JointPlateElement> joint =
        side_side_outplane("side_side_outplane_custom");
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
bool JointPlateElement::compute_side_side_outplane(
    InteractionFeaturePlate& connection,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    std::vector<InteractionFeaturePlate>& connections,
    const Settings& settings
) const {

    if (parameters.library == "side_side_outplane_three_fingers")
        plate_from_library("side_side_outplane_three_fingers", connection, elements, {});
    else if (parameters.library == "side_side_outplane_fingers")
        plate_from_library("side_side_outplane_fingers", connection, elements, {});
    else if (parameters.library == "side_side_outplane_graded_fingers")
        plate_from_library("side_side_outplane_graded_fingers", connection, elements, {});
    else if (parameters.library == "side_side_outplane_miter_tenon")
        plate_from_library("side_side_outplane_miter_tenon", connection, elements, {});
    else if (parameters.library == "side_side_outplane_tenons")
        plate_from_library(
            "side_side_outplane_tenons",
            connection,
            elements,
            {{"t", parameters.taper}, {"chamfer", parameters.chamfer ? 1.0 : 0.0}, {"modify", parameters.modify_outline ? 1.0 : 0.0},
             {"x0", parameters.x[0]}, {"x1", parameters.x[1]}, {"y0", parameters.y[0]}, {"y1", parameters.y[1]},
             {"z0e", parameters.z[0]}, {"z1e", parameters.z[1]}}
        );
    else if (parameters.library == "side_side_outplane_annen_fingers")
        ::side_side_outplane_annen_fingers(
            connection, elements, 17, parameters.taper > 0.0 ? parameters.taper : 15.0
        );
    else if (parameters.library == "side_side_outplane_linked_tenons")
        ::side_side_outplane_linked_tenons(connection, connections, parameters.disable_divisions);
    else if (parameters.library == "side_side_outplane_linked_tenons_wall")
        ::side_side_outplane_linked_tenons_wall(connection, connections);
    else if (parameters.library == "side_side_outplane_capped_fingers")
        plate_from_library("side_side_outplane_capped_fingers", connection, elements, {});
    else if (parameters.library == "side_side_outplane_notch")
        plate_from_library("side_side_outplane_notch", connection, elements, {});
    else if (parameters.library == "side_side_outplane_custom")
        ::side_side_outplane_custom(connection, settings);
    else
        return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<JointPlateElement> JointPlateElement::top_side_three_tenons() {

    return from_library("top_side_three_tenons", 20);
}

std::shared_ptr<JointPlateElement> JointPlateElement::top_side_annen_tenons(double tab_length) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_side_annen_tenons", 20);
    joint->division_distance = tab_length;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::top_side_annen_shifted_tenons(int divisions, double long_edge) {

    const std::shared_ptr<JointPlateElement> joint =
        from_library("top_side_annen_shifted_tenons", 20);
    joint->parameters.divisions = divisions;
    joint->division_distance = long_edge;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::top_side_dense_tenons(int divisions, double shift) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_side_dense_tenons", 20);
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::top_side_tenons(int divisions, double shift) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_side_tenons", 20);
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::top_side_milled_wedge() {

    return from_library("top_side_milled_wedge", 20);
}

std::shared_ptr<JointPlateElement> JointPlateElement::top_side_snap_fit(int divisions) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_side_snap_fit", 20);
    joint->parameters.divisions = divisions;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::top_side_bridle(double shoulder, double tongue) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_side_bridle", 20);
    joint->parameters.z = {-0.5 * tongue, 0.5 * tongue};
    joint->shift = shoulder;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::top_side_custom(
    const std::vector<Polyline>& male, const std::vector<Polyline>& female
) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_side_custom", 20);
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
bool JointPlateElement::compute_top_side(
    InteractionFeaturePlate& connection,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    std::vector<InteractionFeaturePlate>& connections,
    const Settings& settings
) const {

    if (parameters.library == "top_side_three_tenons")
        plate_from_library("top_side_three_tenons", connection, elements, {});
    else if (parameters.library == "top_side_annen_tenons")
        ::top_side_annen_tenons(connection, elements, connections);
    else if (parameters.library == "top_side_annen_shifted_tenons")
        ::top_side_annen_shifted_tenons(
            connection, elements, connections, parameters.divisions, settings.limit_min_joint_length
        );
    else if (parameters.library == "top_side_dense_tenons")
        plate_from_library("top_side_dense_tenons", connection, elements, {});
    else if (parameters.library == "top_side_tenons")
        plate_from_library("top_side_tenons", connection, elements, {});
    else if (parameters.library == "top_side_milled_wedge")
        ::top_side_milled_wedge(connection);
    else if (parameters.library == "top_side_snap_fit")
        ::top_side_snap_fit(connection);
    else if (parameters.library == "top_side_bridle")
        ::top_side_bridle(connection, elements, parameters.z[1] - parameters.z[0]);
    else if (parameters.library == "top_side_custom")
        ::top_side_custom(connection, settings);
    else
        return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<JointPlateElement> JointPlateElement::top_top_centre_pin(double radius) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_top_centre_pin", 40);
    joint->line_radius = radius;
    joint->division_distance = 6.0;
    joint->shift = 0.95;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::top_top_polylabel_pin(double radius) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_top_polylabel_pin", 40);
    joint->line_radius = radius;
    joint->division_distance = 6.0;
    joint->shift = 0.95;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::top_top_circle_pins(int divisions, double shift, double radius) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_top_circle_pins", 40);
    joint->line_radius = radius;
    joint->division_distance = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::top_top_ring_pins(double division_length, double shift, double radius) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_top_ring_pins", 40);
    joint->line_radius = radius;
    joint->division_distance = division_length;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::top_top_grid_pins(double division_length, double shift, double radius) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_top_grid_pins", 40);
    joint->line_radius = radius;
    joint->division_distance = division_length;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::top_top_rectangle_pins(double division_length, double shift, double radius) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_top_rectangle_pins", 40);
    joint->line_radius = radius;
    joint->division_distance = division_length;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::top_top_custom(
    const std::vector<Polyline>& male, const std::vector<Polyline>& female
) {

    const std::shared_ptr<JointPlateElement> joint = from_library("top_top_custom", 40);
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
bool JointPlateElement::compute_top_top(
    InteractionFeaturePlate& connection,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const Settings& settings
) const {

    if (parameters.library == "top_top_custom") {
        ::top_top_custom(connection, settings);
        return true;
    }

    if (elements.size() < 2)
        throw std::invalid_argument(
            "Drill joints require the participating plates in orient(contact, elements)"
        );

    if (parameters.library == "top_top_centre_pin") {
        ::top_top_centre_pin(connection, elements);
        return true;
    }
    if (parameters.library == "top_top_polylabel_pin") {
        ::top_top_polylabel_pin(connection, elements);
        return true;
    }

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
std::shared_ptr<JointPlateElement> JointPlateElement::cross_slot() {

    return from_library("cross_slot", 30);
}

std::shared_ptr<JointPlateElement> JointPlateElement::cross_sliced_half_lap(double shift) {

    const std::shared_ptr<JointPlateElement> joint = from_library("cross_sliced_half_lap", 30);
    joint->shift = shift;

    return joint;
}

int JointPlateElement::cross_half_lap_id(CrossDrills drills) {

    return 32 + static_cast<int>(drills);
}

std::shared_ptr<JointPlateElement>
JointPlateElement::cross_half_lap(double shift, CrossDrills drills) {

    const std::shared_ptr<JointPlateElement> joint = from_library("cross_half_lap", 30);
    joint->shift = shift;
    joint->variant =
        cross_half_lap_id(drills); // the drills are stored as the id, which a saved joint keeps

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::cross_brussels_half_lap(double shift) {

    const std::shared_ptr<JointPlateElement> joint = from_library("cross_brussels_half_lap", 30);
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::cross_custom(
    const std::vector<Polyline>& male, const std::vector<Polyline>& female
) {

    const std::shared_ptr<JointPlateElement> joint = from_library("cross_custom", 30);
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
bool JointPlateElement::compute_cross(
    InteractionFeaturePlate& connection, const Settings& settings
) const {

    if (parameters.library == "cross_slot")
        plate_from_library("cross_slot", connection, {}, {});
    else if (parameters.library == "cross_sliced_half_lap")
        plate_from_library("cross_sliced_half_lap", connection, {}, {});
    else if (parameters.library == "cross_half_lap")
        plate_from_library("cross_half_lap", connection, {}, {{"drills", static_cast<double>(variant - 32)}});
    else if (parameters.library == "cross_brussels_half_lap")
        plate_from_library("cross_brussels_half_lap", connection, {}, {});
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
std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_rotated(const std::string& library) {

    const std::shared_ptr<JointPlateElement> joint = from_library(library, 13);
    joint->division_distance = 300.0;
    joint->shift = 0.5;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_rotated_miter() {

    return side_side_rotated("side_side_rotated_miter");
}

std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_rotated_hooks(int divisions, double shift) {

    const std::shared_ptr<JointPlateElement> joint = side_side_rotated("side_side_rotated_hooks");
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement>
JointPlateElement::side_side_rotated_keys(int divisions, double shift) {

    const std::shared_ptr<JointPlateElement> joint = side_side_rotated("side_side_rotated_keys");
    joint->parameters.divisions = divisions;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::side_side_rotated_custom(
    const std::vector<Polyline>& male, const std::vector<Polyline>& female
) {

    const std::shared_ptr<JointPlateElement> joint = side_side_rotated("side_side_rotated_custom");
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
bool JointPlateElement::compute_side_side_rotated(
    InteractionFeaturePlate& connection,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const Settings& settings
) const {

    if (parameters.library == "side_side_rotated_miter")
        ::side_side_rotated_miter(connection);
    else if (parameters.library == "side_side_rotated_hooks")
        plate_from_library("side_side_rotated_hooks", connection, elements, {});
    else if (parameters.library == "side_side_rotated_keys")
        plate_from_library("side_side_rotated_keys", connection, elements, {});
    else if (parameters.library == "side_side_rotated_custom")
        ::side_side_rotated_custom(connection, settings);
    else
        return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════
std::shared_ptr<JointPlateElement>
JointPlateElement::side_removal(bool merge_with_joint, double shift) {

    const std::shared_ptr<JointPlateElement> joint = from_library("side_removal", 0);
    joint->parameters.merge_with_joint = merge_with_joint;
    joint->shift = shift;

    return joint;
}

std::shared_ptr<JointPlateElement> JointPlateElement::jupiter_scarf() {

    return from_library("jupiter_scarf", 12);
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
bool JointPlateElement::compute_side_removal(
    InteractionFeaturePlate& connection,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements
) const {

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
std::shared_ptr<JointPlateElement> JointPlateElement::border() {

    return from_library("border", 60);
}

std::shared_ptr<JointPlateElement> JointPlateElement::border_custom(
    const std::vector<Polyline>& male, const std::vector<Polyline>& female
) {

    const std::shared_ptr<JointPlateElement> joint = from_library("border_custom", 60);
    joint->parameters.outlines = {male, female};

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
bool JointPlateElement::compute_b(
    InteractionFeaturePlate& connection, const Settings& settings
) const {

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

JointAnnenElement::JointAnnenElement(
    std::vector<std::shared_ptr<FacePlanarElement>> elements,
    std::vector<InteractionFeaturePlate> joints,
    const std::vector<std::vector<int>>& groups,
    const Settings& settings
) {
    align_annen_joints(groups, elements, joints);
    build_geometry(joints, elements, {}, settings);
    connections = std::move(joints);
    name = "JointAnnenElement";
}

JointVidyElement::JointVidyElement(
    std::vector<std::shared_ptr<FacePlanarElement>> elements,
    std::vector<InteractionFeaturePlate> joints,
    const std::vector<std::vector<int>>& groups,
    const Settings& settings
) {
    std::unordered_map<uint64_t, int> map = joints_by_element_pair(elements, joints);
    add_vidy_shadow_joints(groups, elements, joints, map, settings.angle);
    build_geometry(joints, elements, {}, settings);
    connections = std::move(joints);
    name = "JointVidyElement";
}

namespace {

/// Builds a closed jointed quad plate with (prof_b, prof_t) on seam_side and attaches mortises.
std::shared_ptr<FacePlanarElement> make_vidy_node_plate(
    const std::array<Point, 4>& ring_b,
    const std::array<Point, 4>& ring_t,
    size_t seam_side,
    const std::array<std::vector<Point>, 2>& prof,
    const Vector& facet_normal,
    const Vector& v_ins,
    const std::vector<wood_vidy::VidyMortiseSlot>& slots,
    const std::string& plate_name
) {

    std::vector<Point> sb(ring_b.begin(), ring_b.end());
    std::vector<Point> st(ring_t.begin(), ring_t.end());

    if (compute_newell(sb).dot(facet_normal) > 0.0) {
        std::reverse(sb.begin(), sb.end());
        std::reverse(st.begin(), st.end());
    }

    sb.push_back(sb.front());
    st.push_back(st.front());
    std::shared_ptr<FacePlanarElement> plate =
        std::make_shared<FacePlanarElement>(Polyline(sb), Polyline(st), plate_name);

    std::vector<Point> jb;
    std::vector<Point> jt;

    for (size_t k = 0; k < 4; k++) {
        jb.push_back(ring_b[k]);
        jt.push_back(ring_t[k]);

        if (k == seam_side) {
            jb.insert(jb.end(), prof[0].begin(), prof[0].end());
            jt.insert(jt.end(), prof[1].begin(), prof[1].end());
        }
    }

    if (compute_newell(jb).dot(facet_normal) > 0.0) {
        std::reverse(jb.begin(), jb.end());
        std::reverse(jt.begin(), jt.end());
    }

    jb.push_back(jb.front());
    jt.push_back(jt.front());
    wood_vidy::vidy_attach_plate_features(*plate, facet_normal, Polyline(jb), Polyline(jt), slots);
    plate->set_insertion_vectors({v_ins});

    return plate;
}

/// Computes four miter corners by intersecting offset_plane with four side planes.
std::array<Point, 4> intersect_quad_sides(
    const Plane& offset_plane, const std::array<Plane, 4>& sides, const Point& fallback
) {

    std::array<Point, 4> pts;

    for (size_t k = 0; k < 4; k++)
        pts[k] = Intersection::plane_plane_plane(offset_plane, sides[(k + 3) % 4], sides[k])
                     .value_or(fallback);

    return pts;
}

}

VidyDoubleLayerNode JointVidyElement::folded_double_layer_node(
    double dihedral_degrees,
    double seam_width,
    double wing_length,
    double plate_thickness,
    double total_offset,
    double pin_pitch,
    bool cutaway_front
) {

    const double phi = std::clamp(dihedral_degrees, 60.0, 150.0) * Tolerance::PI / 180.0;
    const double w = std::max(600.0, seam_width);
    const double l = std::max(500.0, wing_length);
    const double t = std::max(15.0, plate_thickness);
    const double h_tot = std::max(2.4 * t, total_offset);
    const double scale = std::clamp(t / 45.0, 0.80, 1.25);
    const double pitch = std::clamp(pin_pitch, 450.0 * scale, 1000.0);
    const Vector n_a(1.0, 0.0, 0.0);
    const Vector dir_b(-std::sin(phi), 0.0, -std::cos(phi));
    const Vector n_b(-std::cos(phi), 0.0, std::sin(phi));
    const Vector perp_a(0.0, 0.0, 1.0);
    const Vector perp_b = -dir_b;
    const Point c_a(0.0, 0.5 * w, 0.5 * l);
    const Point e0(0.0, 0.0, l);
    const Point e1(0.0, w, l);
    const Point c_b = e0 + dir_b * (0.5 * l) + Vector(0.0, 0.5 * w, 0.0);
    const Vector bis = (n_a + n_b).normalized();
    Vector mn_a = Vector(0.0, -1.0, 0.0).cross(bis).normalized();

    if (mn_a.dot(perp_a) < 0.0)
        mn_a = -mn_a;

    const std::array<Plane, 4> sides_a = {
        Plane::from_point_normal({0.0, 0.5 * w, 0.0}, {0.0, 0.0, -1.0}),
        Plane::from_point_normal({0.0, w, 0.5 * l}, {0.0, 1.0, 0.0}),
        Plane::from_point_normal({0.0, 0.5 * w, l}, mn_a),
        Plane::from_point_normal({0.0, 0.0, 0.5 * l}, {0.0, -1.0, 0.0})
    };
    const std::array<Plane, 4> sides_b = {
        Plane::from_point_normal({0.0, 0.5 * w, l}, -mn_a),
        Plane::from_point_normal(e1 + dir_b * (0.5 * l), {0.0, 1.0, 0.0}),
        Plane::from_point_normal(e0 + dir_b * l + Vector(0.0, 0.5 * w, 0.0), dir_b),
        Plane::from_point_normal(e0 + dir_b * (0.5 * l), {0.0, -1.0, 0.0})
    };
    const std::array<double, 4> hs = {0.0, t, h_tot - t, h_tot};
    std::array<Plane, 4> pa;
    std::array<Plane, 4> pb;
    std::array<std::array<Point, 4>, 4> ra;
    std::array<std::array<Point, 4>, 4> rb;

    for (size_t hi = 0; hi < 4; hi++) {
        pa[hi] = Plane::from_point_normal(c_a + n_a * hs[hi], n_a);
        pb[hi] = Plane::from_point_normal(c_b + n_b * hs[hi], n_b);
        ra[hi] = intersect_quad_sides(pa[hi], sides_a, e0);
        rb[hi] = intersect_quad_sides(pb[hi], sides_b, e0);
    }

    const double sin_a = std::max(0.20, std::sin(Tolerance::PI - phi));
    const double l_tab = t / sin_a;
    const double l_offs = (h_tot - t) / sin_a;
    const int cnt_sp = std::max(1, static_cast<int>(std::floor((w - 0.65 * pitch) / pitch)));
    const int cnt_dp = cnt_sp >= 2 ? (cnt_sp - 1) : 1;
    const Point ref_in = Point::mid_point(rb[0][0], rb[0][1]);
    const Point ref_out = Point::mid_point(rb[2][0], rb[2][1]);
    std::vector<wood_vidy::VidyMortiseSlot> slots_a_in;
    std::vector<wood_vidy::VidyMortiseSlot> slots_a_out;
    std::vector<wood_vidy::VidyMortiseSlot> slots_b_out;

    const wood_vidy::VidyPinBand dp_band = {
        {rb[0][0], rb[0][1], rb[1][0], rb[1][1]},
        ref_in,
        perp_b,
        l_tab,
        l_offs,
        false,
        cnt_dp,
        pitch,
        2.0,
        0.0,
        scale
    };
    const wood_vidy::VidyPinBand sp_band = {
        {rb[2][0], rb[2][1], rb[3][0], rb[3][1]},
        ref_out,
        perp_b,
        l_tab,
        0.0,
        false,
        cnt_sp,
        pitch,
        2.0,
        0.0,
        scale
    };
    const wood_vidy::VidyPinBand ep_band = {
        {ra[0][2], ra[0][3], ra[1][2], ra[1][3]},
        ref_in,
        perp_a,
        l_tab,
        l_offs,
        false,
        cnt_dp,
        pitch,
        0.0,
        0.0,
        scale
    };
    const wood_vidy::VidyPinBand ex_band = {
        {ra[2][2], ra[2][3], ra[3][2], ra[3][3]},
        ref_out,
        perp_a,
        l_tab,
        0.0,
        true,
        cnt_sp,
        pitch,
        0.0,
        0.0,
        scale
    };
    const std::array<std::vector<Point>, 2> dp_p =
        wood_vidy::vidy_emit_double_pins(dp_band, c_a, n_a, hs, 0, slots_a_in, slots_a_out);
    const std::array<std::vector<Point>, 2> sp_p =
        wood_vidy::vidy_emit_single_pins(sp_band, c_a, n_a, hs[2], hs[3], slots_a_out);
    const std::array<std::vector<Point>, 2> ep_p =
        wood_vidy::vidy_emit_extend_pins(ep_band, c_b, n_b, hs[2], hs[3], slots_b_out);
    const std::array<std::vector<Point>, 2> ex_p = wood_vidy::vidy_emit_extend_lip(ex_band, false);

    VidyDoubleLayerNode node;
    node.inner_a =
        make_vidy_node_plate(ra[0], ra[1], 2, ep_p, n_a, perp_a, slots_a_in, "inner_wall");
    node.outer_a =
        make_vidy_node_plate(ra[2], ra[3], 2, ex_p, n_a, -perp_b, slots_a_out, "outer_wall");
    node.inner_b = make_vidy_node_plate(rb[0], rb[1], 0, dp_p, n_b, perp_b, {}, "inner_roof");
    node.outer_b =
        make_vidy_node_plate(rb[2], rb[3], 0, sp_p, n_b, -perp_a, slots_b_out, "outer_roof");

    // Full-corner perpendicular cavity border boards along open boundary edges
    const std::vector<size_t> b_sides_a =
        cutaway_front ? std::vector<size_t>{0, 1} : std::vector<size_t>{0, 1, 3};
    const std::vector<size_t> b_sides_b =
        cutaway_front ? std::vector<size_t>{1, 2} : std::vector<size_t>{1, 2, 3};

    for (const size_t k : b_sides_a) {
        CavityBorderSpec spec;
        spec.facet_plane = pa[0];
        spec.boundary_plane = sides_a[k];
        spec.start_plane = sides_a[(k + 3) % 4];
        spec.end_plane = sides_a[(k + 1) % 4];
        spec.inner_level = t + 0.4;
        spec.outer_level = h_tot - t - 0.4;
        spec.thickness = t;
        spec.miter_start =
            std::find(b_sides_a.begin(), b_sides_a.end(), (k + 3) % 4) != b_sides_a.end();
        spec.start_neighbor_thickness = t;
        spec.miter_end =
            std::find(b_sides_a.begin(), b_sides_a.end(), (k + 1) % 4) != b_sides_a.end();
        spec.end_neighbor_thickness = t;
        spec.start_setback = 0.5;
        spec.end_setback = 0.5;
        const std::shared_ptr<FacePlanarElement> brd =
            compute_cavity_border_board(spec, fmt::format("border_wall_{}", k));

        if (brd)
            node.borders.push_back(brd);
    }

    for (const size_t k : b_sides_b) {
        CavityBorderSpec spec;
        spec.facet_plane = pb[0];
        spec.boundary_plane = sides_b[k];
        spec.start_plane = sides_b[(k + 3) % 4];
        spec.end_plane = sides_b[(k + 1) % 4];
        spec.inner_level = t + 0.4;
        spec.outer_level = h_tot - t - 0.4;
        spec.thickness = t;
        spec.miter_start =
            std::find(b_sides_b.begin(), b_sides_b.end(), (k + 3) % 4) != b_sides_b.end();
        spec.start_neighbor_thickness = t;
        spec.miter_end =
            std::find(b_sides_b.begin(), b_sides_b.end(), (k + 1) % 4) != b_sides_b.end();
        spec.end_neighbor_thickness = t;
        spec.start_setback = 0.5;
        spec.end_setback = 0.5;
        const std::shared_ptr<FacePlanarElement> brd =
            compute_cavity_border_board(spec, fmt::format("border_roof_{}", k));

        if (brd)
            node.borders.push_back(brd);
    }

    return node;
}

void JointPlateElement::write_proto(wood_proto::JointElement& proto) const {

    JointElement::write_proto(proto);
    if (!proto.mutable_plate_parameters()->ParseFromString(parameters.pb_dumps()))
        throw std::runtime_error("Invalid plate joint parameters");

    proto.set_variant(variant);
    proto.set_division_distance(division_distance);
    proto.set_shift(shift);
    for (const InteractionFeaturePlate& connection : connections)
        if (!proto.add_connections()->ParseFromString(connection.pb_dumps()))
            throw std::runtime_error("Invalid joint connection");
}

void JointPlateElement::read_proto(const wood_proto::JointElement& proto) {

    JointElement::read_proto(proto);
    if (proto.has_plate_parameters())
        parameters = JointPlateParameters::pb_loads(proto.plate_parameters().SerializeAsString());

    variant = proto.variant();
    division_distance = proto.division_distance();
    shift = proto.shift();
    for (const session_proto::Interaction& item : proto.connections()) {
        const std::shared_ptr<InteractionFeaturePlate> connection =
            std::dynamic_pointer_cast<InteractionFeaturePlate>(
                Interaction::pb_loads(item.SerializeAsString())
            );
        if (!connection)
            throw std::runtime_error("Invalid plate joint connection");
        connections.push_back(*connection);
    }
}

}
