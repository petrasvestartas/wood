#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The WoodSession API in the order compas_model presents a model: elements, the model, its interactions, the geometry of each element, the file.
int main() {

    globals::reset_defaults();   // the solver's parameters, as a yml would set them

    // ═══════════════════════════════════════════════════════════════════════════
    // Elements - a Plate is a session_cpp::Element; the constructor keeps outlines only, nothing is lofted yet
    // ═══════════════════════════════════════════════════════════════════════════

    const Polyline bottom_a({Point(0, 0, 0), Point(400, 0, 0), Point(400, 300, 0), Point(0, 300, 0), Point(0, 0, 0)});
    const Polyline top_a({Point(0, 0, 40), Point(400, 0, 40), Point(400, 300, 40), Point(0, 300, 40), Point(0, 0, 40)});
    const std::shared_ptr<Plate> plate_a = std::make_shared<Plate>(bottom_a, top_a, "plate");

    const Polyline bottom_b({Point(400, 0, 0), Point(400, 300, 0), Point(400, 300, 300), Point(400, 0, 300), Point(400, 0, 0)});   // its bottom face lies on plate_a's end face
    const Polyline top_b({Point(440, 0, 0), Point(440, 300, 0), Point(440, 300, 300), Point(440, 0, 300), Point(440, 0, 0)});
    const std::shared_ptr<Plate> plate_b = std::make_shared<Plate>(bottom_b, top_b, "plate");

    fmt::print("plate_a: {} outlines, thickness {}\n", plate_a->polylines.size(), plate_a->thickness);

    // ═══════════════════════════════════════════════════════════════════════════
    // Model - a WoodSession is a session_cpp::Session; add() puts an element in the tree and in the interaction graph
    // ═══════════════════════════════════════════════════════════════════════════

    WoodSession model("api");
    model.add(plate_a);
    model.add(plate_b);
    fmt::print("model: {} plates, {} elements\n", model.plates().size(), model.element_guids().size());

    // ═══════════════════════════════════════════════════════════════════════════
    // Interactions - contacts are face overlaps on the graph edges; joints are the modifiers the solver puts on the same edges
    // ═══════════════════════════════════════════════════════════════════════════

    model.compute_contacts();
    for (const ContactPair& pair : model.contacts())
        for (const FaceContact& contact : pair.faces)
            fmt::print("contact: element {} face {} with element {} face {}, {} points\n", pair.element_a, contact.face_a, pair.element_b, contact.face_b, contact.area.point_count());

    model.compute_joints();
    for (const WoodJoint& joint : model.joints())
        fmt::print("joint: type {} ({}) between {} and {}\n", joint.joint_type, joint.name, joint.element_a.substr(0, 8), joint.element_b.substr(0, 8));

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry - compas_model's elementgeometry and modelgeometry; each lofts on first call and is cached until the plate changes
    // ═══════════════════════════════════════════════════════════════════════════

    for (const std::shared_ptr<Plate>& plate : model.plates()) {
        const Mesh& alone = plate->element_geometry();        // the plate alone, never cut
        const Mesh& with_joints = plate->model_geometry();    // the plate with its joints cut in, the one to inspect
        const BRep& brep = plate->model_brep();               // the same solid as a boundary representation, opt-in
        fmt::print("{}: {} faces alone, {} faces with joints, brep {} faces, volume {:.0f} mm3\n", plate->name, alone.number_of_faces(), with_joints.number_of_faces(), brep.face_count(), with_joints.volume());
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // File - one group per plate with its outlines, contacts and joints; pb_dump lofts every plate not yet lofted and writes the model geometry
    // ═══════════════════════════════════════════════════════════════════════════

    model.add_to_tree(true, true, true, true);
    model.pb_dump(pb_path("live").string());
    fmt::print("{}\n", model.str());

    // ═══════════════════════════════════════════════════════════════════════════
    // Datasets - the same session from a data/<name>.yml, which also sets the solver's parameters
    // ═══════════════════════════════════════════════════════════════════════════

    WoodSession dataset = WoodSession::yaml_load(globals::Dataset::inplane_hexshell);
    dataset.compute_joints();
    fmt::print("{}: {} plates, {} joints\n", globals::Dataset::inplane_hexshell, dataset.plates().size(), dataset.joints().size());
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The WoodSession API end to end: elements, model, contacts and joints, the two geometries of a plate, the file.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/4_wood_session_api && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 4_wood_session_api

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
