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
// (ULONG is 'unsigned long' and HANDLE an integer in tier0). Rename the DXVK ones while its
// headers are parsed; the sizes match, and the engine's definitions stay untouched afterwards.
#define ULONG  DXVK_ULONG
#define HANDLE DXVK_HANDLE
#include <d3d9.h>
#undef ULONG
#undef HANDLE

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
