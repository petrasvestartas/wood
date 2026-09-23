#include "pch.h"
#include "wood_serialization.h"
#include "wood_element_beam.h"
#include "wood_element_geometry.h"
#include "element_beam.pb.h"
using namespace session_cpp;

namespace wood_session {

Beam::Beam() : Element("beam") {}

Beam::Beam(const Polyline& axis, double radius, const std::string& name)
    : Element(name), axis(axis), radii(axis.segment_count(), radius) {}

Beam::Beam(const Polyline& axis, const std::vector<double>& radii, const std::vector<Vector>& directions, int allowed_type, const std::string& name)
    : Element(name), axis(axis), radii(radii), directions(directions), allowed_type(allowed_type) {}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Beam> Beam::from_element(const Element& e) {

    std::shared_ptr<Beam> beam = std::make_shared<Beam>();
    static_cast<Element&>(*beam) = e;
    beam->guid() = e.guid();

    const std::string bytes = e.element_data_dumps();
    if (!bytes.empty() && bytes.front() == '{') {
        try {
            const nlohmann::json payload = nlohmann::json::parse(bytes);
            if (payload.contains("axis") && !payload["axis"].is_null())
                beam->axis = Polyline::jsonload(payload["axis"]);
            if (payload.contains("radii"))
                beam->radii = payload["radii"].get<std::vector<double>>();
            if (payload.contains("directions"))
                for (const nlohmann::json& direction : payload["directions"])
                    beam->directions.push_back(Vector::jsonload(direction));
            beam->allowed_type = payload.value("allowed_type", -1);
        } catch (const std::exception&) {
        }
        return beam;
    }

    wood_proto::Beam proto;
    if (!proto.ParseFromString(bytes))
        return beam;

    if (proto.has_axis())
        beam->axis = Polyline::pb_loads(proto.axis().SerializeAsString());
    beam->radii.assign(proto.radii().begin(), proto.radii().end());
    for (const session_proto::Vector& direction : proto.directions())
        beam->directions.push_back(Vector::pb_loads(direction.SerializeAsString()));
    beam->allowed_type = proto.allowed_type();
    for (const session_proto::Plane& cut : proto.cuts())
        beam->cuts.push_back(Plane::pb_loads(cut.SerializeAsString()));

    return beam;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

double Beam::radius(int segment) const {
    return segment >= 0 && segment < (int)radii.size() ? radii[segment] : 0.0;
}

bool Beam::has_direction(int segment) const {
    return segment >= 0 && segment < (int)directions.size();
}

std::vector<Polyline> Beam::sections() const {

    std::vector<Polyline> sections;
    const int segments = static_cast<int>(axis.segment_count());
    if (segments < 1 || radii.empty())
        return sections;

    const std::vector<Point> points = axis.get_points();
    for (int i = 0; i <= segments; i++) {

        const int segment = std::min(i, segments - 1);
        Vector along = points[segment + 1] - points[segment];
        if (i > 0 && i < segments)
            along = along.normalized() + (points[i] - points[i - 1]).normalized();

        const Vector up = has_direction(segment) ? directions[segment] : Vector::z_axis();
        sections.push_back(square_section(points[i], along, up, radius(segment)));
    }

    return sections;
}

const ElementGeometry& Beam::element_geometry(bool mesh_or_brep) const {

    std::optional<ElementGeometry>& cache = mesh_or_brep ? _element_geometry_mesh : _element_geometry_brep;
    if (!cache)
        cache = compute_element_geometry(mesh_or_brep);

    return *cache;
}

ElementGeometry Beam::compute_element_geometry(bool mesh_or_brep) const {

    if (mesh_or_brep)
        return sweep_sections(sections());

    return brep_sections(sections());
}

const ElementGeometry& Beam::model_geometry(bool mesh_or_brep) const {

    std::optional<ElementGeometry>& cache = mesh_or_brep ? _model_geometry_mesh : _model_geometry_brep;
    if (!cache)
        cache = compute_model_geometry(mesh_or_brep);

    return *cache;
}

ElementGeometry Beam::compute_model_geometry(bool mesh_or_brep) const {
    return cut_geometry(element_geometry(mesh_or_brep), cuts);
}

void Beam::invalidate_geometry() {
    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_geometry_mesh.reset();
    _model_geometry_brep.reset();
    _geometry_synced = false;
}

/// The up directions moved by xform; while xform tilts z every segment without one takes xform·z, the world z sections() used before the move.
static std::vector<Vector> transformed_directions(const std::vector<Vector>& directions, size_t segments, const Xform& xform) {

    std::vector<Vector> moved = transformed_list(directions, xform);
    const Vector up = Vector::z_axis().transformed(xform);

    if (up != Vector::z_axis() && moved.size() < segments)
        moved.resize(segments, up);

    return moved;
}

std::shared_ptr<Beam> Beam::transformed(const Xform& xform) const {

    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<Beam> beam = std::make_shared<Beam>(axis.transformed(xform), radii, transformed_directions(directions, axis.segment_count(), xform), allowed_type, name);
    beam->guid() = guid();
    beam->cuts = transformed_list(cuts, xform);
    beam->set_features(transformed_features(_features, xform));
    beam->set_insertion_vectors(transformed_list(_insertion_vectors, xform));

    return beam;
}

void Beam::place(const Xform& xform) {

    Element::place(xform);
    directions = transformed_directions(directions, axis.segment_count(), xform);
    axis.transform(xform);
    cuts = transformed_list(cuts, xform);

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_geometry_mesh.reset();
    _model_geometry_brep.reset();
}

void Beam::compute_geometry_impl(bool mesh_or_brep) {

    const std::vector<Polyline> rings = sections();
    if (!rings.empty()) {
        set_geometry(model_geometry(mesh_or_brep));
        _element_geometry_mesh.reset();
        _element_geometry_brep.reset();
        _model_geometry_mesh.reset();
        _model_geometry_brep.reset();
    }

    std::vector<ElementFeature> next;
    next.push_back(polyline_feature("axis", axis));
    for (const Polyline& ring : rings)
        next.push_back(polyline_feature("section", ring));
    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
}

AABB Beam::aabb(double inflate) const {

    const double reach = radii.empty() ? 0.0 : *std::max_element(radii.begin(), radii.end());

    return AABB::from_polyline(axis, inflate + reach);
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json Beam::element_data_jsondump() const {

    wood_proto::Beam proto;
    proto.ParseFromString(element_data_dumps());

    return json_of(proto);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string Beam::element_data_dumps() const {

    wood_proto::Beam proto;
    proto.mutable_axis()->ParseFromString(axis.pb_dumps());
    proto.mutable_radii()->Add(radii.begin(), radii.end());
    for (const Vector& direction : directions)
        proto.add_directions()->ParseFromString(direction.pb_dumps());
    proto.set_allowed_type(allowed_type);
    for (const Plane& cut : cuts)
        proto.add_cuts()->ParseFromString(cut.pb_dumps());

    return proto.SerializeAsString();
}

/// The element factory of a serialized beam: the protobuf bytes decoded as an Element and promoted to a Beam.
static std::shared_ptr<Element> beam_from_protobuf(const std::string& data) {
    return Beam::from_element(Element::pb_loads(data));
}

void Beam::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), beam_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

std::string Beam::str() const {

    std::ostringstream os;
    os << "Beam(name=" << name << ", segments=" << axis.segment_count() << ", radii=" << radii.size() << ")";

    return os.str();
}

} // namespace wood_session
