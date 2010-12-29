#ifndef __UNITY_MONO_UTILS_H
#define __UNITY_MONO_UTILS_H

#include <stdio.h>
#include <mono/metadata/object.h>

#include "config.h"
#if defined HOST_WIN32 && ! defined HANDLE
typedef void *HANDLE;
#endif

/**
 *	Custom exit function, called instead of system exit()
 */
#if ! defined (TARGET_WIN32)
void unity_mono_exit( int code );
#else
_CRTIMP __declspec(noreturn) void __cdecl unity_mono_exit(_In_ int code);
#endif

/**
 *	Redirects mono output where we want it.
 */
void unity_mono_redirect_output( HANDLE handle );

/**
 *	Closes redirected output files.
 */
void unity_mono_close_output();

extern MonoString* mono_unity_get_embeddinghostname();

void mono_unity_write_to_unity_log(MonoString* str);

#ifdef WIN32
FILE* unity_fopen( const char *name, const char *mode );
#endif

extern mono_bool mono_unity_socket_security_enabled_get ();
extern void mono_unity_socket_security_enabled_set (mono_bool enabled);

#endif
