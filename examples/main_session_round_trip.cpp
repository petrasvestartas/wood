#include "wood_session.h"
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
    const wood_session::WoodSession a = wood_session::WoodSession::load(DATASET);
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

    fmt::print("\n{} failed\n", failures);
    return failures;
}
