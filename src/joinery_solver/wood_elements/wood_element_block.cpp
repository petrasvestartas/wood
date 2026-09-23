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

Block::Block() : Element("block") {}

Block::Block(const std::vector<Polyline>& loops, const std::string& name) : Element(name), loops(loops) {}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Block> Block::from_element(const Element& e) {

    std::shared_ptr<Block> block = std::make_shared<Block>();
    static_cast<Element&>(*block) = e;
    block->guid() = e.guid();

    wood_proto::Block proto;
    if (!proto.ParseFromString(e.element_data_dumps()))
        return block;

    for (const session_proto::Polyline& loop : proto.loops())
        block->loops.push_back(Polyline::pb_loads(loop.SerializeAsString()));
    for (const session_proto::Plane& cut : proto.cuts())
        block->cuts.push_back(Plane::pb_loads(cut.SerializeAsString()));

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
        _element_geometry_mesh = bottom.empty() ? Mesh() : Mesh::loft(bottom, top, true);
    }

    return *_element_geometry_mesh;
}

const BRep& Block::element_geometry_brep() const {

    if (!_element_geometry_brep) {
        const auto [bottom, top] = split_loops(loops);
        _element_geometry_brep = bottom.empty() ? BRep() : brep_between_loops(bottom, top);
    }

    return *_element_geometry_brep;
}

const Mesh& Block::model_geometry_mesh() const {

    if (!_model_geometry_mesh) {
        _model_geometry_mesh = cut_geometry(element_geometry_mesh(), cuts);
    }

    return *_model_geometry_mesh;
}

const BRep& Block::model_geometry_brep() const {

    if (!_model_geometry_brep) {
        _model_geometry_brep = cut_geometry(element_geometry_brep(), cuts);
    }

    return *_model_geometry_brep;
}

void Block::invalidate_geometry() {
    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_geometry_mesh.reset();
    _model_geometry_brep.reset();
    _geometry_synced = false;
}

std::shared_ptr<Block> Block::transformed(const Xform& xform) const {

    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<Block> block = std::make_shared<Block>(transformed_list(loops, xform), name);
    block->guid() = guid();
    block->cuts = transformed_list(cuts, xform);
    block->set_features(transformed_features(_features, xform));
    block->set_insertion_vectors(transformed_list(_insertion_vectors, xform));

    return block;
}

void Block::place(const Xform& xform) {

    Element::place(xform);
    loops = transformed_list(loops, xform);
    cuts = transformed_list(cuts, xform);

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_geometry_mesh.reset();
    _model_geometry_brep.reset();
}

void Block::compute_geometry_mesh_impl() {

    if (loops.size() >= 2 && loops.size() % 2 == 0) {
        set_geometry(model_geometry_mesh());
    }
    compute_geometry_features();
}

void Block::compute_geometry_brep_impl() {

    if (loops.size() >= 2 && loops.size() % 2 == 0) {
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
    proto.ParseFromString(element_data_dumps());

    return json_of(proto);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string Block::element_data_dumps() const {

    wood_proto::Block proto;
    for (const Polyline& loop : loops)
        proto.add_loops()->ParseFromString(loop.pb_dumps());
    for (const Plane& cut : cuts)
        proto.add_cuts()->ParseFromString(cut.pb_dumps());

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
    os << "Block(name=" << name << ", loops=" << loops.size() << ", faces=" << geometry_mesh().number_of_faces() << ")";

    return os.str();
}

}  // namespace wood_session
