CHANGELOG
=====

## v0.1.3
 * 2023-07-18 20:26:00

### xchcp.exe
 * Add symbolic codepage names (like korea, euc-kr,...)

### xnotepad
 * added "XPAccApp.ini" setting support.
 * added multi tab support
 * close tab, close all files.
 * Search wrap around / find previous menu.
 * Duplicate edit check.
 * Untiltedled time stamp.

## v0.1.2
 * 2023-07-03 15:42:00

###  xcalc
 * Fixed uninitialized calc total memory base ([M+]) bug.
 * Expanded Standard Calc layout:
   - Add total memory minus ([M-]) button.
   - Add parenthsis buttons.
   - Change [Sqrt] button to [x^y] button.
   - Enlarge [+] and [=] buttons.
 * Saves setting in "~/AppData/XPAccApp.ini" file, not registry.
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
