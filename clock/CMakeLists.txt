# Simple Clock app slave CMakeList.txt

project(XClock)

list(APPEND SOURCE

    main.c
    winclock.c
    main.h
)

add_executable(xclock ${SOURCE} clock.rc)

target_link_libraries(xclock comctl32)
