cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )
set( CMAKE_POSITION_INDEPENDENT_CODE ON )

project( pepr3d )

# Make Cinder use local boost on unix platforms, this fixes missing -fpic compile switch on Ubuntu 16+
if(UNIX)
   set(CINDER_BOOST_USE_SYSTEM 1)
endif()

# Setting openGL preference to GLVND hides CMake warning and falls back to legacy library if not available
set(OpenGL_GL_PREFERENCE GLVND)

set( PEPR3D_BUILD_CINDER ON CACHE BOOL "Enable if Cinder should be built together with Pepr3D" )
set( PEPR3D_CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/cinder" CACHE STRING "Path to Cinder" )

if( PEPR3D_BUILD_CINDER )
    add_subdirectory( lib/cinder )
endif()

get_filename_component( CINDER_PATH "${PEPR3D_CINDER_PATH}" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

# Gather source files
set(PEPR3D_SRC_PATH ${APP_PATH}/src)
set(PEPR3D_TEST_MAIN_FILE ${PEPR3D_SRC_PATH}/main.test.cpp)
set(PEPR3D_MAIN_FILE ${PEPR3D_SRC_PATH}/Pepr3D.cpp)

file(GLOB SRC_FILES_IMGUI 
    ${APP_PATH}/lib/cinder-imgui/src/*
    ${APP_PATH}/lib/cinder-imgui/lib/imgui/*
)


file(GLOB_RECURSE SRC_FILES_PEPR3D
	LIST_DIRECTORES false
	"${PEPR3D_SRC_PATH}/*.cpp"
	"${PEPR3D_SRC_PATH}/*.h"
	)
list(REMOVE_ITEM SRC_FILES_PEPR3D ${PEPR3D_TEST_MAIN_FILE})
list(REMOVE_ITEM SRC_FILES_PEPR3D ${PEPR3D_MAIN_FILE})
	
ci_make_app(
    APP_NAME    "pepr3d"
    CINDER_PATH ${CINDER_PATH}
    SOURCES     ${SRC_FILES_IMGUI} ${SRC_FILES_PEPR3D} ${PEPR3D_MAIN_FILE}
    INCLUDES    ${APP_PATH}/src ${APP_PATH}/lib/cinder-imgui/include ${APP_PATH}/lib/cinder-imgui/lib/imgui
#   RESOURCES   ${APP_PATH}/../../data/models/8lbs.obj
#   ASSETS_PATH
#   BLOCKS
#   LIBRARIES
)

# Group source files into filters (for MSVC)
foreach(_source IN ITEMS ${SRC_FILES_PEPR3D})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${PEPR3D_SRC_PATH}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

# --- Tests ---
# Create separate project for tests
# (do not link to cinder or imgui)
add_subdirectory(${APP_PATH}/lib/googletest)
add_executable(pepr3dtests ${SRC_FILES_PEPR3D} ${PEPR3D_TEST_MAIN_FILE})
add_dependencies(pepr3dtests gtest)
target_link_libraries(pepr3dtests gtest)
target_include_directories(pepr3dtests PRIVATE ${APP_PATH}/src ${APP_PATH}/lib/cinder-imgui/include ${APP_PATH}/lib/cinder-imgui/lib/imgui ${APP_PATH}/lib/googletest/googletest/include)

# Set build args for each platform
if(MSVC)
  target_compile_options(pepr3d PRIVATE /W3 /std:c++17)
  target_compile_options(pepr3dtests PRIVATE /W3 /std:c++17 /D_TEST_)
  target_compile_options(cinder PRIVATE /W0)
  
  # Note: /std:c++14 flag is not present in cinder INTERFACE_COMPILE_OPTIONS for some reason for MSVC
  
  # Set MSVC startup project
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT pepr3d)
  
  # Copy resharper settings
  file(COPY ${APP_PATH}/pepr3d.sln.DotSettings DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
else()
  target_compile_options(pepr3d PRIVATE -Wall -Wextra -pedantic -std=c++17)
  target_compile_options(pepr3dtests PRIVATE -Wall -Wextra -pedantic -std=c++17 -D_TEST_)

  # Replace c++14 flag forced by Cinder with c++17
  get_target_property(CINDER_COMPILE_FLAGS cinder INTERFACE_COMPILE_OPTIONS)
  list(REMOVE_ITEM CINDER_COMPILE_FLAGS -std=c++14)
  list(APPEND CINDER_COMPILE_FLAGS -std=c++17)
  set_target_properties(cinder PROPERTIES INTERFACE_COMPILE_OPTIONS "${CINDER_COMPILE_FLAGS}")

#   target_compile_options(cinder PRIVATE -w -fpic)
endif()