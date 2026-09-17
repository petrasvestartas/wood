#include "wood_pch.h"
#include "wood_session.h"
using namespace session_cpp;

namespace internal {

std::filesystem::path session_data_dir() {
    return std::filesystem::path(wood_session::globals::DATA_SET_INPUT_FOLDER);
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

std::vector<std::shared_ptr<wood_session::Plate>> load_plates(const std::string& dataset_name, double duplicate_pts_tol) {
    const std::vector<Polyline> polylines = load_polylines(dataset_name, duplicate_pts_tol);
    if (polylines.size() % 2 != 0)
        throw std::runtime_error("load_plates: unpaired outline in " + dataset_name);
    std::vector<std::shared_ptr<wood_session::Plate>> elements;
    elements.reserve(polylines.size() / 2);
    for (size_t i = 0; i < polylines.size(); i += 2)
        elements.push_back(std::make_shared<wood_session::Plate>(polylines[i], polylines[i + 1]));
    return elements;
}

std::vector<Polyline> load_polylines(const std::string& dataset_name, double duplicate_pts_tol) {
    const double tolerance = duplicate_pts_tol > 0.0 ? duplicate_pts_tol : wood_session::globals::DUPLICATE_PTS_TOL;
    const std::filesystem::path path = dataset_path(dataset_name, ".obj");
    if (!std::filesystem::exists(path))
        throw std::runtime_error("load_polylines: dataset OBJ not found: " + path.string());
    std::vector<Polyline> polylines = file_obj::read_file_obj_polylines(path.string());
    if (polylines.empty())
        throw std::runtime_error("load_polylines: no polylines in " + path.string());
    if (tolerance > 0.0)
        for (Polyline& polyline : polylines)
            polyline.remove_consecutive_duplicates(tolerance);
    wood_session::globals::DUPLICATE_PTS_TOL = tolerance;
    return polylines;
}

}  // namespace internal
