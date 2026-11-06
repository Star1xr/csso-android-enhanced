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

		CSGameRules *pRules = CSGameRules();
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
