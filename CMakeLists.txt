cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
cmake_policy(SET CMP0074 NEW) # use ProjectName_ROOT for find_package

project(pepr3d)

# ------------------------------------------------------------------------
# ----------------------------- DEPENDENCIES -----------------------------
# ------------------------------------------------------------------------
get_filename_component(APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# -- Boost --
find_package(Boost REQUIRED)
if(NOT Boost_FOUND)
  message(STATUS "This project requires the Boost library!")
  return()
endif()


include_directories(BEFORE ${Boost_INCLUDE_DIR})

# -- gtest --
# Note: Gtest must be added as soon as possible to prevent other dependencies from
# including their own version with custom settings.
set(INSTALL_GTEST OFF CACHE BOOL "Disable installation of gtest")
add_subdirectory(${APP_PATH}/lib/googletest EXCLUDE_FROM_ALL)
set_target_properties(gtest gtest_main PROPERTIES FOLDER Dependencies)

# -- CGAL --
set(CGAL_DONT_OVERRIDE_CMAKE_FLAGS TRUE CACHE BOOL "Force CGAL to maintain CMAKE flags")
set(CGAL_NO_BLANKET_LINKING TRUE CACHE BOOL "Prevent CGAL_USE_FILE from linking to CGAL in every project")

find_package(CGAL REQUIRED COMPONENTS Core)
if(NOT CGAL_FOUND )
  message(FATAL_ERROR "This project requires the CGAL library!")
  return()
endif()
if(NOT CGAL_DIR)
  message(FATAL_ERROR "CGAL_DIR must be set to the root CGAL directory.") # need to find the include files
  return()
endif()

include(${CGAL_USE_FILE})
link_directories(${CGAL_LIBRARIES_DIR} ${CGAL_3RD_PARTY_LIBRARIES_DIRS})

if(WIN32) # Copy cgal 3rd party .dlls into the working directory of the project
  file(COPY ${CGAL_DIR}/auxiliary/gmp/lib/libgmp-10.dll ${CGAL_DIR}/auxiliary/gmp/lib/libmpfr-4.dll
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# -- Assimp --
# Note: find_package(assimp) on Windows breaks on second run
find_package(Assimp REQUIRED)
if(NOT ASSIMP_FOUND )
  message(FATAL_ERROR "This project requires the Assimp library!")
  return()
endif()

# -- Freetype --
# Needs FREETYPE_DIR environment variable defined to the path of Freetype, with directories include/ and lib/ inside
find_package(Freetype REQUIRED)
message(STATUS "Freetype library found at " ${FREETYPE_LIBRARIES})

# -- Cinder --
set(CINDER_BOOST_USE_SYSTEM TRUE CACHE BOOL "Use system boost for Cinder")


# Setting openGL preference to GLVND hides CMake warning and falls back to legacy library if not
# available
set(OpenGL_GL_PREFERENCE GLVND)

set(PEPR3D_BUILD_CINDER ON CACHE BOOL "Enable if Cinder should be built together with Pepr3D")
set(PEPR3D_CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/cinder" CACHE STRING "Path to Cinder")

if(PEPR3D_BUILD_CINDER)
  add_subdirectory(lib/cinder)
  set_target_properties(cinder PROPERTIES FOLDER Dependencies)
endif()

get_filename_component(CINDER_PATH "${PEPR3D_CINDER_PATH}" ABSOLUTE)

include("${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake")

# ------------------------------------------------------------------------
# ------------------------------- SOURCES --------------------------------
# ------------------------------------------------------------------------

# Gather source files
set(PEPR3D_SRC_PATH ${APP_PATH}/src)
set(PEPR3D_TEST_MAIN_FILE ${PEPR3D_SRC_PATH}/main.test.cpp)
set(PEPR3D_MAIN_FILE ${PEPR3D_SRC_PATH}/main.cpp)

file(GLOB SRC_FILES_IMGUI ${APP_PATH}/lib/peprimgui/peprimgui.cpp
          ${APP_PATH}/lib/imgui/* ${APP_PATH}/lib/imgui/misc/cpp/*)

file(GLOB_RECURSE SRC_FILES_PEPR3D LIST_DIRECTORES false "${PEPR3D_SRC_PATH}/*.cpp"
                  "${PEPR3D_SRC_PATH}/*.h")

file(GLOB_RECURSE SRC_FILES_PEPR3DTESTS LIST_DIRECTORES false "${PEPR3D_SRC_PATH}/*.test.cpp")
file(GLOB_RECURSE SRC_FILES_FTGL LIST_DIRECTORES false "${APP_PATH}/lib/FTGL/*.cpp")
file(GLOB_RECURSE SRC_FILES_POLY2TRI LIST_DIRECTORES false "${APP_PATH}/lib/poly2tri/*.cc")

# Remove test files from pepr3d sources
foreach(_source IN ITEMS ${SRC_FILES_PEPR3DTESTS})
  list(REMOVE_ITEM SRC_FILES_PEPR3D ${_source})
endforeach()

list(REMOVE_ITEM SRC_FILES_PEPR3D ${PEPR3D_MAIN_FILE})

ci_make_app(APP_NAME "pepr3d"
            CINDER_PATH ${CINDER_PATH}
            SOURCES     ${SRC_FILES_IMGUI} ${APP_PATH}/resources/Resources.rc ${SRC_FILES_PEPR3D} ${PEPR3D_MAIN_FILE} ${SRC_FILES_FTGL} ${SRC_FILES_POLY2TRI}
            INCLUDES    ${APP_PATH}/src ${APP_PATH}/lib/cereal/include ${APP_PATH}/lib/imgui/misc/cpp ${APP_PATH}/lib/poly2tri ${APP_PATH}/lib/FTGL ${APP_PATH}/lib/threadpool ${APP_PATH}/lib/peprimgui ${APP_PATH}/lib/imgui ${FREETYPE_INCLUDE_DIRS}
            RESOURCES   ${APP_PATH}/resources/icon.ico
            # ASSETS_PATH
            # BLOCKS
            # LIBRARIES
)

target_include_directories(pepr3d PRIVATE ${ASSIMP_INCLUDE_DIR})
target_link_libraries(pepr3d ${ASSIMP_LIBRARY_RELEASE})

target_link_libraries(pepr3d ${FREETYPE_LIBRARIES})

target_link_libraries(pepr3d ${CGAL_LIBRARIES} ${CGAL_3RD_PARTY_LIBRARIES})
#add_dependencies(pepr3d CGAL::CGAL CGAL::CGAL_Core)

# Group source files into filters (for MSVC)
foreach(_source IN ITEMS ${SRC_FILES_PEPR3D})
  get_filename_component(_source_path "${_source}" PATH)
  file(RELATIVE_PATH _source_path_rel "${PEPR3D_SRC_PATH}" "${_source_path}")
  string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
  source_group("${_group_path}" FILES "${_source}")
endforeach()

# --- Tests ---
# Create separate project for tests
add_executable(pepr3dtests ${SRC_FILES_IMGUI} ${SRC_FILES_PEPR3D} ${SRC_FILES_PEPR3DTESTS} ${SRC_FILES_FTGL} ${SRC_FILES_POLY2TRI})
#add_dependencies(pepr3dtests gtest CGAL::CGAL CGAL::CGAL_Core)

target_include_directories(pepr3dtests
                           PRIVATE ${APP_PATH}/src
                                   ${APP_PATH}/lib/threadpool
                                   ${APP_PATH}/lib/cereal/include
                                   ${APP_PATH}/lib/poly2tri
                                   ${APP_PATH}/lib/FTGL
                                   ${APP_PATH}/lib/peprimgui
                                   ${APP_PATH}/lib/imgui
                                   ${APP_PATH}/lib/imgui/misc/cpp
                                   ${APP_PATH}/lib/cinder/include
                                   ${APP_PATH}/lib/googletest/googletest/include)
target_include_directories(pepr3dtests PRIVATE ${ASSIMP_INCLUDE_DIR})
target_include_directories(pepr3dtests PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(pepr3dtests gtest ${ASSIMP_LIBRARY_RELEASE} cinder ${FREETYPE_LIBRARIES})
target_link_libraries(pepr3dtests ${CGAL_LIBRARIES} ${CGAL_3RD_PARTY_LIBRARIES})

# copy dlls into working directory on Windows
if(WIN32)
  #assimp
  if(ASSIMP_DLL_FILE)
    file(COPY ${ASSIMP_DLL_FILE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})      
  endif()
endif()

# copy test files to binary dir
file(COPY ${APP_PATH}/tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# ------------------------------------------------------------------------
# --------------------------- COMPILER FLAGS -----------------------------
# ------------------------------------------------------------------------

target_compile_definitions(pepr3d PRIVATE $<$<CONFIG:RELEASE>:CI_MIN_LOG_LEVEL=3>)

if(MSVC)
  target_compile_options(pepr3d PRIVATE /W3 /std:c++17 /D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)
  target_compile_options(pepr3dtests PRIVATE /W3 /std:c++17 /D_TEST_ /D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING /DPEPR3D_EDGE_CONSISTENCY_CHECK)
  target_compile_options(cinder PRIVATE /W0)

  # Note: /std:c++14 flag is not present in cinder INTERFACE_COMPILE_OPTIONS for some reason for
  # MSVC

  # Set MSVC startup project
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT pepr3d)

  # Copy resharper settings
  file(COPY ${APP_PATH}/pepr3d.sln.DotSettings DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
else()
  target_compile_options(pepr3d PRIVATE -Wall -Wextra -pedantic -std=c++17)
  target_compile_options(pepr3dtests PRIVATE -Wall -Wextra -pedantic -std=c++17 -D_TEST_ -DPEPR3D_EDGE_CONSISTENCY_CHECK)

  # Replace c++14 flag forced by Cinder with c++17
  get_target_property(CINDER_COMPILE_FLAGS cinder INTERFACE_COMPILE_OPTIONS)
  list(REMOVE_ITEM CINDER_COMPILE_FLAGS -std=c++14)
  list(APPEND CINDER_COMPILE_FLAGS -std=c++17)
  set_target_properties(cinder PROPERTIES INTERFACE_COMPILE_OPTIONS "${CINDER_COMPILE_FLAGS}")

  # target_compile_options(cinder PRIVATE -w -fpic)
endif()
