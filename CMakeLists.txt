cmake_minimum_required(VERSION 3.22)
project(wood CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# session_cpp: where the kernel comes from. First hit wins:
#
#   1. -DSESSION_CPP_LOCAL=<dir>, or the SESSION_CPP_LOCAL environment variable
#   2. ../session_cpp next to this repo - a plain checkout beside wood.
#   3. ../session/session_cpp - the wood_research layout (see ../README.md), where
#      the kernel arrives inside the `session` monorepo it is authored in. One
#      checkout shared by wood and wood_nano, so `git pull` there updates both.
#   4. a fresh FetchContent clone of GitHub main (CI, or a lone checkout)
#
# This used to be a bare `set(SESSION_CPP_DIR "C:/pc/3_code/.../session_cpp")`
# pointing at one developer's machine, so configuring anywhere else failed with
# `add_subdirectory given source ... which is not an existing directory`, and
# then `Unknown CMake command "LINK_PROTOBUF"` because that macro comes from
# session_cpp and was never defined. That path stays as a last local candidate
# so the machine it named keeps working. wood_nano resolves the kernel the same
# way, in the same order.
include(FetchContent)

set(SESSION_CPP_LOCAL "" CACHE PATH
    "Local session_cpp checkout; ../session_cpp, ../session/session_cpp, or a GitHub clone when empty")

set(SESSION_CPP_DIR "")
foreach(_candidate IN ITEMS
        "${SESSION_CPP_LOCAL}"
        "$ENV{SESSION_CPP_LOCAL}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../session_cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../session/session_cpp"
        "C:/pc/3_code/code_rust/session/session_cpp")
    if(NOT "${_candidate}" STREQUAL "" AND EXISTS "${_candidate}/CMakeLists.txt")
        get_filename_component(SESSION_CPP_DIR "${_candidate}" REALPATH)
        break()
    endif()
endforeach()

if(SESSION_CPP_DIR)
    message(STATUS "wood: using local session_cpp at ${SESSION_CPP_DIR}")
else()
    FetchContent_Declare(session_cpp
        GIT_REPOSITORY https://github.com/petrasvestartas/session_cpp.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
    )
    FetchContent_GetProperties(session_cpp)
    if(NOT session_cpp_POPULATED)
        FetchContent_Populate(session_cpp)
    endif()
    set(SESSION_CPP_DIR "${session_cpp_SOURCE_DIR}")
    message(STATUS "wood: fetched session_cpp to ${SESSION_CPP_DIR}")
endif()

# No generated/ mirror here any more.
#
# session_cpp used to locate its own protobuf sources through CMAKE_SOURCE_DIR, which
# under add_subdirectory() is THIS project rather than session_cpp - so every consumer
# had to copy session_cpp/generated into its own root just to be found. session_cpp now
# anchors those paths to its own directory (SESSION_CPP_ROOT), so the copy is obsolete.
#
# Deleting it also removes the failure it caused. file(COPY) preserves timestamps, so a
# regenerated element.pb.h could land here OLDER than the objects already compiled
# against the previous one; make skipped the rebuild and linked two different layouts of
# session_proto::Element together. That surfaced at runtime as
#   malloc(): invalid size (unsorted)   in Element::pb_dumps -> ArenaStringPtr::Set
# which reads as a serialization bug in whatever you last touched. There is now one copy
# of the generated sources, in session_cpp, and nothing to go stale.

set(ENABLE_UNITY_BUILD OFF CACHE BOOL "Disable session_cpp unity build" FORCE)
add_subdirectory(${SESSION_CPP_DIR} session_cpp EXCLUDE_FROM_ALL)

set(WOOD_SOURCES
    src/joinery_solver/wood_config.cpp
    src/joinery_solver/wood_settings.cpp
    src/joinery_solver/wood_serialization.cpp
    src/joinery_solver/wood_io.cpp
    src/joinery_solver/wood_view.cpp
    src/joinery_solver/wood_elements/wood_element.cpp
    src/joinery_solver/wood_elements/wood_element_cut_plane.cpp
    src/joinery_solver/wood_elements/wood_element_plate.cpp
    src/joinery_solver/wood_elements/wood_element_plate_curved.cpp
    src/joinery_solver/wood_elements/wood_element_column.cpp
    src/joinery_solver/wood_elements/wood_element_beam_variable.cpp
    src/joinery_solver/wood_elements/wood_element_beam_curved.cpp
    src/joinery_solver/wood_elements/wood_element_support.cpp
    src/joinery_solver/wood_elements/wood_element_beam.cpp
    src/joinery_solver/wood_elements/wood_element_block.cpp
    src/joinery_solver/wood_elements/wood_element_joint.cpp
    src/joinery_solver/wood_elements/wood_element_joint_plate.cpp
    src/joinery_solver/wood_elements/wood_element_joint_beam.cpp
    src/joinery_solver/wood_elements/wood_element_pin.cpp
    src/joinery_solver/wood_elements/wood_element_connector_part.cpp
    src/joinery_solver/wood_elements/wood_element_geometry.cpp
    src/joinery_solver/wood_algorithms/wood_solid_boolean.cpp
    src/joinery_solver/wood_algorithms/wood_brep_drill.cpp
    src/joinery_solver/wood_algorithms/wood_profile_boolean.cpp
    src/joinery_solver/wood_elements/wood_profile.cpp
    src/templates/grid/grid_plan.cpp
    src/templates/grid/grid_levels.cpp
    src/templates/grid/grid_joints.cpp
    src/templates/grid/grid.cpp
    src/templates/vault/vault.cpp
    src/templates/floor/floor_guide.cpp
    src/templates/floor/floor.cpp
    src/templates/truss/truss_guide.cpp
    src/templates/truss/truss.cpp
    src/templates/module/framed_panel.cpp
    src/templates/module/module_contact.cpp
    src/templates/module/wall.cpp
    src/templates/module/room_module.cpp
    src/templates/module/cassette_shell.cpp
    src/joinery_solver/wood_interaction/wood_interaction_contact/wood_interaction_contact_face.cpp
    src/joinery_solver/wood_interaction/wood_interaction_contact/wood_interaction_contact_axis.cpp
    src/joinery_solver/wood_interaction/wood_interaction_contact/wood_interaction_contact_cross.cpp
    src/joinery_solver/wood_interaction/wood_interaction_feature/wood_interaction_feature_plate.cpp
    src/joinery_solver/wood_interaction/wood_interaction_feature/wood_interaction_feature_beam.cpp
    src/joinery_solver/wood_interaction/wood_interaction_feature/wood_interaction_feature_plate_beam.cpp
    src/joinery_solver/wood_interaction/wood_interaction_feature/wood_interaction_feature_solid.cpp
    src/joinery_solver/wood_interaction/wood_interaction_feature/wood_interaction_feature_plane.cpp
    src/joinery_solver/wood_algorithms/wood_contact_detection.cpp
    src/joinery_solver/wood_algorithms/wood_feature_detection.cpp
    src/joinery_solver/wood_algorithms/wood_feature_detection_beam.cpp
    src/joinery_solver/wood_algorithms/wood_feature_construction.cpp
    src/joinery_solver/wood_algorithms/wood_feature_solver.cpp
    src/joinery_solver/wood_algorithms/wood_assignment.cpp
    src/joinery_solver/wood_algorithms/wood_merge_modifier.cpp
    src/joinery_solver/wood_session.cpp
    src/joinery_solver/wood_instance.cpp
    src/joinery_solver/wood_algorithms/wood_three_valence.cpp
)

# The wood protobuf messages (src/proto/*.proto), generated into generated/ by tools/regen_proto.sh
# and committed, as session_cpp does with its own; the wood_regen_proto target below runs the same
# command when the kernel's pinned protoc is present.
file(GLOB WOOD_PROTO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/generated/*.pb.cc)
if(NOT MSVC)
    set_source_files_properties(${WOOD_PROTO_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")
endif()

# wood_core: the joinery solver, compiled ONCE and shared by every target.
#
# This used to be a bare ${WOOD_SOURCES} file list splatted into each
# add_executable, so the 11 sources were rebuilt for each of the 11 wood
# targets - 121 compilations of the same translation units, measured at ~5.8 s
# and ~5.8 MB of objects per target. That cost is also the only reason there
# were two exe macros: ADD_SESSION_EXE existed purely to avoid paying it on the
# 10 targets that never call wood. session_cpp already does the right thing
# (add_library(session_core OBJECT ...) at its CMakeLists:234); this mirrors it,
# so one macro now serves everything.
# Clipper2 does the 2D polygon boolean in face_overlap_area (wood_face_to_face.cpp).
# It ships inside session_cpp but session_core does not compile it, so the three
# translation units it needs are built here. Any consumer that compiles
# wood_face_to_face.cpp itself (wood_nano's _joinery_solver) has to add them too.
set(WOOD_CLIPPER2_SOURCES
    ${SESSION_CPP_DIR}/src/clipper2/clipper.engine.cpp
    ${SESSION_CPP_DIR}/src/clipper2/clipper.offset.cpp
    ${SESSION_CPP_DIR}/src/clipper2/clipper.rectclip.cpp
)

add_library(wood_core OBJECT ${WOOD_SOURCES} ${WOOD_CLIPPER2_SOURCES} ${WOOD_PROTO_SOURCES})
# The std and kernel headers every wood file needs, compiled once and reused by every target.
target_precompile_headers(wood_core PRIVATE src/joinery_solver/pch.h)
# One warning set for the library and every target, so the precompiled header matches everywhere.
if(NOT MSVC)
    target_compile_options(wood_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_include_directories(wood_core PUBLIC
    ${SESSION_CPP_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/joinery_solver
    ${CMAKE_CURRENT_SOURCE_DIR}/src/joinery_solver/wood_elements
    ${CMAKE_CURRENT_SOURCE_DIR}/src/joinery_solver/wood_interaction
    ${CMAKE_CURRENT_SOURCE_DIR}/src/joinery_solver/wood_interaction/wood_interaction_contact
    ${CMAKE_CURRENT_SOURCE_DIR}/src/joinery_solver/wood_interaction/wood_interaction_feature
    ${CMAKE_CURRENT_SOURCE_DIR}/src/joinery_solver/wood_interaction/wood_interaction_structure
    ${CMAKE_CURRENT_SOURCE_DIR}/src/joinery_solver/wood_algorithms
    ${CMAKE_CURRENT_SOURCE_DIR}/generated
)
# session_core's usage requirements: src, fmt, json, guid, yaml, and the protobuf gencode,
# protobuf and abseil headers as SYSTEM. Linking keeps that flag; spelling the include list out
# through $<TARGET_PROPERTY:session_core,INTERFACE_INCLUDE_DIRECTORIES> turned abseil into a plain
# -I and flooded every wood file with its -Wpedantic warnings. An object library linking an object
# library takes only the usage requirements; ADD_EXE still links session_core's objects itself.
target_link_libraries(wood_core PUBLIC session_core)

# Manifold, the mesh boolean behind solid_boolean and apply_solid_cuts: guaranteed closed results
# where wood's BSP split had opened up on chained cuts (the floor column head). Pinned, static, no
# tests, bindings, 2D cross sections or parallel backend, so it pulls in no further dependency.
FetchContent_Declare(manifold
    GIT_REPOSITORY https://github.com/elalish/manifold.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
)
set(MANIFOLD_TEST OFF)
set(MANIFOLD_CROSS_SECTION OFF)
set(MANIFOLD_CBIND OFF)
set(MANIFOLD_PYBIND OFF)
set(MANIFOLD_JSBIND OFF)
set(MANIFOLD_PAR OFF)
set(MANIFOLD_EXPORT OFF)
set(MANIFOLD_DOWNLOADS OFF)
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(manifold)
if(EXISTS "${manifold_SOURCE_DIR}/src/impl.cpp")
    file(READ "${manifold_SOURCE_DIR}/src/impl.cpp" _manifold_impl)
    string(REPLACE
        "if (std::abs(dot(v - base, normal)) < tolerance_) {"
        "if (std::abs(dot(v - base, normal)) < tolerance_ && dot(faceNormal_[h / 3], normal) > 0) {"
        _manifold_impl_patched "${_manifold_impl}")
    if(NOT "${_manifold_impl}" STREQUAL "${_manifold_impl_patched}")
        file(WRITE "${manifold_SOURCE_DIR}/src/impl.cpp" "${_manifold_impl_patched}")
    endif()
endif()
if(EXISTS "${manifold_SOURCE_DIR}/src/edge_op.cpp")
    file(READ "${manifold_SOURCE_DIR}/src/edge_op.cpp" _manifold_edge_op)
    string(REPLACE
        "if (ref.meshID != oldRef.meshID || ref.faceID != oldRef.faceID ||\n            la::dot(faceNormal_[pair / 3], faceNormal_[tri]) < -0.5) {"
        "if (la::dot(faceNormal_[pair / 3], faceNormal_[tri]) > -1.0 + 1e-6 &&\n            (ref.meshID != oldRef.meshID || ref.faceID != oldRef.faceID ||\n             la::dot(faceNormal_[pair / 3], faceNormal_[tri]) < -0.5)) {"
        _manifold_edge_op_patched "${_manifold_edge_op}")
    if(NOT "${_manifold_edge_op}" STREQUAL "${_manifold_edge_op_patched}")
        file(WRITE "${manifold_SOURCE_DIR}/src/edge_op.cpp" "${_manifold_edge_op_patched}")
    endif()
endif()
target_link_libraries(wood_core PUBLIC manifold::manifold)

find_program(WOOD_PROTOC protoc HINTS ${SESSION_CPP_DIR}/.protoc/36.2/bin NO_DEFAULT_PATH)
if(WOOD_PROTOC)
    file(GLOB WOOD_PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/*.proto)
    add_custom_target(wood_regen_proto
        COMMAND ${WOOD_PROTOC} --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/generated
                --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/src/proto
                --proto_path=${SESSION_CPP_DIR}/../session_proto
                ${WOOD_PROTO_FILES}
        DEPENDS ${WOOD_PROTO_FILES}
        COMMENT "Regenerating wood/generated from src/proto"
        VERBATIM
    )
endif()

# Every executable in this project: wood solver + session kernel + protobuf.
# Extra sources (e.g. the dataset runners) go in ${ARGN} and are compiled per target.
function(ADD_EXE NAME SRC)
    add_executable(${NAME} ${SRC} ${ARGN})
    target_link_libraries(${NAME} PRIVATE wood_core session_core)
    if(NOT MSVC)
        target_compile_options(${NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    target_precompile_headers(${NAME} REUSE_FROM wood_core)
    LINK_PROTOBUF(${NAME})
endfunction()

ADD_EXE(1_session_contacts               examples/1_session_contacts.cpp)
ADD_EXE(2_session_hard_coded_contacts    examples/2_session_hard_coded_contacts.cpp)
ADD_EXE(3_elements_tree                  examples/3_elements_tree.cpp)
ADD_EXE(4_datasets                       examples/4_datasets.cpp)
ADD_EXE(5_contacts                       examples/5_contacts.cpp)
ADD_EXE(6_features                       examples/6_features.cpp)
ADD_EXE(7_traversal                      examples/7_traversal.cpp)
ADD_EXE(8_settings                       examples/8_settings.cpp)
ADD_EXE(9_custom_joint                   examples/9_custom_joint.cpp)
ADD_EXE(10_assignment                     examples/10_assignment.cpp)
ADD_EXE(11_beams                          examples/11_beams.cpp)
ADD_EXE(12_serialization                 examples/12_serialization.cpp)
ADD_EXE(13_viewer                        examples/13_viewer.cpp)
ADD_EXE(14_cross_joints                  examples/14_cross_joints.cpp)
ADD_EXE(15_profile_cuts                  examples/15_profile_cuts.cpp)
ADD_EXE(16_drill_solids                  examples/16_drill_solids.cpp)
ADD_EXE(17_solid_features                examples/17_solid_features.cpp)
ADD_EXE(18_cutting_gallery               examples/18_cutting_gallery.cpp)
ADD_EXE(19_plate_joint_library           examples/19_plate_joint_library.cpp)
ADD_EXE(20_butt_frame                     examples/20_butt_frame.cpp)
ADD_EXE(21_butt_house                     examples/21_butt_house.cpp)
ADD_EXE(22_butt_pairs                     examples/22_butt_pairs.cpp)
ADD_EXE(23_butt_assemblies                examples/23_butt_assemblies.cpp)
ADD_EXE(main_all_datasets                examples/main_all_datasets.cpp src/joinery_solver/wood_datasets.cpp)
ADD_EXE(main_dataset_runner              examples/main_dataset_runner.cpp src/joinery_solver/wood_datasets.cpp)
# main_cross_polylines is gone: it was built on session_cpp's ElementFeature /
# CrossElementFeature subsystem, deleted upstream in 57a9b3f7 as "dead
# ElementFeature/Joint subsystem (only-self-referential, real joinery bypasses
# it)". There is no replacement to port it to - Element::add_feature is a mesh
# transform, not a pairwise relation - and it was the only caller left.
ADD_EXE(main_session_round_trip          examples/main_session_round_trip.cpp)
ADD_EXE(joint_elements                   tests/joint_elements.cpp)
ADD_EXE(interaction_ownership            tests/interaction_ownership.cpp)
ADD_EXE(floor_elements                   tests/floor_elements.cpp)
ADD_EXE(brep_drill                       tests/brep_drill.cpp)
ADD_EXE(joint_hilti                      tests/joint_hilti.cpp)
ADD_EXE(joint_pins                       tests/joint_pins.cpp)
ADD_EXE(joint_beams                      tests/joint_beams.cpp)
ADD_EXE(joint_border                     tests/joint_border.cpp)
ADD_EXE(joint_butt                       tests/joint_butt.cpp)
ADD_EXE(plate_assignment                 tests/plate_assignment.cpp)
ADD_EXE(rust_elements                    tests/rust_elements.cpp)
target_compile_definitions(rust_elements PRIVATE WOOD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
# The geometry oracle: every plate joint design measured on a joined pair, goldens under tests/golden/joint_library.
ADD_EXE(joint_library                    tests/joint_library.cpp)
target_compile_definitions(joint_library PRIVATE WOOD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
# The joint units on the JointPlate family pages: every oracle design written as what each plate keeps inside the joint's box (tools/joint_library_figures/make.sh).
ADD_EXE(joint_tiles                      tools/joint_library_figures/joint_tiles.cpp)
target_compile_definitions(joint_tiles PRIVATE WOOD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
# The scoreboard: every dataset against the 2025 solver's merged outlines in tests/golden/reference_2025 (tools/reference_2025.py).
ADD_EXE(reference_datasets               tests/reference_datasets.cpp)
target_compile_definitions(reference_datasets PRIVATE WOOD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
ADD_EXE(templates_shells_1_translation_shell      examples/templates_shells_1_translation_shell.cpp)
ADD_EXE(templates_folding_1_reflex_fold            examples/templates_folding_1_reflex_fold.cpp)
ADD_EXE(templates_chevron_1_chevron                examples/templates_chevron_1_chevron.cpp)
ADD_EXE(templates_reciprocal_1_rotation    examples/templates_reciprocal_1_rotation.cpp)
ADD_EXE(templates_reciprocal_2_move        examples/templates_reciprocal_2_move.cpp)
ADD_EXE(templates_folding_2_diamond_mesh           examples/templates_folding_2_diamond_mesh.cpp)
ADD_EXE(templates_folding_3_prismatic_fold         examples/templates_folding_3_prismatic_fold.cpp)
ADD_EXE(templates_folding_4_folded_barrel          examples/templates_folding_4_folded_barrel.cpp)
ADD_EXE(templates_folding_5_counter_fold           examples/templates_folding_5_counter_fold.cpp)
ADD_EXE(templates_folding_6_elevated_fold          examples/templates_folding_6_elevated_fold.cpp)
ADD_EXE(templates_folding_7_conical_fold           examples/templates_folding_7_conical_fold.cpp)
ADD_EXE(templates_folding_8_two_hinged_frame       examples/templates_folding_8_two_hinged_frame.cpp)
ADD_EXE(templates_folding_9_three_hinged_frame     examples/templates_folding_9_three_hinged_frame.cpp)
ADD_EXE(templates_folding_10_three_hinged_arch     examples/templates_folding_10_three_hinged_arch.cpp)
ADD_EXE(templates_folding_11_triangle_fold         examples/templates_folding_11_triangle_fold.cpp)
ADD_EXE(templates_folding_12_square_fold           examples/templates_folding_12_square_fold.cpp)
ADD_EXE(templates_folding_13_hexagon_fold          examples/templates_folding_13_hexagon_fold.cpp)
ADD_EXE(templates_folding_14_pyramid               examples/templates_folding_14_pyramid.cpp)
ADD_EXE(templates_folding_15_truncated_pyramid     examples/templates_folding_15_truncated_pyramid.cpp)
ADD_EXE(templates_folding_16_cross_fold            examples/templates_folding_16_cross_fold.cpp)
ADD_EXE(templates_folding_17_cross_fold_rising     examples/templates_folding_17_cross_fold_rising.cpp)
ADD_EXE(templates_folding_18_cross_fold_sloping    examples/templates_folding_18_cross_fold_sloping.cpp)
ADD_EXE(templates_folding_19_fold_composition      examples/templates_folding_19_fold_composition.cpp)
ADD_EXE(templates_folding_20_triangle_square       examples/templates_folding_20_triangle_square.cpp)
ADD_EXE(templates_folding_21_triangle_triangle     examples/templates_folding_21_triangle_triangle.cpp)
ADD_EXE(templates_folding_22_triangle_pentagon     examples/templates_folding_22_triangle_pentagon.cpp)
ADD_EXE(templates_folding_23_triangle_hexagon      examples/templates_folding_23_triangle_hexagon.cpp)
ADD_EXE(templates_folding_25_one_fold              examples/templates_folding_25_one_fold.cpp)
ADD_EXE(templates_folding_26_three_folds           examples/templates_folding_26_three_folds.cpp)
ADD_EXE(templates_folding_27_counter_centre        examples/templates_folding_27_counter_centre.cpp)
ADD_EXE(templates_folding_28_ridge_to_valley       examples/templates_folding_28_ridge_to_valley.cpp)
ADD_EXE(templates_folding_29_elevated_ridge_to_valley examples/templates_folding_29_elevated_ridge_to_valley.cpp)
ADD_EXE(templates_folding_30_alternate_fold        examples/templates_folding_30_alternate_fold.cpp)
ADD_EXE(templates_folding_31_deep_centre           examples/templates_folding_31_deep_centre.cpp)
ADD_EXE(templates_folding_32_deep_centre_valley    examples/templates_folding_32_deep_centre_valley.cpp)
ADD_EXE(templates_folding_34_a_frame               examples/templates_folding_34_a_frame.cpp)
ADD_EXE(templates_folding_35_pointed_arch          examples/templates_folding_35_pointed_arch.cpp)
ADD_EXE(templates_folding_36_folded_truncated_pyramid examples/templates_folding_36_folded_truncated_pyramid.cpp)
ADD_EXE(templates_folding_38_truncated_pyramid_corner_folds examples/templates_folding_38_truncated_pyramid_corner_folds.cpp)
ADD_EXE(templates_folding_39_truncated_pyramid_corner_edge_folds examples/templates_folding_39_truncated_pyramid_corner_edge_folds.cpp)
ADD_EXE(templates_cylindrical_folding_1_reflection                     examples/templates_cylindrical_folding_1_reflection.cpp)
ADD_EXE(templates_cylindrical_folding_2_curved_folded_plate            examples/templates_cylindrical_folding_2_curved_folded_plate.cpp)
ADD_EXE(templates_cylindrical_folding_3_mendrisio_pavilion             examples/templates_cylindrical_folding_3_mendrisio_pavilion.cpp)
ADD_EXE(templates_cylindrical_folding_4_two_hinged_frame               examples/templates_cylindrical_folding_4_two_hinged_frame.cpp)
ADD_EXE(templates_cylindrical_folding_5_two_hinged_arch                examples/templates_cylindrical_folding_5_two_hinged_arch.cpp)
ADD_EXE(templates_cylindrical_folding_6_arch_with_top_hinge            examples/templates_cylindrical_folding_6_arch_with_top_hinge.cpp)
ADD_EXE(templates_cylindrical_folding_7_a_frame                        examples/templates_cylindrical_folding_7_a_frame.cpp)
ADD_EXE(templates_cylindrical_folding_8_groin_vault                    examples/templates_cylindrical_folding_8_groin_vault.cpp)
ADD_EXE(templates_cylindrical_folding_9_pavilion_vault                 examples/templates_cylindrical_folding_9_pavilion_vault.cpp)
ADD_EXE(templates_cylindrical_folding_10_pentagon_vault                examples/templates_cylindrical_folding_10_pentagon_vault.cpp)
ADD_EXE(templates_cylindrical_folding_11_hexagon_vault                 examples/templates_cylindrical_folding_11_hexagon_vault.cpp)
ADD_EXE(templates_cylindrical_folding_12_octagon_vault                 examples/templates_cylindrical_folding_12_octagon_vault.cpp)
ADD_EXE(templates_cylindrical_folding_13_curved_folded_beam            examples/templates_cylindrical_folding_13_curved_folded_beam.cpp)
ADD_EXE(templates_cylindrical_folding_14_corner_fan                    examples/templates_cylindrical_folding_14_corner_fan.cpp)
ADD_EXE(templates_cylindrical_folding_15_corner_fan_subdivisions       examples/templates_cylindrical_folding_15_corner_fan_subdivisions.cpp)
ADD_EXE(templates_cylindrical_folding_16_cylinder_carving              examples/templates_cylindrical_folding_16_cylinder_carving.cpp)
ADD_EXE(templates_cylindrical_folding_17_cylinder_carving_rings        examples/templates_cylindrical_folding_17_cylinder_carving_rings.cpp)
ADD_EXE(templates_developable_folding_1_curved_fold                    examples/templates_developable_folding_1_curved_fold.cpp)
ADD_EXE(templates_developable_folding_2_curved_crease_unfolding        examples/templates_developable_folding_2_curved_crease_unfolding.cpp)
ADD_EXE(templates_developable_folding_3_catenary_unfolding             examples/templates_developable_folding_3_catenary_unfolding.cpp)
ADD_EXE(templates_developable_folding_4_crease_quadrants               examples/templates_developable_folding_4_crease_quadrants.cpp)
ADD_EXE(templates_developable_folding_5_synclastic_unfolding           examples/templates_developable_folding_5_synclastic_unfolding.cpp)
ADD_EXE(templates_developable_folding_6_anticlastic_unfolding          examples/templates_developable_folding_6_anticlastic_unfolding.cpp)
ADD_EXE(templates_developable_folding_7_apex_level_unfolding           examples/templates_developable_folding_7_apex_level_unfolding.cpp)
ADD_EXE(templates_developable_folding_8_support_level_unfolding        examples/templates_developable_folding_8_support_level_unfolding.cpp)
ADD_EXE(templates_developable_folding_9_alternating_unfolding          examples/templates_developable_folding_9_alternating_unfolding.cpp)
ADD_EXE(templates_developable_folding_10_sideways_unfolding            examples/templates_developable_folding_10_sideways_unfolding.cpp)
ADD_EXE(templates_developable_folding_11_corrugated_dome               examples/templates_developable_folding_11_corrugated_dome.cpp)
ADD_EXE(templates_developable_folding_12_corrugated_funnel             examples/templates_developable_folding_12_corrugated_funnel.cpp)
ADD_EXE(templates_cross_1_vda_mesh               examples/templates_cross_1_vda_mesh.cpp)
ADD_EXE(templates_module_1_wall                  examples/templates_module_1_wall.cpp)
ADD_EXE(templates_module_2_cassette_shell         examples/templates_module_2_cassette_shell.cpp)
ADD_EXE(templates_module_3_cassette_house         examples/templates_module_3_cassette_house.cpp)
ADD_EXE(templates_module_4_room_module            examples/templates_module_4_room_module.cpp)
ADD_EXE(templates_module_5_module_block           examples/templates_module_5_module_block.cpp)
ADD_EXE(templates_module_6_bank_pavilion          examples/templates_module_6_bank_pavilion.cpp)
ADD_EXE(templates_grid_1_footprint         examples/templates_grid_1_footprint.cpp)
ADD_EXE(templates_grid_2_solid             examples/templates_grid_2_solid.cpp)
ADD_EXE(templates_grid_3_lines             examples/templates_grid_3_lines.cpp)
ADD_EXE(templates_grid_4_reference         examples/templates_grid_4_reference.cpp)
ADD_EXE(templates_grid_5_framing           examples/templates_grid_5_framing.cpp)
ADD_EXE(templates_truss_1_pratt               examples/templates_truss_1_pratt.cpp)
ADD_EXE(templates_truss_2_library             examples/templates_truss_2_library.cpp)
ADD_EXE(element_plate                    examples/elements/element_plate.cpp)
ADD_EXE(element_plate_holes              examples/elements/element_plate_holes.cpp)
ADD_EXE(element_plate_session            examples/elements/element_plate_session.cpp)
ADD_EXE(element_plate_assign_joints      examples/elements/element_plate_assign_joints.cpp)
ADD_EXE(element_plate_curved             examples/elements/element_plate_curved.cpp)
ADD_EXE(element_beam                     examples/elements/element_beam.cpp)
ADD_EXE(element_beam_curved              examples/elements/element_beam_curved.cpp)
ADD_EXE(element_beam_variable            examples/elements/element_beam_variable.cpp)
ADD_EXE(element_beam_variable_cut        examples/elements/element_beam_variable_cut.cpp)
ADD_EXE(element_column                   examples/elements/element_column.cpp)
ADD_EXE(element_column_session           examples/elements/element_column_session.cpp)
ADD_EXE(element_block                    examples/elements/element_block.cpp)
ADD_EXE(element_support                  examples/elements/element_support.cpp)
ADD_EXE(element_joint_plate_top_side_three_tenons examples/elements/element_joint_plate_top_side_three_tenons.cpp)
ADD_EXE(element_joint_plate_top_side_annen_tenons examples/elements/element_joint_plate_top_side_annen_tenons.cpp)
ADD_EXE(element_joint_plate_top_side_dense_tenons examples/elements/element_joint_plate_top_side_dense_tenons.cpp)
ADD_EXE(element_joint_plate_top_side_tenons examples/elements/element_joint_plate_top_side_tenons.cpp)
ADD_EXE(element_joint_plate_top_side_snap_fit examples/elements/element_joint_plate_top_side_snap_fit.cpp)
ADD_EXE(element_joint_plate_vidy_node examples/elements/element_joint_plate_vidy_node.cpp)
ADD_EXE(element_joint_plate_top_side_custom examples/elements/element_joint_plate_top_side_custom.cpp)
ADD_EXE(element_joint_plate_side_side_inplane_three_fingers examples/elements/element_joint_plate_side_side_inplane_three_fingers.cpp)
ADD_EXE(element_joint_plate_side_side_inplane_dovetails examples/elements/element_joint_plate_side_side_inplane_dovetails.cpp)
ADD_EXE(element_joint_plate_side_side_inplane_butterfly_keys examples/elements/element_joint_plate_side_side_inplane_butterfly_keys.cpp)
ADD_EXE(element_joint_plate_side_side_inplane_tooth_keys examples/elements/element_joint_plate_side_side_inplane_tooth_keys.cpp)
ADD_EXE(element_joint_plate_side_side_inplane_custom examples/elements/element_joint_plate_side_side_inplane_custom.cpp)
ADD_EXE(element_joint_plate_side_side_outplane_three_fingers examples/elements/element_joint_plate_side_side_outplane_three_fingers.cpp)
ADD_EXE(element_joint_plate_side_side_outplane_fingers examples/elements/element_joint_plate_side_side_outplane_fingers.cpp)
ADD_EXE(element_joint_plate_side_side_outplane_graded_fingers examples/elements/element_joint_plate_side_side_outplane_graded_fingers.cpp)
ADD_EXE(element_joint_plate_side_side_outplane_miter_tenon examples/elements/element_joint_plate_side_side_outplane_miter_tenon.cpp)
ADD_EXE(element_joint_plate_side_side_outplane_tenons examples/elements/element_joint_plate_side_side_outplane_tenons.cpp)
ADD_EXE(element_joint_plate_side_side_outplane_linked_tenons examples/elements/element_joint_plate_side_side_outplane_linked_tenons.cpp)
ADD_EXE(element_joint_plate_side_side_outplane_linked_tenons_wall examples/elements/element_joint_plate_side_side_outplane_linked_tenons_wall.cpp)
ADD_EXE(element_joint_plate_side_side_outplane_capped_fingers examples/elements/element_joint_plate_side_side_outplane_capped_fingers.cpp)
ADD_EXE(element_joint_plate_side_side_outplane_notch examples/elements/element_joint_plate_side_side_outplane_notch.cpp)
ADD_EXE(element_joint_plate_side_side_outplane_custom examples/elements/element_joint_plate_side_side_outplane_custom.cpp)
ADD_EXE(element_joint_plate_cross_slot examples/elements/element_joint_plate_cross_slot.cpp)
ADD_EXE(element_joint_plate_cross_custom examples/elements/element_joint_plate_cross_custom.cpp)
ADD_EXE(element_joint_plate_top_top_centre_pin examples/elements/element_joint_plate_top_top_centre_pin.cpp)
ADD_EXE(element_joint_plate_top_top_polylabel_pin examples/elements/element_joint_plate_top_top_polylabel_pin.cpp)
ADD_EXE(element_joint_plate_top_top_circle_pins examples/elements/element_joint_plate_top_top_circle_pins.cpp)
ADD_EXE(element_joint_plate_top_top_ring_pins examples/elements/element_joint_plate_top_top_ring_pins.cpp)
ADD_EXE(element_joint_plate_top_top_grid_pins examples/elements/element_joint_plate_top_top_grid_pins.cpp)
ADD_EXE(element_joint_plate_top_top_rectangle_pins examples/elements/element_joint_plate_top_top_rectangle_pins.cpp)
ADD_EXE(element_joint_plate_top_top_custom examples/elements/element_joint_plate_top_top_custom.cpp)
ADD_EXE(element_joint_plate_border examples/elements/element_joint_plate_border.cpp)
ADD_EXE(element_joint_plate_parameters   examples/elements/element_joint_plate_parameters.cpp)
ADD_EXE(element_joint_plate_side_side_rotated_miter examples/elements/element_joint_plate_side_side_rotated_miter.cpp)
ADD_EXE(element_joint_plate_side_side_rotated_hooks examples/elements/element_joint_plate_side_side_rotated_hooks.cpp)
ADD_EXE(element_joint_plate_side_side_rotated_keys examples/elements/element_joint_plate_side_side_rotated_keys.cpp)
ADD_EXE(element_joint_plate_side_side_rotated_custom examples/elements/element_joint_plate_side_side_rotated_custom.cpp)
ADD_EXE(element_joint_plate_side_side_rotated_side_removal examples/elements/element_joint_plate_side_side_rotated_side_removal.cpp)
ADD_EXE(element_joint_drill                  examples/elements/element_joint_drill.cpp)
ADD_EXE(element_joint_cutter                 examples/elements/element_joint_cutter.cpp)
ADD_EXE(element_joint_beam_cross_slot examples/elements/element_joint_beam_cross_slot.cpp)
ADD_EXE(element_joint_beam_cross_sliced_half_lap examples/elements/element_joint_beam_cross_sliced_half_lap.cpp)
ADD_EXE(element_joint_beam_cross_half_lap examples/elements/element_joint_beam_cross_half_lap.cpp)
ADD_EXE(element_joint_beam_cross_half_lap_diagonal_drills examples/elements/element_joint_beam_cross_half_lap_diagonal_drills.cpp)
ADD_EXE(element_joint_beam_cross_half_lap_drill examples/elements/element_joint_beam_cross_half_lap_drill.cpp)
ADD_EXE(element_joint_beam_cross_brussels_half_lap examples/elements/element_joint_beam_cross_brussels_half_lap.cpp)
ADD_EXE(element_joint_beam_top_side_milled_wedge examples/elements/element_joint_beam_top_side_milled_wedge.cpp)
ADD_EXE(element_joint_beam_side_side_inplane_splayed_scarf examples/elements/element_joint_beam_side_side_inplane_splayed_scarf.cpp)
ADD_EXE(element_joint_beam_jupiter_scarf examples/elements/element_joint_beam_jupiter_scarf.cpp)
ADD_EXE(element_joint_beam_side_side_inplane_cross_splayed_scarf examples/elements/element_joint_beam_side_side_inplane_cross_splayed_scarf.cpp)
ADD_EXE(element_joint_beam_side_side_rotated_miter examples/elements/element_joint_beam_side_side_rotated_miter.cpp)
ADD_EXE(element_joint_beam_wedge             examples/elements/element_joint_beam_wedge.cpp)
ADD_EXE(element_joint_beam_butt              examples/elements/element_joint_beam_butt.cpp)
ADD_EXE(element_joint_beam_butt_plates       examples/elements/element_joint_beam_butt_plates.cpp)
ADD_EXE(element_joint_plate_side_side_inplane_hilti examples/elements/element_joint_plate_side_side_inplane_hilti.cpp)
ADD_EXE(element_joint_plate_side_side_inplane_hilti_angles examples/elements/element_joint_plate_side_side_inplane_hilti_angles.cpp)
ADD_EXE(element_joint_beam_rectangle_plate   examples/elements/element_joint_beam_rectangle_plate.cpp)
ADD_EXE(element_joint_beam_butterfly_key     examples/elements/element_joint_beam_butterfly_key.cpp)
ADD_EXE(element_joint_beam_centred_pins      examples/elements/element_joint_beam_centred_pins.cpp)
ADD_EXE(element_joint_beam_headed_pins       examples/elements/element_joint_beam_headed_pins.cpp)
ADD_EXE(element_pin                          examples/elements/element_pin.cpp)
ADD_EXE(element_profile                      examples/elements/element_profile.cpp)
ADD_EXE(templates_floor_1_floorguide     examples/templates_floor_1_floorguide.cpp)
ADD_EXE(templates_floor_2_column_model   examples/templates_floor_2_column_model.cpp)
ADD_EXE(templates_floor_3_contacts       examples/templates_floor_3_contacts.cpp)
ADD_EXE(templates_floor_7_contacts_cantilevers examples/templates_floor_7_contacts_cantilevers.cpp)
ADD_EXE(templates_floor_8_rectangle      examples/templates_floor_8_rectangle.cpp)
ADD_EXE(templates_floor_9_orientation    examples/templates_floor_9_orientation.cpp)


ADD_EXE(templates_vault_1_arch           examples/templates_vault_1_arch.cpp)
ADD_EXE(templates_vault_2_barrel         examples/templates_vault_2_barrel.cpp)
ADD_EXE(templates_vault_3_barrel_staggered examples/templates_vault_3_barrel_staggered.cpp)
ADD_EXE(templates_vault_4_dome           examples/templates_vault_4_dome.cpp)
ADD_EXE(templates_vault_5_wall           examples/templates_vault_5_wall.cpp)
ADD_EXE(templates_vault_6_cross_vault    examples/templates_vault_6_cross_vault.cpp)
ADD_EXE(templates_vault_7_cloister_vault examples/templates_vault_7_cloister_vault.cpp)
ADD_EXE(templates_vault_8_star_vault     examples/templates_vault_8_star_vault.cpp)
ADD_EXE(templates_vault_9_gothic_ribbed_vault examples/templates_vault_9_gothic_ribbed_vault.cpp)

# Documentation: `cmake --build build --target docs` writes build/docs/html from the headers'
# docstrings, src/docs.md as the main page (its Mermaid fences rendered by the footer) and
# docs/examples.md, docs/elements.md with a page per element under docs/elements, docs/templates.md and a page per
# template family under docs/templates (their pictures under docs/images, the floor's beside its page in docs/templates/floor); Doxygen only.
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME "Wood")
    set(DOXYGEN_PROJECT_BRIEF "Timber joinery on the session kernel")
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/src/docs.md)
    set(DOXYGEN_FILE_PATTERNS *.h *.md)
    set(DOXYGEN_FILTER_PATTERNS "*.md=${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/mermaid_filter.sh")
    set(DOXYGEN_EXAMPLE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    set(DOXYGEN_IMAGE_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/images
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/images/diagrams
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/images/examples
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/templates
    )
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_MARKDOWN_SUPPORT YES)
    set(DOXYGEN_TOC_INCLUDE_HEADINGS 3)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_TREEVIEW_WIDTH 280)
    set(DOXYGEN_DISABLE_INDEX YES)
    set(DOXYGEN_FULL_SIDEBAR NO)
    set(DOXYGEN_PROJECT_LOGO ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/compas_icon_white.png)
    set(DOXYGEN_HTML_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/header.html)
    set(DOXYGEN_HTML_FOOTER ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/footer.html)
    # doxygen-awesome-css v2.3.4 (MIT, jothepro/doxygen-awesome-css), committed under docs/doxygen/;
    # material.css on top of it gives the MkDocs Material appearance.
    set(DOXYGEN_HTML_EXTRA_STYLESHEET
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/doxygen-awesome.css
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/material.css)
    file(GLOB_RECURSE WOOD_DOC_EXTRA_IMAGES
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/images/*.webp
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/images/*.png
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/images/*.svg
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/templates/*.webp
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/templates/*.png
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/templates/*.svg
    )
    set(DOXYGEN_HTML_EXTRA_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/doxygen-awesome-darkmode-toggle.js
        ${WOOD_DOC_EXTRA_IMAGES}
    )
    set(DOXYGEN_HTML_COLORSTYLE LIGHT)
    set(DOXYGEN_HTML_DYNAMIC_SECTIONS NO)
    set(DOXYGEN_SHOW_FILES YES)
    set(DOXYGEN_SOURCE_BROWSER NO)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
    set(DOXYGEN_QUIET YES)
    set(DOXYGEN_HAVE_DOT NO)
    doxygen_add_docs(docs
        ${CMAKE_CURRENT_SOURCE_DIR}/src/docs.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/elements.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/elements
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/templates.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/templates
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/examples.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/cutting_gallery.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/wood_kernel.md
        ${CMAKE_CURRENT_SOURCE_DIR}/src/joinery_solver
        ${CMAKE_CURRENT_SOURCE_DIR}/src/templates
        COMMENT "Doxygen: build/docs/html"
    )
endif()

