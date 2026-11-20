#include "src/templates/truss/carpentry_truss.h"
#include "wood_joint_classification.h"

namespace wood_truss {

CarpentryTruss::CarpentryTruss(const TrussGuide& guide, double thickness, const std::string& name)
    : WoodSession(name),
      guide(guide),
      thickness(thickness) {

    // members: a beam per chord segment and per web, a rectangle thickness deep across the plane
    add_members();

    // joints: every pair classified, the joint its category asks for cut into both
    add_joints();
}

void CarpentryTruss::add_members() {

    for (size_t c = 0; c < guide.chords.size(); c++)
        for (size_t s = 0; s + 1 < guide.chords[c].axis.point_count(); s++)
            add_member(Line::from_points(guide.chords[c].axis.get_point(s), guide.chords[c].axis.get_point(s + 1)), guide.chords[c].radius, fmt::format("chord_{}_{}", c, s), "chords");

    for (size_t w = 0; w < guide.webs.size(); w++)
        add_member(Line::from_points(guide.webs[w].start(), guide.webs[w].end()), guide.webs[w].radius, fmt::format("web_{}", w), "webs");
}

void CarpentryTruss::add_member(const Line& axis, double radius, const std::string& name, const std::string& group) {

    const Vector half_across = Vector::y_axis() * (0.5 * thickness);
    const Vector half_in_plane = Vector::y_axis().cross(axis.to_vector()).normalized() * radius;
    const Point start = axis.start();
    const Polyline section({start - half_across - half_in_plane, start + half_across - half_in_plane, start + half_across + half_in_plane, start - half_across + half_in_plane});
    add(EdgeStraightElement::from_section(axis, section.closed(), name), group_named(group));
}

void CarpentryTruss::add_joints() {

    const std::vector<std::shared_ptr<EdgeStraightElement>> beams = get_elements<EdgeStraightElement>();

    for (size_t i = 0; i < beams.size(); i++)
        for (size_t j = i + 1; j < beams.size(); j++) {
            const JointClassification meeting = classify_joint(*beams[i], *beams[j]);
            if (meeting.category == JointCategory::none)
                continue;

            // two ends at a node another member runs past: each bears on that member
            bool carried = false;
            for (size_t k = 0; k < beams.size() && meeting.category == JointCategory::end_end_outplane; k++) {
                const JointClassification past = classify_joint(*beams[k], *beams[i]);
                carried = carried || (past.category == JointCategory::side_end && !past.ends[0] && Point::distance(past.point, meeting.point) < beams[k]->radius(0));
            }
            if (carried)
                continue;

            if (meeting.category != JointCategory::side_end)
                throw std::runtime_error(fmt::format("{} and {}: no joint for {} yet", beams[i]->name, beams[j]->name, to_string(meeting.category)));

            const std::shared_ptr<EdgeStraightElement> end = meeting.ends[0] ? beams[i] : beams[j];
            const std::shared_ptr<EdgeStraightElement> side = meeting.ends[0] ? beams[j] : beams[i];
            const std::shared_ptr<JointBeamElement> joint = JointBeamElement::from_library(meeting.family == JointFamily::t ? "blind_mortise_tenon" : "notched_oblique_thrust", *end, *side);
            joint->name = fmt::format("{}_{}_{}", joint->design, end->name, side->name);
            add(joint, group_named("joints"));
            add_interaction(joint, end, joint->interaction(0));
            add_interaction(joint, side, joint->interaction(1));
        }
}

}
