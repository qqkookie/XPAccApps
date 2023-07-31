/*
 * Clock
 *
 * Copyright 1998 Marcel Baur <mbaur@g26.ethz.ch>
 *
 * Clock is partially based on
 * - Program Manager by Ulrich Schmied
 * - rolex.c by Jim Peterson
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "main.h"
#include <commctrl.h>
#include <shellapi.h>

#define INITIAL_WINDOW_SIZE     300
#define DRAGBAR_DROP            100      // dragbar drop from top
#define TIMER_ID 1

CLK_GLOBALS Globals;

static BOOL LoadSettings(VOID);
static BOOL SaveSettings(VOID);

/***********************************************************************/

        /* analog clock */
        CheckMenuRadioItem(hPropertiesMenu, IDM_ANALOG, IDM_DIGITAL, IDM_ANALOG, MF_CHECKED);
        EnableMenuItem(hPropertiesMenu, IDM_FONT, MF_GRAYED);
    }
    else
    {
        /* digital clock */
        CheckMenuRadioItem(hPropertiesMenu, IDM_ANALOG, IDM_DIGITAL, IDM_DIGITAL, MF_CHECKED);
        EnableMenuItem(hPropertiesMenu, IDM_FONT, 0);
    }

    CheckMenuItem(hPropertiesMenu, IDM_NOTITLE, (Globals.bWithoutTitle ? MF_CHECKED : MF_UNCHECKED));

    CheckMenuItem(hPropertiesMenu, IDM_ONTOP, (Globals.bAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hPropertiesMenu, IDM_SECONDS, (Globals.bSeconds ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hPropertiesMenu, IDM_DATE, (Globals.bDate ? MF_CHECKED : MF_UNCHECKED));
}

static VOID CLOCK_UpdateWindowCaption(VOID)
{
    WCHAR szCaption[MAX_STRING_LEN];
    int chars = 0;

    /* Set frame caption */
    if (!Globals.bNoTitleBar )
	    chars = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, NULL, NULL,
                               szCaption, _countof(szCaption));        
    if (chars) {
        wcscpy( szCaption + chars -1, L" - ");
        chars += 2 ;
    }
    LoadStringW(0, IDS_CLOCK, szCaption + chars, MAX_STRING_LEN - chars);
    SetWindowTextW(Globals.hMainWnd, szCaption);

    LONG style = GetWindowLongW(Globals.hMainWnd, GWL_STYLE);

    if (Globals.bNoTitleBar) {
	    style = (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUPWINDOW ;
//	    SetMenu(Globals.hMainWnd, 0);
    }
    else {
	    style = (style & ~WS_POPUPWINDOW)| WS_OVERLAPPEDWINDOW;
//        SetMenu(Globals.hMainWnd, Globals.hMainMenu);
    }
    SetWindowLongW(Globals.hMainWnd, GWL_STYLE, style);

    if (Globals.bNoTitleBar && Globals.bAnalog)
        SetWindowRgn(Globals.hMainWnd, Globals.hCircle, TRUE);
    else
        SetWindowRgn(Globals.hMainWnd, NULL, TRUE);

    SetWindowPos(Globals.hMainWnd,
            (Globals.bAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST),
            0,0,0,0,  SWP_DRAWFRAME|SWP_NOMOVE|SWP_NOSIZE);
}

static VOID Clk_ToggleTitle(VOID)
{
    Globals.bNoTitleBar = !Globals.bNoTitleBar;
    Clk_UpdateWindowTitleBar();
}

static VOID Clk_ToggleOnTop(VOID)
{
    Globals.bAlwaysOnTop = !Globals.bAlwaysOnTop;
	SetWindowPos(Globals.hMainWnd,
        (Globals.bAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST),
        0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
}

/***********************************************************************/

// Reset clock period timer by clock 
static BOOL Clk_ResetTimer(void)
{
    UINT period; /* in milliseconds */

    KillTimer(Globals.hMainWnd, TIMER_ID);

    if (!Globals.bSeconds)
    	period = 1000;
	else if (!Globals.bAnalog)
	    period = 500;
	else
	    period = 250;

    if (!SetTimer (Globals.hMainWnd, TIMER_ID, period, NULL)) {
        WCHAR szApp[MAX_STRING_LEN];
        LoadStringW(Globals.hInstance, IDS_CLOCK, szApp, MAX_STRING_LEN);
        MessageBoxW(0, L"No available timers", szApp, MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }
    return TRUE;
}

// Choose Font (NOT USED)
static VOID Clk_ChooseFont(VOID)
{
    LOGFONTW lf;
    CHOOSEFONTW cf;
    memset(&cf, 0, sizeof(cf));
    lf = Globals.logfont;
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = Globals.hMainWnd;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS;
    if (ChooseFontW(&cf)) {
	    Globals.logfont = lf;
	    ResizeFont();
    }
}

/***********************************************************************/
// menu handling utility

#include <windowsx.h>

static VOID Clk_SetMenuCheckmarks(HMENU hmpop);

// Display popup menu
static VOID DisplayContextMenu(LPARAM lpm) 
{ 
    HMENU hmpop = LoadMenu(Globals.hInstance,  MAKEINTRESOURCEW(POPUP_MENU));
    if (!hmpop) 
        return; 

    Clk_SetMenuCheckmarks(hmpop);
    TrackPopupMenu( GetSubMenu(hmpop, 0), TPM_LEFTALIGN | TPM_RIGHTBUTTON,
            GET_X_LPARAM(lpm), GET_Y_LPARAM(lpm), 0, Globals.hMainWnd, NULL);  
    DestroyMenu(hmpop);
}

// menu action common epilog
static void ApplyMenu(void)
{
    // Clk_UpdateMenuCheckmarks();
    if (!Globals.bAnalog)
	    ResizeFont();
    InvalidateRect(Globals.hMainWnd, NULL, FALSE);   
}

// If the lparam point position is within the client area,
// return client xy as LPARAM, else return 0
static LPARAM ClientPoint(HWND hWnd, LPARAM lpm)
{
    RECT crct;
    GetClientRect(hWnd, &crct);  // client area of window 
    POINT cpt = { GET_X_LPARAM(lpm),  GET_Y_LPARAM(lpm) };
    ScreenToClient(hWnd, &cpt);

    return ( PtInRect(&crct, cpt) ? MAKELPARAM( cpt.x, cpt.y ) : 0);
}

// Set popup menu item checked or disabled state
static VOID Clk_SetMenuCheckmarks(HMENU hmpop)
{
    HMENU hPropertiesMenu = GetSubMenu(hmpop, 0);
    if (!hPropertiesMenu)
	    return;

    if(Globals.bAnalog) {
        /* analog clock */
        CheckMenuRadioItem(hPropertiesMenu, IDM_ANALOG, IDM_DIGITAL, IDM_ANALOG, MF_CHECKED);
//        EnableMenuItem(hPropertiesMenu, IDM_FONT, MF_GRAYED);
        EnableMenuItem(hPropertiesMenu, IDM_24HOURS, MF_GRAYED);
    }
    else
    {
        /* digital clock */
        CheckMenuRadioItem(hPropertiesMenu, IDM_ANALOG, IDM_DIGITAL, IDM_DIGITAL, MF_CHECKED);
//        EnableMenuItem(hPropertiesMenu, IDM_FONT, MF_ENABLED);
        EnableMenuItem(hPropertiesMenu, IDM_24HOURS, MF_ENABLED);
    }
//    EnableMenuItem(hPropertiesMenu, IDM_DATE, Globals.bNoTitleBar ? MF_GRAYED : MF_ENABLED);

    CheckMenuItem(hPropertiesMenu, IDM_NOTITLE, (Globals.bNoTitleBar ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hPropertiesMenu, IDM_ONTOP, (Globals.bAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hPropertiesMenu, IDM_SECONDS, (Globals.bSeconds ? MF_CHECKED : MF_UNCHECKED));
//    CheckMenuItem(hPropertiesMenu, IDM_DATE, (Globals.bDate ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hPropertiesMenu, IDM_24HOURS, (Globals.b24Hours ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hPropertiesMenu, IDM_DARKCOLOR, (Globals.bDarkColor ? MF_CHECKED : MF_UNCHECKED));
}

/***********************************************************************
 *  All handling of popup menu events
 */
 
static int Clk_MenuCommand (WPARAM wParam)
{
    WCHAR szApp[MAX_STRING_LEN];
    WCHAR szAppRelease[MAX_STRING_LEN];
    switch (wParam) {

        /* switch to analog/digital */
        case IDM_ANALOG:
        case IDM_DIGITAL: {
            Globals.bAnalog = (wParam == IDM_ANALOG);
	        Clk_ResetTimer();
            Clk_UpdateWindowTitleBar();
            ApplyMenu();
            break;
        }
        /* change font */
        /*
        case IDM_FONT: {
            Clk_ChooseFont();
            break;
        }
        */
        /* hide title bar */
        case IDM_NOTITLE: {
	        Clk_ToggleTitle();
            // Clk_UpdateMenuCheckmarks();
            break;
        }
        /* always on top */
        case IDM_ONTOP: {
	        Clk_ToggleOnTop();
            // Clk_UpdateMenuCheckmarks();
            break;
        }
        /* show or hide seconds */
        case IDM_SECONDS: {
            Globals.bSeconds = !Globals.bSeconds;
	        Clk_ResetTimer();
            ApplyMenu();
            break;
        }
        /* show or hide date */
        /*
        case IDM_DATE: {
            Globals.bDate = !Globals.bDate;
            Clk_UpdateMenuCheckmarks();
            Clk_UpdateWindowTitleBar();
            break;
        }
        */
        /* show 24 hours on digital clock */
        case IDM_24HOURS: {
            Globals.b24Hours = !Globals.b24Hours;
            ApplyMenu();
            break;
        }
        /* show or hide date */
        case IDM_DARKCOLOR: {
            Globals.bDarkColor = !Globals.bDarkColor;
            ApplyColor();
            break;
        }
        /* show "about" box */
        case IDM_ABOUT: {
            LoadStringW(Globals.hInstance, IDS_CLOCK, szApp,  _countof(szApp));
            lstrcpyW(szAppRelease,szApp);
            ShellAboutW(Globals.hMainWnd, szApp, szAppRelease, 0);
            break;
        }

        case IDM_EXIT: {
            PostMessageW(Globals.hMainWnd, WM_CLOSE, 0, 0);
            break;
        }
    }
    return 0;
}

/***********************************************************************
 *
 *           Clk_WndProc
 */

static LRESULT WINAPI Clk_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static time_t last;

    switch (msg) {
	    /* L button drag moves the window */
        case WM_NCHITTEST: {
	        LRESULT ret = DefWindowProcW(hWnd, msg, wParam, lParam);
            // if y < drag drop then drag, else popup menu. 
	        if (ret == HTCLIENT && Globals.bNoTitleBar
                && GET_Y_LPARAM(ClientPoint(hWnd, lParam)) < DRAGBAR_DROP)
                return HTCAPTION;

            return ret;
	    }

        case WM_NCLBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK: {
	        Clk_ToggleTitle();
            break;
        }

        case WM_PAINT: {
            DrawClock();
            last = (time(0)/60) *60; 
            break;
        }

        case WM_SIZE: {
            Globals.WinW = LOWORD(lParam);
            Globals.WinH = HIWORD(lParam);
  
            if ( !Globals.bAnalog )
	            ResizeFont();
            else if ( Globals.bNoTitleBar )
                SetAnalogRegion();
            break;
        }

        case WM_COMMAND: {
            Clk_MenuCommand(wParam);
            break;
        }
            
        case WM_TIMER: {
            /* Could just invalidate what has changed,
             * but it doesn't really seem worth the effort
             * When no second, don't redraw clock each second.
             */
             if ( Globals.bSeconds || time(0) >= last + 58)
	            InvalidateRect(Globals.hMainWnd, NULL, FALSE);
	        break;
        }

        case WM_DESTROY: {
            SaveSettings();
            PostQuitMessage (0);
            break;
        }

        case WM_CONTEXTMENU:{
            // If the position is in the client area, display a shortcut menu. 
            if (ClientPoint(hWnd, lParam) > 0) 
            { 
                DisplayContextMenu(lParam); 
                return TRUE; 
            } 
            return DefWindowProc(hWnd, msg, wParam, lParam);
            break;
        }

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

/***********************************************************************
 *
 *           WinMain
 */

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    MSG      msg;
    WNDCLASSW class;

    InitCommonControls();
    /* Setup Globals */
    memset(&Globals, 0, sizeof (Globals));
    Globals.WinW = Globals.WinH = INITIAL_WINDOW_SIZE;
    Globals.bAnalog         = TRUE;
    Globals.bSeconds        = TRUE;
//    Globals.bDate           = TRUE;

    if (!prev){
        class.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        class.lpfnWndProc   = Clk_WndProc;
        class.cbClsExtra    = 0;
        class.cbWndExtra    = 0;
        class.hInstance     = hInstance;
        class.hIcon         = LoadIconW(0, (LPCWSTR)IDI_APPLICATION);
        class.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
        class.hbrBackground = 0;
        class.lpszMenuName  = 0;
        class.lpszClassName = L"ClkCls";
    }

    if (!RegisterClassW(&class)) return FALSE;

    HWND hWnd = CreateWindowW( L"ClkCls", L"XClock",
                    (WS_POPUPWINDOW|WS_CAPTION|WS_OVERLAPPEDWINDOW )&(~WS_VISIBLE),
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT , CW_USEDEFAULT,
                    0, 0, hInstance, 0);

    Globals.hMainWnd = hWnd;
    if (!Clk_ResetTimer())
        return FALSE;

    Globals.hInstance = hInstance;
    LoadSettings();
    InitPallet();

    // Globals.hMainMenu = LoadMenuW(0, MAKEINTRESOURCEW(MAIN_MENU));
    // SetMenu(hWnd, Globals.hMainMenu);

    SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE)|WS_EX_TOOLWINDOW );
    ResizeFont();
    Clk_UpdateWindowTitleBar();

    ShowWindow (hWnd, show);
    UpdateWindow (hWnd);

    while (GetMessageW(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    KillTimer(hWnd, TIMER_ID);
    DeleteObject(Globals.hFont);

    return 0;
}

/***********************************************************************/
//       SAVE/LOAD settings

#ifdef PRIVATEPROFILE_INI
#include <Shlobj.h>

#define PFSECTION L"XClock"

static TCHAR ProfilePath[MAX_PATH] = {0};

static BOOL FileExists(LPCTSTR szFilename)
{
    return GetFileAttributes(szFilename) != INVALID_FILE_ATTRIBUTES;
}

#define GETSETTINGINT(key, def) \
            GetPrivateProfileInt(PFSECTION, key, def, ProfilePath)

#define LOADSETTINGSTR(key, buf, def) \
            GetPrivateProfileString(PFSECTION, key, def, \
            buf, _countof(buf), ProfilePath)


#define SAVESETTINGINT(key, value) \
            _itow_s(value, _savebuf, _countof(_savebuf), 10),\
            WritePrivateProfileString(PFSECTION, key, _savebuf, ProfilePath)
#define SAVESETTINGSTR(key, str) \
            WritePrivateProfileString(PFSECTION, key, str, ProfilePath)

static BOOL LoadSettings(VOID)
{
    if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, ProfilePath)))
        return FALSE;

    wcscat_s(ProfilePath, MAX_PATH, L"\\" PRIVATEPROFILE_INI);

    int px = GETSETTINGINT(L"PosX", 200);
    int py = GETSETTINGINT(L"PosY", 200);
    Globals.WinW = GETSETTINGINT(L"Width", INITIAL_WINDOW_SIZE);
    Globals.WinH  = GETSETTINGINT(L"Height", INITIAL_WINDOW_SIZE);
    MoveWindow(Globals.hMainWnd, px,py, Globals.WinW, Globals.WinH, FALSE);

    UINT flags = GETSETTINGINT(L"Flags", 0x10);
    Globals.bAnalog         = (BOOL)  flags & (0x1);
    Globals.bAlwaysOnTop    = (BOOL)  flags & (0x01 <<1);
    Globals.bNoTitleBar   = (BOOL)  flags & (0x01 <<2);
    Globals.bSeconds        = (BOOL)  flags & (0x01 <<3);
    Globals.b24Hours        = (BOOL)  flags & (0x01 <<4);
    Globals.bDarkColor      = (BOOL)  flags & (0x01 <<5);
//    Globals.bDate           = (BOOL)  flags & (0x01 <<6);

    if (FileExists(ProfilePath))
        return TRUE;
    return SaveSettings();
}

static BOOL SaveSettings(VOID)
{
    TCHAR _savebuf[256];
    WINDOWPLACEMENT wp;
    wp.length = sizeof(wp);
    GetWindowPlacement(Globals.hMainWnd, &wp);
    RECT wnr = wp.rcNormalPosition;
    SAVESETTINGINT(L"PosX", wnr.left);
    SAVESETTINGINT(L"PosY", wnr.top);
    SAVESETTINGINT(L"Width", wnr.right - wnr.left);
    SAVESETTINGINT(L"Height", wnr.bottom - wnr.top);

    UINT flags = 0;
    flags |= Globals.bAnalog ? 0x1 : 0;
    flags |= Globals.bAlwaysOnTop ? 0x1 <<1 : 0;
    flags |= Globals.bNoTitleBar ? 0x1 <<2 : 0;
    flags |= Globals.bSeconds ? 0x1 <<3 : 0;
    flags |= Globals.b24Hours ? 0x1 <<4 : 0;
    flags |= Globals.bDarkColor ? 0x1 <<5 : 0;
//    flags |= Globals.bDate ? 0x1 <<6 : 0;

    SAVESETTINGINT(L"Flags", flags);

    return FileExists(ProfilePath);
}
#else
static BOOL LoadSettings(VOID  { return FALSE;}
static BOOL SaveSettings(VOID) { return FALSE;}
#endif

#if 0
// Moved to winclock.c: DrawClock()
/***********************************************************************
 *
 *           Clk_Paint
 */
static VOID Clk_Paint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC dcMem, dc;
    HBITMAP bmMem, bmOld;

    dc = BeginPaint(hWnd, &ps);

    /* Use an offscreen dc to avoid flicker */
    dcMem = CreateCompatibleDC(dc);
    bmMem = CreateCompatibleBitmap(dc, ps.rcPaint.right - ps.rcPaint.left,
				    ps.rcPaint.bottom - ps.rcPaint.top);

    bmOld = SelectObject(dcMem, bmMem);

    SetViewportOrgEx(dcMem, -ps.rcPaint.left, -ps.rcPaint.top, NULL);
    /* Erase the background */
    FillRect(dcMem, &ps.rcPaint, GetSysColorBrush(COLOR_3DFACE));

    if(Globals.bAnalog)
	    AnalogClock(dcMem, Globals.MaxX, Globals.MaxY, Globals.bSeconds, Globals.bNoTitleBar);
    else
	    DigitalClock(dcMem, Globals.MaxX, Globals.MaxY, Globals.bSeconds, Globals.hFont);

    /* Blit the changes to the screen */
    BitBlt(dc, 
	   ps.rcPaint.left, ps.rcPaint.top,
	   ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
           dcMem,
	   ps.rcPaint.left, ps.rcPaint.top,
           SRCCOPY);

    SelectObject(dcMem, bmOld);
    DeleteObject(bmMem);
    DeleteDC(dcMem);
    
    EndPaint(hWnd, &ps);
}
#endif
