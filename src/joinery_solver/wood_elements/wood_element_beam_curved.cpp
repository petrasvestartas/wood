#include "pch.h"
#include "wood_element_beam_curved.h"
#include "wood_element_geometry.h"
#include "element_beam_curved.pb.h"
#include "primitives.h"

namespace wood_session {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

BeamCurved::BeamCurved() : Element("beam_curved") {}

BeamCurved::BeamCurved(const std::vector<Point>& points, const std::vector<Vector>& directions, const Polyline& section, const std::string& name)
    : Element(name), axis(NurbsCurve::create_interpolated(points)), directions(directions), section(section) {

    std::vector<double> chords{0.0};
    for (size_t k = 0; k + 1 < points.size(); k++)
        chords.push_back(chords.back() + (points[k + 1] - points[k]).magnitude());

    const double t0 = axis.domain().first;
    const double span = axis.domain().second - t0;
    for (const double chord : chords)
        parameters.push_back(chords.back() > 0.0 ? t0 + span * chord / chords.back() : t0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<BeamCurved> BeamCurved::from_element(Element e) {

    const std::string bytes = e.element_data_dumps();
    std::shared_ptr<BeamCurved> beam = std::make_shared<BeamCurved>();
    static_cast<Element&>(*beam) = std::move(e);

    wood_proto::BeamCurved proto;
    if (!proto.ParseFromString(bytes))
        return beam;

    if (proto.has_axis())
        beam->axis = NurbsCurve::pb_loads(proto.axis().SerializeAsString());

    beam->parameters.assign(proto.parameters().begin(), proto.parameters().end());
    for (const session_proto::Vector& direction : proto.directions())
        beam->directions.push_back(Vector::pb_loads(direction.SerializeAsString()));

    if (proto.has_section())
        beam->section = Polyline::pb_loads(proto.section().SerializeAsString());

    return beam;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

Plane BeamCurved::frame(double t, const Vector& up) const {

    const Vector tangent = axis.tangent_at(t).normalized();
    const Vector across = up.cross(tangent).normalized();

    return Plane(axis.point_at(t), across, tangent.cross(across));
}

std::vector<Polyline> BeamCurved::sections() const {

    std::vector<Point> profile = section.get_points();
    if (profile.size() > 1 && (profile.front() - profile.back()).magnitude() < Tolerance::ZERO_TOLERANCE)
        profile.pop_back();

    double area = 0.0;
    for (size_t i = 0; i < profile.size(); i++)
        area += profile[i][0] * profile[(i + 1) % profile.size()][1] - profile[(i + 1) % profile.size()][0] * profile[i][1];

    if (area < 0.0)
        std::reverse(profile.begin(), profile.end());

    std::vector<Polyline> placed;
    for (size_t k = 0; k < parameters.size() && k < directions.size(); k++) {
        const Plane station = frame(parameters[k], directions[k]);
        std::vector<Point> corners;
        for (size_t i = 0; i <= profile.size(); i++)
            corners.push_back(station.origin() + station.x_axis() * profile[i % profile.size()][0] + directions[k] * profile[i % profile.size()][1]);

        placed.emplace_back(corners);
    }

    return placed;
}

std::vector<NurbsCurve> BeamCurved::rails() const {

    const std::vector<Polyline> rings = sections();
    std::vector<NurbsCurve> curves;
    for (size_t i = 0; !rings.empty() && i + 1 < rings[0].point_count(); i++) {
        std::vector<Point> corners;
        for (const Polyline& ring : rings)
            corners.push_back(ring[i]);

        curves.push_back(NurbsCurve::create_interpolated(corners));
    }

    return curves;
}

const Mesh& BeamCurved::element_geometry_mesh() const {

    if (!_element_geometry_mesh)
        _element_geometry_mesh = parameters.size() < 2 ? Mesh() : sweep_sections(sections());

    return *_element_geometry_mesh;
}

/// A straight pcurve from (u0, v0) to (u1, v1).
static NurbsCurve uv_line(double u0, double v0, double u1, double v1) {
    return NurbsCurve::create(false, 1, {Point(u0, v0, 0.0), Point(u1, v1, 0.0)});
}

/// A planar cap on the rail ends at one side facing outward, its edges each running from corner i to i + 1 when forward; the start cap walks them backwards, since its outward normal is against the tangent the corners wind about.
static BRepRef cap_face(BRep& brep, const std::vector<Point>& corners, const std::vector<int>& edges, const Vector& outward, bool start) {

    const int n = static_cast<int>(corners.size());
    std::vector<Point> loop = corners;
    if (start)
        std::reverse(loop.begin(), loop.end());

    const Point origin = Point::centroid(loop);
    const Vector normal = outward.normalized();
    const Vector x = ((loop[1] - loop[0]) - normal * (loop[1] - loop[0]).dot(normal)).normalized();
    const Vector y = normal.cross(x);
    std::vector<Point> flat;
    double low_x = 1e300, low_y = 1e300, high_x = -1e300, high_y = -1e300;
    for (const Point& point : loop) {
        flat.emplace_back((point - origin).dot(x), (point - origin).dot(y), 0.0);
        low_x = std::min(low_x, flat.back()[0]);
        low_y = std::min(low_y, flat.back()[1]);
        high_x = std::max(high_x, flat.back()[0]);
        high_y = std::max(high_y, flat.back()[1]);
    }

    NurbsSurface patch(3, false, 2, 2, 2, 2);
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            patch.set_cv(i, j, origin + x * (i ? high_x : low_x) + y * (j ? high_y : low_y));

    const int surface = brep.add_surface(patch);
    const std::pair<double, double> u = patch.domain(0);
    const std::pair<double, double> v = patch.domain(1);
    std::vector<Point> uv;
    for (const Point& point : flat)
        uv.emplace_back(u.first + (u.second - u.first) * (point[0] - low_x) / (high_x - low_x), v.first + (v.second - v.first) * (point[1] - low_y) / (high_y - low_y), 0.0);

    std::vector<BRepRef> refs;
    for (int k = 0; k < n; k++) {
        const int edge = start ? edges[(2 * n - 2 - k) % n] : edges[k];
        const BRepOrientation orientation = start ? BRepOrientation::Reversed : BRepOrientation::Forward;
        const Point& from = start ? uv[(k + 1) % n] : uv[k];
        const Point& to = start ? uv[k] : uv[(k + 1) % n];
        brep.add_pcurve(edge, surface, brep.add_curve_2d(uv_line(from[0], from[1], to[0], to[1])));
        refs.push_back({edge, orientation});
    }

    return {brep.add_face(surface, {{brep.add_wire(refs), BRepOrientation::Forward}}), BRepOrientation::Forward};
}

const BRep& BeamCurved::element_geometry_brep() const {

    if (_element_geometry_brep)
        return *_element_geometry_brep;

    const std::vector<NurbsCurve> curves = rails();
    const int n = static_cast<int>(curves.size());
    BRep brep;
    brep.name = name;
    std::vector<int> starts, ends, long_edges, start_edges, end_edges;
    std::vector<Point> start_points, end_points;
    for (const NurbsCurve& rail : curves) {
        start_points.push_back(rail.point_at_start());
        end_points.push_back(rail.point_at_end());
        starts.push_back(brep.add_vertex(start_points.back()));
        ends.push_back(brep.add_vertex(end_points.back()));
        long_edges.push_back(brep.add_edge(brep.add_curve_3d(rail), starts.back(), ends.back()));
    }

    for (int i = 0; i < n; i++) {
        start_edges.push_back(brep.add_edge(brep.add_curve_3d(NurbsCurve::create(false, 1, {start_points[i], start_points[(i + 1) % n]})), starts[i], starts[(i + 1) % n]));
        end_edges.push_back(brep.add_edge(brep.add_curve_3d(NurbsCurve::create(false, 1, {end_points[i], end_points[(i + 1) % n]})), ends[i], ends[(i + 1) % n]));
    }

    std::vector<BRepRef> faces;
    for (int i = 0; n >= 3 && i < n; i++) {
        const int next = (i + 1) % n;
        const int surface = brep.add_surface(Primitives::create_ruled(curves[next], curves[i]));
        const std::pair<double, double> u = brep.m_surfaces[surface].domain(0);
        const std::pair<double, double> v = brep.m_surfaces[surface].domain(1);
        brep.add_pcurve(long_edges[next], surface, brep.add_curve_2d(uv_line(u.first, v.first, u.second, v.first)));
        brep.add_pcurve(end_edges[i], surface, brep.add_curve_2d(uv_line(u.second, v.second, u.second, v.first)));
        brep.add_pcurve(long_edges[i], surface, brep.add_curve_2d(uv_line(u.first, v.second, u.second, v.second)));
        brep.add_pcurve(start_edges[i], surface, brep.add_curve_2d(uv_line(u.first, v.second, u.first, v.first)));
        const int wire = brep.add_wire({{long_edges[next], BRepOrientation::Forward}, {end_edges[i], BRepOrientation::Reversed}, {long_edges[i], BRepOrientation::Reversed}, {start_edges[i], BRepOrientation::Forward}});
        faces.push_back({brep.add_face(surface, {{wire, BRepOrientation::Forward}}), BRepOrientation::Forward});
    }

    if (n >= 3) {
        faces.push_back(cap_face(brep, start_points, start_edges, -axis.tangent_at(axis.domain().first), true));
        faces.push_back(cap_face(brep, end_points, end_edges, axis.tangent_at(axis.domain().second), false));
        brep.add_solid({{brep.add_shell(faces), BRepOrientation::Forward}});
    }

    _element_geometry_brep = n >= 3 ? brep : BRep();

    return *_element_geometry_brep;
}

std::vector<Plane> BeamCurved::compute_planes() const {
    return face_planes(model_geometry_mesh());
}

void BeamCurved::invalidate_geometry() {
    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    Element::invalidate_geometry();
}

void BeamCurved::place(const Xform& xform) {

    Element::place(xform);
    axis = axis.transformed(xform);
    directions = transformed_list(directions, xform);
    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
}

void BeamCurved::compute_geometry_mesh_impl() {

    if (parameters.size() >= 2)
        set_geometry(model_geometry_mesh());

    compute_geometry_features();
}

void BeamCurved::compute_geometry_brep_impl() {

    if (parameters.size() >= 2)
        set_geometry(model_geometry_brep());

    compute_geometry_features();
}

void BeamCurved::compute_geometry_features() {

    std::vector<Point> points;
    for (const double t : parameters)
        points.push_back(axis.point_at(t));

    std::vector<ElementFeature> next;
    next.push_back(polyline_feature("axis", Polyline(points)));
    const std::vector<Polyline> rings = sections();
    if (!rings.empty())
        next.push_back(polyline_feature("section", rings.front()));

    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string BeamCurved::element_data_dumps() const {

    wood_proto::BeamCurved proto;
    if (!proto.mutable_axis()->ParseFromString(axis.pb_dumps()))
        throw std::runtime_error("Failed to parse NurbsCurve protobuf data");

    for (const double t : parameters)
        proto.add_parameters(t);

    for (const Vector& direction : directions)
        if (!proto.add_directions()->ParseFromString(direction.pb_dumps()))
            throw std::runtime_error("Failed to parse Vector protobuf data");

    if (section.point_count() > 0 && !proto.mutable_section()->ParseFromString(section.pb_dumps()))
        throw std::runtime_error("Failed to parse Polyline protobuf data");

    return proto.SerializeAsString();
}

/// The element factory of a serialized curved beam: the protobuf bytes decoded as an Element and promoted to a BeamCurved.
static std::shared_ptr<Element> beam_curved_from_protobuf(const std::string& data) {
    return BeamCurved::from_element(Element::pb_loads(data));
}

void BeamCurved::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), beam_curved_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

std::string BeamCurved::str() const {
    return fmt::format("BeamCurved(name={}, stations={}, section_pts={})", name, parameters.size(), section.point_count());
}

} // namespace wood_session
