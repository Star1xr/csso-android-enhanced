// CS:SO RocketUI HUD bridge (step 1.4b), client side.
// The client DLL is built with the old toolchain, so no RmlUi header may be included here. This code only
// reads CS:SO game data and hands it to the rocketui module through the plain structs in rocketui/rocketui.h.
#ifndef RKHUD_BRIDGE_H
#define RKHUD_BRIDGE_H

#include "csso_ui_config.h"

#if defined( USE_CSSOROCKETUI )

// True when the RocketUI module is loaded, has its HUD context and rocket_enable is 1.
bool RocketHud_IsActive();

// Chat (hud_basechat.cpp / cs_hud_chat.cpp hooks). Both return false when RocketUI isn't drawing the HUD.
// OnChatPrintf gets the raw ChatPrintf text (with CS color control characters) and the sender's player index (0: server).
bool RocketHud_OnChatPrintf( int iPlayerIndex, const char *msg );
// Returns true when RocketUI opened its own typing line, the vgui one must not open.
bool RocketHud_OnChatOpen( bool teamOnly );

// First line of a vgui HUD panel's ShouldDraw(): hides the old panel while RocketUI draws the same thing.
#define ROCKET_HUD_HIDE_VGUI() do { if ( RocketHud_IsActive() ) return false; } while ( 0 )

#else

#define ROCKET_HUD_HIDE_VGUI() do { } while ( 0 )

#endif

#endif // RKHUD_BRIDGE_H
