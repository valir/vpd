find_program(ASCIIDOCTOR NAMES asciidoctor
  DOC "Used to generate the web site pages")

set(ASCIIDOCTOR_OPTS
  -D ${CMAKE_CURRENT_BINARY_DIR}/out
  -b html5)

set(WEB_PAGES
  index
  news
  compiling
  faq
  roadmap)
set(WEB_FILES)
foreach(m IN LISTS WEB_PAGES)
  set(mf ${CMAKE_CURRENT_BINARY_DIR}/${m})
  set(ms ${CMAKE_CURRENT_SOURCE_DIR}/${m}.txt)
  add_custom_command(OUTPUT ${mf}
    COMMAND ${ASCIIDOCTOR} ${ASCIIDOCTOR_OPTS} ${ms}
      DEPENDS ${ms}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building webpage ${mf}"
      VERBATIM)
  list(APPEND WEB_FILES ${mf})
endforeach()

add_custom_target(web ALL DEPENDS ${WEB_FILES})


