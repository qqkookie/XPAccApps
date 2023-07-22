# Console CHCP utility CMakeList.txt

project(XChCp)

include_directories(../conutils)

add_executable(xchcp chcp.c chcp.rc)
add_compile_definitions(CONSOLE_APP NOMAINFEST)
target_link_libraries(xchcp conutils ${PSEH_LIB})
add_link_options(/SUBSYSTEM:CONSOLE)

# set_module_type(chcp win32cui UNICODE)
# add_importlibs(chcp msvcrt kernel32)
# set_target_properties(chcp PROPERTIES SUFFIX ".exe")
# add_cd_file(TARGET chcp DESTINATION reactos/system32 FOR all)
