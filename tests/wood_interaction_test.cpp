#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

static int failures = 0;

static void check(bool condition, std::string_view label) {

    if (condition)
        return;

    std::cerr << "FAIL " << label << "\n";
    ++failures;
}

static InteractionContact face_contact(int a = 0, int b = 1) {
    return InteractionContact(ContactFace(a, b, ContactType::unknown, Polyline::rectangle(Point(0, 0, 0), Vector::x_axis(), Vector::y_axis(), 100, 100)));
}

static std::shared_ptr<Element> element(WoodSession& session, const std::string& name) {

    const std::shared_ptr<Element> value = std::make_shared<Element>(name);
    session.add_element(value);

    return value;
}

static void add_interaction_test() {

    WoodSession scene("add");
    const std::shared_ptr<Element> a = element(scene, "a");
    const std::shared_ptr<Element> b = element(scene, "b");

    scene.add_edge(a->guid(), b->guid(), "authored");
    Interaction& stored = scene.add_interaction(a, b);

    check(&scene.add_interaction(b, a) == &stored && scene.interactions.size() == 1, "Either order makes one record");
    check(scene.graph.edges.at(a->guid()).at(b->guid()).attribute == "authored", "Existing edge attributes retained");
    check(scene.graph.edges.at(b->guid()).at(a->guid()).guid() == stored.guid, "Both edge copies share identity");
    check(&stored.session() == &scene, "Record bound to session");
}

static void add_interaction_record_test() {

    WoodSession scene("record");
    const std::shared_ptr<Element> a = element(scene, "a");
    const std::shared_ptr<Element> b = element(scene, "b");
    const std::shared_ptr<Element> c = element(scene, "c");
    Interaction& stored = scene.add_interaction(a, b);

    FeaturePlate plate;
    plate.joint_type = 11;
    FeatureBeam beam;
    for (int i = 0; i < 4; ++i)
        beam.volumes[i] = Polyline({Point(i, 0, 0), Point(i, 1, 0)});

    Interaction record; // Relative to (b, a).
    record.contacts = {face_contact(1, 0), face_contact(1, 0)};
    record.features = {InteractionFeature(plate), InteractionFeature(beam)};
    record.features[0].contact = 1;
    record.structure = InteractionStructure{};
    scene.add_interaction(b, a, record);

    check(stored.contacts.size() == 1 && stored.contacts[0].face()->face_a == 0 && stored.features[0].contact == 0, "Contacts oriented to the edge, merged and remapped");
    check(stored.features[0].plate()->element_a == b->guid() && stored.features[0].plate()->contact.face_a == 1, "Plate feature oriented to its host");
    check(stored.features[1].beam()->volumes[0].get_point(0) == Point(2, 0, 0), "Beam volumes follow the edge");
    check(stored.structure && scene.consistent() && a->features_count() == 3 && b->features_count() == 1, "Structure stored, features hosted");

    Interaction bad = record;
    bad.features[0].contact = 10;
    bool rejected = false;
    try {
        scene.add_interaction(a, c, bad);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && !scene.graph.has_edge({a->guid(), c->guid()}), "Invalid record rejected before any edge");
}

static void has_interaction_test() {

    WoodSession scene("has");
    const std::shared_ptr<Element> a = element(scene, "a");
    const std::shared_ptr<Element> b = element(scene, "b");
    const std::shared_ptr<Element> c = element(scene, "c");

    scene.add_edge(a->guid(), b->guid(), "authored");
    check(scene.has_interaction(b, a) && !scene.has_interaction(a, c), "Bare edge found in either order");
}

static void remove_interaction_test() {

    WoodSession scene("remove");
    const std::shared_ptr<Element> a = element(scene, "a");
    const std::shared_ptr<Element> b = element(scene, "b");
    const std::shared_ptr<Element> c = element(scene, "c");

    Interaction record;
    record.contacts = {face_contact()};
    record.features = {InteractionFeature(FeaturePlate())};
    record.features[0].contact = 0;
    const std::string id = scene.add_interaction(a, b, record).guid;
    record.features.clear();
    scene.add_interaction(a, c, record);
    a->add_feature(ElementFeature("custom", -1, {}, "keep"));

    scene.remove_interaction(b, a);
    scene.remove_interaction(b, a);
    check(!scene.has_interaction(a, b) && !scene.interactions.count(id), "Removal clears edge and record");
    check(a->features_count() == 2 && b->features_count() == 0 && scene.has_interaction(a, c), "Removal keeps other pair and custom features");
    check(!WoodSession::pb_loads(scene.pb_dumps()).has_interaction(a, b), "Removed interaction stays absent after round trip");
}

static void get_interaction_test() {

    WoodSession scene("get");
    const std::shared_ptr<Element> a = element(scene, "a");
    const std::shared_ptr<Element> b = element(scene, "b");

    scene.add_edge(a->guid(), b->guid(), "authored");
    check(!scene.get_interaction(a, b), "Bare edge holds no record");

    Interaction record;
    record.contacts = {face_contact()};
    Interaction& stored = scene.add_interaction(a, b, record);
    const WoodSession& view = scene;
    check(scene.get_interaction(b, a) == &stored && view.get_interaction(a, b) == &stored, "Either order, const or not");

    const WoodSession loaded = WoodSession::pb_loads(scene.pb_dumps());
    const Interaction* copy = loaded.get_interaction(a, b);
    check(copy && copy->guid == stored.guid && &copy->contacts[0].session() == &loaded, "Loaded record found and bound");

    WoodSession copied = scene;
    check(&copied.get_interaction(a, b)->contacts[0].session() == &copied, "Copied record rebound");
}

int main() {

    add_interaction_test();
    add_interaction_record_test();
    has_interaction_test();
    remove_interaction_test();
    get_interaction_test();

    return failures ? 1 : 0;
}
