//========= Gyroscope options page ==========================================//
#ifndef OPTIONSSUBGYRO_H
#define OPTIONSSUBGYRO_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>

class CCvarToggleCheckButton;
class CCvarSlider;

namespace vgui
{
	class Label;
	class TextEntry;
}

//-----------------------------------------------------------------------------
// Purpose: gyroscope options (cvars from gyroscope.cpp). The layout is built in code,
//			so no .res file is needed.
//-----------------------------------------------------------------------------
class COptionsSubGyro : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( COptionsSubGyro, vgui::PropertyPage );

public:
	COptionsSubGyro( vgui::Panel *parent );
	~COptionsSubGyro();

protected:
	virtual void OnResetData();
	virtual void OnApplyChanges();
	virtual void PerformLayout();

	MESSAGE_FUNC_PTR( OnControlModified, "ControlModified", panel );
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel )
	{
		OnControlModified( panel );
	}

private:
	void UpdateSensitivityLabel();

	CCvarToggleCheckButton	*m_pGyroEnable;
	CCvarToggleCheckButton	*m_pReverseX;
	CCvarToggleCheckButton	*m_pReverseY;
	vgui::Label				*m_pSensitivityPreLabel;
	CCvarSlider				*m_pSensitivitySlider;
	vgui::TextEntry			*m_pSensitivityLabel;
};

#endif // OPTIONSSUBGYRO_H
