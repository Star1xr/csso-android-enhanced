#ifndef CSSO_ROCKETUIIMPL_H
#define CSSO_ROCKETUIIMPL_H

// RmlUi first: it declares a function named Assert, which tier0/dbg.h turns into a macro.
#include <RmlUi/Core.h>

#include "rocketui/rocketui.h"
#include "tier3/tier3.h"
#include "cdll_int.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "tier1/utlpair.h"


class RocketUIImpl : public CTier3AppSystem<IRocketUI>
{
	typedef CTier3AppSystem<IRocketUI> BaseClass;

public:
	static RocketUIImpl m_Instance;
	RocketUIImpl();

	// IAppSystem
	bool Connect( CreateInterfaceFn factory ) override;
	void Disconnect( void ) override;
	void *QueryInterface( const char *pInterfaceName ) override;
	InitReturnVal_t Init( void ) override;
	void Shutdown( void ) override;

	// IRocketUI
	void RunFrame( float time ) override;
	bool ReloadDocuments() override;
	bool HandleInputEvent( const InputEvent_t &event ) override;
	void HandleTextInput( int codepoint ) override;
	void DenyInputToGame( bool value, const char *why ) override;
	bool IsConsumingInput( void ) override;
	void EnableCursor( bool state ) override;
	Rml::ElementDocument *LoadDocumentFile( RocketDesinationContext_t ctx, const char *filepath,
											LoadDocumentFn loadDocumentFunc = nullptr, UnloadDocumentFn unloadDocumentFunc = nullptr ) override;
	void RenderHUDFrame() override;
	void RenderMenuFrame() override;
	Rml::Context *AccessHudContext() override { return m_ctxHud; }
	Rml::Context *AccessMenuContext() override { return m_ctxMenu; }
	void RegisterPauseMenu( TogglePauseMenuFn fn ) override { m_togglePauseMenuFunc = fn; }
	void ShowOptionsPanel( bool show ) override;
	void ShowPopup( const char *text, float timeout ) override;

	float GetTime() const { return m_fTime; }

private:
	bool LoadFonts();
	bool LoadFont( const char *filepath, const char *family );
	void UpdateScreenSize();
	void RenderContext( Rml::Context *ctx );

	IVEngineClient *m_pEngine;

	CUtlVector<unsigned char *> m_fontAllocs; // FreeType keeps using these until Shutdown
	CUtlVector<CUtlString> m_inputConsumers;
	float m_fTime;
	int m_width, m_height;
	int m_mouseX, m_mouseY;
	bool m_bCursorVisible;
	bool m_bRmlInitialised;

	Rml::Context *m_ctxMenu;
	Rml::Context *m_ctxHud;
	Rml::Context *m_ctxCurrent;

	TogglePauseMenuFn m_togglePauseMenuFunc;

	struct ReloadEntry_t
	{
		RocketDesinationContext_t ctx;
		LoadDocumentFn load;
		UnloadDocumentFn unload;
	};
	CUtlVector<ReloadEntry_t> m_documentReloadFuncs;
	int m_numInputConsumers;
};

#endif
