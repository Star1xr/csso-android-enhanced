#include "rkmenu.h"
#include "rocketuiimpl.h"

#include <RmlUi/Core.h>

#include "tier0/dbg.h"
#include "tier1/convar.h"
#include "tier1/strtools.h"
#include "tier1/utlvector.h"
#include "cdll_int.h"

namespace
{
	IVEngineClient *s_pEngine = NULL;

	Rml::ElementDocument *s_pMainDoc = NULL;
	Rml::ElementDocument *s_pOptionsDoc = NULL;

	bool s_bInGame = false;
	bool s_bMainShown = false;
	bool s_bMainGrab = false;
	bool s_bOptionsGrab = false;
	bool s_bOptionsUnloadPending = false;

	struct Popup_t
	{
		Rml::ElementDocument *doc;
		float endTime;
	};
	CUtlVector<Popup_t> s_popups;
	float s_fTime = 0.0f;

	void Grab( bool &flag, bool want, const char *why )
	{
		if ( flag == want )
			return;
		flag = want;
		RocketUIImpl::m_Instance.DenyInputToGame( want, why );
	}

	// Walks up from the clicked element to the first one with an id.
	Rml::String ClickedId( Rml::Event &event )
	{
		for ( Rml::Element *e = event.GetTargetElement(); e; e = e->GetParentNode() )
		{
			if ( !e->GetId().empty() )
				return e->GetId();
		}
		return Rml::String();
	}

	void Command( const char *cmd )
	{
		if ( s_pEngine )
			s_pEngine->ClientCmd_Unrestricted( cmd );
	}

	class MainMenuListener : public Rml::EventListener
	{
	public:
		void ProcessEvent( Rml::Event &event ) override
		{
			const Rml::String id = ClickedId( event );

			if ( id == "menu_start" )
			{
				Rml::String map = "de_dust2";
				if ( s_pMainDoc )
				{
					if ( Rml::Element *sel = s_pMainDoc->GetElementById( "map_select" ) )
						map = sel->GetAttribute<Rml::String>( "value", map );
				}
				// The value comes from our own .rml, but never put anything but a map name into a command.
				char safe[64];
				int n = 0;
				for ( size_t i = 0; i < map.size() && n < (int)sizeof( safe ) - 1; i++ )
				{
					const char c = map[i];
					if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '_' || c == '-' )
						safe[n++] = c;
				}
				safe[n] = 0;
				if ( n > 0 )
				{
					char cmd[96];
					V_snprintf( cmd, sizeof( cmd ), "map %s", safe );
					Command( cmd );
				}
			}
			else if ( id == "menu_options" )
				RocketMenu::ShowOptions( true );
			else if ( id == "menu_console" )
				Command( "toggleconsole" );
			else if ( id == "menu_quit" )
				Command( "quit" );
		}
	};

	class OptionsListener : public Rml::EventListener
	{
	public:
		void ProcessEvent( Rml::Event &event ) override
		{
			const Rml::String id = ClickedId( event );
			if ( id == "save" )
			{
				Msg( "RocketUI: saving settings\n" );
				Command( "mat_savechanges" );
				Command( "host_writeconfig" );
			}
			else if ( id == "close" )
				RocketMenu::ShowOptions( false );
		}
	};

	MainMenuListener s_mainListener;
	OptionsListener s_optionsListener;

	void UnloadMainMenu();

	void LoadMainMenu()
	{
		if ( s_pMainDoc )
			return;
		s_pMainDoc = RocketUIImpl::m_Instance.LoadDocumentFile( ROCKET_CONTEXT_MENU, "menu.rml", LoadMainMenu, UnloadMainMenu );
		if ( !s_pMainDoc )
		{
			Warning( "RocketUI: couldn't load menu.rml (is csso/rocketui/ installed?)\n" );
			return;
		}
		s_pMainDoc->AddEventListener( Rml::EventId::Click, &s_mainListener );
		s_bMainShown = false; // Frame() decides
		s_pMainDoc->Hide();
	}

	void UnloadMainMenu()
	{
		if ( !s_pMainDoc )
			return;
		s_pMainDoc->RemoveEventListener( Rml::EventId::Click, &s_mainListener );
		s_pMainDoc->Close();
		s_pMainDoc = NULL;
		s_bMainShown = false;
		Grab( s_bMainGrab, false, "MainMenu" );
	}

	void UnloadOptions()
	{
		if ( s_pOptionsDoc )
		{
			s_pOptionsDoc->RemoveEventListener( Rml::EventId::Click, &s_optionsListener );
			s_pOptionsDoc->Close();
			s_pOptionsDoc = NULL;
		}
		s_bOptionsUnloadPending = false;
	}
}

bool RocketMenu::Init( IVEngineClient *engine )
{
	s_pEngine = engine;
	LoadMainMenu();
	return s_pMainDoc != NULL;
}

void RocketMenu::Shutdown()
{
	s_pEngine = NULL;
	s_pMainDoc = s_pOptionsDoc = NULL;
	s_popups.Purge();
	s_bMainShown = s_bMainGrab = s_bOptionsGrab = s_bOptionsUnloadPending = false;
}

void RocketMenu::Frame( bool inGame, float time )
{
	s_fTime = time;
	s_bInGame = inGame;

	// Main menu: only while there is no game running.
	if ( s_pMainDoc )
	{
		const bool wantShown = !inGame;
		if ( wantShown != s_bMainShown )
		{
			s_bMainShown = wantShown;
			if ( wantShown )
				s_pMainDoc->Show();
			else
				s_pMainDoc->Hide();
			Grab( s_bMainGrab, wantShown, "MainMenu" );
		}
	}

	// Closing a document from inside its own click handler is not safe, so it is done here.
	if ( s_bOptionsUnloadPending )
		UnloadOptions();

	for ( int i = s_popups.Count() - 1; i >= 0; i-- )
	{
		if ( time >= s_popups[i].endTime )
		{
			s_popups[i].doc->Close();
			s_popups.Remove( i );
		}
	}
}

void RocketMenu::ShowOptions( bool show )
{
	if ( show )
	{
		if ( s_bOptionsUnloadPending )
			UnloadOptions();
		if ( !s_pOptionsDoc )
		{
			// Loaded on every open so the controls read the current convar values.
			s_pOptionsDoc = RocketUIImpl::m_Instance.LoadDocumentFile( s_bInGame ? ROCKET_CONTEXT_HUD : ROCKET_CONTEXT_MENU, "panel_options.rml" );
			if ( !s_pOptionsDoc )
			{
				Warning( "RocketUI: couldn't load panel_options.rml\n" );
				return;
			}
			s_pOptionsDoc->AddEventListener( Rml::EventId::Click, &s_optionsListener );
		}
		s_pOptionsDoc->Show();
		s_pOptionsDoc->PullToFront();
		Grab( s_bOptionsGrab, true, "OptionsPanel" );
	}
	else
	{
		if ( s_pOptionsDoc )
		{
			s_pOptionsDoc->Hide();
			s_bOptionsUnloadPending = true;
		}
		Grab( s_bOptionsGrab, false, "OptionsPanel" );
	}
}

void RocketMenu::ShowPopup( const char *text, float timeout )
{
	if ( !text || timeout <= 0.0f )
		return;

	Rml::ElementDocument *doc = RocketUIImpl::m_Instance.LoadDocumentFile( s_bInGame ? ROCKET_CONTEXT_HUD : ROCKET_CONTEXT_MENU, "panel_popup.rml" );
	if ( !doc )
		return;

	if ( Rml::Element *e = doc->GetElementById( "text" ) )
		e->SetInnerRML( Rml::StringUtilities::EncodeRml( text ) );
	doc->Show();
	doc->Animate( "opacity", Rml::Property( 0.0f, Rml::Property::NUMBER ), timeout, Rml::Tween( Rml::Tween::Quadratic ) );

	Popup_t p = { doc, s_fTime + timeout + 0.1f };
	s_popups.AddToTail( p );
}

CON_COMMAND( rocket_test_popup, "Create a test popup in RocketUI: rocket_test_popup <text> <seconds>" )
{
	if ( args.ArgC() != 3 )
	{
		Msg( "Usage: rocket_test_popup <text> <seconds>\n" );
		return;
	}
	RocketMenu::ShowPopup( args.Arg( 1 ), (float)atof( args.Arg( 2 ) ) );
}

CON_COMMAND( rocket_options, "Open the RocketUI options panel" )
{
	RocketMenu::ShowOptions( true );
}
