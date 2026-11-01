#include "csso_ui_config.h"

#if defined( USE_CSSOROCKETUI )

#include "cbase.h"
#include "GameUI_Interface.h"

// memdbgon must be the last include file in a .cpp file.
#include "tier0/memdbgon.h"

// The RocketUI module lives outside the client DLL. Route its Loadout button
// through a client command so GameUI can open the existing CS:SO loadout page.
CON_COMMAND( rocketui_open_loadout, "Open the CS:SO loadout page from RocketUI" )
{
	GameUI().SendMainMenuCommand( "OpenLoadoutDialog" );
}

#endif // USE_CSSOROCKETUI
