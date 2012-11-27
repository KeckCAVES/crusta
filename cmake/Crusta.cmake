set(CRUSTA_SOURCES
    include/crusta/QuadNodeData.h
    include/crusta/QuadCache.hpp
    include/crusta/LightingShader.h
    include/crusta/FocusViewEvaluator.h
    include/crusta/FrustumVisibility.h
    include/crusta/map/MapTool.h
    include/crusta/map/Polyline.h
    include/crusta/map/PolylineTool.h
    include/crusta/map/Shape.h
    include/crusta/shader/ShaderAtlasDataSource.h
    include/crusta/shader/ShaderColorMapper.h
    include/crusta/shader/ShaderColorMixer.h
    include/crusta/shader/ShaderColorMultiplier.h
    include/crusta/shader/ShaderColorReader.h
    include/crusta/shader/ShaderDataSource.h
    include/crusta/shader/ShaderDecoratedLineRenderer.h
    include/crusta/shader/ShaderFileFragment.h
    include/crusta/shader/ShaderFragment.h
    include/crusta/shader/ShaderMultiDataSource.h
    include/crusta/shader/ShaderTopographySource.h
    include/crusta/DebugTool.h
    include/crusta/LodEvaluator.h
    
    source/crusta/ViewLod.cpp
    source/crusta/FrustumVisibility.cpp
    source/crusta/FocusViewEvaluator.cpp
    source/crusta/shader/ShaderAtlasDataSource.cpp
    source/crusta/shader/ShaderColorMixer.cpp
    source/crusta/shader/ShaderColorMultiplier.cpp
    source/crusta/shader/ShaderColorReader.cpp
    source/crusta/shader/ShaderDataSource.cpp
    source/crusta/shader/ShaderDecoratedLineRenderer.cpp
    source/crusta/shader/ShaderFileFragment.cpp
    source/crusta/shader/ShaderFragment.cpp
    source/crusta/shader/ShaderMultiDataSource.cpp
    source/crusta/shader/ShaderTopographySource.cpp
    source/crusta/LodEvaluator.cpp
    source/crusta/StatsManager.cpp
    source/crusta/SurfacePoint.cpp
)

# Sources that have different code paths depending on
# whether the slicing tool is enabled or not.
set(CRUSTA_HYBRID_SOURCES
    include/crusta/CrustaApp.h
    include/crusta/ColorMapper.h
    include/crusta/Crusta.h
    include/crusta/DataManager.h
    include/crusta/QuadCache.h
    include/crusta/QuadNodeDataBundles.h
    include/crusta/QuadTerrain.h
    include/crusta/StatsManager.h
    include/crusta/SurfaceApproximation.h
    include/crusta/map/MapManager.h
    include/crusta/map/PolylineRenderer.h
    
    source/crusta/ColorMapper.cpp
    source/crusta/Crusta.cpp
    source/crusta/CrustaApp.cpp
    source/crusta/DataManager.cpp
    source/crusta/DebugTool.cpp
    source/crusta/LightingShader.cpp
    source/crusta/QuadCache.cpp
    source/crusta/QuadNodeData.cpp
    source/crusta/QuadNodeDataBundles.cpp
    source/crusta/QuadTerrain.cpp
    source/crusta/StatsManager.cpp
    source/crusta/SurfaceApproximation.cpp
    source/crusta/SurfaceProbeTool.cpp
    source/crusta/SurfaceProjector.cpp
    source/crusta/SurfaceTool.cpp
    source/crusta/map/MapManager.cpp
    source/crusta/map/MapTool.cpp
    source/crusta/map/Polyline.cpp
    source/crusta/map/PolylineRenderer.cpp
    source/crusta/map/PolylineTool.cpp
    source/crusta/map/Shape.cpp
    source/crusta/shader/ShaderColorMapper.cpp
)

# Sources applicable only to the slicing tool.
set(CRUSTA_SLICING_SOURCES
    include/crusta/SliceTool.h
    source/crusta/SliceTool.cpp
)

add_library(crustacommon STATIC ${CRUSTA_SOURCES})

add_crusta_exe(crusta ${CRUSTA_HYBRID_SOURCES})
target_link_libraries(crusta crustacommon)

if(CRUSTA_SLICING)
    message(STATUS "Enabled building crusta-slicing")
    add_crusta_exe(crusta-slicing ${CRUSTA_HYBRID_SOURCES} ${CRUSTA_SLICING_SOURCES})
    set_property(TARGET crusta-slicing APPEND PROPERTY COMPILE_DEFINITIONS CRUSTA_SLICING)
    target_link_libraries(crusta-slicing crustacommon)
endif()

install(FILES share/mapSymbolAtlas.tga
        DESTINATION ${SHARE_PATH}/images)

install(FILES share/mapSymbols.cfg
              share/mouse.cfg
              share/trackpad.cfg
        DESTINATION ${SHARE_PATH}/config)

install(FILES share/decoratedRenderer.fp
        DESTINATION ${SHARE_PATH}/shaders)
