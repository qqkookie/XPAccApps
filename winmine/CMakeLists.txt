# WinMine Clone game slave CMakeList.txt

project(XWinMine)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND SOURCE

    main.c
    dialog.c
    main.h
    ../debug.h
)

link_libraries(comctl32 )

add_executable(xwinmine ${SOURCE} rsrc.rc)
