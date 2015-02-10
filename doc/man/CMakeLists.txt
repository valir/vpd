
find_program(A2X_EXECUTABLE NAMES a2x a2x.py)

set(A2X_OPTS
    -v
    -d manpage
    -f manpage)

set(MAN_NAMES vpd)
set(MAN_FILES)
foreach(m IN LISTS MAN_NAMES)
    set(mf ${CMAKE_CURRENT_SOURCE_DIR}/${m})
    set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.txt)
    add_custom_command(OUTPUT ${mf}
        COMMAND ${A2X_EXECUTABLE} ${A2X_OPTS} ${ms}
        DEPENDS ${ms}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building manpage ${mf}"
        VERBATIM)
    list(APPEND MAN_FILES ${mf}.1)
endforeach()

add_custom_target(man ALL DEPENDS ${MAN_FILES})
install(FILES ${MAN_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1)

