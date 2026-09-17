#include "wood_element_block.h"

#include <sstream>

namespace wood_session {

using session_cpp::Element;
using session_cpp::Mesh;
using session_cpp::Polyline;

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

Block::Block() : Element("block") {}

Block::Block(const std::vector<Polyline>& loops, const std::string& name) : Element(Mesh::from_polylines(loops), name) {}

// ═══════════════════════════════════════════════════════════════════════════
// Serialization
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Block> Block::from_element(const Element& e) {
    auto block = std::make_shared<Block>();
    static_cast<Element&>(*block) = e;
    block->guid() = e.guid();
    return block;
}

void Block::register_type() {
    const Element::Factory factory = [](const std::string& data) -> std::shared_ptr<Element> {
        return from_element(Element::pb_loads(data));
    };
    Element::register_type(ELEMENT_TYPE, factory);
    Element::register_type(LEGACY_ELEMENT_TYPE, factory);
}

// ═══════════════════════════════════════════════════════════════════════════
// Text
// ═══════════════════════════════════════════════════════════════════════════

std::string Block::str() const {
    std::ostringstream os;
    os << "Block(name=" << name << ", faces=" << (std::holds_alternative<Mesh>(geometry()) ? std::get<Mesh>(geometry()).number_of_faces() : 0) << ")";
    return os.str();
}

}  // namespace wood_session
