cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0011 NEW)


project(CRUSTA)

if(APPLE)
add_definitions(-D__DARWIN__)
endif(APPLE)

#-------------------
# Include VRUI variables and macros
#-------------------
find_package(VRUI REQUIRED HINTS ${CMAKE_SOURCE_DIR})

#-------------------
# Target independent configuration
#-------------------
set(CRUSTA_SHARE_PATH "${CMAKE_INSTALL_PREFIX}/share/crusta")
add_definitions(-DCRUSTA_SHARE_PATH=\"${CRUSTA_SHARE_PATH}\")
include_directories(../include)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

#-------------------
# Crusta shared dynamic link library containing the core crusta components
#-------------------

add_library(Crusta SHARED
    ../include/crusta/basics.h
    ../include/crusta/CacheRequest.h
    ../include/crusta/checkGl.h
    ../include/crusta/ColorTextureSpecs.h
    ../include/crusta/CompositeShader.h
    ../include/crusta/Crusta.h
    ../include/crusta/CrustaComponent.h
    ../include/crusta/DataManager.h
    ../include/crusta/DemSpecs.h
    ../include/crusta/Ellipsoid.h
    ../include/crusta/FocusViewEvaluator.h
    ../include/crusta/FrustumVisibility.h
    ../include/crusta/LightingShader.h
    ../include/crusta/LodEvaluator.h
    ../include/crusta/map/MapManager.h
    ../include/crusta/map/MapTool.h
    ../include/crusta/map/Polyline.h
    ../include/crusta/map/PolylineRenderer.h
    ../include/crusta/map/PolylineTool.h
    ../include/crusta/map/Shape.h
    ../include/crusta/PixelSpecs.h
    ../include/crusta/Polyhedron.h
    ../include/crusta/QuadCache.h
    ../include/crusta/QuadCache.hpp
    ../include/crusta/QuadNodeData.h
    ../include/crusta/QuadNodeData.hpp
    ../include/crusta/QuadTerrain.h
    ../include/crusta/QuadtreeFile.h
    ../include/crusta/QuadtreeFile.hpp
    ../include/crusta/QuadtreeFileHeaders.h
    ../include/crusta/QuadtreeFileSpecs.h
    ../include/crusta/Refinement.h
    ../include/crusta/Refinement.hpp
    ../include/crusta/Scope.h
    ../include/crusta/Scope.hpp
    ../include/crusta/SurfaceTool.h
    ../include/crusta/Tool.h
    ../include/crusta/TreeIndex.h
    ../include/crusta/Triacontahedron.h
    ../include/crusta/ViewLod.h
    ../include/crusta/VisibilityEvaluator.h

    ../source/crusta/CacheRequest.cpp
    ../source/crusta/ColorTextureSpecs.cpp
    ../source/crusta/CompositeShader.cpp
    ../source/crusta/Crusta.cpp
    ../source/crusta/DataManager.cpp
    ../source/crusta/DemSpecs.cpp
    ../source/crusta/FocusViewEvaluator.cpp
    ../source/crusta/FrustumVisibility.cpp
    ../source/crusta/LightingShader.cpp
    ../source/crusta/LodEvaluator.cpp
    ../source/crusta/map/MapManager.cpp
    ../source/crusta/map/MapTool.cpp
    ../source/crusta/map/Polyline.cpp
    ../source/crusta/map/PolylineRenderer.cpp
    ../source/crusta/map/PolylineTool.cpp
    ../source/crusta/map/Shape.cpp
    ../source/crusta/QuadCache.cpp
    ../source/crusta/QuadNodeData.cpp
    ../source/crusta/QuadTerrain.cpp
    ../source/crusta/Scope.cpp
    ../source/crusta/SurfaceTool.cpp
    ../source/crusta/Tool.cpp
    ../source/crusta/TreeIndex.cpp
    ../source/crusta/Triacontahedron.cpp
    ../source/crusta/ViewLod.cpp
)

vrui_set_target_properties(Crusta)

#-------------------
# Crusta application wrapping the core components into an executable
#-------------------

add_executable(crustaApp
    ../include/crusta/CrustaApp.h

    ../source/crusta/CrustaApp.cpp
)

vrui_set_target_properties(crustaApp)
target_link_libraries(crustaApp Crusta)


#-------------------
# Crusta vislet module wrapping the core components into a vislet
#-------------------

add_library(CrustaVislet MODULE
    ../include/crusta/CrustaVislet.h

    ../source/crusta/CrustaVislet.cpp
)

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
if(APPLE)
   find_library(GDAL_LIBRARY GDAL)
else(APPLE)
   find_package(GDAL)
   include_directories(${GDAL_INCLUDE_DIR})
endif(APPLE)

set(CONSTRUO_SOURCES
    ../include/construo/ArcInfoBinaryGridImageFile.h
    ../include/construo/Builder.h
    ../include/construo/Builder.hpp
    ../include/construo/Converters.h
    ../include/construo/DynamicFilter.h
    ../include/construo/Filter.h
    ../include/construo/GdalImageFile.h
    ../include/construo/GdalImageFile.hpp
    ../include/construo/GdalTransform.h
    ../include/construo/GeometryTypes.h
    ../include/construo/GeoTransform.h
    ../include/construo/ImageCoverage.h
    ../include/construo/ImageFile.h
    ../include/construo/ImageFile.hpp
    ../include/construo/ImageFileLoader.h
    ../include/construo/ImageFileLoader.hpp
    ../include/construo/ImagePatch.h
    ../include/construo/ImagePatch.hpp
    ../include/construo/ImageTransform.h
    ../include/construo/ImageTransformReader.h
    ../include/construo/MagickImageFile.h
    ../include/construo/PixelHelpers.h
    ../include/construo/PngImageFile.h
    ../include/construo/PpmImageFile.h
    ../include/construo/ReiImageFile.h
    ../include/construo/Sampler.h
    ../include/construo/SphereCoverage.h
    ../include/construo/TpmFile.h
    ../include/construo/TpmImageFile.h
    ../include/construo/Tree.h
    ../include/construo/Tree.hpp
    ../include/construo/UtmTransform.h
    ../include/construo/Visualizer.h

    ../source/construo/ArcInfoBinaryGridImageFile.cpp
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
    ../source/construo/TpmFile.cpp
    ../source/construo/TpmImageFile.cpp
    ../source/construo/UtmTransform.cpp
    ../source/construo/Visualizer.cpp
)

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
        LIBRARY DESTINATION lib)
install(TARGETS crustaApp
        RUNTIME DESTINATION bin)
install(TARGETS CrustaVislet
        LIBRARY DESTINATION lib)

#shader files
file(GLOB CRUSTA_SHADERS ../share/*.vs ../share/*.fs ../share/*.gs)
install(FILES ${CRUSTA_SHADERS} DESTINATION ${CRUSTA_SHARE_PATH})
