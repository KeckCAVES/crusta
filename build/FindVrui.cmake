#
# Provides
#   VRUI_FOUND
#   VRUI_PATH
#   VRUI_CFLAGS
#   VRUI_LINKFLAGS
#   VRUI_PLUGINFILEEXT
#   VRUI_PLUGINCFLAGS
#   VRUI_PLUGINLINKFLAGS
#   VRUI_PLUGINHOSTLINKFLAGS
#

# Macro to extract the content from the makeinclude
macro( EXTRACT_VRUI_FLAG variable name makeinclude )
    # Get the string corresponding to name
    file( STRINGS ${${makeinclude}} ${variable}
          LIMIT_COUNT 1
          REGEX "^${name} =.*" )

    # Remove the name and leading whitespace
    string( REGEX REPLACE "^${name} =[ \t]*" "" ${variable} ${${variable}} )

    # Remove trailing whitespace
    if (${variable})
        string( REGEX REPLACE "[ \t]+$"          "" ${variable} ${${variable}} )
    endif()

    # Remove -ffriend-injection and -flat_namespace
    if (${variable})
        string(REPLACE "-ffriend-injection" "" ${variable} ${${variable}} )
    endif()
    if (${variable})
        string(REPLACE "-flat_namespace"    "" ${variable} ${${variable}} )
    endif()
endmacro()

# Confirm the vrui path
FIND_PATH(VRUI_PATH
    NAMES etc/Vrui.makeinclude etc/Vrui.debug.makeinclude etc/Vrui.cfg
    PATHS $ENV{VRUI_PATH} /usr /usr/local
    PATH_SUFFIXES Vrui-1.0 VRUI Vrui vrui vrui-one)

if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    find_file( VRUI_MAKEINCLUDE etc/Vrui.debug.makeinclude
               PATHS ${VRUI_PATH} NO_DEFAULT_PATH)
    if( NOT VRUI_MAKEINCLUDE )
        message( WARNING "Debug build type but debug Vrui not found; using non-debug version" )
    endif()
endif()

if( NOT VRUI_MAKEINCLUDE )
    find_file( VRUI_MAKEINCLUDE etc/Vrui.makeinclude
    PATHS ${VRUI_PATH} NO_DEFAULT_PATH)
endif()

if( VRUI_MAKEINCLUDE )
    set( VRUI_FOUND TRUE )

    extract_vrui_flag( VRUI_CFLAGS              "VRUI_CFLAGS"              VRUI_MAKEINCLUDE )
    extract_vrui_flag( VRUI_LINKFLAGS           "VRUI_LINKFLAGS"           VRUI_MAKEINCLUDE )
    extract_vrui_flag( VRUI_PLUGINFILEEXT       "VRUI_PLUGINFILEEXT"       VRUI_MAKEINCLUDE )
    extract_vrui_flag( VRUI_PLUGINCFLAGS        "VRUI_PLUGINCFLAGS"        VRUI_MAKEINCLUDE )
    extract_vrui_flag( VRUI_PLUGINLINKFLAGS     "VRUI_PLUGINLINKFLAGS"     VRUI_MAKEINCLUDE )
    extract_vrui_flag( VRUI_PLUGINHOSTLINKFLAGS "VRUI_PLUGINHOSTLINKFLAGS" VRUI_MAKEINCLUDE )
else()
    set( VRUI_FOUND FALSE )
endif()

if( VRUI_FOUND )
    if( NOT Vrui_FIND_QUIETLY )
        message( STATUS "Found Vrui (via " ${VRUI_MAKEINCLUDE} ")" )
    endif()
else()
    if( Vrui_FIND_REQUIRED )
        message( FATAL_ERROR "Required package Vrui NOT FOUND; try setting its path in a VRUI_PATH environment variable or the CMAKE_PREFIX_PATH" )
    endif()
endif()

