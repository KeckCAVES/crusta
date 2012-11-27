#
# Optionally bypass CMake's normal variable caching mechanism and hardcode in
# some common values, if that is the kind of thing you're in to.
#

#
# Give path to the Vrui.makeinclude makefile fragment
# (Or, pass to cmake -DVRUI_MAKEINCLUDE=/path/to/Vrui-2.4/share/Vrui.makeinclude)
#
#set( VRUI_MAKEINCLUDE $ENV{HOME}/Vrui-2.4/share/Vrui.makeinclude )

#
# Set the installation path prefix.
# (Or, pass to cmake -DCMAKE_INSTALL_PREFIX=/path/to/install)
#
#set( CMAKE_INSTALL_PREFIX $ENV{HOME}/Crusta )

#
# Enable use of slicing tool.
# (Or, pass to cmake -DCRUSTA_SLICING=ON)
# This has potential performance overhead even if the tool is not in use.
#
#set( CRUSTA_SLICING ON )


#####

# Version 2.8.3 or greater is required for packaging
cmake_minimum_required(VERSION 2.8.3)

# Add local cmake directory to the module search path (for FindVrui)
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

# By deafult, disable building of crusta with the slicing tool
set( CRUSTA_SLICING OFF
     CACHE BOOL "Enable the slicing tool. This will create another executable called crusta-slicing that has the slicing tool available, at the possible cost of speed and other features.")

if(CRUSTA_BUNDLE)
    set(BIN_PATH "${CRUSTA_BUNDLE}/Contents/MacOS")
    set(SHARE_PATH "${CRUSTA_BUNDLE}/Contents/Resources")

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};@loader_path/../Frameworks;@loader_path/../Libraries")

    set(VRUI_MAKEINCLUDE "${CRUSTA_BUNDLE}/Contents/Libraries/Vrui-2.4/share/Vrui.makeinclude")
else()
    set(BIN_PATH "${CMAKE_INSTALL_PREFIX}/bin")
    set(SHARE_PATH "${CMAKE_INSTALL_PREFIX}/share/crusta")
endif()


##-- Setup required external libraries

if(CRUSTA_BUNDLE)
    find_library(GDAL_LIBRARY GDAL
                 PATHS "${CRUSTA_BUNDLE}/Contents/Frameworks"
                 NO_DEFAULT_PATH)
else()
    # Setup GDAL
    if(APPLE)
        find_library(GDAL_LIBRARY GDAL)
        if(GDAL_NOT_FOUND)
            message(FATAL_ERROR "Required GDAL library not found")
        endif(GDAL_NOT_FOUND)
    else(APPLE)
        find_package(GDAL REQUIRED)
        include_directories(${GDAL_INCLUDE_DIR})
    endif(APPLE)
endif()

# Setup Vrui
find_package(Vrui REQUIRED)
add_definitions(${VRUI_CFLAGS})

# Setup GLU
find_library(X11_OPENGL_GLU_LIBRARIES
             NAMES GLU
             PATH_SUFFIXES X11)


##-- Setup compile properties

# Add the Crusta include directory
include_directories(include)

# Report Crusta version
add_definitions(-DCRUSTA_VERSION=\"${PROJECT_VERSION}\")

# Show all warnings, don't complain about variadic macros
set_property(GLOBAL APPEND PROPERTY CMAKE_CXX_FLAGS "-Wall")

# Report share path to source files
add_definitions(-DCRUSTA_SHARE_PATH="${SHARE_PATH}")

# Enable multi-context GLEW
add_definitions(-DGLEW_MX)

# Common setup for executables
macro(add_crusta_exe)
  add_executable(${ARGV})
  target_link_libraries(${ARGV0} crustacore ${VRUI_LINKFLAGS} ${GDAL_LIBRARY} ${X11_OPENGL_GLU_LIBRARIES})
  install(TARGETS ${ARGV0} DESTINATION ${BIN_PATH})
endmacro()


#-------------------
# Target specifications
#-------------------


##-- Main Crusta components

include(Core)
include(Crusta)
include(Construo)

include(Package)
