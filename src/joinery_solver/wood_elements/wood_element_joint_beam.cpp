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
std::shared_ptr<JointBeam> JointBeam::butt(Element& running, Element& butting) {

    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    joint->name = "butt";
    joint->is_butt = true;
    joint->targets = {running.guid(), butting.guid()};

    // both solids' corners, centres and the size of the pair: the running member as earlier joints left it, so a corner run on twice closes
    const WoodElement* running_wood = dynamic_cast<const WoodElement*>(&running);
    const Mesh running_mesh = running_wood ? running_wood->stock_mesh() : running.element_geometry_mesh();
    const std::vector<Point> running_points = running_mesh.to_vertices_and_faces().first;
    const std::vector<Point> butting_points = butting.element_geometry_mesh().to_vertices_and_faces().first;
    Point running_centre(0.0, 0.0, 0.0);
    Point butting_centre(0.0, 0.0, 0.0);
    for (const Point& point : running_points)
        running_centre = running_centre + Vector(point[0], point[1], point[2]) / (double)running_points.size();
    for (const Point& point : butting_points)
        butting_centre = butting_centre + Vector(point[0], point[1], point[2]) / (double)butting_points.size();
    double size = 0.0;
    for (const std::vector<Point>* points : {&running_points, &butting_points})
        for (const Point& point : *points)
            size = std::max({size, Point::distance(point, running_centre), Point::distance(point, butting_centre)});
    const double tolerance = 1e-6 * size;

    // the running member's face the butting member looks at: the one the butting member's centre lies farthest in front of
    const std::vector<Plane> running_planes = running.planes();
    Plane face;
    double farthest = -std::numeric_limits<double>::max();
    for (const Plane& plane : running_planes) {
        const double distance = (butting_centre - plane.origin()).dot(plane.z_axis());
        if (distance > farthest) {
            farthest = distance;
            face = plane;
        }
    }

    // the butting member cut behind that face: a block on the face, as wide and as deep as the pair
    const Point foot = butting_centre - face.z_axis() * (butting_centre - face.origin()).dot(face.z_axis());
    const Polyline square = Polyline::rectangle(foot - face.x_axis() * (2.0 * size) - face.y_axis() * (2.0 * size), face.x_axis(), face.y_axis(), 4.0 * size, 4.0 * size);
    const Vector behind = face.z_axis() * (-4.0 * size);
    joint->butt_solids[1] = InteractionFeatureSolid(Mesh::loft({square}, {square.translated(behind)}, true), SolidOperation::subtract);

    // the running member's end toward the butting member: a face across the face found, looking toward the butting member's centre
    const Vector toward = (butting_centre - running_centre).normalized();
    std::optional<Plane> end;
    double most = 0.2;
    for (const Plane& plane : running_planes) {
        if (std::abs(plane.z_axis().dot(face.z_axis())) > 0.5)
            continue;
        if (plane.z_axis().dot(toward) > most) {
            most = plane.z_axis().dot(toward);
            end = plane;
        }
    }

    // a corner when that end lies inside the butting member: the running member gains its end face run on to the butting member's far face
    if (!end)
        return joint;
    double nearest = std::numeric_limits<double>::max();
    double far = -std::numeric_limits<double>::max();
    for (const Point& point : butting_points) {
        const double distance = (point - end->origin()).dot(end->z_axis());
        nearest = std::min(nearest, distance);
        far = std::max(far, distance);
    }
    if (!(nearest < -tolerance && far > tolerance))
        return joint;

    // the end face's outline: the running member's corners on its plane, in order around their centre
    std::vector<Point> corners;
    for (const Point& point : running_points)
        if (std::abs((point - end->origin()).dot(end->z_axis())) < tolerance * 1e3)
            corners.push_back(point);
    if (corners.size() < 3)
        return joint;
    Point middle(0.0, 0.0, 0.0);
    for (const Point& point : corners)
        middle = middle + Vector(point[0], point[1], point[2]) / (double)corners.size();
    const Vector x_axis = end->x_axis();
    const Vector y_axis = end->y_axis();
    std::sort(corners.begin(), corners.end(), [&](const Point& a, const Point& b) {
        return std::atan2((a - middle).dot(y_axis), (a - middle).dot(x_axis)) < std::atan2((b - middle).dot(y_axis), (b - middle).dot(x_axis));
    });
    corners.erase(std::unique(corners.begin(), corners.end(), [&](const Point& a, const Point& b) { return Point::distance(a, b) < tolerance * 1e3; }), corners.end());
    corners.push_back(corners.front());
    const Polyline outline(corners);
    joint->butt_solids[0] = InteractionFeatureSolid(Mesh::loft({outline}, {outline.translated(end->z_axis() * far)}, true), SolidOperation::add);

    return joint;
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
