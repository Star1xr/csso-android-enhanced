#include "rkgamemenus.h"
#include "rocketuiimpl.h" // first: pulls in RmlUi before tier0/dbg.h defines Assert

#include <RmlUi/Core.h>

#include "tier0/dbg.h"
#include "tier1/convar.h"
#include "tier1/strtools.h"
#include "cdll_int.h"

namespace
{
	IVEngineClient *s_pEngine = NULL;

	struct Menu_t
	{
		Rml::ElementDocument *doc;
		Rml::Element *content;
		bool open;
		bool grab;
		bool canCancel;
	};
	Menu_t s_buy, s_team, s_class;

	// Buy menu state
	RocketBuyMenu_t s_buyData;
	bool s_bHaveBuyData = false;
	int s_buySel = 0;
	bool s_bBuyDirty = false;
	Rml::String s_catsCache, s_itemsCache, s_moneyCache;
	Rml::Element *s_buyCats = NULL, *s_buyItems = NULL, *s_buyMoney = NULL;

	// Class menu state
	int s_classTeam = 0;

	void Command( const char *cmd )
	{
		if ( s_pEngine )
			s_pEngine->ClientCmd_Unrestricted( cmd );
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

	void SetOpen( Menu_t &m, const char *why, bool open )
	{
		if ( !m.doc || m.open == open )
			return;
		m.open = open;
		if ( open )
		{
			m.doc->Show( Rml::ModalFlag::None, Rml::FocusFlag::Document );
			m.doc->PullToFront();
			if ( !m.grab )
			{
				m.grab = true;
				RocketUIImpl::m_Instance.DenyInputToGame( true, why );
			}
		}
		else
		{
			m.doc->Hide();
			if ( m.grab )
			{
				m.grab = false;
				RocketUIImpl::m_Instance.DenyInputToGame( false, why );
			}
		}
	}

	// The touch buttons would sit on top of the menu; the vgui buy menu hides them the same way.
	void BuyTouch( bool menuOpen )
	{
		ConVarRef disable( "touch_disable_on_buymenu" );
		if ( disable.IsValid() && !disable.GetBool() )
			return;
		Command( menuOpen ? "touch_enable 0" : "touch_enable 1" );
	}

	void SetCached( Rml::Element *e, Rml::String &cache, const Rml::String &rml )
	{
		if ( !e || cache == rml )
			return;
		cache = rml;
		e->SetInnerRML( rml );
	}

	void SetContent( Menu_t &m, const Rml::String &rml )
	{
		if ( m.content )
			m.content->SetInnerRML( rml );
	}

	// ---- buy menu ----
	void RebuildBuy()
	{
		s_bBuyDirty = false;
		if ( !s_buy.doc )
			return;

		if ( s_buySel < 0 || s_buySel >= s_buyData.numCategories )
			s_buySel = 0;

		char buf[96];
		Rml::String cats;
		for ( int i = 0; i < s_buyData.numCategories; i++ )
		{
			Q_snprintf( buf, sizeof( buf ), "<div class=\"cat%s\" id=\"cat_%d\">", i == s_buySel ? " sel" : "", i );
			cats += buf;
			cats += Rml::StringUtilities::EncodeRml( s_buyData.categories[i].name );
			cats += "</div>";
		}
		SetCached( s_buyCats, s_catsCache, cats );

		Rml::String items;
		if ( s_buyData.numCategories > 0 )
		{
			const RocketBuyCategory_t &c = s_buyData.categories[s_buySel];
			for ( int i = 0; i < c.numItems; i++ )
			{
				const RocketBuyItem_t &it = c.items[i];
				const bool off = it.disabled || it.price > s_buyData.money;
				Q_snprintf( buf, sizeof( buf ), "<div class=\"item%s\" id=\"item_%d\">", off ? " off" : "", i );
				items += buf;
				items += Rml::StringUtilities::EncodeRml( it.name[0] ? it.name : it.alias );
				Q_snprintf( buf, sizeof( buf ), "<span class=\"price\">$%d</span></div>", it.price );
				items += buf;
			}
			if ( c.numItems == 0 )
				items += "<div class=\"empty\">Nothing to buy here.</div>";
		}
		else
			items += "<div class=\"empty\">Buy menu config not available.</div>";
		SetCached( s_buyItems, s_itemsCache, items );

		Q_snprintf( buf, sizeof( buf ), "$%d", s_buyData.money );
		SetCached( s_buyMoney, s_moneyCache, buf );
	}

	// Letters, digits and '_' only: the alias ends up in a console command.
	bool SafeAlias( const char *alias, char *out, int outLen )
	{
		int n = 0;
		for ( const char *p = alias; *p && n < outLen - 1; p++ )
		{
			const char c = *p;
			if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '_' )
				out[n++] = c;
		}
		out[n] = 0;
		return n > 0;
	}

	void CloseBuy()
	{
		if ( s_buy.open )
			BuyTouch( false );
		SetOpen( s_buy, "BuyMenu", false );
	}

	class BuyListener : public Rml::EventListener
	{
	public:
		void ProcessEvent( Rml::Event &event ) override
		{
			if ( event.GetId() == Rml::EventId::Keydown )
			{
				if ( event.GetParameter<int>( "key_identifier", 0 ) == Rml::Input::KI_ESCAPE )
					CloseBuy();
				return;
			}

			const Rml::String id = ClickedId( event );
			if ( id.size() > 4 && !id.compare( 0, 4, "cat_" ) )
			{
				s_buySel = atoi( id.c_str() + 4 );
				s_bBuyDirty = true; // rebuilt in Frame(), not from inside this handler
			}
			else if ( id.size() > 5 && !id.compare( 0, 5, "item_" ) )
			{
				const int idx = atoi( id.c_str() + 5 );
				if ( s_buySel < 0 || s_buySel >= s_buyData.numCategories )
					return;
				const RocketBuyCategory_t &c = s_buyData.categories[s_buySel];
				if ( idx < 0 || idx >= c.numItems )
					return;
				const RocketBuyItem_t &it = c.items[idx];
				char alias[40];
				if ( it.disabled || it.price > s_buyData.money || !SafeAlias( it.alias, alias, sizeof( alias ) ) )
					return;
				char cmd[64];
				Q_snprintf( cmd, sizeof( cmd ), "buy %s", alias );
				Command( cmd );
			}
			else if ( id == "buy_rebuy" )
				Command( "rebuy" );
			else if ( id == "buy_auto" )
				Command( "autobuy" );
			else if ( id == "buy_close" )
				CloseBuy();
		}
	};
	BuyListener s_buyListener;

	// ---- team menu ----
	class TeamListener : public Rml::EventListener
	{
	public:
		void ProcessEvent( Rml::Event &event ) override
		{
			if ( event.GetId() == Rml::EventId::Keydown )
			{
				if ( event.GetParameter<int>( "key_identifier", 0 ) == Rml::Input::KI_ESCAPE && s_team.canCancel )
					SetOpen( s_team, "TeamMenu", false );
				return;
			}

			const Rml::String id = ClickedId( event );
			if ( id == "team_t" )
				Command( "jointeam 2" );
			else if ( id == "team_ct" )
				Command( "jointeam 3" );
			else if ( id == "team_auto" )
				Command( "jointeam 0" );
			else if ( id == "team_spec" )
				Command( "spectate" );
			else if ( id != "team_cancel" )
				return;
			SetOpen( s_team, "TeamMenu", false );
		}
	};
	TeamListener s_teamListener;

	// ---- class menu ----
	const char *const s_tClasses[7] = { "Phoenix Connection", "Leet Krew", "Separatist", "Balkan", "Professional", "Anarchist", "Pirate" };
	const char *const s_ctClasses[7] = { "SEAL Team 6", "GSG-9", "SAS", "GIGN", "FBI", "IDF", "SWAT" };
	const int s_firstTClass = 1;  // CS_CLASS_PHOENIX_CONNNECTION, keep in sync with cs_shareddefs.h
	const int s_firstCTClass = 8; // CS_CLASS_SEAL_TEAM_6

	class ClassListener : public Rml::EventListener
	{
	public:
		void ProcessEvent( Rml::Event &event ) override
		{
			if ( event.GetId() == Rml::EventId::Keydown )
			{
				if ( event.GetParameter<int>( "key_identifier", 0 ) == Rml::Input::KI_ESCAPE && s_class.canCancel )
					SetOpen( s_class, "ClassMenu", false );
				return;
			}

			const Rml::String id = ClickedId( event );
			if ( id == "class_auto" )
				Command( "joinclass 0" );
			else if ( id.size() > 6 && !id.compare( 0, 6, "class_" ) )
			{
				const int cls = atoi( id.c_str() + 6 );
				const int first = s_classTeam == ROCKET_TEAM_CT ? s_firstCTClass : s_firstTClass;
				if ( cls < first || cls >= first + 7 )
					return;
				char cmd[32];
				Q_snprintf( cmd, sizeof( cmd ), "joinclass %d", cls );
				Command( cmd );
			}
			else if ( id != "class_cancel" )
				return;
			SetOpen( s_class, "ClassMenu", false );
		}
	};
	ClassListener s_classListener;

	void LoadMenu( Menu_t &m, const char *file, Rml::EventListener *listener )
	{
		m.doc = RocketUIImpl::m_Instance.LoadDocumentFile( ROCKET_CONTEXT_HUD, file );
		if ( !m.doc )
		{
			Warning( "RocketUI: couldn't load %s\n", file );
			return;
		}
		m.content = m.doc->GetElementById( "content" );
		m.doc->AddEventListener( Rml::EventId::Click, listener );
		m.doc->AddEventListener( Rml::EventId::Keydown, listener );
		m.open = false;
		m.doc->Hide();
	}
}

void RocketGameMenus::Init( IVEngineClient *engine )
{
	s_pEngine = engine;
	memset( &s_buy, 0, sizeof( s_buy ) );
	memset( &s_team, 0, sizeof( s_team ) );
	memset( &s_class, 0, sizeof( s_class ) );

	LoadMenu( s_buy, "hud_buymenu.rml", &s_buyListener );
	if ( s_buy.doc )
	{
		s_buyCats = s_buy.doc->GetElementById( "cats" );
		s_buyItems = s_buy.doc->GetElementById( "items" );
		s_buyMoney = s_buy.doc->GetElementById( "money" );
	}
	LoadMenu( s_team, "hud_teammenu.rml", &s_teamListener );
	LoadMenu( s_class, "hud_classmenu.rml", &s_classListener );
}

void RocketGameMenus::Shutdown()
{
	s_pEngine = NULL;
	memset( &s_buy, 0, sizeof( s_buy ) );
	memset( &s_team, 0, sizeof( s_team ) );
	memset( &s_class, 0, sizeof( s_class ) );
	s_buyCats = s_buyItems = s_buyMoney = NULL;
	s_bHaveBuyData = s_bBuyDirty = false;
	s_catsCache = s_itemsCache = s_moneyCache = "";
}

void RocketGameMenus::Frame()
{
	if ( s_bBuyDirty && s_buy.open && s_bHaveBuyData )
		RebuildBuy();
}

void RocketGameMenus::Reset()
{
	CloseBuy();
	SetOpen( s_team, "TeamMenu", false );
	SetOpen( s_class, "ClassMenu", false );
	s_bHaveBuyData = false;
}

bool RocketGameMenus::AnyOpen()
{
	return s_buy.open || s_team.open || s_class.open;
}

void RocketGameMenus::BuyShow( bool show )
{
	if ( !s_buy.doc )
		return;
	if ( show )
	{
		if ( s_buy.open )
			return;
		s_catsCache = s_itemsCache = s_moneyCache = "";
		SetOpen( s_buy, "BuyMenu", true );
		BuyTouch( true );
		if ( s_bHaveBuyData )
			RebuildBuy();
	}
	else
		CloseBuy();
}

bool RocketGameMenus::BuyIsOpen()
{
	return s_buy.open;
}

void RocketGameMenus::BuySet( const RocketBuyMenu_t &menu )
{
	s_buyData = menu;
	s_bHaveBuyData = true;
	if ( s_buy.open )
		RebuildBuy();
}

void RocketGameMenus::TeamShow( bool show, const RocketTeamMenu_t &info )
{
	if ( !s_team.doc )
		return;
	if ( !show )
	{
		SetOpen( s_team, "TeamMenu", false );
		return;
	}

	s_team.canCancel = info.canCancel;
	char buf[128];
	Rml::String rml = "<div class=\"title\">CHOOSE YOUR TEAM</div>";
	Q_snprintf( buf, sizeof( buf ), "<div class=\"btn t\" id=\"team_t\">Terrorists<span class=\"cnt\">%d players</span></div>", info.numT );
	rml += buf;
	Q_snprintf( buf, sizeof( buf ), "<div class=\"btn ct\" id=\"team_ct\">Counter-Terrorists<span class=\"cnt\">%d players</span></div>", info.numCT );
	rml += buf;
	rml += "<div class=\"btn\" id=\"team_auto\">Auto assign</div>";
	if ( info.allowSpectate )
		rml += "<div class=\"btn\" id=\"team_spec\">Spectate</div>";
	if ( info.canCancel )
		rml += "<div class=\"btn cancel\" id=\"team_cancel\">Cancel</div>";
	SetContent( s_team, rml );
	SetOpen( s_team, "TeamMenu", true );
}

void RocketGameMenus::ClassShow( bool show, int team, bool canCancel )
{
	if ( !s_class.doc )
		return;
	if ( !show )
	{
		SetOpen( s_class, "ClassMenu", false );
		return;
	}

	s_classTeam = team;
	s_class.canCancel = canCancel;
	const bool ct = ( team == ROCKET_TEAM_CT );
	const int first = ct ? s_firstCTClass : s_firstTClass;
	const char *const *names = ct ? s_ctClasses : s_tClasses;

	char buf[128];
	Rml::String rml = "<div class=\"title\">CHOOSE YOUR CLASS</div>";
	for ( int i = 0; i < 7; i++ )
	{
		Q_snprintf( buf, sizeof( buf ), "<div class=\"btn %s\" id=\"class_%d\">%s</div>", ct ? "ct" : "t", first + i, names[i] );
		rml += buf;
	}
	rml += "<div class=\"btn\" id=\"class_auto\">Random</div>";
	if ( canCancel )
		rml += "<div class=\"btn cancel\" id=\"class_cancel\">Cancel</div>";
	SetContent( s_class, rml );
	SetOpen( s_class, "ClassMenu", true );
}
