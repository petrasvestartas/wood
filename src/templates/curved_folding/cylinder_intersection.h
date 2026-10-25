#pragma once
#include "wood_session.h"
#include "development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// The intersection of parallel cylinders in sequence, ((a and b) and c) and so on, one curved plate per surviving arc.
class CylinderIntersection : public WoodSession {
public:
    /// One cylinder: its circle in the section square to the axis.
    struct Cylinder {
        double y; // mm, the axis across.
        double z; // mm, the axis up.
        double radius; // mm, the circle.
    };

    const std::vector<Cylinder> cylinders; // The cylinders in the order they intersect.
    const double length; // mm, every cylinder along x from 0.
    const double thickness; // mm, every plate, inside the intersection.
    const Point origin; // Where the section's origin stands at x = 0.

private:
    /// One surviving arc of a cylinder's circle, counter-clockwise from start to end.
    struct Arc {
        size_t cylinder; // The cylinder it lies on.
        double start; // radians, the first end.
        double end; // radians, the second end, after start.
    };

    double _distortion = 0.0; // The largest relative length error of any plate's development.

public:
    /// The intersection as the session named name.
    CylinderIntersection(
        const std::vector<Cylinder>& cylinders,
        double length = 12000.0,
        double thickness = 60.0,
        const Point& origin = Point(0.0, 0.0, 0.0),
        const std::string& name = "cylinder_intersection"
    );

    /// Not copied, as a session is not.
    CylinderIntersection(const CylinderIntersection&) = delete;

    /// Not assigned, as it is not copied.
    CylinderIntersection& operator=(const CylinderIntersection&) = delete;

    /// The largest relative length error of any plate's development, 0 for an exact cylinder.
    double distortion() const {
        return _distortion;
    }

private:
    /// The arcs of the circles of the given radii shrink smaller that lie inside every other such circle, in turn round the section.
    std::vector<Arc> compute_arcs(double shrink) const;

    /// Every surviving arc a plate, plate_<k> in the group plates: the outer arc and the inner one a thickness inside.
    void add_plates(const std::vector<Arc>& outer, const std::vector<Arc>& inner);

    /// The point of cylinder c's circle shrunk by shrink at angle, at x along the axis.
    Point on_circle(size_t c, double shrink, double angle, double x) const;

    /// The arc as a rational NURBS at x along the axis.
    NurbsCurve arc_curve(const Arc& arc, double shrink, double x) const;
};

inline CylinderIntersection::CylinderIntersection(
    const std::vector<Cylinder>& cylinders,
    double length,
    double thickness,
    const Point& origin,
    const std::string& name
)
    : WoodSession(name),
      cylinders(cylinders),
      length(length),
      thickness(thickness),
      origin(origin) {

    if (cylinders.size() < 2 || length <= 0.0 || thickness <= 0.0)
        throw std::invalid_argument("A cylinder intersection needs two cylinders, a length and a thickness");

    // outer: every circle's arc inside all the others, the boundary of ((a and b) and c) and so on
    const std::vector<Arc> outer = compute_arcs(0.0);

    // inner: the same for the circles a thickness smaller, so two neighbours meet on the plane through both their crossings
    const std::vector<Arc> inner = compute_arcs(thickness);

    bool same = outer.size() == inner.size();

    for (size_t k = 0; k < outer.size() && same; k++)
        same = outer[k].cylinder == inner[k].cylinder;

    if (!same)
        throw std::invalid_argument("The thickness removes an arc of the intersection");

    // plates: every arc a cylinder plate along the axis, between its outer and inner arc
    add_plates(outer, inner);

    // contacts: every two plates that share a crease, found by the session's contact detection
    compute_face_contacts(0);
}

inline std::vector<CylinderIntersection::Arc> CylinderIntersection::compute_arcs(double shrink) const {

    // per circle the angles inside every other circle: the intersection of the intervals each other circle covers
    std::vector<Arc> arcs;
    const double full = 2.0 * Tolerance::PI;

    for (size_t i = 0; i < cylinders.size(); i++) {
        const double r = cylinders[i].radius - shrink;
        double start = -Tolerance::PI;
        double end = Tolerance::PI;
        bool empty = false;
        bool unset = true;

        for (size_t j = 0; j < cylinders.size() && !empty; j++) {
            if (j == i)
                continue;

            // the angles of circle i inside circle j: around the direction to j's centre, half a chord angle either side
            const double dy = cylinders[j].y - cylinders[i].y;
            const double dz = cylinders[j].z - cylinders[i].z;
            const double d = std::hypot(dy, dz);
            const double s = cylinders[j].radius - shrink;
            const double cosine = (r * r + d * d - s * s) / (2.0 * r * d);

            if (cosine >= 1.0) {
                empty = true;
                continue;
            }

            if (cosine <= -1.0)
                continue;

            const double towards = std::atan2(dz, dy);
            const double half = std::acos(cosine);

            // the first cut sets the interval, every next one narrows it, both measured around the first's middle
            if (unset) {
                start = towards - half;
                end = towards + half;
                unset = false;
                continue;
            }

            const double middle = 0.5 * (start + end);
            const double shifted = towards + full * std::round((middle - towards) / full);
            start = std::max(start, shifted - half);
            end = std::min(end, shifted + half);
            empty = start >= end;
        }

        if (!empty && !unset)
            arcs.push_back({i, start, end});
    }

    // round the section: by the angle of every arc's middle point about the centroid of the arcs' ends
    double cy = 0.0;
    double cz = 0.0;

    for (const Arc& arc : arcs) {
        const Point a = on_circle(arc.cylinder, shrink, arc.start, 0.0);
        const Point b = on_circle(arc.cylinder, shrink, arc.end, 0.0);
        cy += a[1] + b[1];
        cz += a[2] + b[2];
    }

    cy /= static_cast<double>(2 * arcs.size());
    cz /= static_cast<double>(2 * arcs.size());
    std::vector<double> angles;

    for (const Arc& arc : arcs) {
        const Point middle = on_circle(arc.cylinder, shrink, 0.5 * (arc.start + arc.end), 0.0);
        angles.push_back(std::atan2(middle[2] - cz, middle[1] - cy));
    }

    // every next arc the one of the smallest angle left
    std::vector<Arc> sorted;
    std::vector<bool> taken(arcs.size(), false);

    for (size_t n = 0; n < arcs.size(); n++) {
        size_t best = arcs.size();

        for (size_t k = 0; k < arcs.size(); k++)
            if (!taken[k] && (best == arcs.size() || angles[k] < angles[best]))
                best = k;

        taken[best] = true;
        sorted.push_back(arcs[best]);
    }

    return sorted;
}

inline Point CylinderIntersection::on_circle(size_t c, double shrink, double angle, double x) const {

    const double r = cylinders[c].radius - shrink;

    return origin + Vector(x, cylinders[c].y + r * std::cos(angle), cylinders[c].z + r * std::sin(angle));
}

inline NurbsCurve CylinderIntersection::arc_curve(const Arc& arc, double shrink, double x) const {

    const Point start = on_circle(arc.cylinder, shrink, arc.start, x);
    const Point middle = on_circle(arc.cylinder, shrink, 0.5 * (arc.start + arc.end), x);
    const Point end = on_circle(arc.cylinder, shrink, arc.end, x);

    return Primitives::arc(start, middle, end);
}

inline void CylinderIntersection::add_plates(const std::vector<Arc>& outer, const std::vector<Arc>& inner) {

    const std::shared_ptr<TreeNode> group = add_group("plates");

    for (size_t k = 0; k < outer.size(); k++) {
        // the outer and inner arcs at both ends of the axis, the inner of the same cylinder
        const NurbsCurve outer_a = arc_curve(outer[k], 0.0, 0.0);
        const NurbsCurve outer_b = arc_curve(outer[k], 0.0, length);
        const NurbsCurve inner_a = arc_curve(inner[k], thickness, 0.0);
        const NurbsCurve inner_b = arc_curve(inner[k], thickness, length);
        const std::shared_ptr<PlateCurved> plate = std::make_shared<PlateCurved>(
            outer_a,
            outer_b,
            inner_a,
            inner_b,
            fmt::format("plate_{}", k)
        );
        add(plate, group);

        // its development, the proof it is cut flat
        const Development development(outer_a, outer_b, 32);
        _distortion = std::max(_distortion, development.distortion);
    }
}
