#ifndef __UNITY_MONO_UTILS_H
#define __UNITY_MONO_UTILS_H

#include <glib.h>
#include <stdio.h>
#include <mono/metadata/object.h>
#include <mono/metadata/marshal.h>

typedef void(*vprintf_func)(const char* msg, va_list args);
typedef struct {
	void* (*malloc_func)(size_t size);
	void(*free_func)(void *ptr);
	void* (*calloc_func)(size_t nmemb, size_t size);
	void* (*realloc_func)(void *ptr, size_t size);
} MonoMemoryCallbacks;

/**
 *	Custom exit function, called instead of system exit()
 */
void unity_mono_exit( int code );

/**
 *	Closes redirected output files.
 */
void unity_mono_close_output(void);

extern MonoString* mono_unity_get_embeddinghostname(void);

void mono_unity_write_to_unity_log(MonoString* str);

#ifdef WIN32
FILE* unity_fopen( const char *name, const char *mode );
#endif

extern gboolean mono_unity_socket_security_enabled_get (void);
MONO_API extern void mono_unity_socket_security_enabled_set (gboolean enabled);
MONO_API void mono_unity_set_vprintf_func(vprintf_func func);


void unity_mono_install_memory_callbacks(MonoMemoryCallbacks* callbacks);

gboolean
unity_mono_method_is_inflated (MonoMethod* method);

MONO_API gboolean
unity_mono_method_is_generic (MonoMethod* method);

typedef const char*(*UnityFindPluginCallback)(const char*);

MONO_API void
mono_set_find_plugin_callback(UnityFindPluginCallback find);

MONO_API UnityFindPluginCallback
mono_get_find_plugin_callback();

MonoAssembly* mono_unity_mscorlib();
const char* mono_unity_image_name_for(MonoMethod* method);
void* mono_unity_get_field_address(MonoObject *obj, MonoVTable *vt, MonoClassField *field);
MonoObject* mono_unity_compare_exchange(MonoObject **location, MonoObject *value, MonoObject *comparand);
void mono_unity_init_obj(void* obj, MonoClass* klass);
MonoObject* mono_unity_isinst_sealed(MonoObject* obj, MonoClass* targetType);
MonoClass* mono_unity_get_generic_definition(MonoClass* klass);
MonoClass* mono_unity_get_class_for_generic_parameter(MonoGenericContainer* generic_container, gint index);
MonoClass* mono_unity_class_inflate_generic_class(MonoClass *gklass, MonoGenericContext *context);
MonoVTable* mono_unity_class_get_vtable(MonoClass* klass);
gboolean mono_unity_class_has_parent_unsafe(MonoClass *klass, MonoClass *parent);
guint64 mono_unity_get_method_hash(MonoMethod *method);
MonoGenericContext* mono_unity_class_get_generic_context(MonoClass* klass);
void mono_unity_install_finalize_runtime_invoke(MonoDomain* domain, RuntimeInvokeFunction callback);
#endif
