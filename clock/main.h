/*
 *  Clock (winclock.h)
 *
 *  Copyright 1998 by Marcel Baur <mbaur@g26.ethz.ch>
 *  This file is essentially rolex.c by Jim Peterson.
 *  Please see my winclock.c and/or his rolex.c for references.
 *
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

#include <Windows.h>

#include "clock_res.h"

#define MAX_STRING_LEN      255
#define DEFAULTICON OIC_WINLOGO

typedef struct
{
  HWND    hMainWnd;
  HANDLE  hInstance;
//  HMENU   hMainMenu;
  HFONT   hFont;
  LOGFONT logfont;
  HRGN    hCircle;

  int     WinW;
  int     WinH;

  BOOL    bAnalog;
  BOOL    bAlwaysOnTop;
  BOOL    bNoTitleBar;
  BOOL    bSeconds;
//  BOOL    bDate;

  BOOL    b24Hours;
  BOOL    bDarkColor;


} CLK_GLOBALS;

extern CLK_GLOBALS Globals;

// void AnalogClock(HDC dc, int X, int Y, BOOL bSeconds, BOOL border);
// void DigitalClock(HDC dc, int X, int Y, BOOL bSeconds, HFONT font);

void DrawClock(void);
void SetAnalogRegion(void);
void ResizeFont(void);

void InitPallet(void);
void ApplyColor(void);
