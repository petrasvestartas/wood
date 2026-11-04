# Copies every examples/**/*.cpp to ${WOOD_DOC_EXAMPLES_OUT} without the trailing
#   /*
#   |||||||| DESCRIPTION ||||||||
#   ...
#   */
# block (description, directory and build lines), so `\include` on a docs page shows the program alone.
# The build lines are shown once, on the Examples page. Run by the wood_doc_examples target, before every docs build.
file(GLOB_RECURSE WOOD_EXAMPLE_FILES RELATIVE ${WOOD_EXAMPLES_DIR} ${WOOD_EXAMPLES_DIR}/*.cpp)
foreach(example IN LISTS WOOD_EXAMPLE_FILES)
    file(READ ${WOOD_EXAMPLES_DIR}/${example} text)
    string(FIND "${text}" "/*\n|||||||| DESCRIPTION" footer_start)
    if(footer_start EQUAL -1)
        string(FIND "${text}" "/*\r\n|||||||| DESCRIPTION" footer_start)
    endif()
    if(NOT footer_start EQUAL -1)
        string(SUBSTRING "${text}" 0 ${footer_start} text)
        string(REGEX REPLACE "[\r\n ]+$" "\n" text "${text}")
    endif()
    file(WRITE ${WOOD_DOC_EXAMPLES_OUT}/${example} "${text}")
endforeach()
