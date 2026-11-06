// RocketUI (RmlUi) interface for CS:SO. Ported from Kisak-Strike (public/rocketui/rocketui.h).
// Differences: no IShaderDeviceDependentObject plumbing (this engine has none; screen size is polled).
#if !defined( __ROCKETUI_H__ )
#define __ROCKETUI_H__

#include "appframework/IAppSystem.h"
#include "inputsystem/InputEnums.h"

#define ROCKETUI_INTERFACE_VERSION "RocketUI001"

namespace Rml
{
	class Element;
	class ElementDocument;
	class Context;
	class EventListener;
}

enum RocketDesinationContext_t
{
	ROCKET_CONTEXT_MENU,
	ROCKET_CONTEXT_HUD,
	ROCKET_CONTEXT_CURRENT, // whichever context is active.
};

typedef void ( *LoadDocumentFn )( void );
typedef void ( *UnloadDocumentFn )( void );
typedef void ( *TogglePauseMenuFn )( void );


// ---------------------------------------------------------------------------------------------
// HUD data. The client DLL is built with the old toolchain (no C++14 library, so RmlUi headers can't
// be included there). The client only fills these plain structs from the CS:SO game data and the
// module (which owns every RmlUi document) draws them. No Rml types may appear below this line.
// ---------------------------------------------------------------------------------------------
#define ROCKET_HUD_NAME_LEN 64

enum RocketHudTeam_t
{
	ROCKET_TEAM_UNASSIGNED = 0,
	ROCKET_TEAM_SPECTATOR = 1,
	ROCKET_TEAM_T = 2,
	ROCKET_TEAM_CT = 3,
};

// Infobar (health, armor, ammo, money) + round timer. Filled every frame by the client.
struct RocketHudStatus_t
{
	bool showInfobar;       // local (or observed) player is alive
	int health;
	int armor;
	bool helmet;
	bool defuser;
	bool hasC4;
	bool inBuyZone;
	int clip;               // -1: this weapon shows no ammo (knife, ...)
	int reserve;
	char weapon[32];        // weapon alias without "weapon_"
	int money;

	bool showTimer;
	float roundTimeLeft;    // seconds, <0: unknown
	bool freezePeriod;
	bool bombPlanted;
	int scoreCT;
	int scoreT;
};

struct RocketHudKill_t
{
	char attacker[ROCKET_HUD_NAME_LEN];
	char victim[ROCKET_HUD_NAME_LEN];
	char assister[ROCKET_HUD_NAME_LEN];
	char weapon[32];        // event weapon name ("ak47", "knife", ...)
	int attackerTeam;       // RocketHudTeam_t
	int victimTeam;
	bool headshot;
	bool wallbang;
	bool noscope;
	bool blind;
	bool suicide;           // world / self kill, no attacker shown
	bool localAttacker;     // highlight: the local player got the kill
	bool localVictim;       // highlight: the local player died
};

class IRocketUI : public IAppSystem
{
public:
	// Updates time + the current context. Call once per frame.
	virtual void RunFrame( float time ) = 0;
	// Reload documents from disk
	virtual bool ReloadDocuments() = 0;

	// Feed input into the UI. Returns true if the game must not see the event.
	virtual bool HandleInputEvent( const InputEvent_t &event ) = 0;
	// Text typed by the user (UTF-32 code point), only used while input is consumed.
	virtual void HandleTextInput( int codepoint ) = 0;

	// Start/stop consuming input, tell us why so input leaks can be debugged
	virtual void DenyInputToGame( bool value, const char *why = "Unknown" ) = 0;
	virtual bool IsConsumingInput( void ) = 0;
	virtual void EnableCursor( bool state ) = 0;

	// The Load/Unload functions are for hot-reloading (rocket_reload).
	virtual Rml::ElementDocument *LoadDocumentFile( RocketDesinationContext_t ctx, const char *filepath,
			LoadDocumentFn loadDocumentFunc = nullptr, UnloadDocumentFn unloadDocumentFunc = nullptr ) = 0;

	virtual void RenderHUDFrame() = 0;
	virtual void RenderMenuFrame() = 0;

	virtual Rml::Context* AccessHudContext() = 0;
	virtual Rml::Context* AccessMenuContext() = 0;

	// The pause menu registers itself here so ESC can toggle it
	virtual void RegisterPauseMenu( TogglePauseMenuFn showPauseMenuFunc ) = 0;

	// Options panel (module owned, convar controls). Opened from the main menu and from the client's pause menu.
	virtual void ShowOptionsPanel( bool show ) = 0;
	// Short message that fades out after \a timeout seconds.
	virtual void ShowPopup( const char *text, float timeout ) = 0;

	// ---- HUD (step 1.4b). All of these are cheap no-ops while rocket_enable is 0. ----
	// Once per frame from the client, before RenderHUDFrame().
	virtual void HudSetStatus( const RocketHudStatus_t &status ) = 0;
	virtual void HudAddKill( const RocketHudKill_t &kill ) = 0;
	// Level change / disconnect: drop killfeed, chat history, ...
	virtual void HudReset() = 0;
};

extern IRocketUI *g_pRocketUI; // defined by the client

inline IRocketUI *RocketUI() { return g_pRocketUI; }

#endif
