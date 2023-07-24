/*
 * Management of the debugging channels
 *
 * Copyright 2000 Alexandre Julliard
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static const char * const debug_classes[] = { "fixme", "err", "warn", "trace" };

static int rosfmt_default_dbg_vlog( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                                    const char *file, const char *func, const int line, const char *format, va_list args );

/* varargs wrapper for funcs.dbg_vprintf */
int wine_dbg_printf( const char *format, ... )
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = vprintf( format, valist );
    va_end(valist);
    return ret;
}

/* printf with temp buffer allocation */
const char *wine_dbg_sprintf( const char *format, ... )
{
    static const int max_size = 200;
    char *ret;
    int len;
    va_list valist;

    va_start(valist, format);
    //ret = funcs.get_temp_buffer( max_size );
    ret = malloc( max_size );
    len = vsnprintf( ret, max_size, format, valist );
    if (len == -1 || len >= max_size) ret[max_size-1] = 0;
    // else funcs.release_temp_buffer( ret, len + 1 );
    else free(ret);
    va_end(valist);
    return ret;
}

/* ReactOS compliant debug format wrapper for funcs.dbg_vlog */
int ros_dbg_log( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                  const char *file, const char *func, const int line, const char *format, ... )
{
    int ret;
    va_list valist;

    // if (!(__wine_dbg_get_channel_flags( channel ) & (1 << cls))) return -1;

    va_start(valist, format);
    ret = rosfmt_default_dbg_vlog( cls, channel, file, func, line, format, valist );
    va_end(valist);
    return ret;
}

/* ReactOS format (default) */
static int rosfmt_default_dbg_vlog( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                                    const char *file, const char *func, const int line, const char *format, va_list args )
{
    int ret = 0;

    if (cls < sizeof(debug_classes)/sizeof(debug_classes[0]))
        ret += wine_dbg_printf( "%s:", debug_classes[cls] );

    if (file && line)
        ret += wine_dbg_printf( "(%s:%d) ", file, line );
    else
//        ret += wine_dbg_printf( "%s:%s: ", channel->name, func );
        ret += wine_dbg_printf( "%s: ", func );

    if (format)
        ret += vprintf( format, args );
    return ret;
}

