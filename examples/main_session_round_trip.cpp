#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const int SESSION = 0;   // config::SESSION_NAMES

static int failures = 0;
static void check(const bool ok, const std::string& what) {

    if (ok)
        return;

    std::cout << fmt::format("FAIL {}\n", what);
    failures++;
}

static size_t tree_nodes(const Session& session) {
    return session.tree.root() ? session.tree.root()->descendants().size() + 1 : 0;
}

/// Whether one of features is the male side of joint.
static bool hosts_male_side(const std::vector<ElementFeature>& features, const FeaturePlate& joint) {

    for (const ElementFeature& feature : features)
        if (feature.guid() == joint.feature_guid(0))
            return true;

    return false;
}

/// Bytes as hex text, the way Element::jsondump writes element_data.
static std::string to_hex(const std::string& bytes) {

    static const char* digits = "0123456789abcdef";
    std::string out;
    for (const unsigned char c : bytes) {
        out.push_back(digits[c >> 4]);
        out.push_back(digits[c & 15]);
    }

    return out;
}

/// Same kind, same faces or segments, same polygon size.
static bool same_contact(const InteractionContact& x, const InteractionContact& y) {

    if (x.guid != y.guid || x.kind() != y.kind() || !x.coincides(y))
        return false;
    if (x.face())
        return x.face()->polygon.point_count() == y.face()->polygon.point_count();
    if (x.cross())
        return x.cross()->polygon.point_count() == y.cross()->polygon.point_count();

    return x.axis()->segment.length() == y.axis()->segment.length();
}

int main() {

    config::reset_defaults();
    WoodSession a = WoodSession::pb_load(config::SESSION_NAMES[SESSION]);
    const int vertices_before = a.graph.number_of_vertices();
    a.compute_contacts();
    a.compute_joints();

    a.sync_joint_features();
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "wood_session_round_trip.pb";
    a.pb_dump(path.string());
    const WoodSession b = WoodSession::pb_load(path);
    const Session kernel = Session::pb_load(path.string());
    std::filesystem::remove(path);

    check(a.name == b.name, "session name");
    check(a.guid() == b.guid(), "session guid");
    check(a.objects.elements->size() == b.objects.elements->size(), fmt::format("element count ({})", a.objects.elements->size()));
    const std::vector<std::shared_ptr<Plate>> plates_a = a.plates();
    const std::vector<std::shared_ptr<Plate>> plates_b = b.plates();
    check(plates_a.size() == plates_b.size(), fmt::format("plate count ({})", plates_a.size()));
    check(a.columns().size() == b.columns().size(), "column count");
    check(a.blocks().size() == b.blocks().size(), "block count");

    bool typed = true;
    for (const std::shared_ptr<Element>& element : *a.objects.elements)
        typed = typed && a.get_element<Element>(element->guid()) == element
                && (element->element_type_name() != Plate::ELEMENT_TYPE || std::dynamic_pointer_cast<Plate>(element));

    check(typed, "every element is held once, and every \"Plate\" is a Plate object");
    check(a.element_guids() == b.element_guids(), "every element guid, in order");

    check(tree_nodes(a) == tree_nodes(b), fmt::format("tree node count ({})", tree_nodes(a)));
    check(a.objects.polylines->size() == b.objects.polylines->size(), "loose polyline count");
    check(a.objects.meshes->size() == b.objects.meshes->size(), "loose mesh count");
    check(a.graph.number_of_vertices() == b.graph.number_of_vertices(), "graph vertex count");
    check(a.graph.number_of_vertices() == vertices_before, "contacts and joints are interaction records, not graph nodes");
    check(a.graph.number_of_edges() == b.graph.number_of_edges(), fmt::format("edge count ({})", a.graph.number_of_edges()));

    check(kernel.objects.elements->size() == a.objects.elements->size() && kernel.graph.number_of_edges() == a.graph.number_of_edges() && tree_nodes(kernel) == tree_nodes(a),
          "the kernel's Session reader opens the wood file: same elements, edges and tree");

    check(a.interactions.size() == b.interactions.size(), fmt::format("interaction count ({})", a.interactions.size()));
    bool records = true;
    size_t contact_count = 0;
    size_t feature_count = 0;
    for (const auto& [guid, ia] : a.interactions) {

        const auto found = b.interactions.find(guid);
        if (found == b.interactions.end()) {
            records = false;
            break;
        }

        const Interaction& ib = found->second;
        records = records && a.edge_of(ia) == b.edge_of(ib) && !a.edge_of(ia).first.empty()
                  && a.graph.edges.at(a.edge_of(ia).first).at(a.edge_of(ia).second).guid() == guid
                  && ia.contacts.size() == ib.contacts.size() && ia.features.size() == ib.features.size();
        for (size_t k = 0; records && k < ia.contacts.size(); ++k) {
            records = same_contact(ia.contacts[k], ib.contacts[k]);
            contact_count++;
        }
        for (size_t k = 0; records && k < ia.features.size(); ++k) {
            const InteractionFeature& fa = ia.features[k];
            const InteractionFeature& fb = ib.features[k];
            records = fa.guid == fb.guid && fa.contact == fb.contact && fa.kind() == fb.kind()
                      && fa.plate()->feature_guids == fb.plate()->feature_guids && fa.contact >= 0 && fa.contact < (int)ia.contacts.size()
                      && fa.plate()->name == fb.plate()->name && fa.plate()->joint_type == fb.plate()->joint_type
                      && fa.plate()->element_a == fb.plate()->element_a && fa.plate()->contact.face_a == fb.plate()->contact.face_a;
            feature_count++;
        }
    }

    check(records, fmt::format("every interaction by its edge guid, its edge, its contacts ({}) and its features ({}) survive the pb", contact_count, feature_count));

    bool encoded = true;
    for (const auto& [guid, interaction] : a.interactions) {
        const Interaction json = Interaction::jsonload(interaction.jsondump());
        const Interaction pb = Interaction::pb_loads(interaction.pb_dumps());
        encoded = encoded && json.guid == guid && pb.guid == guid && json.contacts.size() == interaction.contacts.size()
                  && pb.contacts.size() == interaction.contacts.size() && json.features.size() == interaction.features.size()
                  && pb.features.size() == interaction.features.size();
    }

    check(encoded, "Interaction round-trips through its own JSON and protobuf");

    const std::vector<FeaturePlate> joints_a = a.get_joints();
    const std::vector<FeaturePlate> joints_b = b.get_joints();
    check(joints_a.size() == joints_b.size(), fmt::format("joint count ({})", joints_a.size()));
    bool joints_ok = joints_a.size() == joints_b.size();

    for (size_t i = 0; joints_ok && i < joints_a.size(); ++i) {
        const FeaturePlate& ja = joints_a[i];
        const FeaturePlate& jb = joints_b[i];
        joints_ok = ja.element_a == jb.element_a && ja.element_b == jb.element_b
                    && ja.joint_type == jb.joint_type
                    && ja.contact.face_a == jb.contact.face_a && ja.contact.face_b == jb.contact.face_b
                    && ja.contact.polygon.point_count() == jb.contact.polygon.point_count()
                    && ja.male_outlines[0].size() == jb.male_outlines[0].size()
                    && ja.male_outlines[1].size() == jb.male_outlines[1].size()
                    && ja.female_outlines[0].size() == jb.female_outlines[0].size()
                    && ja.divisions == jb.divisions && ja.shift == jb.shift
                    && ja.linked_joints == jb.linked_joints
                    && ja.male_fabrication_types == jb.male_fabrication_types && ja.female_fabrication_types == jb.female_fabrication_types
                    && ja.joint_lines[0].start() == jb.joint_lines[0].start()
                    && ja.joint_lines[1].end() == jb.joint_lines[1].end()
                    && ja.joint_volumes[0].has_value() == jb.joint_volumes[0].has_value()
                    && ja.feature_guid(0) == jb.feature_guid(0)
                    && ja.feature_guid(1) == jb.feature_guid(1);
    }

    check(joints_ok, "every joint: elements, type, faces, polygon, both outline splits, lines, volumes, cut types, links, feature guids");

    bool hosted = true;
    for (const FeaturePlate& joint : joints_a)
        hosted = hosted && a.get_element<Element>(joint.element_a) && a.get_element<Element>(joint.element_b);

    check(hosted, "every joint's edge resolves to two elements the scene owns");

    size_t attached = 0;
    for (const std::shared_ptr<Element>& element : *a.objects.elements)
        for (const ElementFeature& feature : element->features())
            if (feature.feature_type == "joint") { attached++; }

    check(attached == 2 * joints_a.size(),
          fmt::format("one joint feature per host element ({} of {})", attached, 2 * joints_a.size()));

    bool sides = true;
    for (const FeaturePlate& joint : joints_a) {
        const std::vector<ElementFeature> male = a.get_element_features(joint.element_a);
        sides = sides && hosts_male_side(male, joint);
    }

    check(sides, "the interactions hand each element the joint side it hosts");

    size_t top_empty = 0, mismatch = 0, ins_mismatch = 0;
    for (size_t i = 0; i < plates_a.size() && i < plates_b.size(); ++i) {
        if (plates_a[i]->features.top.empty())
            top_empty++;
        if (plates_a[i]->features.top.size() != plates_b[i]->features.top.size())
            mismatch++;
        if (plates_a[i]->insertion_vectors().size() != plates_b[i]->insertion_vectors().size())
            ins_mismatch++;
    }

    check(top_empty < plates_a.size() && mismatch == 0 && ins_mismatch == 0,
          "solver results landed on the scene's own plates, and survive the pb");

    const size_t interactions_before = a.interactions.size();
    a.get_collisions();
    check(a.interactions.size() == interactions_before,
          fmt::format("get_collisions() keeps the interactions on their edges ({})", interactions_before));

    if (plates_a.empty())
        return failures;

    const std::shared_ptr<Plate> plate = plates_a[0];
    const std::shared_ptr<Plate> proto = std::dynamic_pointer_cast<Plate>(Element::pb_loads_polymorphic(plate->pb_dumps()));
    check(proto && proto->polylines.size() == plate->polylines.size() && proto->polylines[0].point_count() == plate->polylines[0].point_count()
          && plate->element_data_dumps().front() != '{',
          "a Plate's element_data is protobuf and rebuilds the outlines");

    nlohmann::ordered_json legacy = plate->jsondump();
    legacy["element_data"] = to_hex(plate->element_data_jsondump().dump());
    const std::shared_ptr<Plate> json = Plate::from_element(Element::jsonload(legacy));
    check(json && json->polylines.size() == plate->polylines.size() && json->polylines[1].point_count() == plate->polylines[1].point_count(),
          "a Plate written with the JSON payload still loads");

    const size_t before = a.objects.elements->size();
    const std::shared_ptr<Plate> probe = std::make_shared<Plate>(plate->polylines[0], plate->polylines[1], "probe");
    const std::string probe_guid = probe->guid();

    a.add(probe);
    check(a.objects.elements->size() == before + 1 && a.get_element<Plate>(probe_guid) == probe
          && a.get_object<Element>(probe_guid) == probe,
          "add() puts the same object in the lookup and the session");
    check(a.get_element<Column>(probe_guid) == nullptr, "get_element rejects the wrong type");
    check(a.remove_object(probe_guid) && a.objects.elements->size() == before
          && a.get_object<Element>(probe_guid) == nullptr,
          "remove_object() takes it out of both");

    return failures;
}

/*
description: load a session .pb -> contacts and joints into the interaction store -> pb round trip -> back into a WoodSession; prints only what differs, exit code = failures.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_session_round_trip -j4 && ./build/main_session_round_trip
*/
