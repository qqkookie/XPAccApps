# Notepad app slave CMakeList.txt

project(XNotePad)

if(DBG)
    add_definitions(-D_DEBUG=1)
endif()

list(APPEND SOURCE

    main.c
    settings.c
    dialog.c
    file.c
    text.c
    printing.c
    
    notepad.h
    dialog.h
)

link_libraries(comctl32 )

add_executable(xnotepad ${SOURCE} rsrc.rc)
