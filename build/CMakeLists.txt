cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0011 NEW)


project(CRUSTA)

if(APPLE)
add_definitions(-D__DARWIN__)
endif(APPLE)

#-------------------
# Include VRUI variables and macros
#-------------------

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

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

#-------------------
# Crusta shared dynamic link library containing the core crusta components
#-------------------

add_library(Crusta SHARED ../source/crusta/CacheRequest.cpp
                          ../source/crusta/ColorTextureSpecs.cpp
                          ../source/crusta/Crusta.cpp
                          ../source/crusta/DemSpecs.cpp
                          ../source/crusta/FocusViewEvaluator.cpp
                          ../source/crusta/FrustumVisibility.cpp
                          ../source/crusta/LodEvaluator.cpp
                          ../source/crusta/Node.cpp
                          ../source/crusta/QuadCache.cpp
                          ../source/crusta/QuadNodeData.cpp
                          ../source/crusta/QuadTerrain.cpp
						  ../source/crusta/Scope.cpp
                          ../source/crusta/simplexnoise1234.cpp
                          ../source/crusta/Spheroid.cpp
                          ../source/crusta/TreeIndex.cpp
                          ../source/crusta/Triacontahedron.cpp
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
# Construo application
#-------------------

find_package(PNG)
add_definitions(${PNG_DEFINITIONS})
include_directories(${PNG_INCLUDE_DIR})

find_package(ImageMagick COMPONENTS Magick++)
include_directories(${ImageMagick_Magick++_INCLUDE_DIRS})

#GDAL
IF(APPLE)
   FIND_LIBRARY(GDAL_LIBRARY GDAL)
ENDIF (APPLE)

set(CONSTRUO_SOURCES ../source/construo/ArcInfoBinaryGridImageFile.cpp
                     ../source/construo/Construo.cpp
					 ../source/construo/Filter.cpp
                     ../source/construo/GdalTransform.cpp
                     ../source/construo/GdalImageFile.cpp
                     ../source/construo/GeoTransform.cpp
                     ../source/construo/ImageCoverage.cpp
                     ../source/construo/ImageTransform.cpp
                     ../source/construo/ImageTransformReader.cpp
                     ../source/construo/PngImageFile.cpp
                     ../source/construo/PpmImageFile.cpp
                     ../source/construo/SphereCoverage.cpp
                     ../source/construo/UtmTransform.cpp
                     ../source/construo/Visualizer.cpp)

if(${ImageMagick_Magick++_FOUND})
list(APPEND CONSTRUO_SOURCES ../source/construo/MagickImageFile.cpp)
add_definitions(-DCONSTRUO_USE_IMAGEMAGICK)
endif(${ImageMagick_Magick++_FOUND})

add_executable(construo ${CONSTRUO_SOURCES})

#cmake fraking sucks if you don't follow their directory structure!!!!
#set_target_properties(construo PROPERTIES
#                               COMPILE_DEFINITIONS "CONSTRUO_BUILD")

vrui_set_target_properties(construo)

target_link_libraries(construo Crusta ${PNG_LIBRARIES}
                               ${ImageMagick_Magick++_LIBRARIES}
                               ${GDAL_LIBRARY})



#-------------------
# Crusta installation
#-------------------

install(TARGETS Crusta
        LIBRARY DESTINATION ${VRUI_LIB_DIR})
install(TARGETS CrustaVislet
        LIBRARY DESTINATION ${VRUI_LIB_DIR}/VRVislets)

#shader files
file(GLOB CRUSTA_SHADERS ../share/*.vs ../share/*.fs ../share/*.gs)
install(FILES ${CRUSTA_SHADERS} DESTINATION ${CRUSTA_SHARE_PATH})
