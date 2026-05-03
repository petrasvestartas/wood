cmake_minimum_required(VERSION 3.22)
project(wood CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(GENERATED_DIR "${CMAKE_SOURCE_DIR}/generated")
add_subdirectory(session_cpp EXCLUDE_FROM_ALL)

set(WOOD_SOURCES
    wood/wood_beams.cpp
    wood/wood_element.cpp
    wood/wood_face_to_face.cpp
    wood/wood_globals.cpp
    wood/wood_internal.cpp
    wood/wood_joint.cpp
    wood/wood_joint_detection.cpp
    wood/wood_main.cpp
    wood/wood_merge.cpp
)

function(ADD_WOOD_EXE NAME SRC)
    add_executable(${NAME} ${SRC} ${WOOD_SOURCES} ${ARGN})
    target_include_directories(${NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/session_cpp
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/wood
    )
    target_link_libraries(${NAME} PRIVATE session_core)
    LINK_PROTOBUF(${NAME})
endfunction()

ADD_WOOD_EXE(main_wood_00_hello        wood/main_wood_00_hello.cpp)
ADD_WOOD_EXE(main_wood_01_new_geometry wood/main_wood_01_new_geometry.cpp)
ADD_WOOD_EXE(main_wood_02_joint_types  wood/main_wood_02_joint_types.cpp)
ADD_WOOD_EXE(main_wood_03_new_joint    wood/main_wood_03_new_joint.cpp)
ADD_WOOD_EXE(main_wood_04_all_datasets wood/main_wood_04_all_datasets.cpp wood/wood_test.cpp)
ADD_WOOD_EXE(main_wood_fold_1          wood/main_wood_fold_1.cpp wood/wood_test.cpp)
ADD_WOOD_EXE(main_wood_fold_2          wood/main_wood_fold_2.cpp wood/wood_test.cpp)
ADD_WOOD_EXE(main_5                    wood/main_5.cpp wood/wood_test.cpp)
ADD_WOOD_EXE(main_2                    wood/main_2.cpp wood/wood_joint_detection.cpp)
