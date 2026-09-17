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

/// Drop a closing vertex that repeats the first one (to 1e-6).
void strip_closing(std::vector<Point>& v) {
    if (v.size() > 3) {
        const Point& f = v.front();
        const Point& l = v.back();
        if (std::abs(f[0]-l[0]) < 1e-6 && std::abs(f[1]-l[1]) < 1e-6 &&
            std::abs(f[2]-l[2]) < 1e-6) { v.pop_back(); }
    }
}


/// The loops as one mesh: one n-gon face per loop, vertices NOT shared between faces -
/// unwelded, the mesh is exactly those loops, so Mesh::face_outlines() gives them back.
Mesh mesh_from_loops(const std::vector<Polyline>& loops) {
    std::vector<Point> verts;
    std::vector<std::vector<size_t>> faces;
    faces.reserve(loops.size());
    for (const Polyline& loop : loops) {
        std::vector<Point> pts = loop.get_points();
        strip_closing(pts);
        if (pts.size() < 3) { continue; }
        std::vector<size_t> face(pts.size());
        for (size_t k = 0; k < pts.size(); ++k) {
            face[k] = verts.size();
            verts.push_back(pts[k]);
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
