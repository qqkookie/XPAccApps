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

static LPCTSTR s_szRegistryKey = _T("Software\\Microsoft\\Notepad");
static DWORD dwPointSize, dx, dy;

#ifdef PRIVATEPROFILE_INI
#include <Shlobj.h>

#define PFSECTION _T("XNotePad")

static TCHAR ProfilePath[MAX_PATH] = {0};
static BOOL LoadSettingsFromProfile(VOID);
static BOOL SaveSettingsFromProfile(VOID);

#endif

// Get resource string, returns pointer to internal static buffer.
LPCTSTR GetString(UINT rid)
{
    static TCHAR _strbuf[STR_LONG];
    return ( LoadString( NULL, rid, _strbuf, _countof(_strbuf)) > 0
        ? _strbuf : _T(""));
}

/***********************************************************************
 *           NOTEPAD_LoadSettingsFromRegistry
 *
 *  Load settings from registry HKCU\Software\Microsoft\Notepad or "~\AppData\Local\XPAccApps.ini" file. 
 */
VOID LoadAppSettings(VOID)
{
    DWORD _data, _cb;
#define LoadRegInt(name, var, defval) \
        do { _cb = sizeof(DWORD); (Settings.var) = ( hKey && RegGetValueW(hKey, NULL, _T(name), \
        RRF_RT_DWORD|RRF_ZEROONFAILURE, NULL, &_data, &_cb) == ERROR_SUCCESS ) ? _data : (defval);} while(0)

#define LoadRegStr(name, buf) \
        do { _cb = _countof(Settings.buf); if (hKey) RegGetValueW( \
            hKey, NULL, _T(name), RRF_RT_REG_SZ, NULL, (Settings.buf), &_cb);} while(0)

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

#ifdef PRIVATEPROFILE_INI
    if (LoadSettingsFromProfile())
        goto skip_reg;
#endif

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
        RegGetValueW(hKey, NULL, _T("iPointSize"), RRF_RT_DWORD, NULL, &dwPointSize, &_cb);
        RegGetValueW(hKey, NULL, _T("iPointSize"), RRF_RT_DWORD, NULL, &dx, &_cb);
        RegGetValueW(hKey, NULL, _T("iPointSize"), RRF_RT_DWORD, NULL, &dy, &_cb);

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

static BOOL SaveDword(HKEY hKey, LPCTSTR pszValueNameT, DWORD dwValue)
{
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue)) == ERROR_SUCCESS;
}

static BOOL SaveString(HKEY hKey, LPCTSTR pszValueNameT, LPCTSTR pszValue)
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

#ifdef PRIVATEPROFILE_INI
    if (SaveSettingsFromProfile())
        return;
#endif

    if (RegCreateKeyEx(HKEY_CURRENT_USER, s_szRegistryKey,
                       0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKey, &dwDisposition) == ERROR_SUCCESS)
    {
        SaveDword(hKey, _T("lfCharSet"), Settings.lfFont.lfCharSet);
        SaveDword(hKey, _T("lfClipPrecision"), Settings.lfFont.lfClipPrecision);
        SaveDword(hKey, _T("lfEscapement"), Settings.lfFont.lfEscapement);
        SaveString(hKey, _T("lfFaceName"), Settings.lfFont.lfFaceName);
        SaveDword(hKey, _T("lfItalic"), Settings.lfFont.lfItalic);
        SaveDword(hKey, _T("lfOrientation"), Settings.lfFont.lfOrientation);
        SaveDword(hKey, _T("lfOutPrecision"), Settings.lfFont.lfOutPrecision);
        SaveDword(hKey, _T("lfPitchAndFamily"), Settings.lfFont.lfPitchAndFamily);
        SaveDword(hKey, _T("lfQuality"), Settings.lfFont.lfQuality);
        SaveDword(hKey, _T("lfStrikeOut"), Settings.lfFont.lfStrikeOut);
        SaveDword(hKey, _T("lfUnderline"), Settings.lfFont.lfUnderline);
        SaveDword(hKey, _T("lfWeight"), Settings.lfFont.lfWeight);
        // SaveDword(hKey, _T("iPointSize"), PointSizeFromHeight(Settings.lfFont.lfHeight));
        SaveDword(hKey, _T("iPointSize"), dwPointSize );

        SaveDword(hKey, _T("fWrap"), Settings.bWrapLongLines ? 1 : 0);
        SaveDword(hKey, _T("fStatusBar"), Settings.bShowStatusBar ? 1 : 0);
        SaveString(hKey, _T("szHeader"), Settings.szHeader);
        SaveString(hKey, _T("szTrailer"), Settings.szFooter);
        SaveDword(hKey, _T("iMarginLeft"), Settings.lMargins.left);
        SaveDword(hKey, _T("iMarginTop"), Settings.lMargins.top);
        SaveDword(hKey, _T("iMarginRight"), Settings.lMargins.right);
        SaveDword(hKey, _T("iMarginBottom"), Settings.lMargins.bottom);
        SaveDword(hKey, _T("iWindowPosX"), Settings.main_rect.left);
        SaveDword(hKey, _T("iWindowPosY"), Settings.main_rect.top);
        SaveDword(hKey, _T("iWindowPosDX"), Settings.main_rect.right - Settings.main_rect.left);
        SaveDword(hKey, _T("iWindowPosDY"), Settings.main_rect.bottom - Settings.main_rect.top);
        SaveString(hKey, _T("searchString"), Settings.szFindText);
        SaveString(hKey, _T("replaceString"), Settings.szReplaceText);
        SaveString(hKey, _T("txtTypeFilter"), Settings.txtTypeFilter);
        SaveString(hKey, _T("moreTypeFilter"), Settings.moreTypeFilter);

        RegCloseKey(hKey);
    }
}

#ifdef PRIVATEPROFILE_INI

// read ini file and copy to buffer
int LoadIniString(LPCTSTR key, LPTSTR buf, int buflen)
{
    return GetPrivateProfileString(PFSECTION, key, _T(""),
        buf, buflen, ProfilePath);
}

// read ini file entry and return pointer to internal buffer
LPCTSTR GetIniString(LPCTSTR key, LPCTSTR defval)
{
    static TCHAR _inibuf[STR_LONG];
    return ((GetPrivateProfileString(PFSECTION, key, _T(""),
            _inibuf, STR_LONG, ProfilePath) > 0) ? _inibuf : defval);
}

// write string to ini file
VOID PutIniString(LPCTSTR key, LPCTSTR value)
{
    WritePrivateProfileString(PFSECTION, key, value, ProfilePath);;
}

static BOOL LoadSettingsFromProfile(VOID)
{
#define LOADSETTINGINT(key, var, def) \
            Settings.var = GetPrivateProfileInt(PFSECTION, _T(key), def, ProfilePath)

#define LOADSETTINGSTR(key, var,def) \
            GetPrivateProfileString(PFSECTION, _T(key), _T(def), \
            Settings.var, _countof(Settings.var), ProfilePath)

    if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, ProfilePath)))
        return FALSE;

    _tcscat_s(ProfilePath, MAX_PATH, _T("\\") _T(PRIVATEPROFILE_INI));

    LOADSETTINGINT("lfCharSet", lfFont.lfCharSet, DEFAULT_CHARSET);
    LOADSETTINGINT("lfClipPrecision", lfFont.lfClipPrecision, 0 );
    LOADSETTINGINT("lfEscapement", lfFont.lfEscapement, 0 );
    LOADSETTINGINT("lfItalic", lfFont.lfItalic, 0 );
    LOADSETTINGINT("lfOrientation", lfFont.lfOrientation, 0 );

    LOADSETTINGINT("lfOutPrecision", lfFont.lfOutPrecision, 0 );
    LOADSETTINGINT("lfPitchAndFamily", lfFont.lfPitchAndFamily, (FIXED_PITCH | FF_MODERN) );
    LOADSETTINGINT("lfQuality", lfFont.lfQuality, 0 );
    LOADSETTINGINT("lfStrikeOut", lfFont.lfStrikeOut, 0 );
    LOADSETTINGINT("lfUnderline", lfFont.lfStrikeOut, 0 );
    LOADSETTINGINT("lfWeight", lfFont.lfWeight, FW_NORMAL);
    dwPointSize = GetPrivateProfileInt(PFSECTION, _T("iPointSize"), 100, ProfilePath);

    LOADSETTINGINT("fWrap", bWrapLongLines, TRUE );
    LOADSETTINGINT("fStatusBar", bShowStatusBar, TRUE );
    LOADSETTINGINT("iMarginLeft", lMargins.left, 750);
    LOADSETTINGINT("iMarginTop", lMargins.top, 1000 );
    LOADSETTINGINT("iMarginRight", lMargins.right, 750 );
    LOADSETTINGINT("iMarginBottom", lMargins.bottom, 1000 );

    LOADSETTINGINT("iWindowPosX", main_rect.left, 0 );
    LOADSETTINGINT("iWindowPosY", main_rect.top, 0 );
    dx = GetPrivateProfileInt(PFSECTION, _T("iWindowPosDX"), dx, ProfilePath);
    dy = GetPrivateProfileInt(PFSECTION, _T("iWindowPosDY"), dy, ProfilePath);

    LOADSETTINGSTR("searchString", szFindText, "" );
    LOADSETTINGSTR("replaceString", szReplaceText, "" );

    LOADSETTINGSTR("lfFaceName", lfFont.lfFaceName, "");
    LOADSETTINGSTR("szHeader", szHeader, "" );
    LOADSETTINGSTR("szTrailer", szFooter, "" );

    LOADSETTINGSTR("txtTypeFilter", txtTypeFilter, "*.md;*.mdx" );
    LOADSETTINGSTR("moreTypeFilter", moreTypeFilter, "" );

    MRU_Load();

    if (FileExists(ProfilePath))
        return TRUE;
    return SaveSettingsFromProfile();
}

static BOOL SaveSettingsFromProfile(VOID)
{
    TCHAR _savebuf[256];
#define SAVESETTINGINT(name, value) \
            _itot_s( Settings.value, _savebuf, _countof(_savebuf), 10),\
            WritePrivateProfileString(PFSECTION, _T(name), _savebuf, ProfilePath)
#define SAVESETTINGSTR(name, value) \
            WritePrivateProfileString(PFSECTION, _T(name), Settings.value, ProfilePath);

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
    WritePrivateProfileString(PFSECTION, _T("iPointSize"), _savebuf, ProfilePath);

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

    return FileExists(ProfilePath);
}
#endif // PRIVATEPROFILE_INI
