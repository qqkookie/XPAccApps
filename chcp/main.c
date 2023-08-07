/*
 * Copyright 2019 Erich E. Hoover
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
#include "resource.h"

#include "../debug.h"

#define MSG_MAX     1024

#ifdef _DEBUG
#define DBG_PRINTF  _xpa_dbg_printf
#else
#define DBG_PRINTF(...)
#endif

static  struct { TCHAR *name; int codepage; }
    CP_table[] = {
    { L"utf-8",         65001 },
    { L"windows-1252",  1252 },
    { L"oem-us",        437 },
    { L"dos-ibm",       850 },
    { L"euc-kr",        949 },
    { L"korea",	        949 },
    { L"shift-jis",     932 },
    { L"japan",		    932 },
    { L"big5",          950 },
    { L"taiwan",	    950 },
    { L"gbk",		    936 },
    { L"china",		    936 },

    { L"iso-8859-1",    28591 },
    { L"iso-8859-2",    28592 },
    { L"iso-8859-3",    28593 },
    { L"iso-8859-4",    28594 },
    { L"iso-8859-5",    28595 },
    { L"iso-8859-6",    28596 },
    { L"iso-8859-7",    28597 },
    { L"iso-8859-8",    28598 },
    { L"iso-8859-9",    28598 },
    { L"iso-8859-13",   28603 },
    { L"iso-8859-15",   28605 },
};

static HANDLE console_handle = NULL;

static void output_writeconsole(const WCHAR *str, DWORD wlen)
{
    DWORD count, len;
    char  msg[MSG_MAX*2];
    if (WriteConsoleW(console_handle, str, wlen, &count, NULL))
        return;
    /*
     * On Windows WriteConsoleW() fails if the output is redirected.
     * So fall back to WriteFile(), assuming the console encoding is
     * still the right* one in that case.
     */
    len = WideCharToMultiByte(GetOEMCP(), 0, str, wlen, msg, _countof(msg), NULL, NULL);
    if (len)
        WriteFile(console_handle, msg, len, &count, FALSE);
}

static void output_message(int id, ...)
{
    va_list va_args;
    DWORD len;
    WCHAR msg[MSG_MAX] = {0}, fmt[MSG_MAX] = {0};

    va_start(va_args, id);

    if ( id == 0 )
        wcscpy(fmt, va_arg(va_args, WCHAR *));
    else if (!LoadStringW(NULL, id, fmt, _countof(fmt)))
    {
       DBG_PRINTF(L"LoadString failed: err=0x%x, id=%d\n", GetLastError(), id);
       return;
    }

    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, fmt, 0, 0, msg, _countof(msg), &va_args);

    if ( !len && GetLastError() != ERROR_NO_WORK_DONE)
    {
        DBG_PRINTF(L"Could not format string: err=0x%x, id=%d, fmt=%s\n", GetLastError(), id, fmt);
        return;
    }
    output_writeconsole(msg, len);
    va_end(va_args);
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    console_handle = GetStdHandle(STD_ERROR_HANDLE);

            output_message(STRING_CHCP_HELP);
        for ( int ii = 0; ii < _countof(CP_table) ; ii++)
            output_message( 0, L"%1\n", CP_table[ii].name); 
        return 0;

    if (argc == 1)
    {
        output_message(STRING_ACTIVE_CODE_PAGE, GetConsoleCP());
        return 0;
    }
    else if ( !lstrcmpW(argv[1], L"/?"))
    {
        output_message(STRING_CHCP_HELP);
        for ( int ii = 0; ii < _countof(CP_table) ; ii++)
            output_message( 0, L"%1\n", CP_table[ii].name); 
        return 0;
    }
    else if ( argc != 2 )
    {
        output_message(STRING_CHCP_HELP);
        DBG_PRINTF(L"unexpected arguments:");
        for (int ii = 0; ii < argc; ii++)
            DBG_PRINTF(L" %s", argv[ii]);
        DBG_PRINTF(L"\n");
        return 1;
    }

    int codepage = _wtoi(argv[1]);
    if (codepage == 0)
    {
        for ( int ii = 0; ii < _countof(CP_table) ; ii++)
        {
            if ( !wcsicmp(argv[1], CP_table[ii].name))
            {
                codepage = CP_table[ii].codepage;
                goto found;
            }
        }
        output_message(STRING_ERR_INVALID_PARAM, argv[1]);    
        return 1;
    }
found:;

    int oldcp = GetConsoleCP();

    if ( !SetConsoleCP(codepage) || ! SetConsoleOutputCP(codepage))
    {
        output_message(STRING_ERR_INVALID_CODE_PAGE);
        SetConsoleCP(oldcp);
        return 1;
    }
    output_message(STRING_ACTIVE_CODE_PAGE, codepage);

    return 0;
}
