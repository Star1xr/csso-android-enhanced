//========= HUD safe area (notch / rounded corners) ==========================//
//
// Purpose: cl_hud_safearea (0..50, 1080p units) moves the in-game HUD towards the centre of the
//          screen on both sides. Elements in the left half move right, elements in the right half
//          move left, wide / centred / full-screen elements (scope, vignette, flash, timer) stay.
//          Only CHudElement panels are touched, so the buy menu, scoreboard and the main menus
//          are not affected.
//
// How: the left / right HUD elements are put into two full-screen sized container panels once,
//      and the containers are offset. Elements keep doing their own layout relative to the screen.
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_hud_safearea( "cl_hud_safearea", "0", FCVAR_ARCHIVE, "HUD margin towards the screen centre on both sides, 0 - 99 (1080p units). Not used by menus.", true, 0, true, 99 );

namespace
{
	class CHudSafeAreaContainer : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE( CHudSafeAreaContainer, vgui::Panel );
	public:
		CHudSafeAreaContainer( vgui::Panel* pParent, const char* pszName ) : vgui::Panel( pParent, pszName )
		{
			SetPaintEnabled( false );
			SetPaintBackgroundEnabled( false );
			SetPaintBorderEnabled( false );
			SetMouseInputEnabled( false );
			SetKeyBoardInputEnabled( false );
			SetVisible( true );
		}
	};

	CHudSafeAreaContainer* g_pLeftContainer = NULL;
	CHudSafeAreaContainer* g_pRightContainer = NULL;
	vgui::Panel* g_pContainerParent = NULL;

	// elements that already were put into a container (or deliberately left alone)
	CUtlVector<vgui::Panel*> g_Handled;
	int g_nLastHudCount = -1;
	int g_nLastScreenW = 0;
	int g_nLastScreenH = 0;
}

static void HudSafeArea_Sort( vgui::Panel* pViewport )
{
	int sw, sh;
	vgui::surface()->GetScreenSize( sw, sh );

	if ( !g_pLeftContainer || g_pContainerParent != pViewport )
	{
		g_pContainerParent = pViewport;
		g_Handled.RemoveAll();
		g_pLeftContainer = new CHudSafeAreaContainer( pViewport, "HudSafeAreaLeft" );
		g_pRightContainer = new CHudSafeAreaContainer( pViewport, "HudSafeAreaRight" );
	}

	FOR_EACH_VEC( gHUD.m_HudList, i )
	{
		vgui::Panel* pPanel = dynamic_cast<vgui::Panel*>( gHUD.m_HudList[i] );
		if ( !pPanel || g_Handled.HasElement( pPanel ) )
			continue;

		// only plain children of the viewport, anything else has its own parent for a reason
		if ( pPanel->GetParent() != pViewport )
			continue;

		int x, y, w, h;
		pPanel->GetBounds( x, y, w, h );
		if ( w <= 0 || h <= 0 )
			continue;		// not laid out yet, try again later

		g_Handled.AddToTail( pPanel );

		// The kill feed and the weapon list ("inventory") are full width panels that draw at their right
		// edge: they move with the right side.
		const char* pszName = gHUD.m_HudList[i]->GetName();
		if ( pszName && ( Q_stristr( pszName, "DeathNotice" ) || Q_stristr( pszName, "WeaponSelection" ) ) )
		{
			pPanel->SetParent( g_pRightContainer );
			continue;
		}

		// wide elements (full screen overlays, the round timer block, ...) stay
		if ( w >= sw / 2 )
			continue;

		int cx = x + w / 2;
		if ( cx < sw / 2 && x + w <= sw / 2 )
			pPanel->SetParent( g_pLeftContainer );
		else if ( cx > sw / 2 && x >= sw / 2 )
			pPanel->SetParent( g_pRightContainer );
	}
}

void CSHudSafeArea_Update()
{
	int nInset = (int)( cl_hud_safearea.GetFloat() + 0.5f );

	// nothing to do until the feature has been used once
	if ( nInset <= 0 && !g_pLeftContainer )
		return;

	if ( !g_pClientMode || !g_pClientMode->GetViewport() )
		return;

	vgui::Panel* pViewport = g_pClientMode->GetViewport();

	// new elements may appear later (level load), so look again when the list changes
	if ( nInset > 0 && ( !g_pLeftContainer || g_nLastHudCount != gHUD.m_HudList.Count() ) )
	{
		g_nLastHudCount = gHUD.m_HudList.Count();
		HudSafeArea_Sort( pViewport );
	}

	if ( !g_pLeftContainer || !g_pRightContainer )
		return;

	int sw, sh;
	vgui::surface()->GetScreenSize( sw, sh );

	int px = ( nInset > 0 ) ? vgui::scheme()->GetProportionalScaledValue( nInset ) : 0;

	// containers have the size of the screen so that "right edge" anchors keep working
	g_pLeftContainer->SetBounds( px, 0, sw, sh );
	g_pRightContainer->SetBounds( -px, 0, sw, sh );
	g_nLastScreenW = sw;
	g_nLastScreenH = sh;
}
