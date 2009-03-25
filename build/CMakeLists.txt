cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0011 NEW)


project(CRUSTA)


#-------------------
# Include VRUI variables and macros
#-------------------

set(VRUI_PATH /Users/saru/dev/lib/vrui)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR} )
#find_package(VRUI)
include(${CMAKE_SOURCE_DIR}/FindVRUI.cmake)

#-------------------
# Target independent configuration
#-------------------

set(CRUSTA_SHARE_PATH "${VRUI_SHARE_DIR}/Crusta")
add_definitions(-DCRUSTA_SHARE_PATH=\"${CRUSTA_SHARE_PATH}\")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../lib)
include_directories(../include)

#list(APPEND CMAKE_CXX_FLAGS_DEBUG -Wall)
#list(APPEND CMAKE_CXX_FLAGS_RELEASE -Wall)

#-------------------
# Crusta shared dynamic link library containing the core crusta components
#-------------------

add_library(Crusta SHARED ../source/crusta/Crusta.cpp
                          ../source/crusta/FocusViewEvaluator.cpp
                          ../source/crusta/FrustumVisibility.cpp
                          ../source/crusta/LodEvaluator.cpp
                          ../source/crusta/QuadCache.cpp
                          ../source/crusta/QuadTerrain.cpp
                          ../source/crusta/simplexnoise1234.cpp
                          ../source/crusta/Spheroid.cpp
                          ../source/crusta/ViewLod.cpp)

vrui_set_target_properties(Crusta)


#-------------------
# Crusta application wrapping the core components into an executable
#-------------------

add_executable(crustaApp ../source/crusta/CrustaApp.cpp)

vrui_set_target_properties(crustaApp)
target_link_libraries(crustaApp Crusta)


#-------------------
# Crusta vislet module wrapping the core components into a vislet
#-------------------

add_library(CrustaVislet MODULE ../source/crusta/CrustaVislet.cpp)

vrui_set_target_properties(CrustaVislet)
target_link_libraries(CrustaVislet Crusta)

if(APPLE)
set_target_properties(CrustaVislet PROPERTIES
                      SUFFIX ".bundle")
endif(APPLE)




#-------------------
# Crusta installation
#-------------------

install(TARGETS Crusta CrustaVislet
        LIBRARY DESTINATION ${VRUI_LIB_DIR})

#shader files
file(GLOB CRUSTA_SHADERS ../share/*.vs ../share/*.fs ../share/*.gs)
install(FILES ${CRUSTA_SHADERS} DESTINATION ${CRUSTA_SHARE_PATH})
