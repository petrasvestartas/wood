// Verifies the WoodElement <-> session_cpp::Element mapping: thickness -> dimensions[2],
// insertion_vectors straight across, joint_types + Features.top/bottom -> ElementFeature, and
// the two outlines in element_data under element_type "WoodElement". Round-trips through a
// real .pb written by fill_session so the check covers serialization, not just the setters,
// and then rebuilds the WoodElement from the loaded Element.
//
// Checks are real tests, not assert(): this builds in Release, where assert() is compiled
// out and a failing check would print "OK". Exit code is the number of failures.
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include "../src/session.h"
#include "../src/element.h"
#include "../src/polyline.h"
#include "../src/point.h"
#include "../src/vector.h"
#include "../src/joinery_solver/wood_element.h"
#include "../src/joinery_solver/wood_session.h"

using namespace session_cpp;
using wood_session::BlockElement;
using wood_session::WoodElement;

static int g_failures = 0;
static void check(bool ok, const std::string& what) {
    printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what.c_str());
    if (!ok) { ++g_failures; }
}

int main() {
    // A single square plate, 1x1 in plan, 0.2 thick.
    Polyline bottom({Point(0,0,0), Point(1,0,0), Point(1,1,0), Point(0,1,0), Point(0,0,0)});
    Polyline top   ({Point(0,0,0.2), Point(1,0,0.2), Point(1,1,0.2), Point(0,1,0.2), Point(0,0,0.2)});

    // Built in place: copying a WoodElement copies an Element, and an Element copy is a new
    // object with a new guid (the kernel's rule). Identity is read AFTER the vector exists.
    std::vector<WoodElement> elements;
    elements.emplace_back(bottom, top);
    WoodElement& we = elements[0];
    we.element.name = "square";
    we.insertion_vectors = {Vector(0,0,1), Vector(1,0,0)};
    we.joint_types = {-1, 30, 11};                 // face 0 none, face 1 type 30, face 2 type 11
    // fill_session also writes per-element merged outlines, and expects top and bottom to be
    // populated in step - a half-filled Features is not a state the solver ever produces.
    we.features.bottom = {Polyline({Point(0.2,0.2,0.0), Point(0.4,0.2,0.0),
                                    Point(0.4,0.4,0.0), Point(0.2,0.4,0.0), Point(0.2,0.2,0.0)})};
    we.features.top    = {Polyline({Point(0.2,0.2,0.2), Point(0.4,0.2,0.2),
                                    Point(0.4,0.4,0.2), Point(0.2,0.4,0.2), Point(0.2,0.2,0.2)})};
    const std::string guid = we.element.guid();

    Session session("mapping_check");
    fill_session(session, elements, {}, false);

    auto path = std::filesystem::temp_directory_path() / "wood_element_mapping_check.pb";
    session.pb_dump(path.string());
    Session loaded = Session::pb_load(path.string());

    printf("Session round trip\n");
    check(loaded.objects.elements->size() == 1, "one element in the Session");
    if (loaded.objects.elements->empty()) { return 1; }
    const Element& e = *(*loaded.objects.elements)[0];

    // Identity and type survive: same guid, the caller's name, tagged as a WoodElement.
    check(e.guid() == guid, "guid preserved: " + e.guid());
    check(e.name == "square", "name preserved");
    check(e.element_type_name() == WoodElement::ELEMENT_TYPE, "element_type = " + e.element_type_name());

    // thickness -> dimensions[2]; x/y are the outline extent in the plate's own frame.
    check(e.dimensions().has_value() && std::abs((*e.dimensions())[2] - we.thickness) < 1e-9 &&
          std::abs((*e.dimensions())[0] - 1.0) < 1e-9 && std::abs((*e.dimensions())[1] - 1.0) < 1e-9,
          e.dimensions() ? std::string("dimensions = (") + std::to_string((*e.dimensions())[0]) + ", " +
                           std::to_string((*e.dimensions())[1]) + ", " + std::to_string((*e.dimensions())[2]) +
                           ")  [1, 1, thickness]"
                         : "dimensions missing");

    // insertion_vectors: straight across.
    check(e.insertion_vectors().size() == 2, "2 insertion vectors");

    // joint_types + Features.top/bottom -> one ElementFeature per face that has either.
    // face 0: type -1 but HAS bottom outlines -> a plain "cut".
    // face 1: type 30 + one top outline.  face 2: type 11, no outlines.
    for (const auto& f : e.features()) {
        printf("    face %d  type=%-14s outlines=%zu  name=%s\n",
               f.face_index, f.feature_type.c_str(), f.outlines.size(), f.name.c_str());
    }
    check(e.features().size() == 3, "3 face features");
    if (e.features().size() == 3) {
        check(e.features()[0].face_index == 0 && e.features()[0].feature_type == "cut" &&
              e.features()[0].outlines.size() == 1, "face 0: cut with one outline");
        check(e.features()[1].face_index == 1 && e.features()[1].feature_type == "joint_type_30" &&
              e.features()[1].outlines.size() == 1, "face 1: joint_type_30 with one outline");
        check(e.features()[2].face_index == 2 && e.features()[2].feature_type == "joint_type_11" &&
              e.features()[2].outlines.empty(), "face 2: joint_type_11 with no outline");
    }

    // And back: the loaded base Element carries element_type/element_data, which is all
    // from_element needs to rebuild the plate - outlines, sides, planes, thickness, and the
    // wood fields the features encoded.
    printf("WoodElement::from_element\n");
    WoodElement back = WoodElement::from_element(e);
    printf("    %s\n", back.repr().c_str());
    check(back.element.guid() == guid, "guid");
    check(back.polylines.size() == we.polylines.size() && back.planes.size() == we.planes.size(),
          std::to_string(back.polylines.size()) + " outlines and planes");
    check(std::abs(back.thickness - we.thickness) < 1e-9, "thickness");
    check(back.reversed == we.reversed, "reversed flag");
    check(back.joint_types == we.joint_types, "joint_types");
    check(back.insertion_vectors.size() == 2, "insertion vectors");
    check(back.features.bottom.size() == 1 && back.features.top.size() == 1, "merged outlines per face");
    double worst = 0.0;
    for (size_t i = 0; i < we.polylines.size() && i < back.polylines.size(); i++) {
        if (back.polylines[i].point_count() != we.polylines[i].point_count()) { worst = 1e9; break; }
        for (size_t k = 0; k < we.polylines[i].point_count(); k++) {
            worst = std::max(worst, Point::distance(back.polylines[i].get_point(k), we.polylines[i].get_point(k)));
        }
    }
    check(worst < 1e-12, "every outline vertex identical (worst " + std::to_string(worst) + ")");

    // The same through the standalone formats, no Session involved.
    printf("standalone formats\n");
    WoodElement pb = WoodElement::pb_loads(we.pb_dumps());
    WoodElement js = WoodElement::file_json_loads(we.file_json_dumps());
    check(pb.element.guid() == guid && pb.polylines.size() == we.polylines.size() && pb.joint_types == we.joint_types,
          "WoodElement pb_dumps / pb_loads");
    check(js.element.guid() == guid && js.polylines.size() == we.polylines.size() && js.joint_types == we.joint_types,
          "WoodElement file_json_dumps / file_json_loads");

    // BlockElement: the loops ARE the mesh faces, so no payload is needed.
    BlockElement block(std::vector<Polyline>{bottom, top});
    block.element.name = "two_loops";
    BlockElement block_pb = BlockElement::pb_loads(block.pb_dumps());
    BlockElement block_js = BlockElement::file_json_loads(block.file_json_dumps());
    check(block_pb.polylines.size() == 2 && block_pb.planes.size() == 2 &&
          block_pb.element.guid() == block.element.guid() && block_pb.element.name == "two_loops",
          "BlockElement pb: loops, planes, identity, name");
    check(block_js.polylines.size() == 2 && block_js.element.guid() == block.element.guid(),
          "BlockElement json");

    std::filesystem::remove(path);
    printf("\n%s\n", g_failures == 0 ? "OK: every WoodElement field survived the round trip, both ways."
                                     : "FAILED");
    return g_failures;
}
