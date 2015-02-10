

set(A2X_OPTS
    -D ${CMAKE_BINARY_DIR}/web
    -d article
    --icons
    -f xhtml)

set(WEB_PAGES vpd.txt)
set(WEB_FILES)
foreach(m in LISTS WEB_PAGES)
    set(mf ${CMAKE_BINARY_DIR}/${m})
    set(ms ${CMAKE_SOURCE_DIR}/${m}.txt)
    add_custom_command(OUTPUT ${mf}
        COMMAND ${A2X_EXECUTABLE} ${A2X_OPTS} ${ms}
        DEPENDS ${ms}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building manpage ${mf}"
        VERBATIM)
    list(APPEND WEB_FILES ${mf})
endforeach()

add_custom_target(web ALL DEPENDS ${WEB_FILES})


