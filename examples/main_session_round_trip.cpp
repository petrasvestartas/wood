#include "wood_session.h"

#include <fmt/core.h>

#include <algorithm>

using namespace session_cpp;
using namespace wood_session;

const int SESSION = 0;   // globals::SESSION_NAMES

static int failures = 0;
static void check(const bool ok, const std::string& what) {
    if (ok)
        return;
    fmt::print("FAIL {}\n", what);
    failures++;
}

static size_t tree_nodes(const Session& session) {
    return session.tree.root() ? session.tree.root()->descendants().size() + 1 : 0;
}

int main() {
    globals::reset_defaults();
    WoodSession a = WoodSession::pb_load(globals::SESSION_NAMES[SESSION]);
    const int vertices_before = a.graph.number_of_vertices();
    a.compute_contacts();
    a.compute_joints();

    const Session reloaded = Session::pb_loads(a.to_session().pb_dumps());
    const WoodSession b = WoodSession::from_session(reloaded);

    check(a.name == b.name, "session name");
    check(a.guid() == b.guid(), "session guid");
    check(a.elements.size() == b.elements.size(), fmt::format("element count ({})", a.elements.size()));
    const std::vector<std::shared_ptr<WoodElement>> plates_a = a.plates();
    const std::vector<std::shared_ptr<WoodElement>> plates_b = b.plates();
    check(plates_a.size() == plates_b.size(), fmt::format("plate count ({})", plates_a.size()));
    check(a.columns().size() == b.columns().size(), "column count");
    check(a.solids().size() == b.solids().size(), "solid count");

    bool shared = true;
    for (const std::shared_ptr<Element>& element : *a.objects.elements)
        shared = shared && a.get_object<Element>(element->guid()) ==
                 std::visit([](const auto& wood) -> std::shared_ptr<Element> { return wood->element; },
                            a.elements.at(element->guid()));
    check(shared, "every wood object and the session hold the SAME Element object");
    check(a.element_guids() == b.element_guids(), "every element guid, in order");

    check(tree_nodes(a) == tree_nodes(b), fmt::format("tree node count ({})", tree_nodes(a)));
    check(a.objects.polylines->size() == b.objects.polylines->size(), "loose polyline count");
    check(a.objects.meshes->size() == b.objects.meshes->size(), "loose mesh count");
    check(a.graph.number_of_vertices() == b.graph.number_of_vertices(), "graph vertex count");
    check(a.graph.number_of_vertices() == vertices_before, "contacts and joints are edge payload, not graph nodes");
    check(a.graph.number_of_edges() == b.graph.number_of_edges(), fmt::format("edge count ({})", a.graph.number_of_edges()));

    const std::vector<ContactPair> contacts_a = a.contacts();
    const std::vector<ContactPair> contacts_b = b.contacts();
    bool rings = contacts_a.size() == contacts_b.size();
    check(rings, fmt::format("contact pair count ({})", contacts_a.size()));
    for (size_t i = 0; rings && i < contacts_a.size(); ++i) {
        rings = contacts_a[i].element_a == contacts_b[i].element_a &&
                contacts_a[i].element_b == contacts_b[i].element_b &&
                contacts_a[i].faces.size() == contacts_b[i].faces.size();
        for (size_t k = 0; rings && k < contacts_a[i].faces.size(); ++k) {
            const FaceContact& fa = contacts_a[i].faces[k];
            const FaceContact& fb = contacts_b[i].faces[k];
            rings = fa.face_a == fb.face_a && fa.face_b == fb.face_b && fa.type == fb.type &&
                    fa.area.point_count() == fb.area.point_count();
        }
    }
    check(rings, "every contact: its element pair, and every ring's faces, class and point count");

    const std::vector<WoodJoint> joints_a = a.joints();
    const std::vector<WoodJoint> joints_b = b.joints();
    check(joints_a.size() == joints_b.size(), fmt::format("joint count ({})", joints_a.size()));
    bool joints_ok = joints_a.size() == joints_b.size();
    for (size_t i = 0; joints_ok && i < joints_a.size(); ++i) {
        const WoodJoint& ja = joints_a[i];
        const WoodJoint& jb = joints_b[i];
        joints_ok = ja.element_a == jb.element_a && ja.element_b == jb.element_b
                    && ja.joint_type == jb.joint_type
                    && ja.contact.face_a == jb.contact.face_a && ja.contact.face_b == jb.contact.face_b
                    && ja.contact.area.point_count() == jb.contact.area.point_count()
                    && ja.m_outlines[0].size() == jb.m_outlines[0].size()
                    && ja.m_outlines[1].size() == jb.m_outlines[1].size()
                    && ja.f_outlines[0].size() == jb.f_outlines[0].size()
                    && ja.divisions == jb.divisions && ja.shift == jb.shift
                    && ja.linked_joints == jb.linked_joints
                    && ja.m_cut_types == jb.m_cut_types && ja.f_cut_types == jb.f_cut_types
                    && ja.joint_lines[0].start() == jb.joint_lines[0].start()
                    && ja.joint_lines[1].end() == jb.joint_lines[1].end()
                    && ja.joint_volumes_pair_a_pair_b[0].has_value() == jb.joint_volumes_pair_a_pair_b[0].has_value()
                    && ja.feature_guid(0) == jb.feature_guid(0)
                    && ja.feature_guid(1) == jb.feature_guid(1);
    }
    check(joints_ok, "every joint: elements, type, faces, area, both outline splits, lines, volumes, cut types, links, feature guids");

    bool hosted = true;
    for (const WoodJoint& joint : joints_a)
        hosted = hosted && a.elements.count(joint.element_a) && a.elements.count(joint.element_b);
    check(hosted, "every joint edge resolves to two elements the scene owns");

    size_t attached = 0;
    for (const std::shared_ptr<Element>& element : *a.objects.elements)
        for (const ElementFeature& feature : element->features())
            if (feature.feature_type == "joint") { attached++; }
    check(attached == 2 * joints_a.size(),
          fmt::format("one joint feature per host element ({} of {})", attached, 2 * joints_a.size()));
    bool sides = true;
    for (const WoodJoint& joint : joints_a) {
        const std::vector<ElementFeature> male = a.get_element_features(joint.element_a);
        sides = sides && std::any_of(male.begin(), male.end(), [&joint](const ElementFeature& feature) {
            return feature.guid() == joint.feature_guid(0);
        });
    }
    check(sides, "the graph hands each element the joint side it hosts");

    size_t top_empty = 0, mismatch = 0, ins_mismatch = 0;
    for (size_t i = 0; i < plates_a.size() && i < plates_b.size(); ++i) {
        if (plates_a[i]->features.top.empty())
            top_empty++;
        if (plates_a[i]->features.top.size() != plates_b[i]->features.top.size())
            mismatch++;
        if (plates_a[i]->insertion_vectors.size() != plates_b[i]->insertion_vectors.size())
            ins_mismatch++;
    }
    check(top_empty < plates_a.size() && mismatch == 0 && ins_mismatch == 0,
          "solver results landed on the scene's own plates, and survive the pb");

    const size_t interactions_before = a.get_interactions().size();
    a.get_collisions();
    check(a.get_interactions().size() == interactions_before,
          fmt::format("get_collisions() keeps the interactions on their edges ({})", interactions_before));

    check(WoodInteraction::from_attribute("bvh_collision").empty(), "WoodInteraction rejects bvh_collision");
    check(WoodInteraction::from_attribute("default").empty(), "WoodInteraction rejects default");
    check(WoodInteraction::from_attribute("").empty(), "WoodInteraction rejects an empty attribute");
    check(WoodInteraction::from_attribute("{\"type\":\"something_else\"}").empty(), "WoodInteraction rejects another grammar");

    if (plates_a.empty())
        return failures;

    const size_t before = a.elements.size();
    const std::shared_ptr<WoodElement> plate = plates_a[0];
    const auto probe = std::make_shared<WoodElement>(plate->polylines[0], plate->polylines[1], "probe");
    const std::string probe_guid = probe->element->guid();
    a.add(probe);
    check(a.elements.size() == before + 1 && a.get_element<WoodElement>(probe_guid) == probe
          && a.get_object<Element>(probe_guid) == probe->element,
          "add() puts the same object in the map, the lookup and the session");
    check(a.get_element<WoodColumn>(probe_guid) == nullptr, "get_element rejects the wrong type");
    check(a.remove_object(probe_guid) && a.elements.size() == before && !a.elements.count(probe_guid)
          && a.get_object<Element>(probe_guid) == nullptr,
          "remove_object() takes it out of all three");

    return failures;
}

/*
description: load a session .pb -> contacts and joints onto its graph edges -> pb round trip -> back into a WoodSession; prints only what differs, exit code = failures.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_session_round_trip -j8 && ./build/main_session_round_trip
*/
