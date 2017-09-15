

#include "config.h"
#include "mini.h"
#include "pmip_my_callstack.h"
#include "seq-points.h"

#include <mono/metadata/mono-debug.h>
#include <mono/metadata/mono-debug-debugger.h>
#include <mono/metadata/debug-mono-symfile.h>
#include <mono/utils/mono-counters.h>

#if !defined(DISABLE_JIT) && !defined(DISABLE_LLDB)

static gboolean enabled;
static mono_mutex_t mutex;
static FILE* fd;

#define pmip_my_callstack_lock() mono_os_mutex_lock (&mutex)
#define pmip_my_callstack_unlock() mono_os_mutex_unlock (&mutex)

void
mono_pmip_my_callstack_init (const char *options)
{
	if (g_getenv("PMIP_ENABLE"))
	{
		char* file_name = g_strdup_printf("pmip.%d", GetCurrentProcessId());
		char* path = g_build_filename(g_get_tmp_dir(), file_name, NULL);

		mono_os_mutex_init_recursive(&mutex);

		fd = _fsopen(path, "w", _SH_DENYNO);

		g_free(file_name);
		g_free(path);

		if (fd)
			enabled = TRUE;
	}
}

static char *
pmip_pretty(MonoMethod* method)
{
	char* formattedPMIP;

	MonoDomain *domain = mono_domain_get();
	if (!domain)
		domain = mono_get_root_domain();

	MonoClass* klass = method->klass;
	char* method_name = mono_method_full_name(method, TRUE);

	MonoDebugSourceLocation* location = mono_debug_lookup_source_location(method, 0, domain);
	MonoDebugMethodInfo* minfo = mono_debug_lookup_method(method);

	char *lineNumber, *filePath;
	if (location)
	{
		lineNumber = g_strdup_printf("%d", location->row);
		filePath = g_strdup(location->source_file);
	}
	else
	{
		lineNumber = g_strdup("<UNKNOWN>");
		filePath = g_strdup("<UNKNOWN>");
	}

	char* assembly_name = klass->image->module_name;

	formattedPMIP = g_strdup_printf("[%s] %s Line %s File %s", assembly_name, method_name, lineNumber, filePath);

	mono_debug_free_source_location(location);
	g_free(method_name);
	g_free(lineNumber);
	g_free(filePath);

	return formattedPMIP;
}


void
mono_pmip_my_callstack_save_method_info (MonoCompile *cfg)
{
	char* pretty_name;

	if (!enabled)
		return;

	pmip_my_callstack_lock ();
	pretty_name = pmip_pretty(cfg->method);
	fprintf(fd, "%p;%p;%s\n", cfg->native_code, ((char*)cfg->native_code) + cfg->code_size, pretty_name);
	g_free(pretty_name);
	fflush (fd);
	pmip_my_callstack_unlock ();
}

void
mono_pmip_my_callstack_remove_method (MonoDomain *domain, MonoMethod *method, MonoJitDynamicMethodInfo *info)
{
}

void
mono_pmip_my_callstack_save_trampoline_info (MonoTrampInfo *info)
{
	if (!enabled)
		return;

	pmip_my_callstack_lock ();
	fprintf (fd, "%p;%p;%s\n", info->code, ((char*)info->code) + info->code_size, info->name ? info->name : "");
	fflush (fd);
	pmip_my_callstack_unlock ();
}

void
mono_pmip_my_callstack_save_specific_trampoline_info (gpointer arg1, MonoTrampolineType tramp_type, MonoDomain *domain, gpointer code, guint32 code_len)
{

}

#else

void
mono_pmip_my_callstack_init (const char *options)
{
	g_error ("lldb support has been disabled at configure time.");
}

void
mono_pmip_my_callstack_save_method_info (MonoCompile *cfg)
{
}

void
mono_pmip_my_callstack_save_trampoline_info (MonoTrampInfo *info)
{
}

void
mono_pmip_my_callstack_remove_method (MonoDomain *domain, MonoMethod *method, MonoJitDynamicMethodInfo *info)
{
}

void
mono_pmip_my_callstack_save_specific_trampoline_info (gpointer arg1, MonoTrampolineType tramp_type, MonoDomain *domain, gpointer code, guint32 code_len)
{
}

#endif
