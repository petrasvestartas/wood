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

static std::shared_ptr<InteractionContactFace> face_contact(int a = 0, int b = 1) {
    return std::make_shared<InteractionContactFace>(a, b, ContactType::unknown, Polyline::rectangle(Point(0, 0, 0), Vector::x_axis(), Vector::y_axis(), 100, 100));
}

static std::shared_ptr<InteractionFeatureBeam> beam_feature() {

    std::shared_ptr<InteractionFeatureBeam> beam = std::make_shared<InteractionFeatureBeam>();

    for (int i = 0; i < 4; ++i)
        beam->volumes[i] = Polyline({Point(i, 0, 0), Point(i, 1, 0)});

    return beam;
}

static std::shared_ptr<Element> element(WoodSession& session, const std::string& name) {

    const std::shared_ptr<Element> value = std::make_shared<Element>(name);
    session.add_element(value);

    return value;
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Interactions
// ═══════════════════════════════════════════════════════════════════════════

static void add_interaction_test() {

    WoodSession scene("add");
    const std::shared_ptr<Element> a = element(scene, "a");
    const std::shared_ptr<Element> b = element(scene, "b");
    scene.add_edge(a->guid(), b->guid(), "authored");

    std::shared_ptr<InteractionContactCross> cross = std::make_shared<InteractionContactCross>();
    cross->faces_a = {2, 3};
    cross->faces_b = {4, 5};

    std::shared_ptr<InteractionContactFace> glue = face_contact();
    glue->name = "glue";

    scene.add_interaction(a, b, glue);
    scene.add_interaction(a, b, std::make_shared<InteractionContactAxis>(Line::from_points(Point(0, 0, 0), Point(1, 0, 0)), 0.5, 0.5, 0, 1, 0, 2));
    scene.add_interaction(a, b, cross);
    scene.add_interaction(a, b, std::make_shared<InteractionFeaturePlate>());
    scene.add_interaction(a, b, beam_feature());
    scene.add_interaction(a, b, std::make_shared<InteractionFeaturePlateBeam>());
    const std::vector<std::shared_ptr<Interaction>> list = scene.get_interaction(b, a);
    const std::string id = scene.graph.edges.at(a->guid()).at(b->guid()).guid();

    check(list.size() == 6 && scene.interactions.at(id).size() == 6, "Every leaf type stored on the one edge");
    check(list[0]->name == "glue" && list[0]->interaction_type_name() == "InteractionContactFace", "A named contact keeps its name");
    check(dynamic_cast<InteractionContactFace*>(list[0].get()) && dynamic_cast<InteractionContactAxis*>(list[1].get()) && dynamic_cast<InteractionContactCross*>(list[2].get()), "Contacts keep their types");
    check(dynamic_cast<InteractionFeaturePlate*>(list[3].get()) && dynamic_cast<InteractionFeatureBeam*>(list[4].get()) && dynamic_cast<InteractionFeaturePlateBeam*>(list[5].get()), "Features keep their types");
    check(std::is_abstract_v<InteractionContact> && std::is_abstract_v<InteractionFeature> && std::is_abstract_v<InteractionStructure>, "Only the leaves are concrete");
    check(a->features_count() == 5 && b->features_count() == 1, "Three contacts, a plate side and a beam joint on a, a plate side on b");
    check(scene.graph.edges.at(a->guid()).at(b->guid()).attribute == "authored", "Existing edge attributes retained");
    check(scene.graph.edges.at(b->guid()).at(a->guid()).guid() == id, "Both edge copies share identity");
}

static void either_order_test() {

    WoodSession scene("order");
    const std::shared_ptr<Element> a = element(scene, "a");
    const std::shared_ptr<Element> b = element(scene, "b");
    scene.add_edge(a->guid(), b->guid(), "authored");

    const std::shared_ptr<Interaction> face = scene.add_interaction(b, a, face_contact(1, 0));
    const std::shared_ptr<Interaction> again = scene.add_interaction(a, b, face_contact(0, 1));
    const std::shared_ptr<Interaction> beam = scene.add_interaction(b, a, beam_feature());

    std::shared_ptr<InteractionFeaturePlate> plate = std::make_shared<InteractionFeaturePlate>();
    plate->contact = *face_contact(1, 0);
    plate->contact_guid = face->guid();
    scene.add_interaction(b, a, plate);

    check(dynamic_cast<InteractionContactFace&>(*face).face_a == 0, "Contact oriented to the stored edge");
    check(again == face && scene.get_interaction(a, b).size() == 3, "A coincident contact is stored once");
    check(dynamic_cast<InteractionFeatureBeam&>(*beam).volumes[0].get_point(0) == Point(2, 0, 0), "Beam volumes follow the edge");
    check(plate->element_a == b->guid() && plate->element_b == a->guid(), "Plate endpoints filled from the call");
    check(scene.get_interaction(b, a) == scene.get_interaction(a, b), "Either order, the same list");
    check(scene.consistent(), "Feature refers to its contact by guid");
}

static void has_interaction_test() {

    WoodSession scene("has");
    const std::shared_ptr<Element> a = element(scene, "a");
    const std::shared_ptr<Element> b = element(scene, "b");
    const std::shared_ptr<Element> c = element(scene, "c");

    scene.add_edge(a->guid(), b->guid(), "authored");

    check(scene.has_interaction(b, a) && !scene.has_interaction(a, c), "Bare edge found in either order");
    check(scene.get_interaction(a, b).empty(), "Bare edge holds no interaction");
}

static void remove_interaction_test() {

    WoodSession scene("remove");
    const std::shared_ptr<Element> a = element(scene, "a");
    const std::shared_ptr<Element> b = element(scene, "b");
    const std::shared_ptr<Element> c = element(scene, "c");

    scene.add_interaction(a, b, face_contact());
    scene.add_interaction(a, b, std::make_shared<InteractionFeaturePlate>());
    scene.add_interaction(a, c, face_contact());
    const std::string id = scene.graph.edges.at(a->guid()).at(b->guid()).guid();
    a->add_feature(ElementFeature("custom", -1, {}, "keep"));

    scene.remove_interaction(b, a);
    scene.remove_interaction(b, a);

    check(!scene.has_interaction(a, b) && !scene.interactions.count(id), "Removal clears edge and interactions");
    check(a->features_count() == 2 && b->features_count() == 0 && scene.has_interaction(a, c), "Removal clears the hosted features, keeps the other pair and custom features");
    check(!WoodSession::pb_loads(scene.pb_dumps()).has_interaction(a, b), "Removed interaction stays absent after round trip");
}

static void round_trip_test() {

    WoodSession scene("round trip");
    const std::shared_ptr<Element> a = element(scene, "a");
    const std::shared_ptr<Element> b = element(scene, "b");

    const std::shared_ptr<Interaction> face = scene.add_interaction(a, b, face_contact());
    std::shared_ptr<InteractionFeaturePlate> plate = std::make_shared<InteractionFeaturePlate>();
    plate->name = "ss_e_ip_2";
    plate->joint_type = 12;
    plate->contact = *face_contact();
    plate->contact_guid = face->guid();
    scene.add_interaction(a, b, plate);
    scene.add_interaction(a, b, beam_feature());

    const WoodSession loaded = WoodSession::pb_loads(scene.pb_dumps());
    const std::vector<std::shared_ptr<Interaction>> list = loaded.get_interaction(b, a);
    const InteractionFeaturePlate* joint = list.size() == 3 ? dynamic_cast<const InteractionFeaturePlate*>(list[1].get()) : nullptr;

    check(joint && joint->guid() == plate->guid() && joint->name == "ss_e_ip_2" && joint->joint_type == 12 && joint->contact_guid == face->guid(), "Plate joint restored with its guid, name, fields and contact link");
    check(dynamic_cast<const InteractionContactFace*>(list[0].get()) && dynamic_cast<const InteractionFeatureBeam*>(list[2].get()), "Derived types restored");
    check(*list[2] == *scene.get_interaction(a, b)[2], "Beam joint restored field for field");
    check(loaded.consistent(), "Loaded features still find their contacts");

    WoodSession copied = scene;
    const std::vector<std::shared_ptr<Interaction>> copies = copied.get_interaction(a, b);

    check(copies[1] != plate && copies[1]->guid() == plate->guid() && dynamic_cast<const InteractionFeaturePlate*>(copies[1].get()), "Copy is deep, keeps guids and types");

    copies[1]->name = "changed";

    check(plate->name == "ss_e_ip_2", "Copy does not share records");
}

int main() {

    add_interaction_test();
    either_order_test();
    has_interaction_test();
    remove_interaction_test();
    round_trip_test();

    return failures ? 1 : 0;
}
