#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;


int main() {

    // WoodSession:
    WoodSession wood_session("elements");

    const std::shared_ptr<Plate> plate0 = Plate::from_rectangle({0, 0, 0}, {1, 0, 0}, {0, 1, 0}, 400, 300, 40);
    const std::shared_ptr<Plate> plate1 = Plate::from_rectangle({150, 0, 40}, {0, 1, 0}, {0, 0, 1}, 300, 400, 40);
    const std::shared_ptr<Beam> beam = std::make_shared<Beam>(Polyline({{500, 50, 900}, {900, 50, 900}}), 50.0);
    const std::shared_ptr<Column> column = std::make_shared<Column>(Line::from_points({950, 50, 0}, {950, 50, 950}), Polyline::rectangle({900, 0, 0}, {1, 0, 0}, {0, 1, 0}, 100, 100));
    const std::shared_ptr<Block> block = std::make_shared<Block>(std::vector<Polyline>{Polyline::rectangle({1200, 0, 0}, {1, 0, 0}, {0, 1, 0}, 200, 400), Polyline::rectangle({1170, 0, 250}, {1, 0, 0}, {0, 1, 0}, 260, 400)});
    const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_3(8, 0.5);
    joint->is_visible = false; // joints start hidden, the viewer lists them with the lamp off; true draws it

    wood_session.add(plate0);
    wood_session.add(plate1);
    wood_session.add(beam);
    wood_session.add(column);
    wood_session.add(block);
    wood_session.add(joint);

    std::cout << wood_session << "\n";

    // Compute contact
    std::shared_ptr<InteractionContactFace> contact = wood_session.compute_face_contact(column, beam);
    wood_session.add_interaction(column, beam, contact);
    // std::cout << *contact;

    // Compute contact with volume and orient the joint to it, then add the joint features to the plates:
    std::shared_ptr<InteractionContactFace> contact_with_volume = wood_session.compute_face_contact(plate0, plate1); // compute contact
    joint->orient(contact_with_volume); // orient the joint to the contact
    wood_session.add_interaction(plate1, plate0, contact_with_volume); // this only marks that two elements are connected
    wood_session.add_interaction(joint, plate1, joint->interaction_feature(0)); // this adds the joint male feature to the plate1
    wood_session.add_interaction(joint, plate0, joint->interaction_feature(1)); // this adds the joint female featureto the plate0
    // std::cout << *contact_with_volume << "\n";

    // Element and model geometry:
    for (const std::shared_ptr<Element>& element : wood_session.elements()) {
        std::cout << *element << "\n";
        std::cout << element->model_geometry_mesh() << "\n"; // When we call these methods the plates outlines gets merged from interactions with plate outelines.
        std::cout << element->model_geometry_brep() << "\n";
        std::cout << plate0->element_geometry_mesh() << "\n";
        std::cout << plate0->element_geometry_brep() << "\n";
    }

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
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 1_session_contacts --parallel 4 && ./build/1_session_contacts && ../bash/publish-scene.sh --target 1_session_contacts

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
