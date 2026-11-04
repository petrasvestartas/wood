#include "wood_session.h"
#include <set>

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
};
static const std::set<std::string> OLD_TAGS = {
    "Solid",
    "BlockElement",
    "Support",
    "Plate",
    "WoodElement",
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
    writes_its_name(FacePlanarElement::from_rectangle(
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 100.0, 100.0, 10.0
    ));
    writes_its_name(NodeBlockElement::box({0.0, 0.0, 0.0}, 100.0, 100.0, 100.0));
    writes_its_name(std::make_shared<NodeSupportElement>(Plane::xy_plane(), "support"));

    std::cout << "element_tags: every scene loads, old tags as their classes, new tags written\n";
    return 0;
}
