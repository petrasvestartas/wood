#include "wood_session.h"
#include "wood_assignment.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    WoodSession wood_session("assignment");
    wood_session.add(Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 1000, 500, Vector(0, 0, 40)));
    wood_session.add(Plate::from_rectangle(Point(1000, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 1000, 500, Vector(0, 0, 40)));

    // A point near a side slot with a positive type sets that side; a negative type names the bottom or top face.
    assign_feature_types(wood_session.plates(), wood_session.settings, {Point(1000, 250, 0), Point(500, 250, 40)}, {3, -40});
    // A line whose start sits near a side slot sets that side's insertion vector.
    assign_insertion_vectors(wood_session.plates(), wood_session.settings, {Line::from_points(Point(1000, 250, 0), Point(1000, 250, 300))});

    for (const std::shared_ptr<Plate>& plate : wood_session.plates()) {
        std::cout << plate->name << " feature types:";
        for (int type : plate->feature_types)
            std::cout << " " << type;
        std::cout << " | insertion:";
        for (const Vector& v : plate->insertion_vectors())
            std::cout << " (" << v[0] << "," << v[1] << "," << v[2] << ")";
        std::cout << "\n";
    }

    wood_session.compute_features(face_to_face);
    std::cout << wood_session.get_plate_features().size() << " joints from the assigned types\n";

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
the per-face tables the sidecars would give, filled from geometry placed on the plates: points with a type into feature_types, lines into the insertion vectors, then a solve that reads them.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 8_assignment --parallel 4 && ./build/8_assignment && ../bash/publish-scene.sh --target 8_assignment

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
