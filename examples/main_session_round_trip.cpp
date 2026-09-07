#include "wood_session.h"
#include "wood_face_to_face.h"
#include "../src/session.h"

#include <fmt/core.h>

const char* DATASET = "data/floor_model.pb";

static int failures = 0;
static void check(bool ok, const std::string& what) {
    fmt::print("  [{}] {}\n", ok ? "PASS" : "FAIL", what);
    if (!ok) failures++;
}

static size_t tree_nodes(const session_cpp::Session& s) {
    return s.tree.root() ? s.tree.root()->descendants().size() + 1 : 0;
}

int main() {
    wood_session::WoodSession a = wood_session::WoodSession::load(DATASET);
    a.add_contacts(wood_session::face_contacts(wood_session::contact_view(a)));
    // Wrapper copies share the kernel element, so the solver's guids name the scene's plates.
    std::vector<wood_session::WoodElement> solver_plates;
    for (const wood_session::WoodElement* p : a.plates()) { solver_plates.push_back(*p); }
    wood_session::globals::reset_defaults();
    a.add_joints(get_connection_zones(solver_plates, face_to_face));
    const session_cpp::Session& sa = *a.to_session();
    const wood_session::WoodSession b = wood_session::WoodSession::from_session(
        std::make_shared<session_cpp::Session>(session_cpp::Session::pb_loads(sa.pb_dumps())));
    const session_cpp::Session& sb = *b.session;

    fmt::print("{}\n", a.str());

    check(a.name() == b.name(), "session name");
    check(a.guid() == b.guid(), "session guid");
    check(a.objects.size() == b.objects.size(), "object count");
    check(a.plates().size() == b.plates().size(), "plate count");
    check(a.columns().size() == b.columns().size(), "column count");
    check(a.solids().size() == b.solids().size(), "solid count");

    bool guids = a.objects.size() == b.objects.size();
    for (size_t i = 0; guids && i < a.objects.size(); ++i)
        guids = a.lookup.count(b.lookup.begin()->first) > 0 &&
                std::visit([](const auto& o) { return o->element->guid(); }, a.objects[i]) ==
                std::visit([](const auto& o) { return o->element->guid(); }, b.objects[i]);
    check(guids, "every object guid, in order");

    check(tree_nodes(sa) == tree_nodes(sb), fmt::format("tree node count ({})", tree_nodes(sa)));
    check(sa.objects.polylines->size() == sb.objects.polylines->size(), "loose polyline count");
    check(sa.objects.meshes->size() == sb.objects.meshes->size(), "loose mesh count");
    check(sa.graph.number_of_vertices() == sb.graph.number_of_vertices(), "graph vertex count");

    check(a.contacts().size() == b.contacts().size(),
          fmt::format("contact count ({})", a.contacts().size()));
    check(sa.graph.number_of_edges() == sb.graph.number_of_edges(),
          fmt::format("edge count ({})", sa.graph.number_of_edges()));
    check(a.contacts().size() == (size_t)sa.graph.number_of_edges(), "one edge per contact, 1:1");

    const std::vector<wood_session::WoodContact*> ca = a.contacts();
    const std::vector<wood_session::WoodContact*> cb = b.contacts();
    bool rings = ca.size() == cb.size();
    for (size_t i = 0; rings && i < ca.size(); ++i) {
        rings = ca[i]->element->guid() == cb[i]->element->guid() && ca[i]->faces.size() == cb[i]->faces.size();
        for (size_t k = 0; rings && k < ca[i]->faces.size(); ++k) {
            const wood_session::FaceContact& fa = ca[i]->faces[k];
            const wood_session::FaceContact& fb = cb[i]->faces[k];
            rings = fa.face_a == fb.face_a && fa.face_b == fb.face_b && fa.type == fb.type
                    && fa.area.point_count() == fb.area.point_count();
        }
    }
    check(rings, "every contact: guid, and every ring's face pair, class and point count");
    check(a.contact_pairs() == b.contact_pairs(), "every edge resolves to the same element guids");
    bool resolved = true;
    for (const auto& [u, v] : b.contact_pairs()) resolved = resolved && b.lookup.count(u) && b.lookup.count(v);
    check(resolved, "every edge endpoint is an object the scene owns");

    check(a.joints().size() == b.joints().size(), fmt::format("joint count ({})", a.joints().size()));
    check(a.joint_pairs() == b.joint_pairs(), "every joint edge resolves to the same element guids");
    bool joints_ok = a.joints().size() == b.joints().size();
    for (size_t i = 0; joints_ok && i < a.joints().size(); ++i) {
        const wood_session::WoodJoint& ja = *a.joints()[i];
        const wood_session::WoodJoint& jb = *b.joints()[i];
        joints_ok = ja.element->guid() == jb.element->guid() && ja.element_a == jb.element_a
                    && ja.element_b == jb.element_b && ja.joint_type == jb.joint_type
                    && ja.contact.face_a == jb.contact.face_a && ja.contact.face_b == jb.contact.face_b
                    && ja.contact.area.point_count() == jb.contact.area.point_count()
                    && ja.m_outlines[0].size() == jb.m_outlines[0].size()
                    && ja.f_outlines[0].size() == jb.f_outlines[0].size()
                    && ja.divisions == jb.divisions && ja.shift == jb.shift
                    && ja.linked_joints == jb.linked_joints;
    }
    check(joints_ok, "every joint: guid, elements, type, faces, area, outlines, divisions, links");
    size_t on_contact = 0, alone = 0;
    for (const auto& [u, v] : a.joint_pairs()) {
        const auto it = sa.graph.edges.find(u);
        const wood_session::EdgeLink l = wood_session::EdgeLink::from_attribute(it->second.at(v).attribute);
        (l.contact.empty() ? alone : on_contact)++;
    }
    fmt::print("  joints on a contact edge: {}, joints on a pair with no contact: {}\n", on_contact, alone);

    const wood_session::EdgeLink l = wood_session::EdgeLink::from_attribute("cabc-1j0f-2");
    check(l.contact == "abc-1" && l.joint == "0f-2", "EdgeLink parses c<guid>j<guid>");
    check(wood_session::EdgeLink{"abc", ""}.to_attribute() == "cabc", "EdgeLink writes c<guid>");
    check(wood_session::EdgeLink::from_attribute("bvh_collision").contact.empty(), "EdgeLink rejects bvh_collision");
    check(wood_session::EdgeLink::from_attribute("default").contact.empty(), "EdgeLink rejects default");
    check(wood_session::EdgeLink::from_attribute("").contact.empty(), "EdgeLink rejects an empty attribute");
    check(wood_session::EdgeLink::from_attribute("cj0f-2").joint == "0f-2", "EdgeLink parses a joint-only edge cj<guid>");

    fmt::print("\n{} failed\n", failures);
    return failures;
}
