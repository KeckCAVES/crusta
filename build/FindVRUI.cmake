# Set the environment variable VRUI_PATH before running cmake (e.g.
# VRUI_PATH=/usr/local/VRUI).
#
# The script creates the following macros:
#
# VRUI_SET_TARGET_PROPERTIES( target )
# -- Sets the properties required to make a vrui enabled target.
#
# The script defines the following variables:
#
# VRUI_INCLUDE_DIR  -- Include directory for vrui headers.
# VRUI_BIN_DIR      -- 
# VRUI_LIB_DIR      --
#
# VRUI_LIBRARIES    -- VRUI library files

# FindVRUI.cmake
###############################################################################
###############################################################################
# Locate VRUI, dependencies, etc.
###############################################################################
###############################################################################

# Confirm the vrui path
IF(NOT VRUI_PATH)
  MESSAGE(FATAL_ERROR "Specify the VRUI_PATH")
ENDIF(NOT VRUI_PATH)
IF(NOT EXISTS "${VRUI_PATH}/etc/Vrui.cfg")
  MESSAGE(FATAL_ERROR "Can't Find ${VRUI_PATH}/etc/Vrui.cfg")
ENDIF(NOT EXISTS "${VRUI_PATH}/etc/Vrui.cfg")

SET(VRUI_INCLUDE_DIR "${VRUI_PATH}/include")

SET(VRUI_BIN_DIR "${VRUI_PATH}/bin")
IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
  SET(VRUI_LIB_DIR "${VRUI_PATH}/lib")
ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4)
  SET(VRUI_LIB_DIR "${VRUI_PATH}/lib64")
ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  SET(VRUI_LIB_DIR "${VRUI_LIB_DIR}/debug")
  SET(VRUI_BIN_DIR "${VRUI_BIN_DIR}/debug")
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

SET(VRUI_LIB_NAMES Misc Plugins Realtime Comm Math Geometry GLWrappers
                   GLSupport GLXSupport GLGeometry GLMotif Images Sound
                   ALSupport Vrui)
FOREACH(name ${VRUI_LIB_NAMES})
FIND_LIBRARY(A_VRUI_LIB
             NAMES ${name} ${name}.g++ ${name}.g++-3 ${name}.g++-4
             PATHS ${VRUI_LIB_DIR}
             NO_DEFAULT_PATH)
SET(VRUI_LIBRARIES ${VRUI_LIBRARIES} ${A_VRUI_LIB})
UNSET(A_VRUI_LIB CACHE)
ENDFOREACH(name)

# Dependencies
FIND_PACKAGE(X11 REQUIRED)
#under mac os x find_package(opengl) uses the framework version. we need the X11
FIND_PATH(X11_OPENGL_INCLUDE_DIR GL/gl.h)
FIND_LIBRARY(X11_OPENGL_GL_LIBRARIES
             NAMES GL
             PATH_SUFFIXES X11)
FIND_LIBRARY(X11_OPENGL_GLU_LIBRARIES
             NAMES GLU
             PATH_SUFFIXES X11)
SET(X11_OPENGL_LIBRARIES ${X11_OPENGL_GL_LIBRARIES} ${X11_OPENGL_GLU_LIBRARIES})

###############################################################################
###############################################################################
# VRUI SET TARGET PROPERTIES
###############################################################################
###############################################################################
MACRO(VRUI_SET_TARGET_PROPERTIES target)

  TARGET_LINK_LIBRARIES(${target} ${X11_LIBRARIES} ${X11_OPENGL_GL_LIBRARIES}
                                  ${VRUI_LIBRARIES})

  SET_TARGET_PROPERTIES(${target} PROPERTIES
                        COMPILE_DEFINITIONS "GL_GLEXT_PROTOTYPES"
                        COMPILE_FLAGS "${COMPILE_FLAGS} -I${X11_OPENGL_INCLUDE_DIR} -I${VRUI_INCLUDE_DIR}")

ENDMACRO(VRUI_SET_TARGET_PROPERTIES target)


############
MARK_AS_ADVANCED(VRUI_INCLUDE_DIR VRUI_BIN_DIR VRUI_LIB_DIR VRUI_LIBRARIES)
