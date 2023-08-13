CHANGELOG
=====

## v0.1.6
 * 2023-08-12 00:00:00
 * All "CMakeLists.txt" clean up
 * Wine debug.c -> debug.h
 xnotepad major update.
 xchcp ydate


### xclac
 * Alt-X key to quit
 * Bold buttons
 * Simplified menu

### xclipbrd
 * Alt-X key to quit
 * Accelerator resource to "clipbrd.rc"
 
### xclock
 * Alt-X key to quit

### xmspaint
 * Alt-X key to quit
 * F2 key to save
 * Accelerator resource to "rsrc.rc"
 * Uses Ini file, not windows registry

### xnotepad
 * Alt-X key to quit
 * Accelerator resource to "rsrc.rc"
 * Readonly mode (-v -r option)
 * Insert datetime to short date/time
 * TODO : MRU on menu :DONE
 * TODO : ISO date on ALt-F6:DONE
 * TODO : BUG - duplicated MRU entry. DONE
 * TODO : check MRU file ok: DONE
 * TODO : read only file option/ no modify on readonly or view only mode) :DONE
 * TODO : Reload on modify DONE

### xchcp
 * based on Wine version.
 * code clean up.
 * debug.c, consutils dependancy removed. 

### xtaskmgr, xwordpad
TODO: save setting inifile
TODO: High Dpi
TODO: Alt-X to quit

## v0.1.5
 * 2023-07-31 13:32:00
 * Import Wine codebase
 * Wine chcp, clock, notepad, taskmgr, wordpad, winemine
 * fontview, winmine from ROS

### xnotepad
 * Major code rearrange/clean up
 * Globals -> Globals + Settings separation
 * Modify F1-F5 accelerator key
 * Enum encodng/eoln reorder
 * Encodings name change
 * STRING_TEXT_TYPE_FILTER, STRING_MORE_TYPE_FILTER
 * STRING_LOSS_OF_UNICODE_CHARACTERS
 * Use GETSTRING(), LOADSTRING()
 * UpdateWindowLayout() in dialog.c
 * misc.c -> file.c + search
 * MAX_STRING_LEN -> STR_LONG

### xmine
 * Minesweeper ported. HighDPI OK.
 * Add reset score
 * Save to XPAccApps.ini OK

### xclipbrd
 * font selection
 * font height adjust
 * Menu simplify

### xclock
 * Code clean up, refactoring
 * winclock.h merged -> main.h
 * Painting to DrawClock()
 * Saves settings.
 * Scale to High Dpi OK
 * Main menu -> popup menu.
 * Add 24 hours, dark mode, exit menu
 * Remove font/date menu (date on titlebar is default)
 * fix color scheme
 * clock redraw optimization.

## v0.1.4
 * 2023-07-22 16:16:00
 * Project name change: WinXPApp -> XPAccApps
 * Add project() name to CMakeLists.txt

### xmspaint
 * Ported to Windows 11
 * Scaled for DPI 200%
 * Adjust menu layout
 * Expanded MRU as submenu
 * Separate Zoom menu.
 * More zoom steps
 * New command -> Close command.
 * No save on unmodified new image
 * Zoom tool 6 range -> 7 range

### xnotepad
 * Clean up UpdateMenuRecent()

### xclipbrd
 * Porting and font scaling.

### xcalc
 * DPI scaling fix.

### wordpad
 * Wordpad ported
 * TODO: DPI scaling WIP.

## v0.1.3
 * 2023-07-18 20:26:00

### xchcp.exe
 * Add symbolic codepage names (like korea, euc-kr,...)

### xnotepad
 * added "XPAccApps.ini" setting support.
 * added multi tab support
 * close tab, close all files.
 * Search wrap around / find previous menu.
 * Duplicate edit check.
 * Untiltedled time stamp.
 * MRU Recent support
 * Add misc.c & code rearrage.
 * setting clean up

## v0.1.2
 * 2023-07-03 15:42:00

###  xcalc
 * Fixed uninitialized calc total memory base ([M+]) bug.
 * Expanded Standard Calc layout:
   - Add total memory minus ([M-]) button.
   - Add parenthsis buttons.
   - Change [Sqrt] button to [x^y] button.
   - Enlarge [+] and [=] buttons.
 * Saves setting in "~/AppData/XPAccApps.ini" file, not registry.
   - Remembers window position.
 * Adjust unit conversion list order.
 * Add force units (newton, pound-force etc) in unit conversion.
 * Removed old EU currencies like Deuche Mark.
 * Add 18 major currencies (USD, GBP, JPY,...,BTC) in currency conversions.
 * Online FX currency exchange rate update lookup.
 * Tootip on [M] indicator shows memory value.
 * [MR] button is disabled on empty memory.
 * Added Chinese character on various CJK unit names
 * Some unused ancient conversion units removed.
    - Area: RAI, VA
    - Length: BARLEYCORNS, CHAINS_UK, CHOU, HUNH, KABIET, KEUB,
		          LINKS_UK, NIEU, SAWK, SEN, VA, YOTE
	  - Volume: BUN, GOU, KWIAN, TANANLOUN, TANG, TO,
	  - Volume units added: Doe(Shou), Hop(Gou), Mal(To)
	  - Weight: BAHT

 * Bug fix: tooltip on layout change.
 * Hexa tooltip.
 * MPFR build OK, "CMakeLists.txt" ENABLE_MULTI_PRECISION
 * [MS], [MR] works correctly on MPFR + hexa moode.
 * TODO: History tape window? Make HTMLHELP work.

### xchcp
Winodws chcp.com replacement.
Added charmap, clipbrd, mspaint, notepad, wordpad source.

## v0.1.1

 * 2023-06-13 15:18:55
 * Initial directory setup
 * Add calc orignal source
