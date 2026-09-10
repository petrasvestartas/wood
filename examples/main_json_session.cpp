#include "session.h"
#include "element.h"
#include "intersection.h"
#include "json.h"
#include "wood_element.h"
#include "wood_session.h"

#include <fmt/core.h>

#include <filesystem>
#include <fstream>

using namespace session_cpp;

const char* INPUT = "WoodStep3_data.json";
const char* OUTPUT = "WoodStep4.pb";
const double COPLANAR_TOLERANCE = 5.0;

static std::vector<Point> from_json(const nlohmann::json& array) {
    std::vector<Point> points;
    points.reserve(array.size());
    for (const auto& p : array)
        points.emplace_back(p[0].get<double>(), p[1].get<double>(), p[2].get<double>());
    return points;
}

int main() {
    const std::filesystem::path input = internal::output_dir() / INPUT;
    std::ifstream file(input);
    if (!file) {
        fmt::print(stderr, "not found: {}\n", input.string());
        return 1;
    }
    const nlohmann::json data = nlohmann::json::parse(file);
    const std::vector<int> adjacency = data["adjacency"].get<std::vector<int>>();

    Session session("WoodStep4_Joints");
    const auto elements = session.add_group("Elements");
    std::vector<Element*> plates;
    for (size_t i = 0; i < data["elements"].size(); i++) {
        const nlohmann::json& item = data["elements"][i];
        const std::vector<Point> bottom = from_json(item["polygon"]);
        const std::vector<Point> top = item.contains("polygon_top") ? from_json(item["polygon_top"]) : bottom;
        const wood_session::WoodElement plate{Polyline(bottom), Polyline(top)};
        const auto element = std::make_shared<Element>(plate.loft_mesh(), "plate_" + std::to_string(i));
        session.add_element(element, elements);
        plates.push_back(element.get());
    }

    const auto joints = session.add_group("Joints");
    std::vector<std::tuple<int, int, int, int, int, Polyline>> found = Intersection::face_to_face(adjacency, plates, COPLANAR_TOLERANCE);
    for (size_t k = 0; k < found.size(); k++) {
        auto& [a, b, i, j, type, polygon] = found[k];
        const auto outline = std::make_shared<Polyline>(std::move(polygon));
        outline->name = "joint_" + std::to_string(k);
        session.add_polyline(outline, joints);
        session.add_edge(plates[a]->guid(), plates[b]->guid(), std::to_string(i) + "," + std::to_string(j) + "," + std::to_string(type) + "," + outline->guid());
    }
    session.pb_dump((internal::output_dir() / OUTPUT).string());
    return 0;
}

/*
description: plates and adjacency from a step-3 JSON -> face_to_face intersections -> joints as graph edges -> data/output/WoodStep4.pb.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_json_session -j8 && ./build/main_json_session
cloudflare: ../bash/publish-scene.sh data/output/WoodStep4.pb
view: https://petrasvestartas.github.io/session/
*/
