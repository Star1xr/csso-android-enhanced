#include "rocketfilesystem.h"

#include "tier0/platform.h"
#include "filesystem.h"
#include "tier2/tier2.h"

RocketFileSystem RocketFileSystem::m_Instance;

Rml::FileHandle RocketFileSystem::Open( const Rml::String &path )
{
	// "rocketui/" + path; no "csgo/" or "csso/" anywhere, the GAME path decides the mod dir.
	Rml::String rocketPath = "rocketui/";
	rocketPath += path;
	return (Rml::FileHandle)g_pFullFileSystem->Open( rocketPath.c_str(), "rb", "GAME" );
}

void RocketFileSystem::Close( Rml::FileHandle file )
{
	g_pFullFileSystem->Close( (FileHandle_t)file );
}

size_t RocketFileSystem::Read( void *buffer, size_t size, Rml::FileHandle file )
{
	return g_pFullFileSystem->Read( buffer, size, (FileHandle_t)file );
}

bool RocketFileSystem::Seek( Rml::FileHandle file, long offset, int origin )
{
	FileSystemSeek_t seek = FILESYSTEM_SEEK_HEAD;
	if ( origin == SEEK_CUR )
		seek = FILESYSTEM_SEEK_CURRENT;
	else if ( origin == SEEK_END )
		seek = FILESYSTEM_SEEK_TAIL;
	g_pFullFileSystem->Seek( (FileHandle_t)file, offset, seek );
	return true;
}

size_t RocketFileSystem::Tell( Rml::FileHandle file )
{
	return g_pFullFileSystem->Tell( (FileHandle_t)file );
}

size_t RocketFileSystem::Length( Rml::FileHandle file )
{
	return g_pFullFileSystem->Size( (FileHandle_t)file );
}
