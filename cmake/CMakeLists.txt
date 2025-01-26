########################################################################
# PROPERTIES
########################################################################
cmake_minimum_required(VERSION 3.26)

project(wood LANGUAGES C CXX)
# Set the C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CGAL_CMAKE_EXACT_NT_BACKEND BOOST_BACKEND CACHE STRING "Set CGAL backend to Boost")
set(CGAL_DISABLE_GMP ON CACHE BOOL "Disable GMP in CGAL")
set(CMAKE_DISABLE_FIND_PACKAGE_GMP ON CACHE BOOL "Disable CMake find package for GMP")


# Platform-specific compiler options
if(MSVC)
    # MSVC specific flags
    # Modify runtime library configuration for static linking
    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
    )
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
elseif(CMAKE_COMPILER_IS_GNUCXX)
    # GCC specific flags for static linking
    # Note: This might be needed if you are fully working with static libraries
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif()

# Print the C++ compiler info
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")

# Check for C++ compiler
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR APPLE)
  message(STATUS "The C++ compiler is GCC.")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  message(STATUS "The C++ compiler is Microsoft Visual C++.")
else()
  message(STATUS "The C++ compiler is not GCC or Microsoft Visual C++.")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(ColourBold  "${Esc}[1m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(Magenta     "${Esc}[35m")
  set(Cyan        "${Esc}[36m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan    "${Esc}[1;36m")
  set(BoldWhite   "${Esc}[1;37m")
endif()


set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
cmake_policy(SET CMP0097 NEW)
# cmake_policy(SET CMP0144 NEW)
# Set global property (all targets are impacted)
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

# debug
set(RELEASE_DEBUG "" CACHE STRING "DEBUG to true ")
if(RELEASE_DEBUG)
  message("${BoldBlue} RELEASE IS BEING CONFIGURED ${ColourReset}" )
  set(CMAKE_BUILD_TYPE_INIT "Release")
else()
  message("${BoldBlue} AUTHOR_WARNING DEBUG IS BEING CONFIGURED ${ColourReset}" )
  set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
endif()





########################################################################
#
# START SUPERBUILD - DOWNLOAD LIBRARIES FROM GITHUB REPOSIROTRIES
#
########################################################################
#cmake_policy(SET CMP0097 NEW)
cmake_policy(SET CMP0135 NEW)
include(ExternalProject)



SET(GET_LIBS "" CACHE STRING "Set option to download dependencies")
if (GET_LIBS)
  message(AUTHOR_WARNING "GET_LIBS_" ${GET_LIBS})

  #######################################################################
  # CDT
  ####################################################################### 
  ExternalProject_Add(cdt
  GIT_REPOSITORY    https://github.com/artem-ogre/CDT.git
  GIT_TAG           master
      CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        #-DCMAKE_INSTALL_PREFIX:PATH="${CMAKE_BINARY_DIR}/install"
      SOURCE_DIR   "${CMAKE_BINARY_DIR}/install/cdt/"
        #INSTALL_DIR   "${CMAKE_INSTALL_PREFIX}/install"
      CONFIGURE_COMMAND "" #do not configure
      BUILD_COMMAND "" #do not buld
      INSTALL_COMMAND "" #installer for now is empty
  )

  #######################################################################
  # CLIPPER2
  ####################################################################### 
  ExternalProject_Add(clipper_2
      GIT_REPOSITORY https://github.com/AngusJohnson/Clipper2.git
      GIT_TAG main
      CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        #-DCMAKE_INSTALL_PREFIX:PATH="${CMAKE_BINARY_DIR}/install"
      SOURCE_DIR   "${CMAKE_BINARY_DIR}/install/clipper_2"
        #INSTALL_DIR   "${CMAKE_INSTALL_PREFIX}/install"
      CONFIGURE_COMMAND "" #do not configure
      BUILD_COMMAND "" #do not buld
      INSTALL_COMMAND "" #installer for now is empty
  )



  ########################################################################
  # EIGEN
  ######################################################################## 
  ExternalProject_Add(eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install/eigen #has not effect
    SOURCE_DIR   "${CMAKE_BINARY_DIR}/install/eigen" #install directory is in build/install/eigen
    BUILD_COMMAND "" #do not build
    INSTALL_COMMAND "" #do not install
  )

  ########################################################################
  # GOOGLETEST ///googletest/include
  ######################################################################## 
  set(GOOGLE_TEST_OS "")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      set(GOOGLE_TEST_OS "$<$<PLATFORM_ID:Windows>:-Dgtest_force_shared_crt:BOOL=ON>")
  endif()

    unset(GTEST_ROOT CACHE)
    unset(GMOCK_ROOT CACHE)

      ExternalProject_Add(googletest
          GIT_REPOSITORY https://github.com/google/googletest.git
          GIT_TAG 58d77fa8070e8cec2dc1ed015d66b454c8d78850
          PREFIX ${CMAKE_BINARY_DIR}/install/googletest
          INSTALL_DIR ${CMAKE_BINARY_DIR}/install/googletest
          CMAKE_ARGS
              -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
              -DCMAKE_POSITION_INDEPENDENT_CODE=ON
          ${GOOGLE_TEST_OS} #$<$<PLATFORM_ID:Windows>:-Dgtest_force_shared_crt:BOOL=ON>
          BUILD_COMMAND
              COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release --target install
            # COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Debug --target install
          INSTALL_COMMAND  ""
          )
      set(GTEST_ROOT ${CMAKE_BINARY_DIR}/install/googletest CACHE PATH "" FORCE)
      set(GMOCK_ROOT ${CMAKE_BINARY_DIR}/install/googletest CACHE PATH "" FORCE)


  ########################################################################
  # BOOST
  ########################################################################
  ExternalProject_Add(boost
    URL https://archives.boost.io/release/1.78.0/source/boost_1_78_0.tar.gz
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/boost ${CMAKE_BINARY_DIR}/install/boost/include/boost
  )



  ########################################################################
  # CGAL 
  # https://github.com/CGAL/cgal/releases/download/v5.6.1/CGAL-5.6.1-library.zip
  # https://github.com/CGAL/cgal/releases/download/v6.0.1/CGAL-6.0.1-library.zip
  ######################################################################## 

  ExternalProject_Add(cgal
    URL https://github.com/CGAL/cgal/releases/download/v6.0.1/CGAL-6.0.1-library.zip
    #GIT_REPOSITORY https://github.com/CGAL/cgal.git
    #GIT_TAG v5.4.1
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      #-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
    SOURCE_DIR   "${CMAKE_BINARY_DIR}/install/cgal"
    BUILD_ALWAYS "" #do not build
    INSTALL_COMMAND "" #installer for now is empty
  )

  # SET(CGAL_CMAKE_EXACT_NT_BACKEND BOOST_BACKEND CACHE STRING "")
  # SET(CGAL_DISABLE_GMP ON CACHE BOOL "")
  # SET(CMAKE_DISABLE_FIND_PACKAGE_GMP ON CACHE BOOL "")

  # if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # ExternalProject_Add(cgal_auxiliary
    #   URL https://github.com/CGAL/cgal/releases/download/v5.5/CGAL-5.5-win64-auxiliary-libraries-gmp-mpfr.zip
    #   CMAKE_ARGS
    #     -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    #     -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    #   SOURCE_DIR "${CMAKE_BINARY_DIR}/install/cgal/auxiliary"
    #   CONFIGURE_COMMAND "" # Do not configure
    #   BUILD_COMMAND "" # Do not build
    #   INSTALL_COMMAND "" # Installer for now is empty
    # )
  # endif()

  ########################################################################
  # lnlib
  ########################################################################
  ExternalProject_Add(lnlib
    URL https://github.com/BIMCoderLiang/LNLib/archive/refs/tags/v1.1.zip
    URL_HASH SHA256=485ab44f67170f7b655082a65398be821c60ece75b0162c261848b7a747d21cc
    PREFIX lnlib
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install/eigen
    BUILD_COMMAND "" # Do not build
    INSTALL_COMMAND "" # Do not install
  )


  

endif()



########################################################################
#
# CONFIGURE THE PROJECT
#
########################################################################

###############################################################################
#EXECUTABLE LINK HEADER ONLY DIRECTORY
###############################################################################
SET(BUILD_MY_PROJECTS "" CACHE STRING "Build Project")

if (BUILD_MY_PROJECTS)
  message("${BoldBlue} BUILD_MY_PROJECTS_ ${BUILD_MY_PROJECTS} ${ColourReset}" )

  ###############################################################################
  # CREATE EXE
  ###############################################################################
  add_executable(${PROJECT_NAME} main.cpp 
  "${CMAKE_SOURCE_DIR}/src/wood/include/wood_globals.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_box_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/rtree_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_inscribe_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_intersection_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_xform_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_math_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_plane_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_polyline_mesh_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_polyline_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_rectangle_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_skeleton.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_vector_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/clipper_util.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/wood_element.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/wood_joint.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/wood_joint_lib.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/wood_test.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/wood_main.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/wood_xml.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/cgal_mesh_boolean.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/database_writer.cpp"
  "${CMAKE_SOURCE_DIR}/src/wood/include/tinyply/tinyply.cpp"
  ) 
  
  ###############################################################################
  # PCH support
  ###############################################################################
  message("${BoldBlue}  PRECOMPILED HEADER ${ColourReset}")
  target_precompile_headers(${PROJECT_NAME} PRIVATE  stdafx.h) # With PUBLIC they will be used by targets using this target
  
  set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)

  ###############################################################################
  # LIBRARIES
  ###############################################################################

  ###############################################################################
  # LINK HEADERS TO EXE
  # check path:
  # message(AUTHOR_WARNING "${CMAKE_SOURCE_DIR}/viewer/include/stb")
  ###############################################################################

  list(APPEND petras_include_paths 
  #my_code
  "${CMAKE_SOURCE_DIR}/src/wood/include/"
  #cdt
  "${CMAKE_BINARY_DIR}/install/cdt/CDT/include/"
  #clipper
  "${CMAKE_BINARY_DIR}/install/clipper_2/CPP/Clipper2Lib/include/"
  #boost
  "${CMAKE_BINARY_DIR}/install/boost/include/"
  #eigen
  "${CMAKE_BINARY_DIR}/install/eigen/"
  #cgal
  "${CMAKE_BINARY_DIR}/install/cgal/include"
  #googletest
  "${CMAKE_BINARY_DIR}/install/googletest/include/"
  # SQLite
  "${CMAKE_SOURCE_DIR}/src/sqlite/"
  )


  # if (WIN32) # append gmp to the list
  #   list(APPEND petras_include_paths "${CMAKE_BINARY_DIR}/install/cgal/auxiliary/gmp/include")
  # endif()

  message("${BoldBlue} Include directories: ${petras_include_paths}")

  target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${petras_include_paths}>") #header for the library

  ###############################################################################
  # LINK LIBRARIES TO EXE, STATIC LIBRARIES PATHS MUST BE AT THE END
  ###############################################################################
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      message("________________________________________linking mac and gnu")


      target_link_directories(${PROJECT_NAME} PUBLIC "${CMAKE_BINARY_DIR}/" )
      string(ASCII 27 Esc)
      message("${BoldRed}Error: ${CMAKE_BINARY_DIR}/.${ColorReset}")
      set(ColorReset "${Esc}[m")
      set(BoldRed "${Esc}[1;31m")
      set(BoldGreen "${Esc}[1;32m")
      set(BoldYellow "${Esc}[1;33m")
      message("${CMAKE_BINARY_DIR}/" )
      target_link_libraries(${PROJECT_NAME} PUBLIC 
      Clipper2 
      # gmp
      # mpfr
      gtest
      gmock
      sqlite3
      )
  elseif(APPLE)
    target_link_directories(${PROJECT_NAME} PUBLIC "${CMAKE_BINARY_DIR}/" )
      string(ASCII 27 Esc)
      message("${BoldRed}Error: ${CMAKE_BINARY_DIR}/.${ColorReset}")
      set(ColorReset "${Esc}[m")
      set(BoldRed "${Esc}[1;31m")
      set(BoldGreen "${Esc}[1;32m")
      set(BoldYellow "${Esc}[1;33m")
      message("${CMAKE_BINARY_DIR}/" )
      target_link_libraries(${PROJECT_NAME} PUBLIC 
      Clipper2 
      # /usr/local/lib/libgmp.a
      # /usr/local/lib/libmpfr.a
      gtest
      gmock
      sqlite3
      )
  else()
    target_link_directories(${PROJECT_NAME} PUBLIC "${CMAKE_BINARY_DIR}/Release/" )
    target_link_libraries(${PROJECT_NAME} PUBLIC 
      Clipper2 
      # libgmp-10
      # libmpfr-4
      gtest
      gmock
      sqlite3
    ) 


  endif()

  ###############################################################################
  # BOOST
  ###############################################################################

  set(BOOST_ROOT ${CMAKE_BINARY_DIR}/install/boost)

  ###############################################################################
  # CGAL
  ###############################################################################


  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/install")

  find_package(Boost 1.72 REQUIRED)
  find_package(CGAL REQUIRED)
  target_link_libraries(${PROJECT_NAME} PUBLIC CGAL::CGAL)

  ###############################################################################
  # GOOGLETEST
  ###############################################################################
  target_link_directories(${PROJECT_NAME} PUBLIC "${CMAKE_BINARY_DIR}/install/googletest/lib/" )


endif ()


########################################################################
#
# COMPILE LIBRARIES TO REDUCE BUILD TIME
#
########################################################################
SET(COMPILE_LIBS "" CACHE STRING "Set option to compile dependencies")
if (COMPILE_LIBS)


  ###############################################################################
  # CLIPPER
  ###############################################################################
  set(CLIPPER2_INC
  "${CMAKE_BINARY_DIR}/install/clipper_2/CPP/Clipper2Lib/include/clipper2/clipper.core.h"
  "${CMAKE_BINARY_DIR}/install/clipper_2/CPP/Clipper2Lib/include/clipper2/clipper.engine.h"
  "${CMAKE_BINARY_DIR}/install/clipper_2/CPP/Clipper2Lib/include/clipper2/clipper.h"
  "${CMAKE_BINARY_DIR}/install/clipper_2/CPP/Clipper2Lib/include/clipper2/clipper.minkowski.h"
  "${CMAKE_BINARY_DIR}/install/clipper_2/CPP/Clipper2Lib/include/clipper2/clipper.offset.h"
 )

  set(CLIPPER2_SRC
  "${CMAKE_BINARY_DIR}/install/clipper_2/CPP/Clipper2Lib/src/clipper.engine.cpp"
  "${CMAKE_BINARY_DIR}/install/clipper_2/CPP/Clipper2Lib/src/clipper.offset.cpp"
  )

  add_library(Clipper2 STATIC ${CLIPPER2_INC} ${CLIPPER2_SRC}  )
  target_include_directories(Clipper2  PUBLIC "${CMAKE_BINARY_DIR}/install/clipper_2/CPP/Clipper2Lib/include/") #link headers to clipper library



  ###############################################################################
  # CGAL
  ###############################################################################

  # if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR APPLE)
  #   message("(CMAKE_COMPILER_IS_GNUCC or CMAKE_COMPILER_IS_GNUCXX)")
  # else()
  #   target_link_directories(${PROJECT_NAME} PUBLIC "${CMAKE_BINARY_DIR}/install/cgal/auxiliary/gmp/lib/" )
  # endif()

  
  # if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR APPLE))
  #   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
  #     COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
  #         "${CMAKE_BINARY_DIR}/install/cgal/auxiliary/gmp/lib/libgmp-10.dll"      # <--this is in-file
  #         "${CMAKE_BINARY_DIR}/Release")                 # <--this is out-file path

  #   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
  #     COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
  #         "${CMAKE_BINARY_DIR}/install/cgal/auxiliary/gmp/lib/libmpfr-4.dll"      # <--this is in-file
  #         "${CMAKE_BINARY_DIR}/Release")                 # <--this is out-file path
  # endif()


  ###############################################################################
  # SQLITE
  ###############################################################################
  message( "${CMAKE_SOURCE_DIR}/src/sqlite/sqlite3.c")
  add_library(sqlite3 STATIC "${CMAKE_SOURCE_DIR}/src/sqlite/sqlite3.c"  )
  target_include_directories(sqlite3 PUBLIC "${CMAKE_SOURCE_DIR}/src/sqlite/") #link headers to sqlite library


endif ()
