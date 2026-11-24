# Gives every picture of the built pages loading="lazy": a page of 400 GIFs loads the ones on the screen. Run after Doxygen.
file(GLOB WOOD_PAGES ${WOOD_HTML_DIR}/elements*.html ${WOOD_HTML_DIR}/templates*.html ${WOOD_HTML_DIR}/examples.html ${WOOD_HTML_DIR}/architecture.html)
foreach(page IN LISTS WOOD_PAGES)
    file(READ ${page} text)
    string(REPLACE "<img src=" "<img loading=\"lazy\" src=" text "${text}")
    file(WRITE ${page} "${text}")
endforeach()
