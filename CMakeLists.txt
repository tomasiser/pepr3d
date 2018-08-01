cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( pepr3d )

add_subdirectory(lib/cinder)

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/cinder" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
    APP_NAME    "pepr3d"
    CINDER_PATH ${CINDER_PATH}
    SOURCES     ${APP_PATH}/src/Pepr3D.cpp
    INCLUDES    ${APP_PATH}/src
#   RESOURCES   ${APP_PATH}/../../data/models/8lbs.obj
#   ASSETS_PATH
#   BLOCKS
#   LIBRARIES
)