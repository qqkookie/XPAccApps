# Wordpad app slave CMakeList.txt

project(XWordPad)

# remove_definitions(-D_WIN32_IE=0x600)

# add_definitions(-D__ROS_LONG64__)

# include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE

    olecallback.c
    print.c
    registry.c
    wordpad.c
    wordpad.h
    ../wordpad_ROS/debug.c
)

add_executable(xwordpad ${SOURCE} wordpad.rc)

target_link_libraries(xwordpad Comctl32) 

# if(MSVC)
#     # Disable warning C4477 (printf format warnings)
#     add_compile_options(wordpad "/we4477")
# endif()
