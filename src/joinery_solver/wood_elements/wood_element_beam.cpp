#include "pch.h"
#include "wood_serialization.h"
#include "wood_element_beam.h"
#include "wood_element_geometry.h"
#include "wood_profile.h"
#include "element_beam.pb.h"
using namespace session_cpp;

namespace wood_session {

Beam::Beam() : Element("beam") {}

Beam::Beam(const Polyline& axis, double radius, const std::string& name)
    : Element(name), axis(axis), radii(axis.segment_count(), radius) {}

Beam::Beam(const Polyline& axis, const std::vector<double>& radii, const std::vector<Vector>& directions, int allowed_type, const std::string& name)
    : Element(name), axis(axis), radii(radii), directions(directions), allowed_type(allowed_type) {}

Beam::Beam(const Polyline& axis, const std::vector<Polyline>& profile, const std::vector<Vector>& directions, const std::string& name)
    : Element(name), axis(axis), radii(axis.segment_count(), compute_size(profile).first / 2.0), directions(directions), profile(profile) {}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Beam> Beam::from_element(Element e) {

    const std::string bytes = e.element_data_dumps();
    std::shared_ptr<Beam> beam = std::make_shared<Beam>();
    static_cast<Element&>(*beam) = std::move(e);

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
    for (const session_proto::Polyline& ring : proto.profile())
        beam->profile.push_back(Polyline::pb_loads(ring.SerializeAsString()));

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
        sections.push_back(profile.empty() ? square_section(points[i], along, up, radius(segment)) : profile_section(points[i], along, up, profile[0]));
    }

    return sections;
}

/// Every profile loop placed at both ends of a one-segment axis: what a hollow beam lofts between.
static std::pair<std::vector<Polyline>, std::vector<Polyline>> profile_ends(const Beam& beam) {

    const Vector along = beam.axis.get_point(1) - beam.axis.get_point(0);
    const Vector up = beam.has_direction(0) ? beam.directions[0] : Vector::z_axis();

    std::pair<std::vector<Polyline>, std::vector<Polyline>> ends;
    for (const Polyline& ring : beam.profile) {
        ends.first.push_back(profile_section(beam.axis.get_point(0), along, up, ring));
        ends.second.push_back(profile_section(beam.axis.get_point(1), along, up, ring));
    }

    return ends;
}

const Mesh& Beam::element_geometry_mesh() const {

    if (!_element_geometry_mesh) {
        if (profile.size() > 1 && axis.segment_count() == 1) {
            const std::pair<std::vector<Polyline>, std::vector<Polyline>> ends = profile_ends(*this);
            _element_geometry_mesh = Mesh::loft(ends.first, ends.second, true);
        } else
            _element_geometry_mesh = sweep_sections(sections());
    }

    return *_element_geometry_mesh;
}

const BRep& Beam::element_geometry_brep() const {

    if (!_element_geometry_brep) {
        if (profile.size() > 1 && axis.segment_count() == 1) {
            const std::pair<std::vector<Polyline>, std::vector<Polyline>> ends = profile_ends(*this);
            _element_geometry_brep = brep_between_loops(ends.first, ends.second);
        } else
            _element_geometry_brep = brep_sections(sections());
    }

    return *_element_geometry_brep;
}

const Mesh& Beam::model_geometry_mesh() const {

    if (!_model_geometry_mesh) {
        _model_geometry_mesh = cut_mesh(element_geometry_mesh(), cuts);
    }

    return *_model_geometry_mesh;
}

const BRep& Beam::model_geometry_brep() const {

    if (!_model_geometry_brep) {
        _model_geometry_brep = cut_brep(element_geometry_brep(), cuts);
    }

    return *_model_geometry_brep;
}

std::vector<Plane> Beam::compute_planes() const {
    return face_planes(model_geometry_mesh());
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
    beam->profile = profile;
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

void Beam::compute_geometry_mesh_impl() {

    const std::vector<Polyline> rings = sections();
    if (!rings.empty()) {
        set_geometry(model_geometry_mesh());
    }
    compute_geometry_features();
}

void Beam::compute_geometry_brep_impl() {

    const std::vector<Polyline> rings = sections();
    if (!rings.empty()) {
        set_geometry(model_geometry_brep());
    }
    compute_geometry_features();
}

void Beam::compute_geometry_features() {

    const std::pair<Polyline, std::vector<Polyline>> trimmed = trim_to_cuts(axis, sections(), cuts);

    std::vector<ElementFeature> next;
    next.push_back(polyline_feature("axis", trimmed.first));

    for (const Polyline& ring : trimmed.second)
        if (ring.point_count() > 0)
            next.push_back(polyline_feature("section", ring));

    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
}

AABB Beam::aabb(double inflate) const {

    const std::pair<double, double> size = compute_size(profile);
    double reach = std::max(size.first, size.second) / 2.0;
    for (const double radius : radii)
        reach = std::max(reach, radius);

    return AABB::from_polyline(axis, inflate + reach);
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json Beam::element_data_jsondump() const {

    wood_proto::Beam proto;
    if (!proto.ParseFromString(element_data_dumps()))
        throw std::runtime_error("Failed to parse Beam protobuf data");

    return json_of(proto);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string Beam::element_data_dumps() const {

    wood_proto::Beam proto;
    if (!proto.mutable_axis()->ParseFromString(axis.pb_dumps()))
        throw std::runtime_error("Failed to parse Line protobuf data");
    proto.mutable_radii()->Add(radii.begin(), radii.end());
    for (const Vector& direction : directions)
        if (!proto.add_directions()->ParseFromString(direction.pb_dumps()))
            throw std::runtime_error("Failed to parse Vector protobuf data");
    proto.set_allowed_type(allowed_type);
    for (const Plane& cut : cuts)
        if (!proto.add_cuts()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Failed to parse Plane protobuf data");
    for (const Polyline& ring : profile)
        if (!proto.add_profile()->ParseFromString(ring.pb_dumps()))
            throw std::runtime_error("Failed to parse Polyline protobuf data");

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
