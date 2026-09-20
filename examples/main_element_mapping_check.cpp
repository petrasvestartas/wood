#include "../src/joinery_solver/wood_session.h"

using namespace session_cpp;
using namespace wood_session;

static int failures = 0;
static void check(const bool ok, const std::string& what) {

    if (ok)
        return;

    std::cout << fmt::format("FAIL {}\n", what);
    failures++;
}

int main() {

    const Polyline bottom({Point(0,0,0), Point(1,0,0), Point(1,1,0), Point(0,1,0), Point(0,0,0)});
    const Polyline top({Point(0,0,0.2), Point(1,0,0.2), Point(1,1,0.2), Point(0,1,0.2), Point(0,0,0.2)});

    const std::shared_ptr<Plate> we = std::make_shared<Plate>(bottom, top, "square");
    we->insertion_vectors() = {Vector(0,0,1), Vector(1,0,0)};
    we->feature_types = {-1, 30, 11};
    we->features.bottom = {Polyline({Point(0.2,0.2,0.0), Point(0.4,0.2,0.0),
                                     Point(0.4,0.4,0.0), Point(0.2,0.4,0.0), Point(0.2,0.2,0.0)})};
    we->features.top    = {Polyline({Point(0.2,0.2,0.2), Point(0.4,0.2,0.2),
                                     Point(0.4,0.4,0.2), Point(0.2,0.4,0.2), Point(0.2,0.2,0.2)})};
    we->compute_geometry();
    const std::string guid = we->guid();

    WoodSession session("mapping_check");
    session.add(we);

    const std::filesystem::path path = std::filesystem::temp_directory_path() / "wood_element_mapping_check.pb";
    session.pb_dump(path.string());
    const Session loaded = Session::pb_load(path.string());
    check(loaded.objects.elements->size() == 1, "one element in the Session");

    if (loaded.objects.elements->empty())
        return 1;

    const std::shared_ptr<Element> element = (*loaded.objects.elements)[0];
    const Element& e = *element;

    check(e.guid() == guid, "guid preserved: " + e.guid());
    check(e.name == "square", "name preserved");
    check(e.element_type_name() == Plate::ELEMENT_TYPE, "element_type = " + e.element_type_name());

    check(e.dimensions().has_value() && std::abs((*e.dimensions())[2] - we->thickness) < 1e-9 &&
          std::abs((*e.dimensions())[0] - 1.0) < 1e-9 && std::abs((*e.dimensions())[1] - 1.0) < 1e-9,
          e.dimensions() ? std::string("dimensions = (") + std::to_string((*e.dimensions())[0]) + ", " +
                           std::to_string((*e.dimensions())[1]) + ", " + std::to_string((*e.dimensions())[2]) +
                           ")  [1, 1, thickness]"
                         : "dimensions missing");

    check(e.insertion_vectors().size() == 2, "2 insertion vectors");

    check(e.features().size() == 3, "3 face features");
    if (e.features().size() == 3) {
        check(e.features()[0].face_index == 0 && e.features()[0].feature_type == "cut" &&
              e.features()[0].outlines.size() == 1, "face 0: cut with one outline");
        check(e.features()[1].face_index == 1 && e.features()[1].feature_type == "joint_type_30" &&
              e.features()[1].outlines.size() == 1, "face 1: joint_type_30 with one outline");
        check(e.features()[2].face_index == 2 && e.features()[2].feature_type == "joint_type_11" &&
              e.features()[2].outlines.empty(), "face 2: joint_type_11 with no outline");
    }

    const std::shared_ptr<Plate> back = std::dynamic_pointer_cast<Plate>(element);
    check(back != nullptr, "Session::pb_load rebuilt the element as a Plate through the registry");

    if (!back)
        return failures;

    check(back->guid() == guid, "guid");
    check(back->polylines.size() == we->polylines.size() && back->planes.size() == we->planes.size(),
          std::to_string(back->polylines.size()) + " outlines and planes");
    check(std::abs(back->thickness - we->thickness) < 1e-9, "thickness");
    check(back->reversed == we->reversed, "reversed flag");
    check(back->feature_types == we->feature_types, "feature_types");
    check(back->insertion_vectors().size() == 2, "insertion vectors");
    check(back->features.bottom.size() == 1 && back->features.top.size() == 1, "merged outlines per face");

    double worst = 0.0;
    for (size_t i = 0; i < we->polylines.size() && i < back->polylines.size(); i++) {

        if (back->polylines[i].point_count() != we->polylines[i].point_count()) {
            worst = 1e9;
            break;
        }

        for (size_t k = 0; k < we->polylines[i].point_count(); k++)
            worst = std::max(worst, Point::distance(back->polylines[i].get_point(k), we->polylines[i].get_point(k)));
    }

    check(worst < 1e-12, "every outline vertex identical (worst " + std::to_string(worst) + ")");

    const std::shared_ptr<Plate> pb = std::dynamic_pointer_cast<Plate>(Element::pb_loads_polymorphic(we->pb_dumps()));
    const std::shared_ptr<Plate> js = std::dynamic_pointer_cast<Plate>(Element::file_json_loads_polymorphic(we->file_json_dumps()));
    check(pb && pb->guid() == guid && pb->polylines.size() == we->polylines.size() && pb->feature_types == we->feature_types,
          "Plate pb_dumps / pb_loads_polymorphic");
    check(js && js->guid() == guid && js->polylines.size() == we->polylines.size() && js->feature_types == we->feature_types,
          "Plate file_json_dumps / file_json_loads_polymorphic");

    Block block(std::vector<Polyline>{bottom, top}, "two_loops");
    const std::string block_guid = block.guid();
    const std::shared_ptr<Block> block_pb = std::dynamic_pointer_cast<Block>(Element::pb_loads_polymorphic(block.pb_dumps()));
    const std::shared_ptr<Block> block_js = std::dynamic_pointer_cast<Block>(Element::file_json_loads_polymorphic(block.file_json_dumps()));
    check(block_pb && block_pb->polylines().size() == 2 && block_pb->planes().size() == 2 &&
          block_pb->guid() == block_guid && block_pb->name == "two_loops",
          "Block pb: loops, planes, identity, name");
    check(block_js && block_js->polylines().size() == 2 && block_js->guid() == block_guid,
          "Block json");

    std::filesystem::remove(path);

    return failures;
}

/*
description: one plate -> WoodSession -> pb round trip -> Element fields and the Plate the registry rebuilds checked; prints only what differs, exit code = failures.

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_element_mapping_check --parallel 4 && ./build/main_element_mapping_check
*/
