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


GString* gEmbeddingHostName = 0;


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

// This is a copy ov mini_class_get_context.
MonoGenericContext* mono_unity_class_get_generic_context(MonoClass* klass)
{
	if (klass->generic_class)
		return &klass->generic_class->context;

	g_assert(klass->generic_container);
	return &klass->generic_container->context;
}

void mono_unity_install_finalize_runtime_invoke(MonoDomain* domain, RuntimeInvokeFunction callback)
{
	domain->finalize_runtime_invoke = callback;
}

//must match the hash in il2cpp code generation
static guint32 hash_string_djb2(guchar *str)
{
	guint32 hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

static guint32 get_array_structure_hash(MonoArrayType *atype)
{
	char buffer[100];
	char *ptr = buffer;

	*ptr++ = '[';

	char numbuffer[10];

	for (int i = 0; i < atype->rank; ++i)
	{
		if (atype->numlobounds > 0 && atype->lobounds[i] != 0)
		{
			itoa(atype->lobounds[i], numbuffer, 10);
			char *ptrnum = numbuffer;
			while (*ptrnum)
				*ptr++ = *ptrnum++;

			*ptr++ = ':';
		}

		if (atype->numsizes > 0 && atype->sizes[i] != 0)
		{
			itoa(atype->sizes[i], numbuffer, 10);
			char *ptrnum = numbuffer;
			while (*ptrnum)
				*ptr++ = *ptrnum++;
		}

		if (i < atype->rank - 1)
			*ptr++ = ',';
	}

	*ptr++ = ']';
	*ptr++ = 0;

	return hash_string_djb2(buffer);
}

void get_type_hashes(MonoType *type, GList *hashes);
void get_type_hashes_generic_class(MonoGenericClass *generic_class, GList *hashes);


static void get_type_hashes_generic_class(MonoGenericClass *generic_class, GList *hashes)
{
	MonoGenericInst *inst = generic_class->context.class_inst;
	for (int i = 0; i < inst->type_argc; ++i)
	{
		MonoType *type = inst->type_argv[i];
		get_type_hashes(type, hashes);
	}
}

static void get_type_hashes(MonoType *type, GList *hashes)
{
	if (type->type != MONO_TYPE_GENERICINST)
	{
		MonoClass *klass = NULL;

		switch (type->type)
		{
		case MONO_TYPE_ARRAY:
		{
			MonoArrayType *atype = type->data.array;
			g_list_append(hashes, MONO_TOKEN_TYPE_SPEC);
			g_list_append(hashes, get_array_structure_hash(atype));
			get_type_hashes(&(atype->eklass->this_arg), hashes);
			break;
		}
		case MONO_TYPE_CLASS:
			klass = type->data.klass;
			break;
		case MONO_TYPE_BOOLEAN:
			klass = mono_defaults.boolean_class;
			break;
		case MONO_TYPE_CHAR:
			klass = mono_defaults.char_class;
			break;
		case MONO_TYPE_I1:
			klass = mono_defaults.sbyte_class;
			break;
		case MONO_TYPE_U1:
			klass = mono_defaults.byte_class;
			break;
		case MONO_TYPE_I2:
			klass = mono_defaults.int16_class;
			break;
		case MONO_TYPE_U2:
			klass = mono_defaults.uint16_class;
			break;
		case MONO_TYPE_I4:
			klass = mono_defaults.int32_class;
			break;
		case MONO_TYPE_U4:
			klass = mono_defaults.uint32_class;
			break;
		case MONO_TYPE_I8:
			klass = mono_defaults.int64_class;
			break;
		case MONO_TYPE_U8:
			klass = mono_defaults.uint64_class;
			break;
		case MONO_TYPE_R4:
			klass = mono_defaults.single_class;
			break;
		case MONO_TYPE_R8:
			klass = mono_defaults.double_class;
			break;
		case MONO_TYPE_STRING:
			klass = mono_defaults.string_class;
			break;
		case MONO_TYPE_OBJECT:
			klass = mono_defaults.object_class;
			break;
		}

		if (klass)
		{
			g_list_append(hashes, klass->type_token);
			g_list_append(hashes, hash_string_djb2(klass->image->module_name));
		}
		
		return;
	}
	else
	{
		get_type_hashes_generic_class(type->data.generic_class, hashes);
	}

}

static GList* get_type_hashes_method(MonoMethod *method)
{
	GList *hashes = monoeg_g_list_alloc();

	hashes->data = method->token;

	if (method->klass->is_inflated)
		get_type_hashes_generic_class(method->klass->generic_class, hashes);
	
	return hashes;
}

//hash combination function must match the one used in IL2CPP codegen
static guint64 combine_hashes(guint64 hash1, guint64 hash2)
{
	return hash1 * 486187739 + hash2;
}

static void combine_all_hashes(gpointer data, gpointer user_data)
{
	guint64 *hash = (guint64*)user_data;
	if (*hash == 0)
		*hash = (guint64)data;
	else
		*hash = combine_hashes(*hash, (guint64)data);
}

guint64 mono_unity_get_method_hash(MonoMethod *method)
{
	GList *hashes = get_type_hashes_method(method);

	guint64 hash = 0;

	g_list_first(hashes);
	g_list_foreach(hashes, combine_all_hashes, &hash);
	g_list_free(hashes);

	return hash;
}