#include "rocketuiimpl.h"

#include "rocketsystem.h"
#include "rocketrender.h"
#include "rocketfilesystem.h"
#include "rocketkeys.h"
#include "customelements.h"
#include "rkmenu.h"
#include "rkhud.h"

#include <RmlUi/Core.h>

#include "tier0/dbg.h"
#include "tier0/threadtools.h"
#include "tier1/convar.h"
#include "tier1/interface.h"
#include "filesystem.h"
#include "inputsystem/ButtonCode.h"
#include "inputsystem/AnalogCode.h"
#include <string.h>

RocketUIImpl RocketUIImpl::m_Instance;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( RocketUIImpl, IRocketUI, ROCKETUI_INTERFACE_VERSION, RocketUIImpl::m_Instance )

static ConVar rocket_enable( "rocket_enable", "1", 0, "Enables RocketUI" );
static ConVar rocket_verbose( "rocket_verbose", "0", 0, "Enables more logging" );

CON_COMMAND_F( rocket_reload, "Reloads all RocketUI Documents", FCVAR_NONE )
{
	if ( RocketUIImpl::m_Instance.ReloadDocuments() )
		ConMsg( "[RocketUI] Documents reloaded.\n" );
	else
		ConMsg( "[RocketUI] Error reloading documents!\n" );
}

RocketUIImpl::RocketUIImpl() :
	m_pEngine( NULL ), m_fTime( 0.0f ), m_width( 1280 ), m_height( 720 ),
	m_bCursorVisible( false ), m_bRmlInitialised( false ), m_mouseX( 0 ), m_mouseY( 0 ),
	m_ctxMenu( NULL ), m_ctxHud( NULL ), m_ctxCurrent( NULL ),
	m_togglePauseMenuFunc( NULL ), m_numInputConsumers( 0 )
{
}

bool RocketUIImpl::Connect( CreateInterfaceFn factory )
{
	if ( !factory || !BaseClass::Connect( factory ) )
		return false;

	m_pEngine = (IVEngineClient *)factory( VENGINE_CLIENT_INTERFACE_VERSION, NULL );
	if ( !m_pEngine || !g_pFullFileSystem )
	{
		Warning( "RocketUI: missing expected interface (engine client / filesystem)\n" );
		return false;
	}
	return true;
}

void RocketUIImpl::Disconnect()
{
	m_pEngine = NULL;
	BaseClass::Disconnect();
}

void *RocketUIImpl::QueryInterface( const char *pInterfaceName )
{
	if ( !Q_strncmp( pInterfaceName, ROCKETUI_INTERFACE_VERSION, Q_strlen( ROCKETUI_INTERFACE_VERSION ) + 1 ) )
		return (IRocketUI *)&RocketUIImpl::m_Instance;

	return BaseClass::QueryInterface( pInterfaceName );
}

bool RocketUIImpl::LoadFont( const char *filepath, const char *family )
{
	CUtlBuffer font;
	if ( !g_pFullFileSystem->ReadFile( filepath, "GAME", font ) )
	{
		Warning( "[RocketUI] Failed to read font %s\n", filepath );
		return false;
	}

	const int fontLen = font.TellPut();
	if ( fontLen <= 0 || fontLen >= ( 8 * 1024 * 1024 ) )
	{
		Warning( "[RocketUI] Font %s has a bad size (%d), not loading.\n", filepath, fontLen );
		return false;
	}

	unsigned char *fontBuffer = new unsigned char[fontLen];
	m_fontAllocs.AddToTail( fontBuffer );
	memcpy( fontBuffer, font.Base(), fontLen );

	if ( !Rml::LoadFontFace( fontBuffer, fontLen, family, Rml::Style::FontStyle::Normal, Rml::Style::FontWeight::Normal, false ) )
	{
		Warning( "[RocketUI] Failed to initialize font %s\n", family );
		return false;
	}
	return true;
}

bool RocketUIImpl::LoadFonts()
{
	return LoadFont( "rocketui/fonts/Lato-Black.ttf", "Lato" );
}

InitReturnVal_t RocketUIImpl::Init( void )
{
	InitReturnVal_t nRetVal = BaseClass::Init();
	if ( nRetVal != INIT_OK )
		return nRetVal;

	UpdateScreenSize();
	RocketRender::m_Instance.SetScreenSize( m_width, m_height );

	Rml::SetFileInterface( &RocketFileSystem::m_Instance );
	Rml::SetRenderInterface( &RocketRender::m_Instance );
	Rml::SetSystemInterface( &RocketSystem::m_Instance );

	if ( !Rml::Initialise() )
	{
		Warning( "RocketUI: Initialise() failed!\n" );
		return INIT_FAILED;
	}
	m_bRmlInitialised = true;

	RocketUI_RegisterCustomElements();

	if ( !LoadFonts() )
	{
		Warning( "RocketUI: Failed to load fonts (is csso/rocketui/ installed?).\n" );
		return INIT_FAILED;
	}

	m_ctxMenu = Rml::CreateContext( "menu", Rml::Vector2i( m_width, m_height ) );
	m_ctxHud = Rml::CreateContext( "hud", Rml::Vector2i( m_width, m_height ) );
	if ( !m_ctxMenu || !m_ctxHud )
	{
		Warning( "RocketUI: Failed to create Hud/Menu context\n" );
		return INIT_FAILED;
	}

	m_ctxMenu->SetDensityIndependentPixelRatio( 1.0f );
	m_ctxHud->SetDensityIndependentPixelRatio( 1.0f );

	// The main menu is optional: without csso/rocketui/ the engine keeps the old vgui menu (Init fails above on fonts).
	if ( !RocketMenu::Init( m_pEngine ) )
		Warning( "RocketUI: main menu document missing, menu will be empty\n" );

	if ( !RocketHud::Init( m_pEngine ) )
		Warning( "RocketUI: some HUD documents are missing (is csso/rocketui/ up to date?)\n" );

	return INIT_OK;
}

void RocketUIImpl::Shutdown()
{
	RocketMenu::Shutdown();
	RocketHud::Shutdown();
	if ( m_bRmlInitialised )
	{
		Rml::Shutdown(); // destroys all contexts, releases fonts/textures
		m_bRmlInitialised = false;
	}
	RocketRender::m_Instance.Shutdown();

	for ( int i = 0; i < m_fontAllocs.Count(); i++ )
		delete[] m_fontAllocs[i];
	m_fontAllocs.Purge();

	m_ctxMenu = m_ctxHud = m_ctxCurrent = NULL;
	BaseClass::Shutdown();
}

void RocketUIImpl::UpdateScreenSize()
{
	int w = 0, h = 0;
	if ( m_pEngine )
		m_pEngine->GetScreenSize( w, h );
	if ( w <= 0 || h <= 0 )
		return;
	if ( w == m_width && h == m_height )
		return;

	m_width = w;
	m_height = h;
	RocketRender::m_Instance.SetScreenSize( w, h );
	if ( m_ctxHud )
		m_ctxHud->SetDimensions( Rml::Vector2i( w, h ) );
	if ( m_ctxMenu )
		m_ctxMenu->SetDimensions( Rml::Vector2i( w, h ) );
}

Rml::ElementDocument *RocketUIImpl::LoadDocumentFile( RocketDesinationContext_t ctx, const char *filepath,
													  LoadDocumentFn loadDocumentFunc, UnloadDocumentFn unloadDocumentFunc )
{
	Rml::Context *dest = NULL;
	switch ( ctx )
	{
	case ROCKET_CONTEXT_MENU:
		dest = m_ctxMenu;
		break;
	case ROCKET_CONTEXT_HUD:
		dest = m_ctxHud;
		break;
	case ROCKET_CONTEXT_CURRENT:
		dest = m_ctxCurrent;
		break;
	}
	if ( !dest )
	{
		Warning( "RocketUI: couldn't load document %s: no context (loaded before the 1st frame?)\n", filepath );
		return NULL;
	}

	Rml::ElementDocument *document = dest->LoadDocument( filepath );

	if ( loadDocumentFunc && unloadDocumentFunc )
	{
		ReloadEntry_t entry = { ctx, loadDocumentFunc, unloadDocumentFunc };
		m_documentReloadFuncs.AddToTail( entry );
	}
	return document;
}

void RocketUIImpl::RunFrame( float time )
{
	m_fTime = time;
	UpdateScreenSize();

	// Elements are added/changed all the time; Update() both contexts once per frame
	// (m_ctxCurrent only tells which one was rendered last).
	if ( m_ctxHud )
		m_ctxHud->Update();
	if ( m_ctxMenu )
		m_ctxMenu->Update();

	RocketMenu::Frame( m_pEngine && m_pEngine->IsInGame(), time );
	RocketHud::Frame( time );
}

void RocketUIImpl::ShowOptionsPanel( bool show )
{
	RocketMenu::ShowOptions( show );
}

void RocketUIImpl::ShowPopup( const char *text, float timeout )
{
	RocketMenu::ShowPopup( text, timeout );
}

void RocketUIImpl::HudSetStatus( const RocketHudStatus_t &status )
{
	if ( rocket_enable.GetBool() )
		RocketHud::SetStatus( status );
}

void RocketUIImpl::HudAddKill( const RocketHudKill_t &kill )
{
	if ( rocket_enable.GetBool() )
		RocketHud::AddKill( kill );
}

void RocketUIImpl::HudChatAdd( const RocketHudChat_t &line )
{
	if ( rocket_enable.GetBool() )
		RocketHud::ChatAdd( line );
}

void RocketUIImpl::HudSetRadar( const RocketHudRadar_t &radar )
{
	if ( rocket_enable.GetBool() )
		RocketHud::SetRadar( radar );
}

void RocketUIImpl::HudScoreboardShow( bool show )
{
	if ( rocket_enable.GetBool() )
		RocketHud::ScoreboardShow( show );
}

void RocketUIImpl::HudScoreboardSet( const RocketHudScoreboard_t &data )
{
	if ( rocket_enable.GetBool() )
		RocketHud::ScoreboardSet( data );
}

void RocketUIImpl::HudChatOpen( bool teamOnly )
{
	if ( rocket_enable.GetBool() )
		RocketHud::ChatOpen( teamOnly );
}

void RocketUIImpl::HudReset()
{
	RocketHud::Reset();
}

void RocketUIImpl::DenyInputToGame( bool value, const char *why )
{
	if ( value )
	{
		m_numInputConsumers++;
		m_inputConsumers.AddToTail( CUtlString( why ) );
	}
	else
	{
		if ( m_numInputConsumers > 0 )
			m_numInputConsumers--;
		m_inputConsumers.FindAndRemove( CUtlString( why ) );
	}

	EnableCursor( m_numInputConsumers > 0 );

	if ( rocket_verbose.GetBool() )
	{
		ConMsg( "input consumers[%d]: ", m_numInputConsumers );
		for ( int i = 0; i < m_inputConsumers.Count(); i++ )
			ConMsg( "(%s) ", m_inputConsumers[i].Get() );
		ConMsg( "\n" );
	}
}

bool RocketUIImpl::IsConsumingInput()
{
	return m_numInputConsumers > 0;
}

void RocketUIImpl::EnableCursor( bool state )
{
	// While the UI owns input the game must not turn the mouse into view movement.
	ConVarRef cl_mouseenable( "cl_mouseenable" );
	if ( cl_mouseenable.IsValid() )
		cl_mouseenable.SetValue( !state );
	m_bCursorVisible = state;
}

static int MouseButtonIndex( ButtonCode_t code )
{
	switch ( code )
	{
	case MOUSE_LEFT:   return 0;
	case MOUSE_RIGHT:  return 1;
	case MOUSE_MIDDLE: return 2;
	case MOUSE_4:      return 3;
	case MOUSE_5:      return 4;
	default:           return -1;
	}
}

static void FingerPosition( const InputEvent_t &e, int w, int h, int &x, int &y )
{
	float fx, fy; // normalized 0..1, carried as float bits (see IN_TouchEvent)
	memcpy( &fx, &e.m_nData2, sizeof( fx ) );
	memcpy( &fy, &e.m_nData3, sizeof( fy ) );
	x = (int)( fx * w );
	y = (int)( fy * h );
}

// Returns true if the game must not see the event.
bool RocketUIImpl::HandleInputEvent( const InputEvent_t &event )
{
	if ( !m_ctxCurrent || !rocket_enable.GetBool() )
		return false;

	// The pause menu is opened with ESC through the registered callback.
	if ( event.m_nType == IE_ButtonPressed && event.m_nData == KEY_ESCAPE )
	{
		if ( m_togglePauseMenuFunc && m_pEngine && m_pEngine->IsInGame() )
			m_togglePauseMenuFunc();
	}

	if ( !IsConsumingInput() )
		return false;
	if ( m_pEngine && m_pEngine->Con_IsVisible() )
		return false;

	switch ( event.m_nType )
	{
	case IE_AnalogValueChanged:
		// The input system posts MOUSE_X / MOUSE_Y separately (absolute position in m_nData2).
		if ( event.m_nData == MOUSE_X )
			m_mouseX = event.m_nData2;
		else if ( event.m_nData == MOUSE_Y )
			m_mouseY = event.m_nData2;
		else if ( event.m_nData == MOUSE_XY )
		{
			m_mouseX = event.m_nData2;
			m_mouseY = event.m_nData3;
		}
		else
			break;
		m_ctxCurrent->ProcessMouseMove( m_mouseX, m_mouseY, 0 );
		break;

	case IE_ButtonDoubleClicked:
	case IE_ButtonPressed:
		if ( IsMouseCode( (ButtonCode_t)event.m_nData ) )
		{
			const ButtonCode_t code = (ButtonCode_t)event.m_nData;
			if ( code == MOUSE_WHEEL_UP )
				m_ctxCurrent->ProcessMouseWheel( -1, 0 );
			else if ( code == MOUSE_WHEEL_DOWN )
				m_ctxCurrent->ProcessMouseWheel( 1, 0 );
			else if ( MouseButtonIndex( code ) >= 0 )
				m_ctxCurrent->ProcessMouseButtonDown( MouseButtonIndex( code ), 0 );
		}
		else
		{
			m_ctxCurrent->ProcessKeyDown( ButtonToRocketKey( (ButtonCode_t)event.m_nData ), 0 );
		}
		break;

	case IE_ButtonReleased:
		if ( IsMouseCode( (ButtonCode_t)event.m_nData ) )
		{
			const int idx = MouseButtonIndex( (ButtonCode_t)event.m_nData );
			if ( idx >= 0 )
				m_ctxCurrent->ProcessMouseButtonUp( idx, 0 );
		}
		else
		{
			m_ctxCurrent->ProcessKeyUp( ButtonToRocketKey( (ButtonCode_t)event.m_nData ), 0 );
		}
		break;

	// Touch: every finger acts as the (single) mouse pointer, which is all menus need.
	case IE_FingerDown:
	{
		int x, y;
		FingerPosition( event, m_width, m_height, x, y );
		m_ctxCurrent->ProcessMouseMove( x, y, 0 );
		m_ctxCurrent->ProcessMouseButtonDown( 0, 0 );
		break;
	}
	case IE_FingerMotion:
	{
		int x, y;
		FingerPosition( event, m_width, m_height, x, y );
		m_ctxCurrent->ProcessMouseMove( x, y, 0 );
		break;
	}
	case IE_FingerUp:
	{
		int x, y;
		FingerPosition( event, m_width, m_height, x, y );
		m_ctxCurrent->ProcessMouseMove( x, y, 0 );
		m_ctxCurrent->ProcessMouseButtonUp( 0, 0 );
		break;
	}

	case IE_FirstVguiEvent + 3: // vgui IE_KeyTyped: m_nData is the typed character
		HandleTextInput( event.m_nData );
		break;

	default:
		return false;
	}

	return true;
}

void RocketUIImpl::HandleTextInput( int codepoint )
{
	if ( !m_ctxCurrent || !IsConsumingInput() || codepoint == 8 ) // RmlUi handles backspace via the key event
		return;
	m_ctxCurrent->ProcessTextInput( (Rml::Character)codepoint );
}

void RocketUIImpl::RenderContext( Rml::Context *ctx )
{
	if ( !rocket_enable.GetBool() || !ctx )
		return;

	m_ctxCurrent = ctx;
	RocketRender::m_Instance.BeginFrame();
	ctx->Render();
	RocketRender::m_Instance.EndFrame();
}

void RocketUIImpl::RenderHUDFrame()
{
	RenderContext( m_ctxHud );
}

void RocketUIImpl::RenderMenuFrame()
{
	RenderContext( m_ctxMenu );
}

bool RocketUIImpl::ReloadDocuments()
{
	rocket_enable.SetValue( false );
	ThreadSleep( 100 ); // let the render thread notice

	CUtlVector<ReloadEntry_t> copy;
	for ( int i = 0; i < m_documentReloadFuncs.Count(); i++ )
		copy.AddToTail( m_documentReloadFuncs[i] );
	m_documentReloadFuncs.Purge(); // the load functions re-register themselves

	for ( int i = 0; i < copy.Count(); i++ )
	{
		const ReloadEntry_t &e = copy[i];
		if ( e.ctx == ROCKET_CONTEXT_HUD && m_ctxCurrent != m_ctxHud )
			continue;
		if ( e.ctx == ROCKET_CONTEXT_MENU && m_ctxCurrent != m_ctxMenu )
			continue;

		e.unload();
		if ( e.ctx != ROCKET_CONTEXT_CURRENT ) // popups etc. are only unloaded
			e.load();
	}

	rocket_enable.SetValue( true );
	return true;
}
