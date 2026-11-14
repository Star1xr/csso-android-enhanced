//========= Gyroscope options page ==========================================//
#include "OptionsSubGyro.h"
#include "CvarToggleCheckButton.h"
#include "cvarslider.h"

#include <KeyValues.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>
#include "tier1/convar.h"
#include <stdio.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

COptionsSubGyro::COptionsSubGyro( vgui::Panel *parent ) : PropertyPage( parent, "OptionsSubGyro" )
{
	m_pGyroEnable = new CCvarToggleCheckButton( this, "GyroEnable", "Enable gyroscope", "gyroscope" );
	m_pReverseX = new CCvarToggleCheckButton( this, "GyroReverseX", "Reverse left/right", "gyroscope_reverse_x" );
	m_pReverseY = new CCvarToggleCheckButton( this, "GyroReverseY", "Reverse up/down", "gyroscope_reverse_y" );

	m_pSensitivityPreLabel = new Label( this, "GyroSensitivityPreLabel", "Gyroscope sensitivity" );
	m_pSensitivitySlider = new CCvarSlider( this, "GyroSensitivitySlider", "Gyroscope sensitivity",
		0.1f, 5.0f, "gyroscope_sensitivity", true );
	m_pSensitivityLabel = new TextEntry( this, "GyroSensitivityLabel" );
	m_pSensitivityLabel->AddActionSignalTarget( this );

	UpdateSensitivityLabel();
}

COptionsSubGyro::~COptionsSubGyro()
{
}

void COptionsSubGyro::PerformLayout()
{
	BaseClass::PerformLayout();

	const int margin = 16;
	int fieldW = MIN( 300, GetWide() - margin * 2 );

	m_pGyroEnable->SetBounds( margin, 12, fieldW, 24 );
	m_pReverseX->SetBounds( margin, 44, fieldW, 24 );
	m_pReverseY->SetBounds( margin, 76, fieldW, 24 );

	m_pSensitivityPreLabel->SetBounds( margin, 116, fieldW, 24 );
	m_pSensitivitySlider->SetBounds( margin, 140, fieldW - 72, 32 );
	m_pSensitivityLabel->SetBounds( margin + fieldW - 64, 144, 64, 24 );
}

void COptionsSubGyro::UpdateSensitivityLabel()
{
	char buf[64];
	Q_snprintf( buf, sizeof( buf ), " %.2f", m_pSensitivitySlider->GetSliderValue() );
	m_pSensitivityLabel->SetText( buf );
}

void COptionsSubGyro::OnResetData()
{
	m_pGyroEnable->Reset();
	m_pReverseX->Reset();
	m_pReverseY->Reset();
	m_pSensitivitySlider->Reset();
	UpdateSensitivityLabel();
}

void COptionsSubGyro::OnApplyChanges()
{
	m_pGyroEnable->ApplyChanges();
	m_pReverseX->ApplyChanges();
	m_pReverseY->ApplyChanges();
	m_pSensitivitySlider->ApplyChanges();
}

void COptionsSubGyro::OnControlModified( Panel *panel )
{
	PostActionSignal( new KeyValues( "ApplyButtonEnable" ) );

	// the HasBeenModified() check keeps an out-of-range cvar value from being overwritten by the slider
	if ( panel == m_pSensitivitySlider && m_pSensitivitySlider->HasBeenModified() )
		UpdateSensitivityLabel();
}

void COptionsSubGyro::OnTextChanged( Panel *panel )
{
	if ( panel != m_pSensitivityLabel )
		return;

	char buf[64];
	m_pSensitivityLabel->GetText( buf, sizeof( buf ) );

	float fValue = 0.0f;
	if ( sscanf( buf, "%f", &fValue ) == 1 && fValue >= 0.0f )
	{
		m_pSensitivitySlider->SetSliderValue( fValue );
		PostActionSignal( new KeyValues( "ApplyButtonEnable" ) );
	}
}
