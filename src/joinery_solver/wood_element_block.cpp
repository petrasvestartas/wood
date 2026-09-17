#include "wood_element_block.h"

#include <cmath>
#include <sstream>

namespace wood_session {

using session_cpp::Element;
using session_cpp::Mesh;
using session_cpp::Point;
using session_cpp::Polyline;

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// The loops as one mesh: one n-gon face per loop, vertices NOT shared between faces -
/// unwelded, the mesh is exactly those loops, so Mesh::face_outlines() gives them back.
Mesh mesh_from_loops(const std::vector<Polyline>& loops) {
    std::vector<Point> verts;
    std::vector<std::vector<size_t>> faces;
    faces.reserve(loops.size());
    for (const Polyline& loop : loops) {
        size_t count = loop.point_count();
        if (count > 3 && loop.get_point(0).distance(loop.get_point(count - 1)) < 1e-6) count--;
        if (count < 3) { continue; }
        std::vector<size_t> face(count);
        for (size_t k = 0; k < count; ++k) {
            face[k] = verts.size();
            verts.push_back(loop.get_point(k));
        }
        faces.push_back(std::move(face));
    }
    if (faces.empty()) { return Mesh{}; }
    return Mesh::from_vertices_and_faces(verts, faces);
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

Block::Block() : Element("block") {}

Block::Block(const std::vector<Polyline>& loops, const std::string& name) : Element(mesh_from_loops(loops), name) {}

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

} // namespace wood_session
