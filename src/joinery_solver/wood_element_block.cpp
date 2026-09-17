#include "wood_pch.h"
#include "wood_element_block.h"

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
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Block> Block::from_element(const Element& e) {

    std::shared_ptr<Block> block = std::make_shared<Block>();
    static_cast<Element&>(*block) = e;
    block->guid() = e.guid();

    return block;
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

/// The element factory of a serialized block: the protobuf bytes decoded as an Element and promoted to a Block.
static std::shared_ptr<Element> block_from_protobuf(const std::string& data) {
    return Block::from_element(Element::pb_loads(data));
}

void Block::register_type() {
    Element::register_type(ELEMENT_TYPE, block_from_protobuf);
    Element::register_type(LEGACY_ELEMENT_TYPE, block_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

std::string Block::str() const {

    std::ostringstream os;
    os << "Block(name=" << name << ", faces=" << (std::holds_alternative<Mesh>(geometry()) ? std::get<Mesh>(geometry()).number_of_faces() : 0) << ")";

    return os.str();
}

}  // namespace wood_session
