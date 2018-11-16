#include <config.h>
#include <glib.h>
#include <mono/metadata/class-internals.h>
#include <mono/metadata/domain-internals.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/object.h>
#include <mono/metadata/tabledefs.h>
#include <mono/utils/mono-error.h>

typedef void (*MonoClassWithCCtorCallback) (MonoClass *klass, MonoMethod *cctor_method, void *user_data);

MONO_API void
mono_unity_reset_statics_from_images (MonoImage **image_array, unsigned int image_array_len, MonoClassWithCCtorCallback cctor_callback, void *user_data);

void
mono_unity_liveness_stop_gc_world (void *state);
void
mono_unity_liveness_start_gc_world (void *state);

void
ves_icall_System_GC_WaitForPendingFinalizers (void);

/**
 * mono_unity_reset_statics_from_images:
 *
 * Reset all static fields of all classes loaded into a current domain from specified imaged.
 * The value of such statics is set to default 0 value.
 */
void
mono_unity_reset_statics_from_images (MonoImage **image_array, unsigned int image_array_len, MonoClassWithCCtorCallback cctor_callback, void *user_data)
{
	guint i, j;
	MonoDomain *domain = mono_domain_get ();

	ves_icall_System_GC_WaitForPendingFinalizers ();
	mono_threads_abort_appdomain_threads (domain, -1);
	mono_threadpool_remove_domain_jobs (domain, -1);
	mono_domain_finalize (domain, -1);
	mono_unity_liveness_stop_gc_world (NULL);

	// Mark images for fast check if class belongs to one of those images.
	mono_loader_lock ();
	for (i = 0; i < image_array_len; ++i)
		image_array[i]->user_info = (void *)((gsize)image_array[i]->user_info | 1);
	mono_loader_unlock ();

	mono_domain_lock (domain);
	// Walk through all domain class instances and reset static fields to 0.
	for (i = 0; i < domain->class_vtable_array->len; ++i) {
		MonoVTable *vtable = (MonoVTable *)g_ptr_array_index (domain->class_vtable_array, i);
		MonoClass *klass = vtable->klass;
		MonoClassField *field;

		// Skip not initialized yet classes - they don't need reset yet.
		if (!vtable->initialized)
			continue;
		// Skip classes without static fields.
		if (!vtable->has_static_fields)
			continue;

		if (!klass)
			continue;
		if (klass->image == mono_defaults.corlib)
			continue;
		// Skip not initialized yet classes.
		if (klass->size_inited == 0)
			continue;
		// Skip classes from unmarked images.
		if (!((gsize)klass->image->user_info & 1))
			continue;

		for (j = 0; j < mono_class_get_field_count (klass); j++) {
			field = &klass->fields[j];
			// Only static fields.
			if (!(field->type->attrs & FIELD_ATTRIBUTE_STATIC))
				continue;
			// And not constants.
			if (field->type->attrs & FIELD_ATTRIBUTE_INIT_ONLY)
				continue;
			if (field->type->attrs & FIELD_ATTRIBUTE_LITERAL)
				continue;

			if (MONO_TYPE_ISSTRUCT (field->type)) {
				// TODO: Proper traverse recursively struct fields.
				//char* offseted = (char*)mono_vtable_get_static_field_data (vtable);
				//offseted += field->offset;
				//if (field->type->type == MONO_TYPE_GENERICINST)
				//{
				//	g_assert(field->type->data.generic_class->cached_class);
				//	mono_traverse_object_internal((MonoObject*)offseted, TRUE, field->type->data.generic_class->cached_class, liveness_state);
				//}
				//else
				//{
				//	mono_traverse_object_internal((MonoObject*)offseted, TRUE, field->type->data.klass, liveness_state);
				//}
				mono_field_static_set_value (vtable, field, 0);
			} else {
				mono_field_static_set_value (vtable, field, 0);
			}

			// vtable->initialized is guint8 and all checks in mono are in a form of "if (vtable->initialized)".
			// Mono sets value 1 and we temporary change it to another non-0 value to use in a callback later.
			if (cctor_callback && vtable->klass->has_cctor)
				vtable->initialized = 7;
		}
	}

	// Find class cctor and report back.
	if (cctor_callback) {
		for (i = 0; i < domain->class_vtable_array->len; ++i) {
			MonoClass *klass;
			MonoMethod *cctor_method;
			MonoVTable *vtable = (MonoVTable *)g_ptr_array_index (domain->class_vtable_array, i);
			if (vtable->initialized != 7)
				continue;

			klass = vtable->klass;
			cctor_method = mono_class_get_cctor (klass);
			if (cctor_method)
				cctor_callback (klass, cctor_method, user_data);

			vtable->initialized = 1;
		}
	}
	mono_domain_unlock (domain);

	// Unmark images back.
	mono_loader_lock ();
	for (i = 0; i < image_array_len; ++i)
		image_array[i]->user_info = (void *)((gsize)image_array[i]->user_info & ~(gsize)1);
	mono_loader_unlock ();

	mono_unity_liveness_start_gc_world (NULL);
}
