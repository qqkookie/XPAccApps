/*
 * PROJECT:    WinXPAccApps Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 */

#include "notepad.h"

#include <shlobj.h>

#include <assert.h>
#include <strsafe.h>

/**********************************************************************/
// File Open Close Save

// Sets Global File Name.
VOID SetFileName(LPCTSTR szFileName)
{
    if ( szFileName && szFileName[0])
        StringCchCopy(Globals.szFileName, _countof(Globals.szFileName), szFileName);
    else 
        LOADSTRING( STRING_UNTITLED, Globals.szFileName );

    Globals.szFileTitle[0] = 0;
    GetFileTitle(szFileName, Globals.szFileTitle, _countof(Globals.szFileTitle));

    if (szFileName && szFileName[0])
        SHAddToRecentDocs(SHARD_PATHW, szFileName);
}

VOID DoOpenFile(LPCTSTR szFileName)
{
    HANDLE hFile;
    TCHAR log[5];
    HLOCAL hLocal;

    /* Close any files and prompt to save changes */
    // if (!DoCloseFile())
    //     return;

    if (FindDupPathTab(szFileName) != -1)
        return;

    hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowLastError();
        goto done;
    }

    BOOL preserve = Globals.szFileName[0]        // old filename
        || SendMessage(Globals.hEdit, EM_GETMODIFY, TRUE, 0); 

    SetFileName(szFileName);                   // new filename
    MRU_Add(szFileName);

    if (preserve)
    {
        AddNewEditTab();
    }
    else
    {
        SetTabHeader();
    }

    StringCchCopy(Globals.pEditInfo->filePath, _countof(Globals.pEditInfo->filePath), szFileName);
    Globals.pEditInfo->pathOK = TRUE;

    /* To make loading file quicker, we use the internal handle of EDIT control */
    hLocal = (HLOCAL)SendMessageW(Globals.hEdit, EM_GETHANDLE, 0, 0);
    if (!ReadText(hFile, &hLocal, &Globals.encFile, &Globals.iEoln))
    {
        ShowLastError();
        goto done;
    }

    Globals.pEditInfo->encFile = Globals.encFile;
    Globals.pEditInfo->iEoln = Globals.iEoln;

    SendMessageW(Globals.hEdit, EM_SETHANDLE, (WPARAM)hLocal, 0);
    /* No need of EM_SETMODIFY and EM_EMPTYUNDOBUFFER here. EM_SETHANDLE does instead. */

    SetFocus(Globals.hEdit);

    /*  If the file starts with .LOG, add a time/date at the end and set cursor after
     *  See http://web.archive.org/web/20090627165105/http://support.microsoft.com/kb/260563
     */
    if (GetWindowText(Globals.hEdit, log, _countof(log)) && !_tcscmp(log, _T(".LOG")))
    {
        static const TCHAR lf[] = _T("\r\n");
        SendMessage(Globals.hEdit, EM_SETSEL, GetWindowTextLength(Globals.hEdit), -1);
        SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)lf);
        DIALOG_EditTimeDate();
        SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)lf);
    }

    // SetFileName(szFileName);
    UpdateWindowCaption(TRUE);
    EnableSearchMenu();
    UpdateStatusBar();

done:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
}

// --------------------------------------------------------------------
BOOL DoSaveFile(VOID)
{
    BOOL bRet = FALSE;
    HANDLE hFile;
    DWORD cchText;

    cchText = GetWindowTextLengthW(Globals.hEdit);
    if (cchText <= 0)
        return TRUE;

    if ( Globals.encFile == ENCODING_ANSIOEM)
    {
        // test for conversion error to ANSI/OEM CP
        int len = min(cchText, 10000);
        TCHAR *txt = malloc(sizeof(TCHAR)* (len+1));
        int cctxt = GetWindowTextW(Globals.hEdit, txt, len);
        int conv_error; 
        WideCharToMultiByte(CP_OEMCP, WC_NO_BEST_FIT_CHARS, txt, cctxt,
            NULL, 0, NULL, &conv_error);
        free(txt);
        if (conv_error )
        {
            int choice =  AlertUnicodeCharactersLost(Globals.szFileName);
            if ( choice == IDRETRY)
            {
                return DIALOG_FileSaveAs();
            }
            else if ( choice == IDABORT)
                return FALSE;
        }
    }

    HLOCAL hLocal = (HLOCAL)SendMessageW(Globals.hEdit, EM_GETHANDLE, 0, 0);
    LPWSTR pszText = LocalLock(hLocal);
    if (!pszText)
    {
        ShowLastError();
        return FALSE;
    }
  
    hFile = CreateFileW(Globals.szFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
                    NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bRet = WriteText(hFile, pszText, cchText, Globals.encFile, Globals.iEoln);
        if (bRet)
            SetEndOfFile(hFile);
        CloseHandle(hFile);
    }

    LocalUnlock(hLocal);

    if (bRet)
    {
        SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0);
        SetFileName(Globals.szFileName);
        MRU_Add(Globals.szFileName);
    }
    else
    {
        ShowLastError();
    }
    return bRet;
}

/**
 * Returns:
 *   TRUE  - User agreed to close (both save/don't save)
 *   FALSE - User cancelled close by selecting "Cancel"
 */
BOOL DoCloseFile(VOID)
{
    int nResult;

    if (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0))
    {
        /* prompt user to save changes */
        nResult = AlertFileNotSaved(Globals.szFileName);
        switch (nResult)
        {
            case IDYES:
                if(!DIALOG_FileSave())
                    return FALSE;
                break;

            case IDNO:
                break;

            case IDCANCEL:
            default:
                return FALSE;
        }
    }

    SetFileName(NULSTR);
    UpdateWindowCaption(TRUE);

    DestroyWindow(Globals.hEdit);
    free(Globals.pEditInfo);
    Globals.hEdit = NULL;
    Globals.pEditInfo = NULL;

    return TRUE;
}

// Cloase all open files and ask to save unsaved files.
BOOL DoCloseAllFiles(VOID)
{
    int ntab = TabCtrl_GetItemCount(Globals.hwTabCtrl);
    for ( int iPage = ntab-1 ; iPage >=0; iPage-- )
    {
        TabCtrl_SetCurSel(Globals.hwTabCtrl, iPage);
        OnTabChange();
        if (!DoCloseFile())
            return FALSE;  // user canceled close all
    }
    return TRUE;
}

/**********************************************************************/
// Search FindNext

static BOOL FindTextAt(FINDREPLACE *pFindReplace, LPCTSTR pszText, INT iTextLength, DWORD dwPosition);

BOOL Search_FindNext(FINDREPLACE *pFindReplace, BOOL bReplace, BOOL bShowAlert)
{
    int iTextLength, iTargetLength;
    size_t iAdjustment = 0;
    LPTSTR pszText = NULL;
    DWORD dwPosition, dwBegin, dwEnd;
    BOOL bMatches = FALSE;
    TCHAR szText[STR_LONG];
    BOOL bSuccess;

    iTargetLength = (int) _tcslen(pFindReplace->lpstrFindWhat);

    /* Retrieve the window text */
    iTextLength = GetWindowTextLength(Globals.hEdit);
    if (iTextLength > 0)
    {
        pszText = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, (iTextLength + 1) * sizeof(TCHAR));
        if (!pszText)
            return FALSE;

        GetWindowText(Globals.hEdit, pszText, iTextLength + 1);
    }

    SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM) &dwBegin, (LPARAM) &dwEnd);
    if (bReplace && ((dwEnd - dwBegin) == (DWORD) iTargetLength))
    {
        if (FindTextAt(pFindReplace, pszText, iTextLength, dwBegin))
        {
            SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM) pFindReplace->lpstrReplaceWith);
            iAdjustment = _tcslen(pFindReplace->lpstrReplaceWith) - (dwEnd - dwBegin);
        }
    }

    if (pFindReplace->Flags & FR_DOWN)
    {
        /* Find Down */
        dwPosition = dwEnd;
        while(dwPosition < (DWORD) iTextLength)
        {
            bMatches = FindTextAt(pFindReplace, pszText, iTextLength, dwPosition);
            if (bMatches)
                break;
            dwPosition++;
        }
    }
    else
    {
        /* Find Up */
        dwPosition = dwBegin;
        while(dwPosition > 0)
        {
            dwPosition--;
            bMatches = FindTextAt(pFindReplace, pszText, iTextLength, dwPosition);
            if (bMatches)
                break;
        }
    }

    if (bMatches)
    {
        /* Found target */
        if (dwPosition > dwBegin)
            dwPosition += (DWORD) iAdjustment;
        SendMessage(Globals.hEdit, EM_SETSEL, dwPosition, dwPosition + iTargetLength);
        SendMessage(Globals.hEdit, EM_SCROLLCARET, 0, 0);
        bSuccess = TRUE;
    }
    else
    {
        /* Can't find target */
        if (bShowAlert)
        {
            _sntprintf(szText, _countof(szText), GETSTRING(STRING_CANNOTFIND), pFindReplace->lpstrFindWhat);
            int retry = MessageBox(Globals.hFindReplaceDlg, szText, G_STR_NOTEPAD,
                MB_RETRYCANCEL|MB_ICONQUESTION|MB_DEFBUTTON2);
            if (retry == IDRETRY)
            {
                if (pFindReplace->Flags & FR_DOWN)
                {
                    /* Move the caret */
                    SendMessage(Globals.hEdit, EM_SETSEL, 0, 0);
                    SendMessage(Globals.hEdit, EM_SCROLLCARET, 0, 0);
                    DIALOG_SearchNext(TRUE);
                }
                else if ( pFindReplace->Flags & ~FR_DOWN)
                {
                    int last = GetWindowTextLength(Globals.hEdit);
                    SendMessage(Globals.hEdit, EM_SETSEL, last, last);
                    SendMessage(Globals.hEdit, EM_SCROLLCARET, 0, 0);
                    DIALOG_SearchNext(FALSE);
                }
            }
                
        }
        bSuccess = FALSE;
    }

    if (pszText)
        HeapFree(GetProcessHeap(), 0, pszText);
    return bSuccess;
}

// --------------------------------------------------------------------
static BOOL FindTextAt(FINDREPLACE *pFindReplace, LPCTSTR pszText, INT iTextLength, DWORD dwPosition)
{
    BOOL bMatches;
    size_t iTargetLength;
    LPCTSTR pchPosition;

    if (!pFindReplace || !pszText)
        return FALSE;

    iTargetLength = _tcslen(pFindReplace->lpstrFindWhat);
    pchPosition = &pszText[dwPosition];

    /* Make proper comparison */
    if (pFindReplace->Flags & FR_MATCHCASE)
        bMatches = !_tcsncmp(pchPosition, pFindReplace->lpstrFindWhat, iTargetLength);
    else
        bMatches = !_tcsnicmp(pchPosition, pFindReplace->lpstrFindWhat, iTargetLength);

    if (bMatches && (pFindReplace->Flags & FR_WHOLEWORD))
    {
        if (dwPosition > 0)
        {
            if (_istalnum(*(pchPosition - 1)) || *(pchPosition - 1) == _T('_'))
                bMatches = FALSE;
        }
        if ((INT)dwPosition + iTargetLength < iTextLength)
        {
            if (_istalnum(pchPosition[iTargetLength]) || pchPosition[iTargetLength] == _T('_'))
                bMatches = FALSE;
        }
    }

    return bMatches;
}

//           NOTEPAD_ReplaceAll
static VOID ReplaceAll(FINDREPLACE *pFindReplace)
{
    BOOL bShowAlert = TRUE;

    SendMessage(Globals.hEdit, EM_SETSEL, 0, 0);

    while (Search_FindNext(pFindReplace, TRUE, bShowAlert))
    {
        bShowAlert = FALSE;
    }
}

static VOID FindTerm(VOID)
{
    Globals.hFindReplaceDlg = NULL;
}

VOID EventSearchReplace (FINDREPLACE *pFindReplace)
{
    Globals.find = *pFindReplace;

    if (pFindReplace->Flags & FR_FINDNEXT)
        Search_FindNext(pFindReplace, FALSE, TRUE);
    else if (pFindReplace->Flags & FR_REPLACE)
        Search_FindNext(pFindReplace, TRUE, TRUE);
    else if (pFindReplace->Flags & FR_REPLACEALL)
        ReplaceAll(pFindReplace);
    else if (pFindReplace->Flags & FR_DIALOGTERM)
        FindTerm();
}

/**********************************************************************/
// MRU Most recently used file list service

#define MRU_MAX    10
#define MRU_key   _T("MRU%02d")

typedef struct {
    int seq;
    int index;
    TCHAR path[MAX_PATH];
} MRU_item_st;

static MRU_item_st MRU[MRU_MAX];

static int youngest;   // generation seq.
static int oldest;     // least recenly used item *index* 

VOID MRU_Init(VOID)
{
    for (int ii = 0 ; ii < MRU_MAX; ii++)
    {
        MRU[ii].seq = 0;
        MRU[ii].index = ii;
        MRU[ii].path[0] = 0;
    }

    youngest = 0; 
    oldest = 0;
}

#define MRUDATA(ii) (MRU[MRU[(ii)].index].path)

VOID MRU_Add(LPCTSTR newpath)
{
    youngest++;
    
    for (int ii = 0; ii < MRU_MAX; ii++) 
    {
        if ( _tcsicmp(MRUDATA(ii), newpath ) == 0)
        {   // existing path, refresh age
            MRU[ii].seq = youngest;
            return;
        }
    }
    // new to list. Replace oldest with new path.
    MRU[oldest].seq = youngest; 
    StringCchCopy(MRUDATA(oldest), MAX_PATH, newpath);

    // select next oldest item (lowest seq) 
    int lowest = youngest;
    for (int ii = 0; ii < MRU_MAX; ii++) 
    {
        if ( MRU[ii].seq < lowest  )
        {
            lowest = MRU[ii].seq;
            oldest = ii;
        }
    }
}

// Simple insertion sort over MRU. key is MRU.seq. Oldest/empty items first.
VOID MRU_Sort(VOID)
{
    int ii, jj, key, inx;
    
    for (ii = 1; ii < MRU_MAX; ii++)
    {
        key = MRU[ii].seq;
        inx =  MRU[ii].index;
        jj = ii;
 
        // Move elements of arr[0..ii-1],
        while (--jj >= 0 && MRU[jj].seq > key)
        {
            MRU[jj + 1].seq = MRU[jj].seq;
            MRU[jj + 1].index = MRU[jj].index;
        }
        MRU[jj+1].seq = key;
        MRU[jj+1].index = inx;
    }

    int kk = MRU_MAX-1;
    while ( kk >= 0 && (MRU[kk].seq > 0))
        kk--;
    oldest = kk;
}

// get n-th younest item. n = 0 : most recently used.
LPCTSTR MRU_Enum(int n)
{
    assert(n >= 0 && n < MRU_MAX);
    return MRUDATA(MRU_MAX - n-1);
}

// Load ols MRU from "~\AppData\Local\XPAccApps.ini" file.
VOID MRU_Load(VOID)
{
    TCHAR keyname[32];
    for (int ii = MRU_MAX-1; ii >=0 ; ii--)
    {
        _stprintf_s(keyname, _countof(keyname), MRU_key, ii);
        // GetPrivateProfileString(PFSECTION, keyname, _T(""), path, _countof(path), ProfilePath);
        LPCTSTR path = GetIniString( keyname, _T(""));
        if (path[0] && FileExists(path))
            MRU_Add(path);
    }
}

VOID MRU_Save(VOID)
{
    // MRU00 = youngest, ..., MRU09 = oldest, empty slot not stored.
    TCHAR keyname[32], path[MAX_PATH];
    for (int ii = 0, jj = 0; ii < MRU_MAX ; ii++)
    {
        _tcscpy(path, MRU_Enum(ii));
        if (path[0])
        {
            _stprintf_s(keyname, _countof(keyname), MRU_key, jj++);
            // WritePrivateProfileString(PFSECTION, keyname, path, ProfilePath);
            PutIniString(keyname, path);
        }
    }
}

// --------------------------------------------------------------------
VOID UpdateMenuRecentList(HMENU menuMain)
{
    TCHAR szTitle[MAX_PATH] = {0}, szMenu[MAX_PATH] = {0};

    MRU_Sort();

#define MENUOFFSET_RECENTLYUSED   6

    HMENU hmsubMRU = GetSubMenu(menuMain, MENUOFFSET_RECENTLYUSED);
    int nmi = GetMenuItemCount(hmsubMRU);
    for (int imi = 0; imi < nmi; imi++)
    {
        DeleteMenu(hmsubMRU, 0, MF_BYPOSITION);
    }

    for (int ii = 0; ii < MRU_MAX; ii++ )
    {
        szTitle[0] = 0;
        if (GetFileTitle(MRU_Enum(ii), szTitle, _countof(szTitle)) != 0 )
            break;
        StringCchPrintf(szMenu, _countof(szMenu), L"&%d  %s", ii+1, szTitle);
        ShortenPath(szMenu, 0);
        /*
        * _KOOKIE_: Fix MRU....
        if (ii < 3 )
        {
            DeleteMenu(menuMain, MENUOFFSET_RECENTLYUSED +ii +1, MF_BYPOSITION);
            InsertMenu(menuMain, MENUOFFSET_RECENTLYUSED +ii +1,
            MF_BYPOSITION|MF_STRING, MENU_RECENT1 +ii, szMenu );
        }
        */
        AppendMenu(hmsubMRU, MF_BYPOSITION|MF_STRING, MENU_RECENT1 +ii, szMenu);
    }
}

/**********************************************************************/
// Utility functions

// Shorten path string to limited langth
VOID ShortenPath(LPTSTR szStr, int maxlen)
{
    int len = _tcslen(szStr);
    if ( maxlen == 0 )
        maxlen = 32; // default
    if (len <= maxlen )
        return;
    int mid = maxlen/2-2;
    _tcscpy(szStr + mid, L"...");
    _tcscpy(szStr + mid +3, szStr+len-mid-(maxlen%2) -1);
}

/**
 * Returns:
 *   TRUE  - if file exists
 *   FALSE - if file does not exist
 */
BOOL FileExists(LPCTSTR szFilename)
{
    return GetFileAttributes(szFilename) != INVALID_FILE_ATTRIBUTES;
}

BOOL HasFileExtension(LPCTSTR szFilename)
{
    LPCTSTR s;

    s = _tcsrchr(szFilename, _T('\\'));
    if (s)
        szFilename = s;
    return _tcsrchr(szFilename, _T('.')) != NULL;
}
