#include "wood_session.h"
#include <set>
#include "element_column.pb.h"
#include "element.pb.h"

using namespace session_cpp;
using namespace wood_session;

/// Every tracked scene and the tags wood no longer writes: Session reads them for good, as the classes they became.
static const std::vector<std::string> SCENES = {
    "data/crea/crea_4x4_beams_without_column_input.pb",
    "data/crea/crea_4x4_ground_input.pb",
    "data/crea/crea_4x4_input.pb",
    "data/crea/crea_full_input.pb",
    "data/face_to_face_detection/face_to_face_detection.pb",
    "data/floor_model.pb",
    "data/session.pb",
    "rust/tests/data/element_beam.pb",
    "rust/tests/data/element_beam_variable.pb",
    "rust/tests/data/element_block.pb",
    "rust/tests/data/element_column.pb",
    "rust/tests/data/element_plate.pb",
    "rust/tests/data/element_support.pb",
    "rust/tests/data/rust_elements.pb",
    "rust/tests/data/rust_elements_legacy.pb",
};
static const std::set<std::string> OLD_TAGS = {
    "Solid",
    "BlockElement",
    "Support",
    "Plate",
    "WoodElement",
    "Beam",
    "BeamCurved",
    "BeamVariable",
    "Joint",
    "JointBeam",
    "JointPlate",
    "JointAnnen",
    "JointVidy",
    "Pin",
    "ConnectorPart",
    "Column",
};

static void check(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}

/// The scene's elements, none left carrying a tag a class has replaced.
static WoodSession load(const std::string& scene) {
    std::ifstream in(std::string(WOOD_SOURCE_DIR) + "/" + scene, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    check(!bytes.empty(), "missing " + scene);
    WoodSession session = WoodSession::pb_loads(bytes);

    for (const std::shared_ptr<Element>& element : *session.objects.elements)
        check(
            OLD_TAGS.count(element->element_type_name()) == 0,
            scene + ": " + element->name + " stays a plain " + element->element_type_name()
        );

    return session;
}

/// The written tag: a class round trips under its own name.
template <typename T> static void writes_its_name(const std::shared_ptr<T>& element) {
    const std::shared_ptr<Element> back = Element::pb_loads_polymorphic(element->pb_dumps());
    check(back->element_type_name() == T::ELEMENT_TYPE, "wrong tag " + back->element_type_name());
    check(std::dynamic_pointer_cast<T>(back) != nullptr, "does not load as its class");
}

int main() {
    for (const std::string& scene : SCENES)
        load(scene);

    check(
        load("rust/tests/data/element_block.pb").get_elements<NodeBlockElement>().size() == 1,
        "the old block loads as a NodeBlockElement"
    );
    check(
        load("rust/tests/data/element_support.pb").get_elements<NodeSupportElement>().size() == 1,
        "the old support loads as a NodeSupportElement"
    );
    check(
        load("rust/tests/data/element_block.pb").get_elements<NodeElement>().size() == 1,
        "a block is a NodeElement"
    );
    check(
        load("rust/tests/data/element_plate.pb").get_elements<FaceElement>().size() == 1,
        "the old plate loads as a FaceElement"
    );
    check(
        load("rust/tests/data/element_beam.pb").get_elements<EdgeStraightElement>().size() == 1,
        "the old beam loads as an EdgeStraightElement"
    );
    check(
        load("rust/tests/data/element_beam_variable.pb").get_elements<EdgeElement>().size() == 1,
        "the old variable beam loads as an EdgeElement"
    );
    const JointPinElement pin(Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 100.0}), 5.0, 0.05);
    const std::shared_ptr<Element> pin_back = Element::pb_loads_polymorphic(pin.pb_dumps());
    check(pin_back->element_type_name() == "JointPinElement", "the pin writes its name");
    check(std::dynamic_pointer_cast<JointPinElement>(pin_back) != nullptr, "the pin loads as itself");
    writes_its_name(EdgeStraightElement::square(
        Polyline({Point(0.0, 0.0, 0.0), Point(100.0, 0.0, 0.0)}), 10.0
    ));
    check(
        load("rust/tests/data/element_column.pb").get_elements<EdgeStraightElement>().size() == 1,
        "the old column loads as an EdgeStraightElement"
    );

    // a column whose level section leans off its axis loads as the same loft
    wood_proto::Column leaning;
    const Polyline foot({{0.0, 0.0, 0.0}, {200.0, 0.0, 0.0}, {200.0, 200.0, 0.0}, {0.0, 200.0, 0.0}, {0.0, 0.0, 0.0}});
    const Line slanted = Line::from_points({100.0, 100.0, 0.0}, {400.0, 100.0, 3000.0});
    check(leaning.mutable_axis()->ParseFromString(slanted.pb_dumps()), "the axis writes");
    check(leaning.mutable_section()->ParseFromString(foot.pb_dumps()), "the section writes");
    session_proto::Element saved;
    check(saved.ParseFromString(Element("leaning").pb_dumps()), "the element writes");
    saved.set_element_type("Column");
    saved.set_element_data(leaning.SerializeAsString());
    const std::shared_ptr<EdgeVariableElement> loft = std::dynamic_pointer_cast<EdgeVariableElement>(
        Element::pb_loads_polymorphic(saved.SerializeAsString())
    );
    check(loft != nullptr, "the leaning column loads as an EdgeVariableElement");
    check(
        std::abs(loft->element_geometry_mesh().volume() - 200.0 * 200.0 * 3000.0) <= 1e-3,
        "the leaning column keeps its volume"
    );

    // a round edge keeps its radius through a save, and a section off square is refused
    const std::shared_ptr<EdgeStraightElement> round = std::dynamic_pointer_cast<EdgeStraightElement>(
        Element::pb_loads_polymorphic(
            EdgeStraightElement::round(Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 900.0}), 100.0)->pb_dumps()
        )
    );
    check(round && round->round_radius == 100.0, "the round radius survives a save");
    bool refused = false;
    try {
        EdgeStraightElement::from_section(slanted, foot);
    } catch (const std::invalid_argument&) {
        refused = true;
    }
    check(refused, "a section off square to the axis is refused");

    // a cutting plane is a joint: it saves as one and cuts a member through its interaction
    const std::shared_ptr<JointCutPlaneElement> plane = std::make_shared<JointCutPlaneElement>(
        Plane::from_point_normal({50.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}), 300.0, "plane"
    );
    const std::shared_ptr<JointCutPlaneElement> plane_back =
        std::dynamic_pointer_cast<JointCutPlaneElement>(Element::pb_loads_polymorphic(plane->pb_dumps()));
    check(
        plane_back && plane_back->size == 300.0 && plane_back->plane().origin().distance({50.0, 0.0, 0.0}) < 1e-9,
        "the plane and its size survive a save"
    );
    WoodSession cut("cut");
    const std::shared_ptr<EdgeStraightElement> stock =
        EdgeStraightElement::square(Polyline({Point(0.0, 0.0, 0.0), Point(100.0, 0.0, 0.0)}), 10.0);
    cut.add(stock);
    cut.add(plane);
    cut.add_interaction(plane, stock, plane->interaction(0));
    check(
        std::abs(stock->model_geometry_mesh().volume() - 50.0 * 20.0 * 20.0) < 1e-6,
        "the plane cuts the member"
    );
    check(cut.get_elements<JointElement>().size() == 1, "the plane is a joint");

    writes_its_name(FacePlanarElement::from_rectangle(
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 100.0, 100.0, 10.0
    ));
    writes_its_name(NodeBlockElement::box({0.0, 0.0, 0.0}, 100.0, 100.0, 100.0));
    writes_its_name(std::make_shared<NodeSupportElement>(Plane::xy_plane(), "support"));

    std::cout << "element_tags: every scene loads, old tags as their classes, new tags written\n";
    return 0;
}
