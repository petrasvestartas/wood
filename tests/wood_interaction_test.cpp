#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

static int failures = 0;
static void check(bool condition, std::string_view label) {
    if (!condition) {
        std::cerr << "FAIL " << label << "\n";
        ++failures;
    }
}

template <class F>
static void rejects(F action, std::string_view label) {
    bool rejected = false;
    try { action(); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, label);
}

static InteractionContact face_contact(int a = 0, int b = 1) {
    return InteractionContact(ContactFace(a, b, ContactType::unknown,
        Polyline::rectangle(Point(0, 0, 0), Vector::x_axis(), Vector::y_axis(), 100, 100)));
}

static std::shared_ptr<Element> element(WoodSession& session, const std::string& name) {
    auto value = std::make_shared<Element>(name);
    session.add_element(value);
    return value;
}

static void manual_test() {
    WoodSession scene("manual");
    auto a = element(scene, "a");
    auto b = element(scene, "b");
    auto c = element(scene, "c");
    auto absent = std::make_shared<Element>("absent");
    rejects([&] { scene.add_interaction(a, absent); }, "Reject unregistered endpoint");
    rejects([&] { scene.add_interaction(a, a); }, "Reject self interaction");
    rejects([&] { scene.add_interaction(a, std::shared_ptr<Element>{}); }, "Reject null endpoint");
    check(scene.interactions.empty() && scene.graph.number_of_edges() == 0, "Invalid endpoints do not mutate scene");

    scene.add_edge(a->guid(), b->guid(), "authored");
    check(scene.has_interaction(a, b) && !scene.get_interaction(*a, *b), "Bare relation can acquire a wood record");
    Interaction& stored = scene.add_interaction(a, b, face_contact());
    const std::string id = stored.guid;
    check(scene.has_interaction(b, a) && &scene.add_interaction(*b, *a) == &stored, "Symmetric interaction lookup");
    check(scene.graph.edges.at(a->guid()).at(b->guid()).attribute == "authored", "Existing edge attributes retained");
    check(scene.graph.edges.at(b->guid()).at(a->guid()).guid() == id, "Both edge copies share identity");
    scene.add_interaction(b, a, face_contact(1, 0));
    check(stored.contacts.size() == 1 && a->features_count() == 1, "Reversed coincident contact reused");

    FeatureBeam beam;
    for (int i = 0; i < 4; ++i)
        beam.volumes[i] = Polyline({Point(i, 0, 0), Point(i, 1, 0)});
    InteractionFeature feature(beam);
    feature.contact = 0;
    scene.add_interaction(b, a, feature);
    scene.add_interaction(a, b, InteractionStructure{});
    check(stored.features.size() == 1 && stored.structure.has_value(), "Feature and structure stored on pair");
    check(stored.features[0].beam()->volumes[0].get_point(0) == Point(2, 0, 0), "Reversed beam volumes follow edge");
    check(&stored.session() == &scene && &stored.contacts[0].session() == &scene
          && &stored.features[0].session() == &scene, "Records bound to session");
    check(a->features_count() == 2 && scene.consistent(), "Manual features hosted and consistent");

    scene.add_interaction(a, c, face_contact(2, 3));
    ElementFeature unrelated("custom", -1, {}, "keep");
    const std::string unrelated_id = unrelated.guid();
    a->add_feature(std::move(unrelated));
    WoodSession loaded = WoodSession::pb_loads(scene.pb_dumps());
    const Interaction* copy = loaded.get_interaction(a->guid(), b->guid());
    check(copy && copy->guid == id && copy->contacts.size() == 1 && copy->features.size() == 1 && copy->structure,
          "Protobuf preserves all interaction payloads");
    check(copy && &copy->session() == &loaded && &copy->features[0].session() == &loaded, "Loaded records bound to loaded session");
    loaded.remove_interaction(*b, *a);
    check(!loaded.has_interaction(*a, *b) && loaded.get_element<Element>(a->guid())->features_count() == 2,
          "Loaded interaction removes its hosted features by identity");
    WoodSession copied = scene;
    check(&copied.get_interaction(a->guid(), b->guid())->contacts[0].session() == &copied, "Copied records rebound");

    scene.remove_interaction(b, a);
    check(!scene.has_interaction(a, b) && !scene.graph.has_edge({a->guid(), b->guid()})
          && !scene.graph.has_edge({b->guid(), a->guid()}) && !scene.interactions.count(id), "Removal clears edge and record");
    check(a->features_count() == 2 && scene.has_interaction(a, c), "Removal preserves other pair and custom features");
    bool kept = false;
    for (const auto& f : a->features()) kept |= f.guid() == unrelated_id;
    check(kept, "Unrelated feature identity retained");
    scene.remove_interaction(a, b);
    const WoodSession removed = WoodSession::pb_loads(scene.pb_dumps());
    check(!removed.has_interaction(*a, *b), "Removed interaction stays absent after round trip");
}

static void payload_test() {
    WoodSession scene("payload");
    auto a = element(scene, "a");
    auto b = element(scene, "b");
    auto c = element(scene, "c");
    scene.add_interaction(a, b, face_contact(4, 5));
    Interaction payload;
    payload.contacts = {face_contact(1, 0), face_contact(1, 0)}; // relative to (b, a)
    InteractionFeature feature{FeaturePlateBeam{}};
    feature.contact = 0;
    payload.features.push_back(feature);
    payload.structure = InteractionStructure{};
    Interaction& stored = scene.add_interaction(b, a, payload);
    check(stored.contacts.size() == 2 && stored.features[0].contact == 1 && stored.structure,
          "Bundle merges contacts and remaps duplicate indices");
    check(stored.contacts[1].face()->face_a == 0 && scene.consistent(), "Bundle contacts follow stored edge orientation");

    Interaction bad = payload;
    bad.features[0].contact = 10;
    rejects([&] { scene.add_interaction(a, c, bad); }, "Reject invalid bundle contact index");
    check(!scene.has_interaction(a, c) && !scene.graph.has_edge({a->guid(), c->guid()}), "Invalid bundle creates no edge");
    rejects([&] { scene.add_interaction(a, b, bad.features[0]); }, "Reject invalid existing contact index");
    check(stored.contacts.size() == 2 && stored.features.size() == 1, "Invalid feature preserves existing payload");

    FeaturePlate joint;
    joint.element_a = a->guid();
    joint.element_b = c->guid();
    rejects([&] { scene.add_interaction(a, b, InteractionFeature(joint)); }, "Reject feature for another pair");

    joint.element_a.clear();
    joint.element_b.clear();
    joint.joint_type = 30;
    InteractionFeature crossing(joint);
    crossing.contact = 0;
    rejects([&] { scene.add_interaction(a, b, crossing); }, "Reject cross joint linked to a face contact");

    InteractionFeature standalone{FeaturePlateBeam{}};
    scene.add_interaction(a, c, standalone);
    check(scene.consistent() && scene.get_interaction(*a, *c)->features[0].contact == -1, "Manual feature can omit contact");
}

static void plate_feature_test() {
    WoodSession scene("plate feature");
    auto a = element(scene, "a");
    auto b = element(scene, "b");
    scene.add_interaction(a, b, face_contact(2, 3));
    FeaturePlate plate;
    plate.joint_type = 11;
    plate.name = "manual_plate_joint";
    InteractionFeature feature(plate);
    feature.contact = 0;
    Interaction& interaction = scene.add_interaction(b, a, feature);
    const FeaturePlate& stored = *interaction.features[0].plate();
    check(stored.element_a == b->guid() && stored.element_b == a->guid()
          && stored.contact.face_a == 3 && stored.contact.face_b == 2, "Manual plate pair and contact oriented to host");
    check(scene.consistent() && a->features_count() == 2 && b->features_count() == 1, "Plate feature sides hosted");
    scene.remove_interaction(a, b);
    check(a->features_count() == 0 && b->features_count() == 0, "Removal clears both plate feature sides");
}

static void instance_test() {
    WoodSession scene("instance interaction");
    const auto definition = std::make_shared<Beam>(Polyline({Point(0, 0, 0), Point(100, 0, 0)}), 10.0);
    scene.add_instance(scene.add_definition(definition, "beam"), Xform::translation(10, 20, 0));
    const auto instance = scene.objects.instances->front();
    auto other = element(scene, "other");
    Interaction& interaction = scene.add_interaction(instance->guid(), other->guid(), face_contact());
    check(instance->features.size() == 1 && instance->features[0].guid() == interaction.contacts[0].guid,
          "Contact hosted on instance");
    check(instance->features[0].outlines[0].get_point(0) == Point(-10, -20, 0), "World contact transformed into instance frame");
    scene.remove_interaction(other->guid(), instance->guid());
    check(instance->features.empty() && scene.interactions.empty(), "Removal cleans instance features");
}

int main() {
    manual_test();
    payload_test();
    plate_feature_test();
    instance_test();
    return failures ? 1 : 0;
}
