# Version 2.8 or greater is required for packaging
cmake_minimum_required(VERSION 2.8)

# Add local cmake directory to the module search path (for FindVrui)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})



#-------------------
# Target independent configuration
#-------------------


##-- Set project name and target version

project(Crusta)
set(${PROJECT_NAME}_MAJOR_VERSION 0)
set(${PROJECT_NAME}_MINOR_VERSION 1)
set(${PROJECT_NAME}_PATCH_VERSION 0)


##-- Setup the default build

# Setup the distribution build option
option(DIST_BUILD "Build for packaging and distribution" OFF)

# Setup the default build type
if(NOT CMAKE_BUILD_TYPE)
    set(DEFAULT_BUILD_TYPE Release CACHE INTERNAL "Default build type")
    set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE} CACHE STRING
        "Choose the type of build (Debug Release RelWithDebInfo MinSizeRel)"
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)


##-- Setup default paths

# Setup the default install prefix
if(NOT DIST_BUILD AND DEFINED CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
        "${CMAKE_SOURCE_DIR}/../install/${CMAKE_BUILD_TYPE}"
        CACHE INTERNAL "Install path prefix, used to setup install paths."
        FORCE)
endif(NOT DIST_BUILD AND DEFINED CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# Setup the default install paths
set(CRUSTA_EXECUTABLE_PATH "${CMAKE_INSTALL_PREFIX}/bin"
    CACHE PATH "Crusta install executable path")
set(CRUSTA_LIBRARY_PATH "${CMAKE_INSTALL_PREFIX}/lib"
    CACHE PATH "Crusta install library path")
set(CRUSTA_SHARE_PATH "${CMAKE_INSTALL_PREFIX}/share/crusta"
    CACHE PATH "Crusta install share path")

# Only use rpath for non-distribution builds
if(NOT DIST_BUILD)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CRUSTA_LIBRARY_PATH}")
    set(CMAKE_INSTALL_NAME_DIR "${CRUSTA_LIBRARY_PATH}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    message(STATUS "Using rpath for executables")
endif(NOT DIST_BUILD)


##-- Setup compile properties

# Add the Crusta include directory
include_directories(../include)

# Set GLEW to multi-context mode and add the share path
add_definitions(-DGLEW_MX -DCRUSTA_SHARE_PATH=\"${CRUSTA_SHARE_PATH}\")

# Show all warnings, don't complain about variadic macros
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")


##-- Setup required external libraries

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

# Setup Vrui
find_package(Vrui REQUIRED)

# Setup GLU
find_library(X11_OPENGL_GLU_LIBRARIES
             NAMES GLU
             PATH_SUFFIXES X11)


##--Check and report build type

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Build type: Debug")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Build type: Release")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(STATUS "Build type: Release with debug info")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    message(STATUS "Build type: Minimum size release")
else()
    message(FATAL_ERROR "Invalid build type \"${CMAKE_BUILD_TYPE}\". "
                        "Valid values are:\n\t" "Debug\n\t" "Release\n\t"
                        "RelWithDebInfo\n\t" "MinSizeRel")
endif()

if(DIST_BUILD)
    message(STATUS "Building in distribution mode: assuming we are going to "
                   "make release packages")
else()
    message(STATUS "Building in non-distribution mode: assuming non-standard "
                   "install location and no packaging")
endif()
message(STATUS "Install to:\n\t"
               "Executables: ${CRUSTA_EXECUTABLE_PATH}\n\t"
               "Libraries: ${CRUSTA_LIBRARY_PATH}\n\t"
               "Shared Files: ${CRUSTA_SHARE_PATH}")



#-------------------
# Target specifications
#-------------------


##-- Main Crusta components

include(CrustaCore)

include(CrustaApp)

include(CrustaVislet)

include(Construo)


##-- Installation and packaging

include(Install)

#if(DIST_BUILD)
    include(Package)
#endif(DIST_BUILD)
