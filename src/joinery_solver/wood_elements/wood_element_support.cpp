#include "pch.h"
#include "wood_serialization.h"
#include "wood_element_support.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include "element_support.pb.h"

namespace wood_session {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

Support::Support() : WoodElement("support") {}

Support::Support(const Plane& plane, const std::string& name) : WoodElement(name), plane(plane) {}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Support> Support::from_element(Element e) {

    const std::string bytes = e.element_data_dumps();
    std::shared_ptr<Support> support = std::make_shared<Support>();
    static_cast<Element&>(*support) = std::move(e);

    wood_proto::Support proto;

    if (!proto.ParseFromString(bytes) || !proto.has_plane())
        return support;

    support->plane = Plane::pb_loads(proto.plane().SerializeAsString());
    support->height = proto.height();
    support->head_plate_diameter = proto.head_plate_diameter();
    support->head_plate_thickness = proto.head_plate_thickness();
    support->head_plate_recess = proto.head_plate_recess();
    support->base_plate_size = proto.base_plate_size();
    support->base_plate_thickness = proto.base_plate_thickness();
    support->base_plate_hole_diameter = proto.base_plate_hole_diameter();
    support->base_plate_hole_spacing = proto.base_plate_hole_spacing();
    support->adjustment_nut_across_flats = proto.adjustment_nut_across_flats();
    support->adjustment_nut_top = proto.adjustment_nut_top();
    support->rod_diameter = proto.rod_diameter();
    support->coupling_nut_across_flats = proto.coupling_nut_across_flats();
    support->coupling_nut_height = proto.coupling_nut_height();
    support->screw_count = proto.screw_count();
    support->screw_diameter = proto.screw_diameter();
    support->screw_length = proto.screw_length();
    support->screw_angle = proto.screw_angle();
    support->screw_circle_diameter = proto.screw_circle_diameter();
    support->anchor_diameter = proto.anchor_diameter();
    support->anchor_embedment = proto.anchor_embedment();
    support->chord_tolerance = proto.chord_tolerance();

    return support;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

/// The point at x, y, z in the support's frame.
static Point local(const Support& support, double x, double y, double z) {
    return support.plane.origin() + support.plane.x_axis() * x + support.plane.y_axis() * y + support.plane.z_axis() * z;
}

/// The closed ring of corners at radius about the axis at level z, the first at angle start, counter-clockwise about z.
static Polyline ring(const Support& support, double cx, double cy, double radius, int corners, double start, double z) {

    std::vector<Point> points;

    for (int k = 0; k < corners; k++) {
        const double angle = start + 2.0 * M_PI * k / corners;
        points.push_back(local(support, cx + radius * std::cos(angle), cy + radius * std::sin(angle), z));
    }

    points.push_back(points.front());

    return Polyline(points);
}

/// The hexagon of a nut given across the flats, from level bottom to level top.
static std::array<Polyline, 2> hexagon(const Support& support, double across_flats, double bottom, double top) {

    const double radius = across_flats / (2.0 * std::cos(M_PI / 6.0));

    return {ring(support, 0.0, 0.0, radius, 6, 0.0, bottom), ring(support, 0.0, 0.0, radius, 6, 0.0, top)};
}

/// The base plate square and its four drillings at levels bottom and top, outer loop first.
static std::array<std::vector<Polyline>, 2> base_plate(const Support& support) {

    const double half = support.base_plate_size * 0.5;
    const double radius = support.base_plate_hole_diameter * 0.5;
    const int corners = circle_segments(radius, support.chord_tolerance);
    std::array<std::vector<Polyline>, 2> loops;

    for (size_t level = 0; level < 2; level++) {
        const double z = level == 0 ? 0.0 : support.base_plate_thickness;
        loops[level].push_back(Polyline({local(support, -half, -half, z), local(support, half, -half, z), local(support, half, half, z), local(support, -half, half, z), local(support, -half, -half, z)}));

        for (const Line& anchor : support.anchors()) {
            const Point centre = anchor.start();
            const double cx = (centre - support.plane.origin()).dot(support.plane.x_axis());
            const double cy = (centre - support.plane.origin()).dot(support.plane.y_axis());
            loops[level].push_back(ring(support, cx, cy, radius, corners, 0.0, z));
        }
    }

    return loops;
}

/// The base plate as a BRep with its anchor holes exact cylinders: the plain plate drilled along every anchor, from just above it, so each hole runs through; the plate with polygonal holes when the drilling fails.
static BRep base_plate_brep(const Support& support, const std::array<std::vector<Polyline>, 2>& plate) {

    std::vector<Drill> holes;

    for (const Line& anchor : support.anchors()) {
        const Vector down = anchor.to_direction();
        holes.push_back({Line::from_points(anchor.start() - down * 1.0, anchor.end()), support.base_plate_hole_diameter * 0.5});
    }

    const std::optional<BRep> drilled = drilled_brep(Mesh::loft({plate[0][0]}, {plate[1][0]}, true), holes);

    if (drilled)
        return *drilled;

    return brep_between_loops(plate[0], plate[1]);
}

/// The level the coupling nut starts at: under the head plate by the nut's own height.
static double coupling_level(const Support& support) {
    return support.height - support.head_plate_thickness - support.coupling_nut_height;
}

Point Support::at(double level) const {
    return local(*this, 0.0, 0.0, level);
}

Point Support::column_foot() const {
    return at(height - head_plate_recess);
}

std::vector<Line> Support::screws() const {

    const double radius = screw_circle_diameter * 0.5;
    const double tilt = screw_angle * 0.5 * M_PI / 180.0;
    std::vector<Line> lines;

    for (int i = 0; i < screw_count; i++) {
        const double angle = 2.0 * M_PI * i / screw_count;
        const Vector outward = plane.x_axis() * std::cos(angle) + plane.y_axis() * std::sin(angle);
        const Point start = at(height) + outward * radius;
        const Vector direction = outward * std::sin(tilt) + plane.z_axis() * std::cos(tilt);
        lines.push_back(Line::from_points(start, start + direction * screw_length));
    }

    return lines;
}

std::vector<Line> Support::anchors() const {

    const double half = base_plate_hole_spacing * 0.5;
    const std::array<std::array<double, 2>, 4> corners = {{{-half, -half}, {half, -half}, {half, half}, {-half, half}}};
    std::vector<Line> lines;

    for (const std::array<double, 2>& corner : corners)
        lines.push_back(Line::from_points(local(*this, corner[0], corner[1], base_plate_thickness), local(*this, corner[0], corner[1], -anchor_embedment)));

    return lines;
}

const Mesh& Support::element_geometry_mesh() const {

    if (!_element_geometry_mesh) {
        const std::array<std::vector<Polyline>, 2> plate = base_plate(*this);
        const std::array<Polyline, 2> adjustment = hexagon(*this, adjustment_nut_across_flats, base_plate_thickness, adjustment_nut_top);
        const std::array<Polyline, 2> coupling = hexagon(*this, coupling_nut_across_flats, coupling_level(*this), coupling_level(*this) + coupling_nut_height);

        Mesh mesh = Mesh::loft(plate[0], plate[1], true);
        append_mesh(mesh, Mesh::loft({adjustment[0]}, {adjustment[1]}, true));
        append_mesh(mesh, drill_mesh(Line::from_points(at(adjustment_nut_top), at(coupling_level(*this))), rod_diameter * 0.5, chord_tolerance));
        append_mesh(mesh, Mesh::loft({coupling[0]}, {coupling[1]}, true));
        append_mesh(mesh, drill_mesh(Line::from_points(at(height - head_plate_thickness), at(height)), head_plate_diameter * 0.5, chord_tolerance));
        _element_geometry_mesh = mesh;
    }

    return *_element_geometry_mesh;
}

const BRep& Support::element_geometry_brep() const {

    if (!_element_geometry_brep) {
        const std::array<std::vector<Polyline>, 2> plate = base_plate(*this);
        const std::array<Polyline, 2> adjustment = hexagon(*this, adjustment_nut_across_flats, base_plate_thickness, adjustment_nut_top);
        const std::array<Polyline, 2> coupling = hexagon(*this, coupling_nut_across_flats, coupling_level(*this), coupling_level(*this) + coupling_nut_height);

        BRep brep = base_plate_brep(*this, plate);
        append_brep(brep, brep_between_loops({adjustment[0]}, {adjustment[1]}));
        append_brep(brep, drill_brep(Line::from_points(at(adjustment_nut_top), at(coupling_level(*this))), rod_diameter * 0.5));
        append_brep(brep, brep_between_loops({coupling[0]}, {coupling[1]}));
        append_brep(brep, drill_brep(Line::from_points(at(height - head_plate_thickness), at(height)), head_plate_diameter * 0.5));
        _element_geometry_brep = brep;
    }

    return *_element_geometry_brep;
}

const Mesh& Support::model_geometry_mesh() const {
    return element_geometry_mesh();
}

const BRep& Support::model_geometry_brep() const {
    return element_geometry_brep();
}

std::vector<Plane> Support::compute_planes() const {
    return face_planes(model_geometry_mesh());
}

std::shared_ptr<Support> Support::transformed(const Xform& xform) const {

    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<Support> support = std::make_shared<Support>(*this);
    support->guid() = guid();
    support->plane = plane.transformed(xform);
    support->_element_geometry_mesh.reset();
    support->_element_geometry_brep.reset();
    support->set_features(transformed_features(_features, xform));
    support->set_insertion_vectors(transformed_list(_insertion_vectors, xform));

    return support;
}

void Support::place(const Xform& xform) {

    Element::place(xform);
    plane = plane.transformed(xform);
    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
}

void Support::compute_geometry_mesh_impl() {
    set_geometry(model_geometry_mesh());
    set_features(session_features(*this));
}

void Support::compute_geometry_brep_impl() {
    set_geometry(model_geometry_brep());
    set_features(session_features(*this));
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json Support::element_data_jsondump() const {

    wood_proto::Support proto;

    if (!proto.ParseFromString(element_data_dumps()))
        throw std::runtime_error("Failed to parse Support protobuf data");

    return json_of(proto);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string Support::element_data_dumps() const {

    wood_proto::Support proto;

    if (!proto.mutable_plane()->ParseFromString(plane.pb_dumps()))
        throw std::runtime_error("Invalid support plane");

    proto.set_height(height);
    proto.set_head_plate_diameter(head_plate_diameter);
    proto.set_head_plate_thickness(head_plate_thickness);
    proto.set_head_plate_recess(head_plate_recess);
    proto.set_base_plate_size(base_plate_size);
    proto.set_base_plate_thickness(base_plate_thickness);
    proto.set_base_plate_hole_diameter(base_plate_hole_diameter);
    proto.set_base_plate_hole_spacing(base_plate_hole_spacing);
    proto.set_adjustment_nut_across_flats(adjustment_nut_across_flats);
    proto.set_adjustment_nut_top(adjustment_nut_top);
    proto.set_rod_diameter(rod_diameter);
    proto.set_coupling_nut_across_flats(coupling_nut_across_flats);
    proto.set_coupling_nut_height(coupling_nut_height);
    proto.set_screw_count(screw_count);
    proto.set_screw_diameter(screw_diameter);
    proto.set_screw_length(screw_length);
    proto.set_screw_angle(screw_angle);
    proto.set_screw_circle_diameter(screw_circle_diameter);
    proto.set_anchor_diameter(anchor_diameter);
    proto.set_anchor_embedment(anchor_embedment);
    proto.set_chord_tolerance(chord_tolerance);

    return proto.SerializeAsString();
}

/// The element factory of a serialized support: the protobuf bytes decoded as an Element and promoted to a Support.
static std::shared_ptr<Element> support_from_protobuf(const std::string& data) {
    return Support::from_element(Element::pb_loads(data));
}

void Support::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), support_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

std::string Support::str() const {

    std::ostringstream os;
    os << "Support(name=" << name << ", height=" << height << ", head_plate_diameter=" << head_plate_diameter << ", is_visible=" << std::boolalpha << is_visible << ", is_locked=" << is_locked << ")";

    return os.str();
}

std::optional<Plane> Support::base_plane() const {
    return plane;
}

}  // namespace wood_session
