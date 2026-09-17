#include "wood_pch.h"
#include "wood_session.h"
#include "yaml.hpp"

namespace wood_session {
namespace globals {

// ═══════════════════════════════════════════════════════════════════════════
// Definitions
// ═══════════════════════════════════════════════════════════════════════════

std::vector<double> JOINTS_PARAMETERS_AND_TYPES;
std::vector<double> JOINT_VOLUME_EXTENSION;
SearchType SEARCH_TYPE = face_to_face;
std::vector<double> BEAMS;
std::array<double, 3> JOINT_SCALE = {1.0, 1.0, 1.0};
double FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE = 150.0;
bool FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED = false;
bool FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE = false;

double DISTANCE = 0.1;
double DISTANCE_SQUARED = 0.01;
double ANGLE = 0.11;
double DUPLICATE_PTS_TOL = 0.0;
double LIMIT_MIN_JOINT_LENGTH = 0.0;

int64_t CLIPPER_SCALE = 1000000;
double CLIPPER_AREA = 0.01;

std::string DATA_SET_INPUT_FOLDER = (std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "data").string();
const std::vector<std::string> DATASET_NAMES = {
    "hexbox_and_corner",
    "vidy_corner",
    "vidy_one_layer",
    "vidy_one_axis_two_layers",
    "vidy_full",
    "inplane_butterflies",
    "inplane_hexshell",
    "inplane_differentdirections",
    "vidy_folding",
    "outofplane_box",
    "outofplane_box_miter",
    "outofplane_tetra",
    "outofplane_dodecahedron",
    "outofplane_icosahedron",
    "outofplane_octahedron",
    "simple_corners",
    "simple_corners_combined",
    "simple_corners_diff_lengths",
    "inplane_hilti",
    "top_to_top_pairs",
    "hexboxes",
    "hex_block_rossiniere",
    "top_to_side_snap_fit",
    "top_to_side_box",
    "top_to_side_corners",
    "annen_corner",
    "annen_box",
    "annen_box_pair",
    "annen_grid_small",
    "annen_grid_full_arch",
    "vda_floor_0",
    "vda_floor_2",
    "cross_and_sides_corner",
    "cross_corners",
    "cross_vda_corner",
    "cross_vda_hexshell",
    "cross_vda_hexshell_reciprocal",
    "cross_vda_single_arch",
    "cross_vda_shell",
    "cross_square_reciprocal_two_sides",
    "cross_square_reciprocal_iseya",
    "cross_ibois_pavilion",
    "cross_brussels_sports_tower",
    "phanomema_node",
    "hello",
    "top_to_side_test",
    "vda_floor_1",
    "cross_brg_slab_0",
};
const std::vector<std::string> SESSION_NAMES = {
    "floor_model",
    "session",
};

const std::string& dataset_name(size_t index) { return DATASET_NAMES.at(index); }

std::string DATA_SET_INPUT_NAME;
std::string DATA_SET_OBJ;
std::string DATA_SET_ADJACENCY;
std::string DATA_SET_THREE_VALENCE;
std::string DATA_SET_INSERTION_VECTORS;
std::string DATA_SET_JOINTS_TYPES;
std::string DATA_SET_OUTPUT_FILE;

std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_IP_MALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_IP_FEMALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_OP_MALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_OP_FEMALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_TS_E_P_MALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_TS_E_P_FEMALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_CR_C_IP_MALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_CR_C_IP_FEMALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_TT_E_P_MALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_TT_E_P_FEMALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_R_MALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_SS_E_R_FEMALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_B_MALE;
std::vector<session_cpp::Polyline> CUSTOM_JOINTS_B_FEMALE;

// ═══════════════════════════════════════════════════════════════════════════
// Loader
// ═══════════════════════════════════════════════════════════════════════════

namespace {

bool parse_bool(const std::string& s) {
    return s == "true" || s == "True" || s == "TRUE" || s == "1" || s == "yes";
}

std::vector<double> parse_doubles(const std::vector<std::string>& xs) {
    std::vector<double> out;
    out.reserve(xs.size());
    for (const std::string& x : xs)
        out.push_back(std::stod(x));
    return out;
}

} // namespace

std::string session_pb(size_t index) {
    if (index >= SESSION_NAMES.size())
        throw std::runtime_error("session_pb: index " + std::to_string(index) + " past the end of SESSION_NAMES");
    return internal::dataset_path(SESSION_NAMES[index], ".pb").string();
}

void reset_defaults() {
    JOINTS_PARAMETERS_AND_TYPES = {
        300, 0.5,  3,
        450, 0.64, 15,
        450, 0.5,  20,
        300, 0.5,  30,
          6, 0.95, 40,
        300, 0.5,  58,
        300, 1.0,  60,
    };
    JOINT_VOLUME_EXTENSION = {0.0, 0.0, 0.0};
    JOINT_SCALE = {1.0, 1.0, 1.0};
    FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE = 150.0;
    FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED = false;
    FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE = false;
    DISTANCE = 0.1;
    DISTANCE_SQUARED = 0.01;
    ANGLE = 0.11;
    DUPLICATE_PTS_TOL = 0.0;
    LIMIT_MIN_JOINT_LENGTH = 0.0;
    CLIPPER_SCALE = 1000000;
    CLIPPER_AREA = 0.01;
    DATA_SET_INPUT_NAME.clear();
    DATA_SET_OBJ.clear();
    DATA_SET_ADJACENCY.clear();
    DATA_SET_THREE_VALENCE.clear();
    DATA_SET_INSERTION_VECTORS.clear();
    DATA_SET_JOINTS_TYPES.clear();
    DATA_SET_OUTPUT_FILE.clear();
    SEARCH_TYPE = face_to_face;
    BEAMS.clear();

    CUSTOM_JOINTS_SS_E_IP_MALE.clear();
    CUSTOM_JOINTS_SS_E_IP_FEMALE.clear();
    CUSTOM_JOINTS_SS_E_OP_MALE.clear();
    CUSTOM_JOINTS_SS_E_OP_FEMALE.clear();
    CUSTOM_JOINTS_TS_E_P_MALE.clear();
    CUSTOM_JOINTS_TS_E_P_FEMALE.clear();
    CUSTOM_JOINTS_CR_C_IP_MALE.clear();
    CUSTOM_JOINTS_CR_C_IP_FEMALE.clear();
    CUSTOM_JOINTS_TT_E_P_MALE.clear();
    CUSTOM_JOINTS_TT_E_P_FEMALE.clear();
    CUSTOM_JOINTS_SS_E_R_MALE.clear();
    CUSTOM_JOINTS_SS_E_R_FEMALE.clear();
    CUSTOM_JOINTS_B_MALE.clear();
    CUSTOM_JOINTS_B_FEMALE.clear();
}

void globals_yaml(const std::string& dataset_name) {
    reset_defaults();

    const std::filesystem::path path = internal::dataset_path(dataset_name, ".yml");
    if (!std::filesystem::exists(path))
        throw std::runtime_error("globals_yaml: missing config " + path.string());

    TINY_YAML::Yaml y(path.string());

    // Every read is gated by y.has(k) and hasData(): TinyYaml null-derefs on an absent key or a bare `key:`.
    auto str = [&](const char* k) -> std::string {
        if (!y[k].hasData())
            throw std::runtime_error(std::string("globals_yaml: key '") + k + "' is present but has no value");
        return y[k].getData<std::string>();
    };
    auto list = [&](const char* k) -> std::vector<std::string> {
        if (!y[k].hasData())
            throw std::runtime_error(std::string("globals_yaml: key '") + k + "' is present but has no value");
        return y[k].getData<std::vector<std::string>>();
    };

    if (y.has("joints_parameters_and_types")) {
        const std::vector<std::string> jpt = list("joints_parameters_and_types");
        if (!jpt.empty()) {
            std::vector<double> parsed = parse_doubles(jpt);
            if (parsed.size() < 21 || parsed.size() % 3 != 0)
                throw std::runtime_error(
                    "globals_yaml: joints_parameters_and_types has " + std::to_string(parsed.size()) +
                    " values; expected at least 21 (7 families x 3) in multiples of 3");
            JOINTS_PARAMETERS_AND_TYPES = std::move(parsed);
        }
    }
    if (y.has("joint_volume_extension")) {
        std::vector<double> parsed = parse_doubles(list("joint_volume_extension"));
        if (parsed.size() < 3 || parsed.size() % 3 != 0)
            throw std::runtime_error(
                "globals_yaml: joint_volume_extension has " + std::to_string(parsed.size()) +
                " values; expected 3 (every joint type) or a multiple of 3 (one triple per type)");
        JOINT_VOLUME_EXTENSION = std::move(parsed);
    }
    if (y.has("joint_scale")) {
        const std::vector<double> s = parse_doubles(list("joint_scale"));
        if (s.size() != 3)
            throw std::runtime_error("globals_yaml: joint_scale needs 3 values, has " + std::to_string(s.size()));
        JOINT_SCALE = {s[0], s[1], s[2]};
    }
    if (y.has("search_type")) {
        const std::string search = str("search_type");
        if (search == "face_to_face")
            SEARCH_TYPE = face_to_face;
        else if (search == "cross_joint")
            SEARCH_TYPE = cross_joint;
        else if (search == "face_to_face_then_cross")
            SEARCH_TYPE = face_to_face_then_cross;
        else
            throw std::runtime_error("globals_yaml: search_type '" + search + "' is not face_to_face, cross_joint or face_to_face_then_cross");
    }
    if (y.has("beams")) {
        BEAMS = parse_doubles(list("beams"));
        if (BEAMS.size() != 6)
            throw std::runtime_error("globals_yaml: beams needs 6 values [radius, allowed type, min_distance, volume_length, cross_or_side_to_end, flip_male], has " + std::to_string(BEAMS.size()));
    }
    if (y.has("face_to_face_side_to_side_joints_dihedral_angle"))
        FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE = std::stod(str("face_to_face_side_to_side_joints_dihedral_angle"));
    if (y.has("face_to_face_side_to_side_joints_all_treated_as_rotated"))
        FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED = parse_bool(str("face_to_face_side_to_side_joints_all_treated_as_rotated"));
    if (y.has("face_to_face_side_to_side_joints_rotated_joint_as_average"))
        FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE = parse_bool(str("face_to_face_side_to_side_joints_rotated_joint_as_average"));
    if (y.has("distance"))
        DISTANCE = std::stod(str("distance"));
    if (y.has("distance_squared"))
        DISTANCE_SQUARED = std::stod(str("distance_squared"));
    if (y.has("angle"))
        ANGLE = std::stod(str("angle"));
    if (y.has("duplicate_pts_tol"))
        DUPLICATE_PTS_TOL = std::stod(str("duplicate_pts_tol"));
    if (y.has("limit_min_joint_length"))
        LIMIT_MIN_JOINT_LENGTH = std::stod(str("limit_min_joint_length"));
    if (y.has("clipper_scale"))
        CLIPPER_SCALE = std::stoll(str("clipper_scale"));
    if (y.has("clipper_area"))
        CLIPPER_AREA = std::stod(str("clipper_area"));

    // File keys resolve relative to the yaml; naming a file that is not there is an error.
    auto file = [&](const char* k, std::string& out) {
        if (!y.has(k))
            return;
        const std::filesystem::path p = path.parent_path() / str(k);
        if (!std::filesystem::exists(p))
            throw std::runtime_error(std::string("globals_yaml: ") + k + " names a missing file " + p.string());
        out = p.string();
    };
    file("obj", DATA_SET_OBJ);
    file("adjacency", DATA_SET_ADJACENCY);
    file("three_valence", DATA_SET_THREE_VALENCE);
    file("insertion_vectors", DATA_SET_INSERTION_VECTORS);
    file("joints_types", DATA_SET_JOINTS_TYPES);
    DATA_SET_INPUT_NAME = path.stem().string();
    DATA_SET_OUTPUT_FILE = "WoodF2F_" + DATA_SET_INPUT_NAME + ".pb";
}

} // namespace globals
} // namespace wood_session
