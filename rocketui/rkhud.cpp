#include "rkhud.h"
#include "rocketuiimpl.h" // first: pulls in RmlUi before tier0/dbg.h defines Assert

#include <RmlUi/Core.h>

#include "tier0/dbg.h"
#include "tier1/convar.h"
#include "tier1/strtools.h"
#include "tier1/utlvector.h"
#include "cdll_int.h"

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
		s_d.info = NULL;
		s_d.hp = s_d.armor = s_d.armorLbl = s_d.clip = s_d.reserve = s_d.weapon = s_d.money = s_d.flags = NULL;
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

	// ---- chat ----
	IVEngineClient *s_pEngine = NULL;

	struct ChatLine_t
	{
		Rml::Element *el;
		float end;
		bool old;
	};
	CUtlVector<ChatLine_t> s_chatLines;
	Rml::ElementDocument *s_pChatDoc = NULL;
	Rml::Element *s_chatList = NULL, *s_chatRow = NULL, *s_chatInput = NULL, *s_chatPrompt = NULL;
	bool s_bChatOpen = false;
	bool s_bChatTeam = false;
	bool s_bChatGrab = false;

	static ConVar rocket_hud_chat_time( "rocket_hud_chat_time", "10", 0, "Seconds a chat line stays visible while the chat is closed" );
	static ConVar rocket_hud_chat_max( "rocket_hud_chat_max", "30", 0, "Max chat lines kept" );

	void ChatClose()
	{
		if ( !s_bChatOpen )
			return;
		s_bChatOpen = false;
		if ( s_chatRow )
			s_chatRow->SetClass( "open", false );
		if ( s_chatInput )
			s_chatInput->Blur();
		if ( s_bChatGrab )
		{
			s_bChatGrab = false;
			RocketUIImpl::m_Instance.DenyInputToGame( false, "Chat" );
		}
	}

	void ChatSubmit()
	{
		Rml::ElementFormControl *ctl = rmlui_dynamic_cast<Rml::ElementFormControl *>( s_chatInput );
		if ( ctl && s_pEngine )
		{
			// The text goes into a console command: only printable characters, no quotes / separators / newlines.
			const Rml::String value = ctl->GetValue();
			char safe[160];
			int n = 0;
			for ( size_t i = 0; i < value.size() && n < (int)sizeof( safe ) - 1; i++ )
			{
				const unsigned char c = (unsigned char)value[i];
				if ( c < 32 || c == '"' || c == ';' || c == 127 )
					continue;
				safe[n++] = (char)c;
			}
			safe[n] = 0;

			if ( n > 0 )
			{
				char cmd[256];
				Q_snprintf( cmd, sizeof( cmd ), "%s \"%s\"", s_bChatTeam ? "say_team" : "say", safe );
				s_pEngine->ClientCmd_Unrestricted( cmd );
			}
			ctl->SetValue( "" );
		}
		ChatClose();
	}

	class ChatListener : public Rml::EventListener
	{
	public:
		void ProcessEvent( Rml::Event &event ) override
		{
			if ( event.GetId() != Rml::EventId::Keydown )
				return;
			const int key = event.GetParameter<int>( "key_identifier", 0 );
			if ( key == Rml::Input::KI_ESCAPE )
				ChatClose();
			else if ( key == Rml::Input::KI_RETURN || key == Rml::Input::KI_NUMPADENTER )
				ChatSubmit();
		}
	};
	ChatListener s_chatListener;

	void UnloadChat();
	void LoadChat()
	{
		if ( s_pChatDoc )
			return;
		s_pChatDoc = RocketUIImpl::m_Instance.LoadDocumentFile( ROCKET_CONTEXT_HUD, "hud_chat.rml", LoadChat, UnloadChat );
		if ( !s_pChatDoc )
		{
			Warning( "RocketUI: couldn't load hud_chat.rml\n" );
			return;
		}
		s_chatList = s_pChatDoc->GetElementById( "lines" );
		s_chatRow = s_pChatDoc->GetElementById( "input_row" );
		s_chatInput = s_pChatDoc->GetElementById( "chat_input" );
		s_chatPrompt = s_pChatDoc->GetElementById( "prompt" );
		s_pChatDoc->AddEventListener( Rml::EventId::Keydown, &s_chatListener );
		s_pChatDoc->Show( Rml::ModalFlag::None, Rml::FocusFlag::None );
	}
	void UnloadChat()
	{
		ChatClose();
		s_chatLines.Purge(); // the elements die with the document
		if ( s_pChatDoc )
		{
			s_pChatDoc->RemoveEventListener( Rml::EventId::Keydown, &s_chatListener );
			s_pChatDoc->Close();
		}
		s_pChatDoc = NULL;
		s_chatList = s_chatRow = s_chatInput = s_chatPrompt = NULL;
	}

	void ClearChat()
	{
		for ( int i = 0; i < s_chatLines.Count(); i++ )
		{
			if ( s_chatList && s_chatLines[i].el )
				s_chatList->RemoveChild( s_chatLines[i].el );
		}
		s_chatLines.Purge();
	}

	// ---- scoreboard ----
	Rml::ElementDocument *s_pSbDoc = NULL;
	Rml::Element *s_sbCtScore = NULL, *s_sbTScore = NULL, *s_sbCtRows = NULL, *s_sbTRows = NULL, *s_sbSpec = NULL;
	bool s_bSbShown = false;
	Rml::String s_sbCtCache, s_sbTCache, s_sbSpecCache;
	int s_sbScoreCache[2] = { -1, -1 };

	void UnloadScoreboard();
	void LoadScoreboard()
	{
		if ( s_pSbDoc )
			return;
		s_pSbDoc = RocketUIImpl::m_Instance.LoadDocumentFile( ROCKET_CONTEXT_HUD, "hud_scoreboard.rml", LoadScoreboard, UnloadScoreboard );
		if ( !s_pSbDoc )
		{
			Warning( "RocketUI: couldn't load hud_scoreboard.rml\n" );
			return;
		}
		s_sbCtScore = s_pSbDoc->GetElementById( "ct_score" );
		s_sbTScore = s_pSbDoc->GetElementById( "t_score" );
		s_sbCtRows = s_pSbDoc->GetElementById( "ct_rows" );
		s_sbTRows = s_pSbDoc->GetElementById( "t_rows" );
		s_sbSpec = s_pSbDoc->GetElementById( "spec" );
		s_sbCtCache = s_sbTCache = s_sbSpecCache = "";
		s_sbScoreCache[0] = s_sbScoreCache[1] = -1;
		s_bSbShown = false;
		s_pSbDoc->Hide();
	}
	void UnloadScoreboard()
	{
		if ( s_pSbDoc )
			s_pSbDoc->Close();
		s_pSbDoc = NULL;
		s_sbCtScore = s_sbTScore = s_sbCtRows = s_sbTRows = s_sbSpec = NULL;
		s_bSbShown = false;
	}

	// Rows of one team, best score first.
	Rml::String BuildRows( const RocketHudScoreboard_t &d, int team )
	{
		int idx[ROCKET_HUD_MAX_PLAYERS];
		int n = 0;
		for ( int i = 0; i < d.numPlayers && i < ROCKET_HUD_MAX_PLAYERS; i++ )
		{
			if ( d.players[i].team == team )
				idx[n++] = i;
		}
		for ( int i = 1; i < n; i++ ) // insertion sort, n <= 64
		{
			const int cur = idx[i];
			int j = i - 1;
			while ( j >= 0 && ( d.players[idx[j]].score < d.players[cur].score ||
				( d.players[idx[j]].score == d.players[cur].score && d.players[idx[j]].kills < d.players[cur].kills ) ) )
			{
				idx[j + 1] = idx[j];
				j--;
			}
			idx[j + 1] = cur;
		}

		Rml::String rml;
		char num[16];
		for ( int k = 0; k < n; k++ )
		{
			const RocketHudScoreEntry_t &p = d.players[idx[k]];
			rml += "<div class=\"row";
			if ( !p.alive )
				rml += " dead";
			if ( p.local )
				rml += " me";
			rml += "\"><span class=\"n\">";
			rml += Rml::StringUtilities::EncodeRml( p.name );
			rml += "</span>";
			const int vals[5] = { p.kills, p.assists, p.deaths, p.score, p.ping };
			for ( int v = 0; v < 5; v++ )
			{
				if ( v == 4 && p.bot )
					Q_strncpy( num, "BOT", sizeof( num ) );
				else
					Q_snprintf( num, sizeof( num ), "%d", vals[v] );
				rml += "<span class=\"v\">";
				rml += num;
				rml += "</span>";
			}
			rml += "</div>";
		}
		return rml;
	}

	void SetCached( Rml::Element *e, Rml::String &cache, const Rml::String &rml )
	{
		if ( !e || cache == rml )
			return;
		cache = rml;
		e->SetInnerRML( rml );
	}
}

bool RocketHud::Init( IVEngineClient *engine )
{
	s_pEngine = engine;
	LoadInfobar();
	LoadTimer();
	LoadKillfeed();
	LoadChat();
	LoadScoreboard();
	return s_d.info && s_d.timer && s_d.kill && s_pChatDoc && s_pSbDoc;
}

void RocketHud::Shutdown()
{
	s_pSbDoc = NULL;
	s_sbCtScore = s_sbTScore = s_sbCtRows = s_sbTRows = s_sbSpec = NULL;
	s_bSbShown = false;
	s_bChatOpen = s_bChatGrab = false;
	s_chatLines.Purge();
	s_pChatDoc = NULL;
	s_chatList = s_chatRow = s_chatInput = s_chatPrompt = NULL;
	s_pEngine = NULL;
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

	// Chat lines fade away (display: none) once they are old and the typing line is closed.
	for ( int i = 0; i < s_chatLines.Count(); i++ )
	{
		const bool wantOld = !s_bChatOpen && time >= s_chatLines[i].end;
		if ( wantOld != s_chatLines[i].old )
		{
			s_chatLines[i].old = wantOld;
			s_chatLines[i].el->SetClass( "old", wantOld );
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

void RocketHud::ChatAdd( const RocketHudChat_t &c )
{
	if ( !s_pChatDoc || !s_chatList )
		return;

	Rml::String rml;
	if ( c.name[0] )
	{
		rml += "<span class=\"name ";
		rml += c.team == ROCKET_TEAM_CT ? "ct" : ( c.team == ROCKET_TEAM_T ? "t" : ( c.team == ROCKET_TEAM_SPECTATOR ? "spec" : "" ) );
		rml += "\">";
		rml += Rml::StringUtilities::EncodeRml( c.name );
		rml += "</span>";
		rml += Rml::StringUtilities::EncodeRml( c.text );
	}
	else
	{
		rml += "<span class=\"srv\">";
		rml += Rml::StringUtilities::EncodeRml( c.text );
		rml += "</span>";
	}

	Rml::ElementPtr entry = s_pChatDoc->CreateElement( "div" );
	if ( !entry )
		return;
	entry->SetClass( "line", true );
	entry->SetInnerRML( rml );
	Rml::Element *el = s_chatList->AppendChild( std::move( entry ) );

	ChatLine_t l = { el, s_fTime + rocket_hud_chat_time.GetFloat(), false };
	s_chatLines.AddToTail( l );

	const int maxLines = MAX( 5, rocket_hud_chat_max.GetInt() );
	while ( s_chatLines.Count() > maxLines )
	{
		s_chatList->RemoveChild( s_chatLines[0].el );
		s_chatLines.Remove( 0 );
	}
}

void RocketHud::ChatOpen( bool teamOnly )
{
	if ( !s_pChatDoc || s_bChatOpen )
		return;
	s_bChatOpen = true;
	s_bChatTeam = teamOnly;
	if ( s_chatPrompt )
		s_chatPrompt->SetInnerRML( teamOnly ? "TEAM" : "SAY" );
	if ( s_chatRow )
		s_chatRow->SetClass( "open", true );
	if ( s_chatInput )
		s_chatInput->Focus();
	if ( !s_bChatGrab )
	{
		s_bChatGrab = true;
		RocketUIImpl::m_Instance.DenyInputToGame( true, "Chat" );
	}
}

CON_COMMAND( rocket_test_chat, "Add a fake chat line in RocketUI" )
{
	RocketHudChat_t c;
	memset( &c, 0, sizeof( c ) );
	Q_strncpy( c.name, "Player", sizeof( c.name ) );
	Q_strncpy( c.text, " : hello from RocketUI", sizeof( c.text ) );
	c.team = ROCKET_TEAM_CT;
	RocketHud::ChatAdd( c );
}

void RocketHud::ScoreboardShow( bool show )
{
	ShowDoc( s_pSbDoc, s_bSbShown, show );
}

void RocketHud::ScoreboardSet( const RocketHudScoreboard_t &d )
{
	if ( !s_pSbDoc || !s_bSbShown )
		return;

	char buf[16];
	if ( s_sbScoreCache[0] != d.scoreCT && s_sbCtScore )
	{
		s_sbScoreCache[0] = d.scoreCT;
		Q_snprintf( buf, sizeof( buf ), "%d", d.scoreCT );
		s_sbCtScore->SetInnerRML( buf );
	}
	if ( s_sbScoreCache[1] != d.scoreT && s_sbTScore )
	{
		s_sbScoreCache[1] = d.scoreT;
		Q_snprintf( buf, sizeof( buf ), "%d", d.scoreT );
		s_sbTScore->SetInnerRML( buf );
	}

	SetCached( s_sbCtRows, s_sbCtCache, BuildRows( d, ROCKET_TEAM_CT ) );
	SetCached( s_sbTRows, s_sbTCache, BuildRows( d, ROCKET_TEAM_T ) );

	Rml::String spec;
	for ( int i = 0; i < d.numPlayers && i < ROCKET_HUD_MAX_PLAYERS; i++ )
	{
		if ( d.players[i].team != ROCKET_TEAM_SPECTATOR )
			continue;
		spec += spec.empty() ? "Spectators: " : ", ";
		spec += Rml::StringUtilities::EncodeRml( d.players[i].name );
	}
	SetCached( s_sbSpec, s_sbSpecCache, spec );
}

void RocketHud::Reset()
{
	ShowDoc( s_pSbDoc, s_bSbShown, false );
	ChatClose();
	ClearChat();
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
