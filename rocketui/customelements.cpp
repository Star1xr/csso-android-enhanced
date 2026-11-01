#include "customelements.h"

#include "tier0/dbg.h"
#include "tier1/strtools.h"
#include "tier3/tier3.h"
#include "vgui/ISystem.h"

#include <stdlib.h>

//-----------------------------------------------------------------------------
// base
//-----------------------------------------------------------------------------
bool KisakConvarSetting::ParseBasicAttributes( const Rml::XMLAttributes &attributes )
{
	const Rml::String desc = Rml::Get( attributes, "desc", Rml::String( "No Description" ) );
	const Rml::String name = Rml::Get( attributes, "convar", Rml::String( "rocket_rml_error_no_convar" ) );

	m_refConvar.Init( name.c_str(), true ); // ignoreMissing: a typo in an .rml must not spam the console
	m_bValid = m_refConvar.IsValid();

	if ( !m_bValid )
		SetInnerRML( "<b>" + desc + "</b> (unavailable)<br/>" );
	else
		SetInnerRML( "<b>" + desc + "</b><br/>" );
	return m_bValid;
}

static Rml::XMLAttributes MakeAttrs()
{
	return Rml::XMLAttributes();
}

//-----------------------------------------------------------------------------
// enum (dropdown)
//-----------------------------------------------------------------------------
KisakConvarSettingEnum::~KisakConvarSettingEnum()
{
	if ( m_pElemSelect )
		m_pElemSelect->RemoveEventListener( Rml::EventId::Change, this );
}

void KisakConvarSettingEnum::Build( const Rml::XMLAttributes &attributes )
{
	if ( !ParseBasicAttributes( attributes ) )
		return;

	m_enumValuesCount = Rml::Get( attributes, "enum-values-count", 0 );
	if ( m_enumValuesCount > ROCKET_MAX_ENUM_VALUES )
		m_enumValuesCount = ROCKET_MAX_ENUM_VALUES;

	Rml::ElementPtr selectPtr = Rml::Factory::InstanceElement( this, "select", "select", MakeAttrs() );
	if ( !selectPtr )
		return;
	m_pElemSelect = static_cast<Rml::ElementFormControlSelect *>( selectPtr.get() );
	AppendChild( std::move( selectPtr ) );

	const int current = m_refConvar.GetInt();
	bool foundValue = false;
	for ( int i = 0; i < m_enumValuesCount; i++ )
	{
		char attr[64];
		V_snprintf( attr, sizeof( attr ), "enum-value-%d", i );
		m_enumValues[i] = Rml::Get( attributes, attr, 0 );

		V_snprintf( attr, sizeof( attr ), "enum-name-%d", i );
		const Rml::String name = Rml::Get( attributes, attr, Rml::String( "RML ERROR! UNKNOWN_ENUM_NAME" ) );

		m_pElemSelect->Add( name, Rml::ToString( m_enumValues[i] ) );
		if ( m_enumValues[i] == current )
		{
			foundValue = true;
			m_pElemSelect->SetSelection( i );
		}
	}

	// The convar has a value that isn't listed in the .rml: show a "Choose..." entry instead of lying.
	if ( !foundValue )
	{
		m_pElemSelect->Add( "Choose...", "none" );
		m_pElemSelect->SetSelection( m_enumValuesCount );
	}

	// After the initial selection, otherwise setting it would write the convar back.
	m_pElemSelect->AddEventListener( Rml::EventId::Change, this );
}

void KisakConvarSettingEnum::ProcessEvent( Rml::Event &event )
{
	if ( !m_bValid )
		return;
	const Rml::String value = event.GetParameter<Rml::String>( "value", "none" );
	if ( value.empty() || value == "none" )
		return;

	const int iValue = atoi( value.c_str() );
	if ( iValue == m_refConvar.GetInt() )
		return;

	DevMsg( "RocketUI: %s -> %d\n", m_refConvar.GetName(), iValue );
	m_refConvar.SetValue( iValue );
}

//-----------------------------------------------------------------------------
// slider
//-----------------------------------------------------------------------------
KisakConvarSettingSlider::~KisakConvarSettingSlider()
{
	if ( m_pElemSlider )
		m_pElemSlider->RemoveEventListener( Rml::EventId::Change, this );
}

void KisakConvarSettingSlider::Build( const Rml::XMLAttributes &attributes )
{
	if ( !ParseBasicAttributes( attributes ) )
		return;

	m_sliderMin = Rml::Get( attributes, "slider-min", 0.0f );
	m_sliderMax = Rml::Get( attributes, "slider-max", 1.0f );
	const float step = Rml::Get( attributes, "slider-step", 0.0f );

	Rml::XMLAttributes inputAttrs = MakeAttrs();
	// "type" has to be there when the element is made, it picks the input subtype.
	inputAttrs["type"] = Rml::String( "range" );
	inputAttrs["min"] = Rml::ToString( m_sliderMin );
	inputAttrs["max"] = Rml::ToString( m_sliderMax );
	inputAttrs["step"] = Rml::ToString( step );
	inputAttrs["value"] = Rml::ToString( m_refConvar.GetFloat() );

	Rml::ElementPtr sliderPtr = Rml::Factory::InstanceElement( this, "input", "input", inputAttrs );
	if ( !sliderPtr )
		return;
	m_pElemSlider = sliderPtr.get();
	AppendChild( std::move( sliderPtr ) );
	m_pElemSlider->AddEventListener( Rml::EventId::Change, this );
}

void KisakConvarSettingSlider::ProcessEvent( Rml::Event &event )
{
	if ( !m_bValid )
		return;
	const Rml::String value = event.GetParameter<Rml::String>( "value", "" );
	if ( value.empty() )
		return;

	const float fValue = (float)atof( value.c_str() );
	if ( fValue < m_sliderMin || fValue > m_sliderMax )
		return;
	m_refConvar.SetValue( fValue );
}

//-----------------------------------------------------------------------------
// url
//-----------------------------------------------------------------------------
KisakURL::~KisakURL()
{
	RemoveEventListener( Rml::EventId::Click, this );
}

void KisakURL::ProcessEvent( Rml::Event & )
{
	if ( g_pVGuiSystem && !m_URL.empty() )
		g_pVGuiSystem->ShellExecute( "open", m_URL.c_str() );
}

//-----------------------------------------------------------------------------
// instancers
//-----------------------------------------------------------------------------
class KisakConvarSettingEnumInstancer : public Rml::ElementInstancer
{
public:
	Rml::ElementPtr InstanceElement( Rml::Element *, const Rml::String &tag, const Rml::XMLAttributes &attributes ) override
	{
		KisakConvarSettingEnum *e = new KisakConvarSettingEnum( tag );
		Rml::ElementPtr ptr( e );
		e->Build( attributes );
		return ptr;
	}
	void ReleaseElement( Rml::Element *element ) override { delete element; }
};

class KisakConvarSettingSliderInstancer : public Rml::ElementInstancer
{
public:
	Rml::ElementPtr InstanceElement( Rml::Element *, const Rml::String &tag, const Rml::XMLAttributes &attributes ) override
	{
		KisakConvarSettingSlider *e = new KisakConvarSettingSlider( tag );
		Rml::ElementPtr ptr( e );
		e->Build( attributes );
		return ptr;
	}
	void ReleaseElement( Rml::Element *element ) override { delete element; }
};

class KisakURLInstancer : public Rml::ElementInstancer
{
public:
	Rml::ElementPtr InstanceElement( Rml::Element *, const Rml::String &tag, const Rml::XMLAttributes &attributes ) override
	{
		KisakURL *e = new KisakURL( tag );
		Rml::ElementPtr ptr( e );

		e->m_URL = Rml::Get( attributes, "url", Rml::String() );
		for ( size_t i = e->m_URL.find( ' ' ); i != Rml::String::npos; i = e->m_URL.find( ' ', i ) )
			e->m_URL.erase( i, 1 );

		e->SetProperty( "color", "#4aa3ff" );
		e->SetProperty( "cursor", "pointer" );
		e->SetInnerRML( "<b>" + Rml::Get( attributes, "text", e->m_URL ) + "</b>" );
		e->AddEventListener( Rml::EventId::Click, e );
		return ptr;
	}
	void ReleaseElement( Rml::Element *element ) override { delete element; }
};

static KisakConvarSettingEnumInstancer s_enumInstancer;
static KisakConvarSettingSliderInstancer s_sliderInstancer;
static KisakURLInstancer s_urlInstancer;

void RocketUI_RegisterCustomElements()
{
	Rml::Factory::RegisterElementInstancer( "KisakConvarSettingEnum", &s_enumInstancer );
	Rml::Factory::RegisterElementInstancer( "KisakConvarSettingSlider", &s_sliderInstancer );
	Rml::Factory::RegisterElementInstancer( "KisakURL", &s_urlInstancer );
}
