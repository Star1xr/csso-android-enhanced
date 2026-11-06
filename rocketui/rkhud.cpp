#include "rkhud.h"
#include "rocketuiimpl.h" // first: pulls in RmlUi before tier0/dbg.h defines Assert

#include <RmlUi/Core.h>

#include "tier0/dbg.h"
#include "tier1/convar.h"
#include "tier1/strtools.h"
#include "tier1/utlvector.h"

static ConVar rocket_hud_killfeed_time( "rocket_hud_killfeed_time", "6", 0, "Seconds a killfeed entry stays on screen" );
static ConVar rocket_hud_killfeed_max( "rocket_hud_killfeed_max", "6", 0, "Max killfeed entries" );

namespace
{
	struct Docs_t
	{
		Rml::ElementDocument *info, *timer, *kill;
		Rml::Element *hp, *armor, *armorLbl, *clip, *reserve, *weapon, *money, *flags;
		Rml::Element *time, *ct, *t;
		Rml::Element *list;
	};
	Docs_t s_d = {};

	struct KillEntry_t
	{
		Rml::Element *el;
		float end;
	};
	CUtlVector<KillEntry_t> s_kills;
	float s_fTime = 0.0f;

	bool s_bInfoShown = false;
	bool s_bTimerShown = false;

	// Last text written per element, so unchanged values don't dirty the layout every frame.
	struct Cache_t
	{
		char hp[16], armor[16], clip[16], reserve[16], weapon[40], money[24], flags[64], time[16], ct[16], t[16], timeClass[16];
		bool helmet;
	};
	Cache_t s_c;

	void SetText( Rml::Element *e, char *cache, int cacheLen, const char *text )
	{
		if ( !e || !Q_strncmp( cache, text, cacheLen ) )
			return;
		Q_strncpy( cache, text, cacheLen );
		e->SetInnerRML( Rml::StringUtilities::EncodeRml( text ) );
	}
	#define SET_TEXT( elem, field, text ) SetText( s_d.elem, s_c.field, sizeof( s_c.field ), text )

	void ClearKills()
	{
		for ( int i = 0; i < s_kills.Count(); i++ )
		{
			if ( s_d.list && s_kills[i].el )
				s_d.list->RemoveChild( s_kills[i].el );
		}
		s_kills.Purge();
	}

	void ShowDoc( Rml::ElementDocument *doc, bool &flag, bool show )
	{
		if ( !doc || flag == show )
			return;
		flag = show;
		if ( show )
			doc->Show( Rml::ModalFlag::None, Rml::FocusFlag::None );
		else
			doc->Hide();
	}

	// ---- load / unload (also used by rocket_reload) ----
	void UnloadInfobar();
	void LoadInfobar()
	{
		if ( s_d.info )
			return;
		s_d.info = RocketUIImpl::m_Instance.LoadDocumentFile( ROCKET_CONTEXT_HUD, "hud_infobar.rml", LoadInfobar, UnloadInfobar );
		if ( !s_d.info )
		{
			Warning( "RocketUI: couldn't load hud_infobar.rml\n" );
			return;
		}
		s_d.hp = s_d.info->GetElementById( "hp" );
		s_d.armor = s_d.info->GetElementById( "armor" );
		s_d.armorLbl = s_d.info->GetElementById( "armor_lbl" );
		s_d.clip = s_d.info->GetElementById( "clip" );
		s_d.reserve = s_d.info->GetElementById( "reserve" );
		s_d.weapon = s_d.info->GetElementById( "weapon" );
		s_d.money = s_d.info->GetElementById( "money" );
		s_d.flags = s_d.info->GetElementById( "flags" );
		memset( &s_c, 0, sizeof( s_c ) );
		s_bInfoShown = false;
		s_d.info->Hide();
	}
	void UnloadInfobar()
	{
		if ( s_d.info )
			s_d.info->Close();
		s_d.info = s_d.hp = s_d.armor = s_d.armorLbl = s_d.clip = s_d.reserve = s_d.weapon = s_d.money = s_d.flags = NULL;
		s_bInfoShown = false;
	}

	void UnloadTimer();
	void LoadTimer()
	{
		if ( s_d.timer )
			return;
		s_d.timer = RocketUIImpl::m_Instance.LoadDocumentFile( ROCKET_CONTEXT_HUD, "hud_roundtimer.rml", LoadTimer, UnloadTimer );
		if ( !s_d.timer )
		{
			Warning( "RocketUI: couldn't load hud_roundtimer.rml\n" );
			return;
		}
		s_d.time = s_d.timer->GetElementById( "time" );
		s_d.ct = s_d.timer->GetElementById( "ct" );
		s_d.t = s_d.timer->GetElementById( "t" );
		s_bTimerShown = false;
		s_d.timer->Hide();
	}
	void UnloadTimer()
	{
		if ( s_d.timer )
			s_d.timer->Close();
		s_d.timer = NULL;
		s_d.time = s_d.ct = s_d.t = NULL;
		s_bTimerShown = false;
	}

	void UnloadKillfeed();
	void LoadKillfeed()
	{
		if ( s_d.kill )
			return;
		s_d.kill = RocketUIImpl::m_Instance.LoadDocumentFile( ROCKET_CONTEXT_HUD, "hud_killfeed.rml", LoadKillfeed, UnloadKillfeed );
		if ( !s_d.kill )
		{
			Warning( "RocketUI: couldn't load hud_killfeed.rml\n" );
			return;
		}
		s_d.list = s_d.kill->GetElementById( "list" );
		s_d.kill->Show( Rml::ModalFlag::None, Rml::FocusFlag::None );
	}
	void UnloadKillfeed()
	{
		s_kills.Purge(); // the elements die with the document
		if ( s_d.kill )
			s_d.kill->Close();
		s_d.kill = NULL;
		s_d.list = NULL;
	}

	const char *TeamClass( int team )
	{
		return team == ROCKET_TEAM_CT ? "ct" : ( team == ROCKET_TEAM_T ? "t" : "" );
	}

	void AppendName( Rml::String &rml, const char *name, int team )
	{
		rml += "<span class=\"";
		rml += TeamClass( team );
		rml += "\">";
		rml += Rml::StringUtilities::EncodeRml( name );
		rml += "</span>";
	}
}

bool RocketHud::Init()
{
	LoadInfobar();
	LoadTimer();
	LoadKillfeed();
	return s_d.info && s_d.timer && s_d.kill;
}

void RocketHud::Shutdown()
{
	s_kills.Purge();
	memset( &s_d, 0, sizeof( s_d ) );
	s_bInfoShown = s_bTimerShown = false;
}

void RocketHud::Frame( float time )
{
	s_fTime = time;
	for ( int i = s_kills.Count() - 1; i >= 0; i-- )
	{
		if ( time >= s_kills[i].end )
		{
			if ( s_d.list )
				s_d.list->RemoveChild( s_kills[i].el );
			s_kills.Remove( i );
		}
	}
}

void RocketHud::SetStatus( const RocketHudStatus_t &st )
{
	char buf[64];

	// Infobar
	ShowDoc( s_d.info, s_bInfoShown, st.showInfobar );
	if ( st.showInfobar && s_d.info )
	{
		Q_snprintf( buf, sizeof( buf ), "%d", st.health );
		SET_TEXT( hp, hp, buf );
		s_d.hp->SetClass( "low", st.health <= 25 );

		Q_snprintf( buf, sizeof( buf ), "%d", st.armor );
		SET_TEXT( armor, armor, buf );
		if ( s_d.armorLbl && s_c.helmet != st.helmet )
		{
			s_c.helmet = st.helmet;
			s_d.armorLbl->SetInnerRML( st.helmet ? "AR+H" : "AR" );
		}

		if ( st.clip >= 0 )
		{
			Q_snprintf( buf, sizeof( buf ), "%d", st.clip );
			SET_TEXT( clip, clip, buf );
			Q_snprintf( buf, sizeof( buf ), "/ %d", st.reserve );
			SET_TEXT( reserve, reserve, buf );
		}
		else
		{
			SET_TEXT( clip, clip, "" );
			SET_TEXT( reserve, reserve, "" );
		}

		SET_TEXT( weapon, weapon, st.weapon );

		Q_snprintf( buf, sizeof( buf ), "$%d", st.money );
		SET_TEXT( money, money, buf );

		buf[0] = 0;
		if ( st.hasC4 )
			Q_strncat( buf, "C4  ", sizeof( buf ), COPY_ALL_CHARACTERS );
		if ( st.defuser )
			Q_strncat( buf, "KIT  ", sizeof( buf ), COPY_ALL_CHARACTERS );
		if ( st.inBuyZone )
			Q_strncat( buf, "BUY ZONE", sizeof( buf ), COPY_ALL_CHARACTERS );
		SET_TEXT( flags, flags, buf );
	}

	// Round timer
	ShowDoc( s_d.timer, s_bTimerShown, st.showTimer );
	if ( st.showTimer && s_d.timer )
	{
		const int secs = st.roundTimeLeft > 0.0f ? (int)( st.roundTimeLeft + 0.99f ) : 0;
		Q_snprintf( buf, sizeof( buf ), "%d:%02d", secs / 60, secs % 60 );
		SET_TEXT( time, time, buf );

		const char *cls = st.bombPlanted ? "bomb" : ( st.freezePeriod ? "freeze" : ( secs <= 10 ? "low" : "" ) );
		if ( s_d.time && Q_strcmp( s_c.timeClass, cls ) )
		{
			Q_strncpy( s_c.timeClass, cls, sizeof( s_c.timeClass ) );
			s_d.time->SetClassNames( cls );
		}

		Q_snprintf( buf, sizeof( buf ), "%d", st.scoreCT );
		SET_TEXT( ct, ct, buf );
		Q_snprintf( buf, sizeof( buf ), "%d", st.scoreT );
		SET_TEXT( t, t, buf );
	}
}

void RocketHud::AddKill( const RocketHudKill_t &k )
{
	if ( !s_d.kill || !s_d.list )
		return;

	Rml::String rml;
	if ( !k.suicide && k.attacker[0] )
	{
		AppendName( rml, k.attacker, k.attackerTeam );
		if ( k.assister[0] )
		{
			rml += " + ";
			rml += Rml::StringUtilities::EncodeRml( k.assister );
		}
	}
	rml += "<span class=\"gun\">";
	rml += Rml::StringUtilities::EncodeRml( k.weapon[0] ? k.weapon : "world" );
	rml += "</span>";
	AppendName( rml, k.victim, k.victimTeam );
	if ( k.headshot )
		rml += "<span class=\"tag\">HS</span>";
	if ( k.wallbang )
		rml += "<span class=\"tag\">WB</span>";
	if ( k.noscope )
		rml += "<span class=\"tag\">NS</span>";
	if ( k.blind )
		rml += "<span class=\"tag\">BLIND</span>";

	Rml::ElementPtr entry = s_d.kill->CreateElement( "div" );
	if ( !entry )
		return;
	entry->SetClass( "kill", true );
	entry->SetClass( "me", k.localAttacker );
	entry->SetClass( "dead", k.localVictim );
	entry->SetInnerRML( rml );
	Rml::Element *el = s_d.list->AppendChild( std::move( entry ) );

	KillEntry_t e = { el, s_fTime + rocket_hud_killfeed_time.GetFloat() };
	s_kills.AddToTail( e );

	// Drop the oldest ones above the limit.
	const int maxEntries = MAX( 1, rocket_hud_killfeed_max.GetInt() );
	while ( s_kills.Count() > maxEntries )
	{
		s_d.list->RemoveChild( s_kills[0].el );
		s_kills.Remove( 0 );
	}
}

void RocketHud::Reset()
{
	ClearKills();
	ShowDoc( s_d.info, s_bInfoShown, false );
	ShowDoc( s_d.timer, s_bTimerShown, false );
}

CON_COMMAND( rocket_test_kill, "Add a fake killfeed entry in RocketUI" )
{
	RocketHudKill_t k;
	memset( &k, 0, sizeof( k ) );
	Q_strncpy( k.attacker, "Attacker", sizeof( k.attacker ) );
	Q_strncpy( k.victim, "Victim", sizeof( k.victim ) );
	Q_strncpy( k.weapon, "ak47", sizeof( k.weapon ) );
	k.attackerTeam = ROCKET_TEAM_T;
	k.victimTeam = ROCKET_TEAM_CT;
	k.headshot = true;
	RocketHud::AddKill( k );
}
