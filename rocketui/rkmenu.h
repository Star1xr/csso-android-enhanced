// CS:SO RocketUI menu documents that live in the rocketui module (the engine loads the module before the client DLL exists):
// main menu, options panel, popups. HUD documents (chat, killfeed, ...) are owned by the client (step 1.4b).
#ifndef CSSO_ROCKETUI_RKMENU_H
#define CSSO_ROCKETUI_RKMENU_H

class IVEngineClient;

namespace RocketMenu
{
	// Loads menu.rml into the menu context. Call after the contexts, fonts and custom elements exist.
	bool Init( IVEngineClient *engine );
	// Contexts are destroyed by Rml::Shutdown, this only forgets our pointers.
	void Shutdown();
	// Once per frame: main menu is visible while not in a game, deferred document closing, popup lifetime.
	void Frame( bool inGame, float time );

	void ShowOptions( bool show );
	bool OptionsIsOpen();
	void ShowPopup( const char *text, float timeout );
}

#endif
