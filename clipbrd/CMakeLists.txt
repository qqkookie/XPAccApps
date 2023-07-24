# Clipbrd app slave CMakeList.txt

project(XClipBrd)

list(APPEND SOURCE
    clipbrd.c
    cliputils.c
    fileutils.c
    scrollutils.c
    winutils.c
    precomp.h)

add_link_options(/SUBSYSTEM:WINDOWS /MANIFEST:NO)

add_executable(xclipbrd ${SOURCE} clipbrd.rc)

target_link_libraries(xclipbrd Htmlhelp)

# add_pch(clipbrd precomp.h SOURCE)
# set_module_type(clipbrd win32gui UNICODE)
# add_importlibs(clipbrd hhctrl user32 gdi32 comdlg32 shell32 msvcrt kernel32)
# add_cd_file(TARGET clipbrd DESTINATION reactos/system32 FOR all)
