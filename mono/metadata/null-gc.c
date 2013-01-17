/*
 * null-gc.c: GC implementation using malloc: will leak everything, just for testing.
 *
 * Copyright 2001-2003 Ximian, Inc (http://www.ximian.com)
 * Copyright 2004-2009 Novell, Inc (http://www.novell.com)
 */

#include "config.h"
#include <glib.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/gc-internal.h>

#ifdef HAVE_NULL_GC

#ifdef DEBUG_WRITE_BARRIERS

static char* heap_base;
static char* heap;
static const int heap_size = 1024*1024*1024;

static void heap_protect ()
{
	static DWORD protect_junk;
	if (!VirtualProtect(heap_base, heap_size, PAGE_READONLY,
		&protect_junk)) {
			DWORD last_error = GetLastError();
			abort ();
	}
}

static void heap_unprotect ()
{
	static DWORD protect_junk;
	if (!VirtualProtect(heap_base, heap_size, PAGE_READWRITE,
		&protect_junk)) {
			DWORD last_error = GetLastError();
			abort ();
	}
}

LONG CALLBACK GCVectoredHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	int i = 0;
	int in_allocd_block = 0; 
	char * addr = (char *) (ExceptionInfo -> ExceptionRecord
                                -> ExceptionInformation[1]);
	in_allocd_block = addr >= heap_base && addr <= (heap_base + heap_size);

	if (ExceptionInfo -> ExceptionRecord -> ExceptionCode == STATUS_ACCESS_VIOLATION) {
		if (in_allocd_block) {
			MonoJitInfo *ji;
			ji = mono_jit_info_table_find (mono_domain_get (), ExceptionInfo->ContextRecord->Eip);
			if (!ji)
			{
				int i = 0;
			}
			heap_unprotect ();
		}
		else
			i = 1;
	}
	else if (ExceptionInfo -> ExceptionRecord -> ExceptionCode == STATUS_SINGLE_STEP){
		if (in_allocd_block) {
			heap_protect ();
		}
		else {
			i = 1;
		}
	} else
		i = 2;

	return  in_allocd_block ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

#endif


#ifdef DEBUG_WRITE_BARRIERS
#define PROTECT_HEAP() heap_protect()
#define UNPROTECT_HEAP() heap_unprotect()
#else
#define PROTECT_HEAP()
#define UNPROTECT_HEAP()
#endif

void
mono_gc_base_init (void)
{
#ifdef DEBUG_WRITE_BARRIERS
	if (!heap_base) {
		heap = heap_base = VirtualAlloc(NULL, heap_size,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_READONLY);

		AddVectoredExceptionHandler (1, &GCVectoredHandler);
	}
#endif
}

void
mono_gc_collect_a_little (int micro_seconds)
{
}

void
mono_gc_collect (int generation)
{
}

int
mono_gc_max_generation (void)
{
	return 0;
}

int
mono_gc_get_generation  (MonoObject *object)
{
	return 0;
}

int
mono_gc_collection_count (int generation)
{
	return 0;
}

void
mono_gc_add_memory_pressure (gint64 value)
{
}

/* maybe track the size, not important, though */
gint64
mono_gc_get_used_size (void)
{
	return 1024*1024;
}

gint64
mono_gc_get_heap_size (void)
{
	return 2*1024*1024;
}

void
mono_gc_disable (void)
{
}

void
mono_gc_enable (void)
{
}

gboolean
mono_gc_is_gc_thread (void)
{
	return TRUE;
}

gboolean
mono_gc_register_thread (void *baseptr)
{
	return TRUE;
}

gboolean
mono_object_is_alive (MonoObject* o)
{
	return TRUE;
}

void
mono_gc_enable_events (void)
{
}

int
mono_gc_register_root (char *start, size_t size, void *descr)
{
	return TRUE;
}

void
mono_gc_deregister_root (char* addr)
{
}

void
mono_gc_weak_link_add (void **link_addr, MonoObject *obj, gboolean track)
{
	*link_addr = obj;
}

void
mono_gc_weak_link_remove (void **link_addr)
{
	*link_addr = NULL;
}

MonoObject*
mono_gc_weak_link_get (void **link_addr)
{
	return *link_addr;
}

void*
mono_gc_make_descr_for_string (gsize *bitmap, int numbits)
{
	return NULL;
}

void*
mono_gc_make_descr_for_object (gsize *bitmap, int numbits, size_t obj_size)
{
	return NULL;
}

void*
mono_gc_make_descr_for_array (int vector, gsize *elem_bitmap, int numbits, size_t elem_size)
{
	return NULL;
}

void*
mono_gc_make_descr_from_bitmap (gsize *bitmap, int numbits)
{
	return NULL;
}

void*
mono_gc_alloc_fixed (size_t size, void *descr)
{
#ifdef DEBUG_WRITE_BARRIERS
	char* old_heap = InterlockedExchangeAdd (&heap, size);
	return old_heap;
#else
	return g_malloc0 (size);
#endif
}

void
mono_gc_free_fixed (void* addr)
{
#ifdef DEBUG_WRITE_BARRIERS
#else
	g_free (addr);
#endif
}

void
mono_gc_wbarrier_set_field (MonoObject *obj, gpointer field_ptr, MonoObject* value)
{
	UNPROTECT_HEAP ();
	*(void**)field_ptr = value;
	PROTECT_HEAP ();
}

void
mono_gc_wbarrier_set_arrayref (MonoArray *arr, gpointer slot_ptr, MonoObject* value)
{
	UNPROTECT_HEAP ();
	*(void**)slot_ptr = value;
	PROTECT_HEAP ();
}

void
mono_gc_wbarrier_arrayref_copy (MonoArray *arr, gpointer slot_ptr, int count)
{
	/* no need to do anything */
}

void
mono_gc_wbarrier_generic_store (gpointer ptr, MonoObject* value)
{
	UNPROTECT_HEAP ();
	*(void**)ptr = value;
	PROTECT_HEAP ();
}

void
mono_gc_wbarrier_generic_store_ptr (gpointer ptr, gpointer value)
{
	UNPROTECT_HEAP ();
	*(void**)ptr = value;
	PROTECT_HEAP ();
}

void
mono_gc_wbarrier_generic_nostore (gpointer ptr)
{
}

void
mono_gc_wbarrier_value_copy (gpointer dest, gpointer src, int count, MonoClass *klass)
{
}

void
mono_gc_wbarrier_object (MonoObject* obj)
{
}

MonoMethod*
mono_gc_get_managed_allocator (MonoVTable *vtable, gboolean for_box)
{
	return NULL;
}

int
mono_gc_get_managed_allocator_type (MonoMethod *managed_alloc)
{
	return -1;
}

MonoMethod*
mono_gc_get_managed_allocator_by_type (int atype)
{
	return NULL;
}

guint32
mono_gc_get_managed_allocator_types (void)
{
	return 0;
}

void
mono_gc_add_weak_track_handle (MonoObject *obj, guint32 gchandle)
{
}

void
mono_gc_change_weak_track_handle (MonoObject *old_obj, MonoObject *obj, guint32 gchandle)
{
}

void
mono_gc_remove_weak_track_handle (guint32 gchandle)
{
}

GSList*
mono_gc_remove_weak_track_object (MonoDomain *domain, MonoObject *obj)
{
	return NULL;
}

void
mono_gc_clear_domain (MonoDomain *domain)
{
}


void
mono_gc_deregister_root_size (char* addr, size_t size)
{
}

int
mono_gc_register_root_wbarrier (char *start, size_t size, void *descr)
{
	return TRUE;
}

void
mono_gc_wbarrier_set_root (gpointer ptr, MonoObject *value)
{
	*(void**)ptr = value;
}

void
mono_gc_wbarrier_memcpy (gpointer dst, gpointer src, size_t size)
{
	UNPROTECT_HEAP ();
	memcpy (dst, src, size);
	PROTECT_HEAP ();
}

#ifdef DEBUG_WRITE_BARRIERS

#include <mono/metadata/method-builder.h>
#include <mono/metadata/opcodes.h>

#define OPDEF(a,b,c,d,e,f,g,h,i,j) \
	a = i,

enum {
#include "mono/cil/opcode.def"
	CEE_LAST
};

#undef OPDEF

static MonoMethod *write_barrier_method;

MonoMethod*
mono_gc_get_write_barrier (void)
{
	MonoMethod *res;
	MonoMethodBuilder *mb;
	MonoMethodSignature *sig;
#ifdef MANAGED_WBARRIER
	int label_no_wb, label_need_wb_1, label_need_wb_2, label2;
	int remset_var, next_var, dummy_var;

#ifdef HAVE_KW_THREAD
	int remset_offset = -1, stack_end_offset = -1;

	MONO_THREAD_VAR_OFFSET (remembered_set, remset_offset);
	MONO_THREAD_VAR_OFFSET (stack_end, stack_end_offset);
	g_assert (remset_offset != -1 && stack_end_offset != -1);
#endif
#endif

	// FIXME: Maybe create a separate version for ctors (the branch would be
	// correctly predicted more times)
	if (write_barrier_method)
		return write_barrier_method;

	/* Create the IL version of mono_gc_barrier_generic_store () */
	sig = mono_metadata_signature_alloc (mono_defaults.corlib, 1);
	sig->ret = &mono_defaults.void_class->byval_arg;
	sig->params [0] = &mono_defaults.int_class->byval_arg;

	mb = mono_mb_new (mono_defaults.object_class, "wbarrier", MONO_WRAPPER_WRITE_BARRIER);

#ifdef MANAGED_WBARRIER
	if (mono_runtime_has_tls_get ()) {
		/* ptr_in_nursery () check */
#ifdef ALIGN_NURSERY
		/*
		 * Masking out the bits might be faster, but we would have to use 64 bit
		 * immediates, which might be slower.
		 */
		mono_mb_emit_ldarg (mb, 0);
		mono_mb_emit_icon (mb, DEFAULT_NURSERY_BITS);
		mono_mb_emit_byte (mb, CEE_SHR_UN);
		mono_mb_emit_icon (mb, (mword)nursery_start >> DEFAULT_NURSERY_BITS);
		label_no_wb = mono_mb_emit_branch (mb, CEE_BEQ);
#else
		// FIXME:
		g_assert_not_reached ();
#endif

		/* Need write barrier if ptr >= stack_end */
		mono_mb_emit_ldarg (mb, 0);
		EMIT_TLS_ACCESS (mb, stack_end, stack_end_offset);
		label_need_wb_1 = mono_mb_emit_branch (mb, CEE_BGE_UN);

		/* Need write barrier if ptr < stack_start */
		dummy_var = mono_mb_add_local (mb, &mono_defaults.int_class->byval_arg);
		mono_mb_emit_ldarg (mb, 0);
		mono_mb_emit_ldloc_addr (mb, dummy_var);
		label_need_wb_2 = mono_mb_emit_branch (mb, CEE_BLE_UN);

		/* Don't need write barrier case */
		mono_mb_patch_branch (mb, label_no_wb);

		mono_mb_emit_byte (mb, CEE_RET);

		/* Need write barrier case */
		mono_mb_patch_branch (mb, label_need_wb_1);
		mono_mb_patch_branch (mb, label_need_wb_2);

		// remset_var = remembered_set;
		remset_var = mono_mb_add_local (mb, &mono_defaults.int_class->byval_arg);
		EMIT_TLS_ACCESS (mb, remset, remset_offset);
		mono_mb_emit_stloc (mb, remset_var);

		// next_var = rs->store_next
		next_var = mono_mb_add_local (mb, &mono_defaults.int_class->byval_arg);
		mono_mb_emit_ldloc (mb, remset_var);
		mono_mb_emit_ldflda (mb, G_STRUCT_OFFSET (RememberedSet, store_next));
		mono_mb_emit_byte (mb, CEE_LDIND_I);
		mono_mb_emit_stloc (mb, next_var);

		// if (rs->store_next < rs->end_set) {
		mono_mb_emit_ldloc (mb, next_var);
		mono_mb_emit_ldloc (mb, remset_var);
		mono_mb_emit_ldflda (mb, G_STRUCT_OFFSET (RememberedSet, end_set));
		mono_mb_emit_byte (mb, CEE_LDIND_I);
		label2 = mono_mb_emit_branch (mb, CEE_BGE);

		/* write barrier fast path */
		// *(rs->store_next++) = (mword)ptr;
		mono_mb_emit_ldloc (mb, next_var);
		mono_mb_emit_ldarg (mb, 0);
		mono_mb_emit_byte (mb, CEE_STIND_I);

		mono_mb_emit_ldloc (mb, next_var);
		mono_mb_emit_icon (mb, sizeof (gpointer));
		mono_mb_emit_byte (mb, CEE_ADD);
		mono_mb_emit_stloc (mb, next_var);

		mono_mb_emit_ldloc (mb, remset_var);
		mono_mb_emit_ldflda (mb, G_STRUCT_OFFSET (RememberedSet, store_next));
		mono_mb_emit_ldloc (mb, next_var);
		mono_mb_emit_byte (mb, CEE_STIND_I);

		/* write barrier slow path */
		mono_mb_patch_branch (mb, label2);
	}
#endif

	mono_mb_emit_ldarg (mb, 0);
	mono_mb_emit_icall (mb, mono_gc_wbarrier_generic_nostore);
	mono_mb_emit_byte (mb, CEE_RET);

	res = mono_mb_create_method (mb, sig, 16);
	mono_mb_free (mb);

	mono_loader_lock ();
	if (write_barrier_method) {
		/* Already created */
		mono_free_method (res);
	} else {
		/* double-checked locking */
		mono_memory_barrier ();
		write_barrier_method = res;
	}
	mono_loader_unlock ();

	return write_barrier_method;
}

#endif

#endif

