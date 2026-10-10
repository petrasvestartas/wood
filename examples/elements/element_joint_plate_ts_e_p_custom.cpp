#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ts_e_p_custom: your own rectangles in the joint's unit box, a notch into the bottom edge of an upright plate standing on a base plate, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ts_e_p_custom");

    // a 400 x 400 base plate and a 250 x 250 upright standing in the middle of its top face, both 40 thick
    const std::shared_ptr<Plate> base = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 400.0, 40.0, "base");
    const std::shared_ptr<Plate> upright = Plate::from_rectangle({180.0, 75.0, 40.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 250.0, 250.0, 40.0, "upright");
    scene.add(base);
    scene.add(upright);

    // the user's tile in the joint's unit box, mapped onto the contact: the upright's thickness along x, the base's along y, z along the
    // 250 long joint line; two dovetailed tenons, each 50 long at the base and 70 at its end, reaching down through the base: the upright's
    // profile per face (x = 0.5 face 0, x = -0.5 face 1) an open polyline from one end of the joint line to the other, cut into its edge;
    // the base's mortises per face (y = -0.5 face 0, y = 0.5 face 1) a closed rectangle each, 70 long so the tenons' heads pass, cut as holes
    std::array<std::vector<Point>, 2> profile;
    for (int face = 0; face < 2; face++) {
        const double x = face == 0 ? 0.5 : -0.5;
        profile[face] = {Point(x, -0.5, 0.5)};
        for (const double centre : {0.25, -0.25}) {
            profile[face].insert(profile[face].end(), {
                Point(x, -0.5, centre + 0.1), Point(x, 0.5, centre + 0.14), Point(x, 0.5, centre - 0.14), Point(x, -0.5, centre - 0.1),
            });
        }
        profile[face].push_back(Point(x, -0.5, -0.5));
    }
    std::array<std::vector<Polyline>, 2> mortises;
    for (int face = 0; face < 2; face++) {
        const double y = face == 0 ? -0.5 : 0.5;
        for (const double centre : {0.25, -0.25})
            mortises[face].push_back(Polyline({{-0.5, y, centre + 0.14}, {0.5, y, centre + 0.14}, {0.5, y, centre - 0.14}, {-0.5, y, centre - 0.14}, {-0.5, y, centre + 0.14}}));
    }
    const std::vector<Polyline> male = {Polyline(profile[0]), Polyline(profile[1])};
    const std::vector<Polyline> female = {mortises[0][0], mortises[1][0], mortises[0][1], mortises[1][1]};

    // the joint from their contact, added, and passed to each plate in its target order: the upright first, the male of a top-side pair
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
    const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_custom(male, female);
    joint->orient(contact, {base, upright});
    scene.add(joint);
    scene.add_interaction(joint, upright, joint->interaction(0));
    scene.add_interaction(joint, base, joint->interaction(1));

    // drawn apart along the base's normal, the direction the upright drops onto the base, so the tenons and the mortises both read
    upright->place(Xform::translation(0.0, 0.0, 120.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The top-side design ts_e_p_custom on one pair: a 400 x 400 base and a 250 x 250 upright standing on its top, both 40 thick, joined by a tile the user draws in the joint's unit box: two dovetailed tenons, 50 long where they leave the upright and 70 at their ends, reaching down through the base. The upright's profile is an open polyline per face from one end of the joint line to the other, cut into its edge; the base's mortises are a closed rectangle each per face, 70 long so the tenons' heads pass, cut as holes. 2024 left a custom pair uncut, so its base never took the mortises; ts_e_p_custom cuts each side as the library's own top-side designs do. The joint is oriented on the face contact and passed to each plate with add_interaction, the upright first; the upright is lifted 120 off afterwards so tenons and mortises read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ts_e_p_custom --parallel 6 && ./build/element_joint_plate_ts_e_p_custom && ../bash/publish-scene.sh --target element_joint_plate_ts_e_p_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
