/*
handle16 <-> wow64 handle32
*/

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <errno.h>

#include "wine/winbase16.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "windows/wownt32.h"
#include "excpt.h"
#include "winternl.h"
#include "kernel16_private.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(thunk);
#define HANDLE_RESERVED 32
typedef struct
{
	HANDLE handle32;
	DWORD wndproc;
} HANDLE_DATA;
HANDLE_DATA handle_hwnd[65536];
WORD get_handle16_data(HANDLE h, HANDLE_DATA handles[], HANDLE_DATA **o);
WORD get_handle16(HANDLE h, HANDLE_DATA handles[])
{
	if (h < HANDLE_RESERVED)
	{
		return h;
	}
	HANDLE_DATA *hd;
	int hnd16 = get_handle16_data(h, handles, &hd);
	hd->handle32 = h;
	return hnd16;
}
WORD get_handle16_data(HANDLE h, HANDLE_DATA handles[], HANDLE_DATA **o)
{
	//?
	if (h < HANDLE_RESERVED)
	{
		*o = &handles[(size_t)h];
		return h;
	}
	WORD fhandle = 0;
	for (WORD i = HANDLE_RESERVED; i; i++)
	{
		if (!handles[i].handle32 && !fhandle)
		{
			fhandle = i;
		}
		if (handles[i].handle32 == h)
		{
			*o = &handles[i];
			return i;
		}
	}
	if (!fhandle)
	{
		ERR("Could not allocate a handle.\n");
	}
	*o = &handles[fhandle];
	return fhandle;
}
HANDLE get_handle32_data(WORD h, HANDLE_DATA handles[], HANDLE_DATA **o)
{
	if (h < HANDLE_RESERVED)
	{
		*o = &handles[(size_t)h];
		(*o)->handle32 = h;
		return;
	}
	*o = &handles[h];
}
HANDLE get_handle32(WORD h, HANDLE_DATA handles[])
{
	if (h < HANDLE_RESERVED)
	{
		return h;
	}
	return handles[h].handle32;
}
HANDLE WINAPI K32WOWHandle16HWND(WORD handle);
HANDLE WINAPI K32WOWHandle32HWND(WORD handle);
//handle16 -> wow64 handle32
HANDLE WINAPI K32WOWHandle32HWND(WORD handle)
{
	HANDLE h32 = get_handle32(handle, handle_hwnd);
	TRACE("handle16 0x%04X->handle32 0x%X\n", handle, h32);
	return h32;
}
//handle16 <- wow64 handle32
HANDLE WINAPI K32WOWHandle16HWND(HANDLE handle)
{
	HANDLE h16 = get_handle16(handle, handle_hwnd);
	TRACE("handle32 0x%X->handle16 0x%04X\n", handle, h16);
	return h16;
}
__declspec(dllexport) void SetWndProc16(WORD hWnd16, DWORD WndProc)
{
	HANDLE_DATA *dat;
	if (!get_handle32_data(hWnd16, handle_hwnd, &dat))
	{
		ERR("Invalid Window Handle SetWndProc16(0x%04X);", hWnd16);
		return;
	}
	dat->wndproc = WndProc;
}
__declspec(dllexport) DWORD GetWndProc16(WORD hWnd16)
{
	HANDLE_DATA *dat;
	if (!get_handle32_data(hWnd16, handle_hwnd, &dat))
	{
		ERR("Invalid Window Handle SetWndProc16(0x%04X);", hWnd16);
		return 0;
	}
	return dat->wndproc;
}