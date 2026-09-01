// Verifies the WoodElement -> session_cpp::Element field mapping in fill_session():
// thickness -> dimensions[2], insertion_vectors straight across, joint_types + Features.top/
// bottom -> ElementFeature. Round-trips through a real .pb so the check covers serialization,
// not just the setters.
#include <cassert>
#include <cstdio>
#include <filesystem>
#include "../src/session.h"
#include "../src/element.h"
#include "../src/polyline.h"
#include "../src/point.h"
#include "../src/vector.h"
#include "../src/joinery_solver/wood_element.h"
#include "../src/joinery_solver/wood_session.h"

using namespace session_cpp;
using wood_session::WoodElement;

int main() {
    // A single square plate, 1x1 in plan, 0.2 thick.
    Polyline bottom({Point(0,0,0), Point(1,0,0), Point(1,1,0), Point(0,1,0), Point(0,0,0)});
    Polyline top   ({Point(0,0,0.2), Point(1,0,0.2), Point(1,1,0.2), Point(0,1,0.2), Point(0,0,0.2)});

    WoodElement we(bottom, top);
    we.insertion_vectors = {Vector(0,0,1), Vector(1,0,0)};
    we.joint_types = {-1, 30, 11};                 // face 0 none, face 1 type 30, face 2 type 11
    // fill_session also writes per-element merged outlines, and expects top and bottom to be
    // populated in step - a half-filled Features is not a state the solver ever produces.
    we.features.bottom = {Polyline({Point(0.2,0.2,0.0), Point(0.4,0.2,0.0),
                                    Point(0.4,0.4,0.0), Point(0.2,0.4,0.0), Point(0.2,0.2,0.0)})};
    we.features.top    = {Polyline({Point(0.2,0.2,0.2), Point(0.4,0.2,0.2),
                                    Point(0.4,0.4,0.2), Point(0.2,0.4,0.2), Point(0.2,0.2,0.2)})};

    Session session("mapping_check");
    fill_session(session, {we}, {}, false);

    auto path = std::filesystem::temp_directory_path() / "wood_element_mapping_check.pb";
    session.pb_dump(path.string());
    Session loaded = Session::pb_load(path.string());

    assert(loaded.objects.elements->size() == 1);
    const auto& e = *(*loaded.objects.elements)[0];

    // thickness -> dimensions[2]; x/y are the outline extent in the plate's own frame.
    assert(e.dimensions().has_value());
    printf("dimensions      = (%.3f, %.3f, %.3f)   [z should be ~0.2, x/y ~1.0]\n",
           (*e.dimensions())[0], (*e.dimensions())[1], (*e.dimensions())[2]);
    assert(std::abs((*e.dimensions())[2] - we.thickness) < 1e-9);

    // insertion_vectors: straight across.
    printf("insertion_vecs  = %zu (expected 2)\n", e.insertion_vectors().size());
    assert(e.insertion_vectors().size() == 2);

    // joint_types + Features.top/bottom -> one ElementFeature per face that has either.
    printf("features        = %zu\n", e.features().size());
    for (const auto& f : e.features()) {
        printf("  face %d  type=%-10s outlines=%zu  name=%s\n",
               f.face_index, f.feature_type.c_str(), f.outlines.size(), f.name.c_str());
    }
    // face 0: type -1 but HAS bottom outlines -> a plain "cut".
    // face 1: type 30 + one top outline.  face 2: type 11, no outlines.
    assert(e.features().size() == 3);
    assert(e.features()[0].face_index == 0 && e.features()[0].feature_type == "cut");
    assert(e.features()[0].outlines.size() == 1);
    assert(e.features()[1].face_index == 1 && e.features()[1].feature_type == "joint_30");
    assert(e.features()[1].outlines.size() == 1);
    assert(e.features()[2].face_index == 2 && e.features()[2].feature_type == "joint_11");
    assert(e.features()[2].outlines.empty());

    std::filesystem::remove(path);
    printf("\nOK: every WoodElement field survived the round trip.\n");
    return 0;
}
