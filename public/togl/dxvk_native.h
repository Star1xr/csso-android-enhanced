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
#include <d3d9.h>
#undef ULONG
#undef HANDLE
#undef HMODULE

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
