# XWinHelp app slave CMakeList.txt

project(XWinHelp)

remove_definitions(-D_UNICODE -DUNICODE)

list(APPEND SOURCE
    callback.c
    hlpfile.c
    macro.c
    winhelp.c)

find_package(FLEX)

FLEX_TARGET(macro_scanner macro.lex.l macro.lex.yy.c)

list(APPEND PCH_SKIP_SOURCE
    ${FLEX_macro_scanner_OUTPUTS}
    string.c)

add_executable(xwinhelp
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    rsrc.rc)

target_link_libraries(xwinhelp comctl32)

# add_definitions(
#    -D__WINESRC__
#    -D__ROS_LONG64__
#    -Dfileno=_fileno
#    -Disatty=_isatty)

# include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

# add_rc_deps(rsrc.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/winhelp.ico)

# set_module_type(winhlp32 win32gui)
# target_link_libraries(winhlp32 wine)
# add_importlibs(winhlp32 user32 gdi32 shell32 comctl32 comdlg32 msvcrt kernel32 ntdll)
# add_pch(winhlp32 precomp.h "${PCH_SKIP_SOURCE}")
# add_cd_file(TARGET winhlp32 DESTINATION reactos FOR all)
