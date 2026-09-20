#include "pch.h"
#include "wood_session.h"
#include "yaml.hpp"
using namespace session_cpp;

namespace wood_session {
namespace config {

// ═══════════════════════════════════════════════════════════════════════════
// Definitions
// ═══════════════════════════════════════════════════════════════════════════

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

/// The string value of a yaml key; TinyYaml null-derefs on a bare `key:`, so a present key without a value is an error.
std::string yaml_string(TINY_YAML::Yaml& y, const std::string& key) {

    if (!y[key].hasData())
        throw std::runtime_error("load_yaml: key '" + key + "' is present but has no value");

    return y[key].getData<std::string>();
}

/// The list-of-strings value of a yaml key; a present key without a value is an error.
std::vector<std::string> yaml_string_list(TINY_YAML::Yaml& y, const std::string& key) {

    if (!y[key].hasData())
        throw std::runtime_error("load_yaml: key '" + key + "' is present but has no value");

    return y[key].getData<std::vector<std::string>>();
}

/// A file key resolved relative to the yaml into out; naming a file that is not there is an error.
void yaml_file(TINY_YAML::Yaml& y, const std::filesystem::path& path, const std::string& key, std::string& out) {

    if (!y.has(key))
        return;

    const std::filesystem::path p = path.parent_path() / yaml_string(y, key);
    if (!std::filesystem::exists(p))
        throw std::runtime_error("load_yaml: " + key + " names a missing file " + p.string());

    out = p.string();
}

} // namespace

std::string session_pb(size_t index) {

    if (index >= SESSION_NAMES.size())
        throw std::runtime_error("session_pb: index " + std::to_string(index) + " past the end of SESSION_NAMES");

    return dataset_path(SESSION_NAMES[index], ".pb").string();
}

void reset_defaults() {
    DATA_SET_INPUT_NAME.clear();
    DATA_SET_OBJ.clear();
    DATA_SET_ADJACENCY.clear();
    DATA_SET_THREE_VALENCE.clear();
    DATA_SET_INSERTION_VECTORS.clear();
    DATA_SET_JOINTS_TYPES.clear();
    DATA_SET_OUTPUT_FILE.clear();
}

Settings load_yaml(const std::string& dataset_name) {

    reset_defaults();

    const std::filesystem::path path = dataset_path(dataset_name, ".yml");
    if (!std::filesystem::exists(path))
        throw std::runtime_error("load_yaml: missing config " + path.string());

    TINY_YAML::Yaml y(path.string());
    Settings settings;

    // Every read is gated by y.has(k) and hasData(): TinyYaml null-derefs on an absent key or a bare `key:`.
    if (y.has("joints_parameters_and_types")) {
        const std::vector<std::string> jpt = yaml_string_list(y, "joints_parameters_and_types");
        if (!jpt.empty()) {
            std::vector<double> parsed = parse_doubles(jpt);
            if (parsed.size() < 21 || parsed.size() % 3 != 0)
                throw std::runtime_error(
                    "load_yaml: joints_parameters_and_types has " + std::to_string(parsed.size()) +
                    " values; expected at least 21 (7 families x 3) in multiples of 3");
            settings.joint_parameters = std::move(parsed);
        }
    }

    if (y.has("joint_volume_extension")) {
        std::vector<double> parsed = parse_doubles(yaml_string_list(y, "joint_volume_extension"));
        if (parsed.size() < 3 || parsed.size() % 3 != 0)
            throw std::runtime_error(
                "load_yaml: joint_volume_extension has " + std::to_string(parsed.size()) +
                " values; expected 3 (every joint type) or a multiple of 3 (one triple per type)");
        settings.joint_volume_extension = std::move(parsed);
    }

    if (y.has("joint_scale")) {
        const std::vector<double> s = parse_doubles(yaml_string_list(y, "joint_scale"));
        if (s.size() != 3)
            throw std::runtime_error("load_yaml: joint_scale needs 3 values, has " + std::to_string(s.size()));
        settings.joint_scale = {s[0], s[1], s[2]};
    }

    if (y.has("search_type")) {
        const std::string search = yaml_string(y, "search_type");
        if (search == "face_to_face")
            settings.search_type = face_to_face;
        else if (search == "cross_joint")
            settings.search_type = cross_joint;
        else if (search == "face_to_face_then_cross")
            settings.search_type = face_to_face_then_cross;
        else
            throw std::runtime_error("load_yaml: search_type '" + search + "' is not face_to_face, cross_joint or face_to_face_then_cross");
    }

    if (y.has("beams")) {
        settings.beams = parse_doubles(yaml_string_list(y, "beams"));
        if (settings.beams.size() != 6)
            throw std::runtime_error("load_yaml: beams needs 6 values [radius, allowed type, min_distance, volume_length, cross_or_side_to_end, flip_male], has " + std::to_string(settings.beams.size()));
    }

    if (y.has("face_to_face_side_to_side_joints_dihedral_angle"))
        settings.dihedral_angle = std::stod(yaml_string(y, "face_to_face_side_to_side_joints_dihedral_angle"));
    if (y.has("face_to_face_side_to_side_joints_all_treated_as_rotated"))
        settings.all_treated_as_rotated = parse_bool(yaml_string(y, "face_to_face_side_to_side_joints_all_treated_as_rotated"));
    if (y.has("face_to_face_side_to_side_joints_rotated_joint_as_average"))
        settings.rotated_joint_as_average = parse_bool(yaml_string(y, "face_to_face_side_to_side_joints_rotated_joint_as_average"));
    if (y.has("distance"))
        settings.distance = std::stod(yaml_string(y, "distance"));
    if (y.has("distance_squared"))
        settings.distance_squared = std::stod(yaml_string(y, "distance_squared"));
    if (y.has("angle"))
        settings.angle = std::stod(yaml_string(y, "angle"));
    if (y.has("duplicate_pts_tol"))
        settings.duplicate_points_tolerance = std::stod(yaml_string(y, "duplicate_pts_tol"));
    if (y.has("limit_min_joint_length"))
        settings.limit_min_joint_length = std::stod(yaml_string(y, "limit_min_joint_length"));
    if (y.has("clipper_scale"))
        settings.clipper_scale = std::stoll(yaml_string(y, "clipper_scale"));
    if (y.has("clipper_area"))
        settings.clipper_area = std::stod(yaml_string(y, "clipper_area"));

    // File keys resolve relative to the yaml; naming a file that is not there is an error.
    yaml_file(y, path, "obj", DATA_SET_OBJ);
    yaml_file(y, path, "adjacency", DATA_SET_ADJACENCY);
    yaml_file(y, path, "three_valence", DATA_SET_THREE_VALENCE);
    yaml_file(y, path, "insertion_vectors", DATA_SET_INSERTION_VECTORS);
    yaml_file(y, path, "joints_types", DATA_SET_JOINTS_TYPES);

    DATA_SET_INPUT_NAME = path.stem().string();
    DATA_SET_OUTPUT_FILE = "WoodF2F_" + DATA_SET_INPUT_NAME + ".pb";

    return settings;
}

std::filesystem::path session_data_dir() {
    return std::filesystem::path(DATA_SET_INPUT_FOLDER);
}

std::filesystem::path output_dir() {

    const std::filesystem::path path = session_data_dir() / "output";
    std::filesystem::create_directories(path);

    return path;
}

std::filesystem::path dataset_path(const std::string& name, const std::string& ext) {
    return name.ends_with(ext) ? std::filesystem::path(name) : session_data_dir() / (name + ext);
}

bool plates_exist(const std::string& name) {
    return std::filesystem::exists(dataset_path(name, ".obj"));
}

} // namespace config
} // namespace wood_session
