# Clipbrd app slave CMakeList.txt

project(XClipBrd)

list(APPEND SOURCE
    clipbrd.c
    cliputils.c
    fileutils.c
    scrollutils.c
    winutils.c
    precomp.h)

# add_link_options(/SUBSYSTEM:WINDOWS /MANIFEST:NO)

add_executable(xclipbrd ${SOURCE} clipbrd.rc)

target_link_libraries(xclipbrd Htmlhelp)
