#include "wood_session.h"
#include "wood_face_to_face.h"
#include "wood_element.h"
#include "../src/session.h"

#include <fmt/core.h>

#include <filesystem>
#include <map>
#include <string>

// A dataset yaml name (hexboxes -> data/hexboxes.yml) or a session .pb name (floor_model -> data/floor_model.pb).
const char* DATASET = "hexboxes";

static void report(const std::string& heading, const std::map<std::string, int>& tally, const size_t total) {
    fmt::print("{} ({}):\n", heading, total);
    for (const auto& [label, count] : tally)
        fmt::print("  {:<12} {}\n", label, count);
}

static wood_session::WoodSession load(const std::string& name) {
    const std::filesystem::path yaml = internal::session_data_dir() / (name + ".yml");
    if (std::filesystem::exists(yaml))
        return wood_session::WoodSession::yaml_load(yaml);
    wood_session::globals::reset_defaults();
    return wood_session::WoodSession::pb_load(internal::session_data_dir() / (name + ".pb"));
}

int main() {
    wood_session::WoodSession scene = load(DATASET);
    if (scene.size() == 0) {
        fmt::print(stderr, "not found: data/{}.yml or data/{}.pb\n", DATASET, DATASET);
        return 1;
    }
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

    fmt::print("{}: {} plates, {} columns, {} solids\n", DATASET, scene.plates().size(), scene.columns().size(), scene.solids().size());
    report("contacts", contacts, total);
    report("joints", joints, scene.joints().size());
    scene.pb_dump(wood_session::pb_path("live"));
    return 0;
}

/*
description: contacts (every touching face pair, by contact type) and joints (what the solver made of them, by joint type) of one dataset -> data/output/pb/live.pb.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_face_to_face -j8 && ./build/main_face_to_face
cloudflare: ../bash/publish-scene.sh --target main_face_to_face
view: https://petrasvestartas.github.io/session/
*/
