/*
 * PROJECT:    ReactOS Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 * COPYRIGHT:  Copyright 2000 Mike McCormack <Mike_McCormack@looksmart.com.au>
 *             Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *             Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *             Copyright 2002 Andriy Palamarchuk
 *             Copyright 2020-2023 Katayama Hirofumi MZ
 */

#include "notepad.h"

#include <shlobj.h>
#include <shellapi.h>

NOTEPAD_GLOBALS Globals;

WCHAR G_STR_NOTEPAD[STR_SHORT] ={0};

static ATOM aFINDMSGSTRING;

/***********************************************************************
 *           NOTEPAD_MenuCommand
 *
 *  All handling of main menu events
 */
static int NOTEPAD_MenuCommand(WPARAM wParam)
{
    switch (wParam)
    {
    case CMD_NEW:        DIALOG_FileNew(); break;
    case CMD_NEW_WINDOW: DIALOG_FileNewWindow(); break;
    case CMD_OPEN:       DIALOG_FileOpen(); break;
    case CMD_SAVE:       DIALOG_FileSave(); break;
    case CMD_SAVE_AS:    DIALOG_FileSaveAs(); break;
    case CMD_PRINT:      DIALOG_FilePrint(); break;
    case CMD_PAGE_SETUP: DIALOG_FilePageSetup(); break;
    case CMD_CLOSE:      DIALOG_FileClose(); break;
    case CMD_EXIT:       DIALOG_FileExit(); break;

    case CMD_UNDO:       DIALOG_EditUndo(); break;
    case CMD_CUT:        DIALOG_EditCut(); break;
    case CMD_COPY:       DIALOG_EditCopy(); break;
    case CMD_PASTE:      DIALOG_EditPaste(); break;
    case CMD_DELETE:     DIALOG_EditDelete(); break;
    case CMD_SELECT_ALL: DIALOG_EditSelectAll(); break;
    case CMD_TIME_DATE:  DIALOG_EditTimeDate(FALSE); break;
    case CMD_TIME_ISO:   DIALOG_EditTimeDate(TRUE); break;

    case CMD_SEARCH:      DIALOG_Search(); break;
    case CMD_SEARCH_NEXT: DIALOG_SearchNext(TRUE); break;
    case CMD_REPLACE:     DIALOG_Replace(); break;
    case CMD_GOTO:        DIALOG_GoTo(); break;
    case CMD_SEARCH_PREV: DIALOG_SearchNext(FALSE); break;

    case CMD_WRAP: DIALOG_EditWrap(); break;
    case CMD_FONT: DIALOG_SelectFont(); break;

    case CMD_STATUSBAR: ToggleStatusBar(); break;

    case CMD_HELP_CONTENTS: DIALOG_HelpContents(); break;
    case CMD_HELP_ABOUT_NOTEPAD: DIALOG_HelpAboutNotepad(); break;

    default:
        {
            int ix = LOWORD(wParam);
            if ( ix >= MENU_RECENT1 &&  ix <= MENU_RECENT9 )
            {
                DIALOG_MenuRecent(ix);
            }
        }
        break;
    }
    return 0;
}

/***********************************************************************
 * Data Initialization
 */
static VOID NOTEPAD_InitData(HINSTANCE hInstance)
{
    LPWSTR p;
#define TXT_TYPE    L"*.txt"
#define ALL_TYPE    L"*.*"

    ZEROMEM(Globals);
    Globals.hInstance = hInstance;
    Globals.encFile = ENCODING_DEFAULT;

    LoadAppSettings();

    p = Globals.szFilter;

    p += LoadString(NULL, STRING_TEXT_FILES_TXT, p, STR_SHORT) + 1;

    if (STRNOT(Settings.txtTypeFilter))
        LoadString(NULL, STRING_TEXT_TYPE_FILTER, Settings.txtTypeFilter, STR_SHORT);

    _stprintf( p, L"%s;%s", TXT_TYPE, Settings.txtTypeFilter );    // additinal text-like types.
    p += _tcslen(p) +1;

    p += LoadString(NULL, STRING_ALL_FILES, p, STR_SHORT) + 1;
    _tcscpy(p, ALL_TYPE);
    p += _tcslen(p) +1;

    if (STROK(Settings.moreTypeFilter))
        _tcscpy(p, Settings.moreTypeFilter);
    else
        LoadString(NULL, STRING_MORE_TYPE_FILTER, p, _countof(Settings.moreTypeFilter));

    for ( int len = _tcslen(p); len > 0 ; len--, p++)
    {
        if (*p == L'\n' || *p == L'$')
            *p = 0;
    }
    *p = '\0';

    Globals.find.lpstrFindWhat = NULL;

    Globals.hDevMode = NULL;
    Globals.hDevNames = NULL;

    LoadLibrary(L"Msftedit.dll");

    // Initialize common controls.
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_STANDARD_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&iccx);

    LOADSTRING(STRING_UNTITLED, Globals.szFileTitle );
    LOADSTRING(STRING_NOTEPAD, G_STR_NOTEPAD);
}

/***********************************************************************
 * Enable/disable items on the menu based on control state
 */
static VOID NOTEPAD_InitMenuPopup(HMENU menuhit, LPARAM index)
{
    int enable;

    UNREFERENCED_PARAMETER(index);

    HMENU menu = Globals.hMenu;

    CheckMenuItem(menu, CMD_WRAP, (Settings.bWrapLongLines ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, CMD_STATUSBAR, (Settings.bShowStatusBar ? MF_CHECKED : MF_UNCHECKED));
    EnableMenuItem(menu, CMD_UNDO,
        SendMessage(Globals.hEdit, EM_CANUNDO, 0, 0) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(menu, CMD_PASTE,
        IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED);
    enable = (int) SendMessage(Globals.hEdit, EM_GETSEL, 0, 0);
    enable = (HIWORD(enable) == LOWORD(enable)) ? MF_GRAYED : MF_ENABLED;
    EnableMenuItem(menu, CMD_CUT, enable);
    EnableMenuItem(menu, CMD_COPY, enable);
    EnableMenuItem(menu, CMD_DELETE, enable);

    EnableMenuItem(menu, CMD_SELECT_ALL,
        GetWindowTextLength(Globals.hEdit) ? MF_ENABLED : MF_GRAYED);

    if (index == 0)
        UpdateMenuRecentList(menuhit);
}

LRESULT CALLBACK EDIT_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            switch (wParam)
            {
                case VK_UP:
                case VK_DOWN:
                case VK_LEFT:
                case VK_RIGHT:
                    StatusBarUpdateCaretPos();
                    break;
                default:
                {
                    UpdateWindowCaption(FALSE);
                    break;
                }
            }
        }
        case WM_LBUTTONUP:
        {
            StatusBarUpdateCaretPos();
            break;
        }
    }
    return CallWindowProc( Globals.EditProc, hWnd, msg, wParam, lParam);
}

//***********************************************************************
//   WndProc
static LRESULT WINAPI
NOTEPAD_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {

    case WM_CREATE:
        Globals.hMainWnd = hWnd;
        Globals.hMenu = GetMenu(hWnd);

        DragAcceptFiles(hWnd, TRUE); /* Accept Drag & Drop */

        /* Create controls */
        CreateStatusTabControl();
        DIALOG_FileNew(); /* Initialize file info */
        DoShowHideStatusBar();

        // For now, the "Help" dialog is disabled due to the lack of HTML Help support
        EnableMenuItem(Globals.hMenu, CMD_HELP_CONTENTS, MF_BYCOMMAND | MF_GRAYED);
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == EN_HSCROLL || HIWORD(wParam) == EN_VSCROLL)
            StatusBarUpdateCaretPos();
        if ((HIWORD(wParam) == EN_CHANGE))
            EnableSearchMenu();
        NOTEPAD_MenuCommand(LOWORD(wParam));
        break;

    case WM_CLOSE:
        if (Globals.hEdit)
            DoCloseFile();

        DestroyWindow(hWnd);
        break;

    case WM_QUERYENDSESSION:
        if (DoCloseFile()) {
            return 1;
        }
        break;

    case WM_DESTROY:
        SaveAppSettings();

        if (Globals.pEditInfo)
            free(Globals.pEditInfo);
        if (Globals.hEdit)
            DestroyWindow(Globals.hEdit);

        if (Globals.hFont)
            DeleteObject(Globals.hFont);
        if (Globals.hDevMode)
            GlobalFree(Globals.hDevMode);
        if (Globals.hDevNames)
            GlobalFree(Globals.hDevNames);
        SetWindowLongPtr(Globals.hEdit, GWLP_WNDPROC, (LONG_PTR)Globals.EditProc);

        PostQuitMessage(0);
        break;

    case WM_SIZE:

        UpdateWindowLayout();
        break;

    /* The entire client area is covered by edit control and by
     * the status bar. So there is no need to erase main background.
     * This resolves the horrible flicker effect during windows resizes. */
    case WM_ERASEBKGND:
        return 1;

    case WM_SETFOCUS:
        CheckFileModeChange();
        SetFocus(Globals.hEdit);
        break;

    case WM_DROPFILES:
    {
        WCHAR szFileName[MAX_PATH];
        HDROP hDrop = (HDROP) wParam;

        DragQueryFile(hDrop, 0, szFileName, _countof(szFileName));
        DragFinish(hDrop);
        TryOpenFile(szFileName, FALSE);
        break;
    }

    case WM_INITMENUPOPUP:
        NOTEPAD_InitMenuPopup((HMENU)wParam, lParam);
        break;

    case WM_NOTIFY:
        {
            UINT nmc = ((LPNMHDR)lParam)->code;

            if ( nmc == TCN_SELCHANGE ) {
                OnTabChange();
                return TRUE;
            }
            else if ( nmc == TCN_SELCHANGING )
                // Return FALSE to allow the selection to change.
                return FALSE;
        }
        /*FALLTHRU*/

    default:
        if (msg == aFINDMSGSTRING)
        {
            EventSearchReplace((FINDREPLACE*)lParam);
            break;
        }
        
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

//***********************************************************************

static BOOL HandleCommandLine(LPWSTR cmdline)
{
    BOOL opt_print = FALSE;
    BOOL opt_readonly = FALSE;
    WCHAR szPath[MAX_PATH];

    while (*cmdline == L' ' || *cmdline == L'-' || *cmdline == L'/')
    {
        WCHAR option;

        if (*cmdline++ == L' ') continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline == L' ') cmdline++;

        switch(tolower(option))
        {
            case 'p':
                opt_print = TRUE;
                break;
            case 'v':
            case 'r':
                opt_readonly = TRUE;
                break;
        }
    }

    if (STROK(cmdline))
    {
        /* file name is passed in the command line */
        LPCWSTR file_name = NULL;
        BOOL file_exists = FALSE;
        WCHAR buf[MAX_PATH];

        if (cmdline[0] == L'"')
        {
            cmdline++;
            cmdline[lstrlen(cmdline) - 1] = 0;
        }

        file_name = cmdline;
        if (FileExists(file_name))
        {
            file_exists = TRUE;
        }
        else if (!HasFileExtension(cmdline))
        {
            static const WCHAR txt[] = L".txt";

            /* try to find file with ".txt" extension */
            if (!_tcscmp(txt, cmdline + _tcslen(cmdline) - _tcslen(txt)))
            {
                file_exists = FALSE;
            }
            else
            {
                _tcsncpy(buf, cmdline, MAX_PATH - _tcslen(txt) - 1);
                _tcscat(buf, txt);
                file_name = buf;
                file_exists = FileExists(file_name);
            }
        }

        GetFullPathName(file_name, _countof(szPath), szPath, NULL);

        if (file_exists)
        {
            TryOpenFile(szPath, FALSE);
 
            InvalidateRect(Globals.hMainWnd, NULL, FALSE);
            if (opt_print)
            {
                DIALOG_FilePrint();
                return FALSE;
            }
            else if ( opt_readonly )
            {
            }
        }
        else
        {
            switch (AlertFileNotExist(file_name))
            {
            case IDYES:
                TryOpenFile(szPath, TRUE);
                break;

            case IDNO:
                break;
            }
        }
    }

    return TRUE;
}

//***********************************************************************/
//            WinMain
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE prev, LPWSTR cmdline, int show)
{
    MSG msg;
    HACCEL hAccel;
    WNDCLASSEX wndclass;
    HMONITOR monitor;
    MONITORINFO info;
    INT x, y;
    RECT rcIntersect;
    static const WCHAR className[] = L"Notepad";
    static const WCHAR winName[] = L"Notepad";

#ifdef _DEBUG
    /* Report any memory leaks on exit */
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    switch (GetUserDefaultUILanguage())
    {
    case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
        SetProcessDefaultLayout(LAYOUT_RTL);
        break;

    default:
        break;
    }

    UNREFERENCED_PARAMETER(prev);

    aFINDMSGSTRING = (ATOM)RegisterWindowMessage(FINDMSGSTRING);

    ZEROMEM(wndclass);
    wndclass.cbSize = sizeof(wndclass);
    wndclass.lpfnWndProc = NOTEPAD_WndProc;
    wndclass.hInstance = Globals.hInstance;
    wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NPICON));
    wndclass.hCursor = LoadCursor(0, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszMenuName = MAKEINTRESOURCE(MAIN_MENU);
    wndclass.lpszClassName = className;
    wndclass.hIconSm = (HICON)LoadImage(hInstance,
                                        MAKEINTRESOURCE(IDI_NPICON),
                                        IMAGE_ICON,
                                        GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON),
                                        0);
    if (!RegisterClassEx(&wndclass))
    {
        ShowLastError();
        return 1;
    }

    /* Setup windows */
    NOTEPAD_InitData(hInstance);

    monitor = MonitorFromRect(&Settings.main_rect, MONITOR_DEFAULTTOPRIMARY);
    info.cbSize = sizeof(info);
    GetMonitorInfoW(monitor, &info);

    x = Settings.main_rect.left;
    y = Settings.main_rect.top;
    if (!IntersectRect(&rcIntersect, &Settings.main_rect, &info.rcWork))
        x = y = CW_USEDEFAULT;

    /* Globals.hMainWnd will be set in WM_CREATE handling */
    CreateWindow(className,
                 winName,
                 WS_OVERLAPPEDWINDOW,
                 x,
                 y,
                 Settings.main_rect.right - Settings.main_rect.left,
                 Settings.main_rect.bottom - Settings.main_rect.top,
                 NULL,
                 NULL,
                 Globals.hInstance,
                 NULL);
    if (!Globals.hMainWnd)
    {
        ShowLastError();
        return 1;
    }

    ShowWindow(Globals.hMainWnd, show);
    UpdateWindow(Globals.hMainWnd);

    if (!HandleCommandLine(cmdline))
        return 0;

    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(ID_ACCEL));

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(Globals.hMainWnd, hAccel, &msg) &&
            !IsDialogMessage(Globals.hFindReplaceDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DestroyAcceleratorTable(hAccel);

    return (int) msg.wParam;
}

