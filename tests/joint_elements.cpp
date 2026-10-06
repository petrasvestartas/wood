#include "wood_session.h"
#include "wood_contact_detection.h"
#include "wood_element_geometry.h"
#include <chrono>
#include <set>

using namespace session_cpp;
using namespace wood_session;

static void check(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message);
}

static void contact_round_trip(const InteractionContactFace& contact) {
    const auto restored = InteractionContactFace::interaction_data_loads(contact.interaction_data_dumps());
    check(restored.face_a == contact.face_a && restored.face_b == contact.face_b, "contact face serialization");
    check(restored.type == contact.type, "contact classification serialization");
    for (int i = 0; i < 4; ++i)
        check(restored.volumes[i].get_points() == contact.volumes[i].get_points(), "contact volume serialization");
    const auto twice = std::dynamic_pointer_cast<InteractionContactFace>(contact.flipped()->flipped());
    check(twice->face_a == contact.face_a && twice->face_b == contact.face_b, "double flip orientation");
    for (int i = 0; i < 4; ++i) check(twice->volumes[i].get_points() == contact.volumes[i].get_points(), "double flip volume");
}

/// The drill features of the element whose guid starts with the connector's.
static size_t drills_of(const Element& element, const std::string& connector) {
    size_t count = 0;
    for (const ElementFeature& feature : element.features())
        if (feature.feature_type == "drill" && feature.guid().starts_with(connector + "/"))
            ++count;
    return count;
}

/// The plate guids of a scene's adjacency pairs, each pair sorted.
static std::set<std::pair<std::string, std::string>> pair_guids(const WoodSession& scene) {
    const std::vector<std::shared_ptr<Plate>> plates = scene.world_elements<Plate>();
    std::set<std::pair<std::string, std::string>> pairs;
    for (const std::pair<int, int>& pair : scene.adjacency)
        pairs.insert(std::minmax(plates[pair.first]->guid(), plates[pair.second]->guid()));
    return pairs;
}

/// Two 100 x 100 x 40 plates stacked at the origin and the four dowels across their contact, the connector not yet added.
static std::shared_ptr<JointBeam> stacked_dowels(WoodSession& scene, std::shared_ptr<Plate>& lower, std::shared_ptr<Plate>& upper) {
    lower = Plate::from_rectangle({0, 0, 0}, {1, 0, 0}, {0, 1, 0}, 100, 100, 40);
    upper = Plate::from_rectangle({0, 0, 40}, {1, 0, 0}, {0, 1, 0}, 100, 100, 40);
    scene.add(lower); scene.add(upper);
    const auto contact = scene.compute_face_contact(lower, upper);
    check(contact != nullptr, "stacked plates touch");
    return JointBeam::dowels(*lower, *upper, *contact, 4.0, 30.0, 20.0, 10.0);
}

/// The scene calls a user's connectors keep through: compute_features keeps them with their cuts and holes; remove_interaction takes the holes with the cut; holes in a moved target are found and drawn in its own frame; graft renumbers adjacency by plate guid and searches when a side has none; adjacency and three-valence groups go through the protobuf; compute_features reads no dataset's sidecars.
static void check_scene_calls() {

    WoodSession kept("kept connector");
    std::shared_ptr<Plate> lower, upper;
    const std::shared_ptr<JointBeam> dowels = stacked_dowels(kept, lower, upper);
    kept.add_joint(dowels);
    const size_t holes = drills_of(*lower, dowels->guid());
    kept.compute_face_contacts();
    kept.compute_features(face_to_face);
    check(kept.get_element<JointBeam>(dowels->guid()) && holes > 0 && drills_of(*lower, dowels->guid()) == holes && lower->solid_cuts.size() == 1, "compute_features keeps the user's dowels, their " + std::to_string(holes) + " holes and their cut");
    kept.remove_interaction(dowels, lower);
    check(drills_of(*lower, dowels->guid()) == 0 && lower->solid_cuts.empty() && drills_of(*upper, dowels->guid()) == holes, "remove_interaction drops the holes with the cut, the other target keeps its own");

    WoodSession moved("moved targets");
    const std::shared_ptr<JointBeam> local = stacked_dowels(moved, lower, upper);
    const Xform shift = Xform::translation(500, 0, 0);
    const std::shared_ptr<JointBeam> placed = std::dynamic_pointer_cast<JointBeam>(local->transformed(shift));
    moved.set_xform(lower->guid(), shift);
    moved.set_xform(upper->guid(), shift);
    moved.add_joint(placed);
    double longest = 0.0, farthest = 0.0;
    for (const SolidCut& cut : lower->solid_cuts)
        for (const Line& drill : cut.drills)
            longest = std::max(longest, drill.length());
    for (const ElementFeature& feature : lower->Element::features())
        if (feature.feature_type == "drill")
            for (const Polyline& circle : feature.outlines)
                for (const Point& point : circle.get_points())
                    farthest = std::max(farthest, point[0]);
    check(std::abs(longest - 40.0) < 1e-6 && farthest < 100.0, "a moved target's holes run past the dowel only where it leaves the target, " + std::to_string(longest) + " long, drawn in its frame up to x " + std::to_string(farthest));

    WoodSession first("first"), second("second"), bare("bare");
    for (int i = 0; i < 2; ++i)
        first.add(Plate::from_rectangle({0, 100.0 * i, 0}, {1, 0, 0}, {0, 1, 0}, 100, 100, 10));
    const std::shared_ptr<TreeNode> group_a = second.add_group("a");
    const std::shared_ptr<TreeNode> group_b = second.add_group("b");
    const auto p2 = Plate::from_rectangle({0, 300, 0}, {1, 0, 0}, {0, 1, 0}, 100, 100, 10);
    const auto p3 = Plate::from_rectangle({0, 400, 0}, {1, 0, 0}, {0, 1, 0}, 100, 100, 10);
    const auto p4 = Plate::from_rectangle({0, 600, 0}, {1, 0, 0}, {0, 1, 0}, 100, 100, 10);
    second.add(p2, group_b); second.add(p3, group_b); second.add(p4, group_a);
    first.adjacency = {{0, 1}};
    second.adjacency = {{0, 1}};
    first.three_valence = {{0}};
    second.three_valence = {{0}, {0, 1, 0, 1}};
    WoodSession merged = first;
    merged.merge(second);
    check(pair_guids(merged).count(std::minmax(p2->guid(), p3->guid())) && merged.adjacency.size() == 2, "merge renumbers the grafted adjacency by plate guid when the tree reorders the plates");
    check(merged.three_valence.size() == 2 && merged.world_elements<Plate>()[merged.three_valence[1][0]]->guid() == p2->guid(), "merge renumbers the three-valence groups by plate guid");
    bare.add(Plate::from_rectangle({0, 900, 0}, {1, 0, 0}, {0, 1, 0}, 100, 100, 10));
    WoodSession searched = first;
    searched.merge(bare);
    check(searched.adjacency.empty(), "merging plates without an adjacency leaves it empty, so every pair is searched");

    const WoodSession restored = WoodSession::pb_loads(merged.pb_dumps());
    check(restored.adjacency == merged.adjacency && restored.three_valence == merged.three_valence, "adjacency and three-valence groups through the protobuf");

    config::reset_defaults();
    const WoodSession dataset = WoodSession::yaml_load("vidy_corner");
    check(!dataset.adjacency.empty(), "vidy_corner has an adjacency sidecar");
    WoodSession fresh("fresh");
    stacked_dowels(fresh, lower, upper);
    fresh.compute_face_contacts();
    fresh.compute_features(face_to_face);
    check(fresh.adjacency.empty(), "compute_features after a dataset load does not take that dataset's adjacency");
    std::cout << "scene calls: user connectors kept, their holes removed with them, moved targets drilled in their frame, adjacency renumbered by guid, stored and never borrowed\n";
}

int main() {
    WoodSession scene("joint API checks");
    const auto a = Plate::from_rectangle({0, 0, 0}, {1, 0, 0}, {0, 1, 0}, 400, 300, 40);
    const auto b = Plate::from_rectangle({150, 0, 40}, {0, 1, 0}, {0, 0, 1}, 300, 400, 40);
    scene.add(a); scene.add(b);
    const auto contact = scene.compute_face_contact(a, b);
    check(contact && contact->face_a == 1 && contact->face_b == 5, "plate contact faces");
    check(!a->geometry_synced() && !b->geometry_synced(), "plate detection must not loft solids");
    check(contact->type == ContactType::side_top, "plate contact classification");
    contact_round_trip(*contact);
    const auto reverse = scene.compute_face_contact(b, a);
    check(reverse && reverse->face_a == 5 && reverse->face_b == 1 && reverse->type == ContactType::side_top, "reverse pair detection");
    check(!scene.compute_face_contact(a, a) && !scene.compute_face_contact(nullptr, b), "missing/self contact");
    const auto joint = JointPlate::side_to_top();
    contact->flip();
    joint->orient(contact);
    scene.add(joint);
    scene.add_interaction(b, a, contact);
    scene.add_interaction(joint, b, joint->interaction_feature(0));
    scene.add_interaction(joint, a, joint->interaction_feature(1));
    scene.add_interaction(joint, a, joint->interaction_feature(1));
    check(scene.get_interaction(joint, a).size() == 1, "joint side attachment is idempotent");
    check(scene.consistent(), "directed joint edges");
    check(!a->features.top.empty() && !b->features.top.empty(), "manual joint merges both plates");
    check(joint->element_geometry_mesh().number_of_faces() > 0 && joint->element_geometry_mesh().is_closed(), "closed joint mesh");
    check(joint->element_geometry_brep().is_valid() && joint->element_geometry_brep().is_solid(), "joint brep solids");
    const auto restored = WoodSession::pb_loads(scene.pb_dumps());
    const auto restored_joint = restored.get_element<JointPlate>(joint->guid());
    check(restored_joint && restored_joint->connections.size() == 1 && restored.consistent(), "joint element round trip");
    check(restored_joint->model_geometry_mesh().number_of_faces() == joint->model_geometry_mesh().number_of_faces(), "joint geometry round trip");

    const auto beam = std::make_shared<Beam>(Polyline({{500, 50, 900}, {900, 50, 900}}), 50.0);
    const auto column = std::make_shared<Column>(Line::from_points({950, 50, 0}, {950, 50, 950}),
        Polyline::rectangle({900, 0, 0}, {1, 0, 0}, {0, 1, 0}, 100, 100));
    scene.add(beam); scene.add(column);
    const auto linear = scene.compute_face_contact(column, beam);
    check(linear && linear->face_a == 5 && linear->face_b == 5 && linear->type == ContactType::end_side, "linear contact");
    check(linear->volumes[0].point_count() == 0, "linear contact has no plate volume");
    check(!scene.compute_face_contact(a, beam), "disjoint contact");
    auto support = Plate::from_rectangle({500, 0, 810}, {1, 0, 0}, {0, 1, 0}, 400, 100, 40);
    auto mixed = scene.compute_face_contact(support, beam);
    check(mixed && mixed->type == ContactType::side_top && mixed->volumes[0].point_count() == 0, "mixed plate/beam contact");
    contact_round_trip(*linear);

    const auto cutter = std::make_shared<Joint>(Plane::from_point_normal({700, 0, 0}, {-1, 0, 0}));
    cutter->targets = {beam->guid()};
    const double uncut = compute_volume(beam->model_geometry_mesh());
    scene.add_joint(cutter);
    check(compute_volume(beam->model_geometry_mesh()) < uncut, "plane cutter modifies beam");
    const auto cut_scene = WoodSession::pb_loads(scene.pb_dumps());
    check(cut_scene.get_element<Joint>(cutter->guid())->cuts.size() == 1, "plane cutter round trip");

    const auto profile = std::make_shared<Joint>(
        Polyline::rectangle({500, 25, 875}, {0, 1, 0}, {0, 0, 1}, 50, 50), Vector(200, 0, 0));
    profile->targets = {beam->guid()};
    const double before_profile = compute_volume(beam->model_geometry_mesh());
    scene.add_joint(profile);
    check(compute_volume(beam->model_geometry_mesh()) < before_profile, "profile cutter modifies beam");
    check(profile->element_geometry_brep().is_valid(), "profile cutter solid");

    WoodSession beams("beam joint");
    auto ba = std::make_shared<Beam>(Polyline({{-100, 0, 0}, {100, 0, 0}}), 10);
    auto bb = std::make_shared<Beam>(Polyline({{0, -100, 0}, {0, 100, 0}}), 10);
    beams.add(ba); beams.add(bb);
    beams.compute_axis_contacts(1);
    beams.compute_beam_features(40, 0.5, 0);
    const size_t beam_count = beams.objects.elements->size();
    check(beam_count == 3, "beam joint element created");
    beams.compute_beam_features(40, 0.5, 0);
    check(beams.objects.elements->size() == beam_count, "beam solve replaces old joint");
    auto beam_copy = WoodSession::pb_loads(beams.pb_dumps());
    int beam_joints = 0;
    for (const auto& e : *beam_copy.objects.elements)
        if (auto j = std::dynamic_pointer_cast<JointBeam>(e)) {
            ++beam_joints;
            check(j->targets.size() == 2 && j->element_geometry_mesh().number_of_faces() > 0, "beam joint geometry round trip");
            check(j->element_geometry_brep().is_valid(), "beam joint brep");
        }
    check(beam_joints == 1, "beam joint type round trip");

    WoodSession drilling("drill joint");
    auto lower = Plate::from_rectangle({0, 0, 0}, {1, 0, 0}, {0, 1, 0}, 100, 100, 10);
    auto upper = Plate::from_rectangle({0, 0, 10}, {1, 0, 0}, {0, 1, 0}, 100, 100, 10);
    drilling.add(lower); drilling.add(upper);
    auto drill_contact = drilling.compute_face_contact(lower, upper);
    check(drill_contact && drill_contact->type == ContactType::top_top, "top-top contact");
    auto drill = std::make_shared<JointPlate>(lower, upper, *drill_contact, 40, 6);
    drill->line_radius = 2.0;
    drilling.add_joint(drill);
    check(drill->element_geometry_mesh().number_of_faces() > 0, "line joint has a solid mesh");
    check(drill->element_geometry_brep().is_valid() && drill->element_geometry_brep().is_solid(), "line joint has brep solids");
    auto moved = drill->transformed(Xform::translation(100, 0, 0));
    check(moved && moved->element_type_name() == "JointPlate", "joint transformation preserves type");

    for (const std::string dataset : {"cross_corners", "annen_box_pair", "vidy_corner", "inplane_hexshell"}) {
        config::reset_defaults();
        auto model = WoodSession::yaml_load(dataset);
        check(!model.plates().empty(), "missing dataset " + dataset);
        auto start = std::chrono::steady_clock::now();
        if (dataset == "cross_corners") model.compute_cross_contacts();
        else model.compute_face_contacts();
        const auto joints = model.compute_features(dataset == "cross_corners" ? cross_joint : face_to_face);
        check(!joints.empty() && model.consistent(), "dataset joints " + dataset);
        const auto count = model.objects.elements->size();
        model.compute_features(dataset == "cross_corners" ? cross_joint : face_to_face);
        check(model.objects.elements->size() == count, "recompute must replace joint elements " + dataset);
        const auto reloaded = WoodSession::pb_loads(model.pb_dumps());
        check(reloaded.get_plate_features().size() == joints.size() && reloaded.consistent(), "dataset serialization " + dataset);
        int multi = 0;
        for (const auto& e : *model.objects.elements)
            if (auto j = std::dynamic_pointer_cast<JointPlate>(e); j && j->targets.size() > 2) ++multi;
        if (dataset == "annen_box_pair" || dataset == "vidy_corner") check(multi > 0, "multi-element joints " + dataset);
        std::cout << dataset << ": " << joints.size() << " connections, " << multi << " multi-element joints, "
                  << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() << " ms including repeat solve and serialization\n";
    }
    check_scene_calls();
    std::cout << "joint API checks passed\n";
}
