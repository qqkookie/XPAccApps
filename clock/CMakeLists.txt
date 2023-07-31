# Simple Clock app slave CMakeList.txt

project(XClock)

list(APPEND SOURCE
    main.c
    winclock.c
    main.h )

add_link_options(/SUBSYSTEM:WINDOWS /MANIFEST:NO)

add_executable(xclock ${SOURCE} clock.rc)

target_link_libraries(xclock comctl32)

# add_pch(clipbrd precomp.h SOURCE)
# set_module_type(clipbrd win32gui UNICODE)
# add_importlibs(clipbrd hhctrl user32 gdi32 comdlg32 shell32 msvcrt kernel32)
# add_cd_file(TARGET clipbrd DESTINATION reactos/system32 FOR all)
