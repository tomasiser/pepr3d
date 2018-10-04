cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )
set( CMAKE_POSITION_INDEPENDENT_CODE ON )

project( pepr3d )


set( PEPR3D_BUILD_CINDER ON CACHE BOOL "Enable if Cinder should be built together with Pepr3D" )
set( PEPR3D_CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/cinder" CACHE STRING "Path to Cinder" )

if( PEPR3D_BUILD_CINDER )
    add_subdirectory( lib/cinder )
endif()

get_filename_component( CINDER_PATH "${PEPR3D_CINDER_PATH}" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

file(GLOB SRC_FILES_IMGUI 
    ${APP_PATH}/lib/cinder-imgui/src/*
    ${APP_PATH}/lib/cinder-imgui/lib/imgui/*
)

set(PEPR3D_SRC_PATH ${APP_PATH}/src)
file(GLOB_RECURSE SRC_FILES_PEPR3D
	LIST_DIRECTORES false
	"${PEPR3D_SRC_PATH}/*.cpp"
	"${PEPR3D_SRC_PATH}/*.h"
	)

ci_make_app(
    APP_NAME    "pepr3d"
    CINDER_PATH ${CINDER_PATH}
    SOURCES     ${SRC_FILES_IMGUI} ${SRC_FILES_PEPR3D}
    INCLUDES    ${APP_PATH}/src ${APP_PATH}/lib/cinder-imgui/include ${APP_PATH}/lib/cinder-imgui/lib/imgui ${APP_PATH}/src
#   RESOURCES   ${APP_PATH}/../../data/models/8lbs.obj
#   ASSETS_PATH
#   BLOCKS
#   LIBRARIES
)



file(
    GLOB_RECURSE _source_list 
    LIST_DIRECTORIES false
    "${PEPR3D_SRC_PATH}/*.cpp*"
    "${PEPR3D_SRC_PATH}/*.h*"
)

# Group source files into filters (for MSVC)
foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${PEPR3D_SRC_PATH}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()


# Set build args for each platform
if(MSVC)
  target_compile_options(pepr3d PRIVATE /W3 /std:c++17)
  target_compile_options(cinder PRIVATE /W0)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT pepr3d)
else(MSVC)
  target_compile_options(pepr3d PRIVATE -Wall -Wextra -pedantic -std=c++17)
  target_compile_options(cinder PRIVATE -w)
endif(MSVC)