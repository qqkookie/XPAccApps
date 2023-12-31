# Global WinXPAccApps project master CMakeList.txt

cmake_minimum_required(VERSION 3.13)
project(XPAccApps)

add_definitions(-D_AMD64_ -D_WIN64)
add_definitions(-D_UNICODE -DUNICODE)

# common setting ini file in ~/AppData/Local
add_compile_definitions( PRIVATEPROFILE_INI="XPAccApps.ini")

if(MSVC)
    # warning C4244, C4267: conversion from 'xxx' to 'yyy', possible loss of data
    # warning C4996: 'xxx': This function or variable may be unsafe. Consider using xxx_s instead. 
    # add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/nologo /wd4244 /wd4267 /wd4996 )
endif()

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY out )

# add_subdirectory(docs)

# Console programs
# add_subdirectory(conutils)
add_subdirectory(chcp)

# Window apps
add_link_options(/SUBSYSTEM:WINDOWS /MANIFEST:NO)

add_subdirectory(calc)
add_subdirectory(taskmgr)
add_subdirectory(notepad)
add_subdirectory(mspaint)
add_subdirectory(clipbrd)
add_subdirectory(wordpad)
add_subdirectory(winmine)
add_subdirectory(clock)
add_subdirectory(winhlp32)
