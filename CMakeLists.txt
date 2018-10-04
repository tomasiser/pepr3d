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

ci_make_app(
    APP_NAME    "pepr3d"
    CINDER_PATH ${CINDER_PATH}
    SOURCES     ${SRC_FILES_IMGUI} ${APP_PATH}/src/Pepr3D.cpp
    INCLUDES    ${APP_PATH}/src ${APP_PATH}/lib/cinder-imgui/include ${APP_PATH}/lib/cinder-imgui/lib/imgui
#   RESOURCES   ${APP_PATH}/../../data/models/8lbs.obj
#   ASSETS_PATH
#   BLOCKS
#   LIBRARIES
)

if(MSVC)
  target_compile_options(pepr3d PRIVATE /W3)
  target_compile_options(cinder PRIVATE /W0)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT pepr3d)
else(MSVC)
  target_compile_options(pepr3d PRIVATE -Wall -Wextra -pedantic)
  target_compile_options(cinder PRIVATE -w)
endif(MSVC)