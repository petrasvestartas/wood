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

    for (const wood_proto::InteractionFeatureSolid& cut : proto.solid_features())
        beam->solid_features.push_back(InteractionFeatureSolid::pb_loads(cut.SerializeAsString()));

    for (const wood_proto::InteractionFeaturePlane& feature : proto.plane_features())
        beam->plane_features.push_back(InteractionFeaturePlane::pb_loads(feature.SerializeAsString()));

    return beam;
}

// ═══════════════════════════════════════════════════════════════════════════
// Outlines
// ═══════════════════════════════════════════════════════════════════════════

/// The outline of the face of side k of four-cornered sections: corner k at every station along the beam, corner k + 1 back, both edges cut by every plane keeping the side its normal points to, closed; empty for other sections.
static Polyline side_outline(const std::vector<Polyline>& sections, size_t k, const std::vector<Plane>& planes) {

    if (sections.size() < 2)
        return Polyline();

    std::vector<Point> along;
    std::vector<Point> back;

    for (const Polyline& section : sections) {
        if (section.point_count() != 5)
            return Polyline();

        along.push_back(section.get_point(k));
        back.push_back(section.get_point((k + 1) % 4));
    }

    Polyline first(along);
    Polyline second(back);

    for (const Plane& plane : planes) {
        first = first.cut_by_plane(plane, true);
        second = second.cut_by_plane(plane, true);
    }

    std::vector<Point> loop = first.get_points();
    const std::vector<Point> returning = second.get_points();

    loop.insert(loop.end(), returning.rbegin(), returning.rend());

    return loop.size() < 3 ? Polyline() : Polyline(loop).closed();
}

/// The height of side k of a four-cornered section: the sum of its two corners' z.
static double side_height(const Polyline& section, size_t k) {
    return section.get_point(k)[2] + section.get_point((k + 1) % 4)[2];
}

/// The side of the first section highest in z, by the midpoint of its two corners.
static size_t top_side(const std::vector<Polyline>& sections) {

    if (sections.empty() || sections[0].point_count() != 5)
        return 0;

    size_t top = 0;

    for (size_t k = 1; k < 4; k++)
        if (side_height(sections[0], k) > side_height(sections[0], top))
            top = k;

    return top;
}

/// The planes the beam is cut by: its own and its plane features'.
static std::vector<Plane> cutting_planes(const BeamVariable& beam) {

    std::vector<Plane> planes = beam.cuts;
    const std::vector<Plane> features = beam.feature_planes();

    planes.insert(planes.end(), features.begin(), features.end());

    return planes;
}

Polyline BeamVariable::top() const {
    return side_outline(sections, top_side(sections), cutting_planes(*this));
}

Polyline BeamVariable::bottom() const {
    return side_outline(sections, (top_side(sections) + 2) % 4, cutting_planes(*this));
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
    return cut_mesh(stock_mesh(), cuts);
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

    for (const InteractionFeatureSolid& cut : solid_features)
        beam->solid_features.push_back(cut.transformed(xform));

    for (const InteractionFeaturePlane& feature : plane_features)
        beam->plane_features.push_back(feature.transformed(xform));

    beam->set_features(transformed_features(_features, xform));
    beam->set_insertion_vectors(transformed_list(_insertion_vectors, xform));

    return beam;
}

void BeamVariable::place(const Xform& xform) {

    Element::place(xform);
    axis.transform(xform);
    sections = transformed_list(sections, xform);
    cuts = transformed_list(cuts, xform);

    for (InteractionFeatureSolid& cut : solid_features)
        cut = cut.transformed(xform);

    for (InteractionFeaturePlane& feature : plane_features)
        feature = feature.transformed(xform);

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

    std::vector<Point> stations;

    for (const Polyline& section : sections)
        stations.push_back(axis.closest_point(section.center(), false).second);

    const std::pair<Polyline, std::vector<Polyline>> trimmed = trim_to_cuts(
        Polyline(stations),
        sections,
        cutting_planes(*this)
    );

    std::vector<ElementFeature> next;

    if (trimmed.first.point_count() > 1)
        next.push_back(polyline_feature("axis", trimmed.first));
    else
        next.push_back(polyline_feature("axis", Polyline({axis.start(), axis.end()})));

    // the end sections only; the inner ones would draw lines across the merged faces
    std::vector<Polyline> ends;

    for (const Polyline& section : trimmed.second)
        if (section.point_count() > 0)
            ends.push_back(section);

    if (!ends.empty())
        next.push_back(polyline_feature("section", ends.front()));

    if (ends.size() > 1)
        next.push_back(polyline_feature("section", ends.back()));

    const Polyline outline = top();

    if (outline.point_count() > 0) {
        next.push_back(polyline_feature("top", outline));
        next.push_back(polyline_feature("bottom", bottom()));
    }

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

    for (const InteractionFeatureSolid& cut : solid_features)
        if (!proto.add_solid_features()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Invalid solid cut");

    for (const InteractionFeaturePlane& feature : plane_features)
        if (!proto.add_plane_features()->ParseFromString(feature.pb_dumps()))
            throw std::runtime_error("Invalid plane feature");

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
