# conutils.lib Console Utility Library CMakeList.txt

list(APPEND SOURCE
    # instream.c
    outstream.c
    pager.c
    screen.c
    stream.c
    utils.c
    # conutils.h
    )

add_library(conutils ${SOURCE})

target_link_libraries(conutils ${PSEH_LIB})
