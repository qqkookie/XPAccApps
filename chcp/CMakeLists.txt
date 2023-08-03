# Console CHCP utility CMakeList.txt

project(XChCp)

include_directories(../conutils)

add_executable(xchcp chcp.c chcp.rc)

add_compile_definitions(CONSOLE_APP NOMAINFEST)

target_link_libraries(xchcp conutils ${PSEH_LIB})

add_link_options(/SUBSYSTEM:CONSOLE)
