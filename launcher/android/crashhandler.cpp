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
#include <unwind.h>
#include <android/log.h>
#include "tier0/dbg.h"
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <exception>
#include <cstdlib>
#include <typeinfo>
#include "libunwind/libunwind.h"

struct sigaction old_sa;

namespace __cxxabiv1
{
	extern "C" std::type_info *__cxa_current_exception_type();
}

extern "C" const char *GetAndroidLogDir();

static int g_CrashLogFd = -1;

static void CrashRawWrite( const char *msg )
{
	if( !msg )
		return;

	if( g_CrashLogFd >= 0 )
		write( g_CrashLogFd, msg, strlen( msg ) );

	__android_log_print( ANDROID_LOG_ERROR, "SRCENG", "%s", msg );
}

#define IN_LIBGCC2 1 // means we want to define __cxxabiv1::__cxa_demangle
namespace __cxxabiv1
{
	extern "C"
	{
		#include "demangle/cp-demangle.c"
	}
}

#define MAX_FRAMES 2048

struct backtrace_t
{
	int count;
	uintptr_t frames[MAX_FRAMES];
};

#define Log(msg) do { CrashRawWrite(msg); DebugLogger()->Write(msg); } while(0)

void printPC(void *pc)
{
	char message[4096];

	const char* symbol = "unknown";

	Dl_info info = { 0 };
	const char *fname = "unknown";

	if( dladdr(pc, &info) <= 0 )
	{
		snprintf( message, sizeof(message), "0x%" PRIXPTR "\n", (uintptr_t)pc );
		Log(message);
		return;
	}

	if( info.dli_fname )
		fname = info.dli_fname;

	if( info.dli_sname )
		symbol = info.dli_sname;

	int status = 0;
	char *demangled = __cxxabiv1::__cxa_demangle(symbol, 0, 0, &status);

	if( NULL != demangled && 0 == status )
		symbol = demangled;

	uintptr_t relative_addr = (uintptr_t)pc - (uintptr_t)info.dli_fbase;

	snprintf( message, sizeof(message), "0x%" PRIXPTR ":\t%s (base=0x%" PRIXPTR ", %s)\n", relative_addr, symbol, (uintptr_t)info.dli_fbase, fname );
	Log(message);
}

_Unwind_Reason_Code UnwindBacktraceCallback(struct _Unwind_Context* unwind_context, void* state_voidp)
{
	uintptr_t pc = _Unwind_GetIP(unwind_context);
	backtrace_t *bt = (backtrace_t*)state_voidp;

	if( bt->count < MAX_FRAMES )
		bt->frames[bt->count++] = pc;
	else
		return _URC_END_OF_STACK;

	return _URC_NO_REASON;
}

static void CrashHandler( int sig, siginfo_t *si, void *uc)
{
	static char message[4096], symbol[256];
	int len, line, logfd, i = 0;


	const ucontext_t* signal_ucontext = (ucontext_t*)uc;
	const mcontext_t* signal_mcontext = &(signal_ucontext->uc_mcontext);
#ifdef __aarch64__
	// Doesn't work good on armv7a
	static backtrace_t bt;
	bt.count = 0;

	_Unwind_Backtrace(UnwindBacktraceCallback, &bt);

	Log(">>> crash report begin\n");

	snprintf(message, sizeof(message), "Signal=%d, errno=%d, code=%d, addr=0x%" PRIXPTR "\n", sig, si->si_errno, si->si_code, (uintptr_t)si->si_addr);
	Log(message);

	for( int i = 0; i < bt.count; i++ )
	{
		printPC( (void*)bt.frames[i] );
	}
#else
	Log(">>> crash report begin\n");

	snprintf(message, sizeof(message), "Signal=%d, errno=%d, code=%d, addr=0x%" PRIXPTR "\n", sig, si->si_errno, si->si_code, (uintptr_t)si->si_addr);
	Log(message);

	// Initialize unw_context and unw_cursor.
	unw_context_t unw_context = {};
	unw_getcontext(&unw_context);
	unw_cursor_t  unw_cursor = {};
	unw_init_local(&unw_cursor, &unw_context);

	while (unw_step(&unw_cursor) > 0) {
		unw_word_t ip = 0;
		unw_get_reg(&unw_cursor, UNW_REG_IP, &ip);
		printPC( (void*)ip );
	}
#endif

	Log(">>> crash report end\n");

	if( g_CrashLogFd >= 0 )
		fsync( g_CrashLogFd );

	if (old_sa.sa_sigaction)
		(*old_sa.sa_sigaction)(sig, si, uc);
	else if(old_sa.sa_handler)
		(*old_sa.sa_handler)(sig);
}

static void TerminateHandler()
{
	CrashRawWrite( ">>> std::terminate called\n" );

	std::type_info *type = __cxxabiv1::__cxa_current_exception_type();
	if( type )
	{
		int status = 0;
		char *demangled = __cxxabiv1::__cxa_demangle( type->name(), 0, 0, &status );
		char message[2048];
		snprintf( message, sizeof(message), ">>> uncaught exception: %s\n",
			( demangled && status == 0 ) ? demangled : type->name() );
		CrashRawWrite( message );
		free( demangled );
	}
	else
	{
		CrashRawWrite( ">>> uncaught exception: unknown (no active exception)\n" );
	}

	if( g_CrashLogFd >= 0 )
		fsync( g_CrashLogFd );

	std::abort();
}

void InitCrashHandler()
{
	const char *logDir = GetAndroidLogDir();
	char crashLog[2048];
	snprintf( crashLog, sizeof(crashLog), "%s/crash.log", ( logDir && logDir[0] ) ? logDir : "." );

	g_CrashLogFd = open( crashLog, O_CREAT | O_WRONLY | O_APPEND, 0666 );
	if( g_CrashLogFd >= 0 )
	{
		char header[2048];
		snprintf( header, sizeof(header), ">>> crash handler installed, log=%s\n", crashLog );
		CrashRawWrite( header );
	}
	else
	{
		__android_log_print( ANDROID_LOG_ERROR, "SRCENG", "crash handler: cannot open %s: %s", crashLog, strerror( errno ) );
	}

	std::set_terminate( TerminateHandler );

	struct sigaction act;
	memset( &act, 0, sizeof(act) );
	act.sa_sigaction = CrashHandler;
	act.sa_flags = SA_SIGINFO | SA_ONSTACK;
	sigaction(SIGSEGV, &act, &old_sa);
	sigaction(SIGABRT, &act, &old_sa);
	sigaction(SIGBUS, &act, &old_sa);
	sigaction(SIGFPE, &act, &old_sa);
	sigaction(SIGTRAP, &act, &old_sa);
	sigaction(SIGILL, &act, &old_sa);
	sigaction(SIGSYS, &act, &old_sa);
}
