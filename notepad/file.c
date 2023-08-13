/*
 * PROJECT:    WinXPAccApps Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 */

#include "notepad.h"

#include <assert.h>
#include <strsafe.h>

/**********************************************************************/
// Low-level file I/O

#define LINE_MAXSIZE 4100
#define WSIZE  sizeof(WCHAR)

// Detect codepage encoding.
// int *bom receives byte size of BOM premeble.
static ENCODING AnalyzeEncoding(const LPBYTE pBytes, int dwSize, int* bom)
{
    static const char bom_utf8[] = { 0xef, 0xbb, 0xbf };
    static const char bom_utf16le[] = { 0xff, 0xfe };
    static const char bom_utf16be[] = { 0xfe, 0xff };

    if (bom && dwSize >= sizeof(bom_utf16le))
    {
        if (dwSize >= sizeof(bom_utf8) && !memcmp(pBytes, bom_utf8, sizeof(bom_utf8)))
        {
            *bom = sizeof(bom_utf8);
            return ENCODING_UTF8;
        }
        if (memcmp(pBytes, bom_utf16le, sizeof(bom_utf16le)) == 0)
        {
            *bom = sizeof(bom_utf16le);
            return ENCODING_UTF16LE;
        }
        if ( memcmp(pBytes, bom_utf16be, sizeof(bom_utf16be)) == 0 )
        {
            *bom = sizeof(bom_utf16be);
            return ENCODING_UTF16BE;
        }
    }

    int utf16mask =  IS_TEXT_UNICODE_SIGNATURE|IS_TEXT_UNICODE_STATISTICS;
    if (IsTextUnicode(pBytes, dwSize, &utf16mask))
        return ENCODING_UTF16LE;

    utf16mask = IS_TEXT_UNICODE_REVERSE_SIGNATURE;
    if (IsTextUnicode(pBytes, dwSize, &utf16mask))
        return ENCODING_UTF16BE;

    for ( signed char *pch = pBytes; *pch > 0 ; pch++) 
    {
        if ( pch >= pBytes + dwSize -1 )    // Pure ascii to the end
            return (ENCODING_ANSIOEM);
    }

    /* is it UTF-8? */
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)pBytes, dwSize, NULL, 0) > 0)
        return ENCODING_UTF8;

    return (ENCODING_ANSIOEM);
}

// Detect end of line discipline
static EOLN CountLineEndings(LPWSTR szText, int cchText)
{
    WCHAR old_ch = 0;
    int n_lf, n_crlf, n_cr;
    n_lf = n_crlf = n_cr = 0;

    for (int ich = 0; ich < cchText; ++ich)
    {
        WCHAR ch = szText[ich];

        if (ch == UNICODE_NULL)
            szText[ich] = L' ';     // Replace L'\0' with unicode SPACE. 
        else if (ch == L'\r')
            n_cr++;
        else if (ch == L'\n')
        {
            if (old_ch == L'\r')
            {
                n_cr--;
                n_crlf++;
            }
            else
                n_lf++;
        }
        old_ch = ch;
    }

    /* Choose the newline code */
    if (n_cr > (n_lf + n_crlf ))
        return EOLN_CR;
    else if (n_crlf > (n_lf + n_cr))
        return EOLN_CRLF;

    return EOLN_LF;
}

// Read text from file. Data is saved in allocated memory heap and
// ppszText is address of pointer to receive memory handle from process heap.
// On successful read, buffer must be freed by caller with HeapFree().
// 
// Ex)  LPWSTR pszText = NULL; ReadText(hFile, &pszText,...)
//      HeapFree(GetProcessHeap(), 0, pszText);
static BOOL ReadText(HANDLE hFile, LPWSTR *ppszText, ENCODING *pencFile, EOLN *piEoln)
{
    BOOL bSuccess = FALSE;
    ENCODING encFile = ENCODING_DEFAULT;
    HANDLE hHeap = GetProcessHeap();
    DWORD dwSize = GetFileSize(hFile, NULL);
    HANDLE hMapping = NULL;
    LPBYTE fileMapView = NULL;
    int cchTextLen = 0;

    if ( !hFile || !ppszText || dwSize == INVALID_FILE_SIZE)
        return FALSE;

    LPWSTR TextBuf = HeapAlloc( hHeap, 0, dwSize * WSIZE + 8);
    if (!TextBuf)
        return FALSE;
    if ( dwSize == 0 )
        goto empty_file;

    hMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    fileMapView = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, dwSize);
    if (hMapping == NULL  || !fileMapView)
        goto done;

    int bomlen = 0;
    encFile = AnalyzeEncoding(fileMapView, dwSize, &bomlen);

    const LPBYTE FileBytes =  &fileMapView[bomlen];    // skip BOM
    LPCWSTR FileText = (LPCWSTR) FileBytes;
    int cByteSize = dwSize - bomlen;

    LPWSTR Resized;
    UINT iCodePage = CP_UTF8;

    if ( dwSize == 0 || cByteSize == 0)
        goto empty_file;

    switch(encFile)
    {

    case ENCODING_UTF16BE:
        /* big endian; Swap bytes */
        _swab(FileBytes, FileBytes, cByteSize);
        /*FALLTHRU*/

    case ENCODING_UTF16LE:
        cchTextLen = (cByteSize +1)/WSIZE;
        Resized = HeapReAlloc( hHeap, 0, TextBuf, (cchTextLen+1)*WSIZE);
        if (Resized)
            TextBuf = Resized;
            
        CopyMemory(TextBuf, FileBytes, cchTextLen*WSIZE);
        TextBuf[cchTextLen] = UNICODE_NULL;
        break;

    case ENCODING_ANSIOEM:
        iCodePage = CP_ACP;
        /*FALLTHRU*/

    case ENCODING_UTF8:
    case ENCODING_UTF8BOM:
        cchTextLen = MultiByteToWideChar(iCodePage, 0, FileBytes, cByteSize, NULL, 0);
        if (cchTextLen == 0)
            goto done;
        Resized = HeapReAlloc( hHeap, 0, TextBuf, (cchTextLen+1)*WSIZE);
        if (Resized)
            TextBuf = Resized;

        MultiByteToWideChar(iCodePage, 0, FileBytes, cByteSize, TextBuf, cchTextLen);
        TextBuf[cchTextLen] = UNICODE_NULL;
        break;

        DEFAULT_UNREACHABLE;
    }

    *piEoln = CountLineEndings(TextBuf, cchTextLen);

empty_file:
    TextBuf[cchTextLen] = UNICODE_NULL;
    *ppszText = TextBuf;
    *pencFile = encFile;
    bSuccess = TRUE;

done:
    if (fileMapView) UnmapViewOfFile(fileMapView);
    if (hMapping) CloseHandle(hMapping);
    if (!bSuccess && TextBuf)
        HeapFree(hHeap, 0, TextBuf);

    return bSuccess;
}

// Write a line to file in give codepage and EOL discipline.
// Maximum line length limited to ~4k bytes in file.
// szLine and cchLen does not include EOL.
// EOL is added automatically according to iEoln. If iEoln == -1, no EOLN added.
static BOOL WriteOneLine(HANDLE hFile, LPCWSTR szLine, int cchLen, ENCODING encFile, EOLN iEoln)
{
    static LPCWSTR WideEOL[] = { L"\n", L"\r\n", L"\r" };
    static LPCSTR CharEOL[]  = { "\n", "\r\n", "\r" };
    HANDLE hHeap = GetProcessHeap();
    UINT iCodePage =  CP_UTF8;
    int cbWrite = 0, cbeol;
    char WriteBuf[LINE_MAXSIZE];

    assert( cchLen == 0 || (szLine[cchLen-1] != '\r' && szLine[cchLen-1] != '\n'));

    switch(encFile)
    {
        case ENCODING_UTF16LE:
        case ENCODING_UTF16BE:

            cbWrite = cchLen * WSIZE;
            memcpy(WriteBuf, szLine, cbWrite);
            if ( iEoln >= 0 )
            {
                cbeol = (iEoln == EOLN_CRLF) ? 4: 2;
                memcpy(WriteBuf +cbWrite, WideEOL[iEoln], cbeol);
                cbWrite += cbeol;
            }
              
            if (encFile == ENCODING_UTF16BE)
                _swab(WriteBuf, WriteBuf, cbWrite);
            break;

        case ENCODING_ANSIOEM:
            iCodePage = CP_ACP;
            /*FALLTHRU*/
        case ENCODING_UTF8:
        case ENCODING_UTF8BOM:

            if ( cchLen > 0)
            {
                cbWrite = WideCharToMultiByte(iCodePage, 0,
                    szLine, cchLen, WriteBuf, sizeof(WriteBuf), NULL, NULL);
                if (cbWrite <= 0) 
                    return FALSE;
            }

            if ( iEoln >= 0 )
            {
                cbeol = (iEoln == EOLN_CRLF) ? 2: 1;
                memcpy(WriteBuf +cbWrite, CharEOL[iEoln], cbeol);
                cbWrite += cbeol;
            }
            assert(cbWrite < sizeof(WriteBuf));
            break;

        default:
            return FALSE;
    }

    int outBytes;
    if (! WriteFile(hFile, WriteBuf, cbWrite, &outBytes, NULL))
        return FALSE;
    assert(cbWrite == outBytes);

    return TRUE;
}

// Write wide string buffer to file.
static BOOL WriteText(HANDLE hFile, LPCWSTR pszText, DWORD dwTextLen, ENCODING encFile, EOLN iEoln)
{
    /* Write the proper byte order marks if not ANSI or UTF-8 without BOM */
    if (encFile != ENCODING_ANSIOEM && encFile != ENCODING_UTF8)
    {
        WCHAR wcBom = 0xFEFF;
        if (!WriteOneLine(hFile, &wcBom, 1, encFile, -1))
            return FALSE;
    }

    DWORD dwPos = 0, dwNext = 0;
    while(dwNext < dwTextLen)
    {
        // Find the next eoln 
        if (pszText[dwNext] == L'\n' || pszText[dwNext] == L'\r')
        {
            if (!WriteOneLine(hFile, pszText + dwPos, dwNext - dwPos, encFile, iEoln))
                return FALSE;

            dwNext += (pszText[dwNext] == L'\r' && pszText[dwNext+1] ==  L'\n') ? 2:1;     // Skip EOL
            dwPos = dwNext;
        }
        else 
            dwNext++;
    }
    if ( dwNext > dwPos )
    {
        if (!WriteOneLine(hFile, pszText + dwPos, dwNext - dwPos, encFile, -1))
            return FALSE;
    }
    return TRUE;
}

/**********************************************************************/
// File Open Close Save

BOOL DoOpenFile(LPCWSTR szFileName)
{
    HANDLE hFile;
    BOOL ok = FALSE;

    hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
       return FALSE;

    LPWSTR pszText = NULL;
    if (!ReadText(hFile, &pszText, &Globals.encFile, &Globals.iEoln)
        || !pszText )
        goto done;

    Globals.pEditInfo->encFile = Globals.encFile;
    Globals.pEditInfo->iEoln = Globals.iEoln;
    Globals.pEditInfo->FileMode = GetFileAttributes(szFileName) &
        (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM) ? FM_READONLY : FM_NORMAL;

    SetWindowText(Globals.hEdit, pszText);
    HeapFree(GetProcessHeap(), 0, pszText);
    SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
    SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0);

    if (Globals.pEditInfo->FileMode == FM_READONLY)
             SendMessage(Globals.hEdit, EM_SETREADONLY, TRUE, 0);

    GetFileTime(hFile, NULL, NULL, &(Globals.pEditInfo->FileTime));
    ok = TRUE;

done:
    CloseHandle(hFile);
    return ok;
}

// --------------------------------------------------------------------
BOOL DoSaveFile(VOID)
{
    BOOL ok = FALSE;
    HANDLE hFile = NULL;
    int len = GetWindowTextLengthW(Globals.hEdit) +2;
    HANDLE hHeap = GetProcessHeap();
    LPWSTR szText = HeapAlloc( hHeap, 0, len*WSIZE);
    if (szText == NULL)
        goto done;

    int cchText = GetWindowText(Globals.hEdit, szText, len);
    if (cchText == 0 )
        goto done;

    szText[cchText] = L'\0';

    hFile = CreateFileW(Globals.szFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
                    NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    ok = WriteText(hFile, szText, cchText, Globals.encFile, Globals.iEoln);
    if (ok)
    {
        SetEndOfFile(hFile);
        SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0);

        FILETIME ft;
        GetFileTime(hFile, NULL, NULL, &ft );
        Globals.pEditInfo->FileTime = ft;

        MRU_Add(Globals.szFileName);
    }

done:
    if ( hFile )
        CloseHandle(hFile);
    if ( szText)
        HeapFree( hHeap, 0, szText);
    return ok;
}

/**********************************************************************/
// Search FindNext

static BOOL FindTextAt(FINDREPLACE *pFindReplace, LPCWSTR pszText, INT iTextLength, DWORD dwPosition);

BOOL Search_FindNext(FINDREPLACE *pFindReplace, BOOL bReplace, BOOL bShowAlert)
{
    int iTextLength, iTargetLength;
    size_t iAdjustment = 0;
    LPWSTR pszText = NULL;
    DWORD dwPosition, dwBegin, dwEnd;
    BOOL bMatches = FALSE;
    WCHAR szText[STR_LONG];
    BOOL bSuccess;

    iTargetLength = (int) _tcslen(pFindReplace->lpstrFindWhat);

    /* Retrieve the window text */
    iTextLength = GetWindowTextLength(Globals.hEdit);
    if (iTextLength > 0)
    {
        pszText = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, (iTextLength + 1) * WSIZE);
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
static BOOL FindTextAt(FINDREPLACE *pFindReplace, LPCWSTR pszText, INT iTextLength, DWORD dwPosition)
{
    BOOL bMatches;
    size_t iTargetLength;
    LPCWSTR pchPosition;

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
            if (_istalnum(*(pchPosition - 1)) || *(pchPosition - 1) == L'_')
                bMatches = FALSE;
        }
        if ((INT)dwPosition + iTargetLength < iTextLength)
        {
            if (_istalnum(pchPosition[iTargetLength]) || pchPosition[iTargetLength] == L'_')
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
#define MRU_key   L"MRU%02d"

typedef struct {
    int seq;
    int index;
    WCHAR path[MAX_PATH];
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

VOID MRU_Add(LPCWSTR newpath)
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
LPCWSTR MRU_Enum(int n)
{
    assert(n >= 0 && n < MRU_MAX);
    return MRUDATA(MRU_MAX - n-1);
}

// Load ols MRU from "~\AppData\Local\XPAccApps.ini" file.
VOID MRU_Load(VOID)
{
    MRU_Init();

    WCHAR keyname[32];
    for (int ii = MRU_MAX-1; ii >=0 ; ii--)
    {
        _stprintf_s(keyname, _countof(keyname), MRU_key, ii);
        LPCWSTR path = ReadIniString( keyname, L"");
        if (path[0] && FileExists(path))
            MRU_Add(path);
    }
}

VOID MRU_Save(VOID)
{
    // MRU00 = youngest, ..., MRU09 = oldest, empty slot not stored.
    WCHAR keyname[32], path[MAX_PATH];
    int ii = 0;
    for (int jj = 0 ; jj < MRU_MAX ; jj++)
    {
        _tcscpy(path, MRU_Enum(jj));
        if (path[0])
        {
            _stprintf_s(keyname, _countof(keyname), MRU_key, ii++);
            // WritePrivateProfileString(PFSECTION, keyname, path, ProfilePath);
            WriteIniString(keyname, path);
        }
    }
    while ( ii < MRU_MAX)
    {
        _stprintf_s(keyname, _countof(keyname), MRU_key, ii++);
        WriteIniString(keyname, L"");
    }
}

// --------------------------------------------------------------------
VOID UpdateMenuRecentList(HMENU menuFile)
{
    WCHAR szTitle[MAX_PATH] = {0}, szMenu[MAX_PATH] = {0};

    MRU_Sort();

#define MENUOFFSET_RECENTLYUSED   6

    HMENU hmMRU = GetSubMenu(menuFile, MENUOFFSET_RECENTLYUSED);
    int nmi = GetMenuItemCount(hmMRU);
    for (int imi = 0; imi < nmi; imi++)
    {
        DeleteMenu(hmMRU, 0, MF_BYPOSITION);
    }

    for (int ii = 0; ii < MRU_MAX; ii++ )
    {
        szTitle[0] = 0;
        if (GetFileTitle(MRU_Enum(ii), szTitle, _countof(szTitle)) != 0 )
            break;
        StringCchPrintf(szMenu, _countof(szMenu), L"&%d  %s", ii+1, szTitle);
        ShortenPath(szMenu, 0);

        AppendMenu(hmMRU, MF_BYPOSITION|MF_STRING, MENU_RECENT1 +ii, szMenu);

        if (ii < 3 )
        {
            DeleteMenu(menuFile, MENUOFFSET_RECENTLYUSED +ii +1, MF_BYPOSITION);
            InsertMenu(menuFile, MENUOFFSET_RECENTLYUSED +ii +1,
                MF_BYPOSITION|MF_STRING, MENU_RECENT1 +ii, szMenu );
        }
    }
}

/**********************************************************************/
// Utility functions

// Shorten path string to limited langth
VOID ShortenPath(LPWSTR szStr, int maxlen)
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
BOOL FileExists(LPCWSTR szFilename)
{
    return GetFileAttributes(szFilename) != INVALID_FILE_ATTRIBUTES;
}

BOOL HasFileExtension(LPCWSTR szFilename)
{
    LPCWSTR s;

    s = _tcsrchr(szFilename, L'\\');
    if (s)
        szFilename = s;
    return _tcsrchr(szFilename, L'.') != NULL;
}

// file time compare. slack time: 0.1 sec.
int FileTimeCompare( FILETIME * ta, FILETIME *tb )
{
    int dt = ta->dwHighDateTime - tb->dwHighDateTime;
    if ( dt != 0 )
        return dt;
    dt = (ta->dwLowDateTime - tb->dwLowDateTime)/1000000;
    return dt;
}


#if 0
static VOID
ReplaceNewLines(LPWSTR pszNew, SIZE_T cchNew, LPCWSTR pszOld, SIZE_T cchOld)
{
    BOOL bPrevCR = FALSE;
    SIZE_T ichNew, ichOld;

    for (ichOld = ichNew = 0; ichOld < cchOld; ++ichOld)
    {
        WCHAR ch = pszOld[ichOld];

        if (ch == L'\n')
        {
            if (!bPrevCR)
            {
                pszNew[ichNew++] = L'\r';
                pszNew[ichNew++] = L'\n';
            }
        }
        else if (ch == '\r')
        {
            pszNew[ichNew++] = L'\r';
            pszNew[ichNew++] = L'\n';
        }
        else
        {
            pszNew[ichNew++] = ch;
        }

        bPrevCR = (ch == L'\r');
    }

    pszNew[ichNew] = UNICODE_NULL;
    assert(ichNew == cchNew);
}


static BOOL
ProcessNewLinesAndNulls(HANDLE hHeap, LPWSTR *ppszText, SIZE_T *pcchText, EOLN *piEoln)
{
    SIZE_T ich, cchText = *pcchText, adwEolnCount[3] = { 0, 0, 0 }, cNonCRLFs;
    LPWSTR pszText = *ppszText;
    EOLN iEoln;
    BOOL bPrevCR = FALSE;

    /* Replace '\0' with SPACE. Count newlines. */
    for (ich = 0; ich < cchText; ++ich)
    {
        WCHAR ch = pszText[ich];
        if (ch == UNICODE_NULL)
            pszText[ich] = L' ';

        if (ch == L'\n')
        {
            if (bPrevCR)
            {
                adwEolnCount[EOLN_CR]--;
                adwEolnCount[EOLN_CRLF]++;
            }
            else
            {
                adwEolnCount[EOLN_LF]++;
            }
        }
        else if (ch == '\r')
        {
            adwEolnCount[EOLN_CR]++;
        }

        bPrevCR = (ch == L'\r');
    }

    /* Choose the newline code */
    if (adwEolnCount[EOLN_CR] > adwEolnCount[EOLN_CRLF])
        iEoln = EOLN_CR;
    else if (adwEolnCount[EOLN_LF] > adwEolnCount[EOLN_CRLF])
        iEoln = EOLN_LF;
    else
        iEoln = EOLN_CRLF;

    cNonCRLFs = adwEolnCount[EOLN_CR] + adwEolnCount[EOLN_LF];
    if (cNonCRLFs != 0)
    {
        /* Allocate a buffer for EM_SETHANDLE */
        SIZE_T cchNew = cchText + cNonCRLFs;
        HANDLE newText = HeapAlloc( hHeap, 0,
                        pszText, (cchNew + 1) * sizeof(WCHAR));
        if (!newText)
            return FALSE; /* Failure */

        ReplaceNewLines(newText, cchNew, pszText, cchText);

        /* Replace with new data */;
        *ppszText = newText;
        *pcchText = cchNew;
    }

    *piEoln = iEoln;
    return TRUE;
}


int CountNewLinesAndNulls(LPWSTR szText, int cchText)
{
    SIZE_T n_cr, n_lf, n_crlf;
    n_cr = n_lf = n_crlf = 0;
    BOOL bPrevCR = FALSE;

    /* Replace '\0' with SPACE. Count newlines. */
    for (int ich = 0; ich < cchText; ++ich)
    {
        WCHAR ch = szText[ich];
        if (ch == UNICODE_NULL)
            szText[ich] = L' ';

        if (ch == L'\n')
        {
            if (bPrevCR)
            {
                n_cr--;
                n_crlf++;
            }
            else
                n_lf++;
        }
        else if (ch == '\r')
            n_cr++;

        bPrevCR = (ch == L'\r');
    }

    return n_cr + n_lf;
}

#endif
#if 0
            /* Get ready for ANSI-to-Wide conversion */
            /*
            cbContent = dwSize - dwPos;
            cchText = 0;
            if (cbContent > 0)
            {
                cchText = MultiByteToWideChar(iCodePage, 0, (LPCSTR)&pBytes[dwPos], (INT)cbContent, NULL, 0);
                if (cchText == 0)
                    goto done;
            }



            more_eol = CountNewLinesAndNulls(&pBytes[dwPos], cbContent);

            /* Re-allocate the buffer for EM_SETHANDLE */
            pszText = HeapReAlloc( hHeap, 0, pszText, (cchText + more_eol + 1) * sizeof(WCHAR));
            if (pszText == NULL)
                goto done;
            *ppszText = pszText;

            /* Do ANSI-to-Wide conversion */
            if (cbContent > 0)
            {
                if (!MultiByteToWideChar(iCodePage, 0,
                        (LPCSTR)&pBytes[dwPos], (INT)cbContent, pszText, (INT)cchText))
                    goto done;
            }
#endif
