#include "pch.h"
#include "element_joint.pb.h"
#include "wood_session.h"
#include "wood_feature_detection_beam.h"

namespace wood_session {

using namespace session_cpp;

JointBeam::JointBeam() {
    name = "JointBeam";
}
JointBeam::JointBeam(const InteractionFeatureBeam& feature) : feature(feature) {
    name = "JointBeam";
}
JointBeam::JointBeam(InteractionFeatureBeam input, const std::function<void(InteractionFeatureBeam&)>& builder)
    : feature(std::move(input)) {
    if (!builder)
        throw std::invalid_argument("A custom beam joint needs a builder");
    name = "JointBeam";
    builder(feature);
}
JointBeam::JointBeam(const Beam& source, const Beam& target, const InteractionContactAxis& contact,
                     double volume_length, double cross_or_side_to_end, int flip_male) {
    name = "JointBeam";
    if (!beam_to_beam(source, target, contact, volume_length, cross_or_side_to_end, flip_male, feature))
        throw std::invalid_argument("Cannot construct joint for the given beam contact");
    targets = {source.guid(), target.guid()};
}
std::shared_ptr<JointBeam> JointBeam::from_contact(const Beam& source, const Beam& target,
                                                   const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male) {
    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    if (!beam_to_beam(source, target, contact, volume_length, cross_or_side_to_end, flip_male, joint->feature))
        return nullptr;
    joint->targets = {source.guid(), target.guid()};
    return joint;
}
// ═══════════════════════════════════════════════════════════════════════════
// Connectors
// ═══════════════════════════════════════════════════════════════════════════

/// The contact polygon's corners without the closing point and without corners on a straight line through their neighbours, compas_tf's _merge_collinear_points.
static std::vector<Point> merge_collinear(const Polyline& polygon) {

    std::vector<Point> points = polygon.get_points();

    if (polygon.is_closed())
        points.pop_back();

    for (size_t pass = 0; pass < points.size() + 1; pass++) {
        const size_t n = points.size();

        if (n < 3)
            return points;

        std::vector<Point> kept;
        bool dropped = false;

        for (size_t i = 0; i < n; i++) {
            const Vector v1 = points[i] - points[(i + n - 1) % n];
            const Vector v2 = points[(i + 1) % n] - points[i];
            const double l1 = v1.magnitude();
            const double l2 = v2.magnitude();

            if (l1 < 1e-9 || (l2 >= 1e-9 && v1.cross(v2).magnitude() / (l1 * l2) < 1e-3)) {
                dropped = true;
                continue;
            }

            kept.push_back(points[i]);
        }

        if (!dropped)
            return kept;

        points = kept;
    }

    return points;
}

/// The longest edge of the contact whose middle is not below the corners' mean height, start to end in the polygon's winding.
static Line top_edge(const std::vector<Point>& points) {

    double mean = 0.0;

    for (const Point& point : points)
        mean += point[2] / points.size();

    Line best;
    bool best_top = false;
    double best_length = -1.0;

    for (size_t i = 0; i < points.size(); i++) {
        const Point& a = points[i];
        const Point& b = points[(i + 1) % points.size()];
        const bool top = 0.5 * (a[2] + b[2]) >= mean;
        const double length = (b - a).magnitude();

        if ((top && !best_top) || (top == best_top && length > best_length)) {
            best = Line::from_points(a, b);
            best_top = top;
            best_length = length;
        }
    }

    return best;
}

/// A point of the wedge frame: origin plus x, y, z along its axes.
static Point frame_point(const Point& origin, const std::array<Vector, 3>& axes, double x, double y, double z) {
    return origin + axes[0] * x + axes[1] * y + axes[2] * z;
}

/// The closed loop of the points.
static Polyline closed_loop(std::vector<Point> points) {

    points.push_back(points.front());

    return Polyline(points);
}

/// The pocket under one slanted wedge face p0-p1: the face rectangle over the wedge length and the same rectangle pocket_depth into the member, compas_tf inclined_face_box_outlines.
static std::array<Polyline, 2> wedge_pocket(const Point& origin, const std::array<Vector, 3>& axes, const std::array<double, 2>& p0, const std::array<double, 2>& p1, const std::array<double, 2>& middle, double length, double depth) {

    double slant_y = p1[0] - p0[0];
    double slant_z = p1[1] - p0[1];
    const double slant = std::sqrt(slant_y * slant_y + slant_z * slant_z);
    slant_y /= slant;
    slant_z /= slant;

    double normal_y = -slant_z;
    double normal_z = slant_y;
    const double centre_y = 0.5 * (p0[0] + p1[0]);
    const double centre_z = 0.5 * (p0[1] + p1[1]);

    if (normal_y * (centre_y - middle[0]) + normal_z * (centre_z - middle[1]) < 0.0) {
        normal_y = -normal_y;
        normal_z = -normal_z;
        slant_y = -slant_y;
        slant_z = -slant_z;
    }

    const double hx = 0.5 * length;
    const double hy = 0.5 * slant;
    const std::array<std::array<double, 2>, 4> signs = {{{-1.0, -1.0}, {1.0, -1.0}, {1.0, 1.0}, {-1.0, 1.0}}};
    std::vector<Point> face;
    std::vector<Point> deep;

    for (const std::array<double, 2>& sign : signs) {
        const double y = centre_y + slant_y * hy * sign[1];
        const double z = centre_z + slant_z * hy * sign[1];
        face.push_back(frame_point(origin, axes, hx * sign[0], y, z));
        deep.push_back(frame_point(origin, axes, hx * sign[0], y - normal_y * depth, z - normal_z * depth));
    }

    return {closed_loop(face), closed_loop(deep)};
}

std::shared_ptr<JointBeam> JointBeam::wedge(const Element& a, const Element& b, const InteractionContactFace& contact, double length_margin, double pocket_depth, double dowel_radius, double dowel_spacing, int dowel_sides) {

    const std::array<std::array<double, 2>, 3> profile = {{{0.0, -197.0}, {-31.75593, 11.530606}, {31.75593, 11.530606}}};
    const std::array<double, 2> dowel = {80.0, -100.0};

    const std::vector<Point> points = merge_collinear(contact.polygon);
    const Line edge = top_edge(points);
    const Vector normal = compute_newell(points).normalized();
    const Vector x = edge.to_vector().normalized();
    const Vector y = (normal - x * normal.dot(x)).normalized();
    const std::array<Vector, 3> axes = {x, y, x.cross(y)};
    const Point origin = edge.center();
    const double length = std::max(edge.length() - 2.0 * length_margin, 1e-6);

    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    joint->name = "wedge";
    joint->targets = {a.guid(), b.guid()};

    std::array<std::vector<Point>, 2> ends;

    for (const std::array<double, 2>& corner : profile) {
        ends[0].push_back(frame_point(origin, axes, -0.5 * length, corner[0], corner[1]));
        ends[1].push_back(frame_point(origin, axes, 0.5 * length, corner[0], corner[1]));
    }

    joint->parts = {{closed_loop(ends[0]), closed_loop(ends[1])}};

    const int count = dowel_spacing > 0.0 ? std::max(static_cast<int>(length / dowel_spacing), 1) : 1;

    for (int i = 0; i < count; i++) {
        const double station = -0.5 * length + (i + 0.5) * (length / count);
        const Point start = frame_point(origin, axes, station, -dowel[0], dowel[1]);
        const Point end = frame_point(origin, axes, station, dowel[0], dowel[1]);
        const Vector flat = Vector(end[0] - start[0], end[1] - start[1], 0.0).normalized();
        const Point centre = start + (end - start) * 0.5;
        joint->drill_lines.push_back(Line::from_points(centre - flat * dowel[0], centre + flat * dowel[0]));
    }

    joint->line_radius = dowel_radius;
    joint->chord_tolerance = 2.0 * dowel_radius * std::pow(std::sin(M_PI / (2.0 * dowel_sides)), 2) * (1.0 + 1e-9);

    const std::array<double, 2> middle = {(profile[0][0] + profile[1][0] + profile[2][0]) / 3.0, (profile[0][1] + profile[1][1] + profile[2][1]) / 3.0};
    const std::array<std::array<Polyline, 2>, 2> pockets = {
        wedge_pocket(origin, axes, profile[0], profile[1], middle, length, pocket_depth),
        wedge_pocket(origin, axes, profile[0], profile[2], middle, length, pocket_depth),
    };
    const Point centre = Point::centroid(points);

    for (const Element* member : {&a, &b}) {
        const bool positive = (member->model_geometry_mesh().centroid() - centre).dot(normal) >= 0.0;
        joint->cutters.push_back({positive ? pockets[1] : pockets[0]});
    }

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::array<Polyline, 2>> JointBeam::bodies() const {

    if (!parts.empty())
        return parts;

    std::vector<std::array<Polyline, 2>> result;
    for (int k = 0; k < 4; k += 2)
        if (feature.volumes[k].point_count() >= 4 && feature.volumes[k + 1].point_count() >= 4)
            result.push_back({feature.volumes[k], feature.volumes[k + 1]});
    return result;
}
void JointBeam::place(const Xform& xform) {

    Joint::place(xform);

    for (Polyline& volume : feature.volumes)
        volume = volume.transformed(xform);

    for (std::array<Polyline, 2>& part : parts)
        part = {part[0].transformed(xform), part[1].transformed(xform)};

    for (std::vector<std::array<Polyline, 2>>& target : cutters)
        for (std::array<Polyline, 2>& cutter : target)
            cutter = {cutter[0].transformed(xform), cutter[1].transformed(xform)};
}

void JointBeam::write_proto(wood_proto::Joint& proto) const {

    Joint::write_proto(proto);
    if (!proto.mutable_beam_feature()->ParseFromString(feature.pb_dumps()))
        throw std::runtime_error("Invalid beam feature");

    for (const std::array<Polyline, 2>& part : parts)
        for (const Polyline& loop : part)
            if (!proto.add_parts()->ParseFromString(loop.pb_dumps()))
                throw std::runtime_error("Invalid connector part");

    for (const std::vector<std::array<Polyline, 2>>& target : cutters) {
        wood_proto::JointCutter* cutter = proto.add_cutters();

        for (const std::array<Polyline, 2>& pair : target)
            for (const Polyline& loop : pair)
                if (!cutter->add_loops()->ParseFromString(loop.pb_dumps()))
                    throw std::runtime_error("Invalid connector cutter");
    }
}

void JointBeam::read_proto(const wood_proto::Joint& proto) {

    Joint::read_proto(proto);

    for (int i = 0; i + 1 < proto.parts_size(); i += 2)
        parts.push_back({Polyline::pb_loads(proto.parts(i).SerializeAsString()), Polyline::pb_loads(proto.parts(i + 1).SerializeAsString())});

    for (const wood_proto::JointCutter& cutter : proto.cutters()) {
        cutters.push_back({});

        for (int i = 0; i + 1 < cutter.loops_size(); i += 2)
            cutters.back().push_back({Polyline::pb_loads(cutter.loops(i).SerializeAsString()), Polyline::pb_loads(cutter.loops(i + 1).SerializeAsString())});
    }

    if (!proto.has_beam_feature())
        return;

    const std::shared_ptr<InteractionFeatureBeam> feature = std::dynamic_pointer_cast<InteractionFeatureBeam>(Interaction::pb_loads(proto.beam_feature().SerializeAsString()));
    if (!feature)
        throw std::runtime_error("Invalid beam joint feature");
    this->feature = *feature;
}

}
