# Version 2.8 or greater is required for packaging
cmake_minimum_required(VERSION 2.8)

# Add local cmake directory to the module search path (for FindVrui)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})



#-------------------
# Target independent configuration
#-------------------


##-- Set project name and target version

project(Crusta)
set(PROJECT_MAJOR_VERSION 1)
set(PROJECT_MINOR_VERSION 0)
set(PROJECT_PATCH_VERSION 0)
set(PROJECT_VERSION "${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_PATCH_VERSION}")


##-- Setup the defaults

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

if(CRUSTA_BUNDLE)
    set(BIN_PATH "${CRUSTA_BUNDLE}/Contents/MacOS")
    set(SHARE_PATH "${CRUSTA_BUNDLE}/Contents/Resources")

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};@loader_path/../Frameworks;@loader_path/../Libraries")

    set(VRUI_MAKEINCLUDE "${CRUSTA_BUNDLE}/Contents/Libraries/Vrui-2.1/share/Vrui.makeinclude")
else()
    set(BIN_PATH "${CMAKE_INSTALL_PREFIX}/bin")
    set(SHARE_PATH "${CMAKE_INSTALL_PREFIX}/share/crusta")
endif()


##-- Setup compile properties

# Add the Crusta include directory
include_directories(../include)

# Show all warnings, don't complain about variadic macros
set_property(GLOBAL APPEND PROPERTY CMAKE_CXX_FLAGS "-Wall")


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

# Setup GLU
find_library(X11_OPENGL_GLU_LIBRARIES
             NAMES GLU
             PATH_SUFFIXES X11)


#-------------------
# Target specifications
#-------------------


##-- Main Crusta components

include(Core)
include(Crusta)
include(Construo)

#include(Package)
