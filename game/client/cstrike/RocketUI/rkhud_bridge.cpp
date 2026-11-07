#include "cbase.h"
#include "rkhud_bridge.h"

#if defined( USE_CSSOROCKETUI )

#include "rocketui/rocketui.h"
#include "c_cs_player.h"
#include "c_cs_playerresource.h"
#include "c_cs_team.h"
#include "cs_gamerules.h"
#include "weapon_csbase.h"
#include "GameEventListener.h"
#include "igamesystem.h"
#include "hud_basechat.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "vgui/ILocalize.h"
#include "cs_weapon_parse.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "../VGUI/cstrikespectatorgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IRocketUI *RocketUI_ClientGet(); // vgui_int.cpp

bool RocketHud_IsActive()
{
	IRocketUI *pRocket = RocketUI_ClientGet();
	if ( !pRocket || !pRocket->AccessHudContext() )
		return false;

	static ConVarRef rocketEnable( "rocket_enable" );
	return !rocketEnable.IsValid() || rocketEnable.GetBool();
}

namespace
{
	int TeamToRocket( int team )
	{
		switch ( team )
		{
		case TEAM_CT:        return ROCKET_TEAM_CT;
		case TEAM_TERRORIST: return ROCKET_TEAM_T;
		case TEAM_SPECTATOR: return ROCKET_TEAM_SPECTATOR;
		default:             return ROCKET_TEAM_UNASSIGNED;
		}
	}

	// Strips the "weapon_" prefix of a weapon class name.
	const char *WeaponAlias( const char *name )
	{
		const char *alias = StringAfterPrefix( name, "weapon_" );
		return alias ? alias : name;
	}

	void FillStatus( RocketHudStatus_t &st )
	{
		memset( &st, 0, sizeof( st ) );
		st.clip = -1;
		st.roundTimeLeft = -1.0f;

		C_CSPlayer *pLocal = C_CSPlayer::GetLocalCSPlayer();
		if ( !pLocal || !engine->IsInGame() )
			return;

		// Observing someone in first person / chase: show that player's bar.
		C_CSPlayer *pPlayer = pLocal;
		if ( pLocal->IsObserver() && ( pLocal->GetObserverMode() == OBS_MODE_IN_EYE || pLocal->GetObserverMode() == OBS_MODE_CHASE ) )
		{
			C_CSPlayer *pTarget = ToCSPlayer( pLocal->GetObserverTarget() );
			if ( pTarget )
				pPlayer = pTarget;
		}

		st.showInfobar = pPlayer->IsAlive();
		if ( st.showInfobar )
		{
			st.health = MAX( pPlayer->GetHealth(), 0 );
			st.armor = pPlayer->ArmorValue();
			st.helmet = pPlayer->HasHelmet();
			st.defuser = pPlayer->HasDefuser();
			st.hasC4 = pPlayer->HasC4();
			st.inBuyZone = pLocal->IsInBuyZone();
			st.money = MAX( pLocal->GetAccount(), 0 );

			CWeaponCSBase *pWeapon = pPlayer->GetActiveCSWeapon();
			if ( pWeapon )
			{
				V_strncpy( st.weapon, WeaponAlias( pWeapon->GetName() ), sizeof( st.weapon ) );
				if ( pWeapon->GetMaxClip1() > 0 )
				{
					st.clip = MAX( pWeapon->Clip1(), 0 );
					st.reserve = MAX( pWeapon->GetReserveAmmoCount( AMMO_POSITION_PRIMARY ), 0 );
				}
			}
		}

		CCSGameRules *pRules = CSGameRules();
		if ( pRules )
		{
			st.showTimer = true;
			st.roundTimeLeft = pRules->GetRoundRemainingTime();
			st.freezePeriod = pRules->IsFreezePeriod();
			st.bombPlanted = pRules->m_bBombPlanted;
		}

		C_Team *pCT = GetGlobalTeam( TEAM_CT );
		C_Team *pT = GetGlobalTeam( TEAM_TERRORIST );
		st.scoreCT = pCT ? pCT->Get_Score() : 0;
		st.scoreT = pT ? pT->Get_Score() : 0;
	}
}

bool RocketHud_OnChatPrintf( int iPlayerIndex, const char *msg )
{
	if ( !RocketHud_IsActive() || !msg )
		return false;

	RocketHudChat_t line;
	memset( &line, 0, sizeof( line ) );

	const char *p = msg;
	while ( *p == '\n' )
		p++;

	// "\x03Name\x01 : text": the name is the part after the player-name color code.
	if ( (unsigned char)*p == COLOR_PLAYERNAME )
	{
		p++;
		const char *e = p;
		while ( *e && !( (unsigned char)*e > 0 && (unsigned char)*e < COLOR_MAX ) )
			e++;
		V_strncpy( line.name, p, MIN( (int)( e - p ) + 1, (int)sizeof( line.name ) ) );
		p = e;
	}

	// The rest: drop color control characters and newlines.
	int n = 0;
	for ( ; *p && n < (int)sizeof( line.text ) - 1; p++ )
	{
		const unsigned char c = (unsigned char)*p;
		if ( c == '\n' || ( c > 0 && c < COLOR_MAX ) )
			continue;
		line.text[n++] = (char)c;
	}
	line.text[n] = 0;

	if ( !line.name[0] && !line.text[0] )
		return true;

	if ( iPlayerIndex > 0 && g_PR )
		line.team = TeamToRocket( g_PR->GetTeam( iPlayerIndex ) );

	if ( IRocketUI *pRocket = RocketUI_ClientGet() )
		pRocket->HudChatAdd( line );
	return true;
}

static bool s_bScoreboardWanted = false;
static float s_flNextRadar = 0.0f;
static float s_flNextScoreboard = 0.0f;

bool RocketHud_OnScoreboardShow( bool show )
{
	IRocketUI *pRocket = RocketUI_ClientGet();
	if ( !show )
	{
		s_bScoreboardWanted = false;
		if ( pRocket )
			pRocket->HudScoreboardShow( false );
		return false;
	}

	if ( !RocketHud_IsActive() )
		return false;

	s_bScoreboardWanted = true;
	s_flNextScoreboard = 0.0f; // send data on the next Update
	pRocket->HudScoreboardShow( true );
	return true;
}

static void FillScoreboard( RocketHudScoreboard_t &d )
{
	memset( &d, 0, sizeof( d ) );
	if ( !g_PR )
		return;

	C_CS_PlayerResource *pCSPR = dynamic_cast<C_CS_PlayerResource *>( g_PR );
	for ( int i = 1; i <= gpGlobals->maxClients && d.numPlayers < ROCKET_HUD_MAX_PLAYERS; i++ )
	{
		if ( !g_PR->IsConnected( i ) || g_PR->IsHLTV( i ) || g_PR->IsReplay( i ) )
			continue;

		RocketHudScoreEntry_t &e = d.players[d.numPlayers++];
		V_strncpy( e.name, g_PR->GetPlayerName( i ), sizeof( e.name ) );
		e.team = TeamToRocket( g_PR->GetTeam( i ) );
		e.kills = g_PR->GetFrags( i );
		e.assists = g_PR->GetAssists( i );
		e.deaths = g_PR->GetDeaths( i );
		e.score = g_PR->GetPlayerScore( i );
		e.mvps = pCSPR ? pCSPR->GetNumMVPs( i ) : 0;
		e.ping = g_PR->GetPing( i );
		e.alive = g_PR->IsAlive( i );
		e.local = g_PR->IsLocalPlayer( i );
		e.bot = g_PR->IsFakePlayer( i );
	}

	C_Team *pCT = GetGlobalTeam( TEAM_CT );
	C_Team *pT = GetGlobalTeam( TEAM_TERRORIST );
	d.scoreCT = pCT ? pCT->Get_Score() : 0;
	d.scoreT = pT ? pT->Get_Score() : 0;
}

static KeyValues *s_pBuyConfig = NULL;
static bool s_bBuyConfigTried = false;
static float s_flNextBuy = 0.0f;

// "#Token" -> localized UTF-8, anything else is copied.
static void LocalizeText( const char *text, char *out, int outLen )
{
	out[0] = 0;
	if ( !text )
		return;
	if ( text[0] == '#' && g_pVGuiLocalize )
	{
		const wchar_t *w = g_pVGuiLocalize->Find( text );
		if ( w )
		{
			V_UnicodeToUTF8( w, out, outLen );
			if ( out[0] )
				return;
		}
	}
	V_strncpy( out, text, outLen );
}

// Same list the vgui buy menu shows: scripts/buymenuconfig.txt, filtered for the player's team and map type.
static void FillBuyMenu( RocketBuyMenu_t &m )
{
	memset( &m, 0, sizeof( m ) );
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	if ( !pPlayer )
		return;
	m.money = MAX( pPlayer->GetAccount(), 0 );

	if ( !s_pBuyConfig && !s_bBuyConfigTried )
	{
		s_bBuyConfigTried = true;
		s_pBuyConfig = new KeyValues( "BuyMenuConfig" );
		if ( !s_pBuyConfig->LoadFromFile( g_pFullFileSystem, "scripts/buymenuconfig.txt", "GAME" ) )
		{
			Warning( "RocketUI: scripts/buymenuconfig.txt not found, buy menu will be empty\n" );
			s_pBuyConfig->deleteThis();
			s_pBuyConfig = NULL;
		}
	}
	if ( !s_pBuyConfig )
		return;

	for ( KeyValues *kvCat = s_pBuyConfig->GetFirstTrueSubKey(); kvCat && m.numCategories < ROCKET_BUY_MAX_CATEGORIES; kvCat = kvCat->GetNextTrueSubKey() )
	{
		RocketBuyCategory_t &cat = m.categories[m.numCategories];
		memset( &cat, 0, sizeof( cat ) );

		for ( KeyValues *kvItem = kvCat->GetFirstTrueSubKey(); kvItem && cat.numItems < ROCKET_BUY_MAX_ITEMS; kvItem = kvItem->GetNextTrueSubKey() )
		{
			const int iTeam = kvItem->GetInt( "team" );
			if ( iTeam != TEAM_UNASSIGNED && pPlayer->GetTeamNumber() != iTeam )
				continue;
			if ( AliasToWeaponID( kvItem->GetName() ) == WEAPON_NONE )
				continue;

			RocketBuyItem_t &it = cat.items[cat.numItems++];
			V_strncpy( it.alias, kvItem->GetName(), sizeof( it.alias ) );
			it.price = kvItem->GetInt( "price" );

			const char *pszName = kvItem->GetString( "name", NULL );
			if ( AliasToWeaponID( kvItem->GetName() ) == ITEM_DEFUSER && CSGameRules() && CSGameRules()->IsHostageRescueMap() )
				pszName = "#CStrike_WPNHUD_Cutters";
			LocalizeText( pszName, it.name, sizeof( it.name ) );

			it.disabled = kvItem->GetBool( "de_cs_only" ) && CSGameRules() && !CSGameRules()->IsBombDefuseMap() && !CSGameRules()->IsHostageRescueMap();
		}

		if ( cat.numItems == 0 )
			continue; // not for this team

		V_strncpy( cat.name, kvCat->GetName(), sizeof( cat.name ) );
		for ( char *c = cat.name; *c; c++ )
		{
			if ( *c == '_' )
				*c = ' ';
		}
		if ( cat.name[0] >= 'a' && cat.name[0] <= 'z' )
			cat.name[0] -= 'a' - 'A';
		m.numCategories++;
	}
}

bool RocketHud_OnBuyMenuShow( bool show )
{
	IRocketUI *pRocket = RocketUI_ClientGet();
	if ( !show )
	{
		if ( pRocket )
			pRocket->HudBuyMenuShow( false );
		return false;
	}

	if ( !RocketHud_IsActive() )
		return false;

	static RocketBuyMenu_t menu; // ~11 KB, not for the stack
	FillBuyMenu( menu );
	pRocket->HudBuyMenuSet( menu );
	pRocket->HudBuyMenuShow( true );
	s_flNextBuy = gpGlobals->curtime + 0.25f;
	return true;
}

bool RocketHud_OnTeamMenuShow( bool show )
{
	IRocketUI *pRocket = RocketUI_ClientGet();
	RocketTeamMenu_t info;
	memset( &info, 0, sizeof( info ) );

	if ( !show )
	{
		if ( pRocket )
			pRocket->HudTeamMenuShow( false, info );
		return false;
	}

	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	if ( !RocketHud_IsActive() || !pPlayer )
		return false;

	C_Team *pT = GetGlobalTeam( TEAM_TERRORIST );
	C_Team *pCT = GetGlobalTeam( TEAM_CT );
	C_Team *pSpec = GetGlobalTeam( TEAM_SPECTATOR );
	info.numT = pT ? pT->GetNumPlayers() : 0;
	info.numCT = pCT ? pCT->GetNumPlayers() : 0;

	const ConVar *allowSpectators = cvar->FindVar( "mp_allowspectators" );
	info.allowSpectate = allowSpectators && allowSpectators->GetBool() && CSGameRules() &&
		( ( pSpec ? pSpec->GetNumPlayers() : 0 ) < CSGameRules()->GetMaxSpectatorSlots() );
	info.canCancel = pPlayer->GetTeamNumber() != TEAM_UNASSIGNED;
	info.currentTeam = TeamToRocket( pPlayer->GetTeamNumber() );

	pRocket->HudTeamMenuShow( true, info );
	return true;
}

bool RocketHud_OnClassMenuShow( bool show )
{
	IRocketUI *pRocket = RocketUI_ClientGet();
	if ( !show )
	{
		if ( pRocket )
			pRocket->HudClassMenuShow( false, 0, false );
		return false;
	}

	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	if ( !RocketHud_IsActive() || !pPlayer )
		return false;

	pRocket->HudClassMenuShow( true, TeamToRocket( pPlayer->GetTeamNumber() ), pPlayer->IsAlive() );
	return true;
}

bool RocketHud_OnChatOpen( bool teamOnly )
{
	if ( !RocketHud_IsActive() )
		return false;
	RocketUI_ClientGet()->HudChatOpen( teamOnly );
	return true;
}

class CRocketHudSystem : public CAutoGameSystemPerFrame, public CGameEventListener
{
public:
	CRocketHudSystem() : CAutoGameSystemPerFrame( "RocketHudSystem" ) {}

	virtual void LevelInitPostEntity()
	{
		ListenForGameEvent( "player_death" );
	}

	virtual void LevelShutdownPreEntity()
	{
		s_bScoreboardWanted = false;
		StopListeningForAllEvents();
		if ( IRocketUI *pRocket = RocketUI_ClientGet() )
			pRocket->HudReset();
	}

	virtual void Update( float frametime )
	{
		IRocketUI *pRocket = RocketUI_ClientGet();
		if ( !pRocket )
			return;

		RocketHudStatus_t status;
		FillStatus( status );
		pRocket->HudSetStatus( status );

		// Radar: 20 Hz is plenty for dots.
		if ( gpGlobals->curtime >= s_flNextRadar )
		{
			s_flNextRadar = gpGlobals->curtime + 0.05f;
			static RocketHudRadar_t radar; // ~1.5 KB
			memset( &radar, 0, sizeof( radar ) );
			if ( RocketHud_IsActive() )
			{
				CCSMapOverview *pOverview = (CCSMapOverview *)GET_HUDELEMENT( CCSMapOverview );
				if ( pOverview )
					pOverview->FillRocketRadar( radar );
			}
			pRocket->HudSetRadar( radar );
		}

		// Buy menu: keep money / prices fresh, close it when the player leaves the buy zone or dies (the vgui one did the same in OnThink).
		if ( pRocket->HudBuyMenuIsOpen() && gpGlobals->curtime >= s_flNextBuy )
		{
			s_flNextBuy = gpGlobals->curtime + 0.25f;
			C_CSPlayer *pBuyer = C_CSPlayer::GetLocalCSPlayer();
			if ( !pBuyer || !pBuyer->IsAlive() || !pBuyer->IsInBuyZone() )
			{
				pRocket->HudBuyMenuShow( false );
			}
			else
			{
				static RocketBuyMenu_t buyMenu;
				FillBuyMenu( buyMenu );
				pRocket->HudBuyMenuSet( buyMenu );
			}
		}

		// The scoreboard only needs fresh data a few times per second.
		if ( s_bScoreboardWanted && gpGlobals->curtime >= s_flNextScoreboard )
		{
			s_flNextScoreboard = gpGlobals->curtime + 0.25f;
			static RocketHudScoreboard_t scoreboard; // ~7 KB, not for the stack
			FillScoreboard( scoreboard );
			pRocket->HudScoreboardSet( scoreboard );
		}
	}

	virtual void FireGameEvent( IGameEvent *event )
	{
		IRocketUI *pRocket = RocketUI_ClientGet();
		if ( !pRocket || !g_PR || Q_strcmp( event->GetName(), "player_death" ) )
			return;

		const int iVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		const int iKiller = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		const int iAssister = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		const int iLocal = engine->GetLocalPlayer();

		RocketHudKill_t kill;
		memset( &kill, 0, sizeof( kill ) );

		V_strncpy( kill.victim, g_PR->GetPlayerName( iVictim ), sizeof( kill.victim ) );
		kill.victimTeam = TeamToRocket( g_PR->GetTeam( iVictim ) );
		kill.suicide = ( iKiller == 0 || iKiller == iVictim );
		if ( !kill.suicide )
		{
			V_strncpy( kill.attacker, g_PR->GetPlayerName( iKiller ), sizeof( kill.attacker ) );
			kill.attackerTeam = TeamToRocket( g_PR->GetTeam( iKiller ) );
			if ( iAssister > 0 && iAssister != iKiller )
				V_strncpy( kill.assister, g_PR->GetPlayerName( iAssister ), sizeof( kill.assister ) );
		}

		V_strncpy( kill.weapon, event->GetString( "weapon" ), sizeof( kill.weapon ) );
		kill.headshot = event->GetInt( "headshot" ) > 0;
		kill.wallbang = event->GetInt( "penetrated" ) > 0;
		kill.noscope = event->GetInt( "noscope" ) > 0;
		kill.blind = event->GetInt( "blind" ) > 0;
		kill.localAttacker = ( iKiller == iLocal && !kill.suicide );
		kill.localVictim = ( iVictim == iLocal );

		pRocket->HudAddKill( kill );
	}
};

static CRocketHudSystem s_RocketHudSystem;

#endif // USE_CSSOROCKETUI
