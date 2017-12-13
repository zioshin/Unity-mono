#ifndef __IL2CPP_MONO_DEBUGGER_OPAQUE_TYPES_H__
#define __IL2CPP_MONO_DEBUGGER_OPAQUE_TYPES_H__

#if defined(RUNTIME_IL2CPP)
#include "il2cpp-class-internals.h"
#include "il2cpp-object-internals.h"
#include "vm-utils/Debugger.h"
#endif // RUNTIME_IL2CPP
//#include <mono/metadata/handle.h>

#define IL2CPP_MONO_PUBLIC_KEY_TOKEN_LENGTH	17

/* IL offsets used to mark the sequence points belonging to method entry/exit events */
#define METHOD_ENTRY_IL_OFFSET -1
#define METHOD_EXIT_IL_OFFSET 0xffffff

#define NOT_IMPLEMENTED do { g_assert_not_reached (); } while (0)

typedef enum {
	MONO_THREAD_FLAG_DONT_MANAGE = 1, // Don't wait for or abort this thread
	MONO_THREAD_FLAG_NAME_SET = 2, // Thread name set from managed code
	MONO_THREAD_FLAG_APPDOMAIN_ABORT = 4, // Current requested abort originates from appdomain unload
} MonoThreadFlags;

typedef enum {
	ThreadState_Running = 0x00000000,
	ThreadState_SuspendRequested = 0x00000002,
	ThreadState_Background = 0x00000004,
	ThreadState_Unstarted = 0x00000008,
	ThreadState_Stopped = 0x00000010,
	ThreadState_WaitSleepJoin = 0x00000020,
	ThreadState_Suspended = 0x00000040,
	ThreadState_AbortRequested = 0x00000080,
	ThreadState_Aborted = 0x00000100
} MonoThreadState;

//Converted to il2cpp types
#define MonoType Il2CppType
#define MonoClass Il2CppClass
#define MonoImage Il2CppImage
#define MonoMethod MethodInfo
#define MonoClassField FieldInfo
#define MonoArrayType Il2CppArrayType
#define MonoGenericParam Il2CppGenericParameter
#define MonoGenericInst Il2CppGenericInst
#define MonoGenericContext Il2CppGenericContext
#define MonoGenericClass Il2CppGenericClass
#define MonoGenericContainer Il2CppGenericContainer
#define MonoProperty PropertyInfo
#define MonoString Il2CppString
//#define MonoStringHandle Il2CppStringHandle
#define MonoArray Il2CppArraySize
//#define MonoArrayHandle Il2CppArraySizeHandle
#define MonoThread Il2CppThread
#define MonoInternalThread Il2CppInternalThread
#define MonoReflectionType Il2CppReflectionType
#define MonoProfiler Il2CppProfiler
#define MonoAssembly Il2CppAssembly
#define MonoAssembyName Il2CppAssemblyName
#define MonoMethodHeader Il2CppMethodHeaderInfo
#define MonoReflectionAssembly Il2CppReflectionAssembly
#define MonoReflectionAssemblyHandle Il2CppReflectionAssembly*
#define MonoAppDomain Il2CppAppDomain
#define MonoDomain Il2CppDomain
#define MonoDomainFunc Il2CppDomainFunc
#define MonoObject Il2CppObject
#define MonoObjectHandle Il2CppObject*
//#define MonoObjectHandleOut Il2CppObjectHandleOut
#define MonoVTable Il2CppVTable
#define MonoException Il2CppException
//#define MonoExceptionHandle Il2CppExceptionHandle
#define MonoMarshalByRefObject Il2CppMarshalByRefObject

//Unsupported in il2cpp, should never be referenced
#define MonoCustomAttrInfo #error Custom Attributes Not Supported
#define MonoCustomAttrEntry #error Custom Attributes Not Supported
#define CattrNamedArg #error Custom Attributes Not Supported
#define MonoJitTlsData #error Jit TLS Data Unsupported

//still stubs everywhere
typedef struct _Il2CppMonoMethodSignature Il2CppMonoMethodSignature;
typedef struct _Il2CppMonoRuntimeExceptionHandlingCallbacks Il2CppMonoRuntimeExceptionHandlingCallbacks;
typedef struct _Il2CppMonoStackFrameInfo Il2CppMonoStackFrameInfo;
typedef struct Il2CppDefaults Il2CppMonoDefaults;
typedef struct _Il2CppMonoMethodInflated Il2CppMonoMethodInflated;
typedef struct _Il2CppMonoTypeNameParse Il2CppMonoTypeNameParse;

struct _Il2CppMonoMethodInflated
{
	MonoMethod *declaring;
	MonoGenericContext context;
};

typedef enum {
	/* Normal managed frames */
	FRAME_TYPE_MANAGED = 0,
	/* Pseudo frame marking the start of a method invocation done by the soft debugger */
	FRAME_TYPE_DEBUGGER_INVOKE = 1,
	/* Frame for transitioning to native code */
	FRAME_TYPE_MANAGED_TO_NATIVE = 2,
	FRAME_TYPE_TRAMPOLINE = 3,
	/* Interpreter frame */
	FRAME_TYPE_INTERP = 4,
	/* Frame for transitioning from interpreter to managed code */
	FRAME_TYPE_INTERP_TO_MANAGED = 5,
	FRAME_TYPE_NUM = 6
} MonoStackFrameType;

struct _Il2CppMonoStackFrameInfo
{
	MonoStackFrameType type;
	MonoJitInfo *ji;
	MonoMethod *method;
	MonoMethod *actual_method;
	MonoDomain *domain;
	gboolean managed;
	gboolean async_context;
	int native_offset;
	int il_offset;
	gpointer interp_exit_data;
	gpointer interp_frame;
	gpointer lmf;
	guint32 unwind_info_len;
	guint8 *unwind_info;
	mgreg_t **reg_locations;
};

typedef struct _MonoContext
{
	void* dummy;
} _MonoContext;

typedef gboolean (*Il2CppMonoInternalStackWalk) (Il2CppMonoStackFrameInfo *frame, _MonoContext *ctx, gpointer data);

struct _Il2CppMonoRuntimeExceptionHandlingCallbacks
{
	void (*il2cpp_mono_walk_stack_with_state) (Il2CppMonoInternalStackWalk func, MonoThreadUnwindState *state, MonoUnwindOptions options, void *user_data);
};

struct _Il2CppMonoMethodSignature
{
	MonoType *ret;
	guint16 param_count;
	unsigned int generic_param_count : 16;
	unsigned int  call_convention     : 6;
	unsigned int  hasthis             : 1;
	MonoType **params;
};

struct _Il2CppMonoTypeNameParse
{
	MonoAssemblyName assembly;
	void *il2cppTypeNameParseInfo;
};

/*TYPED_HANDLE_DECL (MonoObject);
TYPED_HANDLE_DECL (Il2CppReflectionAssembly);*/
Il2CppMonoDefaults il2cpp_mono_defaults;

typedef void (*Il2CppMonoProfileFunc) (MonoProfiler *prof);
typedef void (*Il2CppMonoProfileAppDomainFunc) (MonoProfiler *prof, MonoDomain *domain);
typedef void (*Il2CppMonoProfileAppDomainResult) (MonoProfiler *prof, MonoDomain *domain, int result);
typedef void (*Il2CppMonoProfileAssemblyFunc) (MonoProfiler *prof, MonoAssembly *assembly);
typedef void (*Il2CppMonoProfileJitResult) (MonoProfiler *prof, MonoMethod *method, MonoJitInfo* jinfo, int result);
typedef void (*Il2CppMonoProfileAssemblyResult) (MonoProfiler *prof, MonoAssembly *assembly, int result);
typedef void (*Il2CppMonoProfileThreadFunc) (MonoProfiler *prof, uintptr_t tid);
typedef gboolean (*Il2CppMonoJitStackWalk) (Il2CppMonoStackFrameInfo *frame, MonoContext *ctx, gpointer data);
typedef void (*Il2CppDomainFunc) (MonoDomain *domain, void* user_data);

typedef void (*emit_assembly_load_callback)(void*, void*);
typedef void(*emit_type_load_callback)(void*, void*, void*);

void il2cpp_set_thread_state_background(MonoThread* thread);
void* il2cpp_domain_get_agent_info(MonoAppDomain* domain);
void il2cpp_domain_set_agent_info(MonoAppDomain* domain, void* agentInfo);
void il2cpp_start_debugger_thread();
void* il2cpp_gc_alloc_fixed(size_t size);
void il2cpp_gc_free_fixed(void* address);
const char* il2cpp_domain_get_name(MonoDomain* domain);

#endif
