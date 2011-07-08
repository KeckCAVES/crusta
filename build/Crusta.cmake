add_executable(crusta ${CORE_FILES}
    ../include/crusta/CrustaApp.h
    ../source/crusta/CrustaApp.cpp
)

set_property(TARGET crusta APPEND PROPERTY COMPILE_FLAGS ${VRUI_CFLAGS})
set_property(TARGET crusta APPEND PROPERTY COMPILE_DEFINITIONS
             GLEW_MX;CRUSTA_VERSION=\"${PROJECT_VERSION}\")

target_link_libraries(crusta ${VRUI_LINKFLAGS} ${GDAL_LIBRARY} ${X11_OPENGL_GLU_LIBRARIES})

install(TARGETS crusta DESTINATION ${BIN_PATH})

install(FILES ../share/mapSymbolAtlas.tga
        DESTINATION ${SHARE_PATH}/images)

install(FILES ../share/mapSymbols.cfg
              ../share/mouse.cfg
              ../share/trackpad.cfg
        DESTINATION ${SHARE_PATH}/config)

install(FILES ../share/decoratedRenderer.fp
        DESTINATION ${SHARE_PATH}/shaders)
