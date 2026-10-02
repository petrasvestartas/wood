#include "pch.h"
#include "src/templates/floor/floor_geometry.h"
#include "wood_brep_drill.h"

using namespace session_cpp;

namespace wood_floor {

using namespace wood_floor::geometry;

/// Places an element, names it and adds it under the group.
void add_placed(wood_session::WoodSession& session, const std::shared_ptr<Element>& element, const Xform& placement, const std::string& name, const std::shared_ptr<TreeNode>& group) {

    element->place(placement);
    element->name = name;
    session.add(element, group);
}

/// The member outlines of one group as elements: ribs and inner beams as variable beams, every other group as plates.
std::vector<std::shared_ptr<Element>> to_members(const std::string& group, const std::vector<Outline>& outlines) {

    std::vector<std::shared_ptr<Element>> members;

    for (const Outline& outline : outlines)
        if (group == "outer_ribs" || group == "inner_ribs")
            members.push_back(to_rib(outline, group));
        else if (group == "inner_beams")
            members.push_back(to_beam(outline, {0, 3}, {1, 2}, group));
        else
            members.push_back(to_plate(outline, group));

    return members;
}

// ═══════════════════════════════════════════════════════════════════════════
// Models
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<TreeNode> add_group(wood_session::WoodSession& session, const std::string& name, const std::shared_ptr<TreeNode>& parent) {

    std::shared_ptr<TreeNode> node = std::make_shared<TreeNode>(name);
    session.Session::add(node, parent);

    return node;
}

std::shared_ptr<wood_session::Column> add_column_model(wood_session::WoodSession& session, const FloorGuide& guide, const Xform& placement, const std::shared_ptr<TreeNode>& group, const std::string& suffix) {

    const std::shared_ptr<wood_session::Support> support = to_support(guide);
    const std::shared_ptr<wood_session::Column> column = to_column(guide, *support);
    add_placed(session, support, placement, "support" + suffix, group);
    add_placed(session, column, placement, "column" + suffix, group);

    const std::shared_ptr<wood_session::Joint> joint = wood_session::Joint::support(*support, *column);
    session.add(joint, group);
    session.add_joint(joint);

    for (const std::shared_ptr<wood_session::Joint>& cutter : to_column_cutters(guide, *column)) {
        cutter->place(placement);
        session.add(cutter, group);
        session.add_joint(cutter);
    }

    return column;
}

void add_quarter_model(wood_session::WoodSession& session, const FloorGuide& guide, const Xform& placement, const std::shared_ptr<TreeNode>& group, const std::string& suffix) {

    const Xform lift = placement * Xform::translation(0.0, 0.0, guide.bay_height);
    const std::vector<std::pair<std::string, std::vector<Outline>>> sections = {
        {"beds", guide.beds()},
        {"tsections", guide.tsections()},
        {"outer_ribs", guide.outer_ribs()},
        {"inner_ribs", guide.inner_ribs()},
        {"wedges_inner_beams", guide.wedges_inner_beams()},
        {"inner_beams", guide.inner_beams()},
    };

    for (const std::pair<std::string, std::vector<Outline>>& section : sections) {
        const std::shared_ptr<TreeNode> node = add_group(session, section.first + suffix, group);
        const std::vector<std::shared_ptr<Element>> members = to_members(section.first, section.second);
        std::map<int, std::shared_ptr<TreeNode>> rows;
        std::map<int, int> counts;

        for (size_t i = 0; i < members.size(); i++) {
            const int row = section.second[i].row;

            if (row < 0) {
                add_placed(session, members[i], lift, fmt::format("{}_{}{}", section.first, i, suffix), node);
                continue;
            }

            if (!rows.count(row))
                rows[row] = add_group(session, fmt::format("{}_{}{}", section.first, row, suffix), node);

            add_placed(session, members[i], lift, fmt::format("{}_{}_{}{}", section.first, row, counts[row]++, suffix), rows[row]);
        }
    }
}

/// compas_tf's computed_thickness of a member outline: the distance between the area centroids of its two loops.
double outline_thickness(const Outline& outline) {
    return (area_centroid(outline.top) - area_centroid(outline.bottom)).magnitude();
}

/// The thickness of a wedge ring member by its name, inner_beams_<k>_<quarter> or oculus_<i>; zero for any other member.
double ring_thickness(const FloorGuide& guide, const std::string& name) {

    if (name.starts_with("inner_beams_"))
        return outline_thickness(guide.inner_beams()[name[12] - '0']);

    if (name.starts_with("oculus_") && name.size() == 8 && name[7] < '4')
        return outline_thickness(guide.oculus()[name[7] - '0']);

    return 0.0;
}

void add_oculus_model(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group) {

    const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
    const std::vector<Outline> outlines = guide.oculus();

    for (size_t i = 0; i < outlines.size(); i++) {
        const std::shared_ptr<Element> member = i < 4 ? std::static_pointer_cast<Element>(to_beam(outlines[i], {1, 0}, {2, 3}, "oculus")) : std::static_pointer_cast<Element>(to_plate(outlines[i], "oculus"));
        add_placed(session, member, lift, fmt::format("oculus_{}", i), group);
    }
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_wedges(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group) {

    std::vector<std::shared_ptr<wood_session::BeamVariable>> ring;

    for (const std::shared_ptr<wood_session::BeamVariable>& beam : session.beam_variables())
        if (ring_thickness(guide, beam->name) > 0.0)
            ring.push_back(beam);

    std::vector<std::shared_ptr<wood_session::JointBeam>> wedges;

    for (size_t i = 0; i < ring.size(); i++)
        for (size_t j = i + 1; j < ring.size(); j++) {
            const std::shared_ptr<wood_session::InteractionContactFace> contact = session.compute_face_contact(ring[i], ring[j]);

            if (!contact || contact->type != wood_session::ContactType::side_side)
                continue;

            const double thickness = std::max(ring_thickness(guide, ring[i]->name), ring_thickness(guide, ring[j]->name));
            const std::shared_ptr<wood_session::JointBeam> wedge = wood_session::JointBeam::wedge(*ring[i], *ring[j], *contact, 1.5 * thickness, 2.0 * thickness / 3.0);
            wedge->name = fmt::format("connector_wedge_{}", wedges.size());
            session.add(wedge, group);
            session.add_joint(wedge);
            wedges.push_back(wedge);
        }

    return wedges;
}

/// The thickness of a quarter member by its name, <group>_<k>_<quarter> for the wedge blocks, inner beams, outer ribs and inner ribs; zero for any other member.
double member_thickness(const FloorGuide& guide, const std::string& name) {

    const size_t quarter = name.find_last_of('_');
    const size_t index = name.find_last_of('_', quarter - 1);

    if (quarter == std::string::npos || index == std::string::npos)
        return 0.0;

    const std::string group = name.substr(0, index);
    const int k = name[index + 1] - '0';

    if (group == "wedges_inner_beams")
        return outline_thickness(guide.wedges_inner_beams()[k]);

    if (group == "inner_beams")
        return outline_thickness(guide.inner_beams()[k]);

    if (group == "outer_ribs")
        return outline_thickness(guide.outer_ribs()[k]);

    if (group == "inner_ribs")
        return outline_thickness(guide.inner_ribs()[k]);

    return 0.0;
}

/// Whether the dowel itself runs inside the solid: a stretch of its line inside it overlaps the dowel.
static bool crosses(const Mesh& solid, const Line& dowel) {

    for (const std::array<double, 2>& stretch : wood_session::inside_stretches(solid, dowel))
        if (stretch[1] > 1e-6 && stretch[0] < dowel.length() - 1e-6)
            return true;

    return false;
}

/// How much of the dowel lies inside the solid, the stretch of its line through its middle clipped to it.
static double depth_inside(const Mesh& solid, const Line& dowel) {

    const double half = 0.5 * dowel.length();

    for (const std::array<double, 2>& stretch : wood_session::inside_stretches(solid, dowel))
        if (stretch[0] <= half + 1e-6 && stretch[1] >= half - 1e-6)
            return std::min(stretch[1], dowel.length()) - std::max(stretch[0], 0.0);

    return 0.0;
}

/// Moves every two dowels of the quarter's sets that meet end to end in one member from its two faces, antiparallel and within align of one axis, onto their common axis; returns how many pairs moved.
static size_t align_dowels(const std::vector<std::shared_ptr<wood_session::JointBeam>>& sets, const std::map<std::string, std::shared_ptr<Element>>& members, double align) {

    size_t aligned = 0;

    for (size_t s = 0; s < sets.size(); s++)
        for (size_t t = s + 1; t < sets.size(); t++)
            for (const std::string& shared : sets[s]->targets) {
                if (std::find(sets[t]->targets.begin(), sets[t]->targets.end(), shared) == sets[t]->targets.end())
                    continue;

                const Mesh& solid = members.at(shared)->element_geometry_mesh();

                for (Line& d : sets[s]->drill_lines)
                    for (Line& e : sets[t]->drill_lines) {
                        const Vector direction = d.to_vector().normalized();
                        const Vector offset = e.center() - d.center();
                        const Vector lateral = offset - direction * offset.dot(direction);

                        if (direction.dot(e.to_vector().normalized()) > -0.999 || std::abs(std::abs(offset.dot(direction)) - d.length()) > 1e-6 || lateral.magnitude() > align || !crosses(solid, d) || !crosses(solid, e))
                            continue;

                        d = Line::from_points(d.start() + lateral * 0.5, d.end() + lateral * 0.5);
                        e = Line::from_points(e.start() - lateral * 0.5, e.end() - lateral * 0.5);
                        aligned++;
                    }
            }

    return aligned;
}

/// Whether two dowels meet end to end on one axis, their bores one through bore.
static bool end_to_end(const Line& a, const Line& b) {

    const Vector direction = a.to_vector().normalized();
    const Vector offset = b.center() - a.center();

    return direction.dot(b.to_vector().normalized()) < -0.999 && (offset - direction * offset.dot(direction)).magnitude() < 1e-6;
}

/// Names every dowel of the sets that does not fit: less than half its length inside a member it joins, or within a diameter of another dowel in one member; a dowel whose bore would run into another's, closer than two radii, is dropped from the later set and named so, since the two bores could not both be exact.
static void report_misfits(std::vector<std::shared_ptr<wood_session::JointBeam>>& sets, const std::map<std::string, std::shared_ptr<Element>>& members, std::vector<std::string>& misfits) {

    for (size_t s = 0; s < sets.size(); s++)
        for (size_t i = 0; i < sets[s]->drill_lines.size(); i++)
            for (const std::string& target : sets[s]->targets) {
                const Line& dowel = sets[s]->drill_lines[i];
                const double depth = depth_inside(members.at(target)->element_geometry_mesh(), dowel);

                if (depth < 0.5 * dowel.length() - 1e-3)
                    misfits.push_back(fmt::format("{} dowel {}: only {:.1f} of its {:.0f} inside {} at ({:.0f} {:.0f} {:.0f})", sets[s]->name, i, depth, 0.5 * dowel.length(), members.at(target)->name, dowel.center()[0], dowel.center()[1], dowel.center()[2]));
            }

    for (size_t t = 0; t < sets.size(); t++)
        for (size_t j = 0; j < sets[t]->drill_lines.size();) {
            bool dropped = false;

            for (size_t s = 0; s < t && !dropped; s++)
                for (const std::string& shared : sets[s]->targets) {
                    if (std::find(sets[t]->targets.begin(), sets[t]->targets.end(), shared) == sets[t]->targets.end())
                        continue;

                    const Mesh& solid = members.at(shared)->element_geometry_mesh();
                    const Line& b = sets[t]->drill_lines[j];

                    for (size_t i = 0; i < sets[s]->drill_lines.size() && !dropped; i++) {
                        const Line& a = sets[s]->drill_lines[i];

                        if (end_to_end(a, b) || !crosses(solid, a) || !crosses(solid, b))
                            continue;

                        const double distance = wood_session::segment_distance(a.start(), a.end(), b.start(), b.end());

                        if (distance < 2.0 * sets[t]->line_radius) {
                            misfits.push_back(fmt::format("{} dowel {} dropped: {:.1f} from {} dowel {} in {} at ({:.0f} {:.0f} {:.0f})", sets[t]->name, j, distance, sets[s]->name, i, members.at(shared)->name, b.center()[0], b.center()[1], b.center()[2]));
                            dropped = true;
                        } else if (distance < 4.0 * sets[t]->line_radius)
                            misfits.push_back(fmt::format("{} dowel {} and {} dowel {}: {:.1f} apart in {}", sets[s]->name, i, sets[t]->name, j, distance, members.at(shared)->name));
                    }
                }

            if (dropped)
                sets[t]->drill_lines.erase(sets[t]->drill_lines.begin() + j);
            else
                j++;
        }

    std::erase_if(sets, [](const std::shared_ptr<wood_session::JointBeam>& set) { return set->drill_lines.empty(); });
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_quarter_dowels(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group, double radius, double length, double offset, double align, std::vector<std::string>* misfits) {

    std::map<std::string, std::vector<std::shared_ptr<Element>>> quarters;
    std::map<std::string, std::shared_ptr<Element>> members;

    for (const std::shared_ptr<Element>& element : session.world_elements())
        if (member_thickness(guide, element->name) > 0.0) {
            quarters[element->name.substr(element->name.find_last_of('_'))].push_back(element);
            members[element->guid()] = element;
        }

    std::vector<std::shared_ptr<wood_session::JointBeam>> joints;

    for (const std::pair<const std::string, std::vector<std::shared_ptr<Element>>>& quarter : quarters) {
        std::vector<std::shared_ptr<wood_session::JointBeam>> sets;

        for (size_t i = 0; i < quarter.second.size(); i++)
            for (size_t j = i + 1; j < quarter.second.size(); j++) {
                const std::shared_ptr<Element>& a = quarter.second[i];
                const std::shared_ptr<Element>& b = quarter.second[j];
                const std::shared_ptr<wood_session::InteractionContactFace> contact = session.compute_face_contact(a, b);
                const std::shared_ptr<wood_session::JointBeam> dowels = contact ? wood_session::JointBeam::dowels(*a, *b, *contact, radius, length, offset) : nullptr;

                if (!dowels)
                    continue;

                dowels->name = fmt::format("connector_dowels_{}", joints.size() + sets.size());
                sets.push_back(dowels);
            }

        align_dowels(sets, members, align);
        std::vector<std::string> found;
        report_misfits(sets, members, misfits ? *misfits : found);

        for (const std::shared_ptr<wood_session::JointBeam>& set : sets) {
            session.add(set, group);
            session.add_joint(set);
            joints.push_back(set);
        }
    }

    return joints;
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_cross_laps(wood_session::WoodSession& session, const std::vector<std::shared_ptr<wood_session::JointBeam>>& plates, const std::shared_ptr<TreeNode>& group) {

    std::vector<std::shared_ptr<wood_session::JointBeam>> laps;

    for (size_t i = 0; i < plates.size(); i++)
        for (size_t j = i + 1; j < plates.size(); j++) {
            if (plates[i]->targets.front() != plates[j]->targets.front())
                continue;

            const std::shared_ptr<wood_session::JointBeam> lap = wood_session::JointBeam::cross_lap(*plates[i], *plates[j]);
            lap->name = fmt::format("connector_cross_lap_{}", laps.size());
            session.add(lap, group);
            session.add_joint(lap);
            laps.push_back(lap);
        }

    return laps;
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_rectangle_plates(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group) {

    std::vector<std::shared_ptr<wood_session::JointBeam>> plates;

    for (const std::shared_ptr<wood_session::Column>& column : session.columns())
        for (const std::shared_ptr<wood_session::BeamVariable>& rib : session.beam_variables()) {
            if (!rib->name.starts_with("outer_ribs_"))
                continue;

            const std::shared_ptr<wood_session::InteractionContactFace> contact = session.compute_face_contact(column, rib);

            if (!contact)
                continue;

            const double thickness = outline_thickness(guide.outer_ribs()[rib->name[11] - '0']);
            const std::shared_ptr<wood_session::JointBeam> plate = wood_session::JointBeam::rectangle_plate(*column, *rib, *contact, thickness);
            plate->name = fmt::format("connector_{}", plates.size());
            session.add(plate, group);
            session.add_joint(plate);
            plates.push_back(plate);
        }

    return plates;
}

std::vector<std::shared_ptr<wood_session::JointBeam>> add_ties(wood_session::WoodSession& session, const std::shared_ptr<TreeNode>& group) {

    std::vector<std::shared_ptr<wood_session::BeamVariable>> ribs;

    for (const std::shared_ptr<wood_session::BeamVariable>& beam : session.beam_variables())
        if (beam->name.starts_with("outer_ribs_"))
            ribs.push_back(beam);

    std::vector<std::shared_ptr<wood_session::JointBeam>> ties;

    for (size_t i = 0; i < ribs.size(); i++)
        for (size_t j = i + 1; j < ribs.size(); j++) {
            const std::shared_ptr<wood_session::InteractionContactFace> contact = session.compute_face_contact(ribs[i], ribs[j]);

            if (!contact || contact->type != wood_session::ContactType::end_end)
                continue;

            const std::shared_ptr<wood_session::JointBeam> tie = wood_session::JointBeam::tie(*ribs[i], *ribs[j], *contact);
            tie->name = fmt::format("outer_rib_connector_{}", ties.size());
            session.add(tie, group);
            session.add_joint(tie);
            ties.push_back(tie);
        }

    return ties;
}

}
