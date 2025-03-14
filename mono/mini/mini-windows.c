/**
 * \file
 * POSIX signal handling support for Mono.
 *
 * Authors:
 *   Mono Team (mono-list@lists.ximian.com)
 *
 * Copyright 2001-2003 Ximian, Inc.
 * Copyright 2003-2008 Ximian, Inc.
 *
 * See LICENSE for licensing information.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
#include <config.h>
#include <signal.h>
#include <math.h>
#include <conio.h>
#include <assert.h>

#include <mono/metadata/coree.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/loader.h>
#include <mono/metadata/tabledefs.h>
#include <mono/metadata/class.h>
#include <mono/metadata/object.h>
#include <mono/metadata/tokentype.h>
#include <mono/metadata/tabledefs.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/profiler-private.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/gc-internals.h>
#include <mono/metadata/threads-types.h>
#include <mono/metadata/verify.h>
#include <mono/metadata/verify-internals.h>
#include <mono/metadata/mempool-internals.h>
#include <mono/metadata/attach.h>
#include <mono/utils/mono-math.h>
#include <mono/utils/mono-compiler.h>
#include <mono/utils/mono-counters.h>
#include <mono/utils/mono-logger-internals.h>
#include <mono/utils/mono-mmap.h>
#include <mono/utils/mono-state.h>
#include <mono/utils/dtrace.h>
#include <mono/utils/mono-context.h>
#include <mono/utils/w32subset.h>
#include "mono/utils/mono-tls-inline.h"

#include "mini.h"
#include "mini-runtime.h"
#include "mini-windows.h"
#include <string.h>
#include <ctype.h>
#include "trace.h"
#include <windows.h>
#include <signal.h>

#include "jit-icalls.h"

#define MONO_HANDLER_DELIMITER ','
#define MONO_HANDLER_DELIMITER_LEN G_N_ELEMENTS(MONO_HANDLER_DELIMITER)-1

#define MONO_HANDLER_ATEXIT_WAIT_KEYPRESS "atexit-waitkeypress"
#define MONO_HANDLER_ATEXIT_WAIT_KEYPRESS_LEN G_N_ELEMENTS(MONO_HANDLER_ATEXIT_WAIT_KEYPRESS)-1

// Typedefs used to setup handler table.
typedef void (*handler)(void);

typedef struct {
	const char * cmd;
	const int cmd_len;
	handler handler;
} HandlerItem;

#if _WIN64
typedef void MONO_SIG_HANDLER_SIGNATURE ((*MonoW32ExceptionHandler));
void win32_seh_init(void);
void win32_seh_cleanup(void);
void win32_seh_set_handler(int type, MonoW32ExceptionHandler handler);

static void (*restore_stack) (void);
static MonoW32ExceptionHandler fpe_handler;
static MonoW32ExceptionHandler ill_handler;
static MonoW32ExceptionHandler segv_handler;

LPTOP_LEVEL_EXCEPTION_FILTER mono_old_win_toplevel_exception_filter;
void *mono_win_vectored_exception_handle;

#define W32_SEH_HANDLE_EX(_ex) \
	if (_ex##_handler) _ex##_handler(er->ExceptionCode, &info, ctx)

static LONG CALLBACK seh_unhandled_exception_filter(EXCEPTION_POINTERS* ep)
{
#ifndef MONO_CROSS_COMPILE
	if (mono_old_win_toplevel_exception_filter) {
		return (*mono_old_win_toplevel_exception_filter)(ep);
	}
#endif

	if (mono_dump_start ())
		mono_handle_native_crash (mono_get_signame (SIGSEGV), NULL, NULL);

	return EXCEPTION_CONTINUE_SEARCH;
}

#if HAVE_API_SUPPORT_WIN32_RESET_STKOFLW && !TARGET_ARM64
static gpointer
get_win32_restore_stack (void)
{
	static guint8 *start = NULL;
	guint8 *code;

	if (start)
		return start;

	const int size = 128;

	/* restore_stack (void) */
	start = code = mono_global_codeman_reserve (size);

	amd64_push_reg (code, AMD64_RBP);
	amd64_mov_reg_reg (code, AMD64_RBP, AMD64_RSP, 8);

	/* push 32 bytes of stack space for Win64 calling convention */
	amd64_alu_reg_imm (code, X86_SUB, AMD64_RSP, 32);

	/* restore guard page */
	amd64_mov_reg_imm (code, AMD64_R11, _resetstkoflw);
	amd64_call_reg (code, AMD64_R11);

	/* get jit_tls with context to restore */
	amd64_mov_reg_imm (code, AMD64_R11, mono_tls_get_jit_tls_extern);
	amd64_call_reg (code, AMD64_R11);

	/* move jit_tls from return reg to arg reg */
	amd64_mov_reg_reg (code, AMD64_ARG_REG1, AMD64_RAX, 8);

	/* retrieve pointer to saved context */
	amd64_alu_reg_imm (code, X86_ADD, AMD64_ARG_REG1, MONO_STRUCT_OFFSET (MonoJitTlsData, stack_restore_ctx));

	/* this call does not return */
	amd64_mov_reg_imm (code, AMD64_R11, mono_restore_context);
	amd64_call_reg (code, AMD64_R11);

	g_assertf ((code - start) <= size, "%d %d", (int)(code - start), size);

	mono_arch_flush_icache (start, code - start);
	MONO_PROFILER_RAISE (jit_code_buffer, (start, code - start, MONO_PROFILER_CODE_BUFFER_EXCEPTION_HANDLING, NULL));

	return start;
}
#elif HAVE_API_SUPPORT_WIN32_RESET_STKOFLW && TARGET_ARM64
static gpointer
get_win32_restore_stack(void)
{
	static guint8* start = NULL;
	guint8* code;

	if (start)
		return start;

	const int size = 64;

	/* restore_stack (void) */
	start = code = mono_global_codeman_reserve(size);

	/* push the incoming frame pointer and return onto the stack */
	arm_stpx_pre(code, ARMREG_FP, ARMREG_LR, ARMREG_SP, -0x10);

	/* update the link pointer to the current stack */
	arm_movspx(code, ARMREG_FP, ARMREG_SP);

	/* stack overflow protection */
	arm_adrlx(code, ARMREG_R8, _resetstkoflw);
	arm_blrx(code, ARMREG_R8);

	/* call mono_tls_get_jit_tls_extern */
	arm_adrlx(code, ARMREG_R8, mono_tls_get_jit_tls_extern);
	arm_blrx(code, ARMREG_R8);

	/* return value is in x0 */
	arm_addx_imm(code, ARMREG_R0, ARMREG_R0, ((int)MONO_STRUCT_OFFSET(MonoJitTlsData, stack_restore_ctx)) & 0xFFF);

	/* this call does not return */
	arm_adrlx(code, ARMREG_R8, mono_restore_context);
	arm_blrx(code, ARMREG_R8);

	g_assertf((code - start) <= size, "%d %d", (int)(code - start), size);

	mono_arch_flush_icache(start, code - start);
	MONO_PROFILER_RAISE(jit_code_buffer, (start, code - start, MONO_PROFILER_CODE_BUFFER_EXCEPTION_HANDLING, NULL));

	return start;
}
#else
static gpointer
get_win32_restore_stack (void)
{
	// _resetstkoflw unsupported on none desktop Windows platforms.
	return NULL;
}
#endif /* HAVE_API_SUPPORT_WIN32_RESET_STKOFLW */

/*
 * Unhandled Exception Filter
 * Top-level per-process exception handler.
 */
LONG CALLBACK seh_vectored_exception_handler(EXCEPTION_POINTERS* ep)
{
	EXCEPTION_RECORD* er;
	CONTEXT* ctx;
	LONG res;
	MonoJitTlsData *jit_tls = mono_tls_get_jit_tls ();
	MonoDomain* domain = mono_domain_get ();
	MonoWindowsSigHandlerInfo info = { TRUE, ep };

	/* If the thread is not managed by the runtime return early */
	if (!jit_tls)
		return EXCEPTION_CONTINUE_SEARCH;

	res = EXCEPTION_CONTINUE_EXECUTION;

	er = ep->ExceptionRecord;
	ctx = ep->ContextRecord;

	switch (er->ExceptionCode) {
	case EXCEPTION_STACK_OVERFLOW:
		if (!mono_aot_only && restore_stack) {
			if (mono_arch_handle_exception (ctx, domain->stack_overflow_ex)) {
				/* need to restore stack protection once stack is unwound
				 * restore_stack will restore stack protection and then
				 * resume control to the saved stack_restore_ctx */
				mono_sigctx_to_monoctx (ctx, &jit_tls->stack_restore_ctx);
#ifdef TARGET_ARM64
				ctx->Pc = (guint64)restore_stack;
#else
				ctx->Rip = (guint64)restore_stack;
#endif 
			}
		} else {
			info.handled = FALSE;
		}
		break;
	case EXCEPTION_ACCESS_VIOLATION:
		W32_SEH_HANDLE_EX(segv);
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		W32_SEH_HANDLE_EX(ill);
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
	case EXCEPTION_INT_OVERFLOW:
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
	case EXCEPTION_FLT_OVERFLOW:
	case EXCEPTION_FLT_UNDERFLOW:
	case EXCEPTION_FLT_INEXACT_RESULT:
		W32_SEH_HANDLE_EX(fpe);
		break;
	default:
		info.handled = FALSE;
		break;
	}

	if (!info.handled) {
		/* Don't copy context back if we chained exception
		* as the handler may have modfied the EXCEPTION_POINTERS
		* directly. We don't pass sigcontext to chained handlers.
		* Return continue search so the UnhandledExceptionFilter
		* can correctly chain the exception.
		*/
		res = EXCEPTION_CONTINUE_SEARCH;
	}

	return res;
}

void win32_seh_init()
{
	if (!mono_aot_only)
		restore_stack = (void (*) (void))get_win32_restore_stack ();

	mono_old_win_toplevel_exception_filter = SetUnhandledExceptionFilter(seh_unhandled_exception_filter);
	mono_win_vectored_exception_handle = AddVectoredExceptionHandler (1, seh_vectored_exception_handler);
}

void win32_seh_cleanup()
{
	guint32 ret = 0;

	if (mono_old_win_toplevel_exception_filter) SetUnhandledExceptionFilter(mono_old_win_toplevel_exception_filter);

	ret = RemoveVectoredExceptionHandler (mono_win_vectored_exception_handle);
	g_assert (ret);
}

void win32_seh_set_handler(int type, MonoW32ExceptionHandler handler)
{
	switch (type) {
	case SIGFPE:
		fpe_handler = handler;
		break;
	case SIGILL:
		ill_handler = handler;
		break;
	case SIGSEGV:
		segv_handler = handler;
		break;
	default:
		break;
	}
}
#endif /* _WIN64 */

#if HAVE_API_SUPPORT_WIN32_CONSOLE
/**
* atexit_wait_keypress:
*
* This function is installed as an atexit function making sure that the console is not terminated before the end user has a chance to read the result.
* This can be handy in debug scenarios (running from within the debugger) since an exit of the process will close the console window
* without giving the end user a chance to look at the output before closed.
*/
static void
atexit_wait_keypress (void)
{

	fflush (stdin);

	printf ("Press any key to continue . . . ");
	fflush (stdout);

	_getch ();

	return;
}

/**
* install_atexit_wait_keypress:
*
* This function installs the wait keypress exit handler.
*/
static void
install_atexit_wait_keypress (void)
{
	atexit (atexit_wait_keypress);
	return;
}

#else

/**
* install_atexit_wait_keypress:
*
* Not supported on WINAPI family.
*/
static void
install_atexit_wait_keypress (void)
{
	return;
}

#endif /* HAVE_API_SUPPORT_WIN32_CONSOLE */

// Table describing handlers that can be installed at process startup. Adding a new handler can be done by adding a new item to the table together with an install handler function.
const HandlerItem g_handler_items[] = { { MONO_HANDLER_ATEXIT_WAIT_KEYPRESS, MONO_HANDLER_ATEXIT_WAIT_KEYPRESS_LEN, install_atexit_wait_keypress },
					{ NULL, 0, NULL } };

/**
 * get_handler_arg_len:
 * @handlers: Get length of next handler.
 *
 * This function calculates the length of next handler included in argument.
 *
 * Returns: The length of next handler, if available.
 */
static size_t
get_next_handler_arg_len (const char *handlers)
{
	assert (handlers != NULL);

	size_t current_len = 0;
	const char *handler = strchr (handlers, MONO_HANDLER_DELIMITER);
	if (handler != NULL) {
		// Get length of next handler arg.
		current_len = (handler - handlers);
	} else {
		// Consume rest as length of next handler arg.
		current_len = strlen (handlers);
	}

	return current_len;
}

/**
 * install_custom_handler:
 * @handlers: Handlers included in --handler argument, example "atexit-waitkeypress,someothercmd,yetanothercmd".
 * @handler_arg_len: Output, length of consumed handler.
 *
 * This function installs the next handler included in @handlers parameter.
 *
 * Returns: TRUE on successful install, FALSE on failure or unrecognized handler.
 */
static gboolean
install_custom_handler (const char *handlers, size_t *handler_arg_len)
{
	gboolean result = FALSE;

	assert (handlers != NULL);
	assert (handler_arg_len);

	*handler_arg_len = get_next_handler_arg_len (handlers);
	for (int current_item = 0; current_item < G_N_ELEMENTS (g_handler_items); ++current_item) {
		const HandlerItem * handler_item = &g_handler_items [current_item];

		if (handler_item->cmd == NULL)
			continue;

		if (*handler_arg_len == handler_item->cmd_len && strncmp (handlers, handler_item->cmd, *handler_arg_len) == 0) {
			assert (handler_item->handler != NULL);
			handler_item->handler ();
			result = TRUE;
			break;
		}
	}
	return result;
}

void
mono_runtime_install_handlers (void)
{
#ifndef MONO_CROSS_COMPILE
	win32_seh_init();
	win32_seh_set_handler(SIGFPE, mono_sigfpe_signal_handler);
	win32_seh_set_handler(SIGILL, mono_crashing_signal_handler);
	win32_seh_set_handler(SIGSEGV, mono_sigsegv_signal_handler);
	if (mini_debug_options.handle_sigint)
		win32_seh_set_handler(SIGINT, mono_sigint_signal_handler);
#endif
}

gboolean
mono_runtime_install_custom_handlers (const char *handlers)
{
	gboolean result = FALSE;

	assert (handlers != NULL);
	while (*handlers != '\0') {
		size_t handler_arg_len = 0;

		result = install_custom_handler (handlers, &handler_arg_len);
		handlers += handler_arg_len;

		if (*handlers == MONO_HANDLER_DELIMITER)
			handlers++;
		if (!result)
			break;
	}

	return result;
}

void
mono_runtime_install_custom_handlers_usage (void)
{
	fprintf (stdout,
		 "Custom Handlers:\n"
		 "   --handlers=HANDLERS            Enable handler support, HANDLERS is a comma\n"
		 "                                  separated list of available handlers to install.\n"
		 "\n"
#if HAVE_API_SUPPORT_WIN32_CONSOLE
		 "HANDLERS is composed of:\n"
		 "    atexit-waitkeypress           Install an atexit handler waiting for a keypress\n"
		 "                                  before exiting process.\n");
#else
		 "No handlers supported on current platform.\n");
#endif /* HAVE_API_SUPPORT_WIN32_CONSOLE */
}

void
mono_runtime_cleanup_handlers (void)
{
#ifndef MONO_CROSS_COMPILE
	win32_seh_cleanup();
#endif
}

void
mono_init_native_crash_info (void)
{
	return;
}

void
mono_cleanup_native_crash_info (void)
{
	return;
}

/* mono_chain_signal:
 *
 *   Call the original signal handler for the signal given by the arguments, which
 * should be the same as for a signal handler. Returns TRUE if the original handler
 * was called, false otherwise.
 */
gboolean
MONO_SIG_HANDLER_SIGNATURE (mono_chain_signal)
{
	/* Set to FALSE to indicate that vectored exception handling should continue to look for handler */
	MONO_SIG_HANDLER_GET_INFO ()->handled = FALSE;
	return TRUE;
}

#if !HAVE_EXTERN_DEFINED_NATIVE_CRASH_HANDLER
#ifndef MONO_CROSS_COMPILE
void
mono_dump_native_crash_info (const char *signal, MonoContext *mctx, MONO_SIG_HANDLER_INFO_TYPE *info)
{
	//TBD
}

void
mono_post_native_crash_handler (const char *signal, MonoContext *mctx, MONO_SIG_HANDLER_INFO_TYPE *info, gboolean crash_chaining)
{
	if (!crash_chaining)
		abort ();
}
#endif /* !MONO_CROSS_COMPILE */
#endif /* !HAVE_EXTERN_DEFINED_NATIVE_CRASH_HANDLER */

#if HAVE_API_SUPPORT_WIN32_TIMERS
#include <mmsystem.h>
static MMRESULT g_timer_event = 0;
static HANDLE g_timer_main_thread = INVALID_HANDLE_VALUE;

static VOID
thread_timer_expired (HANDLE thread)
{
	CONTEXT context;

	context.ContextFlags = CONTEXT_CONTROL;
	if (GetThreadContext (thread, &context)) {
		guchar *ip;

#ifdef _WIN64
#if defined(TARGET_AMD64)
		ip = (guchar*)context.Rip;
#elif defined(TARGET_ARM64)
		ip = (guchar*)context.Pc;
#else
		#error Unknown architecture
#endif 
#else
		ip = (guchar *) context.Eip;
#endif

		MONO_PROFILER_RAISE (sample_hit, (ip, &context));
	}
}

static VOID CALLBACK
timer_event_proc (UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	thread_timer_expired ((HANDLE)dwUser);
}

static VOID
stop_profiler_timer_event (void)
{
	if (g_timer_event != 0) {

		timeKillEvent (g_timer_event);
		g_timer_event = 0;
	}

	if (g_timer_main_thread != INVALID_HANDLE_VALUE) {

		CloseHandle (g_timer_main_thread);
		g_timer_main_thread = INVALID_HANDLE_VALUE;
	}
}

static VOID
start_profiler_timer_event (void)
{
	g_return_if_fail (g_timer_main_thread == INVALID_HANDLE_VALUE && g_timer_event == 0);

	TIMECAPS timecaps;

	if (timeGetDevCaps (&timecaps, sizeof (timecaps)) != TIMERR_NOERROR)
		return;

	g_timer_main_thread = OpenThread (READ_CONTROL | THREAD_GET_CONTEXT, FALSE, GetCurrentThreadId ());
	if (g_timer_main_thread == NULL)
		return;

	if (timeBeginPeriod (1) != TIMERR_NOERROR)
		return;

	g_timer_event = timeSetEvent (1, 0, (LPTIMECALLBACK)timer_event_proc, (DWORD_PTR)g_timer_main_thread, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
	if (g_timer_event == 0) {
		timeEndPeriod (1);
		return;
	}
}

void
mono_runtime_setup_stat_profiler (void)
{
	start_profiler_timer_event ();
	return;
}

void
mono_runtime_shutdown_stat_profiler (void)
{
	stop_profiler_timer_event ();
	return;
}
#elif !HAVE_EXTERN_DEFINED_WIN32_TIMERS
void
mono_runtime_setup_stat_profiler (void)
{
	g_unsupported_api ("timeGetDevCaps, timeBeginPeriod, timeEndPeriod, timeSetEvent, timeKillEvent");
	SetLastError (ERROR_NOT_SUPPORTED);
	return;
}

void
mono_runtime_shutdown_stat_profiler (void)
{
	g_unsupported_api ("timeGetDevCaps, timeBeginPeriod, timeEndPeriod, timeSetEvent, timeKillEvent");
	SetLastError (ERROR_NOT_SUPPORTED);
	return;
}
#endif /* HAVE_API_SUPPORT_WIN32_TIMERS */

#if HAVE_API_SUPPORT_WIN32_OPEN_THREAD
gboolean
mono_setup_thread_context(DWORD thread_id, MonoContext *mono_context)
{
	HANDLE handle;
#if defined(MONO_HAVE_SIMD_REG_AVX) && HAVE_API_SUPPORT_WIN32_CONTEXT_XSTATE
	BYTE context_buffer [2048];
	DWORD context_buffer_len = G_N_ELEMENTS (context_buffer);
	PCONTEXT context = NULL;
	BOOL success = InitializeContext (context_buffer, CONTEXT_INTEGER | CONTEXT_FLOATING_POINT | CONTEXT_CONTROL | CONTEXT_XSTATE, &context, &context_buffer_len);
	success &= SetXStateFeaturesMask (context, XSTATE_MASK_AVX);
	g_assert (success == TRUE);
#else
	CONTEXT context_buffer;
	PCONTEXT context = &context_buffer;
	context->ContextFlags = CONTEXT_INTEGER | CONTEXT_FLOATING_POINT | CONTEXT_CONTROL;
#endif

	g_assert (thread_id != GetCurrentThreadId ());

	handle = OpenThread (THREAD_ALL_ACCESS, FALSE, thread_id);
	g_assert (handle);

	if (!GetThreadContext (handle, context)) {
		CloseHandle (handle);
		return FALSE;
	}

	memset (mono_context, 0, sizeof (MonoContext));
	mono_sigctx_to_monoctx (context, mono_context);

	CloseHandle (handle);
	return TRUE;
}
#elif !HAVE_EXTERN_DEFINED_WIN32_OPEN_THREAD
gboolean
mono_setup_thread_context (DWORD thread_id, MonoContext *mono_context)
{
	g_unsupported_api ("OpenThread");
	SetLastError (ERROR_NOT_SUPPORTED);
	return FALSE;
}
#endif /* HAVE_API_SUPPORT_WIN32_OPEN_THREAD */

gboolean
mono_thread_state_init_from_handle (MonoThreadUnwindState *tctx, MonoThreadInfo *info, void *sigctx)
{
	tctx->valid = FALSE;
	tctx->unwind_data [MONO_UNWIND_DATA_DOMAIN] = NULL;
	tctx->unwind_data [MONO_UNWIND_DATA_LMF] = NULL;
	tctx->unwind_data [MONO_UNWIND_DATA_JIT_TLS] = NULL;

	if (sigctx == NULL) {
		DWORD id = mono_thread_info_get_tid (info);
		mono_setup_thread_context (id, &tctx->ctx);
	} else {
#ifdef ENABLE_CHECKED_BUILD
		g_assert (((CONTEXT *)sigctx)->ContextFlags & CONTEXT_INTEGER);
		g_assert (((CONTEXT *)sigctx)->ContextFlags & CONTEXT_CONTROL);
		g_assert (((CONTEXT *)sigctx)->ContextFlags & CONTEXT_FLOATING_POINT);
#if defined(MONO_HAVE_SIMD_REG_AVX) && HAVE_API_SUPPORT_WIN32_CONTEXT_XSTATE
		DWORD64 features = 0;
		g_assert (((CONTEXT *)sigctx)->ContextFlags & CONTEXT_XSTATE);
		g_assert (GetXStateFeaturesMask (((CONTEXT *)sigctx), &features) == TRUE);
		g_assert ((features & XSTATE_MASK_LEGACY_SSE) != 0);
		g_assert ((features & XSTATE_MASK_AVX) != 0);
#endif
#endif
		mono_sigctx_to_monoctx (sigctx, &tctx->ctx);
	}

	/* mono_set_jit_tls () sets this */
	void *jit_tls = mono_thread_info_tls_get (info, TLS_KEY_JIT_TLS);
	/* SET_APPDOMAIN () sets this */
	void *domain = mono_thread_info_tls_get (info, TLS_KEY_DOMAIN);

	/*Thread already started to cleanup, can no longer capture unwind state*/
	if (!jit_tls || !domain)
		return FALSE;

	/*
	 * The current LMF address is kept in a separate TLS variable, and its hard to read its value without
	 * arch-specific code. But the address of the TLS variable is stored in another TLS variable which
	 * can be accessed through MonoThreadInfo.
	 */
	/* mono_set_lmf_addr () sets this */
	MonoLMF *lmf = NULL;
	MonoLMF **addr = (MonoLMF**)mono_thread_info_tls_get (info, TLS_KEY_LMF_ADDR);
	if (addr)
		lmf = *addr;

	tctx->unwind_data [MONO_UNWIND_DATA_DOMAIN] = domain;
	tctx->unwind_data [MONO_UNWIND_DATA_JIT_TLS] = jit_tls;
	tctx->unwind_data [MONO_UNWIND_DATA_LMF] = lmf;
	tctx->valid = TRUE;

	return TRUE;
}

BOOL
mono_win32_runtime_tls_callback (HMODULE module_handle, DWORD reason, LPVOID reserved, MonoWin32TLSCallbackType callback_type)
{
	if (!mono_win32_handle_tls_callback_type (callback_type))
		return TRUE;

	if (!mono_gc_dllmain (module_handle, reason, reserved))
		return FALSE;

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		mono_install_runtime_load (mini_init);
		break;
	case DLL_PROCESS_DETACH:
		if (coree_module_handle)
			FreeLibrary (coree_module_handle);
		break;
	case DLL_THREAD_DETACH:
		mono_thread_info_detach ();
		break;

	}
	return TRUE;
}
