/*
 * ReactOS Calc (Theming support)
 *
 * Copyright 2007-2017, Carlo Bramini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "calc.h"

#define GET_CB(name) \
    calc_##name = (type_##name)GetProcAddress(hUxTheme, #name); \
    if (calc_##name == NULL) calc_##name = dummy_##name;

static HTHEME WINAPI
dummy_OpenThemeData(HWND hwnd, const WCHAR *pszClassList);

static HRESULT WINAPI
dummy_CloseThemeData(HTHEME hTheme);

static HRESULT WINAPI
dummy_DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
    const RECT *prc, const RECT *prcClip);

static BOOL WINAPI
dummy_IsAppThemed(void);

static BOOL WINAPI
dummy_IsThemeActive(void);

static BOOL WINAPI
dummy_IsThemeBackgroundPartiallyTransparent(HTHEME hTheme, int iPartId, int iStateId);

static HRESULT WINAPI
dummy_DrawThemeParentBackground(HWND hWnd, HDC hdc, RECT *prc);


type_OpenThemeData       calc_OpenThemeData       = dummy_OpenThemeData;
type_CloseThemeData      calc_CloseThemeData      = dummy_CloseThemeData;
type_DrawThemeBackground calc_DrawThemeBackground = dummy_DrawThemeBackground;
type_IsAppThemed         calc_IsAppThemed         = dummy_IsAppThemed;
type_IsThemeActive       calc_IsThemeActive       = dummy_IsThemeActive;
type_IsThemeBackgroundPartiallyTransparent calc_IsThemeBackgroundPartiallyTransparent = \
    dummy_IsThemeBackgroundPartiallyTransparent;
type_DrawThemeParentBackground calc_DrawThemeParentBackground = \
    dummy_DrawThemeParentBackground;

static HMODULE hUxTheme;

static HTHEME WINAPI
dummy_OpenThemeData(HWND hwnd, const WCHAR* pszClassList)
{
    return NULL;
}

static HRESULT WINAPI
dummy_CloseThemeData(HTHEME hTheme)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI
dummy_DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
            const RECT* prc, const RECT* prcClip)
{
    return E_NOTIMPL;
}

static BOOL WINAPI
dummy_IsAppThemed(void)
{
    return FALSE;
}

static BOOL WINAPI
dummy_IsThemeActive(void)
{
    return FALSE;
}

static BOOL WINAPI
dummy_IsThemeBackgroundPartiallyTransparent(HTHEME hTheme, int iPartId, int iStateId)
{
    return FALSE;
}

static HRESULT WINAPI
dummy_DrawThemeParentBackground(HWND hWnd, HDC hdc, RECT *prc)
{
    return E_NOTIMPL;
}

void Theme_Start(HINSTANCE hInstance)
{
    hUxTheme = LoadLibrary(_T("UXTHEME"));
    if (hUxTheme == NULL)
        return;

    GET_CB(OpenThemeData)
    GET_CB(CloseThemeData)
    GET_CB(DrawThemeBackground)
    GET_CB(IsAppThemed)
    GET_CB(IsThemeActive)
    GET_CB(IsThemeBackgroundPartiallyTransparent)
    GET_CB(DrawThemeParentBackground)
}

void Theme_Stop(void)
{
    if(hUxTheme == NULL)
        return;

    FreeLibrary(hUxTheme);
    hUxTheme = NULL;
}

// -----------------------------------

/*
 * To make tooltip work, include Common Control 6.0 as dependancy
 * in embedded "manifest.xml". It should have <dependentAssembly> entry like: 
 * "name="Microsoft.Windows.Common-Controls" version="6.0.0.0" ...
 * And set SS_NOTIFY flag on the button/text control style.
 */

static HWND CreateToolTip(TOOLINFO *pToolInfo, HINSTANCE hInst)
{
    if (!pToolInfo || !pToolInfo->hwnd || !pToolInfo->lpszText || !hInst)
        return NULL;

    HWND hwTip = CreateWindowEx(
                    0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    pToolInfo->hwnd, NULL, hInst, NULL);
    if (!hwTip)
        return NULL;

    SetWindowPos(hwTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    if (!SendMessage(hwTip, TTM_ADDTOOL, 0, (LPARAM)pToolInfo)) {
        DestroyWindow(hwTip);
        hwTip = NULL;
    }
    pToolInfo->lParam = (LPARAM) hwTip;
    return hwTip;
}

// Pretty print doublue: returns static string. Used to format memory tooltip.
TCHAR *string_double(double fval)
{
    static TCHAR buff[50];
    TCHAR *fmt = ( abs(fval) < 1.0e+14 && abs(fval) > 1.0e-6 )
                ? L"%-35.15f" : L"%-30.15g";
    swprintf_s( buff, 40, fmt, fval );

    // remove trailing zeros
    TCHAR *pt = buff + _tcslen(buff) - 1;
    while ( *pt == L' ' || *pt == L'0' ) pt--;
    pt[1] = L'\0';
    return  buff;
}

// Move/resize control 
static HWND MoveControl(HWND hDlg, int item, int dx, int dy, int dw, int dh, BOOL bReaint)
{
    HWND hctr = GetDlgItem(hDlg, item);
    RECT cr;
    GetWindowRect(hctr, &cr);
    POINT coord = {cr.left, cr.top};
    ScreenToClient(hDlg, &coord);
    MoveWindow(hctr, coord.x + dx, coord.y + dy,
                cr.right - cr.left + dw, cr.bottom - cr.top + dh, bReaint);
    return hctr;
}

// Adjust buttons layout
void AdjustLayout(HWND hDlg, DWORD dwLayout)
{
#define MOVE(iid, dx, dy) MoveControl(hDlg, iid, (dx), (dy), 0, 0, TRUE)
    int RH = 30;    // button row height
    RECT dwr;
    GetWindowRect(hDlg, &dwr);

    HWND display = MoveControl(hDlg, IDC_TEXT_OUTPUT, 0, 0, 0, 5, TRUE);
    SendMessage(display, WM_SETFONT, 0, MAKELPARAM(FALSE, 0));
    SendMessage(display, WM_SETTEXT, 0, (LPARAM) _T("Hello"));

    if (dwLayout == IDD_DIALOG_STANDARD && GetDlgItem(hDlg, IDC_BUTTON_LEFTPAR)) {

        MOVE(IDC_TEXT_PARENT, -1000, -1000); // remove spurious residue
        MoveControl(hDlg, IDC_BUTTON_SQRT, -1000, -1000, 0, RH, TRUE); // Remove button

        MoveWindow(hDlg, dwr.left, dwr.top, dwr.right - dwr.left + 0,
            dwr.bottom - dwr.top + RH, TRUE);

        MOVE( IDC_TEXT_PARENT, 1000, 1000);

        MOVE( IDC_BUTTON_7, 0, RH);
        MOVE( IDC_BUTTON_4, 0, RH);
        MOVE( IDC_BUTTON_1, 0, RH);
        MOVE( IDC_BUTTON_0, 0, RH);

        MOVE( IDC_BUTTON_8, 0, RH);
        MOVE( IDC_BUTTON_5, 0, RH);
        MOVE( IDC_BUTTON_2, 0, RH);
        MOVE( IDC_BUTTON_SIGN, 0, RH);

        MOVE( IDC_BUTTON_9, 0, RH);
        MOVE( IDC_BUTTON_6, 0, RH);
        MOVE( IDC_BUTTON_3, 0, RH);
        MOVE( IDC_BUTTON_DOT, 0, RH);

        MoveControl(hDlg, IDC_BUTTON_ADD, 0, 0, 0, RH, TRUE); // enlarge
        MoveControl(hDlg, IDC_BUTTON_EQU, 0, 0, 0, RH, TRUE);
        MOVE( IDC_BUTTON_PERCENT, 0, RH);
        MOVE( IDC_BUTTON_RX, 0, -2*RH);
        MOVE( IDC_BUTTON_MP, 0, RH);
    }
    else if (dwLayout == IDD_DIALOG_CONVERSION) {
        MOVE( IDC_BUTTON_PERCENT, 0, RH-2);
        MOVE( IDC_BUTTON_RX, 0, -2*RH+3);
        MOVE( IDC_BUTTON_SQRT, 0, RH-1);
    }

    HWND hwmem = GetDlgItem(hDlg, IDC_TEXT_MEMORY);
    long style = GetWindowLong(hwmem, GWL_STYLE);
    SetWindowLong(hwmem, GWL_STYLE, style | SS_NOTIFY);

     // Associate the tooltip with the control.
    static TOOLINFO mem_ti;
    mem_ti = (TOOLINFO){0};
    mem_ti.cbSize = sizeof(TOOLINFO);
    mem_ti.hwnd = hDlg;
    mem_ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    mem_ti.uId = (UINT_PTR)hwmem;
    mem_ti.lpszText = L"Memory tooltip";
    mem_ti.lParam = (LPARAM) NULL;

    HWND hwTip = CreateToolTip(&mem_ti, calc.hInstance);

    if (hwTip) {
        SendMessage(hwTip, TTM_ACTIVATE, TRUE, 0);
        mem_ti.lParam = (LPARAM) hwTip;
        calc.memory_ToolTip = &mem_ti;
    }
}
