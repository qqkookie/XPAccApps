/*
 * PROJECT:    ReactOS Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 * COPYRIGHT:  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 *             Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *             Copyright 2002 Andriy Palamarchuk
 *             Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "notepad.h"

#include <commctrl.h>
#include <shellapi.h>

#include <assert.h>
#include <strsafe.h>

static const TCHAR helpfile[] = _T("notepad.hlp");
static const TCHAR szDefaultExt[] = _T("txt");
static const TCHAR txt_files[] = _T("*.txt");

/* Status bar parts index */
#define SBPART_CURPOS   0
#define SBPART_EOLN     1
#define SBPART_ENCODING 2

/* Line endings - string resource ID mapping table */
static UINT EolnToStrId[] = {
    STRING_LF,
    STRING_CRLF,
    STRING_CR,
};

/* Encoding - string resource ID mapping table */
static UINT EncToStrId[] = {
    STRING_UTF8,
    STRING_UTF8_BOM,
    STRING_ANSIOEM,
    STRING_UTF16,
    STRING_UTF16_BE,
};

#define     MAX_NTAB    9       // max number of tabs
#define     XSP         3       // edit window border thickness

static int TH_Height;
static int ST_Height;

/*********************************************************************/
// update window layout on WM_SIZE
VOID UpdateWindowLayout(VOID)
{
   if (!Globals.hwTabCtrl)
        return;

    if (Settings.bShowStatusBar)
    {
        RECT rcStatus;
        SendMessageW(Globals.hStatusBar, WM_SIZE, 0, 0);
        GetWindowRect(Globals.hStatusBar, &rcStatus);

        /* Align status bar parts, only if the status bar resize operation succeeds */
            static const int defaultWidths[] = {200, 200, 200};
        RECT rcStatusBar;
        int parts[3];

        GetClientRect(Globals.hStatusBar, &rcStatusBar);

        parts[0] = rcStatusBar.right - (defaultWidths[1] + defaultWidths[2]);
        parts[1] = rcStatusBar.right - defaultWidths[2];
        parts[2] = -1; // the right edge of the status bar

        parts[0] = max(parts[0], defaultWidths[0]);
        parts[1] = max(parts[1], defaultWidths[0] + defaultWidths[1]);

        SendMessageW(Globals.hStatusBar, SB_SETPARTS, _countof(parts), (LPARAM)parts);
    }

    // MoveWindow(Globals.hEdit, 0, 0, rc.right, rc.bottom, TRUE); 

    RECT rct;
    GetClientRect(Globals.hMainWnd, &rct);
    int ww = rct.right - rct.left;
    int hh = rct.bottom - rct.top;

    if (Settings.bShowStatusBar)
        hh -= ST_Height;

    ww -= XSP*2; hh -= XSP*2;
    MoveWindow(Globals.hwTabCtrl, XSP, XSP, ww, hh, TRUE);

    ww -= XSP*2; hh -= TH_Height + XSP*2;

    SetWindowPos(Globals.hEdit, NULL, XSP, TH_Height + XSP, ww, hh,
                SWP_NOOWNERZORDER | SWP_NOZORDER);
}

// --------------------------------------------------------------------
//   Status bar
VOID DoShowHideStatusBar(VOID)
{
    /* Check if status bar object already exists. */
    if (Globals.hStatusBar == NULL)
    {
        /* Try to create the status bar */
        Globals.hStatusBar = CreateStatusWindow(WS_CHILD | CCS_BOTTOM | SBARS_SIZEGRIP,
                                                NULL,
                                                Globals.hMainWnd,
                                                CMD_STATUSBAR_WND_ID);

        if (Globals.hStatusBar == NULL)
        {
            ShowLastError();
            return;
        }

        /* Load the string for formatting column/row text output */
        LOADSTRING( STRING_LINE_COLUMN, Globals.szStatusBarLineCol );
    }

    /* Update layout of controls */
    SendMessageW(Globals.hMainWnd, WM_SIZE, 0, 0);

    if (Globals.hStatusBar == NULL)
        return;

    /* Update visibility of status bar */
    ShowWindow(Globals.hStatusBar, (Settings.bShowStatusBar ? SW_SHOWNOACTIVATE : SW_HIDE));

    /* Update status bar contents */
    UpdateStatusBar();
}

VOID ToggleStatusBar(VOID)
{
    Settings.bShowStatusBar = !Settings.bShowStatusBar;
    DoShowHideStatusBar();
}

VOID UpdateStatusBar(VOID)
{
    StatusBarUpdateCaretPos();

    SendMessageW(Globals.hStatusBar, SB_SETTEXTW, SBPART_EOLN,
            (LPARAM) GETSTRING(EolnToStrId[Globals.iEoln]));

    SendMessageW(Globals.hStatusBar, SB_SETTEXTW, SBPART_ENCODING,
            (LPARAM) (Globals.encFile == ENCODING_AUTO ? _T("")
                : GETSTRING(EncToStrId[Globals.encFile])));
}

VOID StatusBarUpdateCaretPos(VOID)
{
    int line, col;
    TCHAR buff[MAX_PATH];
    DWORD dwStart, dwSize;

    SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwSize);
    line = SendMessage(Globals.hEdit, EM_LINEFROMCHAR, (WPARAM)dwStart, 0);
    col = dwStart - SendMessage(Globals.hEdit, EM_LINEINDEX, (WPARAM)line, 0);

    _tcscpy(buff, L"   ");
    _stprintf(buff+3, Globals.szStatusBarLineCol, line + 1, col + 1);
    SendMessage(Globals.hStatusBar, SB_SETTEXT, SBPART_CURPOS, (LPARAM)buff);
}

// --------------------------------------------------------------------
/**
 * Sets the caption of the main window according to Globals.szFileTitle:
 *    (untitled) - Notepad      if no file is open
 *    [filename] - Notepad      if a file is given
 */
VOID UpdateWindowCaption(BOOL clearModifyAlert)
{
    TCHAR szCaption[STR_LONG];
    TCHAR szFilename[MAX_PATH];
    BOOL isModified;

    if (clearModifyAlert)
    {
        /* When a file is being opened or created, there is no need to have
         * the edited flag shown when the file has not been edited yet. */
        isModified = FALSE;
    }
    else
    {
        /* Check whether the user has modified the file or not. If we are
         * in the same state as before, don't change the caption. */
        isModified = (BOOL) SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0);
        if (isModified == Globals.bWasModified)
            return;
    }

    /* Remember the state for later calls */
    Globals.bWasModified = isModified;

    /* Determine if the file has been saved or if this is a new file */
    StringCchCopy(szFilename, _countof(szFilename),
        Globals.szFileTitle[0] ? Globals.szFileTitle : GETSTRING(STRING_UNTITLED));

    /* Update the window caption based upon whether the user has modified the file or not */
    StringCchPrintf(szCaption, _countof(szCaption), _T("%s%s - %s"),
                   (isModified ? _T("*") : _T("")), szFilename, G_STR_NOTEPAD);

    SetWindowText(Globals.hMainWnd, szCaption);
}

/*********************************************************************/

//   Edit window

#define EDIT_STYLE_WRAP (WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL)
#define EDIT_STYLE      (EDIT_STYLE_WRAP | WS_HSCROLL | ES_AUTOHSCROLL)
#define EDIT_CLASS      _T("EDIT")

LRESULT CALLBACK EDIT_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

VOID DoCreateEditWindow(VOID)
{
    DWORD dwStyle;
    int iSize;
    LPTSTR pTemp = NULL;
    BOOL bModified = FALSE;

    iSize = 0;

    /* If the edit control already exists, try to save its content */
    if (Globals.hEdit != NULL)
    {
        /* number of chars currently written into the editor. */
        iSize = GetWindowTextLength(Globals.hEdit);
        if (iSize)
        {
            /* Allocates temporary buffer. */
            pTemp = HeapAlloc(GetProcessHeap(), 0, (iSize + 1) * sizeof(TCHAR));
            if (!pTemp)
            {
                ShowLastError();
                return;
            }

            /* Recover the text into the control. */
            GetWindowText(Globals.hEdit, pTemp, iSize + 1);

            if (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0))
                bModified = TRUE;
        }

        /* Restore original window procedure */
        SetWindowLongPtr(Globals.hEdit, GWLP_WNDPROC, (LONG_PTR)Globals.EditProc);

        /* Destroy the edit control */
        DestroyWindow(Globals.hEdit);
    }

    /* Update wrap status into the main menu and recover style flags */
    dwStyle =  EDIT_STYLE_WRAP| WS_VISIBLE | WS_BORDER;
    if (!Settings.bWrapLongLines)
        dwStyle |= EDIT_STYLE;

    /* Create the new edit control */
    Globals.hEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
                                   EDIT_CLASS,
                                   NULL,
                                   dwStyle,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   Globals.hwTabCtrl,
                                   NULL,
                                   Globals.hInstance,
                                   NULL);
    if (Globals.hEdit == NULL)
    {
        if (pTemp)
        {
            HeapFree(GetProcessHeap(), 0, pTemp);
        }

        ShowLastError();
        return;
    }

    SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);
    SendMessage(Globals.hEdit, EM_LIMITTEXT, 0, 0);

    /* If some text was previously saved, restore it. */
    if (iSize != 0)
    {
        SetWindowText(Globals.hEdit, pTemp);
        HeapFree(GetProcessHeap(), 0, pTemp);

        if (bModified)
            SendMessage(Globals.hEdit, EM_SETMODIFY, TRUE, 0);
    }

    /* Sub-class a new window callback for row/column detection. */
    Globals.EditProc = (WNDPROC)SetWindowLongPtr(Globals.hEdit,
                                                 GWLP_WNDPROC,
                                                 (LONG_PTR)EDIT_WndProc);

    /* Finally shows new edit control and set focus into it. */
    ShowWindow(Globals.hEdit, SW_SHOW);
    SetFocus(Globals.hEdit);

    /* Re-arrange controls */
    PostMessageW(Globals.hMainWnd, WM_SIZE, 0, 0);
}

// Add new tab and new Edit Control on the tab.
BOOL AddNewEditTab(VOID)
{
    assert(Globals.hwTabCtrl);

    int ntab = TabCtrl_GetItemCount(Globals.hwTabCtrl);
    if ( ntab >= MAX_NTAB)
        return FALSE;

    ShowWindow(Globals.hEdit, SW_HIDE);

    Globals.hEdit = NULL;
    DoCreateEditWindow();

    ShowWindow(Globals.hEdit, SW_SHOW);

    TCITEM tab;
    ZEROMEM(tab);
    tab.mask = TCIF_TEXT|TCIF_IMAGE|TCIF_PARAM;

    EDITINFO* pedi = calloc(1, sizeof(EDITINFO));
    pedi->cbSize = sizeof(EDITINFO);
    pedi->hwEDIT = Globals.hEdit;

    Globals.pEditInfo = pedi;

    tab.lParam = (LPARAM) pedi;
    tab.pszText = Globals.szFileTitle;
    tab.iImage = -1;

    TabCtrl_InsertItem(Globals.hwTabCtrl, ntab, &tab);
    TabCtrl_SetCurSel(Globals.hwTabCtrl, ntab);
    SetTabHeader();

    return TRUE;
}

// --------------------------------------------------------------------
//   Multi-Tab service 

// Creates a tab control. Returns TRUE on success.
// Measure tab header height, status height. 
BOOL DoCreateTabControl(VOID)
{
    // Get the dimensions of the parent window's client area,
    //  and create a tab control child window of that size.
    Globals.hwTabCtrl = CreateWindow(WC_TABCONTROL, L"", 
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE|TCS_FOCUSNEVER, 
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
        Globals.hMainWnd, NULL, Globals.hInstance, NULL);

    RECT rcTab, rcStatus;
    
    TabCtrl_GetItemRect(Globals.hwTabCtrl, 0, &rcTab);
    TH_Height = rcTab.bottom - rcTab.top + XSP;

    GetWindowRect(Globals.hStatusBar, &rcStatus);
    ST_Height = rcStatus.bottom - rcStatus.top;

    return Globals.hwTabCtrl != NULL;
}

// Context switching chore after tab change.
VOID OnTabChange(VOID)
{
    ShowWindow(Globals.hEdit, SW_HIDE);

    int iPage = TabCtrl_GetCurSel(Globals.hwTabCtrl);

    TCITEM tab;
    ZEROMEM(tab);
    tab.mask =  TCIF_TEXT|TCIF_PARAM;
    tab.pszText = Globals.szFileTitle;
    tab.cchTextMax = _countof(Globals.szFileTitle);
    Globals.szFileName[0] = Globals.szFileTitle[0] = L'0';

    TabCtrl_GetItem(Globals.hwTabCtrl, iPage, (LPARAM) &tab);

    EDITINFO * pedi = (EDITINFO *) tab.lParam;

    // Restore global states of selected edit control. 
    Globals.hEdit = pedi->hwEDIT;
    StringCchCopy( Globals.szFileName, _countof(Globals.szFileName), pedi->filePath );
    Globals.encFile = pedi->encFile;
    Globals.iEoln = pedi->iEoln;
    Globals.bWasModified = pedi->isModified;

    Globals.pEditInfo = pedi;

    ShowWindow(Globals.hEdit, SW_SHOW);
    UpdateWindow(Globals.hEdit);

    if ( Settings.bShowStatusBar )
    {
        UpdateStatusBar();
    }

    UpdateWindowCaption(FALSE);
}

// Set tab header
VOID SetTabHeader(VOID)
{
    TCHAR buf[MAX_PATH];
    int len = _tcslen(Globals.szFileTitle);

    StringCchPrintf( buf, MAX_PATH, (len < 10 ? L"    %s    " : L"  %s  "), Globals.szFileTitle);
    ShortenPath(buf, 0);

    TCITEM tab;
    ZEROMEM(tab);
    tab.mask = TCIF_TEXT|TCIF_IMAGE;
    tab.pszText = (LPTSTR) buf;
    tab.iImage = -1;

    int iPage = TabCtrl_GetCurSel(Globals.hwTabCtrl);
    TabCtrl_SetItem(Globals.hwTabCtrl, iPage, &tab);
}

// Check for file is duplicate (already editing)
// Returns Tab index (zero-based) on matching duplicate file already loaed, -1 on not duplicate.
// Caveats: very rudimentary path name comparison for match checking.
int FindDupPathTab(LPCTSTR filePath)
{
    int ntab = TabCtrl_GetItemCount(Globals.hwTabCtrl);

    TCITEM tab;
    ZEROMEM(tab);
    tab.mask =  TCIF_PARAM;
    EDITINFO *pedi = NULL;

    for ( int iPage = 0 ; iPage < ntab; iPage++ )
    {
        TabCtrl_GetItem(Globals.hwTabCtrl, iPage, (LPARAM) &tab);
        pedi = (EDITINFO *) tab.lParam;
        if ( pedi && pedi->pathOK && pedi->filePath[0]
            && _tcsicmp(filePath, pedi->filePath) == 0 )    // very rudimentary!
        {
            TabCtrl_SetCurSel(Globals.hwTabCtrl, iPage);
            OnTabChange();
            return (iPage);
        }
    }
    return -1;
}

/*********************************************************************/
//   File Open dialog

VOID DIALOG_FileNew(VOID)
{
    LOADSTRING(STRING_UNTITLED, Globals.szFileTitle );

    AddNewEditTab();
    SetWindowText(Globals.hEdit, NULL);
    SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
    Globals.iEoln = EOLN_CRLF;
    Globals.encFile = ENCODING_DEFAULT;
    
    EnableSearchMenu();
    UpdateStatusBar();
    UpdateWindowCaption(TRUE);
}

VOID DIALOG_FileNewWindow(VOID)
{
    TCHAR pszNotepadExe[MAX_PATH];
    GetModuleFileName(NULL, pszNotepadExe, _countof(pszNotepadExe));
    ShellExecute(NULL, NULL, pszNotepadExe, NULL, NULL, SW_SHOWNORMAL);
}

VOID DIALOG_FileOpen(VOID)
{
    OPENFILENAME openfilename;
    TCHAR szPath[MAX_PATH];

    ZEROMEM(openfilename);

    if (Globals.szFileName[0] == 0)
        _tcscpy(szPath, txt_files);
    else
        _tcscpy(szPath, Globals.szFileName);

    openfilename.lStructSize = sizeof(openfilename);
    openfilename.hwndOwner = Globals.hMainWnd;
    openfilename.hInstance = Globals.hInstance;
    openfilename.lpstrFilter = Globals.szFilter;
    openfilename.lpstrFile = szPath;
    openfilename.nMaxFile = _countof(szPath);
    openfilename.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    openfilename.lpstrDefExt = szDefaultExt;

    if (GetOpenFileName(&openfilename)) {
        if (FileExists(openfilename.lpstrFile))
            DoOpenFile(openfilename.lpstrFile);
        else
            AlertFileNotFound(openfilename.lpstrFile);
    }
}

VOID DIALOG_MenuRecent(int menu_id)
{
    const TCHAR *path = MRU_Enum( menu_id -MENU_RECENT -1);
    if ( path && path[0])
        DoOpenFile(path);
}

VOID DIALOG_EditWrap(VOID)
{
    Settings.bWrapLongLines = !Settings.bWrapLongLines;

    EnableMenuItem(Globals.hMenu, CMD_GOTO, (Settings.bWrapLongLines ? MF_GRAYED : MF_ENABLED));

    DoCreateEditWindow();
    DoShowHideStatusBar();
}

// --------------------------------------------------------------------
//    File Save dialog
BOOL DIALOG_FileSave(VOID)
{
    if (Globals.szFileName[0] == 0)
    {
        return DIALOG_FileSaveAs();
    }
    else if (DoSaveFile())
    {
        UpdateWindowCaption(TRUE);
        return TRUE;
    }
    return FALSE;
}

static UINT_PTR
CALLBACK
DIALOG_FileSaveAs_Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hCombo;

    UNREFERENCED_PARAMETER(wParam);

    switch(msg)
    {
        case WM_INITDIALOG:
            hCombo = GetDlgItem(hDlg, ID_ENCODING);

            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) GETSTRING(STRING_UTF8) );
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) GETSTRING(STRING_UTF8_BOM));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) GETSTRING(STRING_ANSIOEM));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) GETSTRING(STRING_UTF16));
//            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) GETSTRING(STRING_UTF16_BE));

            SendMessage(hCombo, CB_SETCURSEL, Globals.encFile, 0);

            hCombo = GetDlgItem(hDlg, ID_EOLN);

            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) GETSTRING(STRING_LF));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) GETSTRING(STRING_CRLF));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) GETSTRING(STRING_CR));

            SendMessage(hCombo, CB_SETCURSEL, Globals.iEoln, 0);
            break;

        case WM_NOTIFY:
            if (((NMHDR *) lParam)->code == CDN_FILEOK)
            {
                hCombo = GetDlgItem(hDlg, ID_ENCODING);
                if (hCombo)
                    Globals.encFile = (ENCODING) SendMessage(hCombo, CB_GETCURSEL, 0, 0);

                hCombo = GetDlgItem(hDlg, ID_EOLN);
                if (hCombo)
                    Globals.iEoln = (EOLN)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
            }
            break;
    }
    return 0;
}

#include <time.h>

BOOL DIALOG_FileSaveAs(VOID)
{
    OPENFILENAME saveas;
    TCHAR szPath[MAX_PATH];

    ZEROMEM(saveas);

    if (STRBAD(Globals.szFileName))
    { 
        TCHAR *p = szPath + LOADSTRING(STRING_UNTITLED, szPath);;
        time_t t = time(NULL);
        _tcsftime(p, MAX_PATH, L"-%m%d%H%M.txt", localtime(&t));
    }
    else
        _tcscpy(szPath, Globals.szFileName);

    saveas.lStructSize = sizeof(OPENFILENAME);
    saveas.hwndOwner = Globals.hMainWnd;
    saveas.hInstance = Globals.hInstance;
    saveas.lpstrFilter = Globals.szFilter;
    saveas.lpstrFile = szPath;
    saveas.nMaxFile = _countof(szPath);
    saveas.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY |
                   OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
    saveas.lpstrDefExt = szDefaultExt;
    saveas.lpTemplateName = MAKEINTRESOURCE(DIALOG_ENCODING);
    saveas.lpfnHook = DIALOG_FileSaveAs_Hook;

    if (GetSaveFileName(&saveas))
    {
        /* HACK: Because in ROS, Save-As boxes don't check the validity
         * of file names and thus, here, szPath can be invalid !! We only
         * see its validity when we call DoSaveFile()... */
        SetFileName(szPath);
        if (DoSaveFile())
        {
            SetTabHeader();
            UpdateWindowCaption(TRUE);
            UpdateStatusBar();
            return TRUE;
        }
        else
        {
            SetFileName(NULSTR);
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

// Close current file and tab. Delete the tab.
VOID DIALOG_FileClose(VOID)
{
    if (DoCloseFile() == FALSE)
        return;

    int iPage = TabCtrl_GetCurSel(Globals.hwTabCtrl);
    TabCtrl_DeleteItem(Globals.hwTabCtrl, iPage);
    int ntab = TabCtrl_GetItemCount(Globals.hwTabCtrl);

    if (ntab == 0)
    {
        PostMessage(Globals.hMainWnd, WM_CLOSE, 0, 0);
        return;
    }

    TabCtrl_SetCurSel(Globals.hwTabCtrl, ((iPage == ntab) ? iPage-1 : iPage));
    OnTabChange();
}

VOID DIALOG_FileExit(VOID)
{
    if (DoCloseAllFiles())
        PostMessage(Globals.hMainWnd, WM_CLOSE, 0, 0);
}

// --------------------------------------------------------------------
// Misc commands

VOID DIALOG_EditUndo(VOID)
{
    SendMessage(Globals.hEdit, EM_UNDO, 0, 0);
}

VOID DIALOG_EditCut(VOID)
{
    SendMessage(Globals.hEdit, WM_CUT, 0, 0);
}

VOID DIALOG_EditCopy(VOID)
{
    SendMessage(Globals.hEdit, WM_COPY, 0, 0);
}

VOID DIALOG_EditPaste(VOID)
{
    SendMessage(Globals.hEdit, WM_PASTE, 0, 0);
}

VOID DIALOG_EditDelete(VOID)
{
    SendMessage(Globals.hEdit, WM_CLEAR, 0, 0);
}

VOID DIALOG_EditSelectAll(VOID)
{
    SendMessage(Globals.hEdit, EM_SETSEL, 0, -1);
}

VOID DIALOG_EditTimeDate(VOID)
{
    SYSTEMTIME st;
    TCHAR szDate[STR_SHORT];
    TCHAR szText[STR_LONG];

    GetLocalTime(&st);

    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szDate, STR_SHORT);
    _tcscpy(szText, szDate);
    _tcscat(szText, _T(" "));
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, szDate, STR_SHORT);
    _tcscat(szText, szDate);
    SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)szText);
}

/*********************************************************************/
// --------------------------------------------------------------------
// Search Events

VOID EnableSearchMenu(VOID)
{
    BOOL bEmpty = (GetWindowTextLengthW(Globals.hEdit) == 0);
    UINT uEnable = MF_BYCOMMAND | (bEmpty ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(Globals.hMenu, CMD_SEARCH, uEnable);
    EnableMenuItem(Globals.hMenu, CMD_SEARCH_NEXT, uEnable);
    EnableMenuItem(Globals.hMenu, CMD_SEARCH_PREV, uEnable);
}

typedef HWND (WINAPI *FINDPROC)(LPFINDREPLACE lpfr);

static VOID DIALOG_SearchDialog(FINDPROC pfnProc)
{
    if (Globals.hFindReplaceDlg != NULL)
    {
        SetFocus(Globals.hFindReplaceDlg);
        return;
    }

    if (!Globals.find.lpstrFindWhat)
    {
        ZEROMEM(Globals.find);
        Globals.find.lStructSize = sizeof(Globals.find);
        Globals.find.hwndOwner = Globals.hMainWnd;
        Globals.find.lpstrFindWhat = Settings.szFindText;
        Globals.find.wFindWhatLen = _countof(Settings.szFindText);
        Globals.find.lpstrReplaceWith = Settings.szReplaceText;
        Globals.find.wReplaceWithLen = _countof(Settings.szReplaceText);
        Globals.find.Flags = FR_DOWN;
    }

    /* We only need to create the modal FindReplace dialog which will */
    /* notify us of incoming events using hMainWnd Window Messages    */

    Globals.hFindReplaceDlg = pfnProc(&Globals.find);
    assert(Globals.hFindReplaceDlg != NULL);
}

VOID DIALOG_Search(VOID)
{
    DIALOG_SearchDialog(FindText);
}

VOID DIALOG_SearchNext(BOOL bDown)
{
    if (bDown)
        Globals.find.Flags |= FR_DOWN;
    else
        Globals.find.Flags &= ~FR_DOWN;

    if (Globals.find.lpstrFindWhat != NULL)
        Search_FindNext(&Globals.find, FALSE, TRUE);
    else
        DIALOG_Search();
}

VOID DIALOG_Replace(VOID)
{
    DIALOG_SearchDialog(ReplaceText);
}

// --------------------------------------------------------------------
// GoTo dialog

typedef struct tagGOTO_DATA
{
    UINT iLine;
    UINT cLines;
} GOTO_DATA, *PGOTO_DATA;

static INT_PTR
CALLBACK
DIALOG_GoTo_DialogProc(HWND hwndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PGOTO_DATA s_pGotoData;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            s_pGotoData = (PGOTO_DATA)lParam;
            SetDlgItemInt(hwndDialog, ID_LINENUMBER, s_pGotoData->iLine, FALSE);
            return TRUE; /* Set focus */

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK)
            {
                UINT iLine = GetDlgItemInt(hwndDialog, ID_LINENUMBER, NULL, FALSE);
                if (iLine <= 0 || s_pGotoData->cLines < iLine) /* Out of range */
                {
                    /* Show error message */
                    MessageBoxW(hwndDialog, GETSTRING(STRING_LINE_NUMBER_OUT_OF_RANGE),
                        G_STR_NOTEPAD, MB_OK);

                    SendDlgItemMessageW(hwndDialog, ID_LINENUMBER, EM_SETSEL, 0, -1);
                    SetFocus(GetDlgItem(hwndDialog, ID_LINENUMBER));
                    break;
                }
                s_pGotoData->iLine = iLine;
                EndDialog(hwndDialog, IDOK);
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDialog, IDCANCEL);
            }
            break;
        }
    }

    return 0;
}

VOID DIALOG_GoTo(VOID)
{
    GOTO_DATA GotoData;
    DWORD dwStart = 0, dwEnd = 0;
    INT ich, cch = GetWindowTextLength(Globals.hEdit);

    /* Get the current line number and the total line number */
    SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM) &dwStart, (LPARAM) &dwEnd);
    GotoData.iLine = (UINT)SendMessage(Globals.hEdit, EM_LINEFROMCHAR, dwStart, 0) + 1;
    GotoData.cLines = (UINT)SendMessage(Globals.hEdit, EM_GETLINECOUNT, 0, 0);

    /* Ask the user for line number */
    if (DialogBoxParam(Globals.hInstance,
                       MAKEINTRESOURCE(DIALOG_GOTO),
                       Globals.hMainWnd,
                       DIALOG_GoTo_DialogProc,
                       (LPARAM)&GotoData) != IDOK)
    {
        return; /* Canceled */
    }

    --GotoData.iLine; /* Make it zero-based */

    /* Get ich (the target character index) from line number */
    if (GotoData.iLine <= 0)
        ich = 0;
    else if (GotoData.iLine >= GotoData.cLines)
        ich = cch;
    else
        ich = (INT)SendMessage(Globals.hEdit, EM_LINEINDEX, GotoData.iLine, 0);

    /* Move the caret */
    SendMessage(Globals.hEdit, EM_SETSEL, ich, ich);
    SendMessage(Globals.hEdit, EM_SCROLLCARET, 0, 0);
}

// --------------------------------------------------------------------
// Miscs

VOID DIALOG_SelectFont(VOID)
{
    CHOOSEFONT cf;
    LOGFONT lf = Settings.lfFont;

    ZEROMEM( cf );
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = Globals.hMainWnd;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS;

    if (ChooseFont(&cf))
    {
        HFONT currfont = Globals.hFont;

        Globals.hFont = CreateFontIndirect(&lf);
        Settings.lfFont = lf;
        SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, TRUE);
        if (currfont != NULL)
            DeleteObject(currfont);
    }
}

VOID DIALOG_HelpContents(VOID)
{
    WinHelp(Globals.hMainWnd, helpfile, HELP_INDEX, 0);
}

VOID DIALOG_HelpAboutNotepad(VOID)
{
    ShellAbout(Globals.hMainWnd, G_STR_NOTEPAD, GETSTRING(STRING_NOTEPAD_AUTHORS),
               LoadIcon(Globals.hInstance, MAKEINTRESOURCE(IDI_NPICON)));
}

/***********************************************************************/
//    Error message box

VOID ShowLastError(VOID)
{
    DWORD error = GetLastError();
    if (error == NO_ERROR)
        return;

    LPTSTR lpMsgBuf = NULL;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, error, 0, (LPTSTR) &lpMsgBuf, 0, NULL);

    MessageBox(Globals.hMainWnd, lpMsgBuf, GETSTRING(STRING_ERROR), MB_OK | MB_ICONERROR);
    LocalFree(lpMsgBuf);
}

int StringMsgBox( int formatId, LPCTSTR szString, DWORD dwFlags)
{
    TCHAR szMessage[STR_LONG];

    /* Load and format szMessage */;
    _sntprintf(szMessage, _countof(szMessage), GETSTRING(formatId), szString);
         
    LPCTSTR title = ((dwFlags & MB_ICONMASK) == MB_ICONEXCLAMATION)
        ? GETSTRING(STRING_ERROR) : G_STR_NOTEPAD;
    return MessageBox(Globals.hMainWnd, szMessage, title, dwFlags);
}

VOID AlertFileNotFound(LPCTSTR szFileName)
{
    StringMsgBox(STRING_NOTFOUND, szFileName, MB_ICONEXCLAMATION | MB_OK);
}

// file not exist, create? Y/N
int AlertFileNotExist(LPCTSTR szFileName)
{
    return StringMsgBox(STRING_DOESNOTEXIST,
                szFileName, MB_ICONEXCLAMATION | MB_YESNO);
}

// file not saved, save? Y/N/Cancel
int AlertFileNotSaved(LPCTSTR szFileName)
{
    return StringMsgBox(STRING_NOTSAVED,
            szFileName[0] ? szFileName : GETSTRING(STRING_UNTITLED),
            MB_ICONQUESTION | MB_YESNOCANCEL);
}

// Ansi data loss, abort/retry/cancel
int AlertUnicodeCharactersLost(LPCWSTR szFileName)
{
    TCHAR szEnc[STR_SHORT];
    TCHAR* szMsg;
    DWORD_PTR args[2];
    int rc;

    LOADSTRING( STRING_ANSIOEM, szEnc);

    args[0] = (DWORD_PTR)szFileName;
    args[1] = (DWORD_PTR)szEnc;
    FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_ARGUMENT_ARRAY,
            GETSTRING(STRING_LOSS_OF_UNICODE_CHARACTERS), 0, 0, (LPWSTR)&szMsg, 0, (va_list *)args);

    rc = MessageBox(Globals.hMainWnd, szMsg, G_STR_NOTEPAD,
            MB_ABORTRETRYIGNORE|MB_DEFBUTTON2|MB_ICONWARNING);
    LocalFree(szMsg);
    return rc;
}
