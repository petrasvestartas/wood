cmake_minimum_required(VERSION 3.22)
project(wood CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(SESSION_CPP_DIR "C:/pc/3_code/code_rust/session/session_cpp")
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
ADD_SESSION_EXE(main_annen_chevron    src/templates/main_annen_chevron.cpp)
ADD_SESSION_EXE(main_beam_reciprocal  src/templates/main_beam_reciprocal.cpp)
ADD_SESSION_EXE(main_annen_faces      src/templates/main_annen_faces.cpp)
ADD_SESSION_EXE(main_json_session     examples/main_json_session.cpp)
ADD_SESSION_EXE(main_loft_holes       examples/main_loft_holes.cpp)
ADD_SESSION_EXE(main_export_xml       examples/main_export_xml.cpp)
ADD_SESSION_EXE(main_chevron_test     src/templates/main_chevron_test.cpp)
ADD_SESSION_EXE(main_vda_mesh         src/templates/main_vda_mesh_test.cpp)
ADD_WOOD_EXE(main_translation_shell   examples/templates/main_translation_shell.cpp)
ADD_WOOD_EXE(main_reflex_fold         examples/templates/main_reflex_fold.cpp)
ADD_WOOD_EXE(main_chevron             examples/templates/main_chevron.cpp)
ADD_SESSION_EXE(main_reciprocal_beam  examples/templates/main_reciprocal_beam.cpp)
