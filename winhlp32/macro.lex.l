%{ /* -*-C-*- */
/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002,2008 Eric Pouech
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
%}
%option noinput nounput never-interactive 8bit
%x quote
%{
// #include "config.h"
#include <assert.h>
#include <stdarg.h>

#define YY_NO_UNISTD_H
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winhelp.h"

#include "../debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhelp);

struct lex_data {
    LPCSTR   macroptr;
    LPSTR    strptr;
    int      quote_stack[32];
    unsigned quote_stk_idx;
    LPSTR    cache_string[32];
    int      cache_used;
    WINHELP_WINDOW* window;
};
static struct lex_data* lex_data = NULL;

struct lexret  yylval;

#define YY_INPUT(buf,result,max_size)\
  if ((result = *lex_data->macroptr ? 1 : 0)) buf[0] = *lex_data->macroptr++;

%}
%%

[-+]?[0-9]+             yylval.integer = strtol(yytext, NULL, 10);	return INTEGER;
[-+]?0[xX][0-9a-f]+	yylval.integer = strtol(yytext, NULL, 16);	return INTEGER;

[a-zA-Z][_0-9a-zA-Z]*   return MACRO_Lookup(yytext, &yylval);

\`	    |
\"	    |
\'          |
<quote>\`   |
<quote>\"   |
<quote>\'   {
    if (lex_data->quote_stk_idx == 0 ||
        (yytext[0] == '\"' && lex_data->quote_stack[lex_data->quote_stk_idx - 1] != '\"') ||
        (yytext[0] == '`'))
    {
        /* opening a new one */
        if (lex_data->quote_stk_idx == 0)
        {
            assert(lex_data->cache_used < ARRAY_SIZE(lex_data->cache_string));
            lex_data->strptr = lex_data->cache_string[lex_data->cache_used] = HeapAlloc(GetProcessHeap(), 0, strlen(lex_data->macroptr) + 1);
            yylval.string = lex_data->strptr;
            lex_data->cache_used++;
            BEGIN(quote);
        }
        else *lex_data->strptr++ = yytext[0];
        lex_data->quote_stack[lex_data->quote_stk_idx++] = yytext[0];
        assert(lex_data->quote_stk_idx < ARRAY_SIZE(lex_data->quote_stack));
    }
    else
    {
        if (yytext[0] == '`') assert(0);
        /* close the current quote */
        if (--lex_data->quote_stk_idx == 0)
        {
            BEGIN INITIAL;
            *lex_data->strptr++ = '\0';
            return STRING;
        }
        else *lex_data->strptr++ = yytext[0];
    }
}

<quote>.                *lex_data->strptr++ = yytext[0];
<quote>\\.	        *lex_data->strptr++ = yytext[1];
<quote><<EOF>>	        return 0;

" "
.			return yytext[0];
%%

#if 0
/* all code for testing macros */
#include "winhelp.h"
static CHAR szTestMacro[256];

static LRESULT CALLBACK MACRO_TestDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_COMMAND && wParam == IDOK)
    {
        GetDlgItemText(hDlg, 99, szTestMacro, sizeof(szTestMacro));
        EndDialog(hDlg, IDOK);
        return TRUE;
    }
    return FALSE;
}

void macro_test(void)
{
    WNDPROC lpfnDlg = MakeProcInstance(MACRO_TestDialogProc, Globals.hInstance);
    DialogBox(Globals.hInstance, STRING_DIALOG_TEST, Globals.active_win->hMainWnd, (DLGPROC)lpfnDlg);
    FreeProcInstance(lpfnDlg);
    macro = szTestMacro;
}
#endif

/* small helper function for debug messages */
static const char* ts(int t)
{
    static char c[2] = {0,0};

    switch (t)
    {
    case EMPTY: return "EMPTY";
    case VOID_FUNCTION: return "VOID_FUNCTION";
    case BOOL_FUNCTION: return "BOOL_FUNCTION";
    case INTEGER: return "INTEGER";
    case STRING: return "STRING";
    case IDENTIFIER: return "IDENTIFIER";
    default: c[0] = (char)t; return c;
    }
}

static int MACRO_CallBoolFunc(void *fn, const char* args, void** ret);

/******************************************************************
 *		MACRO_CheckArgs
 *
 * checks number of arguments against prototype, and stores arguments on
 * stack pa for later call
 * returns -1 on error, otherwise the number of pushed parameters
 */
static int MACRO_CheckArgs(void* pa[], unsigned max, const char* args)
{
    int t;
    unsigned int len = 0, idx = 0;

    WINE_TRACE("Checking %s\n", debugstr_a(args));

    if (yylex() != '(') {WINE_WARN("missing (\n");return -1;}

    if (*args)
    {
        len = strlen(args);
        for (;;)
        {
            t = yylex();
            WINE_TRACE("Got %s <=> %c\n", debugstr_a(ts(t)), *args);

            switch (*args)
            {
            case 'S': 
                if (t != STRING)
                {WINE_WARN("missing S\n");return -1;}
                pa[idx] = (void*)yylval.string;  
                break;
            case 'U':
            case 'I':
                if (t != INTEGER)
                {WINE_WARN("missing U\n");return -1;}   
                pa[idx] = LongToPtr(yylval.integer);
                break;
            case 'B':
                if (t != BOOL_FUNCTION) 
                {WINE_WARN("missing B\n");return -1;}   
                if (MACRO_CallBoolFunc(yylval.function, yylval.proto, &pa[idx]) == 0)
                    return -1;
                break;
            default: 
                WINE_WARN("unexpected %s while args is %c\n", debugstr_a(ts(t)), *args);
                return -1;
            }
            idx++;
            if (*++args == '\0') break;
            t = yylex();
            if (t == ')') goto CheckArgs_end;
            if (t != ',') {WINE_WARN("missing ,\n");return -1;}
            if (idx >= max) {WINE_FIXME("stack overflow (%d)\n", max);return -1;}
        }
    }
    if (yylex() != ')') {WINE_WARN("missing )\n");return -1;}

CheckArgs_end:
    while (len > idx) pa[--len] = NULL;
    return idx;
}

/******************************************************************
 *		MACRO_CallBoolFunc
 *
 * Invokes boolean function fn, which arguments are defined by args
 * stores bool result into ret
 */
static int MACRO_CallBoolFunc(void *fn, const char* args, void** ret)
{
    void*       pa[2];
    int         idx = MACRO_CheckArgs(pa, ARRAY_SIZE(pa), args);

    if (idx < 0) return 0;
    if (!fn)     return 1;

    WINE_TRACE("calling with %u pmts\n", idx);

    switch (strlen(args))
    {
    case 0:
    {
        BOOL (WINAPI *func)(void) = fn;
        *ret = (void *)(ULONG_PTR)func();
        break;
    }
    case 1:
    {
        BOOL (WINAPI *func)(void *) = fn;
        *ret = (void *)(ULONG_PTR)func( pa[0]);
        break;
    }
    default: WINE_FIXME("NIY\n");
    }

    return 1;
}

/******************************************************************
 *		MACRO_CallVoidFunc
 *
 *
 */
static int MACRO_CallVoidFunc(void *fn, const char* args)
{
    void*       pa[6];
    int         idx = MACRO_CheckArgs(pa, ARRAY_SIZE(pa), args);

    if (idx < 0) return 0;
    if (!fn)     return 1;

    WINE_TRACE("calling %p with %u pmts\n", fn, idx);

    switch (strlen(args))
    {
    case 0:
    {
        void (WINAPI *func)(void) = fn;
        func();
        break;
    }
    case 1:
    {
        void (WINAPI *func)(void*) = fn;
        func( pa[0] );
        break;
    }
    case 2:
    {
        void (WINAPI *func)(void*,void*) = fn;
        func( pa[0], pa[1] );
        break;
    }
    case 3:
    {
        void (WINAPI *func)(void*,void*,void*) = fn;
        func( pa[0], pa[1], pa[2] );
        break;
    }
    case 4:
    {
        void (WINAPI *func)(void*,void*,void*,void*) = fn;
        func( pa[0], pa[1], pa[2], pa[3] );
        break;
    }
    case 5:
    {
        void (WINAPI *func)(void*,void*,void*,void*,void*) = fn;
        func( pa[0], pa[1], pa[2], pa[3], pa[4] );
        break;
    }
    case 6:
    {
        void (WINAPI *func)(void*,void*,void*,void*,void*,void*) = fn;
        func( pa[0], pa[1], pa[2], pa[3], pa[4], pa[5] );
        break;
    }
    default: WINE_FIXME("NIY\n");
    }

    return 1;
}

BOOL MACRO_ExecuteMacro(WINHELP_WINDOW* window, LPCSTR macro)
{
    struct lex_data     curr_lex_data, *prev_lex_data;
    BOOL ret = TRUE;
    int t;

    WINE_TRACE("%s\n", debugstr_a(macro));

    prev_lex_data = lex_data;
    lex_data = &curr_lex_data;

    memset(lex_data, 0, sizeof(*lex_data));
    lex_data->macroptr = macro;
    lex_data->window = WINHELP_GrabWindow(window);

    while ((t = yylex()) != EMPTY)
    {
        switch (t)
        {
        case VOID_FUNCTION:
            WINE_TRACE("got type void func(%s)\n", debugstr_a(yylval.proto));
            MACRO_CallVoidFunc(yylval.function, yylval.proto);
            break;
        case BOOL_FUNCTION:
            WINE_WARN("got type bool func(%s)\n", debugstr_a(yylval.proto));
            break;
        default:
            WINE_WARN("got unexpected type %s\n", debugstr_a(ts(t)));
            YY_FLUSH_BUFFER;
            ret = FALSE;
            goto done;
        }
        switch (t = yylex())
        {
        case EMPTY:     goto done;
        case ';':       break;
        default:        ret = FALSE; YY_FLUSH_BUFFER; goto done;
        }
    }

done:
    for (t = 0; t < lex_data->cache_used; t++)
        HeapFree(GetProcessHeap(), 0, lex_data->cache_string[t]);
    lex_data = prev_lex_data;
    WINHELP_ReleaseWindow(window);

    return ret;
}

WINHELP_WINDOW* MACRO_CurrentWindow(void)
{
    return lex_data ? lex_data->window : Globals.active_win;
}

#ifndef yywrap
int yywrap(void) { return 1; }
#endif
