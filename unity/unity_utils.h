#ifndef __UNITY_MONO_UTILS_H
#define __UNITY_MONO_UTILS_H

#include <stdio.h>
#include <mono/metadata/object.h>

/**
 *	Custom exit function, called instead of system exit()
 */
void unity_mono_exit( int code );

/**
 *	Redirects mono output where we want it.
 */
void unity_mono_redirect_output( const char *fout, const char *ferr );

/**
 *	Closes redirected output files.
 */
void unity_mono_close_output(void);

void mono_unity_set_embeddinghostname(const char* name);
extern MonoString* mono_unity_get_embeddinghostname(void);

void mono_unity_write_to_unity_log(MonoString* str);

mono_bool mono_unity_class_is_interface (MonoClass* klass);

#ifdef WIN32
FILE* unity_fopen( const char *name, const char *mode );
#endif

void mono_unity_g_free (void *ptr);
#endif
