#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_joint_classification.h"
#include "wood_assembly_sequence.h"
#include "wood_joint_library.h"
#include "src/templates/truss/carpentry_truss.h"

using namespace session_cpp;
using namespace wood_session;

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// A square beam named name between two points.
static std::shared_ptr<EdgeStraightElement> beam(const Point& start, const Point& end, double radius, const std::string& name) {
    return EdgeStraightElement::square(Polyline({start, end}), radius, name);
}

/// The volume a member loses to its cuts.
static double lost(const Element& member) {
    return compute_volume(member.element_geometry_mesh()) - compute_volume(member.model_geometry_mesh());
}

/// The volume two members' cut solids share.
static double shared(const Element& a, const Element& b) {

    const Mesh common = solid_boolean(a.model_geometry_mesh(), b.model_geometry_mesh(), SolidOperation::intersect);
    return common.number_of_faces() == 0 ? 0.0 : compute_volume(common);
}

/// Every category read from two axes, with its family.
static void classification() {

    const Point origin(0.0, 0.0, 0.0);
    const std::shared_ptr<EdgeStraightElement> carrier = beam(Point(-500.0, 0.0, 0.0), Point(500.0, 0.0, 0.0), 50.0, "carrier");
    const std::vector<std::tuple<std::shared_ptr<EdgeStraightElement>, JointCategory, JointFamily>> cases = {
        {beam(origin, Point(0.0, 600.0, 0.0), 50.0, "square"), JointCategory::side_end, JointFamily::t},
        {beam(origin, Point(-400.0, 400.0, 0.0), 50.0, "leaning"), JointCategory::side_end, JointFamily::angled},
        {beam(Point(500.0, 0.0, 0.0), Point(1200.0, 0.0, 0.0), 50.0, "in_line"), JointCategory::end_end_inplane, JointFamily::splicing},
        {beam(Point(500.0, 0.0, 0.0), Point(900.0, 500.0, 0.0), 50.0, "corner"), JointCategory::end_end_outplane, JointFamily::corner},
        {beam(Point(0.0, -500.0, 100.0), Point(0.0, 500.0, 100.0), 50.0, "crossing"), JointCategory::cross, JointFamily::cross_lap},
        {beam(Point(-300.0, 100.0, 0.0), Point(300.0, 100.0, 0.0), 50.0, "beside"), JointCategory::side_side, JointFamily::specialty},
        {beam(Point(0.0, 400.0, 0.0), Point(0.0, 900.0, 0.0), 50.0, "apart"), JointCategory::none, JointFamily::none},
    };

    for (const std::tuple<std::shared_ptr<EdgeStraightElement>, JointCategory, JointFamily>& item : cases) {
        const JointClassification joint = classify_joint(*carrier, *std::get<0>(item));
        check(
            joint.category == std::get<1>(item) && joint.family == std::get<2>(item),
            fmt::format("classification: {} reads {} {}, not {} {}", std::get<0>(item)->name, to_string(joint.category), to_string(joint.family), to_string(std::get<1>(item)), to_string(std::get<2>(item)))
        );
    }
}

/// The Stirnversatz: its four steps, the two members filling the notch without sharing wood, the strut in along the notch and across it.
static void notched_oblique_thrust() {

    WoodSession scene("notched_oblique_thrust");
    const std::shared_ptr<EdgeStraightElement> carrier = beam(Point(-500.0, 0.0, 0.0), Point(500.0, 0.0, 0.0), 60.0, "carrier");
    const std::shared_ptr<EdgeStraightElement> strut = beam(Point(0.0, 0.0, 0.0), Point(-500.0, 420.0, 0.0), 60.0, "strut");
    scene.add(carrier);
    scene.add(strut);
    const std::shared_ptr<JointBeamElement> joint = JointBeamElement::from_library("notched_oblique_thrust", *strut, *carrier);
    scene.add(joint);
    scene.add_interaction(joint, strut, joint->interaction(0));
    scene.add_interaction(joint, carrier, joint->interaction(1));

    check(joint->steps.size() == 4 && joint->steps[2].removal.number_of_faces() == 0, "notched_oblique_thrust: steps A2, A3, B2 (a kerf), B3");
    check(lost(*carrier) > 1000.0 && lost(*strut) > 1000.0, "notched_oblique_thrust: both members lose wood");
    check(shared(*carrier, *strut) < 1.0, fmt::format("notched_oblique_thrust: the members share {} mm3", shared(*carrier, *strut)));
    check(joint->insertion_directions(0).size() == 3, "notched_oblique_thrust: the strut goes in along the notch and across it, the notch running through");
    check(strut->insertion_vectors().size() == 3, "notched_oblique_thrust: the strut keeps its three insertion directions");

    // the procedure survives a round trip of the scene
    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    const std::shared_ptr<JointBeamElement> read = back.get_element_by_name<JointBeamElement>("notched_oblique_thrust");
    check(read && read->steps.size() == 4 && read->steps[3].label == "B3" && read->target_insertions.size() == 2 && read->design == "notched_oblique_thrust", "notched_oblique_thrust: the steps and insertions are written with the scene");

    // mirrored and with the carrier's axis reversed: the same wood lost, none shared
    for (const double side : {1.0, -1.0}) {
        WoodSession mirrored("mirrored");
        const std::shared_ptr<EdgeStraightElement> reversed = beam(Point(500.0, 0.0, 0.0), Point(-500.0, 0.0, 0.0), 60.0, "reversed");
        const std::shared_ptr<EdgeStraightElement> leaning = beam(Point(500.0 * side, 420.0, 0.0), Point(0.0, 0.0, 0.0), 60.0, "leaning");
        mirrored.add(reversed);
        mirrored.add(leaning);
        const std::shared_ptr<JointBeamElement> mirror = JointBeamElement::from_library("notched_oblique_thrust", *leaning, *reversed);
        mirrored.add(mirror);
        mirrored.add_interaction(mirror, leaning, mirror->interaction(0));
        mirrored.add_interaction(mirror, reversed, mirror->interaction(1));
        check(std::abs(lost(*reversed) - lost(*carrier)) < 1.0 && std::abs(lost(*leaning) - lost(*strut)) < 1.0 && shared(*reversed, *leaning) < 1.0,
              fmt::format("notched_oblique_thrust: mirrored {} loses {} and {} (not {} and {}), sharing {}", side, lost(*reversed), lost(*leaning), lost(*carrier), lost(*strut), shared(*reversed, *leaning)));
    }

    // a strut narrower than the carrier: a blind notch, no way across
    const std::shared_ptr<EdgeStraightElement> narrow = beam(Point(0.0, 0.0, 0.0), Point(-500.0, 420.0, 0.0), 40.0, "narrow");
    check(JointBeamElement::from_library("notched_oblique_thrust", *narrow, *carrier)->insertion_directions(0).size() == 1, "notched_oblique_thrust: a blind notch lets the strut in along it only");
}

/// The blind mortise and tenon: the mortise a third of the member thick, two thirds of it long, half the carrier deep.
static void blind_mortise_tenon() {

    WoodSession scene("blind_mortise_tenon");
    const std::shared_ptr<EdgeStraightElement> carrier = beam(Point(-500.0, 0.0, 0.0), Point(500.0, 0.0, 0.0), 70.0, "carrier");
    const std::shared_ptr<EdgeStraightElement> member = beam(Point(0.0, 0.0, 0.0), Point(0.0, 600.0, 0.0), 60.0, "member");
    scene.add(carrier);
    scene.add(member);
    const std::shared_ptr<JointBeamElement> joint = JointBeamElement::from_library("blind_mortise_tenon", *member, *carrier);
    scene.add(joint);
    scene.add_interaction(joint, member, joint->interaction(0));
    scene.add_interaction(joint, carrier, joint->interaction(1));

    const double mortise = 40.0 * 80.0 * 70.0;
    check(std::abs(lost(*carrier) - mortise) < 1.0, fmt::format("blind_mortise_tenon: the mortise is {} mm3, not {}", lost(*carrier), mortise));
    check(shared(*carrier, *member) < 1.0, "blind_mortise_tenon: the tenon fills the mortise without sharing wood");
    check(joint->insertion_directions(0).size() == 1 && joint->insertion_directions(0)[0].dot(Vector(0.0, -1.0, 0.0)) > 0.999, "blind_mortise_tenon: the member goes in along its axis");

    // a way in the geometry contradicts is refused: sideways out of a blind mortise, the member is locked
    joint->target_insertions = {{Vector(1.0, 0.0, 0.0)}, {Vector(-1.0, 0.0, 0.0)}};
    const AssemblySequence sideways(scene, "carrier");
    check(sideways.locked().size() == 1 && sideways.locked()[0] == "member", "blind_mortise_tenon: a tenon cannot slide sideways out of its mortise");
}

/// Every record of the YAML library on a pair of beams: both cut, no wood shared, the member out along a direction it gives.
static void library() {

    for (const std::string& design : library_designs())
        for (const bool square : {true, false}) {
        const LibraryRecord record = library_record(design);
        if (record.fields.at("corners").find(square ? "square" : "inclined") == std::string::npos)
            continue;
        const std::string angle = square ? "a square corner" : "an inclined corner";
        WoodSession scene(design);
        const std::array<std::shared_ptr<EdgeStraightElement>, 2> pair = library_pair(design, square, Point(0.0, 0.0, 0.0));
        const std::shared_ptr<EdgeStraightElement> member = pair[0];
        const std::shared_ptr<EdgeStraightElement> carrier = pair[1];
        scene.add(carrier);
        scene.add(member);
        const std::shared_ptr<JointBeamElement> joint = JointBeamElement::from_library(design, *member, *carrier);
        scene.add(joint);
        scene.add_interaction(joint, member, joint->interaction(0));
        scene.add_interaction(joint, carrier, joint->interaction(1));
        bool cuts_carrier = false;
        for (const JointCutStep& step : joint->steps)
            cuts_carrier = cuts_carrier || (step.target == 1 && step.removal.number_of_faces() > 0);

        check(lost(*member) > 1000.0 && (!cuts_carrier || lost(*carrier) > 1000.0), fmt::format("library: {} at {} cuts {} and {} mm3", design, angle, lost(*member), lost(*carrier)));
        check(shared(*member, *carrier) < 1.0, fmt::format("library: {} at {} leaves {} mm3 shared", design, angle, shared(*member, *carrier)));
        // every cavity cut into the carrier is filled by the member or a part: no gap the joint leaves open
        Mesh cavity = solid_boolean(carrier->element_geometry_mesh(), carrier->model_geometry_mesh(), SolidOperation::subtract);
        cavity = solid_boolean(cavity, member->model_geometry_mesh(), SolidOperation::subtract);
        for (const std::shared_ptr<JointConnectorPartElement>& part : scene.get_elements<JointConnectorPartElement>())
            cavity = solid_boolean(cavity, part->model_geometry_mesh(), SolidOperation::subtract);
        const double open = cavity.number_of_faces() == 0 ? 0.0 : compute_volume(cavity);
        check(open < 0.01 * lost(*carrier) + 1.0, fmt::format("library: {} at {} leaves {} of {} mm3 cut from the carrier empty", design, angle, open, lost(*carrier)));

        // where the uncut members overlapped, every bit of wood ends in one of them or a part: no slot or mortise cut too deep
        Mesh overlap = solid_boolean(member->element_geometry_mesh(), carrier->element_geometry_mesh(), SolidOperation::intersect);
        const double overlapping = overlap.number_of_faces() == 0 ? 0.0 : compute_volume(overlap);
        overlap = solid_boolean(overlap, member->model_geometry_mesh(), SolidOperation::subtract);
        overlap = solid_boolean(overlap, carrier->model_geometry_mesh(), SolidOperation::subtract);
        for (const std::shared_ptr<JointConnectorPartElement>& part : scene.get_elements<JointConnectorPartElement>())
            overlap = solid_boolean(overlap, part->model_geometry_mesh(), SolidOperation::subtract);
        const double gap = overlap.number_of_faces() == 0 ? 0.0 : compute_volume(overlap);
        check(gap < 0.01 * overlapping + 1.0, fmt::format("library: {} at {} leaves {} of the {} mm3 the members overlapped in neither", design, angle, gap, overlapping));

        // a cut the planar BRep cannot rebuild falls back to a face per triangle: dozens where a joint makes a few
        check(member->model_geometry_brep().face_count() < 30 && carrier->model_geometry_brep().face_count() < 30, fmt::format("library: {} at {} leaves {} and {} faces", design, angle, member->model_geometry_brep().face_count(), carrier->model_geometry_brep().face_count()));
        for (const std::shared_ptr<JointConnectorPartElement>& part : scene.get_elements<JointConnectorPartElement>())
            check(shared(*part, *member) < 1.0 && shared(*part, *carrier) < 1.0, fmt::format("library: {} at {}, its {} shares wood", design, angle, part->name));
        const AssemblySequence sequence(scene, carrier->name);
        check(sequence.locked().empty() && sequence.steps().size() == 2 + joint->parts.size(), fmt::format("library: {} at {} does not come apart\n{}", design, angle, sequence.str()));
        }
}

/// The king post truss comes apart: the tie stays, the king post comes up out of its mortise, every other member out across the plane.
static void king_post() {

    const wood_truss::CarpentryTruss truss(wood_truss::TrussGuide(wood_truss::KingPostCarpentry{}));
    check(truss.get_elements<JointBeamElement>().size() == 9, fmt::format("king_post: 9 joints, not {}", truss.get_elements<JointBeamElement>().size()));

    // mirror images cut alike: a sliver the booleans leave on one side shows as extra faces
    for (const std::array<std::string, 2>& pair : std::vector<std::array<std::string, 2>>{{"chord_1_0", "chord_1_1"}, {"web_1", "web_2"}}) {
        const int left = truss.get_element_by_name<EdgeStraightElement>(pair[0])->model_geometry_brep().face_count();
        const int right = truss.get_element_by_name<EdgeStraightElement>(pair[1])->model_geometry_brep().face_count();
        check(left == right, fmt::format("king_post: {} has {} faces, its mirror {} has {}", pair[0], left, pair[1], right));
    }

    const AssemblySequence sequence(truss, "chord_0_0");
    check(sequence.locked().empty() && sequence.steps().size() == 6, fmt::format("king_post: every member placed\n{}", sequence.str()));
    check(sequence.steps()[1].element == "web_0" && sequence.steps()[1].insertion.dot(Vector(0.0, 0.0, -1.0)) > 0.999, "king_post: the king post goes in second, down into its mortise");

    const std::vector<std::shared_ptr<EdgeStraightElement>> members = truss.get_elements<EdgeStraightElement>();
    for (size_t i = 0; i < members.size(); i++)
        for (size_t j = i + 1; j < members.size(); j++)
            check(shared(*members[i], *members[j]) < 1.0, fmt::format("king_post: {} and {} share wood", members[i]->name, members[j]->name));
}

int main() {

    classification();
    notched_oblique_thrust();
    blind_mortise_tenon();
    library();
    king_post();
    std::cout << "joint_assembly: categories read from axes, two compendium joints cut step by step, the king post truss taken apart" << std::endl;
    return 0;
}
