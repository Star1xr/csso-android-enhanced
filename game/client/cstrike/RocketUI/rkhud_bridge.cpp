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
