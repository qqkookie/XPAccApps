# Global master CMakeList.txt

cmake_minimum_required(VERSION 3.10)
project(XPApps)

# CHoose message resouce language here.
# add_compile_definitions(LANGUAGE_EN_US)
add_compile_definitions(LANGUAGE_KO_KR)

# see https://learn.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource
add_compile_definitions( VER_TRANSLATION=0x0412,03B5 )
add_compile_definitions( VER_PROJVERSION=0,0,1,0)

add_compile_definitions(_UNICODE UNICODE)
# add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
if(MSVC)
#     add_importlibs(calc ntdll)
    # warning C4244, C4267: conversion from 'xxx' to 'yyy', possible loss of data
    # warning C4996: 'xxx': This function or variable may be unsafe. Consider using xxx_s instead. 
    add_compile_options(/nologo /wd4244 /wd4267 /wd4996 )
    add_link_options(/SUBSYSTEM:WINDOWS)
endif()

# add_subdirectory(docs)
add_subdirectory(calc)
# add_subdirectory(notepad)
# add_subdirectory(taskmgr)
