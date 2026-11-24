#include "pch.h"
#include "src/templates/truss/truss.h"
#include "wood_element_geometry.h"
#include "wood_joint_classification.h"
#include "convex_hull.h"
#include "boolean_polyline.h"
#include "wood_element_joint_cut_plane.h"

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// Truss
// ═══════════════════════════════════════════════════════════════════════════
Truss::Truss(const TrussGuide& guide, const std::string& name) : WoodSession(name), guide(guide) {

    // input: the guide's lines as drawn
    add_input();

    // members: a beam per chord run and per web
    add_chords();
    add_webs();

    // chords: the runs mitered at kinks, butted at heels
    add_chord_butts();

    // webs: on the chords, lapped where they cross, mitered with their neighbours at every node
    add_web_butts();

    // joinery: every end on a side the book's joint in place of its butt, all able to go in across the plane
    add_joinery();
}

/// A member of the truss, flush faced: half_thick across the truss plane (normal) like every member, half_depth in it.
static std::shared_ptr<EdgeStraightElement> flush(const Point& start, const Point& end, const Vector& normal, double half_thick, double half_depth, const std::string& name) {

    const Vector across = normal.normalized();
    const Vector in_plane = across.cross((end - start).normalized()).normalized();
    const Polyline section({
        start + across * half_thick + in_plane * half_depth,
        start - across * half_thick + in_plane * half_depth,
        start - across * half_thick - in_plane * half_depth,
        start + across * half_thick - in_plane * half_depth,
        start + across * half_thick + in_plane * half_depth,
    });

    return EdgeStraightElement::from_section(Line::from_points(start, end), section, name);
}

/// How far member's section reaches from its axis along direction.
static double extent(const EdgeStraightElement& member, const Vector& direction) {

    double reach = 0.0;
    const Point base = member.axis.get_point(0);
    for (const Point& corner : member.sections().front().get_points())
        reach = std::max(reach, std::abs((corner - base).dot(direction)));

    return reach;
}

void Truss::add_joinery() {

    // no butt of the draft stays: every one goes, its features with it
    for (const std::shared_ptr<JointBeamElement>& butt : get_elements<JointBeamElement>()) {
        if (!butt->is_butt)
            continue;
        for (const std::string& target : butt->targets)
            remove_interaction(butt, get_element<Element>(target));
        remove_object(butt->guid());
    }

    // the book's joints where every node is one member ending on another, else a plate at every node
    if (book_fits())
        add_book_joinery();
    else
        add_plated_joinery();
}

std::vector<Point> Truss::nodes() const {

    // every end of a member's axis
    std::vector<Point> found;
    for (const std::shared_ptr<EdgeStraightElement>& member : get_elements<EdgeStraightElement>())
        for (const Point& end : {member->axis.get_point(0), member->axis.get_point(member->axis.point_count() - 1)}) {
            bool known = false;
            for (const Point& node : found)
                known = known || Point::distance(node, end) < NODE_TOLERANCE;
            if (!known)
                found.push_back(end);
        }

    return found;
}

std::array<std::vector<std::shared_ptr<EdgeStraightElement>>, 2> Truss::meeting(const Point& node) const {

    // who ends at the node, and who runs through it
    std::array<std::vector<std::shared_ptr<EdgeStraightElement>>, 2> found;
    for (const std::shared_ptr<EdgeStraightElement>& member : get_elements<EdgeStraightElement>()) {
        const Point first = member->axis.get_point(0);
        const Point last = member->axis.get_point(member->axis.point_count() - 1);
        if (ends_at(*member, node))
            found[0].push_back(member);
        else if (Point::distance(Line::from_points(first, last).closest_point(node).second, node) < NODE_TOLERANCE)
            found[1].push_back(member);
    }

    return found;
}

std::shared_ptr<EdgeStraightElement> Truss::carrier_at(const Point& node) const {

    // the member running through
    const std::array<std::vector<std::shared_ptr<EdgeStraightElement>>, 2> met = meeting(node);
    if (!met[1].empty())
        return met[1].front();

    // else a post ending here
    for (const std::shared_ptr<EdgeStraightElement>& member : met[0])
        if (member->name.rfind("web_", 0) == 0 && steepness(*member) > 0.99)
            return member;

    // else the lowest chord (a heel's tie)
    std::shared_ptr<EdgeStraightElement> carrier;
    for (const std::shared_ptr<EdgeStraightElement>& member : met[0])
        if (member->name.rfind("chord_", 0) == 0 && (!carrier || lowness(*member) < lowness(*carrier)))
            carrier = member;
    if (carrier)
        return carrier;

    // else the steepest web
    for (const std::shared_ptr<EdgeStraightElement>& member : met[0])
        if (!carrier || steepness(*member) > steepness(*carrier))
            carrier = member;

    return carrier;
}

bool Truss::book_fits() const {

    // every node one member ending on another: no two ending in one, no chord's kink, no three crossing at a point
    for (const Point& node : nodes()) {
        const std::array<std::vector<std::shared_ptr<EdgeStraightElement>>, 2> met = meeting(node);
        if (met[0].size() + met[1].size() < 2)
            continue;
        const std::shared_ptr<EdgeStraightElement> carrier = carrier_at(node);
        const size_t ending = met[0].size() - (std::find(met[0].begin(), met[0].end(), carrier) != met[0].end() ? 1 : 0);
        if (ending >= 2)
            return false;
        for (const std::shared_ptr<EdgeStraightElement>& member : met[0])
            for (const std::shared_ptr<EdgeStraightElement>& other : met[0])
                if (member != other && same_chord(*member, *other))
                    return false;
    }
    for (const std::pair<Point, std::vector<std::shared_ptr<EdgeStraightElement>>>& crossing : crossings())
        if (crossing.second.size() >= 3)
            return false;

    return true;
}

void Truss::add_book_joinery() {

    const std::vector<TrussBar> bars = guide.bars();
    std::map<JointRole, size_t> turns;
    for (const Point& node : nodes()) {
        const std::array<std::vector<std::shared_ptr<EdgeStraightElement>>, 2> met = meeting(node);
        if (met[0].empty() || met[0].size() + met[1].size() < 2)
            continue;

        // the carrier, running on past the node when it ends there, as far as the end jointed into it reaches along it
        const std::shared_ptr<EdgeStraightElement> carrier = carrier_at(node);
        std::shared_ptr<EdgeStraightElement> end;
        for (const std::shared_ptr<EdgeStraightElement>& member : met[0])
            if (member != carrier)
                end = member;
        if (!end)
            continue;
        if (met[1].empty()) {
            const double half = extent(*carrier, plane_normal().cross(away_from(*carrier, node)).normalized());
            const double along = std::abs(away_from(*end, node).dot(away_from(*carrier, node))) * through_length(*end, *carrier, node);
            add_run_on(carrier, node, std::max(RUN_ON * half, along + extent(*end, plane_normal().cross(away_from(*end, node)).normalized())));
        }

        // the end's joint of the book, by what it does: a heel, a push at an angle or square, a pulling post or other pull
        const bool chords = end->name.rfind("chord_", 0) == 0 && carrier->name.rfind("chord_", 0) == 0;
        const bool heel = chords && steepness(*carrier) < 0.05 && steepness(*end) > 0.05;
        const Vector along_end = away_from(*end, node);
        const Vector along_carrier = (carrier->axis.get_point(1) - carrier->axis.get_point(0)).normalized();
        const bool square = std::abs(along_end.dot(along_carrier)) < std::sin(5.0 * Tolerance::PI / 180.0);
        const bool pulls = pull_at(bars, *end, node) > 1e-6;
        const bool post = std::abs(along_end[2]) > 0.999;
        const bool flat = steepness(*end) < std::sin(30.0 * Tolerance::PI / 180.0);
        const JointRole role = heel           ? (flat ? JointRole::flat_heel : JointRole::heel)
                               : pulls && post ? JointRole::hanging_post
                               : pulls        ? JointRole::pulling
                               : square       ? JointRole::square_push
                                              : JointRole::oblique_push;
        add_role_joint(role, end, carrier, node, turns);
    }

    // crossings: two members passing through each other where neither ends, halved into each other from the faces; square a
    // cross lap (p. 646), oblique the notched oblique cross laps in turn (pp. 672, 676)
    const std::array<std::string, 2> oblique = {"oblique_notched_cross_lap", "double_notched_oblique_cross_lap"};
    size_t turn = 0;
    for (const std::pair<Point, std::vector<std::shared_ptr<EdgeStraightElement>>>& crossing : crossings()) {
        const std::shared_ptr<EdgeStraightElement>& first = crossing.second[0];
        const std::shared_ptr<EdgeStraightElement>& second = crossing.second[1];
        const Vector along_first = first->axis.get_point(first->axis.point_count() - 1) - first->axis.get_point(0);

        // the member rising to the right first, so every crossing of a lattice opens to the same faces
        const bool rising = along_first[2] * along_first[0] >= 0.0;
        const std::shared_ptr<EdgeStraightElement>& a = rising ? first : second;
        const std::shared_ptr<EdgeStraightElement>& b = rising ? second : first;
        const Vector along_a = (a->axis.get_point(1) - a->axis.get_point(0)).normalized();
        const Vector along_b = (b->axis.get_point(1) - b->axis.get_point(0)).normalized();
        const bool square = std::abs(along_a.dot(along_b)) < std::sin(5.0 * Tolerance::PI / 180.0);
        std::shared_ptr<JointBeamElement> lap = square ? nullptr : JointBeamElement::from_library(oblique[turn % 2], *a, *b);
        std::string design = oblique[turn % 2];
        if (!lap) {
            lap = JointBeamElement::from_library("cross_lap", *a, *b);
            design = "cross_lap";
        }
        else
            turn++;
        if (!lap)
            continue;
        lap->name = fmt::format("{}_{}_{}", design, a->name, b->name);
        add(lap, group_named("joinery"));
        add_interaction(lap, a, lap->interaction(0));
        add_interaction(lap, b, lap->interaction(1));
    }
}

void Truss::add_plated_joinery() {

    // every member split at every node on it: the ends of the others and the points where members cross, so each piece runs
    // from node to node
    split_at_nodes();

    // a short piece left hanging past a node (a tie running on past its heel) goes: the truss ends at the node
    for (const std::shared_ptr<EdgeStraightElement>& member : get_elements<EdgeStraightElement>()) {
        const Vector in_plane = plane_normal().cross((member->axis.get_point(1) - member->axis.get_point(0)).normalized()).normalized();
        bool hanging = false;
        for (const Point& end : {member->axis.get_point(0), member->axis.get_point(member->axis.point_count() - 1)})
            hanging = hanging || meeting(end)[0].size() + meeting(end)[1].size() == 1;
        if (hanging && member->axis.length() < OVERHANG * extent(*member, in_plane))
            remove_object(member->guid());
    }

    for (const Point& node : nodes())
        add_plated_node(node);
}

void Truss::add_plated_node(const Point& node) {

    {
        // the members ending here, kept: a loop over a part of the returned array would outlive it (before C++23)
        const std::array<std::vector<std::shared_ptr<EdgeStraightElement>>, 2> met = meeting(node);
        std::vector<std::shared_ptr<EdgeStraightElement>> chords;
        std::vector<std::shared_ptr<EdgeStraightElement>> webs;
        for (const std::shared_ptr<EdgeStraightElement>& member : met[0])
            (member->name.rfind("chord_", 0) == 0 ? chords : webs).push_back(member);
        if (chords.size() + webs.size() < 2)
            return;

        // the chords mitered with each other; two meeting sharp (a heel): the steeper running on, bearing on the flatter, both cut
        // by one plane square to the flatter at the node, so the heel ends in one face
        bool heel_here = false;
        for (size_t k = 1; k < chords.size(); k++)
            for (size_t j = 0; j < k; j++)
                if (!add_miter(chords[j], chords[k], node)) {
                    heel_here = true;
                    const bool first_flatter = steepness(*chords[j]) < steepness(*chords[k]);
                    const std::shared_ptr<EdgeStraightElement>& flatter = first_flatter ? chords[j] : chords[k];
                    const std::shared_ptr<EdgeStraightElement>& steeper_chord = first_flatter ? chords[k] : chords[j];

                    // the steeper running on past the node first, so its cut at the heel's face is whole
                    add_run_on(steeper_chord, node, RUN_ON * extent(*steeper_chord, plane_normal().cross(away_from(*steeper_chord, node)).normalized()) / std::max(std::sin(std::acos(std::clamp(away_from(*flatter, node).dot(away_from(*steeper_chord, node)), -1.0, 1.0))), 0.2));
                    // on the flatter run on past the heel, whose plane its end lies in: the steeper loses all beyond it anyway
                    bear(flatter, steeper_chord, node, 2.0 * half_thickness());
                    const std::shared_ptr<JointCutPlaneElement> heel = std::make_shared<JointCutPlaneElement>(
                        Plane::from_point_normal(node, away_from(*flatter, node)), 4.0 * half_thickness(), fmt::format("heel_cut_{:.0f}_{:.0f}", node[0], node[2]));
                    heel->is_visible = false;
                    add(heel, group_named("joinery"));
                    const Mesh beyond_heel = cut_away(node, away_from(*flatter, node));
                    add_interaction(heel, flatter, std::make_shared<InteractionFeatureSolid>(beyond_heel, SolidOperation::subtract));
                    add_interaction(heel, steeper_chord, std::make_shared<InteractionFeatureSolid>(beyond_heel, SolidOperation::subtract));
                }

        // the webs mitered with each other; two meeting sharp: the flatter bearing on the steeper
        std::vector<std::array<std::shared_ptr<EdgeStraightElement>, 2>> sharp;
        for (size_t k = 1; k < webs.size(); k++)
            for (size_t j = 0; j < k; j++)
                if (!add_miter(webs[j], webs[k], node))
                    sharp.push_back(steepness(*webs[j]) >= steepness(*webs[k]) ? std::array<std::shared_ptr<EdgeStraightElement>, 2>{webs[j], webs[k]} : std::array<std::shared_ptr<EdgeStraightElement>, 2>{webs[k], webs[j]});

        // a chord ending here with no other chord (an outer corner) runs on to the far face of the webs, so the corner is wood
        for (const std::shared_ptr<EdgeStraightElement>& chord : chords) {
            if (chords.size() > 1 || webs.empty())
                continue;
            double reach = 0.0;
            for (const std::shared_ptr<EdgeStraightElement>& web : webs) {
                const double sine = away_from(*chord, node).cross(away_from(*web, node)).magnitude();
                reach = std::max(reach, extent(*web, plane_normal().cross(away_from(*web, node)).normalized()) / std::max(sine, 0.2));
            }
            add_run_on(chord, node, reach);
        }

        // then every web flush on the chords' faces; where the chords are mitered pieces of one chord, on each piece run on past
        // the node into the next: into a straight next piece by a full depth (half of it at most), so no cut ends near the web,
        // past a kink by PROUD
        const std::optional<Point> joined = chords.size() > 1 && !heel_here ? std::optional<Point>(node) : std::nullopt;
        for (const std::shared_ptr<EdgeStraightElement>& chord : chords) {
            double past = PROUD;
            for (const std::shared_ptr<EdgeStraightElement>& other : chords)
                if (away_from(*chord, node).dot(away_from(*other, node)) < -0.9998)
                    past = std::max(past, std::min(2.0 * half_thickness(), 0.5 * other->axis.length()));
            for (const std::shared_ptr<EdgeStraightElement>& web : webs)
                bear(chord, web, joined, past);
        }
        for (const std::array<std::shared_ptr<EdgeStraightElement>, 2>& pair : sharp)
            bear(pair[0], pair[1]);

        // one plate let into them all, pinned: the chords first
        std::vector<std::shared_ptr<EdgeStraightElement>> members = chords;
        members.insert(members.end(), webs.begin(), webs.end());
        add_node_plate(members, node);
    }
}

void Truss::bear(
    const std::shared_ptr<EdgeStraightElement>& holding,
    const std::shared_ptr<EdgeStraightElement>& bearing,
    const std::optional<Point>& joined,
    double past
) {

    // what holding is so far, mitered and cut; its drawn stock where that is no closed solid or the cut is refused
    Mesh body = holding->model_geometry_mesh();
    if (body.number_of_faces() == 0 || !body.is_closed())
        body = holding->element_geometry_mesh();

    // taken PROUD past the truss's faces: every member is as thick as the truss, so a cut flush with them would share their
    // faces, and the boolean leaves shared faces in slivers
    const Point mid = guide.chords.front().axis.get_point(0);
    const Vector across = plane_normal();
    const Vector up(0.0, 0.0, 1.0);
    const Vector along = up.cross(across).normalized();
    Xform proud = Xform::frame_to_world(mid, along, up, across) *
                  Xform::scale_xyz(1.0, 1.0, (half_thickness() + PROUD) / half_thickness()) *
                  Xform::world_to_frame(mid, along, up, across);

    // and past its end at joined, from its far end: the piece taking over there is cut flush with that end, and two cuts
    // meeting on one face leave a fin of bearing between them
    if (joined) {
        const Vector away = away_from(*holding, *joined);
        const Point first = holding->axis.get_point(0);
        const Point last = holding->axis.get_point(holding->axis.point_count() - 1);
        const Point far = Point::distance(first, *joined) < Point::distance(last, *joined) ? last : first;
        const Vector side = across.cross(away).normalized();
        const double length = Point::distance(first, last);
        proud = Xform::frame_to_world(far, side, across, away) * Xform::scale_xyz(1.0, 1.0, (length + past) / length) *
                Xform::world_to_frame(far, side, across, away) * proud;
    }

    try {
        add_interaction(holding, bearing, std::make_shared<InteractionFeatureSolid>(body.transformed(proud), SolidOperation::subtract));
    } catch (const std::invalid_argument&) {
        add_interaction(
            holding, bearing, std::make_shared<InteractionFeatureSolid>(holding->element_geometry_mesh().transformed(proud), SolidOperation::subtract)
        );
    }
}

bool Truss::add_miter(const std::shared_ptr<EdgeStraightElement>& first, const std::shared_ptr<EdgeStraightElement>& second, const Point& node) {

    // two pieces of one line cut square on the node, each keeping its side (another's miter may have run one past it); under
    // MITER_ANGLE a miter would run far along both: not mitered
    const double cosine = away_from(*first, node).dot(away_from(*second, node));
    if (cosine < -0.9998) {
        add_interaction(second, first, std::make_shared<InteractionFeatureSolid>(cut_away(node, away_from(*first, node)), SolidOperation::subtract));
        add_interaction(first, second, std::make_shared<InteractionFeatureSolid>(cut_away(node, away_from(*second, node)), SolidOperation::subtract));
        return true;
    }
    if (cosine > std::cos(MITER_ANGLE * Tolerance::PI / 180.0))
        return false;
    // a miter whose cut for either comes out empty or open (a degenerate meeting) is none: the two bear instead
    const std::shared_ptr<JointBeamElement> miter = JointBeamElement::butt_side_side(*first, *second);
    if (!miter)
        return false;
    for (const InteractionFeatureSolid& cut : miter->butt_solids)
        if (cut.mesh.number_of_faces() == 0 || !cut.mesh.is_closed())
            return false;
    for (const InteractionFeatureSolid& gain : miter->butt_extensions)
        if (gain.mesh.number_of_faces() > 0 && !gain.mesh.is_closed())
            return false;
    miter->name = fmt::format("miter_{}_{}", first->name, second->name);
    add(miter, group_named("joinery"));
    add_interaction(miter, first, miter->interaction(0));
    add_interaction(miter, second, miter->interaction(1));

    return true;
}

Mesh Truss::cut_away(const Point& node, const Vector& keep) const {

    // a box from the plane through node square to keep, out the other way: what a member losing it keeps is on keep's side. It
    // reaches past every member's wood at the node (a heel's run-on at most) and no farther: the kernel's loft caps scale their
    // corners to whole numbers and overflow past about 3 m
    const Vector across = plane_normal();
    const Vector along = across.cross(keep).normalized();
    const double reach = 1400.0;
    const Polyline face({
        node + along * reach + across * reach,
        node - along * reach + across * reach,
        node - along * reach - across * reach,
        node + along * reach - across * reach,
        node + along * reach + across * reach,
    });

    return Mesh::loft({face}, {face.translated(keep * -reach)}, true);
}

void Truss::split_at_nodes() {

    // the nodes: every member's ends and every point where two members cross
    std::vector<Point> points = nodes();
    for (const std::pair<Point, std::vector<std::shared_ptr<EdgeStraightElement>>>& crossing : crossings())
        points.push_back(crossing.first);

    // each member cut at every node strictly inside it, the pieces <name>_<k> in order along it
    for (const std::shared_ptr<EdgeStraightElement>& member : get_elements<EdgeStraightElement>()) {
        const Point first = member->axis.get_point(0);
        const Point last = member->axis.get_point(member->axis.point_count() - 1);
        const Line axis = Line::from_points(first, last);
        std::vector<double> cuts;
        for (const Point& point : points) {
            const std::pair<double, Point> closest = axis.closest_point(point);
            if (Point::distance(closest.second, point) < NODE_TOLERANCE && Point::distance(point, first) > NODE_TOLERANCE && Point::distance(point, last) > NODE_TOLERANCE)
                cuts.push_back(closest.first);
        }
        if (cuts.empty())
            continue;
        std::sort(cuts.begin(), cuts.end());
        std::vector<Point> stops = {first};
        for (const double cut : cuts)
            if (Point::distance(stops.back(), axis.point_at(cut)) > NODE_TOLERANCE)
                stops.push_back(axis.point_at(cut));
        stops.push_back(last);
        const Vector in_plane = plane_normal().cross((last - first).normalized()).normalized();
        const double depth = extent(*member, in_plane);
        const std::string name = member->name;
        const std::string group = name.rfind("chord_", 0) == 0 ? "chords" : "webs";
        remove_object(member->guid());
        for (size_t k = 0; k + 1 < stops.size(); k++)
            add(flush(stops[k], stops[k + 1], plane_normal(), half_thickness(), depth, fmt::format("{}_{}", name, k)), group_named(group));
    }
}

std::vector<std::pair<Point, std::vector<std::shared_ptr<EdgeStraightElement>>>> Truss::crossings() const {

    // the points where members pass through each other, neither ending there, and who passes each
    const std::vector<std::shared_ptr<EdgeStraightElement>> members = get_elements<EdgeStraightElement>();
    std::vector<std::pair<Point, std::vector<std::shared_ptr<EdgeStraightElement>>>> found;
    for (size_t i = 0; i < members.size(); i++)
        for (size_t j = i + 1; j < members.size(); j++) {
            const Line first = Line::from_points(members[i]->axis.get_point(0), members[i]->axis.get_point(members[i]->axis.point_count() - 1));
            const Line second = Line::from_points(members[j]->axis.get_point(0), members[j]->axis.get_point(members[j]->axis.point_count() - 1));
            double t_first = 0.0;
            double t_second = 0.0;
            if (std::abs(first.to_vector().normalized().dot(second.to_vector().normalized())) > 0.9999 ||
                !Intersection::line_line_parameters(first, second, t_first, t_second, 1.0, true, false))
                continue;
            const Point crossing = first.point_at(t_first);
            if (Point::distance(crossing, second.point_at(t_second)) > NODE_TOLERANCE || ends_at(*members[i], crossing) || ends_at(*members[j], crossing))
                continue;
            bool known = false;
            for (std::pair<Point, std::vector<std::shared_ptr<EdgeStraightElement>>>& known_crossing : found)
                if (Point::distance(known_crossing.first, crossing) < NODE_TOLERANCE) {
                    for (const std::shared_ptr<EdgeStraightElement>& member : {members[i], members[j]})
                        if (std::find(known_crossing.second.begin(), known_crossing.second.end(), member) == known_crossing.second.end())
                            known_crossing.second.push_back(member);
                    known = true;
                }
            if (!known)
                found.push_back({crossing, {members[i], members[j]}});
        }

    return found;
}

std::shared_ptr<EdgeStraightElement> Truss::web_member(size_t w) const {

    // its own, or the web it was joined into: the one whose axis holds both its nodes
    const std::shared_ptr<EdgeStraightElement> own = get_element_by_name<EdgeStraightElement>(fmt::format("web_{}", w));
    if (own)
        return own;
    for (const std::shared_ptr<EdgeStraightElement>& member : get_elements<EdgeStraightElement>()) {
        const Line axis = Line::from_points(member->axis.get_point(0), member->axis.get_point(member->axis.point_count() - 1));
        bool holds = member->name.rfind("web_", 0) == 0;
        for (const Point& node : {guide.webs[w].start(), guide.webs[w].end()})
            holds = holds && Point::distance(axis.closest_point(node).second, node) < NODE_TOLERANCE;
        if (holds)
            return member;
    }

    // else the web nearest its middle (a web the drawing gives twice or of no length): the draft's butts only need one
    const Point middle = guide.webs[w].start() + (guide.webs[w].end() - guide.webs[w].start()) * 0.5;
    std::shared_ptr<EdgeStraightElement> nearest;
    double gap = 1e300;
    for (const std::shared_ptr<EdgeStraightElement>& member : get_elements<EdgeStraightElement>()) {
        const Line axis = Line::from_points(member->axis.get_point(0), member->axis.get_point(member->axis.point_count() - 1));
        const double distance = Point::distance(axis.closest_point(middle).second, middle);
        if (member->name.rfind("web_", 0) == 0 && distance < gap) {
            gap = distance;
            nearest = member;
        }
    }

    return nearest;
}

void Truss::add_node_plate(const std::vector<std::shared_ptr<EdgeStraightElement>>& members, const Point& node) {

    // where each member is clear of the others along its axis from the node, and the plate two diameters past its row of pins:
    // as small as its pins allow
    std::vector<const EdgeStraightElement*> raw;
    std::vector<double> clear;
    std::vector<double> reaches;
    for (const std::shared_ptr<EdgeStraightElement>& member : members) {
        double free = 0.0;
        for (const std::shared_ptr<EdgeStraightElement>& other : members)
            if (other != member)
                free = std::max(free, clear_of(*member, *other, node, 0.0));
        raw.push_back(member.get());
        clear.push_back(free);
        reaches.push_back(free + 4.0 * 2.0 * PIN_RADIUS);
    }

    const std::shared_ptr<FacePlanarElement> plate = JointBeamElement::node_plate(raw, node, plane_normal(), reaches, 2.0 * half_thickness() / 3.0, PLATE_INSET, fmt::format("node_plate_{:.0f}_{:.0f}", node[0], node[2]));

    // every way a member leaves the node, and the others' outlines in the truss's plane
    const Vector across = plane_normal();
    const Vector x = Vector(0.0, 0.0, 1.0).cross(across).normalized();
    const Vector y = across.cross(x);
    std::vector<double> angles;
    for (const std::shared_ptr<EdgeStraightElement>& member : members)
        angles.push_back(std::atan2(away_from(*member, node).dot(y), away_from(*member, node).dot(x)));
    std::vector<Point> outline;
    for (const Point& corner : plate->polylines[0].get_points())
        outline.push_back(Point((corner - node).dot(x), (corner - node).dot(y), 0.0));
    std::vector<Polyline> others;
    std::vector<bool> bases;
    for (const std::shared_ptr<EdgeStraightElement>& member : get_elements<EdgeStraightElement>()) {
        if (std::find(members.begin(), members.end(), member) != members.end())
            continue;
        bases.push_back(member->name.rfind("chord_0_0", 0) == 0);
        const Point first = member->axis.get_point(0);
        const Point last = member->axis.get_point(member->axis.point_count() - 1);
        const Vector side = across.cross((last - first).normalized()).normalized() * extent(*member, across.cross((last - first).normalized()).normalized());
        std::vector<Point> corners;
        for (const Point& corner : {first + side, last + side, last - side, first - side, first + side})
            corners.push_back(Point((corner - node).dot(x), (corner - node).dot(y), 0.0));
        others.push_back(Polyline(corners));
    }

    // the plate's way in, in the truss's plane: square to the node's chord away from the members when its path is clear and it
    // keeps PLATE_KEEP_ANGLE off them, else the clear way farthest off them, at least PLATE_OFF_ANGLE (nearer one, its slot runs
    // along it and the plate cannot leave it)
    std::shared_ptr<EdgeStraightElement> chord = members.front();
    const Vector square = across.cross((chord->axis.get_point(1) - chord->axis.get_point(0)).normalized()).normalized();
    Vector toward(0.0, 0.0, 0.0);
    for (const std::shared_ptr<EdgeStraightElement>& member : members)
        if (member != chord)
            toward = toward + away_from(*member, node);
    const double sense = square.dot(toward) > 0.0 ? -1.0 : 1.0;
    const double base = std::atan2(square.dot(y) * sense, square.dot(x) * sense);
    double chosen = base;
    double best = -1.0;
    for (double turn = 0.0; turn < 2.0 * Tolerance::PI; turn += 5.0 * Tolerance::PI / 180.0) {
        const double angle = base + turn;
        double nearest = Tolerance::PI;
        for (const double leaving : angles)
            nearest = std::min(nearest, std::abs(std::remainder(angle - leaving, 2.0 * Tolerance::PI)));
        if (nearest <= best || nearest < PLATE_OFF_ANGLE * Tolerance::PI / 180.0 || (best >= PLATE_KEEP_ANGLE * Tolerance::PI / 180.0 && turn > 0.0))
            continue;
        std::vector<Point> path = outline;
        for (const Point& corner : outline)
            path.push_back(Point(corner[0] + 1500.0 * std::cos(angle), corner[1] + 1500.0 * std::sin(angle), 0.0));
        std::vector<Point> band = ConvexHull::hull_2d(path);
        band.push_back(band.front());
        bool clear_path = true;
        for (const Polyline& other : others)
            clear_path = clear_path && BooleanPolyline::compute_count(Polyline(band), other, 0) == 0;
        if (clear_path) {
            chosen = angle;
            best = nearest;
        }
    }

    // no way clear of the others: the way past the fewest of them, the first chord's counted first (they come off before it in
    // the assembly, but the first chord is the base it is built on), then the farthest off the node's members, the middle of the
    // widest gap
    if (best < 0.0) {
        size_t fewest = 1000 * (others.size() + 1);
        for (double turn = 0.0; turn < 2.0 * Tolerance::PI; turn += 5.0 * Tolerance::PI / 180.0) {
            const double angle = base + turn;
            double nearest = Tolerance::PI;
            for (const double leaving : angles)
                nearest = std::min(nearest, std::abs(std::remainder(angle - leaving, 2.0 * Tolerance::PI)));
            if (nearest < PLATE_OFF_ANGLE * Tolerance::PI / 180.0)
                continue;
            std::vector<Point> path = outline;
            for (const Point& corner : outline)
                path.push_back(Point(corner[0] + 1500.0 * std::cos(angle), corner[1] + 1500.0 * std::sin(angle), 0.0));
            std::vector<Point> band = ConvexHull::hull_2d(path);
            band.push_back(band.front());
            size_t met = 0;
            for (size_t o = 0; o < others.size(); o++)
                if (BooleanPolyline::compute_count(Polyline(band), others[o], 0) > 0)
                    met += bases[o] ? 1000 : 1;
            if (met < fewest || (met == fewest && nearest > best)) {
                chosen = angle;
                best = nearest;
                fewest = met;
            }
        }
    }

    // the plate run on along that way to the members' outer surface, flush with it, kept within them; its slot exactly it, cut
    // in the middle of the thickness only
    const Vector way = x * std::cos(chosen) + y * std::sin(chosen);
    const std::shared_ptr<FacePlanarElement> flush_plate = JointBeamElement::node_plate(raw, node, plane_normal(), reaches, 2.0 * half_thickness() / 3.0, PLATE_INSET, plate->name, 0.0, way, 1500.0);
    const std::shared_ptr<JointBeamElement> pins = JointBeamElement::slotted_plate(raw, *flush_plate, node, way, clear, PIN_RADIUS);
    add(flush_plate, group_named("joinery"));
    add(pins, group_named("joinery"));
    for (size_t m = 0; m < members.size(); m++)
        add_interaction(pins, members[m], pins->interaction(m));
    add_interaction(pins, flush_plate, pins->interaction(members.size()));
}

double Truss::clear_of(const EdgeStraightElement& member, const EdgeStraightElement& other, const Point& node, double run_on) const {

    // along member's axis from the node, where its section leaves other's: beside it (square to other's axis), or, other ending
    // at the node and member leaving away from it, behind its end and any run-on
    const Vector along = away_from(member, node);
    const Vector onward = away_from(other, node);
    const double cosine = along.dot(onward);
    const double sine = std::sqrt(std::max(0.0, 1.0 - cosine * cosine));
    const double half = extent(member, plane_normal().cross(along).normalized());
    const double other_half = extent(other, plane_normal().cross(onward).normalized());
    double clear = sine > 1e-6 ? (other_half + half * std::abs(cosine)) / sine : 1e300;
    if (ends_at(other, node) && cosine < -1e-6)
        clear = std::min(clear, (half * sine + run_on) / -cosine);

    // two leaving along one line one way lie in each other: no side to be clear of; never past the member's own far end
    if (sine <= 1e-6 && cosine > 0.0)
        clear = 0.0;

    return std::min(clear, member.axis.length());
}

bool Truss::ends_at(const EdgeStraightElement& member, const Point& node) {

    return Point::distance(member.axis.get_point(0), node) < NODE_TOLERANCE ||
           Point::distance(member.axis.get_point(member.axis.point_count() - 1), node) < NODE_TOLERANCE;
}

bool Truss::same_chord(const EdgeStraightElement& a, const EdgeStraightElement& b) {

    // chord_<c>_<s>: the same c
    if (a.name.rfind("chord_", 0) != 0 || b.name.rfind("chord_", 0) != 0)
        return false;

    return a.name.substr(0, a.name.rfind('_')) == b.name.substr(0, b.name.rfind('_'));
}

double Truss::half_thickness() const {

    double half = 0.0;
    for (const TrussMember& chord : guide.chords)
        half = std::max(half, chord.radius);

    return half;
}

Vector Truss::plane_normal() const {

    // the truss is drawn in a vertical plane: its normal square to the span and to z
    const std::vector<Point> points = guide.chords.front().axis.get_points();
    const Vector span = points.back() - points.front();

    return Vector(0.0, 0.0, 1.0).cross(span).normalized();
}

Vector Truss::away_from(const EdgeStraightElement& member, const Point& node) {

    const Point first = member.axis.get_point(0);
    const Point last = member.axis.get_point(member.axis.point_count() - 1);

    return (Point::distance(first, node) < Point::distance(last, node) ? last - first : first - last).normalized();
}

double Truss::steepness(const EdgeStraightElement& member) {

    return std::abs((member.axis.get_point(member.axis.point_count() - 1) - member.axis.get_point(0)).normalized()[2]);
}

double Truss::lowness(const EdgeStraightElement& member) {

    return 0.5 * (member.axis.get_point(0)[2] + member.axis.get_point(member.axis.point_count() - 1)[2]);
}

double Truss::pull_at(const std::vector<TrussBar>& bars, const EdgeStraightElement& member, const Point& node) const {

    // the bar of the guide that lies along the member and touches the node
    const Vector along = away_from(member, node);
    for (const TrussBar& bar : bars) {
        const bool touches = Point::distance(bar.start, node) < NODE_TOLERANCE || Point::distance(bar.end, node) < NODE_TOLERANCE;
        if (touches && std::abs((bar.end - bar.start).normalized().dot(along)) > 0.9999)
            return bar.force;
    }

    return 0.0;
}

Mesh Truss::add_run_on(const std::shared_ptr<EdgeStraightElement>& member, const Point& node, double length) {

    // the member's own drawn stock moved and stretched to run from its middle to length past the node, added: the wood a joint
    // into it needs beyond the node; its own faces, not rebuilt, and clear of its far end, so the union leaves no sliver
    const Vector out = away_from(*member, node) * -1.0;
    const Point first = member->axis.get_point(0);
    const Point back = Point::distance(first, node) < NODE_TOLERANCE ? member->axis.get_point(member->axis.point_count() - 1) : first;
    const Vector across = plane_normal();
    const Vector side = across.cross(out).normalized();
    const double span = Point::distance(back, node);
    const double drawn = member->axis.length();
    const Mesh stock = member->element_geometry_mesh().transformed(
        Xform::frame_to_world(back, side, across, out) * Xform::translation(0.0, 0.0, 0.5 * span) *
        Xform::scale_xyz(1.0, 1.0, (0.5 * span + length) / drawn) * Xform::world_to_frame(back, side, across, out)
    );
    const std::shared_ptr<JointBeamElement> run_on = std::make_shared<JointBeamElement>();
    run_on->is_butt = true;
    run_on->butt_solids = {InteractionFeatureSolid(stock, SolidOperation::add), InteractionFeatureSolid()};
    run_on->name = fmt::format("run_on_{}_{:.0f}_{:.0f}", member->name, node[0], node[2]);
    add(run_on, group_named("joinery"));
    add_interaction(run_on, member, run_on->interaction(0));

    return run_on->butt_solids[0].mesh;
}

std::shared_ptr<JointBeamElement> Truss::add_role_joint(
    JointRole role,
    const std::shared_ptr<EdgeStraightElement>& end,
    const std::shared_ptr<EdgeStraightElement>& side,
    const Point& node,
    std::map<JointRole, size_t>& turns
) {

    // the role's designs, the next in turn first, each built as drawn and with its face turned: the first whose member goes in
    // along the way every member does (dropped on the bench, against the plane normal) is kept; else the first that fits
    const std::vector<std::string>& designs = ROLE_DESIGNS.at(role);
    const Vector way = plane_normal() * -1.0;
    std::shared_ptr<JointBeamElement> fallback;
    std::string fallback_design;
    for (size_t attempt = 0; attempt < designs.size(); attempt++) {
        const std::string& design = designs[(turns[role] + attempt) % designs.size()];
        for (const double face : {1.0, -1.0}) {
            const std::shared_ptr<JointBeamElement> joint = build_library_joint(design, {{"face", face}}, *end, *side, node);
            if (!joint)
                continue;
            bool along = false;
            for (const Vector& direction : joint->insertion_directions(0))
                along = along || direction.dot(way) > 0.999;
            if (along) {
                turns[role] += attempt + 1;
                return add_built_joint(joint, design, end, side, node);
            }
            if (!fallback) {
                fallback = joint;
                fallback_design = design;
            }
        }
    }
    if (!fallback)
        return nullptr;
    turns[role]++;

    return add_built_joint(fallback, fallback_design, end, side, node);
}

double Truss::through_length(const EdgeStraightElement& end, const EdgeStraightElement& side, const Point& node) const {

    // how far the end runs on past its node to clear the side's far face, its section's corners included
    const Vector into = away_from(end, node) * -1.0;
    const Vector along_side = (side.axis.get_point(1) - side.axis.get_point(0)).normalized();
    const double sine = std::max(into.cross(along_side).magnitude(), 0.2);
    const Vector in_plane = plane_normal().cross(along_side).normalized();
    const double end_depth = extent(end, plane_normal().cross(into).normalized());

    return (extent(side, in_plane) + end_depth * std::abs(into.dot(along_side))) / sine + 2.0;
}

std::shared_ptr<JointBeamElement> Truss::build_library_joint(
    const std::string& design,
    const std::map<std::string, double>& given,
    EdgeStraightElement& end,
    const EdgeStraightElement& side,
    const Point& node
) const {

    // the joint made on the end as if its stock ran on through the side, so its cuts reach through it
    // a design whose cuts cannot be made at this angle (a cutter left open) is one that does not fit
    const double kept = end.extension;
    end.extension = through_length(end, side, node);
    std::shared_ptr<JointBeamElement> joint;
    try {
        joint = JointBeamElement::from_library(design, end, side, given);
    } catch (const std::exception&) {
        joint = nullptr;
    }
    end.extension = kept;

    return joint;
}

std::shared_ptr<JointBeamElement> Truss::add_built_joint(
    const std::shared_ptr<JointBeamElement>& joint,
    const std::string& design,
    const std::shared_ptr<EdgeStraightElement>& end,
    const std::shared_ptr<EdgeStraightElement>& side,
    const Point& node
) {

    // the end runs on through the side, the wood the joint carves its tail from
    add_run_on(end, node, through_length(*end, *side, node));
    joint->name = fmt::format("{}_{}_{}", design, end->name, side->name);
    add(joint, group_named("joinery"));
    add_interaction(joint, end, joint->interaction(0));
    add_interaction(joint, side, joint->interaction(1));

    return joint;
}

void Truss::add_input() {

    for (const std::vector<TrussMember>* members : {&guide.chords, &guide.webs})
        for (const TrussMember& member : *members) {
            std::shared_ptr<Polyline> drawn = std::make_shared<Polyline>(member.axis);
            drawn->linecolor = Color(0.85f, 0.1f, 0.1f);
            drawn->width = 4.0;
            add_polyline(drawn, group_named("input"));
        }
}

void Truss::add_chords() {

    for (size_t c = 0; c < guide.chords.size(); c++) {
        const std::vector<std::array<Point, 2>> runs = guide.runs(c);
        for (size_t s = 0; s < runs.size(); s++)
            add(flush(runs[s][0], runs[s][1], plane_normal(), half_thickness(), guide.chords[c].radius, fmt::format("chord_{}_{}", c, s)), group_named("chords"));
    }
}

void Truss::add_webs() {

    // webs in line end to end are one member running through their node (a post through a chord's kink)
    std::vector<bool> joined(guide.webs.size(), false);
    for (size_t w = 0; w < guide.webs.size(); w++) {
        if (joined[w])
            continue;
        Point start = guide.webs[w].start();
        Point end = guide.webs[w].end();
        double radius = guide.webs[w].radius;
        bool grew = true;
        while (grew) {
            grew = false;
            for (size_t v = w + 1; v < guide.webs.size(); v++) {
                const Point first = guide.webs[v].start();
                const Point last = guide.webs[v].end();
                if (joined[v] || std::abs((last - first).normalized().dot((end - start).normalized())) < 0.9999)
                    continue;
                const bool after = Point::distance(first, end) < NODE_TOLERANCE || Point::distance(last, end) < NODE_TOLERANCE;
                const bool before = Point::distance(first, start) < NODE_TOLERANCE || Point::distance(last, start) < NODE_TOLERANCE;
                if (!after && !before)
                    continue;
                const Point far = Point::distance(first, after ? end : start) < NODE_TOLERANCE ? last : first;
                (after ? end : start) = far;
                radius = std::max(radius, guide.webs[v].radius);
                joined[v] = true;
                grew = true;
            }
        }
        add(flush(start, end, plane_normal(), half_thickness(), radius, fmt::format("web_{}", w)), group_named("webs"));
    }
}

void Truss::add_butt(
    const std::shared_ptr<JointBeamElement>& butt,
    const std::shared_ptr<EdgeStraightElement>& a,
    const std::shared_ptr<EdgeStraightElement>& b
) {

    if (!butt)
        return;
    butt->name = fmt::format("{}_{}_{}", butt->name, a->name, b->name);
    add(butt, group_named("butts"));
    add_interaction(butt, a, butt->interaction(0));
    add_interaction(butt, b, butt->interaction(1));
}

namespace {

/// Whether a point lies at a node: within the tolerance of another.
bool at(const Point& point, const Point& node) {
    return Point::distance(point, node) < Truss::NODE_TOLERANCE;
}

/// Whether a point lies on a run between its ends, clear of both.
bool inside(const Point& point, const std::array<Point, 2>& run) {

    const Vector along = run[1] - run[0];
    const double t = (point - run[0]).dot(along) / along.dot(along);
    const Point foot = run[0] + along * t;

    return t > 0.0 && t < 1.0 && Point::distance(point, foot) < Truss::NODE_TOLERANCE &&
           !at(point, run[0]) && !at(point, run[1]);
}

/// Signed 2D cross product of oa and ob for monotone-chain convex hull turns.
double hull_turn(
    const std::array<double, 2>& o,
    const std::array<double, 2>& a,
    const std::array<double, 2>& b
) {
    return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0]);
}

/// Sweep a member's solid along away through the truss plane so a member stopping on it loses everything behind its near face.
Mesh shadow(const Mesh& member, const Vector& away, const Plane& plane) {

    std::vector<std::array<double, 2>> points;
    double depth = 0.0;

    if (member.number_of_faces() == 0)
        return Mesh();

    for (const Point& vertex : member.to_vertices_and_faces().first) {
        const Vector from = vertex - plane.origin();
        const std::array<double, 2> flat = {from.dot(plane.x_axis()), from.dot(plane.y_axis())};
        points.push_back(flat);
        points.push_back({flat[0] + away.dot(plane.x_axis()), flat[1] + away.dot(plane.y_axis())});
        depth = std::max(depth, std::abs(from.dot(plane.z_axis())));
    }

    // the convex hull of the member and its swept copy, a monotone chain, points closer than a tenth of a millimetre one
    for (std::array<double, 2>& point : points)
        point = {std::round(point[0] * 10.0) / 10.0, std::round(point[1] * 10.0) / 10.0};

    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());

    std::vector<std::array<double, 2>> hull(2 * points.size());
    size_t k = 0;

    for (size_t i = 0; i < points.size(); i++) {
        while (k >= 2 && hull_turn(hull[k - 2], hull[k - 1], points[i]) <= 1.0)
            k--;

        hull[k++] = points[i];
    }

    for (size_t i = points.size() - 1, low = k + 1; i-- > 0;) {
        while (k >= low && hull_turn(hull[k - 2], hull[k - 1], points[i]) <= 1.0)
            k--;

        hull[k++] = points[i];
    }

    if (k < 4)
        return Mesh();

    hull.resize(k - 1);

    std::vector<Point> loop;

    for (const std::array<double, 2>& point : hull)
        loop.push_back(plane.origin() + plane.x_axis() * point[0] + plane.y_axis() * point[1] -
                       plane.z_axis() * (2.0 * depth + 10.0));

    const Polyline bottom = Polyline(loop).closed();

    return Mesh::loft({bottom}, {bottom.translated(plane.z_axis() * (4.0 * depth + 20.0))}, true);
}

/// Build the half-space prism beyond a cut through point facing out in the plane with normal.
Mesh beyond(
    const Point& point,
    const Vector& out,
    const Vector& normal,
    double size
) {

    const Vector across = normal.cross(out);
    const Polyline square = Polyline({point + across * size - normal * size,
                                      point - across * size - normal * size,
                                      point - across * size + out * size - normal * size,
                                      point + across * size + out * size - normal * size})
                                .closed();

    return Mesh::loft({square}, {square.translated(normal * (2.0 * size))}, true);
}

/// Count how many chord runs have an endpoint at node.
size_t runs_ending(
    const std::vector<std::pair<std::array<Point, 2>, std::shared_ptr<EdgeStraightElement>>>& runs,
    const Point& node
) {

    size_t count = 0;

    for (const std::pair<std::array<Point, 2>, std::shared_ptr<EdgeStraightElement>>& run : runs)
        count += at(node, run.first[0]) || at(node, run.first[1]);

    return count;
}

/// Project a 3D vector onto the x and y axes of the truss plane.
std::array<double, 2> in_plane(const Plane& plane, const Vector& vector) {
    return std::array<double, 2>{vector.dot(plane.x_axis()), vector.dot(plane.y_axis())};
}

/// Return the 2D scalar cross product of a and b.
double cross_2d(const std::array<double, 2>& a, const std::array<double, 2>& b) {
    return a[0] * b[1] - a[1] * b[0];
}

/// Find or insert a truss graph vertex within NODE_TOLERANCE and return its index.
size_t vertex_of(std::vector<Point>& vertices, const Point& point) {

    for (size_t n = 0; n < vertices.size(); n++)
        if (at(point, vertices[n]))
            return n;

    vertices.push_back(point);

    return vertices.size() - 1;
}

/// Record that inner stops on and is let into outer.
void let_in(
    std::vector<std::pair<
        std::shared_ptr<EdgeStraightElement>,
        std::shared_ptr<EdgeStraightElement>>>& lets,
    const std::shared_ptr<EdgeStraightElement>& outer,
    const std::shared_ptr<EdgeStraightElement>& inner
) {
    lets.emplace_back(outer, inner);
}

/// Order incident beams around a node by ascending departure angle.
bool angle_less(
    const std::pair<double, std::shared_ptr<EdgeStraightElement>>& a,
    const std::pair<double, std::shared_ptr<EdgeStraightElement>>& b
) {
    return a.first < b.first;
}

/// Return whether any member passing straight through a node separates angles a and b.
bool apart(const std::vector<std::array<double, 2>>& passing, double a, double b) {

    for (const std::array<double, 2>& through : passing)
        if (cross_2d(through, {std::cos(a), std::sin(a)}) *
                cross_2d(through, {std::cos(b), std::sin(b)}) <
            0.0)
            return true;

    return false;
}

/// Order let-in pairs from outermost to innermost inner-member rank.
struct InnerRankLess {
    std::map<const EdgeStraightElement*, int>* ranks = nullptr; // Rank table per straight beam.

    /// Return whether pair a's inner member has a smaller rank than pair b's.
    bool operator()(
        const std::pair<
            std::shared_ptr<EdgeStraightElement>,
            std::shared_ptr<EdgeStraightElement>>& a,
        const std::pair<
            std::shared_ptr<EdgeStraightElement>,
            std::shared_ptr<EdgeStraightElement>>& b
    ) const {
        return (*ranks)[a.second.get()] < (*ranks)[b.second.get()];
    }
};

} // namespace

void Truss::add_chord_butts() {

    struct Run {
        size_t chord;
        std::array<Point, 2> ends;
        std::shared_ptr<EdgeStraightElement> beam;
    };

    std::vector<Run> runs;
    for (size_t c = 0; c < guide.chords.size(); c++) {
        const std::vector<std::array<Point, 2>> ends = guide.runs(c);
        for (size_t s = 0; s < ends.size(); s++)
            runs.push_back({c, ends[s], get_element_by_name<EdgeStraightElement>(fmt::format("chord_{}_{}", c, s))}
            );
    }

    // kinks: consecutive runs of one chord on the miter
    for (size_t i = 0; i + 1 < runs.size(); i++)
        if (runs[i].chord == runs[i + 1].chord)
            add_butt(
                JointBeamElement::butt_side_side(*runs[i].beam, *runs[i + 1].beam),
                runs[i].beam,
                runs[i + 1].beam
            );

    // heels: where chord ends meet, the one running on reaches past the node (TrussGuide::runs) and the others end on its
    // run, top_side, below; ends meeting with none running on, as at an apex of one slope or a gentle bend, a miter; an end at
    // another chord's bend, on that chord's surface; two chords bending at one node, a half lap
    for (size_t i = 0; i < runs.size(); i++)
        for (size_t j = i + 1; j < runs.size(); j++) {
            if (runs[i].chord == runs[j].chord)
                continue;
            std::optional<Point> heel;
            for (const Point& a : runs[i].ends)
                for (const Point& b : runs[j].ends)
                    if (at(a, b))
                        heel = a;
            if (heel) {
                const std::array<std::vector<Point>, 2> polylines = {
                    guide.chords[runs[i].chord].axis.get_points(), guide.chords[runs[j].chord].axis.get_points()
                };
                std::array<bool, 2> bent = {false, false};
                for (size_t k = 0; k < 2; k++)
                    for (size_t v = 1; v + 1 < polylines[k].size(); v++)
                        bent[k] = bent[k] || at(polylines[k][v], *heel);
                if (bent[0] != bent[1]) {

                    // one chord bends here, the other ends on it: the ending run loses the bending run's shadow
                    const Run& through = bent[0] ? runs[i] : runs[j];
                    const Run& ending = bent[0] ? runs[j] : runs[i];
                    const Point far = at(ending.ends[0], *heel) ? ending.ends[1] : ending.ends[0];
                    const Vector away = *heel - far;
                    const Vector normal = (through.ends[1] - through.ends[0]).cross(away).normalized();
                    const std::shared_ptr<JointBeamElement> joint = JointBeamElement::butt_top_side(*through.beam, *ending.beam);
                    if (joint) {
                        joint->butt_extensions[1] = InteractionFeatureSolid();
                        joint->butt_solids[1] = InteractionFeatureSolid(
                            shadow(
                                through.beam->model_geometry_mesh(),
                                away,
                                Plane::from_point_normal(*heel, normal)
                            ),
                            SolidOperation::subtract
                        );
                    }
                    add_butt(joint, through.beam, ending.beam);
                } else if (bent[0])
                    add_butt(JointBeamElement::butt_cross(*runs[i].beam, *runs[j].beam), runs[i].beam, runs[j].beam);
                else
                    add_butt(JointBeamElement::butt_side_side(*runs[i].beam, *runs[j].beam), runs[i].beam, runs[j].beam);
            }
            for (const Point& end : runs[j].ends)
                if (inside(end, runs[i].ends))
                    add_butt(
                        JointBeamElement::butt_top_side(*runs[i].beam, *runs[j].beam),
                        runs[i].beam,
                        runs[j].beam
                    );
            for (const Point& end : runs[i].ends)
                if (inside(end, runs[j].ends))
                    add_butt(
                        JointBeamElement::butt_top_side(*runs[j].beam, *runs[i].beam),
                        runs[j].beam,
                        runs[i].beam
                    );
        }

    // ends running on: cut along their end cut (TrussGuide::end_cut), unless square, by a joint with a member they carry
    for (size_t c = 0; c < guide.chords.size(); c++)
        for (size_t side = 0; side < 2; side++) {
            const std::optional<std::array<Point, 2>> cut = guide.end_cut(c, side);
            if (!cut)
                continue;
            const std::vector<Point> points = guide.chords[c].axis.get_points();
            const Point node = side == 0 ? points.front() : points.back();
            const Vector on = (node - (side == 0 ? points[1] : points[points.size() - 2])).normalized();
            if (std::abs(((*cut)[1] - (*cut)[0]).dot(on)) < 1e-6)
                continue;
            std::shared_ptr<EdgeStraightElement> runner;
            std::shared_ptr<EdgeStraightElement> carried;
            for (const Run& run : runs)
                if (run.chord == c && (!runner || side == 1))
                    runner = run.beam;
            for (const Run& run : runs)
                if (run.chord != c && (at(run.ends[0], node) || at(run.ends[1], node)))
                    carried = run.beam;
            for (size_t w = 0; !carried && w < guide.webs.size(); w++)
                if (at(guide.webs[w].start(), node) || at(guide.webs[w].end(), node))
                    carried = web_member(w);
            if (!runner || !carried)
                continue;
            const Vector along = ((*cut)[1] - (*cut)[0]).normalized();
            const Vector normal = on.cross(along).normalized();
            Vector out = along.cross(normal);
            out = out.dot(on) < 0.0 ? out * -1.0 : out;
            const std::shared_ptr<JointBeamElement> joint = JointBeamElement::butt_top_side(*carried, *runner);
            if (joint) {
                joint->name = "end";
                joint->butt_extensions = {InteractionFeatureSolid(), InteractionFeatureSolid()};
                joint->butt_solids = {
                    InteractionFeatureSolid(),
                    InteractionFeatureSolid(
                        beyond(
                            (*cut)[0],
                            out,
                            normal,
                            4.0 * Point::distance((*cut)[0], (*cut)[1]) + 1000.0
                        ),
                        SolidOperation::subtract
                    )
                };
            }
            add_butt(joint, carried, runner);
        }
}

void Truss::add_web_butts() {

    std::vector<std::pair<std::array<Point, 2>, std::shared_ptr<EdgeStraightElement>>> runs;

    for (size_t c = 0; c < guide.chords.size(); c++) {
        const std::vector<std::array<Point, 2>> ends = guide.runs(c);

        for (size_t s = 0; s < ends.size(); s++)
            runs.emplace_back(
                ends[s], get_element_by_name<EdgeStraightElement>(fmt::format("chord_{}_{}", c, s))
            );
    }

    std::vector<std::pair<Line, std::shared_ptr<EdgeStraightElement>>> members;

    for (size_t w = 0; w < guide.webs.size(); w++)
        members.emplace_back(
            Line::from_points(guide.webs[w].start(), guide.webs[w].end()),
            web_member(w)
        );

    std::vector<Point> points;

    for (const TrussMember& chord : guide.chords)
        for (const Point& point : chord.axis.get_points())
            points.push_back(point);

    for (const std::pair<Line, std::shared_ptr<EdgeStraightElement>>& member : members) {
        points.push_back(member.first.start());
        points.push_back(member.first.end());
    }

    std::array<Point, 2> far = {points[0], points[0]};

    for (const Point& a : points)
        for (const Point& b : points)
            if (Point::distance(a, b) > Point::distance(far[0], far[1]))
                far = {a, b};

    const Vector along = (far[1] - far[0]).normalized();
    Point off = far[0];

    for (const Point& point : points)
        if ((point - far[0]).cross(along).magnitude() > (off - far[0]).cross(along).magnitude())
            off = point;

    const Plane plane = Plane::from_point_normal(far[0], along.cross(off - far[0]).normalized());

    std::vector<std::pair<Line, std::shared_ptr<EdgeStraightElement>>> lines;

    for (const std::pair<std::array<Point, 2>, std::shared_ptr<EdgeStraightElement>>& run : runs)
        lines.emplace_back(Line::from_points(run.first[0], run.first[1]), run.second);

    lines.insert(lines.end(), members.begin(), members.end());

    // the graph the lines draw: every end and crossing a vertex, each line split at the vertices on it into edges
    std::vector<Point> vertices;

    for (const std::pair<Line, std::shared_ptr<EdgeStraightElement>>& line : lines) {
        vertex_of(vertices, line.first.start());
        vertex_of(vertices, line.first.end());
    }

    for (size_t i = 0; i < lines.size(); i++)
        for (size_t j = i + 1; j < lines.size(); j++) {
            double t_a = 0.0;
            double t_b = 0.0;

            if (Intersection::line_line_parameters(
                    lines[i].first,
                    lines[j].first,
                    t_a,
                    t_b,
                    NODE_TOLERANCE,
                    false
                ) &&
                t_a > 0.0 && t_a < 1.0 && t_b > 0.0 && t_b < 1.0)
                vertex_of(vertices, lines[i].first.point_at(t_a));
        }

    struct Edge {
        size_t from;
        size_t to;
        size_t line;
    };

    std::vector<Edge> edges;

    for (size_t l = 0; l < lines.size(); l++) {
        const Line& line = lines[l].first;
        std::vector<std::pair<double, size_t>> on = {
            {0.0, vertex_of(vertices, line.start())}, {1.0, vertex_of(vertices, line.end())}
        };

        for (size_t n = 0; n < vertices.size(); n++)
            if (inside(vertices[n], {line.start(), line.end()}))
                on.emplace_back(
                    (vertices[n] - line.start()).dot(line.to_vector()) /
                        line.to_vector().dot(line.to_vector()),
                    n
                );

        std::sort(on.begin(), on.end());

        for (size_t k = 0; k + 1 < on.size(); k++)
            edges.push_back({on[k].second, on[k + 1].second, l});
    }

    std::vector<int> rank(lines.size(), std::numeric_limits<int>::max());
    std::vector<bool> left(edges.size(), true);

    for (int layer = 0;
         std::find(left.begin(), left.end(), true) != left.end() &&
         static_cast<size_t>(layer) <= edges.size();
         layer++) {
        std::vector<bool> walked(vertices.size(), false);
        std::vector<size_t> boundary;
        const size_t max_steps = 4 * edges.size() + 16;

        for (size_t pass = 0; pass < max_steps; pass++) {
            std::optional<size_t> start;

            for (size_t e = 0; e < edges.size(); e++)
                if (left[e])
                    for (const size_t n : {edges[e].from, edges[e].to}) {
                        if (walked[n])
                            continue;

                        const std::array<double, 2> cand =
                            in_plane(plane, vertices[n] - vertices[0]);

                        if (!start) {
                            start = n;
                            continue;
                        }

                        const std::array<double, 2> best =
                            in_plane(plane, vertices[*start] - vertices[0]);

                        if (cand[0] < best[0] - 1e-9 ||
                            (std::abs(cand[0] - best[0]) <= 1e-9 && cand[1] < best[1]))
                            start = n;
                    }

            if (!start)
                break;

            size_t current = *start;
            double back = Tolerance::PI;
            std::optional<size_t> first;

            for (size_t step = 0; step < 4 * edges.size(); step++) {
                std::optional<size_t> next;
                double turn = 0.0;

                for (size_t e = 0; e < edges.size(); e++) {
                    if (!left[e] || (edges[e].from != current && edges[e].to != current))
                        continue;

                    const std::array<double, 2> d = in_plane(
                        plane,
                        vertices[edges[e].from == current ? edges[e].to : edges[e].from] -
                            vertices[current]
                    );
                    double clockwise = std::fmod(
                        back - std::atan2(d[1], d[0]) + 4.0 * Tolerance::PI, 2.0 * Tolerance::PI
                    );
                    clockwise = clockwise < 1e-9 ? 2.0 * Tolerance::PI : clockwise;

                    if (!next || clockwise < turn) {
                        next = e;
                        turn = clockwise;
                    }
                }

                if (!next || (first && *next == *first && current == *start))
                    break;

                if (!first)
                    first = next;

                boundary.push_back(*next);
                const size_t other =
                    edges[*next].from == current ? edges[*next].to : edges[*next].from;
                const std::array<double, 2> d =
                    in_plane(plane, vertices[current] - vertices[other]);
                back = std::atan2(d[1], d[0]);
                current = other;
            }

            std::vector<size_t> stack = {*start};
            walked[*start] = true;

            for (size_t visit_step = 0; !stack.empty() && visit_step <= vertices.size();
                 visit_step++) {
                const size_t n = stack.back();
                stack.pop_back();

                for (size_t e = 0; e < edges.size(); e++)
                    if (left[e] && (edges[e].from == n || edges[e].to == n))
                        for (const size_t m : {edges[e].from, edges[e].to})
                            if (!walked[m]) {
                                walked[m] = true;
                                stack.push_back(m);
                            }
            }
        }

        for (const size_t e : boundary) {
            rank[edges[e].line] = std::min(rank[edges[e].line], layer);
            left[e] = false;
        }

        if (boundary.empty())
            break;
    }

    // the hierarchy given: each member's level in place of the layer it was read from
    std::vector<int> levels;

    for (size_t c = 0; c < guide.chords.size(); c++)
        levels.insert(levels.end(), guide.runs(c).size(), guide.chords[c].level);

    for (const TrussMember& web : guide.webs)
        levels.push_back(web.level);

    std::map<const EdgeStraightElement*, int> ranks;
    std::set<const EdgeStraightElement*> leveled;

    for (size_t l = 0; l < lines.size(); l++) {
        ranks[lines[l].second.get()] = levels[l] > 0 ? levels[l] : rank[l];

        if (levels[l] > 0)
            leveled.insert(lines[l].second.get());
    }

    std::vector<std::pair<std::shared_ptr<EdgeStraightElement>, std::shared_ptr<EdgeStraightElement>>>
        lets;

    // each end on the runs: let into a run it lies on or into each run ending there; at a run's lone end the chord runs on over it unless the
    for (const std::pair<Line, std::shared_ptr<EdgeStraightElement>>& member : members)
        for (const Point& end : {member.first.start(), member.first.end()}) {
            std::vector<std::shared_ptr<EdgeStraightElement>> stops;
            std::vector<Vector> along;
            const Vector from = at(end, member.first.start())
                                    ? member.first.end() - member.first.start()
                                    : member.first.start() - member.first.end();

            for (const std::pair<std::array<Point, 2>, std::shared_ptr<EdgeStraightElement>>& run :
                 runs) {
                const bool at_start = at(end, run.first[0]);
                const bool at_end = at_start || at(end, run.first[1]);

                // a lone run end beyond a run passing the node, on its other side, does not meet the member
                bool other_side = false;

                for (const std::pair<std::array<Point, 2>, std::shared_ptr<EdgeStraightElement>>&
                         passing : runs)
                    if (at_end && runs_ending(runs, end) == 1 && inside(end, passing.first)) {
                        const std::array<double, 2> through =
                            in_plane(plane, (passing.first[1] - passing.first[0]).normalized());
                        const Vector leaving =
                            at_start ? run.first[1] - run.first[0] : run.first[0] - run.first[1];
                        other_side =
                            other_side ||
                            cross_2d(through, in_plane(plane, leaving.normalized())) *
                                    cross_2d(through, in_plane(plane, from.normalized())) <
                                0.0;
                    }

                if (other_side)
                    continue;

                if (inside(end, run.first) || (at_end && runs_ending(runs, end) > 1)) {
                    stops.push_back(run.second);
                    along.push_back(
                        at_end
                            ? (at_start ? run.first[1] - run.first[0] : run.first[0] - run.first[1])
                            : Vector(0.0, 0.0, 0.0)
                    );
                } else if (at_end && ranks[member.second.get()] > ranks[run.second.get()])
                    let_in(lets, run.second, member.second);
                else if (at_end && leveled.count(member.second.get()) &&
                         leveled.count(run.second.get()) &&
                         ranks[member.second.get()] < ranks[run.second.get()])
                    add_butt(
                        JointBeamElement::butt_top_side_end(*member.second, *run.second),
                        member.second,
                        run.second
                    );
                else if (at_end)
                    add_butt(
                        JointBeamElement::butt_top_side_end(*run.second, *member.second),
                        run.second,
                        member.second
                    );
            }

            const Vector away = at(end, member.first.start())
                                    ? member.first.end() - member.first.start()
                                    : member.first.start() - member.first.end();

            for (size_t k = 0; k < stops.size(); k++)
                if (along[k].magnitude() == 0.0) {
                    const std::array<double, 2> through =
                        in_plane(plane, stops[k]->axis.get_lines()[0].to_vector().normalized());
                    const double side = cross_2d(through, in_plane(plane, away.normalized()));

                    for (size_t j = stops.size(); j-- > 0;)
                        if (along[j].magnitude() > 0.0 &&
                            cross_2d(through, in_plane(plane, along[j].normalized())) * side <
                                -0.1) {
                            stops.erase(stops.begin() + j);
                            along.erase(along.begin() + j);
                            k -= j < k ? 1 : 0;
                        }
                }

            for (const std::shared_ptr<EdgeStraightElement>& stop : stops)
                let_in(lets, stop, member.second);
        }

    for (const std::pair<Line, std::shared_ptr<EdgeStraightElement>>& member : members)
        for (const Point& end : {member.first.start(), member.first.end()})
            for (const std::pair<Line, std::shared_ptr<EdgeStraightElement>>& other : members)
                if (inside(end, {other.first.start(), other.first.end()}))
                    let_in(lets, other.second, member.second);

    for (const std::pair<std::array<Point, 2>, std::shared_ptr<EdgeStraightElement>>& run : runs)
        for (const Point& end : run.first)
            for (const std::pair<Line, std::shared_ptr<EdgeStraightElement>>& member : members)
                if (inside(end, {member.first.start(), member.first.end()}) &&
                    ranks[member.second.get()] <= ranks[run.second.get()])
                    let_in(lets, member.second, run.second);

    std::vector<std::pair<Point, std::vector<size_t>>> crossings;

    for (size_t i = 0; i < lines.size(); i++)
        for (size_t j = i + 1; j < lines.size(); j++) {
            const Line& a = lines[i].first;
            const Line& b = lines[j].first;
            double t_a = 0.0;
            double t_b = 0.0;

            if (!Intersection::line_line_parameters(
                    a,
                    b,
                    t_a,
                    t_b,
                    NODE_TOLERANCE,
                    false
                ))
                continue;

            const Point crossing = a.point_at(t_a);

            if (!inside(crossing, {a.start(), a.end()}) || !inside(crossing, {b.start(), b.end()}))
                continue;

            size_t c = 0;

            while (c < crossings.size() && !at(crossings[c].first, crossing))
                c++;

            if (c == crossings.size())
                crossings.push_back({crossing, {}});

            for (const size_t k : {i, j})
                if (std::find(crossings[c].second.begin(), crossings[c].second.end(), k) ==
                    crossings[c].second.end())
                    crossings[c].second.push_back(k);
        }

    // at each crossing every two members lapped, each keeping its own layer of the depth they share, a half each where two cross: one member
    for (const std::pair<Point, std::vector<size_t>>& crossing : crossings) {
        std::vector<size_t> crossed = crossing.second;
        std::sort(crossed.begin(), crossed.end());
        const double depth = 2.0 * half_thickness();

        for (size_t i = 0; i < crossed.size(); i++)
            for (size_t j = i + 1; j < crossed.size(); j++) {
                const double layer = -0.5 * depth + depth * static_cast<double>(i + 1) /
                                                        static_cast<double>(crossed.size());
                const Plane lap = Plane::from_point_normal(
                    crossing.first + plane.z_axis() * layer, plane.z_axis()
                );
                add_butt(
                    JointBeamElement::butt_cross(
                        *lines[crossed[i]].second, *lines[crossed[j]].second, lap
                    ),
                    lines[crossed[i]].second,
                    lines[crossed[j]].second
                );
            }
    }

    std::vector<Point> nodes;

    for (const std::pair<Line, std::shared_ptr<EdgeStraightElement>>& member : members)
        for (const Point& end : {member.first.start(), member.first.end()}) {
            bool known = false;

            for (const Point& node : nodes)
                known = known || at(end, node);

            if (!known)
                nodes.push_back(end);
        }

    // at each node, the members ending there sorted by the angle they leave at: each inner one stopping on every outer one, those of one rank
    for (const Point& node : nodes) {
        std::vector<std::pair<double, std::shared_ptr<EdgeStraightElement>>> around;

        for (const std::pair<Line, std::shared_ptr<EdgeStraightElement>>& member : members) {
            const bool from_start = at(member.first.start(), node);

            if (!from_start && !at(member.first.end(), node))
                continue;

            const Vector away = from_start ? member.first.end() - member.first.start()
                                           : member.first.start() - member.first.end();
            around.emplace_back(
                std::atan2(away.dot(plane.y_axis()), away.dot(plane.x_axis())), member.second
            );
        }

        std::sort(around.begin(), around.end(), angle_less);
        bool on_chord = false;

        for (const std::pair<std::array<Point, 2>, std::shared_ptr<EdgeStraightElement>>& run :
             runs)
            on_chord = on_chord || at(node, run.first[0]) || at(node, run.first[1]) ||
                       inside(node, run.first);

        std::vector<std::array<double, 2>> passing;

        for (const std::pair<Line, std::shared_ptr<EdgeStraightElement>>& line : lines)
            if (inside(node, {line.first.start(), line.first.end()}))
                passing.push_back(in_plane(plane, line.first.to_vector().normalized()));

        for (const std::pair<double, std::shared_ptr<EdgeStraightElement>>& outer : around)
            for (const std::pair<double, std::shared_ptr<EdgeStraightElement>>& inner : around)
                if (ranks[outer.second.get()] < ranks[inner.second.get()])
                    let_in(lets, outer.second, inner.second);

        std::set<int> layers;

        for (const std::pair<double, std::shared_ptr<EdgeStraightElement>>& member : around)
            layers.insert(ranks[member.second.get()]);

        for (const int layer : layers) {
            std::vector<std::pair<double, std::shared_ptr<EdgeStraightElement>>> same;

            for (const std::pair<double, std::shared_ptr<EdgeStraightElement>>& member : around)
                if (ranks[member.second.get()] == layer)
                    same.push_back(member);

            if (same.size() < 2)
                continue;

            size_t widest = 0;
            double gap = -1.0;

            for (size_t k = 0; k < same.size(); k++) {
                const double next =
                    k + 1 < same.size() ? same[k + 1].first : same[0].first + 2.0 * Tolerance::PI;

                if (next - same[k].first > gap) {
                    gap = next - same[k].first;
                    widest = k;
                }
            }

            for (size_t k = 0; k < same.size(); k++) {
                if (k == widest && (on_chord || same.size() <= 2))
                    continue;

                const std::pair<double, std::shared_ptr<EdgeStraightElement>>& a = same[k];
                const std::pair<double, std::shared_ptr<EdgeStraightElement>>& b =
                    same[(k + 1) % same.size()];

                if (!apart(passing, a.first, b.first))
                    add_butt(
                        JointBeamElement::butt_side_side(*a.second, *b.second), a.second, b.second
                    );
            }
        }
    }

    // the lets, inner rank by inner rank: the inner, ending inside the outer and never run on, loses the outer's shadow as already
    // cut (its solid swept along the inner's axis), butting on its near face and keeping nothing behind it
    std::stable_sort(lets.begin(), lets.end(), InnerRankLess{&ranks});
    const size_t max_lets = lets.size();

    for (size_t step = 0; !lets.empty() && step < max_lets; step++) {
        size_t next = 0;

        for (size_t k = lets.size(); k-- > 0;) {
            bool waiting = false;

            for (const std::pair<
                     std::shared_ptr<EdgeStraightElement>,
                     std::shared_ptr<EdgeStraightElement>>& other : lets)
                waiting = waiting || other.second == lets[k].first;

            if (!waiting)
                next = k;
        }

        const std::pair<std::shared_ptr<EdgeStraightElement>, std::shared_ptr<EdgeStraightElement>>
            let = lets[next];
        lets.erase(lets.begin() + next);
        const std::shared_ptr<JointBeamElement> joint =
            JointBeamElement::butt_top_side(*let.first, *let.second);

        if (joint) {
            const std::vector<Point> ends = let.second->axis.get_points();
            const Line outer_axis = Line::from_points(
                let.first->axis.get_points().front(), let.first->axis.get_points().back()
            );
            const bool last =
                Point::distance(ends.back(), outer_axis.closest_point(ends.back()).second) <
                Point::distance(ends.front(), outer_axis.closest_point(ends.front()).second);
            const Vector away = last ? ends.back() - ends.front() : ends.front() - ends.back();
            joint->butt_extensions[1] = InteractionFeatureSolid();
            joint->butt_solids[1] = InteractionFeatureSolid(
                shadow(let.first->model_geometry_mesh(), away, plane), SolidOperation::subtract
            );
        }

        add_butt(joint, let.first, let.second);
    }
}

}
