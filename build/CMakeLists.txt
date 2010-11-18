cmake_minimum_required(VERSION 2.6)
#cmake_policy(SET CMP0011 NEW)


project(CRUSTA)

#-------------------
# Include VRUI variables and macros
#-------------------
find_package(VRUI REQUIRED HINTS ${CMAKE_SOURCE_DIR})


#-------------------
# Target independent configuration
#-------------------
set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}"
    CACHE INTERNAL "CMAKE_INSTALL_PREFIX")

set(CRUSTA_EXECUTABLE_PATH "${CMAKE_INSTALL_PREFIX}/bin"
    CACHE PATH "Crusta install executable path")
set(CRUSTA_LIBRARY_PATH "${CMAKE_INSTALL_PREFIX}/lib"
    CACHE PATH "Crusta install library path")
set(CRUSTA_SHARE_PATH "${CMAKE_INSTALL_PREFIX}/share/crusta"
    CACHE PATH "Crusta install share path")
add_definitions(-DCRUSTA_SHARE_PATH=\"${CRUSTA_SHARE_PATH}\" -DGLEW_MX)

include_directories(${VRUI_INCLUDE_DIR})
include_directories(../include)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CRUSTA_LIBRARY_PATH}")
set(CMAKE_INSTALL_NAME_DIR "${CRUSTA_LIBRARY_PATH}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#GDAL
if(APPLE)
   find_library(GDAL_LIBRARY GDAL)
else(APPLE)
   find_package(GDAL)
   include_directories(${GDAL_INCLUDE_DIR})
endif(APPLE)


#-------------------
# Crusta shared dynamic link library containing the core crusta components
#-------------------

add_library(CrustaCore SHARED
    ../include/crusta/basics.h
    ../include/crusta/Cache.h
    ../include/crusta/Cache.hpp
    ../include/crusta/checkGl.h
    ../include/crusta/Crusta.h
    ../include/crusta/CrustaComponent.h
    ../include/crusta/CrustaSettings.h
    ../include/crusta/CrustaVisualizer.h
    ../include/crusta/DataManager.h
    ../include/crusta/DemHeight.h
    ../include/crusta/DemHeightGlobeData.h
    ../include/crusta/DemHeightPixelOps.h
    ../include/crusta/DebugTool.h
    ../include/crusta/ElevationRangeTool.h
    ../include/crusta/FocusViewEvaluator.h
    ../include/crusta/FrustumVisibility.h
    ../include/crusta/GlobeData.h
    ../include/crusta/GlobeFile.h
    ../include/crusta/GlobeFile.hpp
    ../include/crusta/Homography.h
    ../include/crusta/IdGenerator.h
    ../include/crusta/LightingShader.h
    ../include/crusta/LodEvaluator.h
    ../include/crusta/map/MapManager.h
    ../include/crusta/map/MapTool.h
    ../include/crusta/map/Polyline.h
    ../include/crusta/map/PolylineRenderer.h
    ../include/crusta/map/PolylineTool.h
    ../include/crusta/map/Shape.h
    ../include/crusta/PixelOps.h
    ../include/crusta/Polyhedron.h
    ../include/crusta/PolyhedronLoader.h
    ../include/crusta/QuadCache.h
    ../include/crusta/QuadCache.hpp
    ../include/crusta/QuadNodeData.h
    ../include/crusta/QuadNodeDataBundles.h
    ../include/crusta/QuadTerrain.h
    ../include/crusta/QuadtreeFile.h
    ../include/crusta/QuadtreeFile.hpp
    ../include/crusta/Refinement.h
    ../include/crusta/Refinement.hpp
    ../include/crusta/Scope.h
    ../include/crusta/Scope.hpp
    ../include/crusta/Section.h
    ../include/crusta/Sphere.h
    ../include/crusta/StatsManager.h
    ../include/crusta/SurfaceApproximation.h
    ../include/crusta/SurfaceTool.h
    ../include/crusta/TextureColor.h
    ../include/crusta/TextureColorGlobeData.h
    ../include/crusta/TextureColorPixelOps.h
    ../include/crusta/TileIndex.h
    ../include/crusta/Timer.h
    ../include/crusta/Tool.h
    ../include/crusta/TreeIndex.h
    ../include/crusta/Triacontahedron.h
    ../include/crusta/Triangle.h
    ../include/crusta/ViewLod.h
    ../include/crusta/VisibilityEvaluator.h
    ../include/crusta/Visualizer.h
    ../include/GL/glew.h
    ../include/GL/GlProgram.h
    ../include/GL/glxew.h
    ../include/GL/VruiGlew.h
    ../include/GLMotif/ColorHexagon.h
    ../include/GLMotif/ColorMap.h
    ../include/GLMotif/ColorPicker.h
    ../include/GLMotif/ColorPickerWindow.h
    ../include/GLMotif/FileAndFolderSelectionDialog.h
    ../include/GLMotif/PaletteEditor.h

    ../source/crusta/Crusta.cpp
    ../source/crusta/CrustaSettings.cpp
    ../source/crusta/CrustaVisualizer.cpp
    ../source/crusta/DataManager.cpp
    ../source/crusta/DebugTool.cpp
    ../source/crusta/ElevationRangeTool.cpp
    ../source/crusta/FocusViewEvaluator.cpp
    ../source/crusta/FrustumVisibility.cpp
    ../source/crusta/Homography.cpp
    ../source/crusta/LightingShader.cpp
    ../source/crusta/LodEvaluator.cpp
    ../source/crusta/map/MapManager.cpp
    ../source/crusta/map/MapTool.cpp
    ../source/crusta/map/Polyline.cpp
    ../source/crusta/map/PolylineRenderer.cpp
    ../source/crusta/map/PolylineTool.cpp
    ../source/crusta/map/Shape.cpp
    ../source/crusta/PolyhedronLoader.cpp
    ../source/crusta/QuadCache.cpp
    ../source/crusta/QuadNodeData.cpp
    ../source/crusta/QuadNodeDataBundles.cpp
    ../source/crusta/QuadTerrain.cpp
    ../source/crusta/Scope.cpp
    ../source/crusta/Section.cpp
    ../source/crusta/Sphere.cpp
    ../source/crusta/StatsManager.cpp
    ../source/crusta/SurfaceApproximation.cpp
    ../source/crusta/SurfaceTool.cpp
    ../source/crusta/Tool.cpp
    ../source/crusta/TreeIndex.cpp
    ../source/crusta/Triacontahedron.cpp
    ../source/crusta/ViewLod.cpp
    ../source/crusta/Visualizer.cpp
    ../source/GL/glew.c
    ../source/GL/GlProgram.cpp
    ../source/GL/VruiGlew.cpp
    ../source/GLMotif/ColorHexagon.cpp
    ../source/GLMotif/ColorMap.cpp
    ../source/GLMotif/ColorPicker.cpp
    ../source/GLMotif/ColorPickerWindow.cpp
    ../source/GLMotif/FileAndFolderSelectionDialog.cpp
    ../source/GLMotif/PaletteEditor.cpp
)

vrui_set_target_properties(CrustaCore)
target_link_libraries(CrustaCore ${GDAL_LIBRARY})

#-------------------
# Crusta application wrapping the core components into an executable
#-------------------

add_executable(crusta
    ../include/crusta/CrustaApp.h

    ../source/crusta/CrustaApp.cpp
)

vrui_set_target_properties(crusta)
target_link_libraries(crusta CrustaCore)


#-------------------
# Crusta vislet module wrapping the core components into a vislet
#-------------------

add_library(CrustaVislet MODULE
    ../include/crusta/CrustaVislet.h

    ../source/crusta/CrustaVislet.cpp
)

vrui_set_target_properties(CrustaVislet)
target_link_libraries(CrustaVislet CrustaCore)

if(APPLE)
set_target_properties(CrustaVislet PROPERTIES
                      SUFFIX ".bundle")
endif(APPLE)


#-------------------
# Construo application
#-------------------

add_executable(construo
    ../include/construo/Builder.h
    ../include/construo/Builder.hpp
    ../include/construo/construoGlobals.h
    ../include/construo/ConstruoSettings.h
    ../include/construo/ConstruoVisualizer.h
    ../include/construo/Converters.h
    ../include/construo/DemHeightSubsampleFilter.h
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
    ../include/construo/SphereCoverage.h
    ../include/construo/SubsampleFilter.h
    ../include/construo/TextureColorSubsampleFilter.h
    ../include/construo/TpmFile.h
    ../include/construo/TpmImageFile.h
    ../include/construo/TpmImageFile.hpp
    ../include/construo/Tree.h
    ../include/construo/Tree.hpp

    ../source/construo/Construo.cpp
    ../source/construo/construoGlobals.cpp
    ../source/construo/ConstruoSettings.cpp
    ../source/construo/ConstruoVisualizer.cpp
    ../source/construo/GdalImageFile.cpp
    ../source/construo/GdalTransform.cpp
    ../source/construo/GeoTransform.cpp
    ../source/construo/ImageCoverage.cpp
    ../source/construo/ImageTransform.cpp
    ../source/construo/SphereCoverage.cpp
    ../source/construo/TpmFile.cpp
    ../source/construo/Tree.cpp
)

#cmake fraking sucks if you don't follow their directory structure!!!!
#set_target_properties(construo PROPERTIES
#                               COMPILE_DEFINITIONS "CONSTRUO_BUILD")

vrui_set_target_properties(construo)

target_link_libraries(construo CrustaCore ${GDAL_LIBRARY})

#-------------------
# Crusta installation
#-------------------

install(TARGETS CrustaCore  crusta construo CrustaVislet
        RUNTIME DESTINATION "${CRUSTA_EXECUTABLE_PATH}"
        LIBRARY DESTINATION "${CRUSTA_LIBRARY_PATH}"
        ARCHIVE DESTINATION "${CRUSTA_LIBRARY_PATH}"
)

set(CRUSTA_SHARES ../share/mapSymbolAtlas.tga
                  ../share/mapSymbols.cfg
                  ../share/decoratedRenderer.fp
                  ../share/plainRenderer.fp
)
install(FILES ${CRUSTA_SHARES} DESTINATION ${CRUSTA_SHARE_PATH})
