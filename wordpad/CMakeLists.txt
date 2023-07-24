# Wordpad app slave CMakeList.txt

project(XWordPad)

remove_definitions(-D_WIN32_IE=0x600)

add_definitions(-D__ROS_LONG64__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    debug.c
    olecallback.c
    print.c
    registry.c
    wordpad.c)

add_link_options(/SUBSYSTEM:WINDOWS /MANIFEST:NO)

add_executable(xwordpad ${SOURCE} rsrc.rc)
target_link_libraries(xwordpad Comctl32) 

# file(GLOB wordpad_rc_deps res/*.*)
# add_rc_deps(rsrc.rc ${wordpad_rc_deps})
# add_executable(wordpad ${SOURCE} rsrc.rc)
# set_module_type(wordpad win32gui)
# target_link_libraries(wordpad wine uuid)
# add_importlibs(wordpad comdlg32 ole32 shell32 user32 gdi32 advapi32 comctl32 msvcrt kernel32 ntdll)
# add_cd_file(TARGET wordpad DESTINATION reactos/system32 FOR all)

if(MSVC)
    # Disable warning C4477 (printf format warnings)
    add_compile_options(wordpad "/we4477")
endif()
