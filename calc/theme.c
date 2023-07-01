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

HWND MoveControl(HWND hDlg, int item, int dx, int dy, int dw, int dh, BOOL bReaint)
{
    HWND hctr = GetDlgItem(hDlg, item);
    RECT r;
    GetWindowRect(hctr, &r);
    POINT coord = {r.left, r.top};
    ScreenToClient(hDlg, &coord);
    MoveWindow(hctr, coord.x + dx, coord.y + dy, r.right - r.left + dw,
        r.bottom - r.top + dh, bReaint);
    return hctr;
}

void AdjustLayout(HWND hDlg, DWORD dwLayout)
{
    if (dwLayout != IDD_DIALOG_STANDARD || !GetDlgItem(hDlg, IDC_BUTTON_LEFTPAR))
        return;

#define MOVE(iid, dx, dy) MoveControl(hDlg, iid, dx, dy, 0, 0, TRUE)
    int RH = 30;    // button row height 

    MOVE(IDC_TEXT_PARENT, -1000, -1000); // remove spurious residue
    MoveControl(hDlg, IDC_BUTTON_SQRT, -1000, -1000, 0, RH, TRUE); // Remove button

    RECT r;
    GetWindowRect(hDlg, &r);
    MoveWindow(hDlg, r.left, r.top, r.right - r.left + 0,
        r.bottom - r.top + RH, TRUE);

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

    HWND display = MoveControl(hDlg, IDC_TEXT_OUTPUT, 0, 0, 0, 5, TRUE);
    SendMessage(display, WM_SETFONT, 0, MAKELPARAM(FALSE, 0));
    SendMessage(display, WM_SETTEXT, 0, (LPARAM) _T("Hello"));

/*
    HWND hmem = GetDlgItem(hDlg, IDC_BUTTON_7);
    HWND htt = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hDlg, NULL, calc.hInstance, NULL);

    SetWindowPos(htt, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Associate the tooltip with the tool.
    TOOLINFO tti = {0};
    tti.cbSize = sizeof(tti);
    tti.hwnd = hDlg;
    tti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    tti.uId = (UINT_PTR)hmem;
    // tti.hinst = calc.hInstance;
    tti.lpszText = _T("pszTextTEST");
    SendMessage(htt, TTM_ADDTOOL, 0, (LPARAM)&tti);
*/
}
