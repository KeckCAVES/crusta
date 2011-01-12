install(TARGETS CrustaCore  crusta construo CrustaVislet
        RUNTIME DESTINATION "${CRUSTA_EXECUTABLE_PATH}"
        LIBRARY DESTINATION "${CRUSTA_LIBRARY_PATH}"
        ARCHIVE DESTINATION "${CRUSTA_LIBRARY_PATH}"
)

set(CRUSTA_SHARES ../share/mapSymbolAtlas.tga
                  ../share/mapSymbols.cfg
                  ../share/decoratedRenderer.fp
                  ../share/plainRenderer.fp
                  ../share/desktop.cfg
)
install(FILES ${CRUSTA_SHARES} DESTINATION ${CRUSTA_SHARE_PATH})
