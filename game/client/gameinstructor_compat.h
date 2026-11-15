//========= CSSO: compatibility layer for the game instructor port ============//
//
// The game instructor comes from CS:GO (Kisak-Strike), which has splitscreen and
// console storage. CSSO has neither: a single local player, no matchmaking title data.
//
//=============================================================================//
#ifndef GAMEINSTRUCTOR_COMPAT_H
#define GAMEINSTRUCTOR_COMPAT_H
#ifdef _WIN32
#pragma once
#endif

#ifndef MAX_SPLITSCREEN_PLAYERS
#define MAX_SPLITSCREEN_PLAYERS 1
#endif

#ifndef GET_ACTIVE_SPLITSCREEN_SLOT
#define GET_ACTIVE_SPLITSCREEN_SLOT() 0
#endif

#ifndef ACTIVE_SPLITSCREEN_PLAYER_GUARD
#define ACTIVE_SPLITSCREEN_PLAYER_GUARD( x ) ( (void)( x ) )
#endif

#ifndef ASSERT_LOCAL_PLAYER_RESOLVABLE
#define ASSERT_LOCAL_PLAYER_RESOLVABLE() ( (void)0 )
#endif

#ifndef XBX_GetNumGameUsers
#define XBX_GetNumGameUsers() 1
#endif

inline int GameInstructor_CountBitsSet( unsigned int v )
{
	int n = 0;
	for ( ; v; v >>= 1 )
		n += ( v & 1 );
	return n;
}
#define UTIL_CountNumBitsSet( x ) GameInstructor_CountBitsSet( x )

#ifndef GetClientMode
#define GetClientMode() g_pClientMode
#endif

// CS:GO copy protection / self check hooks: nothing to do in CSSO
#ifndef RANDOM_CEG_TEST_SECRET
#define RANDOM_CEG_TEST_SECRET()
#endif
#ifndef RANDOM_CEG_TEST_SECRET_PERIOD
#define RANDOM_CEG_TEST_SECRET_PERIOD( a, b ) ( (void)0 )
#endif
#ifndef STEAMWORKS_TESTSECRET_AMORTIZE
#define STEAMWORKS_TESTSECRET_AMORTIZE( x ) ( (void)0 )
#endif
#ifndef STEAMWORKS_SELFCHECK
#define STEAMWORKS_SELFCHECK() ( (void)0 )
#endif

#endif // GAMEINSTRUCTOR_COMPAT_H
