#include "pch.h"
#include "wood_serialization.h"
#include "wood_element_beam_variable.h"
#include "wood_element_geometry.h"
#include "element_beam_variable.pb.h"

namespace wood_session {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

BeamVariable::BeamVariable() : WoodElement("beam_variable") {}

BeamVariable::BeamVariable(const Line& axis, const std::vector<Polyline>& sections, const std::string& name) : WoodElement(name), axis(axis), sections(sections) {}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<BeamVariable> BeamVariable::between(const Polyline& first, const Polyline& last, const std::string& name) {

    const Line axis = Line::from_points(Point::centroid(first.open_points()), Point::centroid(last.open_points()));

    return std::make_shared<BeamVariable>(axis, std::vector<Polyline>{first, last}, name);
}

std::shared_ptr<BeamVariable> BeamVariable::from_element(Element e) {

    const std::string bytes = e.element_data_dumps();
    std::shared_ptr<BeamVariable> beam = std::make_shared<BeamVariable>();
    static_cast<Element&>(*beam) = std::move(e);

    wood_proto::BeamVariable proto;

    if (!proto.ParseFromString(bytes))
        return beam;

    beam->axis = Line::pb_loads(proto.axis().SerializeAsString());

    for (const session_proto::Polyline& section : proto.sections())
        beam->sections.push_back(Polyline::pb_loads(section.SerializeAsString()));

    for (const session_proto::Plane& cut : proto.cuts())
        beam->cuts.push_back(Plane::pb_loads(cut.SerializeAsString()));

    for (const wood_proto::SolidCut& cut : proto.solid_cuts())
        beam->solid_cuts.push_back(SolidCut::pb_loads(cut.SerializeAsString()));

    return beam;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

const Mesh& BeamVariable::element_geometry_mesh() const {

    if (!_element_geometry_mesh)
        _element_geometry_mesh = loft_stations(sections);

    return *_element_geometry_mesh;
}

const BRep& BeamVariable::element_geometry_brep() const {

    if (!_element_geometry_brep)
        _element_geometry_brep = mesh_brep(element_geometry_mesh());

    return *_element_geometry_brep;
}

Mesh BeamVariable::trimmed_mesh() const {
    return cut_mesh(element_geometry_mesh(), cuts);
}

BRep BeamVariable::trimmed_brep() const {
    return cut_brep(element_geometry_brep(), cuts);
}

std::vector<Plane> BeamVariable::compute_planes() const {
    return face_planes(model_geometry_mesh());
}

std::shared_ptr<BeamVariable> BeamVariable::transformed(const Xform& xform) const {

    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<BeamVariable> beam = std::make_shared<BeamVariable>(axis.transformed(xform), transformed_list(sections, xform), name);
    beam->guid() = guid();
    beam->cuts = transformed_list(cuts, xform);

    for (const SolidCut& cut : solid_cuts)
        beam->solid_cuts.push_back(cut.transformed(xform));

    beam->set_features(transformed_features(_features, xform));
    beam->set_insertion_vectors(transformed_list(_insertion_vectors, xform));

    return beam;
}

void BeamVariable::place(const Xform& xform) {

    Element::place(xform);
    axis.transform(xform);
    sections = transformed_list(sections, xform);
    cuts = transformed_list(cuts, xform);

    for (SolidCut& cut : solid_cuts)
        cut = cut.transformed(xform);

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_mesh_cache.reset();
    _model_brep_cache.reset();
}

void BeamVariable::compute_geometry_mesh_impl() {

    if (sections.size() >= 2)
        set_geometry(model_geometry_mesh());

    compute_geometry_features();
}

void BeamVariable::compute_geometry_brep_impl() {

    if (sections.size() >= 2)
        set_geometry(model_geometry_brep());

    compute_geometry_features();
}

void BeamVariable::compute_geometry_features() {

    std::vector<ElementFeature> next;

    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
}

AABB BeamVariable::aabb(double inflate) const {

    const Mesh& solid = geometry_mesh();

    if (solid.number_of_vertices() > 0)
        return AABB::from_mesh(solid, inflate);

    std::vector<Point> points;

    for (const Polyline& section : sections) {
        const std::vector<Point> corners = section.get_points();
        points.insert(points.end(), corners.begin(), corners.end());
    }

    return AABB::from_points(points, inflate);
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json BeamVariable::element_data_jsondump() const {

    wood_proto::BeamVariable proto;

    if (!proto.ParseFromString(element_data_dumps()))
        throw std::runtime_error("Failed to parse BeamVariable protobuf data");

    return json_of(proto);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string BeamVariable::element_data_dumps() const {

    wood_proto::BeamVariable proto;

    if (!proto.mutable_axis()->ParseFromString(axis.pb_dumps()))
        throw std::runtime_error("Invalid beam axis");

    for (const Polyline& section : sections)
        if (!proto.add_sections()->ParseFromString(section.pb_dumps()))
            throw std::runtime_error("Invalid beam section");

    for (const Plane& cut : cuts)
        if (!proto.add_cuts()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Invalid beam cut");

    for (const SolidCut& cut : solid_cuts)
        if (!proto.add_solid_cuts()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Invalid solid cut");

    return proto.SerializeAsString();
}

/// The element factory of a serialized variable beam: the protobuf bytes decoded as an Element and promoted to a BeamVariable.
static std::shared_ptr<Element> beam_variable_from_protobuf(const std::string& data) {
    return BeamVariable::from_element(Element::pb_loads(data));
}

void BeamVariable::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), beam_variable_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

std::string BeamVariable::str() const {

    std::ostringstream os;
    os << "BeamVariable(name=" << name << ", sections=" << sections.size() << ", faces=" << geometry_mesh().number_of_faces() << ", is_visible=" << std::boolalpha << is_visible << ", is_locked=" << is_locked << ")";

    return os.str();
}

}  // namespace wood_session
