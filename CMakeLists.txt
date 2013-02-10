#
# Optionally bypass CMake's normal variable caching mechanism and hardcode in
# some common values, if that is the kind of thing you're in to.
#

#
# Give path to directory of Vrui.pc, if CMake can't find it automatically.
# (Or, prepend PKG_CONFIG_PATH=/path/to/Vrui/pkgconfig to cmake)
#
#set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:$ENV{HOME}/Vrui-2.6/lib64/pkgconfig" )

#
# Set the installation path prefix.
# (Or, pass to cmake -DCMAKE_INSTALL_PREFIX=/path/to/install)
#
#set( CMAKE_INSTALL_PREFIX  $ENV{HOME}/Crusta  CACHE PATH "Install prefix" )


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
    set(INSTALL_TYPE "System")
  elseif(CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
    set(INSTALL_TYPE "Local")
  else()
    set(INSTALL_TYPE "Simple")
  endif()
endif()


##-- Setup install paths

get_filename_component(BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin" ABSOLUTE CACHE)
if(INSTALL_TYPE STREQUAL "Simple")
  get_filename_component(ETC_DIR "${CMAKE_INSTALL_PREFIX}/etc" ABSOLUTE CACHE)
  get_filename_component(SHARE_DIR "${CMAKE_INSTALL_PREFIX}/share" ABSOLUTE CACHE)
elseif(INSTALL_TYPE STREQUAL "Local")
  get_filename_component(ETC_DIR "${CMAKE_INSTALL_PREFIX}/etc/${PROJECT_NAME}" ABSOLUTE CACHE)
  get_filename_component(SHARE_DIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}" ABSOLUTE CACHE)
elseif(INSTALL_TYPE STREQUAL "System")
  get_filename_component(ETC_DIR "${CMAKE_INSTALL_PREFIX}/../etc/${PROJECT_NAME}" ABSOLUTE CACHE)
  get_filename_component(SHARE_DIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}" ABSOLUTE CACHE)
else()
  message(FATAL_ERROR "Unknown INSTALL_TYPE ${INSTALL_TYPE}; Must be Simple, Local, or System.")
endif()


##-- Setup required external libraries

# Setup pkg-config
include(FindPkgConfig)

# Setup GDAL
find_program(GDAL_CONFIG gdal-config)
if(NOT GDAL_CONFIG)
  message(FATAL_ERROR "Unable to find gdal-config program.")
endif()
execute_process(COMMAND ${GDAL_CONFIG} --libs OUTPUT_VARIABLE GDAL_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GDAL_CONFIG} --cflags OUTPUT_VARIABLE GDAL_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions(${GDAL_CFLAGS})

# Setup Vrui
pkg_search_module(VRUI REQUIRED Vrui-2.6 Vrui)
string(REPLACE "-framework;" "-framework " VRUI_LDFLAGS "${VRUI_LDFLAGS}")
add_definitions(${VRUI_CFLAGS})

# Setup GL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

# Setup GLEW
pkg_search_module(GLEW REQUIRED glewmx glew)
add_definitions(${GLEW_CFLAGS})


##-- Setup compile properties

# Add Crusta src directory for includes
include_directories(src)

# Report Crusta version
add_definitions(-DCRUSTA_VERSION=\"${PROJECT_VERSION}\")

# Show all warnings
set_property(GLOBAL APPEND PROPERTY CMAKE_CXX_FLAGS "-Wall")

# Report resource paths to source files
add_definitions(-DCRUSTA_ETC_PATH="${ETC_DIR}")
add_definitions(-DCRUSTA_SHARE_PATH="${SHARE_DIR}")


##-- Install data and configs

install(DIRECTORY share/ DESTINATION ${SHARE_DIR})
install(DIRECTORY etc/ DESTINATION ${ETC_DIR})


#-------------------
# Target specifications
#-------------------

file(GLOB_RECURSE CRUSTACORE_SOURCES src/crustacore/*)
add_library(crustacore STATIC ${CRUSTACORE_SOURCES})

macro(add_crusta_exe)
  add_executable(${ARGV})
  target_link_libraries(${ARGV0} crustacore ${VRUI_LDFLAGS} ${GDAL_LIBS} ${OPENGL_LIBRARY} ${GLEW_LDFLAGS})
  install(TARGETS ${ARGV0} DESTINATION ${BIN_DIR})
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
    COMMAND echo "${BIN_DIR}/crusta" ${ARGN} \"\$@\" >> ${OUTPUT}
    COMMAND chmod +x ${OUTPUT}
    VERBATIM
    DEPENDS crusta
  )
  add_custom_target(create-${OUTPUT} ALL DEPENDS ${OUTPUT})
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT} DESTINATION ${BIN_DIR})
endmacro()

set(desktopargs -mergeConfig ${SHARE_DIR}/vrui/mouse.cfg)
set(slicetoolargs -settings ${SHARE_DIR}/etc/enableSliceTool.cfg)
add_baked_args_exe(crusta-desktop ${desktopargs})
add_baked_args_exe(crusta-slicing ${slicetoolargs})
add_baked_args_exe(crusta-slicing-desktop ${slicetoolargs} ${desktopargs})

