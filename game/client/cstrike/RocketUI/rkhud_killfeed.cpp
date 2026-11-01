#include "csso_ui_config.h"

#if defined( USE_CSSOROCKETUI )

// Include RmlUi before engine headers: tier0/debug.h defines Assert as a macro.
#include "../../../../rmlui/Include/RmlUi/Core.h"

#include "rkhud_killfeed.h"

#include "cbase.h"
#include "c_playerresource.h"
#include "c_cs_playerresource.h"
#include "hud_macros.h"
#include "rocketui/rocketui.h"
#include "tier0/valve_minmax_off.h"
#include "tier1/strtools.h"

#include <vector>

// memdbgon must be the last include file in a .cpp file.
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( RkHudKillfeed );

static ConVar rocket_hud_killfeed_linger_time(
	"rocket_hud_killfeed_linger_time", "5", 0,
	"How long RocketUI killfeed entries stay visible." );

struct KillfeedEntry
{
	Rml::String attackerName;
	Rml::String weaponName;
	Rml::String victimName;
	bool headshot;
	bool wallbang;
	float createdAt;
};

struct KillfeedData
{
	std::vector<KillfeedEntry> entries;
};

static KillfeedData s_killfeedData;

static Rml::Context *GetRocketHudContext()
{
	IRocketUI *rocket = RocketUI();
	return rocket ? rocket->AccessHudContext() : NULL;
}

static Rml::String GetPlayerName( int playerIndex, const char *fallback )
{
	if ( playerIndex <= 0 || !g_PR )
		return Rml::String( fallback ? fallback : "Unknown" );

	C_CS_PlayerResource *playerResource =
		static_cast<C_CS_PlayerResource *>( GameResources() );
	if ( !playerResource )
		return Rml::String( fallback ? fallback : "Unknown" );

	wchar_t wideName[MAX_DECORATED_PLAYER_NAME_LENGTH] = {};
	playerResource->GetDecoratedPlayerName(
		playerIndex, wideName, sizeof( wideName ),
		k_EDecoratedPlayerNameFlag_Simple );

	if ( !wideName[0] )
		return Rml::String( fallback ? fallback : "Unknown" );

	char playerName[MAX_DECORATED_PLAYER_NAME_LENGTH * 4] = {};
	V_UnicodeToUTF8( wideName, playerName, sizeof( playerName ) );
	return Rml::String( playerName[0] ? playerName :
		( fallback ? fallback : "Unknown" ) );
}

static void LoadRkKillfeed()
{
	RkHudKillfeed *killfeed = GET_HUDELEMENT( RkHudKillfeed );
	if ( killfeed )
		killfeed->LoadDocument();
}

static void UnloadRkKillfeed()
{
	RkHudKillfeed *killfeed = GET_HUDELEMENT( RkHudKillfeed );
	if ( killfeed )
		killfeed->UnloadDocument();
}

RkHudKillfeed::RkHudKillfeed( const char *name ) :
	CHudElement( name ),
	m_pInstance( NULL ),
	m_bVisible( false )
{
	SetHiddenBits( 0 );
}

RkHudKillfeed::~RkHudKillfeed()
{
	StopListeningForAllEvents();
	UnloadDocument();
}

void RkHudKillfeed::LevelInit( void )
{
	s_killfeedData.entries.clear();
	ListenForGameEvent( "player_death" );
	LoadDocument();
}

void RkHudKillfeed::LevelShutdown( void )
{
	StopListeningForAllEvents();
	s_killfeedData.entries.clear();
	UnloadDocument();
}

void RkHudKillfeed::LoadDocument( void )
{
	IRocketUI *rocket = RocketUI();
	Rml::Context *hudContext = GetRocketHudContext();
	if ( !rocket || !hudContext )
	{
		Warning( "[RocketUI] Killfeed HUD context is unavailable.\n" );
		return;
	}

	if ( m_pInstance || m_dataModel )
		UnloadDocument();

	Rml::DataModelConstructor constructor =
		hudContext->CreateDataModel( "killfeed_model" );
	if ( !constructor )
	{
		Warning( "[RocketUI] Could not create killfeed data model.\n" );
		return;
	}

	// Type definitions live on the Context, not the model. Keep them across
	// document reloads and map changes, where the model itself is recreated.
	Rml::DataTypeRegister *typeRegister = constructor.GetDataTypeRegister();
	if ( !typeRegister->GetDefinition<KillfeedEntry>() )
	{
		Rml::StructHandle<KillfeedEntry> entryHandle =
			constructor.RegisterStruct<KillfeedEntry>();
		if ( !entryHandle )
		{
			hudContext->RemoveDataModel( "killfeed_model" );
			Warning( "[RocketUI] Could not register killfeed entry type.\n" );
			return;
		}

		entryHandle.RegisterMember( "attacker_name", &KillfeedEntry::attackerName );
		entryHandle.RegisterMember( "weapon_name", &KillfeedEntry::weaponName );
		entryHandle.RegisterMember( "victim_name", &KillfeedEntry::victimName );
		entryHandle.RegisterMember( "headshot", &KillfeedEntry::headshot );
		entryHandle.RegisterMember( "wallbang", &KillfeedEntry::wallbang );
	}

	if ( !typeRegister->GetDefinition<std::vector<KillfeedEntry>>() &&
		 !constructor.RegisterArray<std::vector<KillfeedEntry>>() )
	{
		hudContext->RemoveDataModel( "killfeed_model" );
		Warning( "[RocketUI] Could not register killfeed entry array.\n" );
		return;
	}

	if ( !constructor.Bind( "killfeed_entries", &s_killfeedData.entries ) )
	{
		hudContext->RemoveDataModel( "killfeed_model" );
		Warning( "[RocketUI] Could not bind killfeed entries.\n" );
		return;
	}

	m_dataModel = constructor.GetModelHandle();
	m_pInstance = rocket->LoadDocumentFile(
		ROCKET_CONTEXT_HUD, "hud_killfeed.rml",
		&LoadRkKillfeed, &UnloadRkKillfeed );

	if ( !m_pInstance )
	{
		hudContext->RemoveDataModel( "killfeed_model" );
		m_dataModel = Rml::DataModelHandle();
		Warning( "[RocketUI] Could not load hud_killfeed.rml.\n" );
		return;
	}

	ShowPanel( false );
}

void RkHudKillfeed::UnloadDocument( void )
{
	Rml::Context *hudContext = GetRocketHudContext();
	if ( hudContext && m_dataModel )
		hudContext->RemoveDataModel( "killfeed_model" );

	m_dataModel = Rml::DataModelHandle();
	if ( m_pInstance )
	{
		m_pInstance->Close();
		m_pInstance = NULL;
	}
	m_bVisible = false;
}

void RkHudKillfeed::ShowPanel( bool show )
{
	if ( !m_pInstance )
		return;

	if ( show && !m_bVisible )
		m_pInstance->Show();
	else if ( !show && m_bVisible )
		m_pInstance->Hide();

	m_bVisible = show;
}

void RkHudKillfeed::RetireExpiredEntries( void )
{
	if ( !gpGlobals || s_killfeedData.entries.empty() )
		return;

	bool changed = false;
	const float lingerTime = MAX( rocket_hud_killfeed_linger_time.GetFloat(), 0.1f );
	while ( !s_killfeedData.entries.empty() &&
			gpGlobals->curtime - s_killfeedData.entries.front().createdAt >= lingerTime )
	{
		s_killfeedData.entries.erase( s_killfeedData.entries.begin() );
		changed = true;
	}

	if ( changed && m_dataModel )
		m_dataModel.DirtyVariable( "killfeed_entries" );
	if ( changed && s_killfeedData.entries.empty() )
		ShowPanel( false );
}

void RkHudKillfeed::SetActive( bool active )
{
	RetireExpiredEntries();
	ShowPanel( active && !s_killfeedData.entries.empty() );
	CHudElement::SetActive( active );
}

bool RkHudKillfeed::ShouldDraw( void )
{
	RetireExpiredEntries();
	return m_pInstance && cl_drawhud.GetBool() &&
		!s_killfeedData.entries.empty() && CHudElement::ShouldDraw();
}

void RkHudKillfeed::FireGameEvent( IGameEvent *event )
{
	if ( !event || Q_strcmp( event->GetName(), "player_death" ) != 0 ||
		 !m_dataModel || !gpGlobals )
		return;

	const int attackerIndex =
		engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
	const int victimIndex =
		engine->GetPlayerForUserID( event->GetInt( "userid" ) );
	const char *weapon = event->GetString( "weapon" );

	KillfeedEntry entry;
	entry.attackerName = GetPlayerName(
		attackerIndex, attackerIndex > 0 ? "Unknown" : "World" );
	entry.victimName = GetPlayerName( victimIndex, "Unknown" );
	entry.weaponName = ( weapon && weapon[0] ) ? weapon : "world";
	entry.headshot = event->GetInt( "headshot" ) != 0;
	entry.wallbang = event->GetInt( "penetrated" ) != 0;
	entry.createdAt = gpGlobals->curtime;

	// Keep the document bounded even if the frame loop is temporarily paused.
	if ( s_killfeedData.entries.size() >= 6 )
		s_killfeedData.entries.erase( s_killfeedData.entries.begin() );
	s_killfeedData.entries.push_back( entry );

	m_dataModel.DirtyVariable( "killfeed_entries" );
	ShowPanel( true );
}

#endif // USE_CSSOROCKETUI
