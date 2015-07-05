#include "unity_utils.h"
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
#include <mono/metadata/mono-endian.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/tokentype.h>
#include <mono/metadata/reflection.h>
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

HANDLE unity_log_output = 0;

void unity_mono_redirect_output( HANDLE handle )
{
	int fd;
	DWORD written;
//	int fd_copy;
	unity_log_output = handle;	
	fd = _open_osfhandle((intptr_t)handle, (_O_APPEND | _O_TEXT));
	stdout->_file = fd;
	_dup2(fd,_fileno(stdout));
	//*stdout = *_fdopen(fd, "at");
	
	setvbuf(stdout, NULL, _IONBF, 0);
	
	//fprintf(stdout, "printf from mono\n");
	//WriteFile(handle,"WriteFile from mono",16,&written,NULL);
}

HANDLE unity_mono_get_log_handle()
{
	return unity_log_output;
}

void unity_mono_close_output()
{
	fclose( stdout );
	fclose( stderr );
}

FILE* unity_fopen( const char *name, const char *mode )
{
	wchar_t wideName[MAX_PATH];
	wchar_t wideMode[MAX_PATH];
	UTF8_2_WIDE(name, wideName);
	UTF8_2_WIDE(mode, wideMode);
	return _wfopen( wideName, wideMode );
}

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


void mono_unity_set_embeddinghostname(const char* name)
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
	set_vprintf_func (func);
}

gboolean
mono_unity_class_is_interface (MonoClass* klass)
{
	return MONO_CLASS_IS_INTERFACE(klass);
}

gboolean
mono_unity_class_is_abstract (MonoClass* klass)
{
	return (klass->flags & TYPE_ATTRIBUTE_ABSTRACT);
}

void
unity_mono_install_memory_callbacks(MonoMemoryCallbacks* callbacks)
{
	g_mem_set_callbacks (callbacks);
}

void mono_unity_thread_clear_domain_fields (void)
{
	/*
	 we need to clear fields that may reference objects living in non-root appdomain
	 since the objects will live but their vtables will be destroyed when domain is torn down.
	 */
	MonoThread* thread = mono_thread_current ();
	thread->principal = NULL;
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

gboolean
unity_mono_method_is_inflated (MonoMethod* method)
{
	return method->is_inflated;
}

gboolean
unity_mono_method_is_generic (MonoMethod* method)
{
	return method->is_generic;
}

static size_t unpack_custom_attr_value(const char* src, int type, char* dest, MonoImage* image)
{
	// Pretty much mimicing load_cattr_value in reflection.c here now, but without the allocs:
	switch(type)
	{
		case MONO_TYPE_U1:
		case MONO_TYPE_I1:
		case MONO_TYPE_BOOLEAN:
		{
			*(MonoBoolean*)dest = *(MonoBoolean*)src;
			return 1;
		}
		case MONO_TYPE_CHAR:
		case MONO_TYPE_U2:
		case MONO_TYPE_I2:
		{
			*(guint16*)dest = *(guint16*)src;
			return 2;
		}
#if SIZEOF_VOID_P == 4
		case MONO_TYPE_U:
		case MONO_TYPE_I:
#endif
		case MONO_TYPE_R4:
		case MONO_TYPE_U4:
		case MONO_TYPE_I4: {
			*(guint32*)dest = *(guint32*)src;
			return 4;
		}
#if SIZEOF_VOID_P == 8
		case MONO_TYPE_U: /* error out instead? this should probably not happen */
		case MONO_TYPE_I:
#endif
		case MONO_TYPE_U8:
		case MONO_TYPE_I8: {
			*(guint64*)dest = *(guint64*)src;
			return 8;
		}
		case MONO_TYPE_R8: {
			*(double*)dest = *(double*)src;
			return 8;
		}
		case MONO_TYPE_STRING: {
			// if the first byte is 0xFF, it's a null/empty string
			UnityMonoMetadataString** str = (UnityMonoMetadataString**)dest;
			if(*src == (char)0xFF)
			{
				*str = NULL;
				return 1;
			}
			
			*str = (UnityMonoMetadataString*)src;
			const char* oldSrc = src;
			src += mono_metadata_decode_value(src, &src);
			return src - oldSrc;
		}
		case MONO_TYPE_CLASS: {
			// type references (i.e. typeof(SomeClass)) are stored as string names
			MonoType** result = (MonoType**)dest;
			if(*src == (char)0xFF)
			{
				*result = NULL;
				return 1;
			}
			
			const char* oldSrc = src;
			guint32 length = mono_metadata_decode_value (src, &src);
			char *utf8Str = g_memdup (src, length + 1);
			utf8Str [length] = 0;
			src += length;
			*result = mono_reflection_type_from_name (utf8Str, image);
			if (!*result)
				g_warning ("Cannot load type '%s'", utf8Str);
			g_free (utf8Str);
			return src - oldSrc;
		}
		default:
		{
			g_error ("Type 0x%02x not handled in custom attr value decoding", type);
			return 0;
		}
	}
}

void unity_mono_unpack_custom_attr_params(MonoImage* image, MonoCustomAttrEntry* entry, void* output, UnityMonoCustomAttrNamedParameterFunc namedParameterHandler)
{
	// Mimicing create_custom_attr in mono/metadata/reflection.c for unpacking the params
	// Also helpful: http://geekswithblogs.net/simonc/archive/2011/06/03/anatomy-of-a-.net-assembly---custom-attribute-encoding.aspx
	const char *src = (const char*)entry->data;
	char *dest = (char*)output;
	int i;
	
	MonoMethodSignature *sig = mono_method_signature(entry->ctor);
	
	// Skip prolog
	src += 2;
	for(i = 0; i < sig->param_count; ++i)
	{
		MonoType* paramType = sig->params[i];
		int type = paramType->type;
		
		if (type == MONO_TYPE_VALUETYPE && paramType->data.klass->enumtype)
			type = mono_class_enum_basetype(paramType->data.klass)->type;
		
		src += unpack_custom_attr_value(src, type, dest, image);
		
		// Advance dest as well
		switch(type)
		{
			case MONO_TYPE_U1:
			case MONO_TYPE_I1:
			case MONO_TYPE_BOOLEAN:
				dest += 1;
				break;
			case MONO_TYPE_CHAR:
			case MONO_TYPE_U2:
			case MONO_TYPE_I2:
				dest += 2;
				break;
#if SIZEOF_VOID_P == 4
			case MONO_TYPE_U:
			case MONO_TYPE_I:
#endif
			case MONO_TYPE_R4:
			case MONO_TYPE_U4:
			case MONO_TYPE_I4:
				dest += 4;
				break;
#if SIZEOF_VOID_P == 8
			case MONO_TYPE_U:
			case MONO_TYPE_I:
#endif
			case MONO_TYPE_U8:
			case MONO_TYPE_I8:
			case MONO_TYPE_R8:
				dest += 8;
				break;
			case MONO_TYPE_STRING:
				dest += sizeof(UnityMonoMetadataString*);
				break;
			case MONO_TYPE_CLASS:
				dest += sizeof(MonoType*);
				break;
			default:
				g_error("Unknown/unsupported parameter type");
				break;
		}
	}
	
	// Now, named parameters
	if(namedParameterHandler == NULL) return;
	guint32 num_named = read16(src);
	src += 2;
	for (i = 0; i < num_named; ++i)
	{
		UnityMonoMetadataString* paramName;
		src++; // skip the field/property specifier
		int type = *(src++);
		
		if (type == MONO_TYPE_SZARRAY)
		{
			g_error("Array types are not supported for named parameter unpacking");
			type = *(src++);
		}
		
		if (type == MONO_TYPE_ENUM)
		{
			// Enum typed parameters pack the actual name of the enum type
			// We don't really care about it for our purposes, just skip past it
			gint type_len = mono_metadata_decode_blob_size (src, &src);
			src += type_len;
		}
		
		paramName = (UnityMonoMetadataString*)src;
		src += mono_metadata_decode_blob_size (src, &src);
		
		// Unpack the attribute value to a buffer. The unpacked size depends on the data type
		// but right now the largest thing we could unpack is an int64 so make the buffer that big.
		char buffer[8];
		src += unpack_custom_attr_value(src, type, buffer, image);
		
		namedParameterHandler(output, paramName, type, &buffer[0]);
	}
}

guint32 unity_mono_metadata_string_length(UnityMonoMetadataString* string)
{
	if(string == NULL) return 0;
	return mono_metadata_decode_blob_size(string, &string);
}

void unity_mono_metadata_string_copy(UnityMonoMetadataString* string, char* buffer, size_t bufferSize)
{
	if(bufferSize == 0) return;
	if(string == NULL)
	{
		buffer[0] = 0;
		return;
	}
	guint32 strLen = mono_metadata_decode_blob_size(string, &string);
	if(strLen + 1 > bufferSize)
		strLen = bufferSize - 1;
	memcpy(buffer, string, strLen);
	buffer[strLen + 1] = 0;
}
