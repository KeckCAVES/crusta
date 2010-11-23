add_executable(crusta
    ../include/crusta/CrustaApp.h

    ../source/crusta/CrustaApp.cpp
)

set_target_properties(crusta PROPERTIES
    COMPILE_FLAGS ${VRUI_CFLAGS})
target_link_libraries(crusta CrustaCore)
