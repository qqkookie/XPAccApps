#ifndef _MSPAINT_H
#define _MSPAINT_H

#ifdef NDEBUG
    #undef DBG
    #undef _DEBUG
#endif

// Additional include's for porting
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <cguid.h>
#include <objidl.h>
#include <Gdiplus.h>
//----------------

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
// #include <atlimage.h>
#include "atlimage.h"
#include <atlpath.h>
#include <atlstr.h>
#include <atlwin.h>
#include <atltypes.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <stdlib.h>
#include <shellapi.h>
#include <htmlhelp.h>

#define UNREACHABLE /**/
#define WINBOOL     BOOL

#include "atlimagedx.h"
#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

// #include <debug.h>
#include "debug.h"

#include "resource.h"
#include "common.h"
#include "drawing.h"
#include "dib.h"
#include "fullscreen.h"
#include "history.h"
#include "miniature.h"
#include "palette.h"
#include "palettemodel.h"
#include "registry.h"
#include "selectionmodel.h"
#include "sizebox.h"
#include "canvas.h"
#include "textedit.h"
#include "toolbox.h"
#include "toolsettings.h"
#include "toolsmodel.h"
#include "winproc.h"
#include "dialogs.h"
#include "globalvar.h"

#endif /* _MSPAINT_H */
