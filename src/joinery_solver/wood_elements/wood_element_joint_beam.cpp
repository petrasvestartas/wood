#include "pch.h"
#include "element_joint.pb.h"
#include "wood_session.h"
#include "wood_feature_detection_beam.h"
#include "wood_brep_drill.h"
#include "wood_element_dowel.h"
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

std::shared_ptr<JointBeam> JointBeam::from_contact(
    const Beam& source,
    const Beam& target,
    const InteractionContactAxis& contact,
    double volume_length,
    double cross_or_side_to_end,
    int flip_male
) {

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

    return joint;
}

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
static Point frame_point(
    const Point& origin,
    const std::array<Vector, 3>& axes,
    double x,
    double y,
    double z
) {
    return origin + axes[0] * x + axes[1] * y + axes[2] * z;
}

/// The chord tolerance that makes a drill of radius a prism of sides sides.
static double sides_tolerance(double radius, int sides) {
    return 2.0 * radius * std::pow(std::sin(M_PI / (2.0 * sides)), 2) * (1.0 + 1e-9);
}

/// The dowel clipped flush to the members it passes through: from where its axis first enters one of them to where it last leaves one, an end that lies inside a member kept as it is; unchanged when it meets none.
static Line flush_dowel(const Line& dowel, const std::vector<const Element*>& members) {

    const double length = dowel.length();
    const Vector d = dowel.to_vector().normalized();
    double low = 1e300;
    double high = -1e300;

    for (const Element* member : members)
        for (const std::array<double, 2>& stretch : inside_stretches(member->element_geometry_mesh(), dowel)) {
            low = std::min(low, stretch[0]);
            high = std::max(high, stretch[1]);
        }

    if (std::min(high, length) - std::max(low, 0.0) < 1e-9)
        return dowel;

    return Line::from_points(low > 0.0 ? dowel.start() + d * low : dowel.start(), high < length ? dowel.start() + d * high : dowel.end());
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

/// The wedge: a prism of the profile, apex down, along the contact's top edge, cut horizontally at the edge's level, shortened by length_margin at both ends, the end nearest the end plane on that plane instead, dowels every dowel_spacing flush with the members and a box pocket pocket_depth deep under the wedge face on each member's side; aimed at a then b.
std::shared_ptr<JointBeam> JointBeam::wedge(
    const Element& a,
    const Element& b,
    const InteractionContactFace& contact,
    double length_margin,
    double pocket_depth,
    const std::optional<Plane>& end,
    double dowel_radius,
    double dowel_spacing,
    int dowel_sides,
    double overshoot,
    const std::array<std::array<double, 2>, 3>& profile,
    const std::array<double, 2>& dowel_offset
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
    joint->name = "wedge";
    joint->is_visible = true;
    joint->targets = {a.guid(), b.guid()};

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

    const int count = dowel_spacing > 0.0 ? std::max(static_cast<int>(length / dowel_spacing), 1) : 1;

    for (int i = 0; i < count; i++) {
        const double station = -0.5 * length + (i + 0.5) * (length / count);
        const Point start = frame_point(
            origin,
            axes,
            station,
            -dowel_offset[0],
            dowel_offset[1]
        );
        const Point end = frame_point(
            origin,
            axes,
            station,
            dowel_offset[0],
            dowel_offset[1]
        );
        const Vector flat = Vector(end[0] - start[0], end[1] - start[1], 0.0).normalized();
        const Point centre = start + (end - start) * 0.5;
        const Line dowel = Line::from_points(centre - flat * dowel_offset[0], centre + flat * dowel_offset[0]);
        joint->drill_lines.push_back(flush_dowel(dowel, {&a, &b}));
    }

    joint->line_radius = dowel_radius;
    joint->chord_tolerance = sides_tolerance(dowel_radius, dowel_sides);
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

/// The plate: width thick, back into the column and front into the rib along the horizontal contact normal, height down from the contact's top edge, four dowels across it margin_x and margin_z radii in from its ends and its top and bottom, dowel_length long but flush with the members; it cuts its box, raised by overshoot, and the dowel holes out of both; aimed at the column then the rib.
std::shared_ptr<JointBeam> JointBeam::rectangle_plate(
    const Element& column,
    const Element& rib,
    const InteractionContactFace& contact,
    double dowel_length,
    double width,
    double back,
    double front,
    double height,
    double dowel_radius,
    double margin_x,
    double margin_z,
    double overshoot,
    int dowel_sides
) {

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
    const std::array<Vector, 3> axes = {x, z.cross(x).normalized(), z};
    const Point origin = top_origin(points);

    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    joint->name = "rectangle_plate";
    joint->is_visible = true;
    joint->targets = {column.guid(), rib.guid()};
    joint->parts = {
        frame_box(
            origin,
            axes,
            -back,
            front,
            width,
            -height,
            0.0
        )
    };

    const std::array<Polyline, 2> pocket = frame_box(
        origin,
        axes,
        -back,
        front,
        width,
        -height,
        overshoot
    );
    joint->cutters = {{pocket}, {pocket}};

    const double half = 0.5 * dowel_length;

    for (const double station : {-back + margin_x * dowel_radius, front - margin_x * dowel_radius})
        for (const double level : {-margin_z * dowel_radius, -height + margin_z * dowel_radius})
            joint->drill_lines.push_back(
                flush_dowel(
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
                    {&column, &rib}
                )
            );

    joint->line_radius = dowel_radius;
    joint->chord_tolerance = sides_tolerance(dowel_radius, dowel_sides);
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
std::shared_ptr<JointBeam> JointBeam::tie(
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
    joint->name = "tie";
    joint->is_visible = true;
    joint->targets = {a.guid(), b.guid()};

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

/// The ring's corners for the dowels: all of them for a quad or less, else four distinct ones, each the corner not yet taken that reaches furthest towards one of the frame's four diagonal directions, the extreme corners of a longer polygon.
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

/// The dowels: centred on the contact along its normal, one at every corner of the contact polygon inset by offset, the four extreme corners of a longer inset; they cut their holes out of both, overshoot past every face a dowel leaves; aimed at a then b.
std::shared_ptr<JointBeam> JointBeam::dowels(
    const Element& a,
    const Element& b,
    const InteractionContactFace& contact,
    double radius,
    double length,
    double offset,
    double overshoot,
    int dowel_sides
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
    joint->name = "dowels";
    joint->is_visible = true;
    joint->targets = {a.guid(), b.guid()};
    joint->cutters = {{}, {}};

    for (const std::array<double, 2>& corner : extreme_corners(ring)) {
        const Point station = origin + x * corner[0] + y * corner[1];
        joint->drill_lines.push_back(Line::from_points(station - normal * (0.5 * length), station + normal * (0.5 * length)));
    }

    joint->line_radius = radius;
    joint->chord_tolerance = sides_tolerance(radius, dowel_sides);
    joint->drill_overshoot = overshoot;

    return joint;
}

/// The screws: each line's start is a head, the screw length long along the line from there; no cutter and no cut, the lines are the pre-drilled holes of both members; aimed at a then b.
std::shared_ptr<JointBeam> JointBeam::screws(
    const Element& a,
    const Element& b,
    const std::vector<Line>& lines,
    double radius,
    double length,
    int sides
) {
    return screws(
        std::vector<const Element*>{&a, &b},
        lines,
        radius,
        length,
        sides
    );
}

std::shared_ptr<JointBeam> JointBeam::screws(
    const std::vector<const Element*>& members,
    const std::vector<Line>& lines,
    double radius,
    double length,
    int sides
) {

    if (lines.empty() || members.size() < 2)
        return nullptr;

    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    joint->name = "screws";
    joint->is_visible = true;
    joint->pre_drill = true;

    for (const Element* member : members)
        joint->targets.push_back(member->guid());

    for (const Line& line : lines) {
        const Point head = line.start();
        joint->drill_lines.push_back(Line::from_points(head, head + line.to_vector().normalized() * length));
    }

    joint->line_radius = radius;
    joint->chord_tolerance = sides_tolerance(radius, sides);

    return joint;
}

/// The frame of a box part: origin at its centre, x along the first side of its first loop, z along the last, y from the first loop to the second; the loops frame_box makes.
static std::pair<Point, std::array<Vector, 3>> box_frame(const std::array<Polyline, 2>& box) {

    if (box[0].point_count() != 5 || box[1].point_count() != 5)
        throw std::invalid_argument("A cross lap needs box parts of four corners");

    std::vector<Point> near = box[0].get_points();
    near.pop_back();
    std::vector<Point> far = box[1].get_points();
    far.pop_back();
    std::vector<Point> corners = near;
    corners.insert(corners.end(), far.begin(), far.end());

    const Vector x = (near[1] - near[0]).normalized();
    const Vector z = (near[3] - near[0]).normalized();
    const Vector y = (Point::centroid(far) - Point::centroid(near)).normalized();

    return {Point::centroid(corners), {x, y, z}};
}

/// The extent of the points along the frame's axis: lowest and highest coordinate.
static std::array<double, 2> extent(const std::vector<Point>& points, const Point& origin, const Vector& axis) {

    std::array<double, 2> range = {1e300, -1e300};

    for (const Point& point : points) {
        range[0] = std::min(range[0], (point - origin).dot(axis));
        range[1] = std::max(range[1], (point - origin).dot(axis));
    }

    return range;
}

/// The eight corners of a box part.
static std::vector<Point> box_corners(const std::array<Polyline, 2>& box) {

    std::vector<Point> corners = box[0].get_points();
    corners.pop_back();
    const std::vector<Point> far = box[1].get_points();
    corners.insert(corners.end(), far.begin(), far.end() - 1);

    return corners;
}

/// The cross lap: each slot exactly as wide as the other part's footprint along the slot, so the parts fit tight, and margin beyond the own part's thickness and past its top or bottom so the cut is through; stored on the connectors as their solid cuts when added; aimed at a then b, hidden, a relation rather than a part.
std::shared_ptr<JointBeam> JointBeam::cross_lap(
    const JointBeam& a,
    const JointBeam& b,
    double share,
    double margin
) {

    if (a.parts.size() != 1 || b.parts.size() != 1)
        throw std::invalid_argument("A cross lap joins two connectors of one box part each");

    const std::pair<Point, std::array<Vector, 3>> frame_a = box_frame(a.parts[0]);
    const std::pair<Point, std::array<Vector, 3>> frame_b = box_frame(b.parts[0]);
    const std::vector<Point> corners_a = box_corners(a.parts[0]);
    const std::vector<Point> corners_b = box_corners(b.parts[0]);

    const std::array<double, 2> z_a = extent(corners_a, frame_a.first, frame_a.second[2]);
    const std::array<double, 2> z_b = extent(corners_b, frame_a.first, frame_a.second[2]);
    const double low = std::max(z_a[0], z_b[0]);
    const double high = std::min(z_a[1], z_b[1]);

    if (high - low <= 0.0)
        throw std::invalid_argument("Cross lap parts do not overlap in height");

    const double lap = low + share * (high - low);
    const std::array<double, 2> across_a = extent(corners_b, frame_a.first, frame_a.second[0]);
    const std::array<double, 2> across_b = extent(corners_a, frame_b.first, frame_b.second[0]);
    const double width_a = extent(corners_a, frame_a.first, frame_a.second[1])[1] * 2.0 + 2.0 * margin;
    const double width_b = extent(corners_b, frame_b.first, frame_b.second[1])[1] * 2.0 + 2.0 * margin;
    const double lap_b = (frame_a.first - frame_b.first).dot(frame_b.second[2]) + lap;
    const std::array<double, 2> z_b_own = extent(corners_b, frame_b.first, frame_b.second[2]);

    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    joint->name = "cross_lap";
    joint->targets = {a.guid(), b.guid()};
    joint->cutters = {
        {
            frame_box(
                frame_a.first,
                frame_a.second,
                across_a[0],
                across_a[1],
                width_a,
                lap,
                z_a[1] + margin
            )
        },
        {
            frame_box(
                frame_b.first,
                frame_b.second,
                across_b[0],
                across_b[1],
                width_b,
                z_b_own[0] - margin,
                lap_b
            )
        },
    };

    return joint;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool JointBeam::is_connector() const {
    return !parts.empty() || !cutters.empty() || pre_drill;
}

Mesh JointBeam::part_mesh(size_t index) const {
    return Mesh::loft({parts.at(index)[0]}, {parts.at(index)[1]}, true);
}

std::vector<InteractionFeatureSolid> JointBeam::part_features(size_t index) const {

    const Mesh part = part_mesh(index);
    std::vector<InteractionFeatureSolid> cuts = solid_features;
    InteractionFeatureSolid bores;

    for (const Line& dowel : drill_lines)
        if (!inside_stretches(part, dowel).empty())
            bores.drills.push_back(dowel);

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
        result.push_back(std::make_shared<Dowel>(drill_lines[i], line_radius, chord_tolerance));
        result.back()->name = fmt::format("{}_{}_{}", name, pre_drill ? "screw" : "dowel", i);
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
