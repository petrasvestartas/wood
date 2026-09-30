#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;


// TODO checklist: [x] complete, [ ] pending; implementation and verification notes below.
// [x] Follow this file structure, without drastic modification, only implementations.
// [x] Fill correct face indices, lines and volumes for the hard-coded
//     a) contact and b) contact_with_volume. Checked against computed contacts.
// [x] Implement compute_face_contact based on element type: linear elements get
//     a polygon, face IDs and contact type; plates also get lines and volumes.
//     Checked plate and linear pairs, reverse order, absent contacts and serialization.
// [x] Move joinery logic to Elements starting with Joint, e.g. JointPlate and JointBeam,
//     with library variants and custom builder overloads; joint-to-target feature edges.
// [x] Update protobuf for Joints and their serialization.
//     Joint/contact payloads and typed elements pass scene round-trip checks.
// [x] Keep polylines/lines for fast plate merging, and provide mesh and BRep solids
//     for element and model geometry. Closed mesh/BRep and drill-solid checks pass.
// [x] Replace compute_contacts callers with explicit compute_face_contacts,
//     compute_lines_contacts or compute_cross_contacts. Singular line API also retained.
// [x] Preserve plate face/cross detection and joint merging with the new API,
//     and verify performance. Cross, Annen, Vidy and hexshell checks pass; detection avoids plate lofts.
// [x] Automatically detect element types in compute_face_contact and fill the
//     necessary parameters. Plate/plate, beam/column and mixed plate/beam checks pass.
// [x] Remove std::string contact_guid from InteractionFeature and serialization;
//     reserve the old protobuf fields. Joint-to-target edges identify feature ownership.
// [x] Support parametric and custom joints from wood_interaction_feature_plate_joints,
//     including joints connecting more than two elements: JointPlate, JointBeam,
//     JointAnnen for alignment, JointVidy for four plates, and generic Joint cutters
//     by plane/profile. Annen/Vidy grouping, custom library, beam and cutter checks pass.
// [x] Remove the convex-profile limit: concave profiles, holes and disconnected results.
//     Compatible through cuts use polygon booleans; pockets use solid polygon splitting.
// [x] Replace fixed 16-side drills with chord-tolerance meshes and exact cylinder BReps.
// [x] Persist applied solid cutters on Plate, Beam, Column and Block in protobuf.
// [x] Add viewer scenes: 13_profile_cuts, 14_drill_solids, 15_solid_cuts.
// [x] Generate 16_cutting_gallery by loading the three saved scenes: 33 elements.
//     All displayed cut results report closed meshes and solid BReps.
//     Limit: general cut-result BReps are faceted; analytic surface booleans are not implemented.
// [x] Expose named library factories, including JointPlate::ts_e_p_3(divisions, shift).
// [x] Keep joint implementations in the Joint, JointPlate and JointBeam file pairs.
// [x] Keep only ContactType on face contacts; select the library in JointPlate.
//     Side-to-top roles follow face indices; other contacts preserve element order.
// [x] Add 17_plate_joint_library to demonstrate named factory parameters.
// Named-factory/contact cleanup: targets 1_elements and 13 through 17 build and run guarded.
// Earlier verification (before the named-factory/contact cleanup): full CMake build; joint_elements, main_session_round_trip,
// interaction_ownership, 1_elements, 7_custom_joint and 12_cross_joints pass.

int main() {

    // WoodSession:
    WoodSession wood_session("elements");

    const std::shared_ptr<Plate> plate0 = Plate::from_rectangle({0, 0, 0}, {1, 0, 0}, {0, 1, 0}, 400, 300, 40);
    const std::shared_ptr<Plate> plate1 = Plate::from_rectangle({150, 0, 40}, {0, 1, 0}, {0, 0, 1}, 300, 400, 40);
    const std::shared_ptr<Beam> beam = std::make_shared<Beam>(Polyline({{500, 50, 900}, {900, 50, 900}}), 50.0);
    const std::shared_ptr<Column> column = std::make_shared<Column>(Line::from_points({950, 50, 0}, {950, 50, 950}), Polyline::rectangle({900, 0, 0}, {1, 0, 0}, {0, 1, 0}, 100, 100));
    const std::shared_ptr<Block> block = std::make_shared<Block>(std::vector<Polyline>{Polyline::rectangle({1200, 0, 0}, {1, 0, 0}, {0, 1, 0}, 200, 400), Polyline::rectangle({1170, 0, 250}, {1, 0, 0}, {0, 1, 0}, 260, 400)});
    const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_3(8, 0.5);

    wood_session.add(plate0);
    wood_session.add(plate1);
    wood_session.add(beam);
    wood_session.add(column);
    wood_session.add(block);

    std::cout << wood_session << "\n";


    // Interaction - Simple Contact:
    std::shared_ptr<InteractionContactFace> contact = std::make_shared<InteractionContactFace>(
        5,
        5,
        ContactType::end_side,
        Polyline::rectangle({900, 0, 850}, {0, 1, 0}, {0, 0, 1}, 100, 100));
    wood_session.add_interaction(column, beam, contact);

    // Same just automatically computed for one single contact
    std::shared_ptr<InteractionContactFace> computed_contact = wood_session.compute_face_contact(column, beam);
    wood_session.add_interaction(column, beam, computed_contact);


    // Interaction - Plate Contact:
    const Line alignment = Line::from_points({170, 300, 40}, {170, 0, 40});
    const Polyline volume0({{150, 0, 40}, {190, 0, 40}, {190, 0, 0}, {150, 0, 0}, {150, 0, 40}});
    const Polyline volume1({{150, 300, 40}, {190, 300, 40}, {190, 300, 0}, {150, 300, 0}, {150, 300, 40}});
    std::shared_ptr<InteractionContactFace> contact_with_volume = std::make_shared<InteractionContactFace>(
        1,
        5,
        ContactType::side_top,
        Polyline::rectangle({150, 0, 40}, {1, 0, 0}, {0, 1, 0}, 40, 300),
        std::array<Line, 2>{alignment, alignment},
        std::array<Polyline, 4>{volume0, volume1, volume0, volume1}
    );
    wood_session.add_interaction(plate0, plate1, contact_with_volume);

    // Same just automatically computed for one contact, for plates the lines and volumes are computed too
    std::shared_ptr<InteractionContactFace> computed_contact_with_volume = wood_session.compute_face_contact(plate0, plate1);

    if (!computed_contact || !computed_contact_with_volume) return 1;
    computed_contact_with_volume->flip();
    joint->orient(computed_contact_with_volume);
    wood_session.add(joint);

    // Apply the joint features to each plate:
    wood_session.add_interaction(plate1, plate0, computed_contact_with_volume); // This only marks that two elements are connected
    wood_session.add_interaction(joint, plate1, joint->interaction_feature(0));
    wood_session.add_interaction(joint, plate0, joint->interaction_feature(1));




    // Get element and model geometry:
    for (const std::shared_ptr<Element>& element : wood_session.elements()) {
        std::cout << *element << "\n";
        std::cout << element->model_geometry_mesh() << "\n"; // When we call these methods the plates outlines gets merged from interactions with plate outelines.
        std::cout << element->model_geometry_brep() << "\n\n";
    }

    std::cout << plate0->element_geometry_mesh() << "\n";
    std::cout << plate0->element_geometry_brep() << "\n\n";

    // Serialize:
    std::string path = pb_path("live");
    wood_session.pb_dump(path);
    std::cout << path << "\n";

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
plates, a beam, a column, a block and a JointPlate; hard-coded and computed face contacts, including plate alignment lines and volumes; directed joint-to-plate feature edges merge the cut outlines, with mesh and BRep geometry generated on demand and the scene serialized to protobuf.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 1_elements --parallel 4 && ./build/1_elements && ../bash/publish-scene.sh --target 1_elements

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
