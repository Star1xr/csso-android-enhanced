#include "rocketsystem.h"
#include "rocketuiimpl.h"

#include "tier0/dbg.h"
#include "tier3/tier3.h"
#include "vgui/ISystem.h"

RocketSystem RocketSystem::m_Instance;

double RocketSystem::GetElapsedTime()
{
	return (double)RocketUIImpl::m_Instance.GetTime();
}

bool RocketSystem::LogMessage( Rml::Log::Type type, const Rml::String &message )
{
	switch ( type )
	{
	case Rml::Log::LT_ERROR:
		Warning( "[RocketUI] ERROR: %s\n", message.c_str() );
		break;
	case Rml::Log::LT_WARNING:
		Warning( "[RocketUI] %s\n", message.c_str() );
		break;
	default:
		DevMsg( "[RocketUI] %s\n", message.c_str() );
		break;
	}
	return true; // never break into a debugger
}

void RocketSystem::SetMouseCursor( const Rml::String & )
{
	// Touch / SDL cursor handling lives in the engine; nothing to do on Android.
}

void RocketSystem::SetClipboardText( const Rml::String &text )
{
	if ( g_pVGuiSystem )
		g_pVGuiSystem->SetClipboardText( text.c_str(), (int)text.size() );
}

void RocketSystem::GetClipboardText( Rml::String &text )
{
	text.clear();
	if ( !g_pVGuiSystem )
		return;
	char buffer[1024];
	int len = g_pVGuiSystem->GetClipboardTextCount();
	if ( len <= 0 )
		return;
	if ( len > (int)sizeof( buffer ) - 1 )
		len = (int)sizeof( buffer ) - 1;
	g_pVGuiSystem->GetClipboardText( 0, buffer, len + 1 );
	buffer[len] = 0;
	text = buffer;
}
