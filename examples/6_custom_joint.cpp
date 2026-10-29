#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const double SEAM_DEPTH = 1.5;
const double TOOTH_HALF = 0.06;
const double NECK_RATIO = 0.45;

/// Three butterfly half-mortise teeth in unit-cube space: y at the plate face, x signed across the seam, z in [-0.5, 0.5] along it.
static Polyline teeth(const double y, const double depth) {
    std::vector<Point> pts;
    for (const double cz : {0.28, 0.0, -0.28}) {
        pts.emplace_back(0.0, y, cz + TOOTH_HALF * NECK_RATIO);
        pts.emplace_back(depth, y, cz + TOOTH_HALF);
        pts.emplace_back(depth, y, cz - TOOTH_HALF);
        pts.emplace_back(0.0, y, cz - TOOTH_HALF * NECK_RATIO);
    }
    return Polyline(pts);
}

int main() {

    WoodSession wood_session("custom_joint");
    wood_session.add(
        Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 1000, 500, 40)
    );
    wood_session.add(
        Plate::from_rectangle(Point(1000, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 1000, 500, 40)
    );

    wood_session.settings.joint_parameters[0 * 3 + 2] = 9;
    wood_session.settings.custom_joints["side_side_inplane"] = {
        std::vector<Polyline>{teeth(-0.5, -SEAM_DEPTH), teeth(0.5, -SEAM_DEPTH)},
        std::vector<Polyline>{teeth(-0.5, SEAM_DEPTH), teeth(0.5, SEAM_DEPTH)},
    };

    wood_session.compute_features(face_to_face);
    for (const InteractionFeaturePlate& joint : wood_session.get_plate_features())
        std::cout << joint.name << " with " << joint.male_outlines[0].front().point_count()
                  << " points per male outline\n";

    wood_session.pb_dump(pb_path("live"));

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
cmake --build build --target 6_custom_joint --parallel 4 && ./build/6_custom_joint && ../bash/publish-scene.sh --target 6_custom_joint

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
