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
};

extern IRocketUI *g_pRocketUI; // defined by the client

inline IRocketUI *RocketUI() { return g_pRocketUI; }

#endif
