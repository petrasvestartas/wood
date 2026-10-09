#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;


int main() {

    // WoodSession:
    WoodSession wood_session("elements");

    const std::shared_ptr<Plate> plate0 = Plate::from_rectangle(
        {0, 0, 0},
        {1, 0, 0},
        {0, 1, 0},
        400,
        300,
        40
    );
    const std::shared_ptr<Plate> plate1 = Plate::from_rectangle(
        {150, 0, 40},
        {0, 1, 0},
        {0, 0, 1},
        300,
        400,
        40
    );
    const std::shared_ptr<Beam> beam = std::make_shared<Beam>(Polyline({{500, 50, 900}, {900, 50, 900}}), 50.0);
    const std::shared_ptr<Column> column = std::make_shared<Column>(
        Line::from_points({950, 50, 0}, {950, 50, 950}),
        Polyline::rectangle(
            {900, 0, 0},
            {1, 0, 0},
            {0, 1, 0},
            100,
            100
        )
    );
    const std::shared_ptr<Block> block = std::make_shared<Block>(
        std::vector<Polyline>{
            Polyline::rectangle(
                {1200, 0, 0},
                {1, 0, 0},
                {0, 1, 0},
                200,
                400
            ),
            Polyline::rectangle(
                {1170, 0, 250},
                {1, 0, 0},
                {0, 1, 0},
                260,
                400
            )
        }
    );
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
        Polyline::rectangle(
            {900, 0, 850},
            {0, 1, 0},
            {0, 0, 1},
            100,
            100
        ));
    wood_session.add_interaction(column, beam, contact);



    // Interaction - Plate Contact:
    const Line alignment = Line::from_points({170, 300, 40}, {170, 0, 40});
    const Polyline volume0({{150, 0, 40}, {190, 0, 40}, {190, 0, 0}, {150, 0, 0}, {150, 0, 40}});
    const Polyline volume1({{150, 300, 40}, {190, 300, 40}, {190, 300, 0}, {150, 300, 0}, {150, 300, 40}});
    std::shared_ptr<InteractionContactFace> contact_with_volume = std::make_shared<InteractionContactFace>(
        1,
        5,
        ContactType::side_top,
        Polyline::rectangle(
            {150, 0, 40},
            {1, 0, 0},
            {0, 1, 0},
            40,
            300
        ),
        std::array<Line, 2>{alignment, alignment},
        std::array<Polyline, 4>{volume0, volume1, volume0, volume1}
    );
    wood_session.add_interaction(plate0, plate1, contact_with_volume);

    contact_with_volume->flip();
    joint->orient(contact_with_volume);
    wood_session.add(joint);

    // Apply the joint features to each plate:
    wood_session.add_interaction(plate1, plate0, contact_with_volume); // This only marks that two elements are connected
    wood_session.add_interaction(joint, plate1, joint->interaction_feature(0));
    wood_session.add_interaction(joint, plate0, joint->interaction_feature(1));

    
    // Get element and model geometry:
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
cmake -S . -B build -DCMAKBUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 2_session_hard_coded_contacts --parallel 4 && ./build/2_session_hard_coded_contacts && ../bash/publish-scene.sh --target 2_session_hard_coded_contacts

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
