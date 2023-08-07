# Console CHCP utility CMakeList.txt

project(XChCp)

# include_directories(../conutils)

list(APPEND SOURCE main.c  ../debug.h)

add_executable(xchcp main.c chcp.rc)

add_compile_definitions(CONSOLE_APP NOMAINFEST)

target_link_libraries(xchcp ${PSEH_LIB})

add_link_options(/SUBSYSTEM:CONSOLE)
