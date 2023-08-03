XP Acc Apps
======

## Introduction
Simple Windows XP Accessories ported from React OS/Wine application
like calculator, notepad, taskmgr, mspinat, wordpad, etc.
It looks, feels and works like Windows XP desktop accessory tools clone.
It supports high-DPI monitor like 144 dpi.

Current vserion: 0.1.6

## What it has:

 * xcalc - XP style calculator app.
   64 bits double precision or multi-precision MPFR math support.
 * xtaskmgr - XP style task manager
 * xchcp.exe - Windows chcp.com replacement
 * xnotepad - Simple notepad-like text editor with multi-tab support
 * xwordpad - Rich text RTF document editor
 * xmspaint - MS Paint-like image editor
 * xmine - Windows MineSweeper clone game
 * xclock - Windows clock
 * xclipbrd - clipboard viewer

## Build/Install

This project can be built from source using `CMake/Ninja` build tool: `CMakeList.txt`
This is developed with Visual Studio 2022 on MS Windows 11

In developer build, all executables are located in directory like
`out/build-cmvc-x64-Release/calc/xcalc.exe`
To uninstall, delete those exe files.

To configure language localization, Add LANGUAGE definition
in "version.rc" like LANGUAGE_KO_KR, and matching APP_TRANSLATION.
Multiple LANGUAGE_* supported.
LANGUAGE_EN_US should be defiend as minimum,
fallback language for unsupported/missing languages.

"`CMakeLists.txt`" in each source subdirectory may have app-specific config items.

To install and use, copy *.exe binary excutables in release ZIP file
to any folder in yuor %Path% directories. No instllation is needed.

Settings of most apps are saved to and loaded from "`~/AppData/Local/XPAccApp.ini`" file
in AppData folder under user's home. Or loaded/saved to registry branch under users's HKCU tree.

## NOTICE

This source codes are based on [ReactOS](https://github.com/reactos/reactos)
and [Wine](https://github.com/wine-mirror/wine) project.
For license/copying, owners/credits, Code of Conduct, see ReactOS/Wine documents

## FAQ

### xcalc
 * Why memory indicator [M] is removed?<P>
[MR] button itself is memory indicator. [MR] button is
disabled when memory is empty. When memory is set,
hovering mouse over [MR] button shows tooltip of memory value.

 * Why [Sqrt] button is replaced by [X^y]?<P>
[x^y] button is more verstile than [Sqrt], when commbinde with [1/x] button.
[x^y] can do [x^2], [x^3], [Sqrt] and cubic root.
123[x^y]2 is same as 123[x^2]. 123[x^y]2[1/x] is same as 123[Sqrt].
123[x^y]3 is [x^3], 123[x^y]3[1/x] is cublic root, etc.
