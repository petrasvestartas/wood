#include "pch.h"
#include "element_joint.pb.h"
#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_feature_detection_beam.h"
#include "wood_brep_drill.h"
#include "wood_element_pin.h"
#include "wood_element_connector_part.h"
#include "../src/clipper2/clipper.h"

namespace wood_session {

using namespace session_cpp;

JointBeam::JointBeam() {
    name = "JointBeam";
}

JointBeam::JointBeam(const InteractionFeatureBeam& feature) : feature(feature) {
    name = "JointBeam";
}

JointBeam::JointBeam(InteractionFeatureBeam input, const std::function<void(InteractionFeatureBeam&)>& builder) : feature(std::move(input)) {

    if (!builder)
        throw std::invalid_argument("A custom beam joint needs a builder");

    name = "JointBeam";
    builder(feature);
}

JointBeam::JointBeam(
    const Beam& source,
    const Beam& target,
    const InteractionContactAxis& contact,
    double volume_length,
    double cross_or_side_to_end,
    int flip_male
) {

    name = "JointBeam";

    if (!beam_to_beam(
        source,
        target,
        contact,
        volume_length,
        cross_or_side_to_end,
        flip_male,
        feature
    ))
        throw std::invalid_argument("Cannot construct joint for the given beam contact");

    targets = {source.guid(), target.guid()};
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

/// The plate joint 2024 built on a beam pair: the two boxes the volumes make, [0] to [1] the first beam's and [2] to [3] the second's, solved as
/// plates with the settings' rows, an end-to-end pair searched as a crossing; inside the zone of both boxes each beam then keeps what its box
/// keeps of the joint, its tenons with it, and loses the rest: the other box and its own mortises, with what the design mills as a solid
/// and its drills. Beams meeting end to end both run on through the zone, their extensions: each loses only what the design mills from it
/// and the other box past the joint.
static void compute_joinery(JointBeam& joint, const Settings& settings, bool end_to_end) {

    WoodSession boxes("beam_joint_boxes");
    boxes.settings = settings;
    boxes.settings.search_type = joint.feature.end_type == 2 ? SearchType::cross_joint : SearchType::face_to_face;
    const std::array<std::shared_ptr<Plate>, 2> box = {
        std::make_shared<Plate>(joint.feature.volumes[0], joint.feature.volumes[1], "box_0"),
        std::make_shared<Plate>(joint.feature.volumes[2], joint.feature.volumes[3], "box_1"),
    };
    boxes.add(box[0]);
    boxes.add(box[1]);

    const std::vector<InteractionFeaturePlate> joints = boxes.compute_features();
    if (joints.empty())
        return;

    joint.joinery = joints.front();

    // the joint's zone, both boxes: inside it each beam keeps what its box keeps and loses the rest
    const Mesh zone = solid_boolean(box[0]->element_geometry_mesh(), box[1]->element_geometry_mesh(), SolidOperation::add);
    for (size_t i = 0; i < 2; i++)
        joint.joinery_sides[i] = joint.joinery.element_a == box[i]->guid() ? 0 : 1;

    // end to end, a design that only redraws the seam (jupiter_scarf): each beam loses the prism beyond its side's seam line, toward the other box
    if (end_to_end && joint.joinery.name == "jupiter_scarf") {
        for (size_t i = 0; i < 2; i++) {
            const int side = joint.joinery_sides[i];
            const std::array<std::vector<Polyline>, 2>& outlines = side == 0 ? joint.joinery.male_outlines : joint.joinery.female_outlines;
            const Polyline& bottom = outlines[0][0];
            const Polyline& top = outlines[1][0];
            const Point first = bottom.get_point(0);
            const Point last = bottom.get_point(bottom.point_count() - 1);
            Vector beyond = (top.get_point(0) - first).cross(last - first).normalized();
            if (beyond.dot(box[1 - i]->polylines[0].center() - first) < 0.0)
                beyond = beyond * -1.0;
            beyond = beyond * (4.0 * (last - first).magnitude());
            std::array<Polyline, 2> faces;
            for (size_t f = 0; f < 2; f++) {
                const Polyline& line = f == 0 ? bottom : top;
                std::vector<Point> points = line.get_points();
                points.push_back(points.back() + beyond);
                points.push_back(points.front() + beyond);
                points.push_back(points.front());
                faces[f] = Polyline(points);
            }
            joint.member_cuts[i] = Mesh::loft({faces[0]}, {faces[1]}, true);
        }
        return;
    }

    // end to end: what both sides mill, and each beam's cut its own pieces and the other box past them, side by side for the boolean to unite
    if (end_to_end) {
        std::array<std::vector<Mesh>, 2> pieces = {JointPlate::side_solids(joint.joinery, 0), JointPlate::side_solids(joint.joinery, 1)};
        std::vector<Mesh> both = pieces[0];
        both.insert(both.end(), pieces[1].begin(), pieces[1].end());
        for (size_t i = 0; i < 2; i++) {
            std::vector<Mesh> cut = pieces[joint.joinery_sides[i]];
            cut.push_back(solid_difference(box[1 - i]->element_geometry_mesh(), both));
            joint.member_cuts[i] = shells_side_by_side(cut);
            joint.member_drills[i] = JointPlate::side_drills(joint.joinery, joint.joinery_sides[i]);
        }
        return;
    }

    // per box, the zone outside the loft of its outer outline, its tenons kept, and its pockets: every hole's prism (its mortises) and
    // what the design mills from it as a solid (the conic cross cuts, the milled key pockets), which no outline merge carries
    std::array<Mesh, 2> outside;
    std::array<std::vector<Mesh>, 2> pockets;
    for (size_t i = 0; i < 2; i++) {
        const Features& merged = box[i]->features;
        if (!merged.top.empty() && merged.top.size() == merged.bottom.size()) {
            outside[i] = solid_difference(zone, {Mesh::loft({merged.bottom[0]}, {merged.top[0]}, true)});
            for (size_t k = 1; k < merged.top.size(); k++)
                pockets[i].push_back(Mesh::loft({merged.bottom[k]}, {merged.top[k]}, true));
        }
        for (const Mesh& piece : JointPlate::side_solids(joint.joinery, joint.joinery_sides[i]))
            pockets[i].push_back(piece);
    }

    for (size_t i = 0; i < 2; i++) {
        // a beam ending on another keeps, as its tenon, what the other beam's pockets take out of that beam
        if (joint.feature.end_type == 1 && joint.joinery_sides[i] == 0 && outside[i].number_of_faces() > 0 && !pockets[1 - i].empty())
            outside[i] = solid_difference(outside[i], pockets[1 - i]);

        // the outside and the pockets, one solid, as a mortise opens onto the other box and a shell beside it would share that face
        Mesh cut = outside[i];
        for (const Mesh& pocket : pockets[i])
            cut = cut.number_of_faces() == 0 ? pocket : solid_boolean(cut, pocket, SolidOperation::add);
        if (cut.number_of_faces() > 0 && compute_volume(cut) > 1e-9 * compute_volume(zone))
            joint.member_cuts[i] = cut;

        // and the design's drills on its box, bored into the beam exactly
        joint.member_drills[i] = JointPlate::side_drills(joint.joinery, joint.joinery_sides[i]);
    }
}

std::shared_ptr<JointBeam> JointBeam::from_contact(
    const Beam& source,
    const Beam& target,
    const InteractionContactAxis& contact,
    double volume_length,
    double cross_or_side_to_end,
    int flip_male,
    const Settings& settings
) {

    // the four volume rectangles, the first beam's pair then the second's
    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    if (!beam_to_beam(
        source,
        target,
        contact,
        volume_length,
        cross_or_side_to_end,
        flip_male,
        joint->feature
    ))
        return nullptr;
    joint->targets = {source.guid(), target.guid()};

    // beams meeting end to end: an end of the one segment on an end of the other, each axis direction pointing away from it
    const Line a = source.axis.get_lines()[contact.segment_a];
    const Line b = target.axis.get_lines()[contact.segment_b];
    bool end_to_end = false;
    Point shared;
    std::array<Vector, 2> away;
    for (const Line& p : {a, Line::from_points(a.end(), a.start())})
        for (const Line& q : {b, Line::from_points(b.end(), b.start())})
            if (!end_to_end && p.start().distance(q.start()) < Tolerance::APPROXIMATION) {
                end_to_end = true;
                shared = p.start();
                away = {(p.end() - p.start()).normalized(), (q.end() - q.start()).normalized()};
            }

    // at an angle, side_side_rotated_miter: each beam's extension trimmed on the miter, the plane through the shared end bisecting the axes, by a
    // block on the other beam's side of it
    if (end_to_end && std::abs(away[0].dot(away[1])) < 0.999) {
        const Vector normal = (away[0] - away[1]).normalized();
        const Vector x_axis = normal.cross(std::abs(normal.dot(Vector::z_axis())) < 0.9 ? Vector::z_axis() : Vector::x_axis()).normalized();
        const Vector y_axis = normal.cross(x_axis).normalized();
        const double reach = 4.0 * (std::max(source.radius(contact.segment_a), target.radius(contact.segment_b)) + std::max(source.extension, target.extension));
        const Polyline face = Polyline::rectangle(shared - x_axis * reach - y_axis * reach, x_axis, y_axis, 2.0 * reach, 2.0 * reach);
        for (size_t i = 0; i < 2; i++) {
            const Vector beyond = normal * (i == 0 ? -reach : reach);
            joint->member_cuts[i] = Mesh::loft({face}, {face.transformed(Xform::translation(beyond[0], beyond[1], beyond[2]))}, true);
        }
        joint->joinery.name = "side_side_rotated_miter";
        joint->joinery_sides = {0, 1};
        return joint;
    }

    // the plate joint on the boxes and the cut it leaves each beam
    compute_joinery(*joint, settings, end_to_end);

    // a beam ending on another has room for one tenon: where the design severs a beam or leaves the two overlapping inside the joint, cut the single milled
    // tenon of top_side_milled_wedge instead
    if (joint->feature.end_type == 1 && joint->joinery.joint_type == 20) {
        const std::array<const Mesh*, 2> stocks = {&source.element_geometry_mesh(), &target.element_geometry_mesh()};
        std::array<Mesh, 2> kept = {*stocks[0], *stocks[1]};
        bool fails = false;
        for (size_t i = 0; i < 2; i++) {
            if (joint->member_cuts[i].number_of_faces() == 0)
                continue;
            const Mesh taken = solid_boolean(*stocks[i], joint->member_cuts[i], SolidOperation::intersect);
            const double expected = compute_volume(*stocks[i]) - (taken.number_of_faces() == 0 ? 0.0 : compute_volume(taken));
            kept[i] = solid_difference(*stocks[i], {joint->member_cuts[i]});
            fails = fails || compute_volume(kept[i]) < expected - 1e-2 * compute_volume(*stocks[i]);
        }
        const Mesh zone = solid_boolean(
            Plate(joint->feature.volumes[0], joint->feature.volumes[1]).element_geometry_mesh(),
            Plate(joint->feature.volumes[2], joint->feature.volumes[3]).element_geometry_mesh(),
            SolidOperation::add
        );
        const Mesh before = solid_boolean(solid_boolean(*stocks[0], *stocks[1], SolidOperation::intersect), zone, SolidOperation::intersect);
        const Mesh after = solid_boolean(solid_boolean(kept[0], kept[1], SolidOperation::intersect), zone, SolidOperation::intersect);
        const double overlap_before = before.number_of_faces() == 0 ? 0.0 : compute_volume(before);
        const double overlap_after = after.number_of_faces() == 0 ? 0.0 : compute_volume(after);
        fails = fails || overlap_after > 0.1 * overlap_before;

        if (fails) {
            Settings single = settings;
            single.joint_parameters[8] = JointPlate::library_id("top_side_milled_wedge");
            *joint = JointBeam();
            beam_to_beam(source, target, contact, volume_length, cross_or_side_to_end, flip_male, joint->feature);
            joint->targets = {source.guid(), target.guid()};
            compute_joinery(*joint, single, false);
        }
    }

    return joint;
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
static Point frame_point(
    const Point& origin,
    const std::array<Vector, 3>& axes,
    double x,
    double y,
    double z
) {
    return origin + axes[0] * x + axes[1] * y + axes[2] * z;
}

/// The pin clipped flush to the members it passes through: from where its axis first enters one of them to where it last leaves one, an end that lies inside a member kept as it is; unchanged when it meets none.
static Line flush_pin(const Line& pin, const std::vector<const Element*>& members) {

    const double length = pin.length();
    const Vector d = pin.to_vector().normalized();
    double low = 1e300;
    double high = -1e300;

    for (const Element* member : members)
        for (const std::array<double, 2>& stretch : inside_stretches(member->element_geometry_mesh(), pin)) {
            low = std::min(low, stretch[0]);
            high = std::max(high, stretch[1]);
        }

    if (std::min(high, length) - std::max(low, 0.0) < 1e-9)
        return pin;

    return Line::from_points(low > 0.0 ? pin.start() + d * low : pin.start(), high < length ? pin.start() + d * high : pin.end());
}

/// The profile cut horizontally at the frame's origin: its corners at or below that level and where its sides cross it, so a profile turned with a tilted frame still ends flush with the level of the contact's top edge.
static std::vector<std::array<double, 2>> below_top(const std::array<std::array<double, 2>, 3>& profile, const std::array<Vector, 3>& axes) {

    std::vector<std::array<double, 2>> corners;

    for (size_t i = 0; i < profile.size(); i++) {
        const std::array<double, 2>& a = profile[i];
        const std::array<double, 2>& b = profile[(i + 1) % profile.size()];
        const double ha = a[0] * axes[1][2] + a[1] * axes[2][2];
        const double hb = b[0] * axes[1][2] + b[1] * axes[2][2];

        if (ha <= 0.0)
            corners.push_back(a);

        if ((ha > 0.0) != (hb > 0.0))
            corners.push_back({a[0] + (b[0] - a[0]) * (ha / (ha - hb)), a[1] + (b[1] - a[1]) * (ha / (ha - hb))});
    }

    return corners;
}

/// The pocket under one slanted wedge face p0-p1: the face rectangle over the wedge length and the same rectangle pocket_depth into the member, compas_tf inclined_face_box_outlines.
static std::array<Polyline, 2> wedge_pocket(
    const Point& origin,
    const std::array<Vector, 3>& axes,
    const std::array<double, 2>& p0,
    const std::array<double, 2>& p1,
    const std::array<double, 2>& middle,
    double length,
    double depth
) {

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
        face.push_back(
            frame_point(
                origin,
                axes,
                hx * sign[0],
                y,
                z
            )
        );
        deep.push_back(
            frame_point(
                origin,
                axes,
                hx * sign[0],
                y - normal_y * depth,
                z - normal_z * depth
            )
        );
    }

    return {Polyline(face).closed(), Polyline(deep).closed()};
}

/// A connector's name: connector_<contact name>, or the fallback when the contact has none.
static std::string connector_name(const InteractionContactFace& contact, const std::string& fallback) {
    return contact.name.empty() ? fallback : "connector_" + contact.name;
}

/// How the two members of a face contact come apart: each along the contact's normal, away from the contact toward its own body.
static std::vector<Vector> pulled_apart(const Element& a, const Element& b, const InteractionContactFace& contact) {

    const std::vector<Point> points = merge_collinear(contact.polygon);
    Vector normal = compute_newell(points).normalized();
    if ((b.element_geometry_mesh().centroid() - Point::centroid(points)).dot(normal) < 0.0)
        normal = normal * -1.0;

    return {normal * -1.0, normal};
}

/// The wedge: a prism of the profile, apex down, along the contact's top edge, cut horizontally at the edge's level, shortened by length_margin at both ends, the end nearest the end plane on that plane instead, pins every pin_spacing flush with the members and a box pocket pocket_depth deep under the wedge face on each member's side; aimed at a then b.
std::shared_ptr<JointBeam> JointBeam::wedge(
    const Element& a,
    const Element& b,
    const InteractionContactFace& contact,
    double length_margin,
    double pocket_depth,
    const std::optional<Plane>& end,
    double pin_radius,
    double pin_spacing,
    int pin_sides,
    double overshoot,
    const std::array<std::array<double, 2>, 3>& profile,
    const std::array<double, 2>& pin_offset
) {

    const std::vector<Point> points = merge_collinear(contact.polygon);
    const Line edge = top_edge(points);
    const Vector normal = compute_newell(points).normalized();
    const Vector x = edge.to_vector().normalized();
    const Vector y = (normal - x * normal.dot(x)).normalized();
    const std::array<Vector, 3> axes = {x, y, x.cross(y)};
    std::array<double, 2> stations = {-0.5 * edge.length() + length_margin, 0.5 * edge.length() - length_margin};

    if (end) {
        Point hit;
        const Point edge_centre = edge.center();
        const Line probe = Line::from_points(edge_centre, edge_centre + x);
        Intersection::line_plane(
            probe, *end, hit, false);
        const double station = (hit - edge_centre).dot(x);

        if (std::abs(station - stations[0]) < std::abs(station - stations[1]))
            stations[0] = station;
        else
            stations[1] = station;
    }

    const Point origin = edge.center() + x * (0.5 * (stations[0] + stations[1]));
    const double length = std::max(end ? stations[1] - stations[0] : edge.length() - 2.0 * length_margin, 1e-6);

    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    joint->name = connector_name(contact, "wedge");
    joint->is_visible = true;
    joint->targets = {a.guid(), b.guid()};
    joint->insertions = pulled_apart(a, b, contact);

    std::array<std::vector<Point>, 2> ends;

    for (const std::array<double, 2>& corner : below_top(profile, axes)) {
        ends[0].push_back(
            frame_point(
                origin,
                axes,
                -0.5 * length,
                corner[0],
                corner[1]
            )
        );
        ends[1].push_back(
            frame_point(
                origin,
                axes,
                0.5 * length,
                corner[0],
                corner[1]
            )
        );
    }

    joint->parts = {{Polyline(ends[0]).closed(), Polyline(ends[1]).closed()}};

    const int count = pin_spacing > 0.0 ? std::max(static_cast<int>(length / pin_spacing), 1) : 1;

    for (int i = 0; i < count; i++) {
        const double station = -0.5 * length + (i + 0.5) * (length / count);
        const Point start = frame_point(
            origin,
            axes,
            station,
            -pin_offset[0],
            pin_offset[1]
        );
        const Point end = frame_point(
            origin,
            axes,
            station,
            pin_offset[0],
            pin_offset[1]
        );
        const Vector flat = Vector(end[0] - start[0], end[1] - start[1], 0.0).normalized();
        const Point centre = start + (end - start) * 0.5;
        const Line pin = Line::from_points(centre - flat * pin_offset[0], centre + flat * pin_offset[0]);
        joint->drill_lines.push_back(flush_pin(pin, {&a, &b}));
    }

    joint->line_radius = pin_radius;
    joint->chord_tolerance = sides_tolerance(pin_radius, pin_sides);
    joint->drill_overshoot = overshoot;

    const std::array<double, 2> middle = {(profile[0][0] + profile[1][0] + profile[2][0]) / 3.0, (profile[0][1] + profile[1][1] + profile[2][1]) / 3.0};
    const std::array<std::array<Polyline, 2>, 2> pockets = {
        wedge_pocket(
            origin,
            axes,
            profile[0],
            profile[1],
            middle,
            length,
            pocket_depth
        ),
        wedge_pocket(
            origin,
            axes,
            profile[0],
            profile[2],
            middle,
            length,
            pocket_depth
        ),
    };
    const Point centre = Point::centroid(points);

    for (const Element* member : {&a, &b}) {
        const Point member_centroid = member->model_geometry_mesh().centroid();
        const bool positive = (member_centroid - centre).dot(normal) >= 0.0;
        joint->cutters.push_back({positive ? pockets[1] : pockets[0]});
    }

    return joint;
}

/// The centre of the box around the contact's top edge: the corners within a fiftieth of the contact's height of its top, or 1 mm when that is more.
static Point top_origin(const std::vector<Point>& points) {

    double top = -1e300;
    double bottom = 1e300;

    for (const Point& point : points) {
        top = std::max(top, point[2]);
        bottom = std::min(bottom, point[2]);
    }

    const double tolerance = std::max(1.0, 0.02 * (top - bottom));
    std::array<double, 3> low = {1e300, 1e300, 1e300};
    std::array<double, 3> high = {-1e300, -1e300, -1e300};

    for (const Point& point : points)
        if (top - point[2] <= tolerance)
            for (int i = 0; i < 3; i++) {
                low[i] = std::min(low[i], point[i]);
                high[i] = std::max(high[i], point[i]);
            }

    return Point(0.5 * (low[0] + high[0]), 0.5 * (low[1] + high[1]), 0.5 * (low[2] + high[2]));
}

/// The box from x0 to x1 and z0 to z1 across the width of the frame, as the loop pair at its two y faces.
static std::array<Polyline, 2> frame_box(
    const Point& origin,
    const std::array<Vector, 3>& axes,
    double x0,
    double x1,
    double width,
    double z0,
    double z1
) {

    std::array<Polyline, 2> loops;

    for (size_t side = 0; side < 2; side++) {
        const double y = side == 0 ? -0.5 * width : 0.5 * width;
        loops[side] = Polyline(
            {
                frame_point(
                    origin,
                    axes,
                    x0,
                    y,
                    z0
                ),
                frame_point(
                    origin,
                    axes,
                    x1,
                    y,
                    z0
                ),
                frame_point(
                    origin,
                    axes,
                    x1,
                    y,
                    z1
                ),
                frame_point(
                    origin,
                    axes,
                    x0,
                    y,
                    z1
                )
            }
        ).closed();
    }

    return loops;
}

/// The frame of a plate let into two members: origin on the contact's top edge, x along the horizontal contact normal toward the rib, y across, z up.
static std::pair<Point, std::array<Vector, 3>> plate_frame(const Element& rib, const InteractionContactFace& contact) {

    std::vector<Point> points = contact.polygon.get_points();

    if (contact.polygon.is_closed())
        points.pop_back();

    const Vector normal = compute_newell(points).normalized();
    const Point centre = Point::centroid(points);
    const Point toward = rib.model_geometry_mesh().centroid();
    const Vector inward(toward[0] - centre[0], toward[1] - centre[1], 0.0);
    Vector x(normal[0], normal[1], 0.0);

    if (x.magnitude() < 1e-9)
        x = inward;

    x = x.normalized();

    if (x.dot(inward) < 0.0)
        x = -x;

    const Vector z(0.0, 0.0, 1.0);

    return {top_origin(points), {x, z.cross(x).normalized(), z}};
}

/// The plate: width thick, back into the column and front into the rib along the horizontal contact normal, height down from the contact's top edge; named after the contact.
std::shared_ptr<Plate> JointBeam::let_in_plate(
    const Element& rib,
    const InteractionContactFace& contact,
    double width,
    double back,
    double front,
    double height
) {

    const auto [origin, axes] = plate_frame(rib, contact);
    const std::array<Polyline, 2> loops = frame_box(
        origin,
        axes,
        -back,
        front,
        width,
        -height,
        0.0
    );
    const std::shared_ptr<Plate> plate = std::make_shared<Plate>(loops[0], loops[1], contact.name.empty() ? "let_in_plate" : contact.name);

    return plate;
}

/// The plate let into the column and the rib: its box raised by overshoot cut out of both, two pins in the column and two in the rib, margin_x and margin_z radii in from its ends and its top and bottom, each across its member from face to face, bored through its member and the plate; aimed at the column, the rib, then the plate.
std::shared_ptr<JointBeam> JointBeam::rectangle_plate(
    const Element& column,
    const Element& rib,
    const Plate& plate,
    const InteractionContactFace& contact,
    double pin_radius,
    double margin_x,
    double margin_z,
    double overshoot,
    int pin_sides
) {

    // the plate's extent in the frame
    const auto [origin, axes] = plate_frame(rib, contact);
    std::array<double, 3> low = {1e300, 1e300, 1e300};
    std::array<double, 3> high = {-1e300, -1e300, -1e300};

    for (const Polyline& loop : {plate.polylines[0], plate.polylines[1]})
        for (const Point& point : loop.get_points())
            for (size_t i = 0; i < 3; i++) {
                low[i] = std::min(low[i], (point - origin).dot(axes[i]));
                high[i] = std::max(high[i], (point - origin).dot(axes[i]));
            }

    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    joint->name = connector_name(contact, "rectangle_plate");
    joint->is_visible = true;
    joint->targets = {column.guid(), rib.guid(), plate.guid()};
    // the column and the rib come apart across their contact, the plate lifts out of its pocket
    joint->insertions = pulled_apart(column, rib, contact);
    joint->insertions.push_back(Vector(0.0, 0.0, 1.0));

    const std::array<Polyline, 2> pocket = frame_box(
        origin,
        axes,
        low[0],
        high[0],
        high[1] - low[1],
        low[2],
        high[2] + overshoot
    );
    joint->cutters = {{pocket}, {pocket}, {}};

    // a probe far longer than any member, which flush_pin trims to the member's stretch: the pin from one face of its member to the other
    const double half = 1e5;

    // two pins in the column, two in the rib, each flush with its own member only
    for (const auto& [station, member] : std::array<std::pair<double, const Element*>, 2>{{{low[0] + margin_x * pin_radius, &column}, {high[0] - margin_x * pin_radius, &rib}}})
        for (const double level : {high[2] - margin_z * pin_radius, low[2] + margin_z * pin_radius})
            joint->drill_lines.push_back(
                flush_pin(
                    Line::from_points(
                        frame_point(
                            origin,
                            axes,
                            station,
                            -half,
                            level
                        ),
                        frame_point(
                            origin,
                            axes,
                            station,
                            half,
                            level
                        )
                    ),
                    {member}
                )
            );

    joint->line_radius = pin_radius;
    joint->chord_tolerance = sides_tolerance(pin_radius, pin_sides);
    joint->drill_overshoot = overshoot;

    return joint;
}

/// The cross section of the tie at y: from the top down to bottom, width wide across the frame z.
static Polyline tie_section(
    const Point& origin,
    const std::array<Vector, 3>& axes,
    double y,
    double top,
    double bottom,
    double width
) {
    return Polyline(
        {
            frame_point(
                origin,
                axes,
                top,
                y,
                -0.5 * width
            ),
            frame_point(
                origin,
                axes,
                bottom,
                y,
                -0.5 * width
            ),
            frame_point(
                origin,
                axes,
                bottom,
                y,
                0.5 * width
            ),
            frame_point(
                origin,
                axes,
                top,
                y,
                0.5 * width
            )
        }
    ).closed();
}

/// The key: length long along the contact normal, top below the contact's top edge, heads head_width wide over head_length at both ends and the neck neck_width wide between, depth deep at the seam deepening straight to end_depth at its ends; each member gets a flat-bottomed pocket pocket_depth deep of its head and neck boxes, the neck overshoot past the seam; aimed at a then b, the defaults the OBJ template's.
/// A member end as drawn: its face, closed, the unit direction out of it and the face's centre.
struct ButtEnd {
    Polyline face;
    Vector out;
    Point centre;
};

/// A member as drawn, before any joint: its ends, its long faces as closed loops with their outward normals, the line or the polygon it
/// was drawn on, its depth across and its uncut solid.
struct ButtMember {
    std::vector<ButtEnd> ends;
    std::vector<Polyline> sides;
    std::vector<Vector> normals;
    std::vector<Point> drawn;
    bool flat = false;
    double depth = 0.0;
    Mesh solid;
};

/// A loop's points without the closing repeat.
static std::vector<Point> loop_points(const Polyline& loop) {

    std::vector<Point> points = loop.get_points();
    if (points.size() > 1 && Point::distance(points.front(), points.back()) < 1e-9)
        points.pop_back();

    return points;
}

/// The mean of points.
static Point mean_point(const std::vector<Point>& points) {

    Vector sum(0.0, 0.0, 0.0);
    for (const Point& point : points)
        sum = sum + Vector(point[0], point[1], point[2]);
    sum = sum / (double)points.size();

    return Point(sum[0], sum[1], sum[2]);
}

/// A loop closed and its Newell normal turned away from a point inside the member.
static std::pair<Polyline, Vector> outward_loop(std::vector<Point> points, const Point& inside) {

    Vector normal = compute_newell(points).normalized();
    if (normal.dot(mean_point(points) - inside) < 0.0)
        normal = normal * -1.0;
    points.push_back(points.front());

    return {Polyline(points), normal};
}

/// A swept member, a beam or a column, from its sections along its axis: an end at each end section looking out along the axis, and a
/// long face per section edge and span.
static ButtMember swept_member(const std::vector<Polyline>& sections, const std::vector<Point>& axis, const Mesh& solid) {

    ButtMember member;
    member.solid = solid;
    member.drawn = axis;

    // the long faces, span by span, each turned away from its span's centre
    for (size_t j = 0; j + 1 < sections.size(); j++) {
        const std::vector<Point> near = loop_points(sections[j]);
        const std::vector<Point> far = loop_points(sections[j + 1]);
        std::vector<Point> both = near;
        both.insert(both.end(), far.begin(), far.end());
        const Point centre = mean_point(both);
        for (size_t k = 0; k < near.size(); k++) {
            const size_t next = (k + 1) % near.size();
            const std::pair<Polyline, Vector> side = outward_loop({near[k], near[next], far[next], far[k]}, centre);
            member.sides.push_back(side.first);
            member.normals.push_back(side.second);
        }
    }

    // the two ends, out along the first and the last span
    const std::vector<Point> first = loop_points(sections.front());
    const std::vector<Point> last = loop_points(sections.back());
    member.ends.push_back({sections.front(), (axis[0] - axis[1]).normalized(), mean_point(first)});
    member.ends.push_back({sections.back(), (axis[axis.size() - 1] - axis[axis.size() - 2]).normalized(), mean_point(last)});

    // its depth, the side of the square its first section spans
    for (const Point& a : first)
        for (const Point& b : first)
            member.depth = std::max(member.depth, Point::distance(a, b) / std::sqrt(2.0));

    return member;
}

/// A member from a beam, a column or a plate as drawn.
static ButtMember butt_member(const Element& element) {

    if (const Beam* beam = dynamic_cast<const Beam*>(&element))
        return swept_member(beam->sections(), beam->axis.get_points(), beam->element_geometry_mesh());

    if (const Column* column = dynamic_cast<const Column*>(&element))
        return swept_member({column->section, column->section.translated(column->axis.to_vector())}, {column->axis.start(), column->axis.end()}, column->element_geometry_mesh());

    const Plate* plate = dynamic_cast<const Plate*>(&element);
    if (!plate)
        throw std::invalid_argument("A butt joins beams, columns and plates");

    ButtMember member;
    member.solid = plate->element_geometry_mesh();
    member.flat = true;
    const std::vector<Point> bottom = loop_points(plate->polylines[0]);
    const std::vector<Point> top = loop_points(plate->polylines[1]);
    std::vector<Point> both = bottom;
    both.insert(both.end(), top.begin(), top.end());
    const Point centre = mean_point(both);
    member.depth = Point::distance(mean_point(bottom), mean_point(top));

    // the polygon it was drawn on, halfway between its faces
    for (size_t i = 0; i < bottom.size(); i++)
        member.drawn.push_back(bottom[i] + (top[i] - bottom[i]) * 0.5);

    // its two faces and an end at each edge, looking out in its plane
    for (const std::vector<Point>* face : {&bottom, &top}) {
        const std::pair<Polyline, Vector> side = outward_loop(*face, centre);
        member.sides.push_back(side.first);
        member.normals.push_back(side.second);
    }
    for (size_t i = 0; i < bottom.size(); i++) {
        const size_t next = (i + 1) % bottom.size();
        const std::pair<Polyline, Vector> end = outward_loop({bottom[i], bottom[next], top[next], top[i]}, centre);
        member.ends.push_back({end.first, end.second, mean_point({bottom[i], bottom[next], top[next], top[i]})});
    }

    return member;
}

/// Where an end's line, through its centre along its direction, meets the line or polygon the other member was drawn on: how far along
/// the end's direction, and how far it misses; a drawn line's ends run on by reach, so a member drawn short still meets; in line or in
/// plane, halfway to the other's end; none when the end runs parallel beside it.
static std::optional<std::array<double, 2>> meeting(const ButtEnd& end, const ButtMember& other, double reach) {

    if (other.flat) {
        const Plane plane = Plane::from_points(other.drawn);
        const double along = end.out.dot(plane.z_axis());

        // in its plane, an edge on edge: halfway to where the end's line crosses the polygon's rim nearest the end, clear of its corners, where side_side_inplane meets
        if (std::abs(along) < 0.05) {
            if (std::abs((end.centre - plane.origin()).dot(plane.z_axis())) > 0.5 * other.depth)
                return std::nullopt;
            std::optional<std::array<double, 2>> crossing;
            for (size_t i = 0; i < other.drawn.size(); i++) {
                const Point& start = other.drawn[i];
                const Vector edge = other.drawn[(i + 1) % other.drawn.size()] - start;
                const Vector normal = edge.cross(plane.z_axis());
                const double facing = end.out.dot(normal);
                if (std::abs(facing) < 1e-9)
                    continue;
                const double t = (start - end.centre).dot(normal) / facing;
                const double s = (end.centre + end.out * t - start).dot(edge) / edge.dot(edge);
                const double margin = 0.5 * other.depth / edge.magnitude();
                if (s > margin && s < 1.0 - margin && (!crossing || std::abs(t) < 2.0 * std::abs((*crossing)[0])))
                    crossing = std::array<double, 2>{0.5 * t, 0.0};
            }
            return crossing;
        }
        const double t = (plane.origin() - end.centre).dot(plane.z_axis()) / along;
        const Point point = end.centre + end.out * t;
        std::vector<Point> flat;
        for (const Point& corner : other.drawn)
            flat.emplace_back((corner - plane.origin()).dot(plane.x_axis()), (corner - plane.origin()).dot(plane.y_axis()), 0.0);
        if (Polyline(flat).point_in_polygon_2d(Point((point - plane.origin()).dot(plane.x_axis()), (point - plane.origin()).dot(plane.y_axis()), 0.0)))
            return std::array<double, 2>{t, 0.0};
        std::vector<Point> loop = other.drawn;
        loop.push_back(loop.front());
        size_t edge = 0;
        Point closest;
        return std::array<double, 2>{t, Polyline(loop).closest_distance_and_point(point, edge, closest)};
    }

    std::optional<std::array<double, 2>> best;
    for (size_t j = 0; j + 1 < other.drawn.size(); j++) {
        const Vector span = other.drawn[j + 1] - other.drawn[j];
        const double length = span.magnitude();
        const Vector along = span / length;
        const double cosine = end.out.dot(along);
        const Vector gap = other.drawn[j] - end.centre;

        // in line, an end on an end: halfway to the other's nearer end, where side_side_inplane meets
        if (1.0 - std::abs(cosine) < 1e-6) {
            const bool facing = cosine > 0.0 ? j == 0 : j + 2 == other.drawn.size();
            if (!facing)
                continue;
            const Point& tip = cosine > 0.0 ? other.drawn[j] : other.drawn[j + 1];
            const Vector across = gap - end.out * gap.dot(end.out);
            const std::array<double, 2> met = {0.5 * (tip - end.centre).dot(end.out), across.magnitude()};
            if (!best || met[1] < (*best)[1])
                best = met;
            continue;
        }
        const double t = (gap.dot(end.out) - cosine * gap.dot(along)) / (1.0 - cosine * cosine);
        const double low = j == 0 ? -reach : 0.0;
        const double high = j + 2 == other.drawn.size() ? length + reach : length;
        const double s = std::clamp((end.out * t - gap).dot(along), low, high);
        const double miss = Point::distance(end.centre + end.out * t, other.drawn[j] + along * s);
        if (!best || miss < (*best)[1])
            best = std::array<double, 2>{t, miss};
    }

    return best;
}

/// The member's end that reaches the other, its line meeting what the other was drawn on within reach along it and within half the larger
/// depth across, or within reach of a polygon's rim; the nearest when more do.
static std::optional<size_t> reaching_end(const ButtMember& member, const ButtMember& other, double reach) {

    std::optional<size_t> found;
    double nearest = std::numeric_limits<double>::max();
    for (size_t i = 0; i < member.ends.size(); i++) {
        const std::optional<std::array<double, 2>> met = meeting(member.ends[i], other, reach);
        const double across = other.flat ? reach : 0.5 * std::max(member.depth, other.depth);
        if (met && std::abs((*met)[0]) <= reach && (*met)[1] <= across && std::abs((*met)[0]) < nearest) {
            nearest = std::abs((*met)[0]);
            found = i;
        }
    }

    return found;
}

/// The member's long face an end's line crosses, turned toward the end (facing) or away from it, the one it crosses nearest its rim, then
/// nearest the end; none when it crosses none within reach.
static std::optional<Plane> crossed_side(const ButtMember& member, const ButtEnd& end, bool facing, double reach) {

    std::optional<Plane> found;
    double best_miss = std::numeric_limits<double>::max();
    double best_t = std::numeric_limits<double>::max();
    for (size_t i = 0; i < member.sides.size(); i++) {
        const Vector& normal = member.normals[i];
        const double along = end.out.dot(normal);
        if ((facing ? -along : along) < 0.05)
            continue;
        const std::vector<Point> corners = loop_points(member.sides[i]);
        const Plane plane = Plane::from_point_normal(corners[0], normal);
        const double t = (plane.origin() - end.centre).dot(normal) / along;
        const Point point = end.centre + end.out * t;
        std::vector<Point> flat;
        for (const Point& corner : corners)
            flat.emplace_back((corner - plane.origin()).dot(plane.x_axis()), (corner - plane.origin()).dot(plane.y_axis()), 0.0);
        size_t edge = 0;
        Point closest;
        const double miss = Polyline(flat).point_in_polygon_2d(Point((point - plane.origin()).dot(plane.x_axis()), (point - plane.origin()).dot(plane.y_axis()), 0.0))
            ? 0.0
            : member.sides[i].closest_distance_and_point(point, edge, closest);
        if (miss > reach)
            continue;
        if (miss < best_miss - 1e-6 || (std::abs(miss - best_miss) <= 1e-6 && std::abs(t) < best_t)) {
            best_miss = miss;
            best_t = std::abs(t);
            found = plane;
        }
    }

    return found;
}

/// What an end gains and loses to stop on a plane: its face run on along its direction past the plane, and the wood beyond the plane around
/// the end, so the end is the plane wherever it was drawn, short of it, past it or across it.
static std::array<InteractionFeatureSolid, 2> end_cut(const ButtEnd& end, const Plane& plane, double size) {

    // the plane turned to look out of the member
    const Vector normal = plane.z_axis().dot(end.out) > 0.0 ? plane.z_axis() : plane.z_axis() * -1.0;
    const std::vector<Point> corners = loop_points(end.face);
    double short_of = 0.0;
    double radius = 0.0;
    for (const Point& corner : corners) {
        short_of = std::max(short_of, (plane.origin() - corner).dot(normal) / end.out.dot(normal));
        radius = std::max(radius, Point::distance(corner, end.centre));
    }

    // the run-on, the end face swept past the plane
    std::array<InteractionFeatureSolid, 2> cut;
    if (short_of > 1e-6)
        cut[0] = InteractionFeatureSolid(Mesh::loft({end.face}, {end.face.translated(end.out * (short_of + size))}, true), SolidOperation::add);

    // the wood beyond the plane, a square box around the end on it, wider where the member meets the plane at a shallow angle and its
    // footprint there runs long
    const Plane beyond = Plane::from_point_normal(end.centre - normal * (end.centre - plane.origin()).dot(normal), normal);
    const double half = (radius + 2.0 * size) / std::max(end.out.dot(normal), 0.1);
    const Polyline square = Polyline::rectangle(beyond.origin() - beyond.x_axis() * half - beyond.y_axis() * half, beyond.x_axis(), beyond.y_axis(), 2.0 * half, 2.0 * half);
    cut[1] = InteractionFeatureSolid(Mesh::loft({square}, {square.translated(normal * (short_of + radius + 4.0 * size))}, true), SolidOperation::subtract);

    return cut;
}

/// How a butt resolves the case its members' ends make: as read from them, every corner side_side, the second member's side on the first's top,
/// or at a corner of any angle the first running on over the second.
enum class ButtMode {
    automatic,
    side_side,
    top_side,
    top_side_end,
    cross,
};

/// The butt of two members in a mode: the configuration their ends make, each reaching end's plane, and what each end gains and loses to
/// stop on it.
static std::shared_ptr<JointBeam> butt_joint(const Element& a, const Element& b, double reach, ButtMode mode) {

    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    joint->is_butt = true;
    joint->targets = {a.guid(), b.guid()};

    // both members as drawn, and the ends that reach the other
    const std::array<ButtMember, 2> members = {butt_member(a), butt_member(b)};
    const double size = std::max(members[0].depth, members[1].depth);
    const double within = reach > 0.0 ? reach : size;
    std::array<std::optional<size_t>, 2> ends = {reaching_end(members[0], members[1], within), reaching_end(members[1], members[0], within)};

    // top_side asked: only the second member's end, on the first, wherever the first's own end lies
    if (mode == ButtMode::top_side && ends[1])
        ends[0] = std::nullopt;

    // no end reaching, or a cross asked: a cross when the solids share wood, nothing when they do not touch
    if (mode == ButtMode::cross || (!ends[0] && !ends[1])) {
        if (solid_boolean(members[0].solid, members[1].solid, SolidOperation::intersect).number_of_faces() == 0)
            return nullptr;
        joint->configuration = ButtConfiguration::cross;
        joint->name = "cross";

        // two beams crossing, a half lap: on the plane halfway between their axes, each loses the other's wood on its own half
        if (members[0].flat || members[1].flat)
            return joint;
        const std::array<Vector, 2> axes = {members[0].ends[1].centre - members[0].ends[0].centre, members[1].ends[1].centre - members[1].ends[0].centre};
        const Vector across = axes[0].cross(axes[1]);
        if (across.magnitude() < 1e-6 * axes[0].magnitude() * axes[1].magnitude())
            return joint;
        const Vector normal = across.normalized();
        const Point origin = members[0].ends[0].centre + normal * (0.5 * (members[1].ends[0].centre - members[0].ends[0].centre).dot(normal));
        const Plane plane = Plane::from_point_normal(origin, normal);
        const double half = axes[0].magnitude() + axes[1].magnitude() + 4.0 * size;
        const Polyline square = Polyline::rectangle(origin - plane.x_axis() * half - plane.y_axis() * half, plane.x_axis(), plane.y_axis(), 2.0 * half, 2.0 * half);
        for (size_t i = 0; i < 2; i++) {
            const Mesh side = Mesh::loft({square}, {square.translated(normal * (i == 0 ? half : -half))}, true);
            joint->butt_solids[i] = InteractionFeatureSolid(solid_boolean(members[1 - i].solid, side, SolidOperation::intersect), SolidOperation::subtract);
        }
        return joint;
    }

    // one end: top_side, the end stopping on the other's face that looks at it
    if (!ends[0] || !ends[1]) {
        const size_t on = ends[0] ? 0 : 1;
        joint->configuration = ButtConfiguration::top_side;
        joint->butt_planes[on] = crossed_side(members[1 - on], members[on].ends[*ends[on]], true, within);
    }

    // both ends: nearly in line, both on the plane halfway, side_side_inplane when in line or in one plane, else a shallow side_side_outplane;
    // top_side at a square corner; side_side_outplane, a miter, at an oblique one
    if (ends[0] && ends[1]) {
        const ButtEnd& end_a = members[0].ends[*ends[0]];
        const ButtEnd& end_b = members[1].ends[*ends[1]];
        const double cosine = end_a.out.dot(end_b.out);
        if (cosine < -0.9) {
            const bool coplanar = members[0].flat && members[1].flat && std::abs(Plane::from_points(members[0].drawn).z_axis().dot(Plane::from_points(members[1].drawn).z_axis())) > 0.996;
            joint->configuration = cosine < -0.996 || coplanar ? ButtConfiguration::side_side_inplane : ButtConfiguration::side_side_outplane;
            const Point middle = end_a.centre + (end_b.centre - end_a.centre) * 0.5;
            const Plane halfway = Plane::from_point_normal(middle, (end_a.out - end_b.out).normalized());
            joint->butt_planes = {halfway, halfway};
        } else if (mode == ButtMode::top_side_end || (mode != ButtMode::side_side && std::abs(cosine) < std::sin(10.0 * Tolerance::TO_RADIANS))) {
            joint->configuration = ButtConfiguration::top_side;
            joint->butt_planes[0] = crossed_side(members[1], end_a, false, within);
            joint->butt_planes[1] = crossed_side(members[0], end_b, true, within);
        } else {
            joint->configuration = ButtConfiguration::side_side_outplane;
            const std::optional<Plane> outer_a = crossed_side(members[0], end_b, false, within);
            const std::optional<Plane> inner_a = crossed_side(members[0], end_b, true, within);
            const std::optional<Plane> outer_b = crossed_side(members[1], end_a, false, within);
            const std::optional<Plane> inner_b = crossed_side(members[1], end_a, true, within);
            if (outer_a && inner_a && outer_b && inner_b) {
                const std::optional<Line> outer = Intersection::plane_plane(*outer_a, *outer_b);
                const std::optional<Line> inner = Intersection::plane_plane(*inner_a, *inner_b);
                if (outer && inner) {
                    const Vector normal = outer->to_direction().cross(inner->start() - outer->start());
                    if (normal.magnitude() > 1e-9) {
                        const Plane halfway = Plane::from_point_normal(outer->start(), normal.normalized());
                        joint->butt_planes = {halfway, halfway};
                    }
                }
            }
        }
    }

    // named by its configuration
    const std::array<std::string, 5> names = {"none", "cross", "top_side", "side_side_outplane", "side_side_inplane"};
    joint->name = names[(size_t)joint->configuration];

    // each reaching end run on and cut on its plane, a plate's edge run on in its plane between its neighbours
    const std::array<const Element*, 2> elements = {&a, &b};
    for (size_t i = 0; i < 2; i++) {
        if (!ends[i] || !joint->butt_planes[i])
            continue;
        const std::array<InteractionFeatureSolid, 2> cut = end_cut(members[i].ends[*ends[i]], *joint->butt_planes[i], size);
        joint->butt_ends[i] = (int)*ends[i];
        joint->butt_extensions[i] = cut[0];
        joint->butt_solids[i] = cut[1];
        if (const Plate* plate = dynamic_cast<const Plate*>(elements[i])) {
            const std::array<Mesh, 2> edge = JointBeam::plate_end(*plate, *ends[i], {{*ends[i], *joint->butt_planes[i]}}, size);
            joint->butt_extensions[i] = InteractionFeatureSolid(edge[0], SolidOperation::add);
            joint->butt_solids[i] = InteractionFeatureSolid(edge[1], SolidOperation::subtract);
        }
    }

    return joint;
}

std::array<Mesh, 2> JointBeam::plate_end(const Plate& plate, size_t end, const std::map<size_t, Plane>& planes, double overshoot) {

    // the plate as drawn, flat in its mid-plane, an outline turned counterclockwise before it is lofted into a solid
    const ButtMember member = butt_member(plate);
    const size_t count = member.drawn.size();
    const Plane middle = Plane::from_points(member.drawn);
    const auto flat = [&middle](const Point& point) { return Point((point - middle.origin()).dot(middle.x_axis()), (point - middle.origin()).dot(middle.y_axis()), 0.0); };
    const auto prism = [&middle](std::vector<Point> outline, double half, const Vector& shear) {
        double area = 0.0;
        for (size_t k = 0; k < outline.size(); k++)
            area += outline[k][0] * outline[(k + 1) % outline.size()][1] - outline[(k + 1) % outline.size()][0] * outline[k][1];
        if (area < 0.0)
            std::reverse(outline.begin(), outline.end());
        std::vector<Point> bottom;
        std::vector<Point> top;
        for (const Point& corner : outline) {
            bottom.push_back(middle.origin() + middle.x_axis() * corner[0] + middle.y_axis() * corner[1] + (middle.z_axis() + shear) * half);
            top.push_back(middle.origin() + middle.x_axis() * corner[0] + middle.y_axis() * corner[1] - (middle.z_axis() + shear) * half);
        }
        bottom.push_back(bottom.front());
        top.push_back(top.front());
        return Mesh::loft({Polyline(bottom)}, {Polyline(top)}, true);
    };
    const auto crossing = [](const std::array<Point, 2>& first, const std::array<Point, 2>& second) -> std::optional<Point> {
        const Vector u = first[1] - first[0];
        const Vector v = second[1] - second[0];
        const double denominator = u[0] * v[1] - u[1] * v[0];
        if (std::abs(denominator) < 1e-9 * u.magnitude() * v.magnitude())
            return std::nullopt;
        const Vector gap = second[0] - first[0];
        return first[0] + u * ((gap[0] * v[1] - gap[1] * v[0]) / denominator);
    };

    // each edge's line as drawn, its plane's line when butted, how far the plane leans out per unit across the plate, and the line it runs
    // on to: the plane's line overshoot past it and past the lean when that runs out
    std::vector<std::array<Point, 2>> drawn(count);
    std::vector<std::optional<std::array<Point, 2>>> cut(count);
    std::vector<std::array<Point, 2>> bound(count);
    std::vector<Vector> outward(count);
    std::vector<Vector> slant(count, Vector(0.0, 0.0, 0.0));
    std::vector<bool> runs(count, false);
    for (size_t j = 0; j < count; j++) {
        drawn[j] = {flat(member.drawn[j]), flat(member.drawn[(j + 1) % count])};
        bound[j] = drawn[j];
        outward[j] = Vector(member.ends[j].out.dot(middle.x_axis()), member.ends[j].out.dot(middle.y_axis()), 0.0).normalized();
        if (!planes.contains(j))
            continue;
        const std::optional<Line> line = Intersection::plane_plane(planes.at(j), middle);
        if (!line)
            continue;
        cut[j] = std::array<Point, 2>{flat(line->start()), flat(line->end())};
        const Vector out = middle.x_axis() * outward[j][0] + middle.y_axis() * outward[j][1];
        slant[j] = out * (-planes.at(j).z_axis().dot(middle.z_axis()) / planes.at(j).z_axis().dot(out));
        const double past = overshoot + 0.5 * member.depth * slant[j].magnitude();
        const std::array<Point, 2> moved = {(*cut[j])[0] + outward[j] * past, (*cut[j])[1] + outward[j] * past};
        for (const Point& corner : drawn[j]) {
            const Vector along = (*cut[j])[1] - (*cut[j])[0];
            const Point foot = (*cut[j])[0] + along * ((corner - (*cut[j])[0]).dot(along) / along.dot(along));
            runs[j] = runs[j] || (foot - corner).dot(outward[j]) + 0.5 * member.depth * slant[j].magnitude() > 1e-6;
        }
        if (runs[j])
            bound[j] = moved;
    }
    std::array<Mesh, 2> result;
    if (!cut[end])
        return result;

    // the run-on, between the drawn edge and its moved line, bounded by its neighbours' lines, run out where they run out
    const size_t before = (end + count - 1) % count;
    const size_t after = (end + 1) % count;
    if (runs[end]) {
        const std::array<std::optional<Point>, 4> corners = {crossing(drawn[end], bound[before]), crossing(drawn[end], bound[after]), crossing(bound[end], bound[after]), crossing(bound[end], bound[before])};
        if (corners[0] && corners[1] && corners[2] && corners[3])
            result[0] = prism({*corners[0], *corners[1], *corners[2], *corners[3]}, 0.5 * member.depth, Vector(0.0, 0.0, 0.0));
    }

    // the wood beyond the edge's plane, a box past its line leaning with the plane; at a concave corner, a valley, only what lies beyond the butted neighbour's
    // plane too, so the valley keeps what lies under the neighbour, while at a convex corner all beyond either plane goes
    const std::array<Point, 2>& line = *cut[end];
    const Vector tangent = (line[1] - line[0]).normalized();
    const Point middle_point = drawn[end][0] + (drawn[end][1] - drawn[end][0]) * 0.5;
    const Point foot = line[0] + tangent * (middle_point - line[0]).dot(tangent);
    double reach = overshoot;
    double area = 0.0;
    for (size_t k = 0; k < count; k++) {
        reach = std::max(reach, 4.0 * Point::distance(drawn[k][0], middle_point));
        area += drawn[k][0][0] * drawn[k][1][1] - drawn[k][1][0] * drawn[k][0][1];
    }
    std::vector<Point> box = {foot - tangent * reach, foot + tangent * reach, foot + tangent * reach + outward[end] * reach, foot - tangent * reach + outward[end] * reach};
    for (const size_t neighbour : {before, after}) {
        const Vector first = neighbour == after ? drawn[end][1] - drawn[end][0] : drawn[neighbour][1] - drawn[neighbour][0];
        const Vector second = neighbour == after ? drawn[neighbour][1] - drawn[neighbour][0] : drawn[end][1] - drawn[end][0];
        const bool concave = (first[0] * second[1] - first[1] * second[0]) * area < 0.0;
        if (!cut[neighbour] || !concave)
            continue;
        const Vector along = (*cut[neighbour])[1] - (*cut[neighbour])[0];
        Vector normal = Vector(-along[1], along[0], 0.0);
        if (normal.dot(outward[neighbour]) < 0.0)
            normal = normal * -1.0;
        std::vector<Point> clipped;
        for (size_t k = 0; k < box.size(); k++) {
            const Point& p = box[k];
            const Point& q = box[(k + 1) % box.size()];
            const double sp = normal.dot(p - (*cut[neighbour])[0]);
            const double sq = normal.dot(q - (*cut[neighbour])[0]);
            if (sp >= 0.0)
                clipped.push_back(p);
            if ((sp >= 0.0) != (sq >= 0.0))
                clipped.push_back(p + (q - p) * (sp / (sp - sq)));
        }
        box = clipped;
    }
    if (box.size() > 2)
        result[1] = prism(box, 0.5 * member.depth + overshoot, slant[end]);

    return result;
}

std::shared_ptr<JointBeam> JointBeam::butt(const Element& a, const Element& b, double reach) {

    return butt_joint(a, b, reach, ButtMode::automatic);
}

std::shared_ptr<JointBeam> JointBeam::butt_side_side(const Element& a, const Element& b, double reach) {

    return butt_joint(a, b, reach, ButtMode::side_side);
}

std::shared_ptr<JointBeam> JointBeam::butt_top_side_end(const Element& top, const Element& side, double reach) {

    return butt_joint(top, side, reach, ButtMode::top_side_end);
}

std::shared_ptr<JointBeam> JointBeam::butt_top_side(const Element& top, const Element& side, double reach) {

    return butt_joint(top, side, reach, ButtMode::top_side);
}

std::shared_ptr<JointBeam> JointBeam::butt_cross(const Element& a, const Element& b) {

    return butt_joint(a, b, 0.0, ButtMode::cross);
}

std::shared_ptr<JointBeam> JointBeam::butterfly_key(
    const Element& a,
    const Element& b,
    const InteractionContactFace& contact,
    double top,
    double length,
    double head_length,
    double head_width,
    double neck_width,
    double depth,
    double end_depth,
    double pocket_depth,
    double overshoot
) {

    std::vector<Point> points = contact.polygon.get_points();

    if (contact.polygon.is_closed())
        points.pop_back();

    const Vector normal = compute_newell(points).normalized();
    const Vector x(0.0, 0.0, -1.0);
    const Vector y = Vector(normal[0], normal[1], 0.0).normalized();
    const std::array<Vector, 3> axes = {x, y, x.cross(y)};
    const Point origin = top_origin(points);
    const double half = 0.5 * length;
    const double neck = half - head_length;

    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    joint->name = connector_name(contact, "butterfly_key");
    joint->is_visible = true;
    joint->targets = {a.guid(), b.guid()};
    joint->insertions = pulled_apart(a, b, contact);

    const std::array<std::array<double, 3>, 4> pieces = {{{-half, -neck, head_width}, {-neck, 0.0, neck_width}, {0.0, neck, neck_width}, {neck, half, head_width}}};

    for (const std::array<double, 3>& piece : pieces) {
        const double bottom0 = top + depth + (end_depth - depth) * std::abs(piece[0]) / half;
        const double bottom1 = top + depth + (end_depth - depth) * std::abs(piece[1]) / half;
        joint->parts.push_back(
            {
                tie_section(
                    origin,
                    axes,
                    piece[0],
                    top,
                    bottom0,
                    piece[2]
                ),
                tie_section(
                    origin,
                    axes,
                    piece[1],
                    top,
                    bottom1,
                    piece[2]
                )
            }
        );
    }

    const double floor = top + pocket_depth;
    const std::vector<std::array<Polyline, 2>> negative = {
        {
            tie_section(
                origin,
                axes,
                -half,
                top,
                floor,
                head_width
            ),
            tie_section(
                origin,
                axes,
                -neck,
                top,
                floor,
                head_width
            )
        },
        {
            tie_section(
                origin,
                axes,
                -neck,
                top,
                floor,
                neck_width
            ),
            tie_section(
                origin,
                axes,
                overshoot,
                top,
                floor,
                neck_width
            )
        },
    };
    const std::vector<std::array<Polyline, 2>> positive = {
        {
            tie_section(
                origin,
                axes,
                neck,
                top,
                floor,
                head_width
            ),
            tie_section(
                origin,
                axes,
                half,
                top,
                floor,
                head_width
            )
        },
        {
            tie_section(
                origin,
                axes,
                -overshoot,
                top,
                floor,
                neck_width
            ),
            tie_section(
                origin,
                axes,
                neck,
                top,
                floor,
                neck_width
            )
        },
    };

    for (const Element* member : {&a, &b}) {
        const Point member_centroid = member->model_geometry_mesh().centroid();
        joint->cutters.push_back((member_centroid - origin).dot(y) < 0.0 ? negative : positive);
    }

    return joint;
}

/// The polygon inset by offset in its frame, as the largest ring Clipper2 leaves on the SCALE grid, in frame coordinates without a closing point; empty when nothing is left.
static std::vector<std::array<double, 2>> inset_polygon(
    const std::vector<Point>& points,
    const Point& origin,
    const Vector& x,
    const Vector& y,
    double offset
) {

    const double SCALE = 1000.0;
    Clipper2Lib::Path64 path;

    for (const Point& point : points) {
        const Vector offset_from_origin = point - origin;
        path.emplace_back(static_cast<int64_t>(std::llround(offset_from_origin.dot(x) * SCALE)), static_cast<int64_t>(std::llround(offset_from_origin.dot(y) * SCALE)));
    }

    const Clipper2Lib::Paths64 inset = Clipper2Lib::InflatePaths(
        {path},
        -offset * SCALE,
        Clipper2Lib::JoinType::Miter,
        Clipper2Lib::EndType::Polygon
    );
    const Clipper2Lib::Path64* best = nullptr;

    for (const Clipper2Lib::Path64& ring : inset)
        if (!best || std::abs(Clipper2Lib::Area(ring)) > std::abs(Clipper2Lib::Area(*best)))
            best = &ring;

    std::vector<std::array<double, 2>> result;

    if (best)
        for (const Clipper2Lib::Point64& point : *best)
            result.push_back({point.x / SCALE, point.y / SCALE});

    return result;
}

/// The ring's corners for the pins: all of them for a quad or less, else four distinct ones, each the corner not yet taken that reaches furthest towards one of the frame's four diagonal directions, the extreme corners of a longer polygon.
static std::vector<std::array<double, 2>> extreme_corners(const std::vector<std::array<double, 2>>& ring) {

    if (ring.size() <= 4)
        return ring;

    std::vector<bool> taken(ring.size(), false);
    std::vector<std::array<double, 2>> corners;

    for (const std::array<double, 2>& diagonal : {std::array<double, 2>{-1.0, -1.0}, std::array<double, 2>{1.0, -1.0}, std::array<double, 2>{1.0, 1.0}, std::array<double, 2>{-1.0, 1.0}}) {
        size_t best = ring.size();

        for (size_t i = 0; i < ring.size(); i++)
            if (!taken[i] && (best == ring.size() || ring[i][0] * diagonal[0] + ring[i][1] * diagonal[1] > ring[best][0] * diagonal[0] + ring[best][1] * diagonal[1]))
                best = i;

        taken[best] = true;
        corners.push_back(ring[best]);
    }

    return corners;
}

/// The pins: centred on the contact along its normal, one at every corner of the contact polygon inset by offset, the four extreme corners of a longer inset; they cut their holes out of both, overshoot past every face a pin leaves; aimed at a then b.
std::shared_ptr<JointBeam> JointBeam::centred_pins(
    const Element& a,
    const Element& b,
    const InteractionContactFace& contact,
    double radius,
    double length,
    double offset,
    double overshoot,
    int pin_sides
) {

    const std::vector<Point> points = merge_collinear(contact.polygon);
    const Point origin = Point::centroid(points);
    Vector normal = compute_newell(points).normalized();

    if ((b.model_geometry_mesh().centroid() - origin).dot(normal) < 0.0)
        normal = -normal;

    const Vector x = top_edge(points).to_vector().normalized();
    const Vector y = normal.cross(x).normalized();
    const std::vector<std::array<double, 2>> ring = inset_polygon(
        points,
        origin,
        x,
        y,
        offset
    );

    if (ring.size() < 3)
        return nullptr;

    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    joint->name = connector_name(contact, "pins");
    joint->is_visible = true;
    joint->targets = {a.guid(), b.guid()};
    joint->insertions = pulled_apart(a, b, contact);
    joint->cutters = {{}, {}};

    for (const std::array<double, 2>& corner : extreme_corners(ring)) {
        const Point station = origin + x * corner[0] + y * corner[1];
        joint->drill_lines.push_back(Line::from_points(station - normal * (0.5 * length), station + normal * (0.5 * length)));
    }

    joint->line_radius = radius;
    joint->chord_tolerance = sides_tolerance(radius, pin_sides);
    joint->drill_overshoot = overshoot;

    return joint;
}

/// Where a pin line leaves the far face of the member it passes through: the start of the line's stretch inside it that ends at the contact.
static Point pin_head(
    const Element& through,
    const Point& station,
    const Vector& direction,
    double reach
) {

    const Line back = Line::from_points(station - direction * reach, station);
    const std::vector<std::array<double, 2>> inside = inside_stretches(through.element_geometry_mesh(), back);
    double start = reach;
    double end = -1.0;

    // the stretch that ends last, at the contact
    for (const std::array<double, 2>& stretch : inside)
        if (stretch[1] > end) {
            end = stretch[1];
            start = stretch[0];
        }

    return back.start() + direction * start;
}

/// A beam's axis made level and unit, empty for an element without an axis or a vertical one.
static std::optional<Vector> level_axis(const Element& element) {

    Vector axis;

    if (const BeamVariable* beam = dynamic_cast<const BeamVariable*>(&element))
        axis = beam->axis.to_vector();
    else if (const Beam* straight = dynamic_cast<const Beam*>(&element); straight && straight->axis.point_count() > 1)
        axis = straight->axis.get_point(straight->axis.point_count() - 1) - straight->axis.get_point(0);

    axis = Vector(axis[0], axis[1], 0.0);

    if (axis.magnitude() < 1e-9)
        return std::nullopt;

    return axis.normalized();
}

std::shared_ptr<JointBeam> JointBeam::headed_pins(
    const Element& a,
    const Element& b,
    const InteractionContactFace& contact,
    PinLayout layout,
    size_t count,
    double offset,
    double shift,
    double radius,
    double length,
    int sides
) {

    const std::vector<Point> points = merge_collinear(contact.polygon);
    const Point origin = Point::centroid(points);
    Vector normal = compute_newell(points).normalized();

    // the member that ends on the contact, its level axis the more square to it, takes the pins along that axis from the far face of the other; neither, a into b square to the contact
    const std::optional<Vector> axis_a = level_axis(a);
    const std::optional<Vector> axis_b = level_axis(b);
    const double end_a = axis_a ? std::abs(axis_a->dot(normal)) : 0.0;
    const double end_b = axis_b ? std::abs(axis_b->dot(normal)) : 0.0;
    const bool a_ends = end_a > end_b && end_a > 0.5;
    const Element& through = a_ends ? b : a;
    const Element& into = a_ends ? a : b;
    const double end = a_ends ? end_a : end_b;
    Vector direction = end > 0.5 ? (a_ends ? *axis_a : *axis_b) : normal;

    if ((into.model_geometry_mesh().centroid() - origin).dot(normal) < 0.0)
        normal = -normal;

    if (direction.dot(normal) < 0.0)
        direction = -direction;

    // x level in the contact and y up it, so a column stands across the height; a level contact takes its top edge
    const Vector level = Vector::z_axis().cross(normal);
    const Vector x = level.magnitude() > 1e-9 ? level.normalized() : top_edge(points).to_vector().normalized();
    const Vector y = normal.cross(x).normalized();
    const std::vector<std::array<double, 2>> ring = inset_polygon(
        points,
        origin,
        x,
        y,
        offset
    );

    if (ring.size() < 3)
        return nullptr;

    // the inset contact's extent along x and y
    double x_min = ring[0][0];
    double x_max = ring[0][0];
    double y_min = ring[0][1];
    double y_max = ring[0][1];

    for (const std::array<double, 2>& corner : ring) {
        x_min = std::min(x_min, corner[0]);
        x_max = std::max(x_max, corner[0]);
        y_min = std::min(y_min, corner[1]);
        y_max = std::max(y_max, corner[1]);
    }

    // the stations on the contact: its inset corners, a column across its height or a row along it, moved by shift
    std::vector<std::array<double, 2>> stations;

    if (layout == PinLayout::corners)
        stations = extreme_corners(ring);

    for (size_t i = 0; layout != PinLayout::corners && i < count; i++) {
        const double t = count == 1 ? 0.5 : static_cast<double>(i) / static_cast<double>(count - 1);

        if (layout == PinLayout::vertical)
            stations.push_back({shift, y_min + (y_max - y_min) * t});
        else
            stations.push_back({x_min + (x_max - x_min) * t, shift});
    }

    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    joint->name = connector_name(contact, "pins");
    joint->is_visible = true;
    joint->pre_drill = true;
    joint->targets = {a.guid(), b.guid()};
    joint->insertions = pulled_apart(a, b, contact);

    // each pin through its station on the contact, from its head on the far face of `through`, length long into `into`
    for (const std::array<double, 2>& station : stations) {
        const Point point = origin + x * station[0] + y * station[1];
        const Point head = pin_head(
            through,
            point,
            direction,
            length
        );
        joint->drill_lines.push_back(Line::from_points(head, head + direction * length));
    }

    joint->line_radius = radius;
    joint->chord_tolerance = sides_tolerance(radius, sides);

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool JointBeam::is_connector() const {
    return !parts.empty() || !cutters.empty() || pre_drill;
}

Vector JointBeam::insertion(size_t target) const {

    if (is_connector())
        return target < insertions.size() ? insertions[target] : Vector(0.0, 0.0, 0.0);

    // each beam's box centre, and the way from the other's to this target's
    const std::array<Point, 2> centres = {
        Point::centroid({feature.volumes[0][0], feature.volumes[0][2], feature.volumes[1][0], feature.volumes[1][2]}),
        Point::centroid({feature.volumes[2][0], feature.volumes[2][2], feature.volumes[3][0], feature.volumes[3][2]}),
    };
    const size_t other = target == 0 ? 1 : 0;
    const Vector away = centres[target] - centres[other];

    // a crossing slides along the normal of the axes; a pair with an end along the axis of the beam that ends, the one more along the way apart
    Vector direction = feature.normal;
    if (feature.end_type != 0)
        direction = std::abs(feature.axes[0].dot(away)) >= std::abs(feature.axes[1].dot(away)) ? feature.axes[0] : feature.axes[1];
    if (direction.dot(away) < 0.0)
        direction = direction * -1.0;

    return direction.normalized();
}

std::shared_ptr<Interaction> JointBeam::interaction(size_t target) const {

    // a butt: the block this target gains or loses
    if (is_butt)
        return std::make_shared<InteractionFeatureSolid>(butt_solids.at(target));

    // pins: their holes are pre-drilled lines the targets read, nothing is cut
    if (pre_drill)
        return std::make_shared<InteractionFeaturePlateBeam>();

    // a connector: the cutters for this target lofted into one solid, and its pins
    if (is_connector()) {
        Mesh mesh;

        if (target < cutters.size())
            for (const std::array<Polyline, 2>& cutter : cutters[target])
                append_mesh(mesh, Mesh::loft({cutter[0]}, {cutter[1]}, true));

        const std::shared_ptr<InteractionFeatureSolid> cut = std::make_shared<InteractionFeatureSolid>();
        cut->mesh = mesh;
        cut->drills = drill_lines;
        cut->drill_radius = line_radius;
        cut->drill_tolerance = chord_tolerance;

        return cut;
    }

    // a beam-to-beam joint: its four volumes, male and female corners swapped for the second target
    const std::shared_ptr<InteractionFeatureBeam> volumes = std::make_shared<InteractionFeatureBeam>(feature);
    volumes->guid() = ::guid();

    if (target == 1) {
        std::swap(volumes->volumes[0], volumes->volumes[2]);
        std::swap(volumes->volumes[1], volumes->volumes[3]);
        std::swap(volumes->axes[0], volumes->axes[1]);
    }

    return volumes;
}

Mesh JointBeam::part_mesh(size_t index) const {
    return Mesh::loft({parts.at(index)[0]}, {parts.at(index)[1]}, true);
}

std::vector<InteractionFeatureSolid> JointBeam::part_features(size_t index) const {

    const Mesh part = part_mesh(index);
    std::vector<InteractionFeatureSolid> cuts = solid_features;
    InteractionFeatureSolid bores;

    for (const Line& pin : drill_lines)
        if (!inside_stretches(part, pin).empty())
            bores.drills.push_back(pin);

    if (!bores.drills.empty()) {
        bores.drill_radius = line_radius;
        bores.drill_tolerance = chord_tolerance;
        cuts.push_back(bores);
    }

    return cuts;
}

BRep JointBeam::part_brep(size_t index) const {

    const std::vector<InteractionFeatureSolid> cuts = part_features(index);

    return cuts.empty() ? brep_between_loops({parts[index][0]}, {parts[index][1]}) : solid_features_brep(part_mesh(index), cuts);
}

std::vector<std::shared_ptr<Joint>> JointBeam::children() const {

    std::vector<std::shared_ptr<Joint>> result;

    for (size_t i = 0; i < parts.size(); i++)
        result.push_back(std::make_shared<ConnectorPart>(*this, i, parts.size() == 1 ? name + "_part" : fmt::format("{}_part_{}", name, i)));

    for (size_t i = 0; i < drill_lines.size(); i++) {
        result.push_back(std::make_shared<Pin>(drill_lines[i], line_radius, chord_tolerance));
        result.back()->name = fmt::format("{}_pin_{}", name, i);
    }

    return result;
}

const Mesh& JointBeam::element_geometry_mesh() const {

    if (!is_connector())
        return Joint::element_geometry_mesh();

    if (!mesh_)
        mesh_ = Mesh();

    return *mesh_;
}

const BRep& JointBeam::element_geometry_brep() const {

    if (!is_connector())
        return Joint::element_geometry_brep();

    if (!brep_)
        brep_ = BRep();

    return *brep_;
}

std::vector<std::array<Polyline, 2>> JointBeam::bodies() const {

    if (!parts.empty())
        return parts;

    std::vector<std::array<Polyline, 2>> result;

    for (int k = 0; k < 4; k += 2)
        if (feature.volumes[k].point_count() >= 4 && feature.volumes[k + 1].point_count() >= 4)
            result.push_back({feature.volumes[k], feature.volumes[k + 1]});

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Transformation
// ═══════════════════════════════════════════════════════════════════════════

void JointBeam::place(const Xform& xform) {

    Joint::place(xform);

    for (Polyline& volume : feature.volumes)
        volume = volume.transformed(xform);
    feature.normal = feature.normal.transformed(xform);
    for (Vector& axis : feature.axes)
        axis = axis.transformed(xform);
    for (Vector& insertion : insertions)
        insertion = insertion.transformed(xform);

    for (std::array<Polyline, 2>& part : parts)
        part = {part[0].transformed(xform), part[1].transformed(xform)};

    for (std::vector<std::array<Polyline, 2>>& target : cutters)
        for (std::array<Polyline, 2>& cutter : target)
            cutter = {cutter[0].transformed(xform), cutter[1].transformed(xform)};

    for (InteractionFeatureSolid& cut : solid_features)
        cut = cut.transformed(xform);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

void JointBeam::write_proto(wood_proto::Joint& proto) const {

    Joint::write_proto(proto);

    if (!proto.mutable_beam_feature()->ParseFromString(feature.pb_dumps()))
        throw std::runtime_error("Invalid beam feature");

    for (const std::array<Polyline, 2>& part : parts)
        for (const Polyline& loop : part)
            if (!proto.add_parts()->ParseFromString(loop.pb_dumps()))
                throw std::runtime_error("Invalid connector part");

    proto.set_drill_overshoot(drill_overshoot);
    proto.set_pre_drill(pre_drill);

    for (const Vector& insertion : insertions)
        if (!proto.add_insertions()->ParseFromString(insertion.pb_dumps()))
            throw std::runtime_error("Invalid connector insertion");

    for (const InteractionFeatureSolid& cut : solid_features)
        if (!proto.add_solid_features()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Invalid connector cut");

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

    for (int i = 0; i + 1 < proto.parts_size(); i += 2){
        const Polyline bottom_loop = Polyline::pb_loads(proto.parts(i).SerializeAsString());
        const Polyline top_loop = Polyline::pb_loads(proto.parts(i + 1).SerializeAsString());
        parts.push_back({bottom_loop, top_loop});
    }

    drill_overshoot = proto.drill_overshoot();
    pre_drill = proto.pre_drill();

    for (const session_proto::Vector& insertion : proto.insertions())
        insertions.push_back(Vector::pb_loads(insertion.SerializeAsString()));

    for (const wood_proto::InteractionFeatureSolid& cut : proto.solid_features())
        solid_features.push_back(InteractionFeatureSolid::pb_loads(cut.SerializeAsString()));

    for (const wood_proto::JointCutter& cutter : proto.cutters()) {
        cutters.push_back({});

        for (int i = 0; i + 1 < cutter.loops_size(); i += 2){
            const Polyline bottom_loop = Polyline::pb_loads(cutter.loops(i).SerializeAsString());
            const Polyline top_loop = Polyline::pb_loads(cutter.loops(i + 1).SerializeAsString());
            cutters.back().push_back({bottom_loop, top_loop});
        }
    }

    if (!proto.has_beam_feature())
        return;

    const std::shared_ptr<Interaction> loaded = Interaction::pb_loads(proto.beam_feature().SerializeAsString());

    const std::shared_ptr<InteractionFeatureBeam> feature = std::dynamic_pointer_cast<InteractionFeatureBeam>(loaded);

    if (!feature)
        throw std::runtime_error("Invalid beam joint feature");

    this->feature = *feature;
}

}
