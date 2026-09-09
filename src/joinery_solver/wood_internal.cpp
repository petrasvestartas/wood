// ═══════════════════════════════════════════════════════════════════════════
// wood/wood_internal.cpp — internal helpers for the wood pipeline.
// Declarations in wood_session.h.
// ═══════════════════════════════════════════════════════════════════════════
#include "wood_session.h"
#include "../src/file_obj.h"
#include "../src/polyline.h"
#include "../src/point.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

using namespace session_cpp;

namespace internal {

// Absolute path to `data/` at the repo root. Centralises the
// `__FILE__.parent_path() × 2` walk so relocating this translation unit
// stays a one-line change.
std::filesystem::path session_data_dir() {
    return std::filesystem::path(__FILE__)
        .parent_path()   // joinery_solver/
        .parent_path()   // src/
        .parent_path()   // repo root
        / "data";
}

std::filesystem::path output_dir() {
    auto out = session_data_dir() / "output";
    std::filesystem::create_directories(out);
    return out;
}

// A name is data/<name>.obj; a path ending in .obj is itself.
static std::filesystem::path obj_path_of(const std::string& dataset_name) {
    return dataset_name.ends_with(".obj") ? std::filesystem::path(dataset_name) : session_data_dir() / (dataset_name + ".obj");
}

bool plates_exist(const std::string& name) {
    return std::filesystem::exists(obj_path_of(name));
}

// Load dataset → WoodElements.
// Sets DATA_SET_INPUT_NAME + DATA_SET_OUTPUT_FILE globals.
std::vector<wood_session::WoodElement> load_plates(const std::string& dataset_name, double duplicate_pts_tol) {
    // The YAML knob was dead: globals_yaml() sets DUPLICATE_PTS_TOL from the
    // config, and this unconditional overwrite with the parameter default
    // (0.0) then stomped it before any dedup could run. Parameter wins when
    // explicitly set; otherwise the config value applies. (Every shipped
    // config currently says 0.0, so behaviour is unchanged until someone
    // actually uses the knob - which is the point.)
    double effective_tol = duplicate_pts_tol > 0.0
                               ? duplicate_pts_tol
                               : wood_session::globals::DUPLICATE_PTS_TOL;
    wood_session::globals::DUPLICATE_PTS_TOL = effective_tol;
    duplicate_pts_tol = effective_tol;

    const std::string obj_path  = obj_path_of(dataset_name).string();
    const std::string obj_short = obj_path_of(dataset_name).stem().string();
    // read_file_obj_polylines opens an ifstream and just loops getline: a
    // missing or unreadable file yields an EMPTY vector, no error. Combined
    // with the __FILE__-baked data dir, that meant zero elements, a full
    // pipeline run on nothing, and a green test - the false-green this repo
    // has already shipped once. Fail loudly instead; callers that want to
    // skip missing datasets check existence first (plates_exist).
    if (!std::filesystem::exists(obj_path)) {
        throw std::runtime_error("load_plates: dataset OBJ not found: " + obj_path);
    }
    auto polylines = file_obj::read_file_obj_polylines(obj_path);
    if (polylines.empty()) {
        throw std::runtime_error("load_plates: no polylines in " + obj_path);
    }

    if (duplicate_pts_tol > 0.0) {
        for (auto& pl : polylines) {
            pl.remove_consecutive_duplicates(duplicate_pts_tol);
        }
    }

    wood_session::globals::DATA_SET_INPUT_NAME  = obj_short;
    wood_session::globals::DATA_SET_OUTPUT_FILE = "WoodF2F_" + obj_short + ".pb";

    std::vector<wood_session::WoodElement> elements;
    elements.reserve(polylines.size() / 2);
    for (size_t i = 0; i + 1 < polylines.size(); i += 2) {
        elements.emplace_back(polylines[i], polylines[i + 1]);
    }
    return elements;
}

// Load raw polylines without pairing — for beam datasets where each polyline is an axis.
std::vector<Polyline> load_polylines(const std::string& dataset_name, double duplicate_pts_tol) {
    wood_session::globals::DUPLICATE_PTS_TOL = duplicate_pts_tol;

    const std::string obj_path  = obj_path_of(dataset_name).string();
    const std::string obj_short = obj_path_of(dataset_name).stem().string();
    auto polylines = file_obj::read_file_obj_polylines(obj_path);

    if (duplicate_pts_tol > 0.0) {
        for (auto& pl : polylines) {
            pl.remove_consecutive_duplicates(duplicate_pts_tol);
        }
    }

    wood_session::globals::DATA_SET_INPUT_NAME  = obj_short;
    wood_session::globals::DATA_SET_OUTPUT_FILE = "WoodF2F_" + obj_short + ".pb";
    return polylines;
}

} // namespace internal
