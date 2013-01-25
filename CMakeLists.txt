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

cmake_minimum_required(VERSION 2.8)


#-------------------
# Target independent configuration
#-------------------


##-- Set project name and target version

project(crusta)

if(NOT PROJECT_VERSION AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(COMMAND git describe --match v*
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  RESULT_VARIABLE GIT_RESULT
                  OUTPUT_VARIABLE PROJECT_VERSION_RAW
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(GIT_RESULT EQUAL 0)
    string(SUBSTRING ${PROJECT_VERSION_RAW} 1 -1 PROJECT_VERSION)
  endif()
endif()
if(NOT PROJECT_VERSION AND EXISTS "${CMAKE_SOURCE_DIR}/VERSION")
  file(STRINGS VERSION PROJECT_VERSION LIMIT_COUNT 1)
endif()
if(NOT PROJECT_VERSION AND CMAKE_SOURCE_DIR MATCHES "^.*/[^-]*-([0-9].*)$")
  set(PROJECT_VERSION ${CMAKE_MATCH_1})
endif()
if(NOT PROJECT_VERSION)
  message(FATAL_ERROR "Unable to determine PROJECT_VERSION. Please set manually.")
endif()


##-- Setup the defaults

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT INSTALL_TYPE)
  if(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    set(INSTALL_TYPE "FHS")
  else()
    set(INSTALL_TYPE "Private")
  endif()
endif()


##-- Setup install paths

set(BIN_PATH "bin")
if(INSTALL_TYPE STREQUAL "Private")
  set(ETC_PATH "etc")
  set(SHARE_PATH "share")
elseif(INSTALL_TYPE STREQUAL "Shared")
  set(ETC_PATH "etc/crusta")
  set(SHARE_PATH "share/crusta")
elseif(INSTALL_TYPE STREQUAL "FHS")
  set(ETC_PATH "../etc/crusta")
  set(SHARE_PATH "share/crusta")
else()
  message(FATAL_ERROR "Unknown INSTALL_TYPE ${INSTALL_TYPE}; Must be Private, Shared, or FHS.")
endif()
get_filename_component(BIN_ABSPATH "${CMAKE_INSTALL_PREFIX}/${BIN_PATH}" ABSOLUTE)
get_filename_component(ETC_ABSPATH "${CMAKE_INSTALL_PREFIX}/${ETC_PATH}" ABSOLUTE)
get_filename_component(SHARE_ABSPATH "${CMAKE_INSTALL_PREFIX}/${SHARE_PATH}" ABSOLUTE)


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
find_program(GDAL_CONFIG gdal-config)
if(NOT GDAL_CONFIG)
  message(FATAL_ERROR "Unable to find gdal-config program.")
endif()
execute_process(COMMAND ${GDAL_CONFIG} --libs OUTPUT_VARIABLE GDAL_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GDAL_CONFIG} --cflags OUTPUT_VARIABLE GDAL_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions(${GDAL_CFLAGS})

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

# Show all warnings
set_property(GLOBAL APPEND PROPERTY CMAKE_CXX_FLAGS "-Wall")

# Report resource paths to source files
add_definitions(-DCRUSTA_ETC_PATH="${ETC_ABSPATH}")
add_definitions(-DCRUSTA_SHARE_PATH="${SHARE_ABSPATH}")


##-- Install data and configs

install(DIRECTORY share/ DESTINATION ${SHARE_PATH})
install(DIRECTORY etc/ DESTINATION ${ETC_PATH})


#-------------------
# Target specifications
#-------------------

file(GLOB_RECURSE CRUSTACORE_SOURCES src/crustacore/*)
add_library(crustacore STATIC ${CRUSTACORE_SOURCES})

macro(add_crusta_exe)
  add_executable(${ARGV})
  target_link_libraries(${ARGV0} crustacore ${VRUI_LDFLAGS} ${GDAL_LIBS} ${OPENGL_LIBRARY} ${GLEW_LDFLAGS})
  install(TARGETS ${ARGV0} DESTINATION ${BIN_PATH})
endmacro()

file(GLOB_RECURSE CONSTRUO_SOURCES src/construo/*)
add_crusta_exe(construo ${CONSTRUO_SOURCES})

file(GLOB_RECURSE CRUSTA_SOURCES src/crusta/*)
add_crusta_exe(crusta ${CRUSTA_SOURCES})

macro(add_baked_args_exe NAME)
  set(OUTPUT ${NAME})
  add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND echo "#!/bin/sh" > ${OUTPUT}
    COMMAND echo "${BIN_ABSPATH}/crusta" ${ARGN} \"\$@\" >> ${OUTPUT}
    COMMAND chmod +x ${OUTPUT}
    VERBATIM
    DEPENDS crusta
  )
  add_custom_target(create-${OUTPUT} ALL DEPENDS ${OUTPUT})
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT} DESTINATION ${BIN_PATH})
endmacro()

set(desktopargs -mergeConfig ${SHARE_ABSPATH}/vrui/mouse.cfg)
set(slicetoolargs -settings ${SHARE_ABSPATH}/etc/enableSliceTool.cfg)
add_baked_args_exe(crusta-desktop ${desktopargs})
add_baked_args_exe(crusta-slicing ${slicetoolargs})
add_baked_args_exe(crusta-slicing-desktop ${slicetoolargs} ${desktopargs})

