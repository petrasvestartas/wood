#include "wood_session.h"
#include "wood_face_to_face.h"
#include "wood_element.h"
#include "../src/polyline.h"
#include "../src/session.h"

#include <fmt/core.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

// A short OBJ name (hexboxes), a full config name, or a block .pb name (floor_model).
const char* DATASET = "hexboxes";

namespace {

std::filesystem::path config_dir() {
    return internal::session_data_dir().parent_path() / "src" / "config";
}

// Short name -> the config name that ends with it.
std::string resolve(const std::string& name) {
    const std::filesystem::path dir = config_dir();
    if (std::filesystem::exists(dir / (name + ".yml")))
        return name;
    const std::string prefixed = "type_plates_name_" + name;
    if (std::filesystem::exists(dir / (prefixed + ".yml")))
        return prefixed;
    const std::string suffix = "_" + name + ".yml";
    if (!std::filesystem::exists(dir))
        return name;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        const std::string file = entry.path().filename().string();
        if (file.size() > suffix.size() && file.compare(file.size() - suffix.size(), suffix.size(), suffix) == 0)
            return entry.path().stem().string();
    }
    return name;
}

void report(const std::string& heading, const std::map<std::string, int>& tally, const size_t total) {
    fmt::print("{} ({}):\n", heading, total);
    for (const auto& [label, count] : tally)
        fmt::print("  {:<12} {}\n", label, count);
}

int run_pb(const std::string& name) {
    const std::filesystem::path pb = internal::session_data_dir() / (name + ".pb");
    if (!std::filesystem::exists(pb)) {
        fmt::print(stderr, "not found: {} - neither a config in {} nor a .pb\n", name, config_dir().string());
        return 1;
    }
    wood_session::globals::reset_defaults();
    wood_session::WoodSession scene = wood_session::WoodSession::pb_load(pb);
    scene.compute_contacts();
    scene.compute_joints(face_to_face);

    std::map<std::string, int> contacts;
    size_t total = 0;
    for (const auto& contact : scene.contacts())
        for (const wood_session::FaceContact& face : contact->faces) {
            contacts[wood_session::contact_type_name(face.type)]++;
            total++;
        }
    std::map<std::string, int> joints;
    for (const auto& joint : scene.joints())
        joints[wood_session::joint_type_name(joint->joint_type)]++;

    fmt::print("{}: {} plates, {} columns, {} solids\n", name, scene.plates().size(), scene.columns().size(), scene.solids().size());
    report("contacts", contacts, total);
    report("joints", joints, scene.joints().size());
    scene.pb_dump(wood_session::pb_path("live"));
    return 0;
}

int run_plates(const std::string& name, const std::string& dataset) {
    wood_session::globals::globals_yaml(dataset);
    const std::vector<wood_session::WoodElement> elements = internal::load_plates(dataset);
    const std::vector<wood_session::ContactPair> pairs = wood_session::face_contacts(elements);

    // The solver mutates its elements, so it gets a copy and the Inputs group keeps the loaded geometry.
    std::vector<wood_session::WoodElement> solved = elements;
    const std::vector<wood_session::WoodJoint> found = get_connection_zones(solved, face_to_face);

    std::map<std::string, int> contacts;
    size_t total = 0;
    for (const wood_session::ContactPair& pair : pairs)
        for (const wood_session::FaceContact& face : pair.faces) {
            contacts[wood_session::contact_type_name(face.type)]++;
            total++;
        }
    std::map<std::string, int> joints;
    for (const wood_session::WoodJoint& joint : found)
        joints[wood_session::joint_type_name(joint.joint_type)]++;

    fmt::print("{}: {} plates\n", name, elements.size());
    report("contacts", contacts, total);
    report("joints", joints, found.size());

    session_cpp::Session session(fmt::format("wood - contacts and joints - {}", name));
    wood_session::add_faces(session, session.add_group("Inputs"), elements);
    wood_session::add_contacts_by_type(session, pairs);
    wood_session::add_joints_by_type(session, found);
    wood_session::pb_dump(session, "live");
    return 0;
}

} // namespace

int main() {
    const std::string dataset = resolve(DATASET);
    if (!internal::plates_exist(dataset))
        return run_pb(DATASET);
    return run_plates(DATASET, dataset);
}

/*
description: contacts (every touching face pair, by contact type) and joints (what the solver made of them, by joint type) of one dataset -> data/output/pb/live.pb.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_face_to_face -j8 && ./build/main_face_to_face
cloudflare: ../bash/publish-scene.sh --target main_face_to_face
view: https://petrasvestartas.github.io/session/
*/
