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
#   2. ../session_cpp next to this repo - a checkout, or a symlink to one. This is
#      the wood_project layout (see ../README.md): one kernel checkout shared by
#      wood and wood_nano, so `git pull` there updates both.
#   3. a fresh FetchContent clone of GitHub main (CI, or a lone checkout)
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
    "Local session_cpp checkout; ../session_cpp or a GitHub clone when empty")

set(SESSION_CPP_DIR "")
foreach(_candidate IN ITEMS
        "${SESSION_CPP_LOCAL}"
        "$ENV{SESSION_CPP_LOCAL}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../session_cpp"
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

add_subdirectory(${SESSION_CPP_DIR} session_cpp EXCLUDE_FROM_ALL)

set(WOOD_SOURCES
    src/joinery_solver/wood_assign.cpp
    src/joinery_solver/wood_beams.cpp
    src/joinery_solver/wood_element.cpp
    src/joinery_solver/wood_face_to_face.cpp
    src/joinery_solver/wood_globals.cpp
    src/joinery_solver/wood_internal.cpp
    src/joinery_solver/wood_joint.cpp
    src/joinery_solver/wood_joint_detection.cpp
    src/joinery_solver/wood_main.cpp
    src/joinery_solver/wood_merge.cpp
    src/joinery_solver/wood_session.cpp
)

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

add_library(wood_core OBJECT ${WOOD_SOURCES} ${WOOD_CLIPPER2_SOURCES})
target_include_directories(wood_core PUBLIC
    ${SESSION_CPP_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/joinery_solver
    # The template headers (reciprocal.h, vda_mesh.h, chevron.h) live here and
    # are included unqualified by the mains under src/templates/temp/.
    ${CMAKE_CURRENT_SOURCE_DIR}/src/templates
    # session_core's own PUBLIC set: src, fmt, json, guid, yaml. Taken from the
    # target rather than respelled, so it cannot drift from session_cpp.
    $<TARGET_PROPERTY:session_core,INTERFACE_INCLUDE_DIRECTORIES>
    # Protobuf gencode headers - what LINK_PROTOBUF adds to each executable. Read from
    # session_cpp, which is the one place they exist; this used to point at a local
    # mirror of them (see the note above add_subdirectory).
    ${SESSION_CPP_DIR}/generated
)

# Every executable in this project: wood solver + session kernel + protobuf.
# Extra sources (e.g. wood_test.cpp) go in ${ARGN} and are compiled per target.
function(ADD_EXE NAME SRC)
    add_executable(${NAME} ${SRC} ${ARGN})
    target_link_libraries(${NAME} PRIVATE wood_core session_core)
    LINK_PROTOBUF(${NAME})
endfunction()

ADD_EXE(main_hello               examples/main_hello.cpp)
ADD_EXE(main_annen_corner        examples/main_annen_corner.cpp)
ADD_EXE(main_element_mapping_check examples/main_element_mapping_check.cpp)
ADD_EXE(main_joint_types         examples/main_joint_types.cpp)
ADD_EXE(main_cross_corners       examples/main_cross_corners.cpp)
ADD_EXE(main_all_datasets        examples/main_all_datasets.cpp src/joinery_solver/wood_test.cpp)
ADD_EXE(main_dataset_runner      examples/main_dataset_runner.cpp src/joinery_solver/wood_test.cpp)
# main_cross_polylines is gone: it was built on session_cpp's ElementFeature /
# CrossElementFeature subsystem, deleted upstream in 57a9b3f7 as "dead
# ElementFeature/Joint subsystem (only-self-referential, real joinery bypasses
# it)". There is no replacement to port it to - Element::add_feature is a mesh
# transform, not a pairwise relation - and it was the only caller left.
ADD_EXE(main_cdt_probe           examples/main_cdt_probe.cpp)
ADD_EXE(main_face_to_face        examples/main_face_to_face.cpp)
ADD_EXE(main_annen_chevron       src/templates/temp/main_annen_chevron.cpp)
ADD_EXE(main_beam_reciprocal     src/templates/temp/main_beam_reciprocal.cpp)
ADD_EXE(main_annen_faces         src/templates/temp/main_annen_faces.cpp)
ADD_EXE(main_json_session        examples/main_json_session.cpp)
ADD_EXE(main_loft_holes          examples/main_loft_holes.cpp)
ADD_EXE(main_export_xml          examples/main_export_xml.cpp)
ADD_EXE(main_chevron_test        src/templates/temp/main_chevron_test.cpp)
ADD_EXE(main_vda_mesh            src/templates/temp/main_vda_mesh_test.cpp)
ADD_EXE(main_translation_shell   examples/templates/main_translation_shell.cpp)
ADD_EXE(main_reflex_fold         examples/templates/main_reflex_fold.cpp)
ADD_EXE(main_chevron             examples/templates/main_chevron.cpp)
ADD_EXE(main_reciprocal_rotation examples/templates/main_reciprocal_rotation.cpp)
ADD_EXE(main_reciprocal_move     examples/templates/main_reciprocal_move.cpp)
