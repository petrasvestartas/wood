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

int main() {
    wood_session::WoodSession a =
        wood_session::WoodSession::from_session(session_cpp::Session::pb_load(DATASET));
    a.add_contacts(wood_session::face_contacts(wood_session::contact_view(a)));

    const session_cpp::Session written = a.to_session("round_trip");
    const wood_session::WoodSession b =
        wood_session::WoodSession::from_session(session_cpp::Session::pb_loads(written.pb_dumps()));

    fmt::print("{} contacts, {} edges\n", a.contacts.size(), a.graph.number_of_edges());

    check(a.plates.size() == b.plates.size(), "plate count");
    check(a.columns.size() == b.columns.size(), "column count");
    check(a.solids.size() == b.solids.size(), "solid count - contacts did not leak in as solids");
    check(a.contacts.size() == b.contacts.size(), "contact count");
    check(a.graph.number_of_edges() == b.graph.number_of_edges(), "edge count");

    bool guids = a.plates.size() == b.plates.size();
    for (size_t i = 0; guids && i < a.plates.size(); ++i)
        guids = a.plates[i].element.guid() == b.plates[i].element.guid();
    check(guids, "plate guids");

    bool rings = a.contacts.size() == b.contacts.size();
    for (size_t i = 0; rings && i < a.contacts.size(); ++i) {
        rings = a.contacts[i].faces.size() == b.contacts[i].faces.size();
        for (size_t k = 0; rings && k < a.contacts[i].faces.size(); ++k) {
            const wood_session::FaceContact& fa = a.contacts[i].faces[k];
            const wood_session::FaceContact& fb = b.contacts[i].faces[k];
            rings = fa.face_a == fb.face_a && fa.face_b == fb.face_b && fa.type == fb.type
                    && fa.area.point_count() == fb.area.point_count();
        }
    }
    check(rings, "every ring: face pair, class, point count");

    check(a.contact_pairs() == b.contact_pairs(), "every edge resolves to the same element pair");

    const wood_session::EdgeLink l = wood_session::EdgeLink::from_attribute("c7j3");
    check(l.contact == 7 && l.joint == 3, "EdgeLink parses c7j3");
    check(wood_session::EdgeLink{7, -1}.to_attribute() == "c7", "EdgeLink writes c7");
    check(wood_session::EdgeLink::from_attribute("bvh_collision").contact == -1, "EdgeLink rejects bvh_collision");
    check(wood_session::EdgeLink::from_attribute("").contact == -1, "EdgeLink rejects an empty attribute");
    check(wood_session::EdgeLink::from_attribute("7").contact == -1, "EdgeLink rejects a bare number");

    fmt::print("\n{} failed\n", failures);
    return failures;
}
