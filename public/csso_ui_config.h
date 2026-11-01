//========= CS:SO Android - UI backend selection ============================//
//
// Exactly one of the following must be defined by the build system (wscript):
//
//   USE_CSSOUI        original CS:SO vgui based UI (default)
//   USE_CSSOROCKETUI  RmlUi ("RocketUI") based UI, ported from Kisak-Strike
//
// Include this header from any translation unit that branches on the UI
// backend, so a bad configuration fails at compile time instead of silently
// building the wrong UI.
//
//   #include "csso_ui_config.h"
//   #if defined( USE_CSSOROCKETUI )
//       ...RocketUI path...
//   #else // USE_CSSOUI
//       ...original UI path...
//   #endif
//
//=============================================================================//

#ifndef CSSO_UI_CONFIG_H
#define CSSO_UI_CONFIG_H
#ifdef _WIN32
#pragma once
#endif

#if defined( USE_CSSOUI ) && defined( USE_CSSOROCKETUI )
#error "USE_CSSOUI and USE_CSSOROCKETUI are mutually exclusive. Configure with or without --rocketui, not both."
#endif

// Dedicated server / tools builds have no client UI at all, don't force a choice there.
#if !defined( DEDICATED ) && !defined( UNITTESTS ) && !defined( USE_CSSOUI ) && !defined( USE_CSSOROCKETUI )
#error "Define either USE_CSSOUI (original UI) or USE_CSSOROCKETUI (RocketUI). waf configure defines one automatically."
#endif

#endif // CSSO_UI_CONFIG_H
