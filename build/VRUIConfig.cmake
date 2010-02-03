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

SET(VRUI_USE_DEBUG FALSE CACHE BOOL "Use debug Vrui library")

# Confirm the vrui path
FIND_PATH(VRUI_PATH etc/Vrui.cfg
    HINTS $ENV{VRUI_PATH}
    PATH_SUFFIXES Vrui-1.0 VRUI Vrui vrui)

IF(NOT VRUI_PATH)
    MESSAGE(FATAL_ERROR "Could not resolve the path to VRUI")
ENDIF(NOT VRUI_PATH)

SET(VRUI_SHARE_DIR   "${VRUI_PATH}/share")
SET(VRUI_INCLUDE_DIR "${VRUI_PATH}/include")
SET(VRUI_BIN_DIR     "${VRUI_PATH}/bin")

IF(IS_DIRECTORY "${VRUI_PATH}/lib64")
  SET(VRUI_LIB_DIR "${VRUI_PATH}/lib64")
ELSE(IS_DIRECTORY "${VRUI_PATH}/lib64")
  SET(VRUI_LIB_DIR "${VRUI_PATH}/lib")
ENDIF(IS_DIRECTORY "${VRUI_PATH}/lib64")

IF(VRUI_USE_DEBUG)
  SET(VRUI_BIN_DIR "${VRUI_BIN_DIR}/debug")
  SET(VRUI_LIB_DIR "${VRUI_LIB_DIR}/debug")
ENDIF(VRUI_USE_DEBUG)

SET(VRUI_LIB_NAMES Misc Plugins Realtime Comm Math Geometry GLWrappers
                   GLSupport GLXSupport GLGeometry GLMotif Images Sound
                   ALSupport Vrui)
FOREACH(name ${VRUI_LIB_NAMES})
  FIND_LIBRARY(A_VRUI_LIB
               NAMES ${name} ${name}.g++ ${name}.g++-3 ${name}.g++-4
               PATHS ${VRUI_LIB_DIR}
               NO_DEFAULT_PATH)
  IF(A_VRUI_LIB)
    SET(VRUI_LIBRARIES ${VRUI_LIBRARIES} ${A_VRUI_LIB})
    UNSET(A_VRUI_LIB CACHE)
  ELSE(A_VRUI_LIB)
    MESSAGE(FATAL_ERROR "Can't find " ${name} " vrui library.")
  ENDIF(A_VRUI_LIB)
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

  TARGET_LINK_LIBRARIES(${target} ${X11_LIBRARIES} ${X11_OPENGL_LIBRARIES}
                                  ${VRUI_LIBRARIES})

  IF(APPLE)
    SET(VRUI_COMPILE_DEFINITIONS "-D__DARWIN__ -DGL_GLEXT_PROTOTYPES")
  ELSE(APPLE)
    SET(VRUI_COMPILE_DEFINITIONS "-DGL_GLEXT_PROTOTYPES")
  ENDIF(APPLE)

  SET_TARGET_PROPERTIES(${target} PROPERTIES
                        COMPILE_FLAGS "${COMPILE_FLAGS} ${VRUI_COMPILE_DEFINITIONS} -I${X11_OPENGL_INCLUDE_DIR} -I${VRUI_INCLUDE_DIR}"
                        INSTALL_RPATH "${INSTALL_RPATH};.;${VRUI_LIB_DIR}")

ENDMACRO(VRUI_SET_TARGET_PROPERTIES target)


############
MARK_AS_ADVANCED(VRUI_INCLUDE_DIR VRUI_BIN_DIR VRUI_LIB_DIR VRUI_LIBRARIES)
