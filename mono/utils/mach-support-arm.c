/*
 * mach-support-arm.c: mach support for ARM
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

void *
mono_mach_arch_get_ip (thread_state_t state)
{
#if defined (arm_thread_state_t)
	arm_thread_state_t *arch_state = (arm_thread_state_t *) state;

	return (void *) arch_state->__pc;
#else
	g_assert_not_reached ();
	return NULL;
#endif
}

void *
mono_mach_arch_get_sp (thread_state_t state)
{
#if defined (arm_thread_state_t)
	arm_thread_state_t *arch_state = (arm_thread_state_t *) state;

	return (void *) arch_state->__sp;
#else
	g_assert_not_reached ();
	return NULL;
#endif
}

#if defined (mcontext_t)
int
mono_mach_arch_get_mcontext_size ()
{
	return sizeof (struct __darwin_mcontext);
}

void
mono_mach_arch_thread_state_to_mcontext (thread_state_t state, mcontext_t context)
{
#if defined (arm_thread_state_t)
	arm_thread_state_t *arch_state = (arm_thread_state_t *) state;
	struct __darwin_mcontext *ctx = (struct __darwin_mcontext *) context;

	ctx->__ss = *arch_state;
#else
	g_assert_not_reached ();
#endif
}
#endif

int
mono_mach_arch_get_thread_state_size ()
{
#if defined (arm_thread_state_t)
	return sizeof (arm_thread_state_t);
#else
	g_assert_not_reached ();
	return 0;
#endif
}

kern_return_t
mono_mach_arch_get_thread_state (thread_port_t thread, thread_state_t state, mach_msg_type_number_t *count)
{
#if defined (arm_thread_state_t)
	arm_thread_state_t *arch_state = (arm_thread_state_t *) state;
	kern_return_t ret;

	*count = ARM_THREAD_STATE_COUNT;

	ret = thread_get_state (thread, ARM_THREAD_STATE_COUNT, (thread_state_t) arch_state, count);

	return ret;
#else
	g_assert_not_reached ();
	return (kern_return_t) 0;
#endif
}

void *
mono_mach_arch_get_tls_value_from_thread (thread_port_t thread, guint32 key)
{
	/* OSX stores TLS values in a hidden array inside the pthread_t structure
	 * They are keyed off a giant array offset 0x48 into the pointer.  This value
	 * is baked into their pthread_getspecific implementation
	 */
	intptr_t *p = (intptr_t *) pthread_from_mach_thread_np (thread);
	intptr_t **tsd = (intptr_t **) (p + 0x48);

	return (void *) tsd [key];
}
#endif
