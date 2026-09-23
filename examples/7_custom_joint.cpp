#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const double SEAM_DEPTH = 2.0;
const double TOOTH_HALF = 1.6;
const double NECK_RATIO = 0.2917;

/// One butterfly tooth face in unit-cube space: y at the plate face, x signed across the seam, z along it.
static Polyline tooth(const double y, const double depth) {
    return Polyline({Point(0.0, y, TOOTH_HALF * NECK_RATIO), Point(depth, y, TOOTH_HALF), Point(depth, y, -TOOTH_HALF), Point(0.0, y, -TOOTH_HALF * NECK_RATIO)});
}

int main() {

    WoodSession wood_session("custom_joint");
    wood_session.add(Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 1000, 500, Vector(0, 0, 40)));
    wood_session.add(Plate::from_rectangle(Point(1000, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 1000, 500, Vector(0, 0, 40)));

    wood_session.settings.joint_parameters[0 * 3 + 2] = 9;   // ss_e_ip id 9: the custom variant
    wood_session.settings.custom_joints["ss_e_ip"] = {
        std::vector<Polyline>{tooth(-0.5, -SEAM_DEPTH), tooth(0.5, -SEAM_DEPTH)},
        std::vector<Polyline>{tooth(-0.5, SEAM_DEPTH), tooth(0.5, SEAM_DEPTH)},
    };

    wood_session.compute_features(face_to_face);
    for (const FeaturePlate& joint : wood_session.get_plate_features())
        std::cout << joint.name << " with " << joint.male_outlines[0].front().point_count() << " points per male outline\n";

    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
a joint variant supplied as outlines: two male and two female tooth faces in unit-cube space in the settings, chosen through the family's joint id, tiled along the seam by the solver.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 7_custom_joint --parallel 4 && ./build/7_custom_joint && ../bash/publish-scene.sh --target 7_custom_joint

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
