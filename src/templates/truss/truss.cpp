#include "pch.h"
#include "src/templates/truss/truss.h"
#include "wood_element_geometry.h"
#include "wood_joint_classification.h"

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

    // no butt stays but a chord's kink miters (the two runs of a bend, a plate joins them): every one goes, its features with it
    for (const std::shared_ptr<JointBeamElement>& butt : get_elements<JointBeamElement>()) {
        if (!butt->is_butt || is_kink_miter(*butt))
            continue;
        for (const std::string& target : butt->targets)
            remove_interaction(butt, get_element<Element>(target));
        remove_object(butt->guid());
    }

    // the nodes: every end of a member's axis
    const std::vector<std::shared_ptr<EdgeStraightElement>> members = get_elements<EdgeStraightElement>();
    std::vector<Point> nodes;
    for (const std::shared_ptr<EdgeStraightElement>& member : members)
        for (const Point& end : {member->axis.get_point(0), member->axis.get_point(member->axis.point_count() - 1)}) {
            bool known = false;
            for (const Point& node : nodes)
                known = known || Point::distance(node, end) < NODE_TOLERANCE;
            if (!known)
                nodes.push_back(end);
        }

    const std::vector<TrussBar> bars = guide.bars();
    std::map<JointRole, size_t> turns;
    for (const Point& node : nodes) {
        // who ends here and who runs through
        std::vector<std::shared_ptr<EdgeStraightElement>> ends;
        std::vector<std::shared_ptr<EdgeStraightElement>> through;
        for (const std::shared_ptr<EdgeStraightElement>& member : members) {
            const Point first = member->axis.get_point(0);
            const Point last = member->axis.get_point(member->axis.point_count() - 1);
            if (Point::distance(first, node) < NODE_TOLERANCE || Point::distance(last, node) < NODE_TOLERANCE)
                ends.push_back(member);
            else if (Point::distance(Line::from_points(first, last).closest_point(node).second, node) < NODE_TOLERANCE)
                through.push_back(member);
        }
        if (ends.size() + through.size() < 2 || ends.empty())
            continue;

        // the carrier the ends are jointed into: the member running through, else a post ending here (running on past the node)
        std::shared_ptr<EdgeStraightElement> carrier = through.empty() ? nullptr : through.front();
        for (const std::shared_ptr<EdgeStraightElement>& member : ends)
            if (!carrier && member->name.rfind("web_", 0) == 0 && steepness(*member) > 0.99)
                carrier = member;

        // else a kink: the two runs of the chord mitered, a plate joining them and every end there
        std::array<std::shared_ptr<EdgeStraightElement>, 2> kink = {nullptr, nullptr};
        for (const std::shared_ptr<EdgeStraightElement>& member : ends)
            for (const std::shared_ptr<EdgeStraightElement>& other : ends)
                if (!kink[0] && member != other && same_chord(*member, *other) && run_index(*member) + 1 == run_index(*other))
                    kink = {member, other};
        if (!carrier && kink[0])
            carrier = kink[0];

        // else the lowest chord (a heel's tie), else the steepest web
        if (!carrier) {
            for (const std::shared_ptr<EdgeStraightElement>& member : ends)
                if (member->name.rfind("chord_", 0) == 0 && (!carrier || lowness(*member) < lowness(*carrier)))
                    carrier = member;
            for (const std::shared_ptr<EdgeStraightElement>& member : ends)
                if (!carrier || (carrier->name.rfind("web_", 0) == 0 && steepness(*member) > steepness(*carrier)))
                    carrier = member;
        }
        // the members of the node in the order they go in: what runs through or the carrier, then the ends, the steepest first
        // (a post before the struts beside it)
        std::vector<std::shared_ptr<EdgeStraightElement>> order = through;
        if (carrier && through.empty())
            order.push_back(carrier);
        std::vector<std::shared_ptr<EdgeStraightElement>> jointed;
        for (const std::shared_ptr<EdgeStraightElement>& end : ends)
            if (end != carrier && end != kink[0] && end != kink[1])
                jointed.push_back(end);
        std::stable_sort(jointed.begin(), jointed.end(), steeper);
        for (const std::shared_ptr<EdgeStraightElement>& run : kink)
            if (run && run != carrier)
                order.push_back(run);
        order.insert(order.end(), jointed.begin(), jointed.end());

        // a carrier ending here runs on past the node: by its section where a plate joins the node, else as far as the farthest
        // end jointed into it reaches along it
        const bool crowded = jointed.size() >= 2 || kink[0];

        // what each member of the node fills there: its drawn prism, the carrier's run on past the node
        Mesh carrier_body;
        std::map<const EdgeStraightElement*, Mesh> bodies;
        double carried = 0.0;
        if (carrier && through.empty() && carrier != kink[0]) {
            double reach = RUN_ON * extent(*carrier, plane_normal().cross(away_from(*carrier, node)).normalized());
            for (const std::shared_ptr<EdgeStraightElement>& end : jointed)
                if (!crowded)
                    reach = std::max(reach, std::abs(away_from(*end, node).dot(away_from(*carrier, node))) * through_length(*end, *carrier, node) + extent(*end, plane_normal().cross(away_from(*end, node)).normalized()));
            carrier_body = add_run_on(carrier, node, reach);
            carried = reach;
        }

        for (const std::shared_ptr<EdgeStraightElement>& member : order)
            bodies[member.get()] = member == carrier && carrier_body.number_of_faces() > 0 ? carrier_body
                                   : member == kink[0] || member == kink[1]                ? member->model_geometry_mesh()
                                                                                           : member->element_geometry_mesh();

        // several ends in one carrier, or a kink: a plate let into them all, pinned, the members meeting cleanly: an end stops on
        // what runs on through the node (flush on its face), a web on a chord, and two ends side by side are mitered on their
        // bisector (the kink's runs already are; two running through cross in a cross lap)
        if (crowded) {
            std::vector<std::shared_ptr<EdgeStraightElement>> runners = through;
            if (carrier && carried > 0.0)
                runners.push_back(carrier);
            const std::set<std::pair<const EdgeStraightElement*, const EdgeStraightElement*>> beside = neighbours(order, runners, node);
            for (size_t k = 1; k < order.size(); k++)
                for (size_t j = 0; j < k; j++) {
                    const std::shared_ptr<EdgeStraightElement>& first = order[j];
                    const std::shared_ptr<EdgeStraightElement>& second = order[k];
                    const bool first_runs = std::find(runners.begin(), runners.end(), first) != runners.end();
                    const bool second_runs = std::find(runners.begin(), runners.end(), second) != runners.end();
                    const bool kinked = kink[0] && (first == kink[0] || first == kink[1]) && (second == kink[0] || second == kink[1]);
                    if ((first_runs && second_runs) || kinked)
                        continue;

                    // an end on what runs on, a web on a chord: flush on its face
                    const bool first_chord = first->name.rfind("chord_", 0) == 0;
                    const bool second_chord = second->name.rfind("chord_", 0) == 0;
                    if (first_runs || second_runs || first_chord != second_chord) {
                        const bool first_holds = first_runs || (!second_runs && first_chord);
                        const std::shared_ptr<EdgeStraightElement>& holding = first_holds ? first : second;
                        const std::shared_ptr<EdgeStraightElement>& ending = first_holds ? second : first;
                        add_interaction(holding, ending, std::make_shared<InteractionFeatureSolid>(bodies.at(holding.get()), SolidOperation::subtract));
                        continue;
                    }

                    // two ends of a kind side by side: mitered on their bisector
                    if (beside.count({first.get(), second.get()}) || beside.count({second.get(), first.get()})) {
                        const Vector from_first = away_from(*first, node);
                        const Vector from_second = away_from(*second, node);
                        add_interaction(second, first, std::make_shared<InteractionFeatureSolid>(cut_away(node, (from_first - from_second).normalized()), SolidOperation::subtract));
                        add_interaction(first, second, std::make_shared<InteractionFeatureSolid>(cut_away(node, (from_second - from_first).normalized()), SolidOperation::subtract));
                    }
                }
            add_node_plate(order, node, carried);
            continue;
        }

        // one end: its joint of the book
        for (const std::shared_ptr<EdgeStraightElement>& end : jointed) {
            const std::shared_ptr<EdgeStraightElement>& side = carrier;

            // a heel: a sloped chord ending on the level one
            const bool chords = end->name.rfind("chord_", 0) == 0 && side->name.rfind("chord_", 0) == 0;
            const bool heel = chords && steepness(*side) < 0.05 && steepness(*end) > 0.05;
            const Vector along_end = away_from(*end, node);
            const Vector along_side = (side->axis.get_point(1) - side->axis.get_point(0)).normalized();
            const bool square = std::abs(along_end.dot(along_side)) < std::sin(5.0 * Tolerance::PI / 180.0);
            const bool pulls = pull_at(bars, *end, node) > 1e-6;
            const bool post = std::abs(along_end[2]) > 0.999;
            const bool flat = steepness(*end) < std::sin(30.0 * Tolerance::PI / 180.0);
            const JointRole role = heel           ? (flat ? JointRole::flat_heel : JointRole::heel)
                                   : pulls && post ? JointRole::hanging_post
                                   : pulls        ? JointRole::pulling
                                   : square       ? JointRole::square_push
                                                  : JointRole::oblique_push;
            add_role_joint(role, end, side, node, turns);
        }

        // what else runs through the node: the end bears on its side (two running through cross in a cross lap)
        for (size_t k = 1; k < order.size(); k++)
            for (size_t j = 0; j < k; j++)
                if (order[j] != carrier && order[k] != carrier &&
                    !(std::find(through.begin(), through.end(), order[j]) != through.end() && std::find(through.begin(), through.end(), order[k]) != through.end()))
                    add_interaction(order[j], order[k], std::make_shared<InteractionFeatureSolid>(bodies.at(order[j].get()), SolidOperation::subtract));
    }

    // crossings: two members passing through each other where neither ends, halved into each other from the faces; square a
    // cross lap (p. 646), oblique the notched oblique cross laps in turn (pp. 672, 676)
    const std::array<std::string, 2> oblique = {"oblique_notched_cross_lap", "double_notched_oblique_cross_lap"};
    size_t turn = 0;
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
            if (Point::distance(crossing, second.point_at(t_second)) > NODE_TOLERANCE)
                continue;
            bool at_an_end = false;
            for (const Line& line : {first, second})
                at_an_end = at_an_end || Point::distance(crossing, line.start()) < NODE_TOLERANCE || Point::distance(crossing, line.end()) < NODE_TOLERANCE;
            if (at_an_end)
                continue;

            // the member rising to the right first, so every crossing of a lattice opens to the same faces
            const bool rising = first.to_vector()[2] * first.to_vector()[0] >= 0.0;
            const std::shared_ptr<EdgeStraightElement>& a = rising ? members[i] : members[j];
            const std::shared_ptr<EdgeStraightElement>& b = rising ? members[j] : members[i];
            const bool square = std::abs(first.to_vector().normalized().dot(second.to_vector().normalized())) < std::sin(5.0 * Tolerance::PI / 180.0);
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

bool Truss::steeper(const std::shared_ptr<EdgeStraightElement>& a, const std::shared_ptr<EdgeStraightElement>& b) {

    return steepness(*a) > steepness(*b) + 1e-9;
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

    return nullptr;
}

void Truss::add_node_plate(const std::vector<std::shared_ptr<EdgeStraightElement>>& members, const Point& node, double carried) {

    // where each end is clear of the others along its axis from the node, the carrier's pins in the node under the ends, and
    // the plate two diameters past each member's row of pins: as small as its pins allow
    std::vector<const EdgeStraightElement*> raw;
    std::vector<double> clear;
    std::vector<double> reaches;
    const std::shared_ptr<EdgeStraightElement>& carrier = members.front();
    for (const std::shared_ptr<EdgeStraightElement>& member : members) {
        double free = 0.0;
        for (const std::shared_ptr<EdgeStraightElement>& other : members)
            if (other != member && member != carrier)
                free = std::max(free, clear_of(*member, *other, node, other == carrier ? carried : 0.0));
        raw.push_back(member.get());
        clear.push_back(free);
        reaches.push_back(free + 4.0 * 2.0 * PIN_RADIUS);
    }

    // the plate out of the node square to its chord (else to the carrier), away from the other members
    std::shared_ptr<EdgeStraightElement> chord = members.front();
    for (const std::shared_ptr<EdgeStraightElement>& member : members)
        if (member->name.rfind("chord_", 0) == 0 && chord->name.rfind("chord_", 0) != 0)
            chord = member;
    const Vector along = (chord->axis.get_point(1) - chord->axis.get_point(0)).normalized();
    Vector way = plane_normal().cross(along).normalized();
    Vector toward(0.0, 0.0, 0.0);
    for (const std::shared_ptr<EdgeStraightElement>& member : members)
        if (member != chord)
            if (ends_at(*member, node))
                toward = toward + away_from(*member, node);
    if (way.dot(toward) > 0.0)
        way = way * -1.0;

    const std::shared_ptr<FacePlanarElement> plate = JointBeamElement::node_plate(raw, node, plane_normal(), reaches, 2.0 * half_thickness() / 3.0, PLATE_INSET, fmt::format("node_plate_{:.0f}_{:.0f}", node[0], node[2]));
    const std::shared_ptr<JointBeamElement> pins = JointBeamElement::slotted_plate(raw, *plate, node, way, clear, PIN_RADIUS);
    add(plate, group_named("joinery"));
    add(pins, group_named("joinery"));
    for (size_t m = 0; m < members.size(); m++)
        add_interaction(pins, members[m], pins->interaction(m));
    add_interaction(pins, plate, pins->interaction(members.size()));
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

    return clear;
}

bool Truss::ends_at(const EdgeStraightElement& member, const Point& node) {

    return Point::distance(member.axis.get_point(0), node) < NODE_TOLERANCE ||
           Point::distance(member.axis.get_point(member.axis.point_count() - 1), node) < NODE_TOLERANCE;
}

std::set<std::pair<const EdgeStraightElement*, const EdgeStraightElement*>> Truss::neighbours(
    const std::vector<std::shared_ptr<EdgeStraightElement>>& members,
    const std::vector<std::shared_ptr<EdgeStraightElement>>& runners,
    const Point& node
) const {

    // every way a member leaves the node (both for one running on through it), by its angle in the truss plane
    const Vector across = plane_normal();
    const Vector x = Vector(0.0, 0.0, 1.0).cross(across).normalized();
    const Vector y = across.cross(x);
    std::vector<std::pair<double, const EdgeStraightElement*>> ways;
    for (const std::shared_ptr<EdgeStraightElement>& member : members) {
        const Vector away = away_from(*member, node);
        ways.emplace_back(std::atan2(away.dot(y), away.dot(x)), member.get());
        if (std::find(runners.begin(), runners.end(), member) != runners.end())
            ways.emplace_back(std::atan2(-away.dot(y), -away.dot(x)), member.get());
    }
    std::sort(ways.begin(), ways.end());

    // two ways next to each other around the node
    std::set<std::pair<const EdgeStraightElement*, const EdgeStraightElement*>> beside;
    for (size_t k = 0; k < ways.size() && ways.size() > 1; k++)
        beside.insert({ways[k].second, ways[(k + 1) % ways.size()].second});

    return beside;
}

Mesh Truss::cut_away(const Point& node, const Vector& keep) const {

    // a box from the plane through node square to keep, out the other way: what a member losing it keeps is on keep's side
    const Vector across = plane_normal();
    const Vector along = across.cross(keep).normalized();
    const double reach = 1e4;
    const Polyline face({
        node + along * reach + across * reach,
        node - along * reach + across * reach,
        node - along * reach - across * reach,
        node + along * reach - across * reach,
        node + along * reach + across * reach,
    });

    return Mesh::loft({face}, {face.translated(keep * -reach)}, true);
}

bool Truss::is_kink_miter(const JointBeamElement& butt) const {

    // a butt between two consecutive runs of one chord
    if (butt.targets.size() != 2)
        return false;
    const std::shared_ptr<EdgeStraightElement> a = get_element<EdgeStraightElement>(butt.targets[0]);
    const std::shared_ptr<EdgeStraightElement> b = get_element<EdgeStraightElement>(butt.targets[1]);

    return a && b && same_chord(*a, *b) && (run_index(*a) + 1 == run_index(*b) || run_index(*b) + 1 == run_index(*a));
}

bool Truss::same_chord(const EdgeStraightElement& a, const EdgeStraightElement& b) {

    // chord_<c>_<s>: the same c
    if (a.name.rfind("chord_", 0) != 0 || b.name.rfind("chord_", 0) != 0)
        return false;

    return a.name.substr(0, a.name.rfind('_')) == b.name.substr(0, b.name.rfind('_'));
}

size_t Truss::run_index(const EdgeStraightElement& chord) {

    return static_cast<size_t>(std::stoul(chord.name.substr(chord.name.rfind('_') + 1)));
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

    // the member's own section from its far end to length past the node, added: the wood a joint into it needs beyond the node
    const Vector out = away_from(*member, node) * -1.0;
    const Point first = member->axis.get_point(0);
    const Point back = Point::distance(first, node) < NODE_TOLERANCE ? member->axis.get_point(member->axis.point_count() - 1) : first;
    const Vector across = plane_normal();
    const std::shared_ptr<EdgeStraightElement> prism = flush(back, node + out * length, across, extent(*member, across), extent(*member, across.cross(out).normalized()), member->name + "_run_on");
    const std::shared_ptr<JointBeamElement> run_on = std::make_shared<JointBeamElement>();
    run_on->is_butt = true;
    run_on->butt_solids = {InteractionFeatureSolid(prism->model_geometry_mesh(), SolidOperation::add), InteractionFeatureSolid()};
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
    const double kept = end.extension;
    end.extension = through_length(end, side, node);
    const std::shared_ptr<JointBeamElement> joint = JointBeamElement::from_library(design, end, side, given);
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
