/*
 * PROJECT:    ReactOS Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 * COPYRIGHT:  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 *             Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *             Copyright 2002 Andriy Palamarchuk
 */

#include "notepad.h"

#include <winreg.h>

#include <assert.h>
#include <strsafe.h>

SETTING_DATA Settings;

static LPCWSTR s_szRegistryKey = L"Software\\Microsoft\\Notepad";
static DWORD dwPointSize, dx, dy;

static BOOL LoadSettings(VOID);
static BOOL SaveSettings(VOID);

/***********************************************************************
 *           NOTEPAD_LoadSettingsFromRegistry
 *
 *  Load settings from registry HKCU\Software\Microsoft\Notepad or "~\AppData\Local\XPAccApps.ini" file. 
 */
VOID LoadAppSettings(VOID)
{
    DWORD _data, _cb;
#define LoadRegInt(name, var, defval) \
        do { _cb = sizeof(DWORD); (Settings.var) = ( hKey && RegGetValueW(hKey, NULL, L#name, \
        RRF_RT_DWORD|RRF_ZEROONFAILURE, NULL, &_data, &_cb) == ERROR_SUCCESS ) ? _data : (defval);} while(0)

#define LoadRegStr(name, buf) \
        do { _cb = _countof(Settings.buf); if (hKey) RegGetValueW( \
            hKey, NULL, L#name, RRF_RT_REG_SZ, NULL, (Settings.buf), &_cb);} while(0)

    HKEY hKey = NULL;
    HFONT hFont;

    DWORD cxScreen = GetSystemMetrics(SM_CXSCREEN), cyScreen = GetSystemMetrics(SM_CYSCREEN);
    dx = min((cxScreen * 1) / 3, 700);
    dy = min((cyScreen * 3) / 4, 1000);
    SetRect( &Settings.main_rect, CW_USEDEFAULT, CW_USEDEFAULT, dx, dy );

    /* Set the default values */
    Settings.bShowStatusBar = TRUE;
    Settings.bWrapLongLines = FALSE;
    SetRect(&Settings.lMargins, 750, 1000, 750, 1000);
    ZEROMEM(Settings.lfFont);
    Settings.lfFont.lfCharSet = DEFAULT_CHARSET;
    dwPointSize = 100;
    Settings.lfFont.lfWeight = FW_NORMAL;
    Settings.lfFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;

    /* FIXME: Settings.fSaveWindowPositions = FALSE; */
    /* FIXME: Settings.fMLE_is_broken = FALSE; */

    LOADSTRING(STRING_DEFAULTFONT, Settings.lfFont.lfFaceName );
    LOADSTRING(STRING_PAGESETUP_HEADERVALUE, Settings.lfFont.lfFaceName );
    LOADSTRING(STRING_PAGESETUP_HEADERVALUE, Settings.szHeader );
    LOADSTRING(STRING_PAGESETUP_FOOTERVALUE, Settings.szFooter );

    if (LoadSettings())
        goto skip_reg;

    /* Open the target registry key */
    if (RegOpenKey(HKEY_CURRENT_USER, s_szRegistryKey, &hKey) != ERROR_SUCCESS)
        hKey = NULL;

    /* Load the values from registry */
    if (hKey)
    {
        LoadRegInt("lfCharSet", lfFont.lfCharSet , DEFAULT_CHARSET );
        LoadRegInt("lfClipPrecision", lfFont.lfClipPrecision, 0);
        LoadRegInt("lfEscapement", lfFont.lfEscapement, 0);
        LoadRegInt("lfItalic", lfFont.lfItalic, 0);
        LoadRegInt("lfOrientation", lfFont.lfOrientation, 0);
        LoadRegInt("lfOutPrecision", lfFont.lfOutPrecision, 0 );
        LoadRegInt("lfPitchAndFamily", lfFont.lfPitchAndFamily, FIXED_PITCH | FF_MODERN);
        LoadRegInt("lfQuality", lfFont.lfQuality, 0);
        LoadRegInt("lfStrikeOut", lfFont.lfStrikeOut, 0);
        LoadRegInt("lfUnderline", lfFont.lfUnderline,0);
        LoadRegInt("lfWeight", lfFont.lfWeight,FW_NORMAL);
        LoadRegInt("fWrap", bWrapLongLines, TRUE);
        LoadRegInt("fStatusBar", bShowStatusBar, TRUE);

        LoadRegInt("iMarginLeft", lMargins.left, 750);
        LoadRegInt("iMarginTop", lMargins.top, 1000);
        LoadRegInt("iMarginRight", lMargins.right,750);
        LoadRegInt("iMarginBottom", lMargins.bottom, 1000);

        LoadRegInt("iWindowPosX", main_rect.left, 0);
        LoadRegInt("iWindowPosY", main_rect.top, 0);

        _cb = sizeof(DWORD);
        RegGetValueW(hKey, NULL, L"iPointSize", RRF_RT_DWORD, NULL, &dwPointSize, &_cb);
        RegGetValueW(hKey, NULL, L"iPointSize", RRF_RT_DWORD, NULL, &dx, &_cb);
        RegGetValueW(hKey, NULL, L"iPointSize", RRF_RT_DWORD, NULL, &dy, &_cb);

        LoadRegStr("searchString", szFindText);
        LoadRegStr("replaceString", szReplaceText);
        LoadRegStr("txtTypeFilter", txtTypeFilter);
        LoadRegStr("moreTypeFilter", moreTypeFilter);

        LoadRegStr("lfFaceName", lfFont.lfFaceName);
        LoadRegStr("szHeader", szHeader);
        LoadRegStr("szTrailer", szFooter);

        RegCloseKey(hKey);
    }

skip_reg:

    // Settings.lfFont.lfHeight = HeightFromPointSize(dwPointSize);
    /* The value is stored as 10 * twips */
    Settings.lfFont.lfHeight = -MulDiv(abs(dwPointSize), GetDpiForWindow(GetDesktopWindow()), 720);
    Settings.main_rect.right = Settings.main_rect.left + dx;
    Settings.main_rect.bottom = Settings.main_rect.top + dy;

    /* WORKAROUND: Far East Asian users may not have suitable fixed-pitch fonts. */
    switch (PRIMARYLANGID(GetUserDefaultLangID()))
    {
        case LANG_CHINESE:
        case LANG_JAPANESE:
        case LANG_KOREAN:
            Settings.lfFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
            break;
    }

    hFont = CreateFontIndirect(&Settings.lfFont);
    SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    if (hFont)
    {
        if (Globals.hFont)
            DeleteObject(Globals.hFont);
        Globals.hFont = hFont;
    }
}

static BOOL SaveDword(HKEY hKey, LPCWSTR pszValueNameT, DWORD dwValue)
{
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue)) == ERROR_SUCCESS;
}

static BOOL SaveString(HKEY hKey, LPCWSTR pszValueNameT, LPCWSTR pszValue)
{
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_SZ, (LPBYTE) pszValue, (DWORD) _tcslen(pszValue) * sizeof(*pszValue)) == ERROR_SUCCESS;
}

/***********************************************************************
 *           NOTEPAD_SaveSettingsToRegistry
 *
 *  Save settings to registry HKCU\Software\Microsoft\Notepad or or "~\AppData\Local\XPAccApps.ini" file. 
 */
VOID SaveAppSettings(VOID)
{
     HKEY hKey = NULL;
     DWORD dwDisposition;

    // GetWindowRect(Globals.hMainWnd, &Settings.main_rect);

    WINDOWPLACEMENT wndpl;
    wndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(Globals.hMainWnd, &wndpl);
    Settings.main_rect = wndpl.rcNormalPosition;

    /* Store the current value as 10 * twips */
    dwPointSize = MulDiv(abs(Settings.lfFont.lfHeight), 720, GetDpiForWindow(GetDesktopWindow()));
    /* round to nearest multiple of 10 */
    dwPointSize -= (dwPointSize+5) % 10 -5;

    if (SaveSettings())
        return;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, s_szRegistryKey,
                       0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKey, &dwDisposition) == ERROR_SUCCESS)
    {
        SaveDword(hKey, L"lfCharSet", Settings.lfFont.lfCharSet);
        SaveDword(hKey, L"lfClipPrecision", Settings.lfFont.lfClipPrecision);
        SaveDword(hKey, L"lfEscapement", Settings.lfFont.lfEscapement);
        SaveString(hKey, L"lfFaceName", Settings.lfFont.lfFaceName);
        SaveDword(hKey, L"lfItalic", Settings.lfFont.lfItalic);
        SaveDword(hKey, L"lfOrientation", Settings.lfFont.lfOrientation);
        SaveDword(hKey, L"lfOutPrecision", Settings.lfFont.lfOutPrecision);
        SaveDword(hKey, L"lfPitchAndFamily", Settings.lfFont.lfPitchAndFamily);
        SaveDword(hKey, L"lfQuality", Settings.lfFont.lfQuality);
        SaveDword(hKey, L"lfStrikeOut", Settings.lfFont.lfStrikeOut);
        SaveDword(hKey, L"lfUnderline", Settings.lfFont.lfUnderline);
        SaveDword(hKey, L"lfWeight", Settings.lfFont.lfWeight);
        // SaveDword(hKey, L"iPointSize", PointSizeFromHeight(Settings.lfFont.lfHeight));
        SaveDword(hKey, L"iPointSize", dwPointSize );

        SaveDword(hKey, L"fWrap", Settings.bWrapLongLines ? 1 : 0);
        SaveDword(hKey, L"fStatusBar", Settings.bShowStatusBar ? 1 : 0);
        SaveString(hKey, L"szHeader", Settings.szHeader);
        SaveString(hKey, L"szTrailer", Settings.szFooter);
        SaveDword(hKey, L"iMarginLeft", Settings.lMargins.left);
        SaveDword(hKey, L"iMarginTop", Settings.lMargins.top);
        SaveDword(hKey, L"iMarginRight", Settings.lMargins.right);
        SaveDword(hKey, L"iMarginBottom", Settings.lMargins.bottom);
        SaveDword(hKey, L"iWindowPosX", Settings.main_rect.left);
        SaveDword(hKey, L"iWindowPosY", Settings.main_rect.top);
        SaveDword(hKey, L"iWindowPosDX", Settings.main_rect.right - Settings.main_rect.left);
        SaveDword(hKey, L"iWindowPosDY", Settings.main_rect.bottom - Settings.main_rect.top);
        SaveString(hKey, L"searchString", Settings.szFindText);
        SaveString(hKey, L"replaceString", Settings.szReplaceText);
        SaveString(hKey, L"txtTypeFilter", Settings.txtTypeFilter);
        SaveString(hKey, L"moreTypeFilter", Settings.moreTypeFilter);

        RegCloseKey(hKey);
    }
}

// Get resource string, returns pointer to internal static buffer.
LPCWSTR GetString(UINT rid)
{
    static WCHAR _strbuf[STR_LONG];
    return ( LoadString( NULL, rid, _strbuf, _countof(_strbuf)) > 0
        ? _strbuf : L"");
}

#ifdef PRIVATEPROFILE_INI

#include <Shlobj.h>

#define _PFSECTION L"XNotePad"

static WCHAR _ProfilePath[MAX_PATH] = {0};

// read ini file entry and return pointer to internal buffer
LPCWSTR ReadIniString(LPCWSTR key, LPCWSTR defval)
{
    static WCHAR _inibuf[STR_LONG];
    return ((GetPrivateProfileString(_PFSECTION, key, L"",
            _inibuf, STR_LONG, _ProfilePath) > 0) ? _inibuf : defval);
}

// write string to ini file
VOID WriteIniString(LPCWSTR key, LPCWSTR value)
{
    WritePrivateProfileString(_PFSECTION, key, value, _ProfilePath);;
}

static BOOL LoadSettings(VOID)
{
#define GETINIINT(key, def) \
            GetPrivateProfileInt(_PFSECTION, L##key, def, _ProfilePath)

#define LOADSETTINGSTR(key, var,def) \
            GetPrivateProfileString(_PFSECTION, L##key, L##def, \
            Settings.var, _countof(Settings.var), _ProfilePath)

    if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, _ProfilePath)))
        return FALSE;
    _tcscat_s(_ProfilePath, MAX_PATH, L"\\" TEXT(PRIVATEPROFILE_INI));

    if (!FileExists(_ProfilePath))
        return FALSE;

    Settings.lfFont.lfCharSet = GETINIINT("lfCharSet", DEFAULT_CHARSET);
    Settings.lfFont.lfClipPrecision = GETINIINT("lfClipPrecision", 0 );
    Settings.lfFont.lfEscapement = GETINIINT("lfEscapement", 0 );
    Settings.lfFont.lfItalic = GETINIINT("lfItalic", 0 );
    Settings.lfFont.lfOrientation = GETINIINT("lfOrientation", 0 );

    Settings.lfFont.lfOutPrecision =  GETINIINT("lfOutPrecision", 0 );
    Settings.lfFont.lfPitchAndFamily = GETINIINT("lfPitchAndFamily", (FIXED_PITCH | FF_MODERN) );
    Settings.lfFont.lfQuality =  GETINIINT("lfQuality", 0 );
    Settings.lfFont.lfStrikeOut = GETINIINT("lfStrikeOut", 0 );
    Settings.lfFont.lfStrikeOut = GETINIINT("lfUnderline", 0 );
    Settings.lfFont.lfWeight = GETINIINT("lfWeight", FW_NORMAL);
    dwPointSize = GETINIINT("iPointSize", 100);

    Settings.bWrapLongLines = GETINIINT("fWrap", TRUE );
    Settings.bShowStatusBar = GETINIINT("fStatusBar", TRUE );
    Settings.lMargins.left = GETINIINT("iMarginLeft", 750 );
    Settings.lMargins.top = GETINIINT("iMarginTop", 1000 );
    Settings.lMargins.right = GETINIINT("iMarginRight", 750 );
    Settings.lMargins.bottom = GETINIINT("iMarginBottom", 1000 );

    Settings.main_rect.left = GETINIINT("iWindowPosX", 0 );
    Settings.main_rect.top = GETINIINT("iWindowPosY", 0 );
    dx = GETINIINT("iWindowPosDX", dx);
    dy = GETINIINT("iWindowPosDY", dy);

    LOADSETTINGSTR("searchString", szFindText, "" );
    LOADSETTINGSTR("replaceString", szReplaceText, "" );

    LOADSETTINGSTR("lfFaceName", lfFont.lfFaceName, "");
    LOADSETTINGSTR("szHeader", szHeader, "" );
    LOADSETTINGSTR("szTrailer", szFooter, "" );

    LOADSETTINGSTR("txtTypeFilter", txtTypeFilter, "*.txt;*.md;*.mdx" );
    LOADSETTINGSTR("moreTypeFilter", moreTypeFilter, "" );

    MRU_Load();

    return TRUE;
}

static BOOL SaveSettings(VOID)
{
    WCHAR _savebuf[256];
#define SAVESETTINGINT(key, value) \
            _itot_s( Settings.value, _savebuf, _countof(_savebuf), 10),\
            WritePrivateProfileString(_PFSECTION, L##key, _savebuf, _ProfilePath)
#define SAVESETTINGSTR(key, str) \
            WritePrivateProfileString(_PFSECTION, L##key, Settings.str, _ProfilePath);

    SAVESETTINGINT("lfCharSet", lfFont.lfCharSet );
    SAVESETTINGINT("lfClipPrecision", lfFont.lfClipPrecision );
    SAVESETTINGINT("lfEscapement", lfFont.lfEscapement );
    SAVESETTINGSTR("lfFaceName", lfFont.lfFaceName);
    SAVESETTINGINT("lfItalic", lfFont.lfItalic );
    SAVESETTINGINT("lfOrientation", lfFont.lfOrientation );
    SAVESETTINGINT("lfOutPrecision", lfFont.lfOutPrecision );
    SAVESETTINGINT("lfPitchAndFamily", lfFont.lfPitchAndFamily );
    SAVESETTINGINT("lfQuality", lfFont.lfQuality );
    SAVESETTINGINT("lfStrikeOut", lfFont.lfStrikeOut );
    SAVESETTINGINT("lfUnderline", lfFont.lfUnderline );
    SAVESETTINGINT("lfWeight", lfFont.lfWeight);

    _itot_s( dwPointSize, _savebuf, _countof(_savebuf), 10);
    WritePrivateProfileString(_PFSECTION, L"iPointSize", _savebuf, _ProfilePath);

    SAVESETTINGINT("fWrap", bWrapLongLines ? 1 : 0);
    SAVESETTINGINT("fStatusBar", bShowStatusBar ? 1 : 0);
    SAVESETTINGSTR("szHeader", szHeader );
    SAVESETTINGSTR("szTrailer", szFooter );
    SAVESETTINGINT("iMarginLeft", lMargins.left);
    SAVESETTINGINT("iMarginTop", lMargins.top);
    SAVESETTINGINT("iMarginRight", lMargins.right);
    SAVESETTINGINT("iMarginBottom", lMargins.bottom);
    SAVESETTINGINT("iWindowPosX", main_rect.left);
    SAVESETTINGINT("iWindowPosY", main_rect.top);
    SAVESETTINGINT("iWindowPosDX", main_rect.right - Settings.main_rect.left);
    SAVESETTINGINT("iWindowPosDY", main_rect.bottom - Settings.main_rect.top);
    SAVESETTINGSTR("searchString", szFindText );
    SAVESETTINGSTR("replaceString", szReplaceText );

    SAVESETTINGSTR("txtTypeFilter",  txtTypeFilter );
    SAVESETTINGSTR("moreTypeFilter",  moreTypeFilter );

    MRU_Sort();
    MRU_Save();

    return FileExists(_ProfilePath);
}
#else
static BOOL LoadSettings(VOID) { return FALSE;}
static BOOL SaveSettings(VOID) { return FALSE;}
#endif // PRIVATEPROFILE_INI
