#include "pch.h"
#include "wood_joint_classification.h"

namespace wood_session {

JointClassification classify_joint(const EdgeStraightElement& a, const EdgeStraightElement& b, double tolerance) {

    const Line axis_a = Line::from_points(a.axis.get_point(0), a.axis.get_point(a.axis.point_count() - 1));
    const Line axis_b = Line::from_points(b.axis.get_point(0), b.axis.get_point(b.axis.point_count() - 1));
    const double reach_a = b.radius(0) + tolerance;
    const double reach_b = a.radius(0) + tolerance;
    const double cosine = std::abs(axis_a.to_vector().normalized().dot(axis_b.to_vector().normalized()));
    JointClassification joint;

    // parallel: the overlap of b projected on a, each an end when the overlap reaches one of its ends
    if (cosine > std::cos(5.0 * Tolerance::PI / 180.0)) {
        const Vector along = axis_a.to_vector().normalized();
        const double first = (axis_b.start() - axis_a.start()).dot(along);
        const double last = (axis_b.end() - axis_a.start()).dot(along);
        const double low = std::max(0.0, std::min(first, last));
        const double high = std::min(axis_a.length(), std::max(first, last));
        joint.point = axis_a.start() + along * (0.5 * (low + high));
        Point on_b;
        double t_b = 0.0;
        Intersection::closest_point_on_segment(joint.point, axis_b, on_b, t_b);
        if (high < low - reach_a || Point::distance(joint.point, on_b) > a.radius(0) + b.radius(0) + tolerance)
            return joint;

        const bool first_on_a = first > -reach_b && first < axis_a.length() + reach_b;
        const bool last_on_a = last > -reach_b && last < axis_a.length() + reach_b;
        joint.ends = {low < reach_a || high > axis_a.length() - reach_a, first_on_a || last_on_a};
        joint.category = joint.ends[0] && joint.ends[1] && Point::distance(joint.point, on_b) < tolerance ? JointCategory::end_end_inplane : JointCategory::side_side;
        joint.family = joint_family(joint.category);
        return joint;
    }

    // crossing or meeting: the closest points of the two segments, each an end when within the other's half section of one
    double t_a = 0.0;
    double t_b = 0.0;
    Intersection::line_line_parameters(axis_a, axis_b, t_a, t_b, 0.0, false, false);
    Point on_a = axis_a.point_at(std::clamp(t_a, 0.0, 1.0));
    Point on_b;
    Intersection::closest_point_on_segment(on_a, axis_b, on_b, t_b);
    Intersection::closest_point_on_segment(on_b, axis_a, on_a, t_a);
    if (Point::distance(on_a, on_b) > a.radius(0) + b.radius(0) + tolerance)
        return joint;

    joint.point = on_a;
    joint.ends = {std::min(t_a, 1.0 - t_a) * axis_a.length() < reach_a, std::min(t_b, 1.0 - t_b) * axis_b.length() < reach_b};
    if (joint.ends[0] && joint.ends[1])
        joint.category = JointCategory::end_end_outplane;
    else if (joint.ends[0] || joint.ends[1])
        joint.category = JointCategory::side_end;
    else
        joint.category = JointCategory::cross;
    joint.family = joint_family(joint.category, cosine < std::sin(5.0 * Tolerance::PI / 180.0));

    return joint;
}

JointCategory plate_category(int joint_type) {

    switch (joint_type / 10) {
    case 1:
        return joint_type == 12 ? JointCategory::side_side_inplane : joint_type == 13 ? JointCategory::side_side_rotated : JointCategory::side_side_outplane;
    case 2:
        return JointCategory::side_top;
    case 3:
        return JointCategory::cross;
    case 4:
        return JointCategory::top_top;
    case 6:
        return JointCategory::border;
    }

    return JointCategory::none;
}

JointFamily joint_family(JointCategory category, bool square) {

    switch (category) {
    case JointCategory::end_end_inplane:
        return JointFamily::splicing;
    case JointCategory::end_end_outplane:
    case JointCategory::side_side_outplane:
    case JointCategory::side_side_rotated:
        return JointFamily::corner;
    case JointCategory::side_end:
        return square ? JointFamily::t : JointFamily::angled;
    case JointCategory::side_top:
    case JointCategory::end_top:
        return JointFamily::t;
    case JointCategory::cross:
        return JointFamily::cross_lap;
    case JointCategory::side_side:
        return JointFamily::specialty;
    case JointCategory::side_side_inplane:
        return JointFamily::edge;
    case JointCategory::border:
        return JointFamily::border;
    case JointCategory::top_top:
        return JointFamily::lap;
    case JointCategory::none:
        break;
    }

    return JointFamily::none;
}

}
