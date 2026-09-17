#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The WoodSession API in the order compas_model presents a model: elements, model, interactions, geometry, file.
int main() {

    globals::reset_defaults();

    // Elements: a Plate is a session_cpp::Element; the constructor keeps the outlines, nothing is lofted yet.
    const std::shared_ptr<Plate> plate_a = Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 400, 300, Vector(0, 0, 40));
    const std::shared_ptr<Plate> plate_b = Plate::from_rectangle(Point(400, 0, 0), Vector(0, 1, 0), Vector(0, 0, 1), 300, 300, Vector(40, 0, 0));
    std::cout << fmt::format("plate_a: {} outlines, thickness {}\n", plate_a->polylines.size(), plate_a->thickness);

    // Model: a WoodSession is a session_cpp::Session; add() puts an element in the tree and in the interaction graph.
    WoodSession model("api");
    model.add(plate_a);
    model.add(plate_b);
    std::cout << fmt::format("model: {} plates, {} elements\n", model.plates().size(), model.element_guids().size());

    // Interactions: contacts are the face overlaps stored on the graph edges.
    model.compute_contacts();
    for (const ContactPair& pair : model.contacts())
        for (const FaceContact& contact : pair.faces)
            std::cout << fmt::format("contact: element {} face {} with element {} face {}\n", pair.element_a, contact.face_a, pair.element_b, contact.face_b);

    // Joints: the modifiers the solver puts on the same edges, one per contact it accepts.
    model.compute_joints();
    for (const WoodJoint& joint : model.joints())
        std::cout << fmt::format("joint: type {} ({})\n", joint.joint_type, joint.name);

    // Geometry: element_geometry is the plate alone, model_geometry the plate with its joints cut in, each as a mesh or a brep; every one is built on first call and cached until the plate changes.
    for (const std::shared_ptr<Plate>& plate : model.plates()) {
        const Mesh& element_mesh = plate->element_geometry_mesh();
        const BRep& element_brep = plate->element_geometry_brep();
        const Mesh& model_mesh = plate->model_geometry_mesh();
        const BRep& model_brep = plate->model_geometry_brep();
        std::cout << fmt::format("{}: element {} faces (brep {}), model {} faces (brep {}), volume {:.0f} mm3\n", plate->name, element_mesh.number_of_faces(), element_brep.face_count(), model_mesh.number_of_faces(), model_brep.face_count(), model_mesh.volume());
    }

    // File: one group per plate with its outlines, contacts and joints; pb_dump lofts every plate not yet lofted and writes the model geometry.
    model.add_to_tree(true, true, true, true);
    model.pb_dump(pb_path("live").string());
    std::cout << model.str() << std::endl;

    // Datasets: the same session from data/<name>.yml, which also sets the solver's parameters.
    WoodSession dataset = WoodSession::yaml_load(globals::Dataset::inplane_hexshell);
    dataset.compute_joints();
    std::cout << fmt::format("{}: {} plates, {} joints\n", globals::Dataset::inplane_hexshell, dataset.plates().size(), dataset.joints().size());

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
