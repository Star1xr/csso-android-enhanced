// Custom RmlUi elements for the CS:SO RocketUI menus (ported from Kisak-Strike rocketui/customelements).
// Differences: RmlUi 5.1 API (children are made with Factory::InstanceElement, no dummy instancer hack),
// ConVarRef is a member (no new/delete), unknown convars are shown as "unavailable" instead of crashing.
//
//   <KisakConvarSettingEnum   convar="mat_picmip" desc="..." enum-values-count="N" enum-value-0=".." enum-name-0=".." .../>
//   <KisakConvarSettingSlider convar="sensitivity" desc="..." slider-min=".." slider-max=".." slider-step=".."/>
//   <KisakURL url="https://..." text="..."/>
//
// The tag names are kept so Kisak's .rml files keep working.
#ifndef CSSO_ROCKETUI_CUSTOMELEMENTS_H
#define CSSO_ROCKETUI_CUSTOMELEMENTS_H

#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

#include "tier1/convar.h"

#define ROCKET_MAX_ENUM_VALUES 32

class KisakConvarSetting : public Rml::Element
{
public:
	KisakConvarSetting( const Rml::String &tag ) : Rml::Element( tag ), m_refConvar( "rocket_rml_error_no_convar", true ) {}

	// Reads convar + desc, adds "<b>desc</b><br/>" as the first children. Returns false if the convar doesn't exist.
	bool ParseBasicAttributes( const Rml::XMLAttributes &attributes );

	ConVarRef m_refConvar;
	bool m_bValid = false;
};

class KisakConvarSettingEnum : public KisakConvarSetting, public Rml::EventListener
{
public:
	KisakConvarSettingEnum( const Rml::String &tag ) : KisakConvarSetting( tag ) {}
	~KisakConvarSettingEnum() override;
	void ProcessEvent( Rml::Event &event ) override;

	void Build( const Rml::XMLAttributes &attributes );

	int m_enumValues[ROCKET_MAX_ENUM_VALUES];
	int m_enumValuesCount = 0;
	Rml::ElementFormControlSelect *m_pElemSelect = nullptr; // owned by this element's DOM
};

class KisakConvarSettingSlider : public KisakConvarSetting, public Rml::EventListener
{
public:
	KisakConvarSettingSlider( const Rml::String &tag ) : KisakConvarSetting( tag ) {}
	~KisakConvarSettingSlider() override;
	void ProcessEvent( Rml::Event &event ) override;

	void Build( const Rml::XMLAttributes &attributes );

	float m_sliderMin = 0.0f;
	float m_sliderMax = 0.0f;
	Rml::Element *m_pElemSlider = nullptr; // owned by this element's DOM
};

class KisakURL : public Rml::Element, public Rml::EventListener
{
public:
	KisakURL( const Rml::String &tag ) : Rml::Element( tag ) {}
	~KisakURL() override;
	void ProcessEvent( Rml::Event &event ) override;

	Rml::String m_URL;
};

// Registers the three instancers (static instancers, RmlUi doesn't take ownership). Call after Rml::Initialise().
void RocketUI_RegisterCustomElements();

#endif
