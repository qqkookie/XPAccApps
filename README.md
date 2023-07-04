 WinXPAccApps
======

## Introduction
Simple Windows XP Accessories ported from React OS application
like calculator, notepad, taskmgr.
It looks, feels and works like Windows XP desktop accessory tools clone.

Current vserion: 0.0.2 - developing calculator

## Build/Install
This project can be built from source using:
 * `CMake/Ninja` build tool: `CMakeList.txt`
 * Visual Studio solution: `WinAccApps.sln`
This is developed with Visual Studio 2022 on MS Windows 11

To configure language localization, set LANGUAGE definition 
in "version.rc" like LANGUAGE_EN_US, and matching APP_TRANSLATION

To install and use, copy *.exe binary excutables in release ZIP file 
to any folder in yur %Path% directories. No instllation is needed. 
To uninstall, delete those exe files. Setings are loaded/saved as 
"~/AppData/XPAccApp.ini" file if it exists in user's home.

## NOTICE
This source codes are based on [ReactOS](https://github.com/reactos/reactos) project.
For license/copying, owners/credits, Code of Conduct, see ReactOS documents

## FAQ
 * Why memory indicator [M] is removed?<P>
[MR] button itself is memory indicator. [MR] button is 
disabled when memory is empty. When memory is set, 
hovering mouse over [MR] button shows tooltip of memory value.
 * Why [Sqrt] button is replaced by [X^y]?<P>
[x^y] button is more verstile than [Sqrt], when commbinde with [1/x] button.
[x^y] can do [x^2], [x^3], [Sqrt] and cubic root.
123[x^y]2 is same as 123[x^2]. 123[x^y]2[1/x] is same as 123[Sqrt]. 
123[x^y]3 is [x^3], 123[x^y]3[1/x] is cublic root, etc.
