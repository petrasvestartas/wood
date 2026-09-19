#include "wood_session.h"

using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};   // config::Dataset::<name>

/// Loads the plates, detects the joints, and writes the scene; no plate is lofted before the file is written.
int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_joints();

    /// element_geometry_mesh(): the plate alone, the loft of its two outlines, never cut; compas_model's elementgeometry.
    /// model_geometry_mesh(): the plate with its joints cut in, the loft of the merged outlines; compas_model's modelgeometry, the one to inspect.
    /// Both loft on first call and stay cached until the plate changes; asking for neither keeps the plates as outlines only.
    const std::shared_ptr<Plate> plate = wood_session.plates().front();
    std::cout << fmt::format("{}: {} faces alone, {} faces with joints\n", plate->name, plate->element_geometry_mesh().number_of_faces(), plate->model_geometry_mesh().number_of_faces());

    /// pb_dump lofts every plate not yet lofted (model geometry) so the viewer sees the cut plates; the tree decides what else is drawn.
    wood_session.add_to_tree(true, true, false, false);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Compute joints between wood elements in a dataset.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/3_joint_detection && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 3_joint_detection

|||||||| WORKFLOW ||||||||
examples/3_joint_detection.cpp
 |
 |-- WoodSession::yaml_load(dataset)             wood_session.cpp   (see 1_io: yml, obj, Plate, add)
 |
 |-- compute_joints(search_type)                 wood_joint_solver.cpp, in pipeline order:
 |    |-- clear_features()
 |    |-- config::load_joint_data                   the four txt sidecars as JointData   wood_config.cpp
 |    |-- adjacent_pairs                          adjacency sidecar | adjacency_search   wood_face_to_face.cpp
 |    |-- detect_joints                           face_to_face_wood              wood_face_to_face.cpp
 |    |    |   prepare_candidate -> side_side (in_plane | out_of_plane | rotated) | top_side | top_top
 |    |    '-- cross_fallback -> plane_to_face, CrossJoint               wood_joint_detection.cpp
 |    |-- link_three_valence_joints               vidy shadow joints, annen alignment   wood_three_valence.cpp
 |    |-- build_joint_geometry -> reuse_or_create_geometry -> create_<family>_joint
 |    |    |-- joints/<family>_<id>.h             unit-box male/female outlines      wood_joint_lib.h
 |    |    '-- joint_get_divisions, apply_unit_scale, joint_orient_to_connection_area, merge_linked_joints
 |    |                                                                               wood_joint.cpp
 |    '-- merge_joints -> joint_membership_per_face -> MergeModifier::apply(plate, membership, joints)
 |                                                                         wood_merge_modifier.cpp
 |         '-- plate.features = merged bottom/top outlines (+holes), plate.invalidate_geometry()
 |    |-- add_joint(joint)                        every joint onto its pair's Interaction as a FeaturePlate
 |    '-- sync_joint_features()                   the joint as an ElementFeature on both host elements
 |
 |-- plate->element_geometry_mesh()              wood_element_plate.cpp: Mesh::loft(bottom, top), cached
 |-- plate->model_geometry_mesh()                Mesh::loft(features.bottom, features.top), cached
 |
 |-- add_to_tree(true, true, false, false)       one group per plate: the plate, "outlines" (the merged ones)
 |
 '-- pb_dump(pb_path("live"))                    sync_geometry -> compute_geometry (model mesh onto the
                                                 Element, keeps the joint features), Session::pb_dump

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
