/*
 * PROJECT:    WinXPAccApps Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 */

#include "notepad.h"

#include <assert.h>
#include <commctrl.h>
#include <strsafe.h>

#define     MAX_NTAB    9       // max number of tabs
#define     XSP         3       // edit window border thickness

static int TH_Height;
static int ST_Height;

// update screen layout on WM_SIZE
void UpdateEditSize(VOID)
{
    if (!Globals.hwTabCtrl)
        return;

    RECT rct;
    GetClientRect(Globals.hMainWnd, &rct);
    int ww = rct.right - rct.left;
    int hh = rct.bottom - rct.top;

    if (Globals.bShowStatusBar )
    {
        hh -= ST_Height-XSP;
    }

    ww -= XSP*2; hh -= XSP*2;
    MoveWindow(Globals.hwTabCtrl, XSP, XSP, ww, hh, TRUE);

    ww -= XSP*2; hh -= TH_Height + XSP*2;
    MoveWindow(Globals.hEdit, XSP, TH_Height + XSP, ww, hh, TRUE);

}

// ----- Multi-Tab service ---------
// 
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
    ZeroMemory(&tab, sizeof (tab));
    tab.mask = TCIF_TEXT|TCIF_IMAGE|TCIF_PARAM;

    EDITINFO* pedi = calloc(1, sizeof(EDITINFO));
    pedi->cbSize = sizeof(EDITINFO);
    pedi->hwEDIT = Globals.hEdit;

    // StringCchCopy(pedi->fileTitle, _countof(pedi->fileTitle), Globals.szFileTitle);
    Globals.pEditInfo = pedi;

    tab.lParam = (LPARAM) pedi;
    tab.pszText = Globals.szFileTitle;
    tab.iImage = -1;

    TabCtrl_InsertItem(Globals.hwTabCtrl, ntab, &tab);
    TabCtrl_SetCurSel(Globals.hwTabCtrl, ntab);
    SetTabHeader();

    return TRUE;
}

// Context switching chore after tab change.
void OnTabChange(VOID)
{
    ShowWindow(Globals.hEdit, SW_HIDE);

    int iPage = TabCtrl_GetCurSel(Globals.hwTabCtrl);

    TCITEM tab;
    ZeroMemory(&tab, sizeof (tab));
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

    if ( Globals.bShowStatusBar )
    {
        DIALOG_StatusBarUpdateAll();
    }

    UpdateWindowCaption(FALSE);
}

// Set tab header
void SetTabHeader()
{
    TCHAR buf[MAX_PATH];
    int len = _tcslen(Globals.szFileTitle);

    StringCchPrintf( buf, MAX_PATH, (len < 10 ? L"    %s    " : L"  %s  "), Globals.szFileTitle);
    LimitStrLen(buf, 0);

    TCITEM tab;
    ZeroMemory(&tab, sizeof (tab));
    tab.mask = TCIF_TEXT|TCIF_IMAGE;
    tab.pszText = (LPTSTR) buf;
    tab.iImage = -1;

    int iPage = TabCtrl_GetCurSel(Globals.hwTabCtrl);
    TabCtrl_SetItem(Globals.hwTabCtrl, iPage, &tab);
}

// Check for file is duplicate (already editing)
// Returns Tab index (zero-based) on matching duplicate file already loaed, -1 on not duplicate.
// Caveats: very rudimentary path name comparison for match checking.
int CheckDupFileName(LPCTSTR filePath)
{
    int ntab = TabCtrl_GetItemCount(Globals.hwTabCtrl);

    TCITEM tab;
    ZeroMemory(&tab, sizeof(tab));
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

// ----- MRU service ---------

# define MRU_MAX    10

typedef struct {
    int seq;
    int index;
    TCHAR path[MAX_PATH];
} MRU_item_st;

static MRU_item_st MRU[MRU_MAX];

static int youngest;   // generation seq.
static int oldest;     // least recenly used item *index* 

void MRU_Init(VOID)
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

void MRU_Add(LPCTSTR newpath)
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
void MRU_Sort(VOID)
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

#define MRU_key   _T("MRU%02d")

// Load ols MRU from "~\AppData\Local\XPAccApps.ini" file.
void MRU_Load()
{
    TCHAR keyname[32], path[MAX_PATH];
    for (int ii = MRU_MAX-1; ii >=0 ; ii--)
    {
        _stprintf_s(keyname, _countof(keyname), MRU_key, ii);
        GetPrivateProfileString(PFSECTION, keyname, _T(""), path, _countof(path), ProfilePath);
        if (path[0] && FileExists(path))
            MRU_Add(path);
    }
}

void MRU_Save()
{
    // MRU00 = youngest, ..., MRU09 = oldest, empty slot not stored.
    TCHAR keyname[32], path[MAX_PATH];
    for (int ii = 0, jj = 0; ii < MRU_MAX ; ii++)
    {
        _tcscpy(path, MRU_Enum(ii));
        if (path[0])
        {
            _stprintf_s(keyname, _countof(keyname), MRU_key, jj++);
            WritePrivateProfileString(PFSECTION, keyname, path, ProfilePath);
        }
    }
}

void UpdateMenuRecent(HMENU menuMain)
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
        LimitStrLen(szMenu, 0);
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

void LimitStrLen(LPTSTR szStr, int limit)
{
    int len = _tcslen(szStr);
    if ( limit == 0 )
        limit = 32; // default
    if (len <= limit )
        return;
    int mid = limit/2-2;
    _tcscpy(szStr + mid, L"...");
    _tcscpy(szStr + mid +3, szStr+len-mid-(limit%2) -1);
}
