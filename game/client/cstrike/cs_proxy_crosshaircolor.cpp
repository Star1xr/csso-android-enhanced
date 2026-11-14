//========= "CrosshairColor" material proxy ==================================//
//
// Purpose: CS:GO materials (scope dot) use a proxy named "CrosshairColor" that writes the
//          crosshair colour into a material var ("resultVar", usually $color).
//
//=============================================================================//

#include "cbase.h"
#include "functionproxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar cl_crosshaircolor;
extern ConVar cl_crosshaircolor_r;
extern ConVar cl_crosshaircolor_g;
extern ConVar cl_crosshaircolor_b;

class CCrosshairColorProxy : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CCrosshairColorProxy::OnBind( void *pC_BaseEntity )
{
	if ( !m_pResult )
		return;

	// same table as the HUD crosshair (weapon_csbase.cpp)
	int r, g, b;
	switch ( cl_crosshaircolor.GetInt() )
	{
		case 0:	r = 250;	g = 50;		b = 50;		break;
		case 1:	r = 50;		g = 250;	b = 50;		break;
		case 2:	r = 250;	g = 250;	b = 50;		break;
		case 3:	r = 50;		g = 50;		b = 250;	break;
		case 4:	r = 50;		g = 250;	b = 250;	break;
		case 5:
			r = cl_crosshaircolor_r.GetInt();
			g = cl_crosshaircolor_g.GetInt();
			b = cl_crosshaircolor_b.GetInt();
			break;
		default: r = 50;	g = 250;	b = 50;		break;
	}

	float v[3] = { r / 255.0f, g / 255.0f, b / 255.0f };
	if ( m_pResult->GetType() == MATERIAL_VAR_TYPE_VECTOR )
		m_pResult->SetVecValue( v, MIN( 3, m_pResult->VectorSize() ) );
	else
		m_pResult->SetFloatValue( v[1] );
}

EXPOSE_INTERFACE( CCrosshairColorProxy, IMaterialProxy, "CrosshairColor" IMATERIAL_PROXY_INTERFACE_VERSION );
