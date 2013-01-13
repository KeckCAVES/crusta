set(CRUSTA_SOURCES
    src/crusta/CrustaApp.h
    src/crusta/ColorMapper.h
    src/crusta/Crusta.h
    src/crusta/DataManager.h
    src/crusta/QuadCache.h
    src/crusta/QuadNodeDataBundles.h
    src/crusta/QuadTerrain.h
    src/crusta/StatsManager.h
    src/crusta/SurfaceApproximation.h
    src/crusta/map/MapManager.h
    src/crusta/map/PolylineRenderer.h
    src/crusta/QuadNodeData.h
    src/crusta/QuadCache.hpp
    src/crusta/LightingShader.h
    src/crusta/FocusViewEvaluator.h
    src/crusta/FrustumVisibility.h
    src/crusta/map/MapTool.h
    src/crusta/map/Polyline.h
    src/crusta/map/PolylineTool.h
    src/crusta/map/Shape.h
    src/crusta/shader/ShaderAtlasDataSource.h
    src/crusta/shader/ShaderColorMapper.h
    src/crusta/shader/ShaderColorMixer.h
    src/crusta/shader/ShaderColorMultiplier.h
    src/crusta/shader/ShaderColorReader.h
    src/crusta/shader/ShaderDataSource.h
    src/crusta/shader/ShaderDecoratedLineRenderer.h
    src/crusta/shader/ShaderFileFragment.h
    src/crusta/shader/ShaderFragment.h
    src/crusta/shader/ShaderMultiDataSource.h
    src/crusta/shader/ShaderTopographySource.h
    src/crusta/DebugTool.h
    src/crusta/LodEvaluator.h
    src/crusta/SliceTool.h
    
    src/crusta/ColorMapper.cpp
    src/crusta/Crusta.cpp
    src/crusta/CrustaApp.cpp
    src/crusta/DataManager.cpp
    src/crusta/DebugTool.cpp
    src/crusta/LightingShader.cpp
    src/crusta/QuadCache.cpp
    src/crusta/QuadNodeData.cpp
    src/crusta/QuadNodeDataBundles.cpp
    src/crusta/QuadTerrain.cpp
    src/crusta/StatsManager.cpp
    src/crusta/SurfaceApproximation.cpp
    src/crusta/SurfaceProbeTool.cpp
    src/crusta/SurfaceProjector.cpp
    src/crusta/SurfaceTool.cpp
    src/crusta/map/MapManager.cpp
    src/crusta/map/MapTool.cpp
    src/crusta/map/Polyline.cpp
    src/crusta/map/PolylineRenderer.cpp
    src/crusta/map/PolylineTool.cpp
    src/crusta/map/Shape.cpp
    src/crusta/shader/ShaderColorMapper.cpp
    src/crusta/ViewLod.cpp
    src/crusta/FrustumVisibility.cpp
    src/crusta/FocusViewEvaluator.cpp
    src/crusta/shader/ShaderAtlasDataSource.cpp
    src/crusta/shader/ShaderColorMixer.cpp
    src/crusta/shader/ShaderColorMultiplier.cpp
    src/crusta/shader/ShaderColorReader.cpp
    src/crusta/shader/ShaderDataSource.cpp
    src/crusta/shader/ShaderDecoratedLineRenderer.cpp
    src/crusta/shader/ShaderFileFragment.cpp
    src/crusta/shader/ShaderFragment.cpp
    src/crusta/shader/ShaderMultiDataSource.cpp
    src/crusta/shader/ShaderTopographySource.cpp
    src/crusta/LodEvaluator.cpp
    src/crusta/StatsManager.cpp
    src/crusta/SurfacePoint.cpp
    src/crusta/SliceTool.cpp
)

add_crusta_exe(crusta ${CRUSTA_SOURCES})

install(DIRECTORY share/ DESTINATION ${SHARE_PATH})
install(DIRECTORY etc/ DESTINATION ${ETC_PATH})

macro(add_baked_args_exe NAME)
  set(OUTPUT ${NAME})
  add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND echo "#!/bin/sh" > ${OUTPUT}
    COMMAND echo "${BIN_PATH}/crusta" ${ARGN} \"\$@\" >> ${OUTPUT}
    COMMAND chmod +x ${OUTPUT}
    VERBATIM
    DEPENDS crusta
  )
  add_custom_target(create-${OUTPUT} ALL DEPENDS ${OUTPUT})
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT} DESTINATION ${BIN_PATH})
endmacro()

set(desktopargs -mergeConfig ${SHARE_PATH}/vrui/mouse.cfg)
set(slicetoolargs -settings ${SHARE_PATH}/etc/enableSliceTool.cfg)
add_baked_args_exe(crusta-desktop ${desktopargs})
add_baked_args_exe(crusta-slicing ${slicetoolargs})
add_baked_args_exe(crusta-slicing-desktop ${slicetoolargs} ${desktopargs})
