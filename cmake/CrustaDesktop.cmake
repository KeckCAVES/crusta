set(CRUSTA_DESKTOP ${CMAKE_CURRENT_BINARY_DIR}/crustaDesktop)

add_custom_command(
    OUTPUT ${CRUSTA_DESKTOP}
    COMMAND echo "#!/bin/sh\\ncrusta -mergeConfig ${CRUSTA_SHARE_PATH}/desktop.cfg \$@" > ${CRUSTA_DESKTOP}
    COMMAND chmod +x ${CRUSTA_DESKTOP}
    VERBATIM
)

add_custom_target(produceCrustaDesktop DEPENDS ${CRUSTA_DESKTOP})
add_dependencies(crusta produceCrustaDesktop)
