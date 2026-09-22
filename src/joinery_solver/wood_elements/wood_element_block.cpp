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

    return block;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

void Block::invalidate_geometry() {
    _geometry_synced = false;
}

void Block::compute_geometry() {

    if (loops.size() >= 2 && loops.size() % 2 == 0) {

        std::vector<Polyline> bottom{loops[0]};
        std::vector<Polyline> top{loops[1]};
        for (size_t i = 2; i + 1 < loops.size(); i += 2) {
            bottom.push_back(loops[i]);
            top.push_back(loops[i + 1]);
        }

        set_geometry(Mesh::loft(bottom, top, true));
    }

    std::vector<ElementFeature> next;
    for (ElementFeature& joint : joint_features(*this))
        next.push_back(std::move(joint));

    set_features(std::move(next));
    _geometry_synced = true;
}

AABB Block::aabb(double inflate) const {

    if (const Mesh* solid = std::get_if<Mesh>(&geometry()))
        return AABB::from_mesh(*solid, inflate);

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
    os << "Block(name=" << name << ", loops=" << loops.size() << ", faces=" << (std::holds_alternative<Mesh>(geometry()) ? std::get<Mesh>(geometry()).number_of_faces() : 0) << ")";

    return os.str();
}

}  // namespace wood_session
