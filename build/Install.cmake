install(TARGETS CrustaCore  crusta construo CrustaVislet
        RUNTIME DESTINATION "${STAGE_EXE_PATH}"
        LIBRARY DESTINATION "${STAGE_LIB_PATH}"
        ARCHIVE DESTINATION "${STAGE_LIB_PATH}"
)

install(PROGRAMS ${CRUSTA_DESKTOP}
        DESTINATION ${STAGE_EXE_PATH})

set(CRUSTA_SHARES ../share/mapSymbolAtlas.tga
                  ../share/mapSymbols.cfg
                  ../share/decoratedRenderer.fp
                  ../share/plainRenderer.fp
                  ../share/desktop.cfg
)
install(FILES ${CRUSTA_SHARES}
        DESTINATION ${STAGE_SHARE_PATH})

