# Global WinXPAccApp project master CMakeList.txt

cmake_minimum_required(VERSION 3.10)
project(XPApps)

add_compile_definitions(_AMD64_ _WIN64)
add_compile_definitions(_UNICODE UNICODE)
# common setting ini file in ~/AppData/Local
add_compile_definitions( PRIVATEPROFILE_INI="XPAccApp.ini")

if(MSVC)
#     add_importlibs(calc ntdll)
    # warning C4244, C4267: conversion from 'xxx' to 'yyy', possible loss of data
    # warning C4996: 'xxx': This function or variable may be unsafe. Consider using xxx_s instead. 
    # add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/nologo /wd4244 /wd4267 /wd4996 )

    add_link_options(/SUBSYSTEM:WINDOWS /MANIFEST:NO)
endif()

# add_subdirectory(docs)
add_subdirectory(calc)
# add_subdirectory(notepad)
add_subdirectory(taskmgr)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY out )
