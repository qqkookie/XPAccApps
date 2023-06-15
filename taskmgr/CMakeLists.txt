# TaskMgr app slave CMakeList.txt

set(xapp xtaskmgr)

# define app version resources
add_compile_definitions( VER_PRODUCTNAME_STR="XTaskMgr")
add_compile_definitions( VER_PRODUCTVERSION=0,0,1,0)
add_compile_definitions( VER_FILEVERSION=0,0,1,1)
add_compile_definitions( VER_FILEDESCRIPTION_STR="Task Manager app")
add_compile_definitions( VER_ORIGINALFILENAME_STR="${xapp}.exe")
add_compile_definitions( _ANONYMOUS_UNION=)

list(APPEND SOURCE
    about.c
    affinity.c
    applpage.c
    column.c
    debug.c
    endproc.c
    graph.c
    graphctl.c
    optnmenu.c
    perfdata.c
    perfpage.c
    priority.c
    proclist.c
    procpage.c
    run.c
    shutdown.c
    taskmgr.c
    trayicon.c
    precomp.h)

# file(GLOB taskmgr_rc_deps res/*.*)
# add_rc_deps(taskmgr.rc ${taskmgr_rc_deps})
# add_executable(taskmgr ${SOURCE} taskmgr.rc)
# set_module_type(taskmgr win32gui UNICODE)
# add_importlibs(taskmgr advapi32 user32 gdi32 shell32 shlwapi comctl32 msvcrt kernel32 ntdll)
# add_pch(taskmgr precomp.h SOURCE)
# add_cd_file(TARGET taskmgr DESTINATION reactos/system32 FOR all)

include_directories(res)
include_directories(sdk)
add_executable(${xapp} ${SOURCE} taskmgr.rc)


link_libraries(comctl32 advapi32 user32 gdi32 shell32 shlwapi msvcrt kernel32 ntdll)
target_link_libraries(${xapp} comctl32)

