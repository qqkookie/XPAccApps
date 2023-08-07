/*
 * Wine debugging interface
 *
 * Copyright 1999 Patrik Stridvall
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

#ifndef __XPA_DEBUG_H
#define __XPA_DEBUG_H

#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

static TCHAR *  _xpa_dbg_sprintf( const TCHAR *format, ... );
static inline TCHAR * _xpa_dbg_sprintf( const TCHAR *format, ... )
{
    int ret;
    static TCHAR buffer[1024];
    va_list args;

    va_start( args, format );
    ret = _vstprintf( buffer, sizeof(buffer)/sizeof(buffer[0]), (TCHAR *) format, args );
    va_end( args );
    buffer [ret] = '\0';
    return buffer;
}

static int _xpa_dbg_printf( const TCHAR *format, ... );
static inline int  _xpa_dbg_printf( const TCHAR *format, ... )
{
    int ret;
    va_list args;

    va_start( args, format );
    ret = _vftprintf( stderr, format, args );
    va_end( args );
    return ret;
}

static int _xpa_dbg_vprintf( const TCHAR *format, va_list args );
static inline int _xpa_dbg_vprintf( const TCHAR *format, va_list args )
{
    TCHAR buffer[1024];

    _vsntprintf( buffer, sizeof(buffer)/sizeof(buffer[0]), format, args );
    return _fputts( buffer, stderr );
}

#define _XPA_TRACE       _xpa_dbg_printf
#define _XPA_DEBUG       _xpa_dbg_printf

#define WINE_TRACE(a,...)      _XPA_TRACE(TEXT(a), __VA_ARGS__)
#define WINE_FIXME(a, ...)     _XPA_DEBUG(TEXT(a), __VA_ARGS__)

#ifndef GUID_DEFINED
#include <guiddef.h>
#include <windef.h>
#endif

#define wine_dbgstr_guid _xpa_dbgstr_guid

static inline const TCHAR *_xpa_dbgstr_guid( const GUID *id )
{
    if (!id) return _T("(null)");
    if (!((ULONG_PTR)id >> 16)) return _xpa_dbg_sprintf( _T("<guid-0x%04hx>"), (WORD)(ULONG_PTR)id );
    return _xpa_dbg_sprintf( _T("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                             (unsigned int)id->Data1, id->Data2, id->Data3,
                             id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                             id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
}

#ifdef __cplusplus
}
#endif

#endif  /* __XPA_DEBUG_H */
