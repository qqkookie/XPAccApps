/*
 * PROJECT:    ReactOS Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 * COPYRIGHT:  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *             Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *             Copyright 2002 Andriy Palamarchuk
 *             Copyright 2000 Mike McCormack <Mike_McCormack@looksmart.com.au>
 *             Copyright 2020-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifndef STRSAFE_NO_DEPRECATE
    #define STRSAFE_NO_DEPRECATE
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <commdlg.h>

#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include "dialog.h"
#include "notepad_res.h"

#define STR_LONG     256
#define STR_SHORT   64

/* Values are indexes of the items in the Encoding combobox. */
typedef enum
{
    ENCODING_AUTO    = -1,
    ENCODING_UTF8    =  0,
    ENCODING_UTF8BOM =  1,
    ENCODING_ANSIOEM =  2,
    ENCODING_UTF16LE =  3,
    ENCODING_UTF16BE =  4,

} ENCODING;

#define ENCODING_DEFAULT    ENCODING_UTF8

typedef enum
{
    EOLN_LF   = 0, /* "\n" */
    EOLN_CRLF = 1, /* "\r\n" */
    EOLN_CR   = 2  /* "\r" */
} EOLN; /* End of line (NewLine) type */

#define EOLN_DEFAULT    EOLN_LF

typedef enum
{
    FM_NORMAL   = 0,
    FM_READONLY = 1,
    FM_OUTDATE  = 2,    // not modified, but externally modified. 
    FM_EDITING  = 3,    // modified
    FM_CLASH    = 4,    // modified and externally modified, too.
} FILEMODE;  // file modification state

// Extra info for each Edit control on eacn Tab.
typedef struct {
    UINT        cbSize;
    HWND        hwEDIT;
    ENCODING    encFile;
    EOLN        iEoln;
    FILEMODE    FileMode;
    FILETIME    FileTime;

    BOOL        pathOK;
    WCHAR       filePath[MAX_PATH];
} EDITINFO;

typedef struct
{
    HINSTANCE hInstance;
    HWND hMainWnd;
    HMENU hMenu;

    HWND hEdit;
    WNDPROC EditProc;
    HFONT hFont; /* Font used by the edit control */

    HWND hStatusBar;
    HWND hFindReplaceDlg;
    FINDREPLACE find;

    ENCODING encFile;
    EOLN iEoln;
    BOOL bWasModified;

    HWND hwTabCtrl;
    EDITINFO *pEditInfo;

    HGLOBAL hDevMode;
    HGLOBAL hDevNames;

    WCHAR szFileName[MAX_PATH];
    WCHAR szFileTitle[MAX_PATH];
    WCHAR szStatusBarLineCol[STR_LONG];
    WCHAR szFilter[STR_LONG*3];

} NOTEPAD_GLOBALS;

extern NOTEPAD_GLOBALS Globals;

WCHAR G_STR_NOTEPAD[];

// ----- settings.c ------------

typedef struct
{
    RECT main_rect;
    LOGFONT lfFont;
    BOOL bWrapLongLines;
    BOOL bShowStatusBar;
    WCHAR szFindText[STR_LONG];
    WCHAR szReplaceText[STR_LONG];

    RECT lMargins; /* The margin values in 100th millimeters */
    WCHAR szHeader[STR_SHORT];
    WCHAR szFooter[STR_SHORT];

    WCHAR txtTypeFilter[STR_LONG];
    WCHAR moreTypeFilter[STR_LONG*2];
} SETTING_DATA;

extern SETTING_DATA Settings;

VOID LoadAppSettings(VOID);
VOID SaveAppSettings(VOID);

LPCWSTR GetString(UINT rid);
#define GETSTRING(id)           GetString(id)
#define LOADSTRING(id, buf)     LoadString( NULL, id, buf, _countof(buf))

LPCWSTR ReadIniString(LPCWSTR key, LPCWSTR defval);
VOID WriteIniString(LPCWSTR key, LPCWSTR value);

// ----- text.c ---------------
// BOOL ReadText(HANDLE hFile, LPWSTR *ppszText, ENCODING *pencFile, EOLN *piEoln);
// BOOL WriteText(HANDLE hFile, LPCWSTR szText, DWORD dwTextLen, ENCODING encFile, EOLN iEoln);

// ----- file.c ---------------
BOOL DoOpenFile(LPCWSTR szFileName);
BOOL DoSaveFile(VOID);

BOOL Search_FindNext(FINDREPLACE *pFindReplace, BOOL bReplace, BOOL bShowAlert);
VOID EventSearchReplace (FINDREPLACE *pFindReplace);

VOID MRU_Init(VOID);
VOID MRU_Add(LPCWSTR newpath);
VOID MRU_Sort(VOID);
LPCWSTR MRU_Enum(int n);
VOID MRU_Load(VOID);
VOID MRU_Save(VOID);

VOID UpdateMenuRecentList(HMENU menuSub);

VOID ShortenPath(LPWSTR szStr, int maxlen);
BOOL FileExists(LPCWSTR szFilename);
BOOL HasFileExtension(LPCWSTR szFilename);
int FileTimeCompare( FILETIME * ta, FILETIME *tb );

// ------------------------------
/* utility macros */

#define ZEROMEM(mem)    ZeroMemory(&mem, sizeof(mem))

#define STROK(s) ((s) && *(s))
#define STRNOT(s) ((!s)||!*(s))
#define NULSTR  L""
