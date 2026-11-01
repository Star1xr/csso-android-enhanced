#ifndef CSSO_ROCKETFILESYSTEM_H
#define CSSO_ROCKETFILESYSTEM_H

#include <RmlUi/Core/FileInterface.h>

// Reads everything RmlUi asks for from <game>/rocketui/ (i.e. csso/rocketui/) via the "GAME" search path.
class RocketFileSystem : public Rml::FileInterface
{
public:
	static RocketFileSystem m_Instance;

	Rml::FileHandle Open( const Rml::String &path ) override;
	void Close( Rml::FileHandle file ) override;
	size_t Read( void *buffer, size_t size, Rml::FileHandle file ) override;
	bool Seek( Rml::FileHandle file, long offset, int origin ) override;
	size_t Tell( Rml::FileHandle file ) override;
	size_t Length( Rml::FileHandle file ) override;
};

#endif
