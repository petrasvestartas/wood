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
    src/wood_assign.cpp
    src/wood_beams.cpp
    src/wood_element.cpp
    src/wood_face_to_face.cpp
    src/wood_globals.cpp
    src/wood_internal.cpp
    src/wood_joint.cpp
    src/wood_joint_detection.cpp
    src/wood_main.cpp
    src/wood_merge.cpp
)

function(ADD_WOOD_EXE NAME SRC)
    add_executable(${NAME} ${SRC} ${WOOD_SOURCES} ${ARGN})
    target_include_directories(${NAME} PRIVATE
        ${SESSION_CPP_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(${NAME} PRIVATE session_core)
    LINK_PROTOBUF(${NAME})
endfunction()

function(ADD_SESSION_EXE NAME SRC)
    add_executable(${NAME} ${SRC})
    target_include_directories(${NAME} PRIVATE ${SESSION_CPP_DIR})
    target_link_libraries(${NAME} PRIVATE session_core)
    LINK_PROTOBUF(${NAME})
endfunction()

ADD_WOOD_EXE(main_wood_00_hello        examples/main_wood_00_hello.cpp)
ADD_WOOD_EXE(main_wood_01_new_geometry examples/main_wood_01_new_geometry.cpp)
ADD_WOOD_EXE(main_wood_02_joint_types  examples/main_wood_02_joint_types.cpp)
ADD_WOOD_EXE(main_wood_03_new_joint    examples/main_wood_03_new_joint.cpp)
ADD_WOOD_EXE(main_wood_04_all_datasets examples/main_wood_04_all_datasets.cpp src/wood_test.cpp)
ADD_WOOD_EXE(main_wood_fold_1          examples/main_wood_fold_1.cpp src/wood_test.cpp)
ADD_WOOD_EXE(main_wood_fold_2          examples/main_wood_fold_2.cpp src/wood_test.cpp)
ADD_WOOD_EXE(main_5                    examples/main_5.cpp src/wood_test.cpp)
ADD_WOOD_EXE(main_2                    examples/main_2.cpp)
ADD_SESSION_EXE(main_reciprocal         examples/main_reciprocal.cpp)
ADD_SESSION_EXE(main_1                  examples/main_1.cpp)
ADD_SESSION_EXE(main_4                  examples/main_4.cpp)
ADD_SESSION_EXE(example_loft_holes      examples/example_loft_holes.cpp)
ADD_SESSION_EXE(export_to_wood_xml      examples/export_to_wood_xml.cpp)
