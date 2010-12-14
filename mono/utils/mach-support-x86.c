/*
 * mach-support-x86.c: mach support for x86
 *
 * Authors:
 *   Geoff Norton (gnorton@novell.com)
 *
 * (C) 2010 Ximian, Inc.
 */

#include <config.h>

#if defined(__MACH__)
#include <stdint.h>
#include <glib.h>
#include <pthread.h>
#include "utils/mono-sigcontext.h"
#include "mach-support.h"
#include <ucontext.h>

void *
mono_mach_arch_get_ip (thread_state_t state)
{
	x86_thread_state32_t *arch_state = (x86_thread_state32_t *) state;

#ifndef AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
	return (void *) arch_state->__eip;
#else
	return (void *) arch_state->eip;
#endif
}

void *
mono_mach_arch_get_sp (thread_state_t state)
{
	x86_thread_state32_t *arch_state = (x86_thread_state32_t *) state;

#ifndef AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER
	return (void *) arch_state->__esp;
#else
	return (void *) arch_state->esp;
#endif
}

int
mono_mach_arch_get_mcontext_size ()
{
#ifndef AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
	return sizeof (struct __darwin_mcontext32);
#else
	return I386_MCONTEXT_SIZE;
#endif
}

void
mono_mach_arch_thread_state_to_mcontext (thread_state_t state, mcontext_t context)
{
	x86_thread_state32_t *arch_state = (x86_thread_state32_t *) state;
#ifndef AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
	struct __darwin_mcontext32 *ctx = (struct __darwin_mcontext32 *) context;
#else
	struct mcontext *ctx = (struct mcontext *) context;
#endif

	ctx->ss = *arch_state;
}

int
mono_mach_arch_get_thread_state_size ()
{
	return sizeof (x86_thread_state32_t);
}

kern_return_t
mono_mach_arch_get_thread_state (thread_port_t thread, thread_state_t state, mach_msg_type_number_t *count)
{
	x86_thread_state32_t *arch_state = (x86_thread_state32_t *) state;
	kern_return_t ret;

	*count = x86_THREAD_STATE32_COUNT;

	ret = thread_get_state (thread, x86_THREAD_STATE32, (thread_state_t) arch_state, count);

	return ret;
}

void *
mono_mach_arch_get_tls_value_from_thread (thread_port_t thread, guint32 key)
{
	/* OSX stores TLS values in a hidden array inside the pthread_t structure
	 * They are keyed off a giant array offset 0x48 into the pointer.  This value
	 * is baked into their pthread_getspecific implementation
	 */
#ifndef AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
	intptr_t *p = (intptr_t *) pthread_from_mach_thread_np (thread);
#else
	// I assign p only to avoid warnings...
	intptr_t *p = NULL;
	g_error ("pthread_from_mach_thread_np unsupported on Mac 10.4");
#endif
	intptr_t **tsd = (intptr_t **) (p + 0x48);

	return (void *) tsd [key];
}
#endif
