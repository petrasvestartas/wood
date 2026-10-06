#include "pch.h"
#include "wood_serialization.h"
#include "wood_element_block.h"
#include "wood_element_geometry.h"
#include "element_block.pb.h"

namespace wood_session {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

Block::Block() : WoodElement("block") {}
Block::Block(const Mesh& mesh, const std::string& name) : WoodElement(name), source_mesh(mesh) {}

Block::Block(const std::vector<Polyline>& loops, const std::string& name) : WoodElement(name), loops(loops) {}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Block> Block::from_element(Element e) {

    const std::string bytes = e.element_data_dumps();
    std::shared_ptr<Block> block = std::make_shared<Block>();
    static_cast<Element&>(*block) = std::move(e);

    wood_proto::Block proto;
    if (!proto.ParseFromString(bytes)) {
        if (block->_geometry_mesh) block->source_mesh = *block->_geometry_mesh;
        return block;
    }

    for (const session_proto::Polyline& loop : proto.loops())
        block->loops.push_back(Polyline::pb_loads(loop.SerializeAsString()));
    for (const session_proto::Plane& cut : proto.cuts())
        block->cuts.push_back(Plane::pb_loads(cut.SerializeAsString()));

    if (proto.has_source_mesh()) block->source_mesh = Mesh::pb_loads(proto.source_mesh().SerializeAsString());
    else if (block->loops.empty() && block->_geometry_mesh) block->source_mesh = *block->_geometry_mesh;
    for (const auto& cut : proto.solid_cuts()) block->solid_cuts.push_back(SolidCut::pb_loads(cut.SerializeAsString()));
    return block;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

/// The loops split into the bottom list and the top list, holes paired in order; empty when the count is odd or below two.
static std::pair<std::vector<Polyline>, std::vector<Polyline>> split_loops(const std::vector<Polyline>& loops) {

    if (loops.size() < 2 || loops.size() % 2 != 0)
        return {};

    std::vector<Polyline> bottom{loops[0]};
    std::vector<Polyline> top{loops[1]};
    for (size_t i = 2; i + 1 < loops.size(); i += 2) {
        bottom.push_back(loops[i]);
        top.push_back(loops[i + 1]);
    }

    return {bottom, top};
}

const Mesh& Block::element_geometry_mesh() const {

    if (!_element_geometry_mesh) {
        const auto [bottom, top] = split_loops(loops);
        _element_geometry_mesh = bottom.empty() ? source_mesh.value_or(Mesh()) : Mesh::loft(bottom, top, true);
    }

    return *_element_geometry_mesh;
}

const BRep& Block::element_geometry_brep() const {

    if (!_element_geometry_brep) {
        const auto [bottom, top] = split_loops(loops);
        _element_geometry_brep = bottom.empty() ? mesh_brep(element_geometry_mesh()) : brep_between_loops(bottom, top);
    }

    return *_element_geometry_brep;
}

Mesh Block::trimmed_mesh() const {
    return cut_mesh(element_geometry_mesh(), cuts);
}

BRep Block::trimmed_brep() const {
    return cut_brep(element_geometry_brep(), cuts);
}

std::vector<Plane> Block::compute_planes() const {
    return face_planes(model_geometry_mesh());
}

std::shared_ptr<Block> Block::transformed(const Xform& xform) const {

    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<Block> block = std::make_shared<Block>(transformed_list(loops, xform), name);
    block->guid() = guid();
    if (source_mesh) block->source_mesh = source_mesh->transformed(xform);
    block->cuts = transformed_list(cuts, xform);
    for (const auto& cut : solid_cuts) block->solid_cuts.push_back(cut.transformed(xform));
    block->set_features(transformed_features(_features, xform));
    block->set_insertion_vectors(transformed_list(_insertion_vectors, xform));

    return block;
}

void Block::place(const Xform& xform) {

    Element::place(xform);
    loops = transformed_list(loops, xform);
    if (source_mesh) source_mesh = source_mesh->transformed(xform);
    cuts = transformed_list(cuts, xform);
    for (auto& cut : solid_cuts) cut = cut.transformed(xform);

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_mesh_cache.reset();
    _model_brep_cache.reset();
}

void Block::compute_geometry_mesh_impl() {

    if (source_mesh || (loops.size() >= 2 && loops.size() % 2 == 0)) {
        set_geometry(model_geometry_mesh());
    }
    compute_geometry_features();
}

void Block::compute_geometry_brep_impl() {

    if (source_mesh || (loops.size() >= 2 && loops.size() % 2 == 0)) {
        set_geometry(model_geometry_brep());
    }
    compute_geometry_features();
}

void Block::compute_geometry_features() {

    std::vector<ElementFeature> next;
    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
}

AABB Block::aabb(double inflate) const {

    const Mesh& solid = geometry_mesh();
    if (solid.number_of_vertices() > 0)
        return AABB::from_mesh(solid, inflate);

    std::vector<Point> points;
    for (const Polyline& loop : loops) {
        const std::vector<Point> corners = loop.get_points();
        points.insert(points.end(), corners.begin(), corners.end());
    }

    return AABB::from_points(points, inflate);
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json Block::element_data_jsondump() const {

    wood_proto::Block proto;
    if (!proto.ParseFromString(element_data_dumps()))
        throw std::runtime_error("Failed to parse Block protobuf data");

    return json_of(proto);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string Block::element_data_dumps() const {

    wood_proto::Block proto;
    if (source_mesh && !proto.mutable_source_mesh()->ParseFromString(source_mesh->pb_dumps())) throw std::runtime_error("Invalid block source mesh");
    for (const Polyline& loop : loops)
        if (!proto.add_loops()->ParseFromString(loop.pb_dumps()))
            throw std::runtime_error("Failed to parse Polyline protobuf data");
    for (const Plane& cut : cuts)
        if (!proto.add_cuts()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Failed to parse Plane protobuf data");

    for (const auto& cut : solid_cuts)
        if (!proto.add_solid_cuts()->ParseFromString(cut.pb_dumps())) throw std::runtime_error("Invalid solid cut");
    return proto.SerializeAsString();
}

/// The element factory of a serialized block: the protobuf bytes decoded as an Element and promoted to a Block.
static std::shared_ptr<Element> block_from_protobuf(const std::string& data) {
    return Block::from_element(Element::pb_loads(data));
}

void Block::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), block_from_protobuf);
    Element::register_type(std::string(LEGACY_ELEMENT_TYPE), block_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

std::string Block::str() const {

    std::ostringstream os;
    os << "Block(name=" << name << ", loops=" << loops.size() << ", faces=" << geometry_mesh().number_of_faces() << ", is_visible=" << std::boolalpha << is_visible << ", is_locked=" << is_locked << ")";

    return os.str();
}

}  // namespace wood_session
