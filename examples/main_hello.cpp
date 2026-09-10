#include "wood_session.h"
#include "../src/session.h"
#include "../src/mesh.h"

using namespace session_cpp;
using namespace wood_session;

const double SEAM_DEPTH = 0.5 * 4;
const double TOOTH_HALF = 0.4 * 4;
const double NECK_RATIO = 0.2917;

// One butterfly tooth face in unit-cube space: y at the plate face, x signed across the seam, z along it.
static Polyline compute_tooth(const double y, const double depth, const double half, const double neck) {
    return Polyline(std::vector<Point>{
        Point(0.0, y, half * neck),
        Point(depth, y, half),
        Point(depth, y, -half),
        Point(0.0, y, -half * neck),
    });
}

int main() {
    globals::globals_yaml("hello");

    const std::vector<Polyline> polylines = {
        Polyline({
            {-500,   0,   0},
            { 500,   0,   0},
            { 500, 500,   0},
            {-500, 500,   0},
            {-500,   0,   0},
        }),
        Polyline({
            {-500,   0, -15},
            { 500,   0, -15},
            { 500, 500, -15},
            {-500, 500, -15},
            {-500,   0, -15},
        }),
        Polyline({
            {-500, -500,   0},
            { 500, -500,   0},
            { 500,   0,   0},
            {-500,   0,   0},
            {-500, -500,   0},
        }),
        Polyline({
            {-500, -500, -15},
            { 500, -500, -15},
            { 500,   0, -15},
            {-500,   0, -15},
            {-500, -500, -15},
        }),
        Polyline({
            {1000,    0,   0},
            {2000,    0,   0},
            {2000,  500,   0},
            {1000,  500,   0},
            {1000,    0,   0},
        }),
        Polyline({
            {1000,    0, -15},
            {2000,    0, -15},
            {2000,  500, -15},
            {1000,  500, -15},
            {1000,    0, -15},
        }),
        Polyline({
            {1000, -500, 134},
            {2000, -500, 134},
            {2000,    0,   0},
            {1000,    0,   0},
            {1000, -500, 134},
        }),
        Polyline({
            {1000, -500, 119},
            {2000, -500, 119},
            {2000,    0, -15},
            {1000,    0, -15},
            {1000, -500, 119},
        }),
    };

    globals::CUSTOM_JOINTS_SS_E_IP_MALE = {
        compute_tooth(-0.5, -SEAM_DEPTH, TOOTH_HALF, NECK_RATIO),
        compute_tooth(0.5, -SEAM_DEPTH, TOOTH_HALF, NECK_RATIO),
    };
    globals::CUSTOM_JOINTS_SS_E_IP_FEMALE = {
        compute_tooth(-0.5, SEAM_DEPTH, TOOTH_HALF, NECK_RATIO),
        compute_tooth(0.5, SEAM_DEPTH, TOOTH_HALF, NECK_RATIO),
    };

    WoodSession scene(globals::DATA_SET_INPUT_NAME);
    for (size_t i = 0; i + 1 < polylines.size(); i += 2)
        scene.add(std::make_shared<WoodElement>(polylines[i], polylines[i + 1]));
    scene.compute_joints(face_to_face);
    pb_dump(scene, "live");
    return 0;
}

/*
description: four hardcoded plates with a custom butterfly joint -> face_to_face joints on the graph edges of their pairs.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_hello -j8 && ./build/main_hello
cloudflare: ../bash/publish-scene.sh --target main_hello
view: https://petrasvestartas.github.io/session/
*/
