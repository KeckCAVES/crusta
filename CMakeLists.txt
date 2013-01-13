#
# Optionally bypass CMake's normal variable caching mechanism and hardcode in
# some common values, if that is the kind of thing you're in to.
#

#
# Give path to Vrui, if CMake can't find it automatically.
# (Or, pass to cmake -DCMAKE_PREFIX_PATH=/path/to/Vrui-2.6)
#
list( APPEND CMAKE_PREFIX_PATH $ENV{HOME}/Vrui-2.6 )

#
# Set the installation path prefix.
# (Or, pass to cmake -DCMAKE_INSTALL_PREFIX=/path/to/install)
#
#set( CMAKE_INSTALL_PREFIX $ENV{HOME}/Crusta )


#####

# Version 2.8.3 or greater is required for packaging
cmake_minimum_required(VERSION 2.8.3)

# Add local cmake directory to the module search path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)


#-------------------
# Target independent configuration
#-------------------


##-- Set project name and target version

project(crusta)
set(PROJECT_MAJOR_VERSION 1)
set(PROJECT_MINOR_VERSION 0)
set(PROJECT_PATCH_VERSION 0)
set(PROJECT_VERSION "${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_PATCH_VERSION}")


##-- Setup the defaults

# Set the default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

set(BIN_PATH "${CMAKE_INSTALL_PREFIX}/bin")
set(SHARE_PATH "${CMAKE_INSTALL_PREFIX}/share/crusta")


##-- Setup required external libraries

# Setup pkg-config
include(FindPkgConfig)
macro(pkg_search_module_more PREFIX)
  foreach(NAME ${ARGN})
    if(NOT(NAME MATCHES "REQUIRED" OR NAME MATCHES "QUIET"))
      find_path(${NAME}_PKG_CONFIG_DIR ${NAME}.pc PATH_SUFFIXES lib/pkgconfig lib64/pkgconfig)
      if(${NAME}_PKG_CONFIG_DIR)
        set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${${NAME}_PKG_CONFIG_DIR}")
      endif()
    endif()
  endforeach()
  pkg_search_module(${ARGV})
endmacro()

# Setup GDAL
find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIR})

# Setup Vrui
pkg_search_module_more(VRUI REQUIRED Vrui-2.6 Vrui)
string(REPLACE "-framework;" "-framework " VRUI_LDFLAGS "${VRUI_LDFLAGS}")
add_definitions(${VRUI_CFLAGS})

# Setup GL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

# Setup GLEW
pkg_search_module_more(GLEW REQUIRED glewmx glew)
add_definitions(${GLEW_CFLAGS})


##-- Setup compile properties

# Add Crusta src directory for includes
include_directories(src)

# Report Crusta version
add_definitions(-DCRUSTA_VERSION=\"${PROJECT_VERSION}\")

# Show all warnings, don't complain about variadic macros
set_property(GLOBAL APPEND PROPERTY CMAKE_CXX_FLAGS "-Wall")

# Report share path to source files
add_definitions(-DCRUSTA_SHARE_PATH="${SHARE_PATH}")

# Common setup for executables
macro(add_crusta_exe)
  add_executable(${ARGV})
  target_link_libraries(${ARGV0} crustacore ${VRUI_LDFLAGS} ${GDAL_LIBRARY} ${OPENGL_LIBRARY} ${GLEW_LDFLAGS})
  install(TARGETS ${ARGV0} DESTINATION ${BIN_PATH})
endmacro()


#-------------------
# Target specifications
#-------------------

include(Core)
include(Crusta)
include(Construo)
include(CrustaDesktop)
