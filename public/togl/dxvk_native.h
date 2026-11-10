//========= DXVK native (D3D9 -> Vulkan) render mechanism ====================//
//
// Selected by rendermechanism.h when the engine is built with --dxvk
// (DXVK_NATIVE defined, DX_TO_GL_ABSTRACTION not defined).
//
// shaderapidx9 is compiled against the real D3D9 interfaces, which are
// implemented by libdxvk_d3d9.so (https://github.com/Star1xr/dxvk-native-android).
// The headers come from <dxvk-dir>/include/native/{directx,windows}, see
// the --dxvk-dir option in the top level wscript.
//
//==========================================================================//
#ifndef TOGL_DXVK_NATIVE_H
#define TOGL_DXVK_NATIVE_H

#undef PROTECTED_THINGS_ENABLE

#include "tier0/basetypes.h"
#include "tier0/platform.h"

// DXVK's windows_base.h typedefs a few Win32 types differently from the engine's own
// (ULONG is 'unsigned long', HANDLE/HMODULE are integers in tier0). Rename the DXVK ones while its
// headers are parsed; the sizes match, and the engine's definitions stay untouched afterwards.
#define ULONG   DXVK_ULONG
#define HANDLE  DXVK_HANDLE
#define HMODULE DXVK_HMODULE
#define LONG    DXVK_LONG
// protected_things.h poisons libc/Win32 names by macro; DXVK/libstdc++ headers use them
#pragma push_macro("CreateThread")
#undef CreateThread
#pragma push_macro("EnterCriticalSection")
#undef EnterCriticalSection
#pragma push_macro("GetAsyncKeyState")
#undef GetAsyncKeyState
#pragma push_macro("GetCommandLine")
#undef GetCommandLine
#pragma push_macro("GetCursorPos")
#undef GetCursorPos
#pragma push_macro("GetKeyState")
#undef GetKeyState
#pragma push_macro("GetNumberOfConsoleInputEvents")
#undef GetNumberOfConsoleInputEvents
#pragma push_macro("GetTickCount")
#undef GetTickCount
#pragma push_macro("Q_strlen")
#undef Q_strlen
#pragma push_macro("ReadConsoleInput")
#undef ReadConsoleInput
#pragma push_macro("RegCloseKey")
#undef RegCloseKey
#pragma push_macro("RegCreateKey")
#undef RegCreateKey
#pragma push_macro("RegCreateKeyEx")
#undef RegCreateKeyEx
#pragma push_macro("RegOpenKey")
#undef RegOpenKey
#pragma push_macro("RegOpenKeyEx")
#undef RegOpenKeyEx
#pragma push_macro("RegQueryValue")
#undef RegQueryValue
#pragma push_macro("RegQueryValueEx")
#undef RegQueryValueEx
#pragma push_macro("RegSetValue")
#undef RegSetValue
#pragma push_macro("RegSetValueEx")
#undef RegSetValueEx
#pragma push_macro("ScreenToClient")
#undef ScreenToClient
#pragma push_macro("WaitForSingleObject")
#undef WaitForSingleObject
#pragma push_macro("_snprintf")
#undef _snprintf
#pragma push_macro("_snwprintf")
#undef _snwprintf
#pragma push_macro("_vsnprintf")
#undef _vsnprintf
#pragma push_macro("_vsnwprintf")
#undef _vsnwprintf
#pragma push_macro("printf")
#undef printf
#pragma push_macro("sprintf")
#undef sprintf
#pragma push_macro("strcat")
#undef strcat
#pragma push_macro("strcmp")
#undef strcmp
#pragma push_macro("strlen")
#undef strlen
#pragma push_macro("strncat")
#undef strncat
#pragma push_macro("strncpy")
#undef strncpy
#pragma push_macro("swprintf")
#undef swprintf
#pragma push_macro("timeGetTime")
#undef timeGetTime
#pragma push_macro("vsprintf")
#undef vsprintf
#pragma push_macro("vswprintf")
#undef vswprintf
#pragma push_macro("wcscat")
#undef wcscat
#pragma push_macro("wcscmp")
#undef wcscmp
#pragma push_macro("wcslen")
#undef wcslen
#pragma push_macro("wcsncat")
#undef wcsncat
#pragma push_macro("wcsncpy")
#undef wcsncpy
#pragma push_macro("wprintf")
#undef wprintf
// <string.h> may already have been parsed with 'strncpy' renamed by a module define; <cstring> then
// does 'using ::strncpy', so make sure the real libc symbol is declared.
extern "C" char *strncpy( char *, const char *, size_t );
#include <d3d9.h>
#pragma pop_macro("CreateThread")
#pragma pop_macro("EnterCriticalSection")
#pragma pop_macro("GetAsyncKeyState")
#pragma pop_macro("GetCommandLine")
#pragma pop_macro("GetCursorPos")
#pragma pop_macro("GetKeyState")
#pragma pop_macro("GetNumberOfConsoleInputEvents")
#pragma pop_macro("GetTickCount")
#pragma pop_macro("Q_strlen")
#pragma pop_macro("ReadConsoleInput")
#pragma pop_macro("RegCloseKey")
#pragma pop_macro("RegCreateKey")
#pragma pop_macro("RegCreateKeyEx")
#pragma pop_macro("RegOpenKey")
#pragma pop_macro("RegOpenKeyEx")
#pragma pop_macro("RegQueryValue")
#pragma pop_macro("RegQueryValueEx")
#pragma pop_macro("RegSetValue")
#pragma pop_macro("RegSetValueEx")
#pragma pop_macro("ScreenToClient")
#pragma pop_macro("WaitForSingleObject")
#pragma pop_macro("_snprintf")
#pragma pop_macro("_snwprintf")
#pragma pop_macro("_vsnprintf")
#pragma pop_macro("_vsnwprintf")
#pragma pop_macro("printf")
#pragma pop_macro("sprintf")
#pragma pop_macro("strcat")
#pragma pop_macro("strcmp")
#pragma pop_macro("strlen")
#pragma pop_macro("strncat")
#pragma pop_macro("strncpy")
#pragma pop_macro("swprintf")
#pragma pop_macro("timeGetTime")
#pragma pop_macro("vsprintf")
#pragma pop_macro("vswprintf")
#pragma pop_macro("wcscat")
#pragma pop_macro("wcscmp")
#pragma pop_macro("wcslen")
#pragma pop_macro("wcsncat")
#pragma pop_macro("wcsncpy")
#pragma pop_macro("wprintf")
#undef ULONG
#undef HANDLE
#undef HMODULE
#undef LONG

typedef DWORD *LPDWORD;

// Formats the engine uses that are not enumerators in d3d9types.h (vendor FOURCCs, all supported by DXVK)
#define D3DFMT_R5G6R5		D3DFMT_R5G6B5
#define D3DFMT_NV_INTZ		((D3DFORMAT)0x5a544e49)	// 'INTZ'
#define D3DFMT_NV_RAWZ		((D3DFORMAT)0x5a574152)	// 'RAWZ'
#define D3DFMT_NV_NULL		((D3DFORMAT)0x4c4c554e)	// 'NULL'
#define D3DFMT_ATI_D16		((D3DFORMAT)0x36314644)	// 'DF16'
#define D3DFMT_ATI_D24S8	((D3DFORMAT)0x34324644)	// 'DF24'
#define D3DFMT_ATI_2N		((D3DFORMAT)0x32495441)	// 'ATI2'
#define D3DFMT_ATI_1N		((D3DFORMAT)0x31495441)	// 'ATI1'

// The window handle handed to D3D9 is an SDL_Window* created with SDL_WINDOW_VULKAN.
typedef void *VD3DHWND;

// GL debug print helpers used by shaderapidx9, no-ops without GL.
#define	GLMPRINTF(args)
#define	GLMPRINTSTR(args)
#define	GLMPRINTTEXT(args)

// Size of the Vulkan drawable of the SDL window (implemented in shaderapidx9/winutils.cpp)
void toglGetClientRect( VD3DHWND hWnd, RECT *destRect );

// D3DX is not part of DXVK, see dxvk_d3dx.h
#include "togl/dxvk_d3dx.h"

#endif // TOGL_DXVK_NATIVE_H
