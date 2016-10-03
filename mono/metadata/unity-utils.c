#include <config.h>
#include <mono/utils/mono-publib.h>
#include <mono/metadata/unity-utils.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <fcntl.h>
#endif
#include <mono/metadata/object.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/tabledefs.h>
#include <mono/metadata/class-internals.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/metadata-internals.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/tokentype.h>
#include <mono/metadata/threadpool-ms.h>
#include <mono/utils/mono-string.h>

#include <glib.h>

#ifdef WIN32
#define UTF8_2_WIDE(src,dst) MultiByteToWideChar( CP_UTF8, 0, src, -1, dst, MAX_PATH )
#endif

#undef exit

void unity_mono_exit( int code )
{
	//fprintf( stderr, "mono: exit called, code %d\n", code );
	exit( code );
}

#ifdef WIN32

extern LONG CALLBACK seh_vectored_exception_handler(EXCEPTION_POINTERS* ep);
LONG mono_unity_seh_handler(EXCEPTION_POINTERS* ep)
{
#if defined(TARGET_X86) || defined(TARGET_AMD64)
	return seh_vectored_exception_handler(ep);
#else
	g_assert_not_reached();
#endif
}

int (*gUnhandledExceptionHandler)(EXCEPTION_POINTERS*) = NULL;

void mono_unity_set_unhandled_exception_handler(void* handler)
{
	gUnhandledExceptionHandler = handler;
}

#endif //Win32

GString* gEmbeddingHostName = 0;


void mono_unity_write_to_unity_log(MonoString* str)
{
	fprintf(stdout, mono_string_to_utf8(str));
	fflush(stdout);
}


MONO_API void mono_unity_set_embeddinghostname(const char* name)
{
	gEmbeddingHostName = g_string_new(name);
}



MonoString* mono_unity_get_embeddinghostname()
{
	if (gEmbeddingHostName == 0)
		mono_unity_set_embeddinghostname("mono");
	return mono_string_new_wrapper(gEmbeddingHostName->str);
}

static gboolean socket_security_enabled = FALSE;

gboolean
mono_unity_socket_security_enabled_get ()
{
	return socket_security_enabled;
}

void
mono_unity_socket_security_enabled_set (gboolean enabled)
{
	socket_security_enabled = enabled;
}

void mono_unity_set_vprintf_func (vprintf_func func)
{
	//set_vprintf_func (func);
}

MONO_API gboolean
mono_unity_class_is_interface (MonoClass* klass)
{
	return MONO_CLASS_IS_INTERFACE(klass);
}

MONO_API gboolean
mono_unity_class_is_abstract (MonoClass* klass)
{
	return (klass->flags & TYPE_ATTRIBUTE_ABSTRACT);
}

void
unity_mono_install_memory_callbacks(MonoMemoryCallbacks* callbacks)
{
	//g_mem_set_callbacks (callbacks);
}

// classes_ref is a preallocated array of *length_ref MonoClass*
// returned classes are stored in classes_ref, number of stored classes is stored in length_ref
// return value is number of classes found (which may be greater than number of classes stored)
unsigned mono_unity_get_all_classes_with_name_case (MonoImage *image, const char *name, MonoClass **classes_ref, unsigned *length_ref)
{
	MonoClass *klass;
	MonoTableInfo *tdef = &image->tables [MONO_TABLE_TYPEDEF];
	int i, count;
	guint32 attrs, visibility;
	unsigned length = 0;

	/* (yoinked from icall.c) we start the count from 1 because we skip the special type <Module> */
	for (i = 1; i < tdef->rows; ++i)
	{
		klass = mono_class_get (image, (i + 1) | MONO_TOKEN_TYPE_DEF);
		if (klass && klass->name && 0 == mono_utf8_strcasecmp (klass->name, name))
		{
			if (length < *length_ref)
				classes_ref[length] = klass;
			++length;
		}
	}

	if (length < *length_ref)
		*length_ref = length;
	return length;
}

MONO_API gboolean
unity_mono_method_is_inflated (MonoMethod* method)
{
	return method->is_inflated;
}

gboolean
unity_mono_method_is_generic (MonoMethod* method)
{
	return method->is_generic;
}

MONO_API MonoMethod*
unity_mono_reflection_method_get_method(MonoReflectionMethod* mrf)
{
	if(!mrf)
		return NULL;

	return mrf->method;
}

MONO_API void
mono_unity_g_free(void *ptr)
{
	g_free (ptr);
}

MONO_API gboolean
mono_class_is_generic (MonoClass *klass)
{
	g_assert(klass);
	return (klass->is_generic);
}

MONO_API gboolean
mono_class_is_inflated (MonoClass *klass)
{
	g_assert(klass);
	return (klass->is_inflated);
}

MONO_API void
mono_thread_pool_cleanup (void)
{
	mono_threadpool_ms_cleanup ();
}

MONO_API void*
mono_class_get_userdata (MonoClass* klass)
{
	return klass->unity_user_data;
}

MONO_API void
mono_class_set_userdata(MonoClass* klass, void* userdata)
{
	klass->unity_user_data = userdata;
}

MONO_API int
mono_class_get_userdata_offset()
{
	return offsetof(struct _MonoClass, unity_user_data);
}


static UnityFindPluginCallback unity_find_plugin_callback;

MONO_API void
mono_set_find_plugin_callback (UnityFindPluginCallback find)
{
	unity_find_plugin_callback = find;
}

MONO_API UnityFindPluginCallback
mono_get_find_plugin_callback ()
{
	return unity_find_plugin_callback;
}

MonoAssembly* mono_unity_mscorlib()
{
	return mono_defaults.corlib->assembly;
}

const char* mono_unity_image_name_for(MonoMethod* method)
{
	return method->klass->image->assembly_name;
}

void* mono_unity_get_field_address(MonoObject *obj, MonoVTable *vt, MonoClassField *field)
{
	// This is a copy of mono_field_get_addr - we need to consider how to expose that on the public API.
	MONO_REQ_GC_UNSAFE_MODE;

	guint8 *src;

	if (field->type->attrs & FIELD_ATTRIBUTE_STATIC) {
		if (field->offset == -1) {
			/* Special static */
			gpointer addr;

			mono_domain_lock(vt->domain);
			addr = g_hash_table_lookup(vt->domain->special_static_fields, field);
			mono_domain_unlock(vt->domain);
			src = (guint8 *)mono_get_special_static_data(GPOINTER_TO_UINT(addr));
		}
		else {
			src = (guint8*)mono_vtable_get_static_field_data(vt) + field->offset;
		}
	}
	else {
		src = (guint8*)obj + field->offset;
	}

	return src;
}

MonoObject* mono_unity_compare_exchange(MonoObject **location, MonoObject *value, MonoObject *comparand)
{
	return ves_icall_System_Threading_Interlocked_CompareExchange_T(location, value, comparand);
}

void mono_unity_init_obj(void* obj, MonoClass* klass)
{
	if (klass->valuetype)
		memset(obj, 0, klass->instance_size - sizeof(MonoObject));
	else
		*(MonoObject**)obj = NULL;
}

MonoObject* mono_unity_isinst_sealed(MonoObject* obj, MonoClass* targetType)
{
	return obj->vtable->klass == targetType ? obj : NULL;
}

MonoClass* mono_unity_get_generic_definition(MonoClass* klass)
{
	if (klass->generic_class && klass->generic_class->container_class)
		return klass->generic_class->container_class;

	return NULL;
}

MonoClass* mono_unity_get_class_for_generic_parameter(MonoGenericContainer* generic_container, gint index)
{
	g_assert(index < generic_container->type_argc);
	return generic_container->type_params[index].info.pklass;
}

MonoClass* mono_unity_class_inflate_generic_class(MonoClass *gklass, MonoGenericContext *context)
{
	MonoError error;
	return mono_class_inflate_generic_class_checked(gklass, context, &error);
}

MonoVTable* mono_unity_class_get_vtable(MonoClass* klass)
{
	return klass->vtable;
}

gboolean mono_unity_class_has_parent_unsafe(MonoClass *klass, MonoClass *parent)
{
	return mono_class_has_parent_fast(klass, parent);
}

