// CS:SO RocketUI in-game menus (step 1.4b stage E): buy menu, team menu, class menu.
// The client fills the plain structs from rocketui.h (it knows the team, the money and scripts/buymenuconfig.txt);
// the module draws the documents and sends buy / rebuy / autobuy / jointeam / spectate / joinclass itself.
#ifndef CSSO_ROCKETUI_RKGAMEMENUS_H
#define CSSO_ROCKETUI_RKGAMEMENUS_H

#include "rocketui/rocketui.h"

class IVEngineClient;

namespace RocketGameMenus
{
	// Loads the menu documents into the hud context. Call after contexts, fonts and custom elements exist.
	void Init( IVEngineClient *engine );
	void Shutdown();
	// Once per frame: deferred rebuilds (never rebuild a document from inside its own click handler).
	void Frame();
	// Level change / disconnect: close everything.
	void Reset();

	void BuyShow( bool show );
	bool BuyIsOpen();
	void BuySet( const RocketBuyMenu_t &menu );
	void TeamShow( bool show, const RocketTeamMenu_t &info );
	void ClassShow( bool show, int team, bool canCancel );

	// True while any of the three is open (the pause menu must stay closed then).
	bool AnyOpen();
}

#endif
