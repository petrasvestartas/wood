#include "pch.h"
#include "wood_io.h"
#include "wood_config.h"
#include "wood_session.h"
using namespace session_cpp;

namespace wood_session {
namespace io {

// ═══════════════════════════════════════════════════════════════════════════
// Sidecars and obj
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::pair<int, int>> load_adjacency(const std::string& adjacency_name) {

    std::vector<std::pair<int, int>> pairs;
    if (adjacency_name.empty())
        return pairs;

    std::ifstream file(adjacency_name);
    int a;
    int b;
    while (file >> a >> b)
        pairs.emplace_back(a, b);

    return pairs;
}

std::vector<std::vector<Vector>> load_insertion_vectors(const std::string& insertion_vectors_name, size_t count) {

    std::vector<std::vector<Vector>> per_element(count);
    if (insertion_vectors_name.empty())
        return per_element;

    std::ifstream file(insertion_vectors_name);
    std::string line;
    size_t element_index = 0;
    while (std::getline(file, line) && element_index < count) {
        std::istringstream stream(line);
        double x;
        double y;
        double z;
        while (stream >> x >> y >> z)
            per_element[element_index].emplace_back(x, y, z);
        element_index++;
    }

    return per_element;
}

std::vector<std::vector<int>> load_feature_types(const std::string& joint_types_name, size_t count) {

    std::vector<std::vector<int>> per_element(count);
    if (joint_types_name.empty())
        return per_element;

    std::ifstream file(joint_types_name);
    std::string line;
    size_t element_index = 0;
    while (std::getline(file, line) && element_index < count) {
        std::istringstream stream(line);
        int value;
        while (stream >> value)
            per_element[element_index].push_back(value);
        element_index++;
    }

    return per_element;
}

std::vector<std::vector<int>> load_three_valence(const std::string& three_valence_name) {

    std::vector<std::vector<int>> rows;
    if (three_valence_name.empty())
        return rows;

    std::ifstream file(three_valence_name);
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::vector<int> row;
        int value;
        while (stream >> value)
            row.push_back(value);
        if (!row.empty())
            rows.push_back(row);
    }

    return rows;
}

std::vector<Polyline> load_obj(const std::string& dataset_name, double duplicate_points_tolerance) {

    const double tolerance = duplicate_points_tolerance;
    const std::filesystem::path path = config::dataset_path(dataset_name, ".obj");
    if (!std::filesystem::exists(path))
        throw std::runtime_error("load_obj: dataset OBJ not found: " + path.string());

    std::vector<Polyline> polylines = file_obj::read_file_obj_polylines(path.string());
    if (polylines.empty())
        throw std::runtime_error("load_obj: no polylines in " + path.string());

    if (tolerance > 0.0)
        for (Polyline& polyline : polylines)
            polyline.remove_consecutive_duplicates(tolerance);

    return polylines;
}

// ═══════════════════════════════════════════════════════════════════════════
// Writing a scene
// ═══════════════════════════════════════════════════════════════════════════


namespace {

/// A plate's merged outlines in the legacy interleaved layout: [hole0_top, hole0_bot, ..., outer_top, outer_bot].
std::vector<Polyline> merged_outlines(const Features& f) {

    std::vector<Polyline> merged;
    if (f.top.empty())
        return merged;

    merged.reserve(f.top.size() * 2);
    for (size_t i = 1; i < f.top.size(); i++) {
        merged.push_back(f.top[i]);
        merged.push_back(f.bottom[i]);
    }
    merged.push_back(f.top[0]);
    merged.push_back(f.bottom[0]);

    return merged;
}

}  // namespace

std::filesystem::path pb_path(const std::string& name) {
    const std::filesystem::path dir = config::output_dir() / "pb";
    std::filesystem::create_directories(dir);
    return dir / (name + ".pb");
}

void write_parity_dumps(const WoodSession& scene, const std::filesystem::path& pb) {

    const std::vector<std::shared_ptr<Plate>> plates = scene.plates();
    std::ofstream meta(pb.string() + "_meta.txt");
    std::ofstream coords(pb.string() + "_coords.txt");

    for (size_t ei = 0; ei < plates.size(); ei++) {

        const std::vector<Polyline> merged = merged_outlines(plates[ei]->features);
        meta << merged.size();
        for (size_t mi = 0; mi < merged.size(); mi++)
            meta << ' ' << merged[mi].point_count();
        meta << '\n';

        coords << "element " << ei << "\n";
        for (size_t mi = 0; mi < merged.size(); mi++) {
            coords << "  poly " << mi << ":";
            for (size_t pi = 0; pi < merged[mi].point_count(); pi++) {
                const Point p = merged[mi].get_point(pi);
                coords << " " << p[0] << " " << p[1] << " " << p[2];
            }
            coords << "\n";
        }
    }
}

}} // namespace wood_session::io
