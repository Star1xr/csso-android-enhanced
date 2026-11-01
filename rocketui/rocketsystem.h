#ifndef CSSO_ROCKETSYSTEM_H
#define CSSO_ROCKETSYSTEM_H

#include <RmlUi/Core/SystemInterface.h>

class RocketSystem : public Rml::SystemInterface
{
public:
	static RocketSystem m_Instance;

	double GetElapsedTime() override;
	bool LogMessage( Rml::Log::Type type, const Rml::String &message ) override;
	void SetMouseCursor( const Rml::String &cursor_name ) override;
	void SetClipboardText( const Rml::String &text ) override;
	void GetClipboardText( Rml::String &text ) override;
};

#endif
