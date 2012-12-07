macro(add_desktop_exe NAME)
  set(OUTPUT ${NAME}-desktop)
  add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND echo "#!/bin/sh" > ${OUTPUT}
    COMMAND echo "${BIN_PATH}/${NAME} -mergeConfig ${SHARE_PATH}/config/mouse.cfg \"\$@\"" >> ${OUTPUT}
    COMMAND chmod +x ${OUTPUT}
    VERBATIM
    DEPENDS ${NAME}
  )
  add_custom_target(create-${OUTPUT} ALL DEPENDS ${OUTPUT})
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT} DESTINATION ${BIN_PATH})
endmacro()

add_desktop_exe(crusta)
if(CRUSTA_SLICING)
  add_desktop_exe(crusta-slicing)
endif()
