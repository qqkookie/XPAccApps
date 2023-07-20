
if(DBG)
    add_definitions(-D_DEBUG=1)
endif()

list(APPEND SOURCE
    dialog.c
    main.c
    printing.c
    settings.c
    text.c
    misc.c
    notepad.h)

add_link_options(/SUBSYSTEM:WINDOWS /MANIFEST:NO)

link_libraries(comctl32 )
# target_link_libraries(${xapp})

# add_rc_deps(rsrc.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/notepad.ico)
add_executable(xnotepad ${SOURCE} rsrc.rc)
#set_module_type(notepad win32gui UNICODE)
# add_importlibs(notepad user32 gdi32 comctl32 comdlg32 advapi32 shell32 msvcrt kernel32)
#add_pch(notepad notepad.h SOURCE)
# add_cd_file(TARGET notepad DESTINATION reactos FOR all)
# add_cd_file(TARGET notepad DESTINATION reactos/system32 FOR all)
