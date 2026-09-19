#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The WoodSession API in the order compas_wood_session presents a wood_session: elements, wood_session, interactions, geometry, file.
int main() {

    config::reset_defaults();


    // ═══════════════════════════════════════════════════════════════════════════
    // Create a wood_session with two plates
    // The plates are added to the session's tree and graph, but not yet lofted.
    // ═══════════════════════════════════════════════════════════════════════════


    WoodSession wood_session("two_plates");
    const std::shared_ptr<Plate> plate_a = Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 400, 300, Vector(0, 0, 40));
    const std::shared_ptr<Plate> plate_b = Plate::from_rectangle(Point(400, 0, 0), Vector(0, 1, 0), Vector(0, 0, 1), 300, 300, Vector(40, 0, 0));
    wood_session.add(plate_a);
    wood_session.add(plate_b);

    std::cout << wood_session.str() << std::endl;

    // ═══════════════════════════════════════════════════════════════════════════
    // Compute the contacts and joints between the plates, and print them.
    // ═══════════════════════════════════════════════════════════════════════════


    // Interactions: one record per graph edge, the contacts inside it, the pair on the edge.
    wood_session.compute_contacts();
    for (const auto& [guid, interaction] : wood_session.interactions) {
        const std::pair<std::string, std::string> ends = wood_session.edge_of(interaction);
        for (const InteractionContact& contact : interaction.contacts)
            if (const ContactFace* face = contact.face())
                std::cout << fmt::format("contact: element {} face {} with element {} face {}\n", ends.first, face->face_a, ends.second, face->face_b);
    }

    // // Joints: the modifiers the solver puts on the same edges, one per contact it accepts.
    // wood_session.compute_joints();
    // for (const FeaturePlate& joint : wood_session.joints())
    //     std::cout << fmt::format("joint: type {} ({})\n", joint.joint_type, joint.name);

    // // Geometry: element_geometry is the plate alone, wood_session_geometry the plate with its joints cut in, each as a mesh or a brep; every one is built on first call and cached until the plate changes.
    // for (const std::shared_ptr<Plate>& plate : wood_session.plates()) {
    //     const Mesh& element_mesh = plate->element_geometry_mesh();
    //     const BRep& element_brep = plate->element_geometry_brep();
    //     const Mesh& wood_session_mesh = plate->model_geometry_mesh();
    //     const BRep& wood_session_brep = plate->model_geometry_brep();
    //     std::cout << fmt::format("{}: element {} faces (brep {}), wood_session {} faces (brep {}), volume {:.0f} mm3\n", plate->name, element_mesh.number_of_faces(), element_brep.face_count(), wood_session_mesh.number_of_faces(), wood_session_brep.face_count(), wood_session_mesh.volume());
    // }

    // // File: one group per plate with its outlines, contacts and joints; pb_dump lofts every plate not yet lofted and writes the wood_session geometry.
    // wood_session.add_to_tree(true, true, true, true);
    // wood_session.pb_dump(pb_path("live").string());
    // std::cout << wood_session.str() << std::endl;

    // // Datasets: the same session from data/<name>.yml, which also sets the solver's parameters.
    // WoodSession dataset = WoodSession::yaml_load(config::Dataset::inplane_hexshell);
    // dataset.compute_joints();
    // std::cout << fmt::format("{}: {} plates, {} joints\n", config::Dataset::inplane_hexshell, dataset.plates().size(), dataset.joints().size());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The WoodSession API end to end: elements, wood_session, contacts and joints, the two geometries of a plate, the file.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/4_wood_session_api && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 4_wood_session_api

|||||||| WORKFLOW ||||||||
examples/4_wood_session_api.cpp
 |
 |-- Plate::from_rectangle(origin, x, y, w, h, thickness)   src/joinery_solver/wood_element_plate.cpp
 |    |-- Polyline::rectangle, Polyline::translated          ../session/session_cpp/src/polyline.cpp
 |    '-- Plate(bottom, top)                                  outlines, planes, thickness; no loft
 |
 |-- WoodSession("api"), add(plate)              wood_session.cpp -> Session::add_element (tree + graph)
 |
 |-- compute_contacts()                          see 2_contact_detection: face_contacts -> graph edges
 |-- interactions                                one Interaction per edge: contacts, features, structure
 |
 |-- compute_joints()                            see 3_joint_detection: get_connection_zones -> graph edges
 |-- get_joints()                                every plate feature as a working FeaturePlate
 |
 |-- element_geometry_mesh()  wood_session_geometry_mesh()      wood_element_plate.cpp: Mesh::loft, cached
 |-- element_geometry_brep()  wood_session_geometry_brep()      brep_between_loops -> BRep::from_polylines(faces, holes)
 |                                               ../session/session_cpp/src/brep.cpp (planar fast path), cached
 |
 |-- add_to_tree(true, true, true, true)         the plate, "outlines", "contacts" (add_contacts_to),
 |                                               "joints" (add_joints_to: areas, volumes, lines, cuts)
 |-- pb_dump(pb_path("live"))                    sync_geometry, Session::pb_dump
 |
 '-- WoodSession::yaml_load(dataset), compute_joints(), joints()      the same over data/<dataset>.yml

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
