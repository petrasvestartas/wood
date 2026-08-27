cmake_minimum_required(VERSION 3.22)
project(wood CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# session_cpp: use a local checkout when there is one, otherwise fetch it.
#
# This used to be a bare `set(SESSION_CPP_DIR "C:/pc/3_code/.../session_cpp")`
# pointing at one developer's machine, so configuring anywhere else failed with
# `add_subdirectory given source ... which is not an existing directory`, and
# then `Unknown CMake command "LINK_PROTOBUF"` because that macro comes from
# session_cpp and was never defined. CI has been red since that landed.
#
# wood_nano already resolves it this way; the same order is used here so a
# developer with the checkout keeps building against it.
include(FetchContent)

set(SESSION_CPP_LOCAL "C:/pc/3_code/code_rust/session/session_cpp" CACHE PATH
    "Local session_cpp checkout; fetched from GitHub when absent")

if(EXISTS "${SESSION_CPP_LOCAL}/CMakeLists.txt")
    set(SESSION_CPP_DIR "${SESSION_CPP_LOCAL}")
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

# session_cpp's CMakeLists locates its generated protobuf sources through
# CMAKE_SOURCE_DIR, which is the TOP-level source dir - this project, not
# session_cpp - once it is added as a subdirectory. Without this it stops with
# "No generated protobuf files found in <wood>/generated". wood_nano carries
# the same copy step for the same reason.
# Refresh it on every configure rather than copying once. This was guarded by
# `if(NOT EXISTS "${CMAKE_SOURCE_DIR}/generated")`, so once the directory
# existed it was never updated again. session_cpp regenerated its protobuf
# sources with a newer protoc, and every build here went on using the stale
# snapshot until it stopped compiling altogether with
#   error C1189: "Protobuf C++ gencode is built with an incompatible version"
# because the pinned PROTOBUF_VERSION no longer matched the runtime headers.
# That is why wood could not be rebuilt for months.
#
# file(COPY) preserves timestamps and skips files that already match, so this
# costs nothing when it is already current.
if(IS_SYMLINK "${CMAKE_SOURCE_DIR}/generated")
    # A developer may symlink it straight at session_cpp; that is always fresh,
    # and copying onto it would have source and destination be the same files.
    message(STATUS "wood: generated/ is a symlink, leaving it as is")
else()
    message(STATUS "wood: syncing session_cpp/generated into ${CMAKE_SOURCE_DIR}/generated")
    file(COPY "${SESSION_CPP_DIR}/generated" DESTINATION "${CMAKE_SOURCE_DIR}")
endif()

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
)

function(ADD_WOOD_EXE NAME SRC)
    add_executable(${NAME} ${SRC} ${WOOD_SOURCES} ${ARGN})
    target_include_directories(${NAME} PRIVATE
        ${SESSION_CPP_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/joinery_solver
    )
    target_link_libraries(${NAME} PRIVATE session_core)
    LINK_PROTOBUF(${NAME})
endfunction()

function(ADD_SESSION_EXE NAME SRC)
    add_executable(${NAME} ${SRC})
    target_include_directories(${NAME} PRIVATE
        ${SESSION_CPP_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(${NAME} PRIVATE session_core)
    LINK_PROTOBUF(${NAME})
endfunction()

ADD_WOOD_EXE(main_hello              examples/main_hello.cpp)
ADD_WOOD_EXE(main_annen_corner       examples/main_annen_corner.cpp)
ADD_WOOD_EXE(main_joint_types        examples/main_joint_types.cpp)
ADD_WOOD_EXE(main_cross_corners      examples/main_cross_corners.cpp)
ADD_WOOD_EXE(main_all_datasets       examples/main_all_datasets.cpp src/joinery_solver/wood_test.cpp)
ADD_WOOD_EXE(main_dataset_runner     examples/main_dataset_runner.cpp src/joinery_solver/wood_test.cpp)
ADD_WOOD_EXE(main_cross_polylines    examples/main_cross_polylines.cpp)
ADD_WOOD_EXE(main_cdt_probe          examples/main_cdt_probe.cpp)
ADD_SESSION_EXE(main_annen_chevron    src/templates/temp/main_annen_chevron.cpp)
ADD_SESSION_EXE(main_beam_reciprocal  src/templates/temp/main_beam_reciprocal.cpp)
ADD_SESSION_EXE(main_annen_faces      src/templates/temp/main_annen_faces.cpp)
ADD_SESSION_EXE(main_json_session     examples/main_json_session.cpp)
ADD_SESSION_EXE(main_loft_holes       examples/main_loft_holes.cpp)
ADD_SESSION_EXE(main_export_xml       examples/main_export_xml.cpp)
ADD_SESSION_EXE(main_chevron_test     src/templates/temp/main_chevron_test.cpp)
ADD_SESSION_EXE(main_vda_mesh         src/templates/temp/main_vda_mesh_test.cpp)
ADD_WOOD_EXE(main_translation_shell   examples/templates/main_translation_shell.cpp)
ADD_WOOD_EXE(main_reflex_fold         examples/templates/main_reflex_fold.cpp)
ADD_WOOD_EXE(main_chevron             examples/templates/main_chevron.cpp)
ADD_SESSION_EXE(main_reciprocal_rotation  examples/templates/main_reciprocal_rotation.cpp)
ADD_SESSION_EXE(main_reciprocal_move      examples/templates/main_reciprocal_move.cpp)
