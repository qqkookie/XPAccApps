# TaskMgr app slave CMakeList.txt

project(XTaskMgr)

set(xapp xtaskmgr)

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

add_link_options(/SUBSYSTEM:WINDOWS /MANIFEST:NO)

include_directories(res)
include_directories(sdk)
add_executable(${xapp} ${SOURCE} taskmgr.rc)

link_libraries(comctl32 shell32 shlwapi)
target_link_libraries(${xapp} comctl32 ntdll Shlwapi)
