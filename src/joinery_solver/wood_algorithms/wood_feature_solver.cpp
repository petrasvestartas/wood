#include "pch.h"
#include "wood_session.h"
#include "wood_contact_detection.h"
#include "wood_feature_detection.h"
#include "wood_merge_modifier.h"
#include "wood_three_valence.h"
using namespace session_cpp;

namespace {
using namespace wood_session;

/// membership[element][face] = [(joint index, is_male)]; shadow joints go to the extra last slot.
std::vector<std::vector<std::vector<std::pair<int, bool>>>> joint_membership_per_face(
    const std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<InteractionFeaturePlate>& all_joints) {

    const size_t element_count = elements.size();
    std::vector<std::vector<std::vector<std::pair<int, bool>>>> membership(element_count);
    for (size_t element_index = 0; element_index < element_count; element_index++)
        membership[element_index].resize(elements[element_index]->planes.size() + 1);

    for (size_t joint_index = 0; joint_index < all_joints.size(); joint_index++) {

        const InteractionFeaturePlate& joint = all_joints[joint_index];
        const int element0 = index_of_plate(elements, joint.element_a);
        const int element1 = index_of_plate(elements, joint.element_b);

        if (joint.link) {
            if (element0 >= 0 && element0 < (int)element_count)
                membership[element0].back().push_back({(int)joint_index, true});
            if (element1 >= 0 && element1 < (int)element_count)
                membership[element1].back().push_back({(int)joint_index, false});
        } else {
            const int face0 = joint.contact.face_a;
            const int face1 = joint.contact.face_b;
            if (element0 >= 0 && element0 < (int)element_count && face0 >= 0 && face0 < (int)membership[element0].size())
                membership[element0][face0].push_back({(int)joint_index, true});
            if (element1 >= 0 && element1 < (int)element_count && face1 >= 0 && face1 < (int)membership[element1].size())
                membership[element1][face1].push_back({(int)joint_index, false});
        }
    }
    return membership;
}

} // anonymous namespace

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Joints
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::pair<int, int>> WoodSession::adjacent_pairs(const std::vector<std::shared_ptr<Plate>>& elements) const {

    if (!adjacency.empty())
        return adjacency;

    return adjacency_search(std::vector<std::shared_ptr<Element>>(elements.begin(), elements.end()), settings.distance);
}

std::vector<InteractionFeaturePlate> WoodSession::detect_features(const std::vector<std::shared_ptr<Plate>>& elements, const std::vector<std::pair<int, int>>& pairs, SearchType search_type) {

    const int element_count = static_cast<int>(elements.size());

    std::vector<InteractionFeaturePlate> joints;
    joints.reserve(pairs.size());
    for (size_t k = 0; k < pairs.size(); ++k) {

        const int index_a = pairs[k].first;
        const int index_b = pairs[k].second;
        if (index_a < 0 || index_b < 0 || index_a >= element_count || index_b >= element_count) {
            std::cerr << fmt::format("  WARNING: adjacency pair {} references elements ({}, {}) but only {} were loaded - skipping.\n", k, index_a, index_b, element_count);
            continue;
        }

        InteractionFeaturePlate joint;
        bool swap_planes_b = false;
        const bool ok = face_to_face_wood(
            *elements[index_a],
            *elements[index_b],
            {index_a, index_b},
            settings,
            search_type,
            joint,
            swap_planes_b
        );

        if (swap_planes_b)
            elements[index_b]->flip();

        if (!ok)
            continue;

        joint.guid() = ::guid();
        joints.push_back(std::move(joint));
    }

    return joints;
}

void WoodSession::build_feature_geometry(std::vector<std::shared_ptr<Plate>>& elements, std::vector<InteractionFeaturePlate>& joints, const std::vector<std::vector<int>>& feature_types) {
    JointPlate::build_geometry(
        joints,
        elements,
        feature_types,
        settings
    );
}

void WoodSession::merge_features(const std::vector<std::shared_ptr<Plate>>& elements, std::vector<InteractionFeaturePlate>& joints) {

    const std::vector<std::vector<std::vector<std::pair<int, bool>>>> membership = joint_membership_per_face(elements, joints);
    const size_t element_count = elements.size();
    for (size_t element_index = 0; element_index < element_count; element_index++) {

        std::vector<Polyline> merged = wood_session::MergeModifier::apply(
            *elements[element_index],
            membership[element_index],
            joints,
            (int)element_index,
            settings.distance_squared
        );
        wood_session::Features& features = elements[element_index]->features;
        features.top.clear();
        features.bottom.clear();

        // the bottom of every pair is the loop on the side of polylines[0], as Plate names its faces
        if (merged.size() >= 2) {
            const size_t hole_count = merged.size() - 2;
            features.top.reserve(1 + hole_count / 2);
            features.bottom.reserve(1 + hole_count / 2);
            features.bottom.push_back(std::move(merged[merged.size() - 2]));
            features.top.push_back(std::move(merged[merged.size() - 1]));
            for (size_t hole_index = 0; hole_index + 2 <= hole_count; hole_index += 2) {
                features.bottom.push_back(std::move(merged[hole_index]));
                features.top.push_back(std::move(merged[hole_index + 1]));
            }
        }

        elements[element_index]->invalidate_geometry();
    }
}

void WoodSession::load_sidecars(const std::vector<std::shared_ptr<Plate>>& elements) {

    if (adjacency.empty())
        adjacency = io::load_adjacency(config::DATA_SET_ADJACENCY);
    if (three_valence.empty())
        three_valence = io::load_three_valence(config::DATA_SET_THREE_VALENCE);

    const std::vector<std::vector<Vector>> vectors = io::load_insertion_vectors(config::DATA_SET_INSERTION_VECTORS, elements.size());
    const std::vector<std::vector<int>> types = io::load_feature_types(config::DATA_SET_JOINTS_TYPES, elements.size());
    for (size_t i = 0; i < elements.size(); ++i) {
        if (elements[i]->insertion_vectors().empty() && i < vectors.size())
            elements[i]->insertion_vectors() = vectors[i];
        if (elements[i]->feature_types.empty() && i < types.size())
            elements[i]->feature_types = types[i];
    }
}

/// A reversed plate lists its side slots backwards, so its insertion vectors are read in the same order.
std::vector<InteractionFeaturePlate> WoodSession::compute_features() {
    return compute_features(settings.search_type);
}

/// Instances take part as world views: one a joint lands on is promoted before the joint is stored, any other dropped unchanged; a stored plate placed off identity takes its view back.
std::vector<InteractionFeaturePlate> WoodSession::compute_features(SearchType search_type) {

    std::vector<std::shared_ptr<Plate>> elements = world_elements<Plate>();
    if (elements.empty())
        return {};

    clear_features();

    std::vector<std::vector<int>> feature_types(elements.size());
    for (size_t i = 0; i < elements.size(); ++i) {

        std::vector<Vector>& vectors = elements[i]->insertion_vectors();
        if (elements[i]->reversed && vectors.size() > 2)
            std::reverse(vectors.begin() + 2, vectors.end());

        feature_types[i] = elements[i]->feature_types;
    }

    std::vector<InteractionFeaturePlate> joints = detect_features(elements, adjacent_pairs(elements), search_type);
    link_three_valence_joints(
        three_valence,
        elements,
        joints,
        settings.angle
    );
    build_feature_geometry(elements, joints, feature_types);
    merge_features(elements, joints);

    std::unordered_set<std::string> jointed;
    for (const InteractionFeaturePlate& joint : joints) {
        jointed.insert(joint.element_a);
        jointed.insert(joint.element_b);
    }

    for (const std::shared_ptr<Plate>& plate : elements)
        if (jointed.count(plate->guid()) || !instance_lookup.count(plate->guid()))
            promote(plate);

    std::vector<int> parent(joints.size());
    for (size_t i = 0; i < parent.size(); ++i)
        parent[i] = static_cast<int>(i);
    auto root = [&](int i) { while (parent[i] != i) i = parent[i]; return i; };
    auto unite = [&](int a, int b) { if (a >= 0 && b >= 0) parent[root(b)] = root(a); };
    std::unordered_map<std::string, int> joint_indices;
    for (size_t i = 0; i < joints.size(); ++i)
        joint_indices[joints[i].guid()] = static_cast<int>(i);
    for (size_t i = 0; i < joints.size(); ++i)
        for (const auto& linked : joints[i].linked_joints)
            if (joint_indices.count(linked))
                unite(static_cast<int>(i), joint_indices.at(linked));
    const bool annen = three_valence.size() > 1 && (three_valence[0].empty() || three_valence[0][0] == 0);
    if (annen) {
        const auto pairs = joints_by_element_pair(elements, joints);
        for (size_t i = 1; i < three_valence.size(); ++i) {
            const auto& group = three_valence[i];
            if (group.size() < 4)
                continue;
            const auto a = pairs.find(pair_key(group[0], group[1]));
            const auto b = pairs.find(pair_key(group[2], group[3]));
            if (a != pairs.end() && b != pairs.end())
                unite(a->second, b->second);
        }
    }
    std::map<int, std::vector<int>> groups;
    for (size_t i = 0; i < joints.size(); ++i)
        groups[root(static_cast<int>(i))].push_back(static_cast<int>(i));
    for (const auto& [id, indices] : groups) {
        std::shared_ptr<JointPlate> element;
        if (indices.size() > 1 && annen)
            element = std::make_shared<JointAnnen>();
        else if (indices.size() > 1)
            element = std::make_shared<JointVidy>();
        else
            element = std::make_shared<JointPlate>();
        for (int index : indices) {
            auto& joint = joints[index];
            joint.sync_features();
            const auto male = get_element<Element>(joint.element_a);
            const auto female = get_element<Element>(joint.element_b);
            if (!male || !female)
                continue;
            add_interaction(male, female, joint.to_contact());
            element->connections.push_back(joint);
        }
        if (element->connections.empty())
            continue;
        element->name = indices.size() > 1 ? element->element_type_name() : element->connections[0].name;
        element->generated = true;
        apply_joint(element, false);
    }

    return joints;
}

} // namespace wood_session

std::vector<wood_session::InteractionFeaturePlate> get_connection_zones(std::vector<std::shared_ptr<wood_session::Plate>>& elements, const wood_session::Settings& settings, SearchType search_type) {

    wood_session::WoodSession scene(wood_session::config::DATA_SET_INPUT_NAME);
    scene.settings = settings;
    for (const std::shared_ptr<wood_session::Plate>& plate : elements)
        scene.add(plate);

    return scene.compute_features(search_type);
}
