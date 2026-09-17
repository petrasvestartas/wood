#include "wood_element_block.h"

#include <fstream>
#include <iterator>

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// BlockElement
// ═══════════════════════════════════════════════════════════════════════════

namespace {

// Drop a closing vertex that repeats the first one (to 1e-6), the same test the
// (bottom, top) constructor and loft_mesh apply.
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


void write_binary(const std::string& filename, const std::string& data) {
    std::ofstream file(filename, std::ios::binary);
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
}

std::string read_binary(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

}  // namespace

BlockElement::BlockElement() : element(std::make_shared<wood_session::TaggedElement>("block", ELEMENT_TYPE)) {}

BlockElement::BlockElement(const std::vector<Polyline>& loops, const std::string& name)
    : element(std::make_shared<wood_session::TaggedElement>(name, ELEMENT_TYPE)) {
    element->set_geometry(mesh_from_loops(loops));
    sync_faces();
}

Mesh BlockElement::mesh() const {
    if (const Mesh* m = std::get_if<Mesh>(&element->geometry())) { return *m; }
    return Mesh{};
}

void BlockElement::sync_faces() {
    polylines = element->polylines();   // Mesh::face_outlines()
    planes    = element->planes();      // one per outline
}

void BlockElement::sync_element() const {}   // the solid in `element` IS the block

std::shared_ptr<Element> BlockElement::to_element() const {
    // The same object, not a copy of it: a block IS its mesh, so there is no payload to
    // refresh and nothing to build.
    return element;
}

BlockElement BlockElement::from_element(const Element& e) {
    BlockElement out;
    // Wrapped once, here, and shared from now on: the kernel has no public setter for
    // element_type / element_data, so owning a tagged element means deriving one. The
    // TaggedElement ctor carries the guid across, so this is the same element, not a new one.
    out.element = std::make_shared<wood_session::TaggedElement>(e, ELEMENT_TYPE, e.element_data_dumps());
    if (!std::holds_alternative<Mesh>(e.geometry())) {
        fprintf(stderr, "  WARNING: BlockElement::from_element: element '%s' carries %s, not a "
                        "Mesh - block left empty.\n", e.name.c_str(), e.geometry_type_name().c_str());
        fflush(stderr);
        return out;
    }
    out.sync_faces();
    return out;
}

nlohmann::ordered_json BlockElement::jsondump() const { return to_element()->jsondump(); }
BlockElement BlockElement::jsonload(const nlohmann::json& data) { return from_element(Element::jsonload(data)); }
std::string BlockElement::file_json_dumps() const { return jsondump().dump(); }
BlockElement BlockElement::file_json_loads(const std::string& json_string) {
    return jsonload(nlohmann::ordered_json::parse(json_string));
}
void BlockElement::file_json_dump(const std::string& filename) const {
    std::ofstream file(filename);
    file << jsondump().dump(2);
}
BlockElement BlockElement::file_json_load(const std::string& filename) {
    std::ifstream file(filename);
    return jsonload(nlohmann::json::parse(file));
}
std::string BlockElement::pb_dumps() const { return to_element()->pb_dumps(); }
BlockElement BlockElement::pb_loads(const std::string& data) { return from_element(Element::pb_loads(data)); }
void BlockElement::pb_dump(const std::string& filename) const { write_binary(filename, pb_dumps()); }
BlockElement BlockElement::pb_load(const std::string& filename) { return pb_loads(read_binary(filename)); }

std::string BlockElement::str() const {
    std::ostringstream os;
    os << "BlockElement(name=" << element->name << ", loops=" << polylines.size() << ")";
    return os.str();
}
std::ostream& operator<<(std::ostream& os, const BlockElement& e) { return os << e.str(); }

} // namespace wood_session
