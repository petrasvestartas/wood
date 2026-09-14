#include "wood_session.h"
#include "../src/file_obj.h"
#include "../src/polyline.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

using namespace session_cpp;

namespace internal {

std::filesystem::path session_data_dir() {
    return std::filesystem::path(wood_session::globals::DATA_SET_INPUT_FOLDER);
}

std::filesystem::path output_dir() {
    const auto path = session_data_dir() / "output";
    std::filesystem::create_directories(path);
    return path;
}

std::filesystem::path dataset_path(const std::string& name, const std::string& ext) {
    return name.ends_with(ext) ? std::filesystem::path(name) : session_data_dir() / (name + ext);
}

bool plates_exist(const std::string& name) {
    return std::filesystem::exists(dataset_path(name, ".obj"));
}

std::vector<wood_session::WoodElement> load_plates(const std::string& dataset_name, double duplicate_pts_tol) {
    const auto polylines = load_polylines(dataset_name, duplicate_pts_tol);
    if (polylines.size() % 2 != 0)
        throw std::runtime_error("load_plates: unpaired outline in " + dataset_name);
    std::vector<wood_session::WoodElement> elements;
    elements.reserve(polylines.size() / 2);
    for (size_t i = 0; i < polylines.size(); i += 2)
        elements.emplace_back(polylines[i], polylines[i + 1]);
    return elements;
}

std::vector<Polyline> load_polylines(const std::string& dataset_name, double duplicate_pts_tol) {
    const double tolerance = duplicate_pts_tol > 0.0 ? duplicate_pts_tol : wood_session::globals::DUPLICATE_PTS_TOL;
    const auto path = dataset_path(dataset_name, ".obj");
    if (!std::filesystem::exists(path))
        throw std::runtime_error("load_polylines: dataset OBJ not found: " + path.string());
    auto polylines = file_obj::read_file_obj_polylines(path.string());
    if (polylines.empty())
        throw std::runtime_error("load_polylines: no polylines in " + path.string());
    if (tolerance > 0.0)
        for (auto& polyline : polylines)
            polyline.remove_consecutive_duplicates(tolerance);
    wood_session::globals::DUPLICATE_PTS_TOL = tolerance;
    wood_session::globals::DATA_SET_INPUT_NAME = path.stem().string();
    wood_session::globals::DATA_SET_OUTPUT_FILE = "WoodF2F_" + path.stem().string() + ".pb";
    return polylines;
}

}
