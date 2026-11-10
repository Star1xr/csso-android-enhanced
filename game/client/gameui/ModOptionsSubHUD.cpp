//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h> // SRC only!!
#endif

#include "ModOptionsSubHUD.h"
#include <stdio.h>

#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "LabeledCommandComboBox.h"
#include "cvarslider.h"
#include "CvarToggleCheckButton.h"
#include "EngineInterface.h"
#include "tier1/convar.h"
#include "BasePanel.h"
#include "GameUI_Interface.h"

#include "hud.h" // for MAX_HUD_COLORS

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CModOptionsSubHUD::CModOptionsSubHUD( vgui::Panel *parent ): vgui::PropertyPage( parent, "ModOptionsSubHUD" )
{
	Button *cancel = new Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	Button *ok = new Button( this, "OK", "#GameUI_OK" );
	ok->SetCommand( "Ok" );

	Button *apply = new Button( this, "Apply", "#GameUI_Apply" );
	apply->SetCommand( "Apply" );

	//=========
	m_pPlayerCountPos = new CLabeledCommandComboBox( this, "PlayerCountPositionComboBox" );
	m_pTeamCounterStyle = new CLabeledCommandComboBox( this, "TeamCounterStyleComboBox" );
	m_pHealthAmmoStyle = new CLabeledCommandComboBox( this, "HealthAmmoStyleComboBox" );
	m_pHUDColor = new CLabeledCommandComboBox( this, "HUDColorComboBox" );
	m_pHUDBackgroundAlpha = new CCvarSlider( this, "HUDBackgroundAlphaSlider", "", 0.0f, 1.0f, "cl_hud_background_alpha" );
	m_pRadarScale = new CCvarSlider( this, "RadarScaleSlider", "", 0.25f, 1.0f, "cl_radar_scale" );
	m_pAlwaysShowInventory = new CCvarToggleCheckButton( this, "AlwaysShowInventoryCheckbox", "#GameUI_HUD_AlwaysShowInventory", "cl_showloadout" );
	m_pRadarRotate = new CCvarToggleCheckButton( this, "RadarRotateCheckbox", "#GameUI_HUD_RotateRadar", "cl_radar_rotate" );
	m_pRadarSquare = new CLabeledCommandComboBox( this, "RadarSquareComboBox" );
	m_pMenuBackground = new CLabeledCommandComboBox( this, "MenuBackgroundComboBox" );
	m_pMenuAgent = new CLabeledCommandComboBox( this, "MenuAgentComboBox" );

	m_pPlayerCountPos->AddItem( "#GameUI_HUD_PlayerCount_Top", "hud_playercount_pos 0" );
	m_pPlayerCountPos->AddItem( "#GameUI_HUD_PlayerCount_Bottom", "hud_playercount_pos 1" );

	// The strings are normally in gameui_<language>.txt. This repo doesn't ship those, so register
	// an English fallback when the loaded language files don't know the tokens yet.
	if ( g_pVGuiLocalize )
	{
		wchar_t szAlive[]   = L"Alive count";
		wchar_t szAvatars[] = L"Player avatars";
		if ( !g_pVGuiLocalize->Find( "#GameUI_HUD_TeamCounterStyle_Alive" ) )
			g_pVGuiLocalize->AddString( "GameUI_HUD_TeamCounterStyle_Alive", szAlive, "" );
		if ( !g_pVGuiLocalize->Find( "#GameUI_HUD_TeamCounterStyle_Avatars" ) )
			g_pVGuiLocalize->AddString( "GameUI_HUD_TeamCounterStyle_Avatars", szAvatars, "" );
	}

	m_pTeamCounterStyle->AddItem( "#GameUI_HUD_TeamCounterStyle_Alive", "hud_teamcounter_style 0" );
	m_pTeamCounterStyle->AddItem( "#GameUI_HUD_TeamCounterStyle_Avatars", "hud_teamcounter_style 1" );

	m_pHealthAmmoStyle->AddItem( "#GameUI_HUD_HealthAmmoStyle_0", "cl_hud_healthammo_style 0" );
	m_pHealthAmmoStyle->AddItem( "#GameUI_HUD_HealthAmmoStyle_1", "cl_hud_healthammo_style 1" );

	m_pRadarSquare->AddItem( "#GameUI_HUD_RadarSquare_0", "cl_radar_square 0" );
	m_pRadarSquare->AddItem( "#GameUI_HUD_RadarSquare_1", "cl_radar_square 1" );
	m_pRadarSquare->AddItem( "#GameUI_HUD_RadarSquare_2", "cl_radar_square 2" );

	m_pMenuAgent->AddItem( "#GameUI_Loadout_Agent_None", "loadout_mainmenu_agent 0" );
	m_pMenuAgent->AddItem( "#GameUI_Loadout_Agent_T", "loadout_mainmenu_agent 1" );
	m_pMenuAgent->AddItem( "#GameUI_Loadout_Agent_CT", "loadout_mainmenu_agent 2" );

	char localization[64];
	char command[64];
	for ( int i = 0; i < ARRAYSIZE(s_MenuBackgrounds); i++ )
	{
		Q_snprintf( command, sizeof( command ), "cl_menu_background %d", i );
		m_pMenuBackground->AddItem( s_MenuBackgrounds[i].szName, command );
	}
	for ( int i = 0; i < MAX_HUD_COLORS; i++ )
	{
		Q_snprintf( localization, sizeof( localization ), "#GameUI_HUD_Color_%d", i );
		Q_snprintf( command, sizeof( command ), "cl_hud_color %d", i );
		m_pHUDColor->AddItem( localization, command );
	}

	m_pPlayerCountPos->AddActionSignalTarget( this );
	m_pTeamCounterStyle->AddActionSignalTarget( this );
	m_pHealthAmmoStyle->AddActionSignalTarget( this );
	m_pHUDColor->AddActionSignalTarget( this );
	m_pHUDBackgroundAlpha->AddActionSignalTarget( this );
	m_pRadarScale->AddActionSignalTarget( this );
	m_pAlwaysShowInventory->AddActionSignalTarget( this );
	m_pRadarRotate->AddActionSignalTarget( this );
	m_pRadarSquare->AddActionSignalTarget( this );
	m_pMenuBackground->AddActionSignalTarget( this );
	m_pMenuAgent->AddActionSignalTarget( this );

	LoadControlSettings( "Resource/ModOptionsSubHUD.res" );

	// If ModOptionsSubHUD.res doesn't know "TeamCounterStyleComboBox" the control is still at 0,0:
	// PerformLayout() then places it (and its caption) below the last row on its own.
	int csX, csY, csW, csT;
	m_pTeamCounterStyle->GetBounds( csX, csY, csW, csT );
	m_bAutoPlaceStyle = ( csX == 0 && csY == 0 );

	m_pTeamCounterStyleLabel = NULL;
	if ( m_bAutoPlaceStyle )
	{
		m_pTeamCounterStyleLabel = new Label( this, "TeamCounterStyleLabel", "Team counter style" );
		m_pTeamCounterStyleLabel->SetZPos( 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: place the team counter style row when the .res doesn't. Idempotent: it only
// looks at the other controls and never moves them.
//-----------------------------------------------------------------------------
void CModOptionsSubHUD::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( !m_bAutoPlaceStyle )
		return;

	int px, py, pw, pt;
	m_pPlayerCountPos->GetBounds( px, py, pw, pt );

	// Caption of the player count combo: a label on the same row, used as template
	Label *pTemplate = NULL;
	int bottom = 0;
	int buttonTop = GetTall();
	for ( int i = 0; i < GetChildCount(); i++ )
	{
		Panel *pChild = GetChild( i );
		if ( !pChild || pChild == m_pTeamCounterStyle || pChild == m_pTeamCounterStyleLabel )
			continue;

		int cx, cy, cw, ct;
		pChild->GetBounds( cx, cy, cw, ct );

		if ( !Q_stricmp( pChild->GetName(), "OK" ) || !Q_stricmp( pChild->GetName(), "Cancel" ) || !Q_stricmp( pChild->GetName(), "Apply" ) )
		{
			buttonTop = MIN( buttonTop, cy );
			continue;
		}

		bottom = MAX( bottom, cy + ct );

		Label *pLabel = dynamic_cast<Label *>( pChild );
		if ( pLabel && !pTemplate && abs( cy - py ) <= pt && cx < px )
			pTemplate = pLabel;
	}

	int gap = MAX( 2, pt / 4 );
	int y = bottom + gap;
	if ( y + pt > buttonTop - gap )
		y = MAX( 0, buttonTop - gap - pt );

	m_pTeamCounterStyle->SetBounds( px, y, pw, pt );

	if ( m_pTeamCounterStyleLabel )
	{
		int lx = px, lw = 0, ly, lt;
		if ( pTemplate )
			pTemplate->GetBounds( lx, ly, lw, lt );
		m_pTeamCounterStyleLabel->SetBounds( lx, y, MAX( lw, 1 ), pt );
		if ( pTemplate )
			m_pTeamCounterStyleLabel->SetFont( pTemplate->GetFont() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CModOptionsSubHUD::~CModOptionsSubHUD()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModOptionsSubHUD::OnControlModified()
{
	PostMessage( GetParent(), new KeyValues( "ApplyButtonEnable" ) );
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModOptionsSubHUD::OnResetData()
{
	ConVarRef hud_playercount_pos( "hud_playercount_pos" );
	if ( hud_playercount_pos.IsValid() )
		m_pPlayerCountPos->SetInitialItem( hud_playercount_pos.GetInt() );

	ConVarRef hud_teamcounter_style( "hud_teamcounter_style" );
	if ( hud_teamcounter_style.IsValid() )
		m_pTeamCounterStyle->SetInitialItem( hud_teamcounter_style.GetInt() );

	ConVarRef cl_hud_healthammo_style( "cl_hud_healthammo_style" );
	if ( cl_hud_healthammo_style.IsValid() )
		m_pHealthAmmoStyle->SetInitialItem( cl_hud_healthammo_style.GetInt() );

	ConVarRef cl_hud_color( "cl_hud_color" );
	if ( cl_hud_color.IsValid() )
		m_pHUDColor->SetInitialItem( cl_hud_color.GetInt() );

	ConVarRef cl_radar_square( "cl_radar_square" );
	if ( cl_radar_square.IsValid() )
		m_pRadarSquare->SetInitialItem( cl_radar_square.GetInt() );

	ConVarRef cl_menu_background( "cl_menu_background" );
	if ( cl_menu_background.IsValid() )
		m_pMenuBackground->SetInitialItem( cl_menu_background.GetInt() );

	ConVarRef loadout_mainmenu_agent( "loadout_mainmenu_agent" );
	if ( loadout_mainmenu_agent.IsValid() )
		m_pMenuAgent->SetInitialItem( loadout_mainmenu_agent.GetInt() );

	m_pHUDBackgroundAlpha->Reset();
	m_pRadarScale->Reset();
	m_pAlwaysShowInventory->Reset();
	m_pRadarRotate->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModOptionsSubHUD::OnApplyChanges()
{
	m_pPlayerCountPos->ApplyChanges();
	m_pTeamCounterStyle->ApplyChanges();
	m_pHealthAmmoStyle->ApplyChanges();
	m_pHUDColor->ApplyChanges();
	m_pHUDBackgroundAlpha->ApplyChanges();
	m_pRadarScale->ApplyChanges();
	m_pAlwaysShowInventory->ApplyChanges();
	m_pRadarRotate->ApplyChanges();
	m_pRadarSquare->ApplyChanges();

	ConVarRef cl_menu_background( "cl_menu_background" );
	if ( cl_menu_background.GetInt() != m_pMenuBackground->GetActiveItem() )
	{
		m_pMenuBackground->ApplyChanges();
		BasePanel()->RestartBackgroundVideo();
	}

	m_pMenuAgent->ApplyChanges();

	// update agent on main menu
	GameUI().UpdateAgentModel();
}