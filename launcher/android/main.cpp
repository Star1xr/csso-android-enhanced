/*
Copyright (C) 2022 nillerusr

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <android/log.h>
#include <SDL_hints.h>
#include "tier0/dbg.h"
#include "tier0/threadtools.h"

char *LauncherArgv[512];
char java_args[4096];
int iLastArgs = 0;

extern void InitCrashHandler();
DLL_EXPORT int LauncherMain( int argc, char **argv ); // from launcher.cpp

DLL_EXPORT int Java_com_valvesoftware_ValveActivity2_setenv(JNIEnv *jenv, jclass *jclass, jstring env, jstring value, jint over)
{
	Msg( "Java_com_valvesoftware_ValveActivity2_setenv %s=%s\n", jenv->GetStringUTFChars(env, NULL), jenv->GetStringUTFChars(value, NULL) );
	return setenv( jenv->GetStringUTFChars(env, NULL), jenv->GetStringUTFChars(value, NULL), over );
}

DLL_EXPORT void Java_com_valvesoftware_ValveActivity2_nativeOnActivityResult()
{
//	Msg( "Java_com_valvesoftware_ValveActivity_nativeOnActivityResult\n" );
}

DLL_EXPORT void Java_com_valvesoftware_ValveActivity2_setArgs(JNIEnv *env, jclass *clazz, jstring str)
{
	strncpy( java_args, env->GetStringUTFChars(str, NULL), sizeof java_args );
}

void SetLauncherArgs()
{
#define A(a,b) LauncherArgv[iLastArgs++] = (char*)a; \
	LauncherArgv[iLastArgs++] = (char*)b
#define D(a) LauncherArgv[iLastArgs++] = (char*)a

	static char binPath[2048];
	snprintf(binPath, sizeof binPath, "%s/hl2_linux", getenv("APP_DATA_PATH") );
	D(binPath);

	D("-nouserclip");

	char *pch;

	pch = strtok (java_args," ");
	while (pch != NULL)
	{
		LauncherArgv[iLastArgs++] = pch;
		pch = strtok (NULL, " ");
	}

	D("-fullscreen");
	D("-nosteam");
	D("-insecure");

#undef A
#undef D
}

float GetTotalMemory()
{
	int64_t mem = 0;

	char meminfo[8196] = { 0 };
	FILE *f = fopen("/proc/meminfo", "r");
	if( !f )
		return 0.f;

	size_t size = fread(meminfo, 1, sizeof(meminfo), f);
	if( !size )
		return 0.f;

	char *s = strstr(meminfo, "MemTotal:");

	if( !s ) return 0.f;

	sscanf(s+9, "%lld", &mem);
	fclose(f);

	return mem/1024/1024.f;
}

void android_property_print(const char *name)
{
	char prop[1024];

	char strValue[64];
	memset (strValue, 0, 64);
	snprintf(prop, sizeof(prop), "getprop %s", name);
	FILE *fp = NULL;
	fp = popen(prop, "r");
	if (!fp) return;

	fgets(strValue, sizeof(strValue), fp);
	pclose(fp);
	fp = NULL;

	Msg("prop %s=%s", name, strValue);
}


static char g_AndroidLogDir[2048];
static int g_AndroidStdoutFd = -1;

extern "C" const char *GetAndroidLogDir()
{
	return g_AndroidLogDir;
}

static bool TryAndroidLogDir( const char *dir )
{
	if( !dir || !dir[0] )
		return false;

	char probe[2048];
	snprintf( probe, sizeof(probe), "%s", dir );

	size_t len = strlen( probe );
	while( len > 1 && probe[len - 1] == '/' )
		probe[--len] = 0;

	if( mkdir( probe, 0777 ) != 0 && errno != EEXIST )
		return false;

	struct stat st;
	if( stat( probe, &st ) != 0 || !S_ISDIR( st.st_mode ) )
		return false;

	char testPath[2048];
	snprintf( testPath, sizeof(testPath), "%s/.write_test", probe );
	int testFd = open( testPath, O_CREAT | O_WRONLY | O_APPEND, 0666 );
	if( testFd < 0 )
		return false;
	close( testFd );
	unlink( testPath );

	snprintf( g_AndroidLogDir, sizeof(g_AndroidLogDir), "%s", probe );
	return true;
}

static void AndroidLogInit()
{
	char candidate[2048];
	const char *appLog = getenv( "APP_LOG_PATH" );
	const char *gamePath = getenv( "VALVE_GAME_PATH" );
	const char *appData = getenv( "APP_DATA_PATH" );

	bool found = false;

	if( appLog && appLog[0] )
		found = TryAndroidLogDir( appLog );

	if( !found && gamePath && gamePath[0] )
	{
		snprintf( candidate, sizeof(candidate), "%s/csso_logs", gamePath );
		found = TryAndroidLogDir( candidate );
	}

	if( !found && appData && appData[0] )
	{
		snprintf( candidate, sizeof(candidate), "%s/csso_logs", appData );
		found = TryAndroidLogDir( candidate );
	}

	if( !found )
	{
		// Last resort. The directory may not be writable, but keep the path
		// in the logs so we know where we tried to write.
		snprintf( g_AndroidLogDir, sizeof(g_AndroidLogDir), "%s", appData ? appData : "." );
	}

	char engineLog[2048];
	char stdoutLog[2048];
	snprintf( engineLog, sizeof(engineLog), "%s/engine.log", g_AndroidLogDir );
	snprintf( stdoutLog, sizeof(stdoutLog), "%s/stdout.log", g_AndroidLogDir );

	// Capture raw stdout/stderr too. SDL, the Vulkan loader and DXVK write
	// important startup failures there, not through Msg().
	g_AndroidStdoutFd = open( stdoutLog, O_CREAT | O_WRONLY | O_APPEND, 0666 );
	if( g_AndroidStdoutFd >= 0 )
	{
		dup2( g_AndroidStdoutFd, STDOUT_FILENO );
		dup2( g_AndroidStdoutFd, STDERR_FILENO );
		setvbuf( stdout, NULL, _IONBF, 0 );
		setvbuf( stderr, NULL, _IONBF, 0 );
	}

	DebugLogger()->Init( engineLog );

	Msg( ">>> Android log directory: %s\n", g_AndroidLogDir );
	Msg( ">>> engine.log: %s\n", engineLog );
	Msg( ">>> stdout.log: %s\n", stdoutLog );
	Msg( ">>> log writable: %s\n", found ? "yes" : "no" );
}

DLL_EXPORT int LauncherMainAndroid( int argc, char **argv )
{
	// Open engine.log before anything else. The normal engine logger is only
	// initialized in CSourceAppSystemGroup::PreInit(), which is far too late:
	// every Msg() before that is buffered in memory and lost on an early exit.
	AndroidLogInit();

	InitCrashHandler();
	Msg( ">>> crash handler installed\n" );

	Msg( ">>> LauncherMainAndroid( argc=%d )\n", argc );
	for( int i = 0; i < argc; i++ )
		Msg( ">>> argv[%d] = '%s'\n", i, argv[i] ? argv[i] : "(null)" );

	char cwd[2048];
	if( getcwd( cwd, sizeof(cwd) ) )
		Msg( ">>> cwd = %s\n", cwd );

	const char *envNames[] = {
		"APP_DATA_PATH", "APP_LIB_PATH", "VALVE_GAME_PATH", "APP_LOG_PATH",
		"EXTRAS_VPK_PATH", "LANG", "DXVK_LOG_PATH", "DXVK_LOG_LEVEL"
	};
	for( size_t i = 0; i < sizeof(envNames) / sizeof(envNames[0]); i++ )
	{
		const char *value = getenv( envNames[i] );
		Msg( ">>> env %s = %s\n", envNames[i], value ? value : "(null)" );
	}

	const char *libPath = getenv( "APP_LIB_PATH" );
	if( libPath && libPath[0] )
	{
		const char *libs[] = {
			"liblauncher.so", "libSDL2.so", "libtier0.so", "libtier1.so",
			"libshaderapidx9.so", "libdxvk_d3d9.so"
		};
		for( size_t i = 0; i < sizeof(libs) / sizeof(libs[0]); i++ )
		{
			char path[2048];
			snprintf( path, sizeof(path), "%s/%s", libPath, libs[i] );
			struct stat st;
			if( stat( path, &st ) == 0 )
				Msg( ">>> lib %s: present (%lld bytes)\n", libs[i], (long long)st.st_size );
			else
				Msg( ">>> lib %s: MISSING (%s)\n", libs[i], strerror( errno ) );
		}
	}

	Msg("GetTotalMemory() = %.2f \n", GetTotalMemory());

	android_property_print("ro.build.version.sdk");
	android_property_print("ro.product.device");
	android_property_print("ro.product.manufacturer");
	android_property_print("ro.product.model");
	android_property_print("ro.product.name");

	SetLauncherArgs();

	Msg( ">>> LauncherMain args (%d):\n", iLastArgs );
	for( int i = 0; i < iLastArgs; i++ )
		Msg( ">>>   [%d] '%s'\n", i, LauncherArgv[i] ? LauncherArgv[i] : "(null)" );

	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
	DeclareCurrentThreadIsMainThread(); // Init thread propertly on Android

	int ret = LauncherMain(iLastArgs, LauncherArgv);
	Msg( ">>> LauncherMain returned %d\n", ret );
	return ret;
}
