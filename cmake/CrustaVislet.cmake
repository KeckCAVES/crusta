add_library(CrustaVislet MODULE
    src/crusta/CrustaVislet.h

    src/crusta/CrustaVislet.cpp
)

set_target_properties(CrustaVislet PROPERTIES
    COMPILE_FLAGS "${VRUI_CFLAGS} ${VRUI_PLUGINCFLAGS}"
    LINK_FLAGS ${VRUI_PLUGINLINKFLAGS}
    SUFFIX .${VRUI_PLUGINFILEEXT})
target_link_libraries(CrustaVislet CrustaCore)
