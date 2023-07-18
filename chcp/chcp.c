/*
 * PROJECT:     ReactOS Change CodePage Command
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Displays or changes the active console input and output code pages.
 * COPYRIGHT:   Copyright 1999 Eric Kohl
 *              Copyright 2017-2021 Hermes Belusca-Maito
 */
/*
 * CHCP.C - chcp internal command.
 *
 * 23-Dec-1998 (Eric Kohl)
 *     Started.
 *
 * 02-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *     Remove all hardcoded strings in En.rc
 */

#include <stdio.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <wincon.h>

#include <conutils.h>

#include "resource.h"

typedef   struct { TCHAR *name; int codepage; } cp_ent;

static cp_ent CP_table[] = {
    { L"utf-8",         65001 },
    { L"windows-1252",  1252 },
    { L"oem-us",        437 },
    { L"dos-ibm",       850 },
    { L"euc-kr",        949 },
    { L"korea",		949 },
    { L"shift-jis",     932 },
    { L"japan",		932 },
    { L"big5",          950 },
    { L"taiwan",	950 },
    { L"gbk",		936 },
    { L"china",		936 },

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

// INT CommandChcp(LPTSTR cmd, LPTSTR param)
int wmain(int argc, WCHAR* argv[])
{
    UINT uOldCodePage, uNewCodePage;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Print help */
    if (argc > 1 && wcscmp(argv[1], L"/?") == 0)
    {
        ConResPuts(StdOut, STRING_CHCP_HELP);
        return 0;
    }

    if (argc == 1)
    {
        /* Display the active code page number */
        ConResPrintf(StdOut, STRING_CHCP_ERROR1, GetConsoleOutputCP());
        return 0;
    }

    if (argc > 2)
    {
        /* Too many parameters */
        ConResPrintf(StdErr, STRING_ERROR_INVALID_PARAM_FORMAT, argv[2]);
        return 1;
    }

    uNewCodePage = (UINT)_wtoi(argv[1]);

    if (uNewCodePage == 0)
    {
        for ( int ii = 0; ii < sizeof(CP_table)/sizeof(cp_ent) ; ii++) {
            if ( !wcsicmp(argv[1], CP_table[ii].name))
            {
                uNewCodePage = CP_table[ii].codepage;
                break;
            }
        }
        if (uNewCodePage == 0)
        {
            ConResPrintf(StdErr, STRING_ERROR_INVALID_PARAM_FORMAT, argv[1]);
            return 1;
        }
    }

/**
 ** IMPORTANT NOTE: This code must be kept synchronized with MODE.COM!SetConsoleCPState()
 **/

    /*
     * Save the original console code page to be restored
     * in case SetConsoleCP() or SetConsoleOutputCP() fails.
     */
    uOldCodePage = GetConsoleCP();

    /*
     * Try changing the console input and output code pages.
     * If it succeeds, refresh the local code page information.
     */
    if (SetConsoleCP(uNewCodePage))
    {
        if (SetConsoleOutputCP(uNewCodePage))
        {
            /* Success, reset the current thread UI language
             * and update the streams cached code page. */
            ConSetThreadUILanguage(0);
            ConStdStreamsSetCacheCodePage(uNewCodePage, uNewCodePage);

            /* Display the active code page number */
            ConResPrintf(StdOut, STRING_CHCP_ERROR1, GetConsoleOutputCP());
            return 0;
        }
        else
        {
            /* Failure, restore the original console code page */
            SetConsoleCP(uOldCodePage);
        }
    }

    /* An error happened, display an error and bail out */
    ConResPuts(StdErr, STRING_CHCP_ERROR4);
    return 1;
}
