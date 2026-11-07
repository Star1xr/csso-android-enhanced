// CS:SO RocketUI HUD documents (step 1.4b). They live in the module because RmlUi needs C++14, which the client DLL toolchain doesn't have.
// The client fills the plain structs from rocketui.h; this file turns them into RmlUi elements.
#ifndef CSSO_ROCKETUI_RKHUD_H
#define CSSO_ROCKETUI_RKHUD_H

#include "rocketui/rocketui.h"

class IVEngineClient;

namespace RocketHud
{
	// Loads the HUD documents into the hud context. Call after contexts, fonts and custom elements exist.
	bool Init( IVEngineClient *engine );
	// Contexts are destroyed by Rml::Shutdown, this only forgets our pointers.
	void Shutdown();
	// Once per frame: killfeed lifetime.
	void Frame( float time );

	void SetStatus( const RocketHudStatus_t &status );
	void AddKill( const RocketHudKill_t &kill );
	void ChatAdd( const RocketHudChat_t &line );
	void ChatOpen( bool teamOnly );
	bool ChatIsOpen();
	void SetRadar( const RocketHudRadar_t &radar );
	void ScoreboardShow( bool show );
	void ScoreboardSet( const RocketHudScoreboard_t &data );
	void Reset();
}

#endif
