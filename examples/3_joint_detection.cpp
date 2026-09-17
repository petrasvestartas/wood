#include "wood_session.h"

using namespace wood_session;

// TODO:
// 1. we need to capture better the attribute of datasest in yaml files, it feels from test files of original several years old files we did not transfer attributes correctily.
// 2. joint_volume_extension in yml files very often do not have any effect on negative or positive values, why?
// 3. The kernel is full of Polyline convertion to std::vector<Point>, use only polylines types.
// 4. Apply the /session-reviewer
// 5. We are using loft_mesh function in wood_element but is must be rather in session_cpp
// 6. wood_session.cpp Group WoodSession::add(const WoodGeometry& object, const Group& parent)  Why here we convert from woo to element again??? we are in woodsesion so it remains!
// 7. Plate must extend Element class 
// 8. Each type of element Plate, Column, Block must have separate file.
// 9. Why loft method is computed three times?
// 10. add to viewer methods, must be attributes of session class, so that we call them like that wood_session.add_joints_by_type wood_session.add_element_geometry
// 11. Do we correctly add element to class or lofted geometry of plates and lofted geometry of plate with features are already present in session? session.add_mesh(plate->element->geometry()); 
// 12. The classes became monstrous, there must be a more readable simplified logic. It is extremely hard to follow the API as a user. We need compentarlizaiton.

// globals::Dataset::Face::<name> for SEARCH = face_to_face, ::Cross::<name> for cross_joint.
const std::string DATASET = globals::Dataset::inplane_hexshell;
const SearchType SEARCH = SearchType::face_to_face;  // face_to_face | cross_joint | face_to_face_then_cross <- these must be part of dataset, so we wont type that.

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);

    // Finds nearby element pairs using OBB/BVH.
    // Tests face planes for coplanarity.
    // Computes the polygon overlap.
    // Stores every valid touching face pair.
    // joint alignment lines
    // minimum joint length
    // insertion direction
    // plate orientation
    // dihedral angle
    // male/female orientation
    // joint volume and cut geometry
    wood_session.compute_joints(SEARCH);

    // TODO: view.add_contacts_by_type | view.add_outlines | view.pb_dump
    // side-side, out-of-plane        orange    11
    // side-side, in-plane            navy      12
    // rotated side-side              dark pink 13
    // top-to-side                              20
    // top-to-top                               40
    // crossing geometry                        30
    add_joints_by_type(wood_session, wood_session.joints());
    pb_dump(wood_session, "live");
    
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Compute joints between wood elements in a dataset.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/3_joint_detection && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 3_joint_detection

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/