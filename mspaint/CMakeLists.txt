# MSPaint app slave CMakeList.txt

project(XMSPaint)

add_definitions(-DINITGUID)

if(DBG)
    add_definitions(-D_DEBUG=1) # CORE-17505
endif()

list(APPEND SOURCE

    canvas.cpp
    dialogs.cpp
    dib.cpp
    drawing.cpp
    fullscreen.cpp
    history.cpp
    main.cpp
    miniature.cpp
    mouse.cpp
    palette.cpp
    palettemodel.cpp
    registry.cpp
    selectionmodel.cpp
    sizebox.cpp
    textedit.cpp
    toolbox.cpp
    toolsettings.cpp
    toolsmodel.cpp
    winproc.cpp
    precomp.h
)

add_executable(xmspaint ${SOURCE} rsrc.rc)

target_link_libraries(xmspaint comctl32 rpcrt4)
