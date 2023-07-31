/*
 *  Clock (winclock.c)
 *
 *  Copyright 1998 by Marcel Baur <mbaur@g26.ethz.ch>
 *
 *  This file is based on  rolex.c  by Jim Peterson.
 *
 *  I just managed to move the relevant parts into the Clock application
 *  and made it look like the original Windows one. You can find the original
 *  rolex.c in the wine /libtest directory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "main.h"

#define _USE_MATH_DEFINES 
#include <math.h>

// #define M_PI 3.14159265358979323846

#define DCLK_FONT   L"Arial"

static DWORD LightPalette[5];
static DWORD DarkPalette[5];
static DWORD (*Palette)[5];

#define FaceColor       ((*Palette)[0])
#define HandColor       ((*Palette)[1])
#define TickColor       ((*Palette)[2])
#define ShadowColor     ((*Palette)[3])
#define BackgroundColor ((*Palette)[4]) 

#define TICK_MIN_RADIUS             64
 
typedef struct
{
    POINT Start;
    POINT End;
} HandData;

static HandData HourHand, MinuteHand, SecondHand;

// Set  circle region for analog clock
void SetAnalogRegion(void)
{
    RECT rect;
    INT diameter = min( Globals.WinW, Globals.WinH );
    Globals.hCircle = CreateEllipticRgn(
            (Globals.WinW - diameter) / 2, (Globals.WinH - diameter) / 2,
            (Globals.WinW + diameter) / 2, (Globals.WinH + diameter) / 2 );

    GetWindowRect( Globals.hMainWnd, &rect );
    MapWindowPoints( 0, Globals.hMainWnd, (LPPOINT)&rect, 2 );
    OffsetRgn( Globals.hCircle, -rect.left, -rect.top );
    SetWindowRgn( Globals.hMainWnd, Globals.hCircle, TRUE );
}

// Analog clock ticks
static void DrawTicks(HDC dc, const POINT* centre, int radius, COLORREF color)
{
    int t;
    /* Minute divisions */
    if ( radius > TICK_MIN_RADIUS ) {
        DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 1 +radius/100 , color)));

        for(t=0; t<60; t++) {
            MoveToEx(dc,
                centre->x + round(sin(t*M_PI/30.0)*0.9*radius),
                centre->y - round(cos(t*M_PI/30.0)*0.9*radius), NULL);
	        LineTo(dc,
		        centre->x + round(sin(t*M_PI/30.0)*0.89*radius),
		        centre->y - round(cos(t*M_PI/30.0)*0.89*radius));
	    }
    }

    /* Hour divisions */
    DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 2 +radius/70, color)));

    for(t=0; t<12; t++) {
        MoveToEx(dc,
                 centre->x + round(sin(t*M_PI/6.0)*0.9*radius),
                 centre->y - round(cos(t*M_PI/6.0)*0.9*radius), NULL);
        LineTo(dc,
               centre->x + round(sin(t*M_PI/6.0)*0.8*radius),
               centre->y - round(cos(t*M_PI/6.0)*0.8*radius));
    }
}

// Analog clock face and ticks
static void DrawFace(HDC dc, const POINT* centre, int radius)
{
    /* Ticks */
    int offset = 1 + radius/200;

    SelectObject(dc, GetStockObject(NULL_PEN));

    OffsetWindowOrgEx(dc, -offset, -offset, NULL);
    DrawTicks(dc, centre, radius, ShadowColor);

    OffsetWindowOrgEx(dc, offset, offset, NULL);
    DrawTicks(dc, centre, radius, TickColor);

    if (Globals.bNoTitleBar)
    {
        SelectObject(dc, GetStockObject(NULL_BRUSH));

        int rim = 5 + radius/70;
        DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, rim, ShadowColor)));
        // radius -= rim/2;
        Ellipse(dc, centre->x - radius, centre->y - radius, centre->x + radius, centre->y + radius);
    }
    DeleteObject(SelectObject(dc, GetStockObject(NULL_PEN)));
}

// single hand
static void DrawHand(HDC dc, HandData* hand)
{
    MoveToEx(dc, hand->Start.x, hand->Start.y, NULL);
    LineTo(dc, hand->End.x, hand->End.y);
}

// draw hour, min, sec hands
static void DrawHands(HDC dc, int radius)
{
    if (Globals.bSeconds) {
#if 0
      	SelectObject(dc, CreatePen(PS_SOLID, 1, ShadowColor));
	    OffsetWindowOrgEx(dc, -SHADOW_DEPTH, -SHADOW_DEPTH, NULL);
            DrawHand(dc, &SecondHand);
	    DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 1, HandColor)));
	    OffsetWindowOrgEx(dc, SHADOW_DEPTH, SHADOW_DEPTH, NULL);
#else
	    SelectObject(dc, CreatePen(PS_SOLID, 1 + radius/200, HandColor));
#endif
        DrawHand(dc, &SecondHand);
	    DeleteObject(SelectObject(dc, GetStockObject(NULL_PEN)));
    }

    int offset = 1 + radius / 150;

    OffsetWindowOrgEx(dc, -offset, -offset, NULL);

    SelectObject(dc, CreatePen(PS_SOLID, 3 + radius/60, ShadowColor));
    DrawHand(dc, &MinuteHand);

    DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 4 +radius/40, ShadowColor)));
    DrawHand(dc, &HourHand);

    OffsetWindowOrgEx(dc, offset, offset, NULL);

    DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 3 +radius/60, HandColor)));
    DrawHand(dc, &MinuteHand);

    DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 4 +radius/40, HandColor)));
    DrawHand(dc, &HourHand);

    DeleteObject(SelectObject(dc, GetStockObject(NULL_PEN)));
}

static void PositionHand(const POINT* centre, double length, double angle, HandData* hand)
{
    hand->Start = *centre;
    hand->End.x = centre->x + round(sin(angle)*length);
    hand->End.y = centre->y - round(cos(angle)*length);
}

// set hands position
static void PositionHands(const POINT* centre, int radius)
{
    SYSTEMTIME st;
    double hour, minute, second;

    /* 0 <= hour,minute,second < 2pi */
    /* Adding the millisecond count makes the second hand move more smoothly */

    GetLocalTime(&st);

    second = st.wSecond + st.wMilliseconds/1000.0;
    minute = st.wMinute + second/60.0;
    hour   = st.wHour % 12 + minute/60.0;

    PositionHand(centre, radius * 0.5,  hour/12.0   * 2.0*M_PI, &HourHand);
    PositionHand(centre, radius * 0.65, minute/60.0 * 2.0*M_PI, &MinuteHand);
    if (Globals.bSeconds)
        PositionHand(centre, radius * 0.79, second/60.0 * 2.0*M_PI, &SecondHand);  
}

static void AnalogClock(HDC dc)
{
    POINT centre;
    int radius;
    
    radius = min(Globals.WinW, Globals.WinH)/2 - 2 ;
    if (radius < 20)
	    return;

    centre.x = Globals.WinW/2;
    centre.y = Globals.WinH/2;

    DrawFace(dc, &centre, radius);

    PositionHands(&centre, radius);
    DrawHands(dc, radius);
}

// Digital clock time
static LPCWSTR GetTimeString(void)
{
    static WCHAR szTime[MAX_STRING_LEN];
   UINT flag = (Globals.bSeconds ? 0: TIME_NOSECONDS)
                | (Globals.b24Hours ? TIME_FORCE24HOURFORMAT|TIME_NOTIMEMARKER:0);

   return (GetTimeFormatW(LOCALE_USER_DEFAULT, flag, 
            NULL, NULL, szTime, _countof(szTime)) ? szTime : L"") ;
}

// Auto-resize digital clock font
void ResizeFont(void)
{
#define MEASUREHEIGHT     -24
    static HFONT measurefont = NULL;

    if ( !measurefont || !Globals.hFont ) {
        LOGFONT lf;
        ZeroMemory(&lf, sizeof(lf));
        lf.lfHeight = MEASUREHEIGHT;
        wcscpy(lf.lfFaceName, DCLK_FONT);
        measurefont = CreateFontIndirectW(&lf);
        Globals.logfont = lf;
    }

    LPCWSTR szTime = GetTimeString();
    HDC dc = GetDC(Globals.hMainWnd);
    SelectObject(dc, measurefont);
    SIZE extent;
    GetTextExtentPointW(dc, szTime, wcslen(szTime), &extent);

    int xscale = MulDiv( Globals.WinW, 86, extent.cx); // 86%
    int yscale = MulDiv( Globals.WinH, 98, extent.cy); // 98%

    Globals.logfont.lfHeight = MulDiv(MEASUREHEIGHT, min(xscale, yscale), 100) +1;
    HFONT newFont = CreateFontIndirectW(&Globals.logfont);
    if (newFont) {
	    SelectObject(dc, newFont);
        DeleteObject(Globals.hFont);
	    Globals.hFont = newFont;
    }
    ReleaseDC(Globals.hMainWnd, dc);
}

static void DigitalClock(HDC dc)
{
    SIZE extent;
    HFONT oldFont;

     LPCWSTR szTime = GetTimeString();
    int len = wcslen(szTime);

    oldFont = SelectObject(dc, Globals.hFont);
    GetTextExtentPointW(dc, szTime, len, &extent);

    SetBkMode(dc, TRANSPARENT);
    SetBkColor(dc, BackgroundColor);

    int offset = extent.cy/80+ 2;
    SetTextColor(dc, ShadowColor);
    TextOutW(dc, (Globals.WinW - extent.cx)/2 + offset,
        (Globals.WinH - extent.cy)/2 + offset, szTime, len);

    SetTextColor(dc, HandColor);
    TextOutW(dc, (Globals.WinW - extent.cx)/2,
        (Globals.WinH - extent.cy)/2, szTime, len);

    SelectObject(dc, oldFont);
}

/***********************************************************************
 *  Handle WM_PAINT
 */
void DrawClock(void)
{
    PAINTSTRUCT ps;
    HDC dcMem, dc;
    HBITMAP bmMem, bmOld;

    dc = BeginPaint(Globals.hMainWnd, &ps);

    /* Use an offscreen dc to avoid flicker */
    dcMem = CreateCompatibleDC(dc);
    bmMem = CreateCompatibleBitmap(dc, ps.rcPaint.right - ps.rcPaint.left,
				    ps.rcPaint.bottom - ps.rcPaint.top);

    bmOld = SelectObject(dcMem, bmMem);

    SetViewportOrgEx(dcMem, -ps.rcPaint.left, -ps.rcPaint.top, NULL);
    /* Erase the background */
    FillRect(dcMem, &ps.rcPaint, GetSysColorBrush(FaceColor));

    if(Globals.bAnalog)
	    AnalogClock(dcMem);
    else
	    DigitalClock(dcMem);

    /* Blit the changes to the screen */
    BitBlt(dc, 
	   ps.rcPaint.left, ps.rcPaint.top,
	   ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
           dcMem,
	   ps.rcPaint.left, ps.rcPaint.top,
           SRCCOPY);

    SelectObject(dcMem, bmOld);
    DeleteObject(bmMem);
    DeleteDC(dcMem);
    
    EndPaint(Globals.hMainWnd, &ps);
}

void InitPallet(void)
{
    Palette = &LightPalette;

    FaceColor       = COLOR_WINDOW;
    HandColor       = GetSysColor(COLOR_WINDOWTEXT);
    TickColor       = GetSysColor(COLOR_WINDOWTEXT);
    ShadowColor     = GetSysColor(COLOR_3DSHADOW);
    BackgroundColor = GetSysColor(COLOR_WINDOW);

    Palette = &DarkPalette;

    FaceColor       = COLOR_GRAYTEXT;
    HandColor       = GetSysColor(COLOR_3DHIGHLIGHT);
    TickColor       = GetSysColor(COLOR_3DHIGHLIGHT);
    ShadowColor     = GetSysColor(COLOR_3DSHADOW);
    BackgroundColor = GetSysColor(COLOR_GRAYTEXT);

    ApplyColor();
}

void ApplyColor(void)
{
    Palette = Globals.bDarkColor ? &DarkPalette : &LightPalette;
}
