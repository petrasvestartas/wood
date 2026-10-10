#include "pch.h"
#include "wood_session.h"
#include "wood_serialization.h"
#include "element_joint.pb.h"

namespace wood_session {

using namespace session_cpp;

Joint::Joint() : Element("Joint") {
    is_visible = false;
}
Joint::Joint(const std::vector<Polyline>& loops, const std::string& name) : Element(name), loops(loops) {
    is_visible = false;
}
Joint::Joint(const Plane& cutter) : Element("JointCutter"), cuts{cutter} {
    is_visible = false;
}
Joint::Joint(const Mesh& cutter, SolidOperation op) : Element(cutter, "JointSolidCutter"), operation(op) {
    is_visible = false;
    if (!cutter.is_closed() || !cutter.is_valid())
        throw std::invalid_argument("A solid cutter needs a valid closed mesh");
}

Joint::Joint(const Polyline& profile, const Vector& direction)
    : Joint(std::vector<Polyline>{profile}, direction) {
}

Joint::Joint(const std::vector<Polyline>& profile, const Vector& direction, SolidOperation op)
    : Element("JointProfileCutter"), cutter_profile(profile), cutter_extrusion(direction), operation(op) {
    is_visible = false;
    if (profile.empty() || direction.magnitude_squared() < 1e-12)
        throw std::invalid_argument("A profile cutter needs closed planar rings and an extrusion direction");
    const Vector normal = direction.normalized();
    for (const Polyline& ring : profile) {
        if (!ring.is_closed() || ring.point_count() < 4)
            throw std::invalid_argument("Open cutter profile");
        for (const Point& point : ring.get_points())
            if (std::abs(normal.dot(point - profile[0][0])) > 1e-7)
                throw std::invalid_argument("Cutter profiles must be coplanar and perpendicular to the extrusion");
        loops.push_back(ring);
        loops.push_back(ring.translated(direction));
    }
}

std::shared_ptr<Joint> Joint::drill(const Line& axis, double radius, double tolerance) {
    circle_segments(radius, tolerance);
    if (axis.length() <= 0)
        throw std::invalid_argument("Drill axis must have positive length");
    const std::shared_ptr<Joint> result = std::make_shared<Joint>();
    result->name = "JointDrill";
    result->line_radius = radius;
    result->chord_tolerance = tolerance;
    result->drill_lines.push_back(axis);
    return result;
}
std::vector<Line> Joint::drill_axes() const {
    return drill_lines;
}

/// The closed circle of the head plate at a level of the support.
static Polyline head_circle(const Support& support, double level) {

    const double radius = support.head_plate_diameter * 0.5;
    const int corners = circle_segments(radius, support.chord_tolerance);
    std::vector<Point> points;

    for (int k = 0; k < corners; k++) {
        const double angle = 2.0 * M_PI * k / corners;
        points.push_back(support.at(level) + support.plane.x_axis() * (radius * std::cos(angle)) + support.plane.y_axis() * (radius * std::sin(angle)));
    }

    points.push_back(points.front());

    return Polyline(points);
}

std::shared_ptr<Joint> Joint::support(const Support& support, const Column& column) {

    const double bottom = support.height - support.head_plate_thickness - support.head_plate_recess;
    const std::shared_ptr<Joint> joint = std::make_shared<Joint>(std::vector<Polyline>{head_circle(support, bottom), head_circle(support, support.height)}, "support");

    for (const Line& pin : support.pins()) {
        const Vector back = pin.to_vector().normalized() * support.head_plate_thickness;
        joint->drill_lines.push_back(Line::from_points(pin.start() - back, pin.end()));
    }

    joint->line_radius = support.pin_diameter * 0.5;
    joint->chord_tolerance = support.chord_tolerance;
    joint->targets = {column.guid()};

    return joint;
}

std::shared_ptr<Interaction> Joint::interaction(size_t) const {

    // only cutting planes: the target takes the planes themselves, the edge keeps an empty record
    if (loops.empty() && drill_axes().empty() && !cuts.empty())
        return std::make_shared<InteractionFeaturePlateBeam>();

    // its solid, or its body with its cuts for a drilling joint, its drills, profile and operation, in the joint's frame
    const std::shared_ptr<InteractionFeatureSolid> cut = std::make_shared<InteractionFeatureSolid>();
    cut->mesh = drill_axes().empty() ? model_geometry_mesh() : cut_mesh(body_mesh(), cuts);
    cut->drills = drill_axes();
    cut->drill_radius = line_radius;
    cut->drill_tolerance = chord_tolerance;
    cut->profile = cutter_profile;
    cut->extrusion = cutter_extrusion;
    cut->operation = operation;

    return cut;
}

std::vector<std::array<Polyline, 2>> Joint::bodies() const {
    return {};
}

Mesh Joint::key_mesh() const {
    return Mesh();
}

Mesh Joint::body_mesh() const {

    if (loops.size() >= 2 && loops.size() % 2 == 0) {
        std::vector<Polyline> bottom{loops[0]};
        std::vector<Polyline> top{loops[1]};

        for (size_t i = 2; i < loops.size(); i += 2) {
            bottom.push_back(loops[i]);
            top.push_back(loops[i + 1]);
        }

        return Mesh::loft(bottom, top, true);
    }

    // a key cut from the plates is the whole of the joint's own solid, the pieces its loops make included
    Mesh mesh = key_mesh();
    if (mesh.number_of_faces() > 0)
        return mesh;

    for (const std::array<Polyline, 2>& body : bodies())
        append_mesh(mesh, Mesh::loft({body[0]}, {body[1]}, true));

    if (mesh.number_of_faces() == 0 && element_type_name() == "Joint" && drill_axes().empty() && _geometry_mesh)
        return *_geometry_mesh;

    return mesh;
}

const Mesh& Joint::element_geometry_mesh() const {

    if (!mesh_) {
        mesh_ = body_mesh();

        // a pin through a key is one solid with it; a joint's other pins stand on their own
        const bool keyed = key_mesh().number_of_faces() > 0;
        for (const Line& axis : drill_axes()) {
            const Mesh pin = drill_mesh(axis, line_radius, chord_tolerance);
            if (keyed)
                mesh_ = solid_boolean(*mesh_, pin, SolidOperation::add);
            else
                append_mesh(*mesh_, pin);
        }
    }

    return *mesh_;
}

const BRep& Joint::element_geometry_brep() const {
    if (!brep_) {
        if (loops.size() >= 2 && loops.size() % 2 == 0) {
            std::vector<Polyline> bottom{loops[0]}, top{loops[1]};
            for (size_t i = 2; i < loops.size(); i += 2) {
                bottom.push_back(loops[i]);
                top.push_back(loops[i + 1]);
            }
            brep_ = brep_between_loops(bottom, top);
        } else {
            const Mesh key = key_mesh();
            const std::vector<std::array<Polyline, 2>> parts = key.number_of_faces() > 0 ? std::vector<std::array<Polyline, 2>>() : bodies();
            brep_ = BRep();
            for (const std::array<Polyline, 2>& body : parts)
                append_brep(*brep_, brep_between_loops({body[0]}, {body[1]}));
            if (key.number_of_faces() > 0)
                brep_ = mesh_brep(key);
            else if (parts.empty() && element_type_name() == "Joint" && drill_axes().empty() && _geometry_brep)
                brep_ = *_geometry_brep;
            else if (parts.empty() && element_type_name() == "Joint" && drill_axes().empty() && _geometry_mesh)
                brep_ = BRep::from_polylines(_geometry_mesh->face_outlines());
        }
        for (const Line& axis : drill_axes())
            append_brep(*brep_, drill_brep(axis, line_radius));
    }
    return *brep_;
}

const Mesh& Joint::model_geometry_mesh() const {
    if (!model_mesh_)
        model_mesh_ = cut_mesh(element_geometry_mesh(), cuts);
    return *model_mesh_;
}
const BRep& Joint::model_geometry_brep() const {
    if (!model_brep_)
        model_brep_ = !cuts.empty() && !drill_axes().empty()
                          ? mesh_brep(model_geometry_mesh())
                          : cut_brep(element_geometry_brep(), cuts);
    return *model_brep_;
}
std::vector<Plane> Joint::compute_planes() const {
    return face_planes(model_geometry_mesh());
}
void Joint::invalidate_geometry() {
    mesh_.reset();
    brep_.reset();
    model_mesh_.reset();
    model_brep_.reset();
    Element::invalidate_geometry();
    reset();
}
void Joint::compute_geometry_mesh_impl() {
    set_geometry(model_geometry_mesh());
}
void Joint::compute_geometry_brep_impl() {
    set_geometry(model_geometry_brep());
}
void Joint::place(const Xform& xform) {
    if (!drill_axes().empty()) {
        const Vector x = Vector(1, 0, 0).transformed(xform), y = Vector(0, 1, 0).transformed(xform), z = Vector(0, 0, 1).transformed(xform);
        const double scale = x.magnitude();
        if (scale <= 0 || std::abs(y.magnitude() - scale) > scale * 1e-9 || std::abs(z.magnitude() - scale) > scale * 1e-9 || std::abs(x.dot(y)) > scale * scale * 1e-9 || std::abs(x.dot(z)) > scale * scale * 1e-9 || std::abs(y.dot(z)) > scale * scale * 1e-9)
            throw std::invalid_argument("Cylindrical drills require a rigid or uniform-scale transform");
        line_radius *= scale;
        chord_tolerance *= scale;
    }
    Element::place(xform);
    loops = transformed_list(loops, xform);
    cuts = transformed_list(cuts, xform);
    cutter_profile = transformed_list(cutter_profile, xform);
    cutter_extrusion = cutter_extrusion.transformed(xform);
    for (Line& axis : drill_lines)
        axis = axis.transformed(xform);
    invalidate_geometry();
}
std::shared_ptr<Joint> Joint::transformed(const Xform& xform) const {
    if (is_mirror(xform))
        return nullptr;
    const std::shared_ptr<Joint> result = std::dynamic_pointer_cast<Joint>(clone());
    result->guid() = guid();
    result->place(xform);
    return result;
}
AABB Joint::aabb(double inflate) const {
    return AABB::from_mesh(model_geometry_mesh(), inflate);
}
std::string Joint::str() const {
    return fmt::format("{}(name={}, targets={}, is_visible={}, is_locked={})", element_type_name(), name, targets.size(), is_visible, is_locked);
}

std::string Joint::element_data_dumps() const {

    wood_proto::Joint proto;
    write_proto(proto);

    return proto.SerializeAsString();
}

void Joint::write_proto(wood_proto::Joint& proto) const {

    proto.set_kind(element_type_name());
    proto.set_line_radius(line_radius);
    proto.set_generated(generated);
    proto.set_chord_tolerance(chord_tolerance);
    proto.set_operation(static_cast<int>(operation));
    if (!proto.mutable_cutter_extrusion()->ParseFromString(cutter_extrusion.pb_dumps()))
        throw std::runtime_error("Invalid cutter extrusion");
    for (const Polyline& ring : cutter_profile)
        if (!proto.add_cutter_profile()->ParseFromString(ring.pb_dumps()))
            throw std::runtime_error("Invalid cutter profile");
    for (const Line& axis : drill_lines)
        if (!proto.add_drill_lines()->ParseFromString(axis.pb_dumps()))
            throw std::runtime_error("Invalid drill axis");
    for (const Polyline& loop : loops)
        if (!proto.add_loops()->ParseFromString(loop.pb_dumps()))
            throw std::runtime_error("Invalid joint loop");
    for (const Plane& cut : cuts)
        if (!proto.add_cuts()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Invalid joint cut");
    for (const std::string& target : targets)
        proto.add_targets(target);
}

nlohmann::ordered_json Joint::element_data_jsondump() const {
    wood_proto::Joint proto;
    if (!proto.ParseFromString(element_data_dumps()))
        throw std::runtime_error("Invalid joint data");
    return json_of(proto);
}
std::shared_ptr<Joint> Joint::from_element(Element element) {
    wood_proto::Joint proto;
    if (!proto.ParseFromString(element.element_data_dumps()))
        throw std::runtime_error("Invalid Joint protobuf data");
    std::shared_ptr<Joint> result;
    if (proto.kind() == "JointAnnen")
        result = std::make_shared<JointAnnen>();
    else if (proto.kind() == "JointVidy")
        result = std::make_shared<JointVidy>();
    else if (proto.kind() == "JointPlate")
        result = std::make_shared<JointPlate>();
    else if (proto.kind() == "JointBeam")
        result = std::make_shared<JointBeam>();
    else if (proto.kind() == "ConnectorPart")
        result = std::make_shared<ConnectorPart>();
    else if (proto.kind() == "Pin" || proto.kind() == "Pin")
        result = std::make_shared<Pin>();
    else
        result = std::make_shared<Joint>();
    static_cast<Element&>(*result) = std::move(element);
    result->read_proto(proto);
    return result;
}
void Joint::read_proto(const wood_proto::Joint& proto) {

    for (const session_proto::Polyline& loop : proto.loops())
        loops.push_back(Polyline::pb_loads(loop.SerializeAsString()));
    for (const session_proto::Plane& cut : proto.cuts())
        cuts.push_back(Plane::pb_loads(cut.SerializeAsString()));
    targets.assign(proto.targets().begin(), proto.targets().end());
    line_radius = proto.line_radius() > 0 ? proto.line_radius() : 1.0;
    generated = proto.generated();
    chord_tolerance = proto.chord_tolerance() > 0 ? proto.chord_tolerance() : 0.05;
    if (proto.has_operation()) {
        if (proto.operation() < 0 || proto.operation() > 2)
            throw std::runtime_error("Invalid solid operation");
        operation = static_cast<SolidOperation>(proto.operation());
    }
    if (proto.has_cutter_extrusion())
        cutter_extrusion = Vector::pb_loads(proto.cutter_extrusion().SerializeAsString());
    for (const session_proto::Polyline& ring : proto.cutter_profile())
        cutter_profile.push_back(Polyline::pb_loads(ring.SerializeAsString()));
    for (const session_proto::Line& axis : proto.drill_lines())
        drill_lines.push_back(Line::pb_loads(axis.SerializeAsString()));
}

void Joint::register_type() {
    for (const std::string kind : {"Joint", "JointPlate", "JointBeam", "JointAnnen", "JointVidy", "JointElement", "ConnectorPart", "Pin", "Pin"})
        Element::register_type(kind, [](const std::string& data) { return Joint::from_element(Element::pb_loads(data)); });
}

int circle_segments(double radius, double tolerance) {
    if (!std::isfinite(radius) || radius <= 0 || !std::isfinite(tolerance) || tolerance <= 0)
        throw std::invalid_argument("Drill radius and chord tolerance must be positive and finite");
    const double angle = 2 * std::asin(std::sqrt(std::min(1.0, tolerance / (2 * radius))));
    const double count = std::ceil(std::acos(-1.0) / angle);
    if (count > 65536)
        throw std::invalid_argument("Drill tolerance requires more than 65536 segments");
    return std::max(8, static_cast<int>(count));
}

Mesh drill_mesh(const Line& axis, double radius, double tolerance) {
    if (axis.length() <= 0)
        throw std::invalid_argument("Drill axis must have positive length");
    const int n = circle_segments(radius, tolerance);
    const Plane plane = Plane::from_point_normal(axis.start(), axis.to_vector());
    std::vector<Point> ring;
    for (int k = 0; k < n; ++k) {
        const double angle = 2 * std::acos(-1.0) * k / n;
        ring.push_back(axis.start() + plane.x_axis() * (radius * std::cos(angle)) + plane.y_axis() * (radius * std::sin(angle)));
    }
    ring.push_back(ring.front());
    Polyline bottom(ring);
    return Mesh::loft({bottom}, {bottom.translated(axis.to_vector())});
}

BRep drill_brep(const Line& axis, double radius) {
    if (axis.length() <= 0 || radius <= 0)
        throw std::invalid_argument("Invalid drill dimensions");
    const Plane frame = Plane::from_point_normal(axis.start(), axis.to_vector());
    const Xform to_world = Xform::frame_to_world(
        axis.start(),
        frame.x_axis(),
        frame.y_axis(),
        frame.z_axis()
    );
    const BRep cylinder = BRep::create_cylinder(radius, axis.length());
    return cylinder.transformed(to_world);
}

}
