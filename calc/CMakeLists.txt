# Calc app slave CMakeList.txt

set(xapp xcalc)

list(APPEND SOURCE
    convert.c
    fun_ieee.c
    rpn_ieee.c
    utl_ieee.c
    winmain.c
    htmlhelp.c
    theme.c
    calc.h
    resource.rc
)

# file(GLOB calc_rc_deps res/*.*)
# add_rc_deps(resource.rc ${calc_rc_deps})
include_directories(res)
add_executable(${xapp} ${SOURCE} resource.rc CMakeLists.txt)

# set_module_type(calc win32gui UNICODE)
# add_importlibs(calc advapi32 comctl32 user32 shell32 gdi32 msvcrt kernel32)
link_libraries(comctl32 advapi32 user32 shell32 gdi32 msvcrt kernel32 )

if(MSVC)
#     add_importlibs(calc ntdll)
    target_link_libraries(${xapp} comctl32 winhttp)
endif()

# add_pch(calc calc.h SOURCE)
# add_cd_file(TARGET calc DESTINATION reactos/system32 FOR all)
