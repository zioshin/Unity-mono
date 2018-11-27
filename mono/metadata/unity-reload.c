#include <config.h>
#include <glib.h>
#include <mono/metadata/class-internals.h>
#include <mono/metadata/domain-internals.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/object.h>
#include <mono/metadata/tabledefs.h>
#include <mono/utils/mono-error.h>

typedef void (*MonoClassWithStaticsCallback) (MonoClass *klass, MonoVTable *vtable, MonoMethod *cctor_method, void *user_data);

MONO_API void
mono_unity_enumerate_classes_with_statics (MonoImage **image_array, unsigned int image_array_len, MonoClassWithStaticsCallback cctor_callback, void *user_data);

MONO_API void
mono_unity_wait_for_pending_finalizers(void);

void
mono_unity_liveness_stop_gc_world (void *state);
void
mono_unity_liveness_start_gc_world (void *state);
gboolean
mono_threads_abort_other_appdomain_threads(MonoDomain *domain, int timeout, MonoInternalThread* exclude_thread);

void
ves_icall_System_GC_WaitForPendingFinalizers (void);

void
mono_unity_wait_for_pending_finalizers()
{
	ves_icall_System_GC_WaitForPendingFinalizers();
}

/**
 * mono_unity_enumerate_classes_with_statics:
 *
 * Enumerate all classes which has statics or static constructor and belong to the specified images.
 */
void
mono_unity_enumerate_classes_with_statics (MonoImage **image_array, unsigned int image_array_len, MonoClassWithStaticsCallback cctor_callback, void *user_data)
{
	if (cctor_callback == NULL)
		return;

	guint i, j;
	MonoDomain *domain = mono_domain_get ();

	// Mark images for fast check if class belongs to one of those images.
	if (image_array_len > 0) {
		mono_loader_lock();
		for (i = 0; i < image_array_len; ++i)
			image_array[i]->user_info = (void *)((gsize)image_array[i]->user_info | 1);
		mono_loader_unlock();
	}

	mono_domain_lock (domain);
	// Walk through all domain class instances and reset static fields to 0.
	for (i = 0; i < domain->class_vtable_array->len; ++i) {
		MonoVTable *vtable = (MonoVTable *)g_ptr_array_index (domain->class_vtable_array, i);
		MonoClass *klass = vtable->klass;
		MonoClassField *field;
		MonoMethod *cctor_method;
		gboolean has_static_field = FALSE;

		if (!klass)
			continue;
		// Skip not initialized yet classes.
		if (klass->size_inited == 0)
			continue;
		// Skip not initialized yet class instances - they don't need reset yet.
		if (!vtable->initialized)
			continue;

		// Skip classes from unmarked images.
		if (image_array_len != 0 && !((gsize)klass->image->user_info & 1))
			continue;

		//// Skip classes without static fields.
		//if (!vtable->has_static_fields)
		//	continue;

		//if (klass->image == mono_defaults.corlib)
		//	continue;
		// Skip empty string overwrite
		//if (klass == mono_defaults.string_class)
		//	continue;
		//if (strcmp(klass->name, "DateTime") == 0)
		//	continue;

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
			// Skip special statics - thread and context statics require special handling.
			if (field->offset == -1)
				continue;

			//if (MONO_TYPE_ISSTRUCT (field->type)) {
			//	// TODO: Proper traverse recursively struct fields.
			//	//char* offseted = (char*)mono_vtable_get_static_field_data (vtable);
			//	//offseted += field->offset;
			//	//if (field->type->type == MONO_TYPE_GENERICINST)
			//	//{
			//	//	g_assert(field->type->data.generic_class->cached_class);
			//	//	mono_traverse_object_internal((MonoObject*)offseted, TRUE, field->type->data.generic_class->cached_class, liveness_state);
			//	//}
			//	//else
			//	//{
			//	//	mono_traverse_object_internal((MonoObject*)offseted, TRUE, field->type->data.klass, liveness_state);
			//	//}
			//	mono_field_static_set_value (vtable, field, 0);
			//} else {
			//	mono_field_static_set_value (vtable, field, 0);
			//}

			// We only interested in one.
			has_static_field = TRUE;
			break;
		}

		cctor_method = mono_class_get_cctor(klass);

		// Report classes that have either static field or cctor.
		if (cctor_method || has_static_field)
			cctor_callback(klass, vtable, cctor_method, user_data);

	}
	mono_domain_unlock (domain);

	// Unmark images.
	if (image_array_len > 0) {
		mono_loader_lock();
		for (i = 0; i < image_array_len; ++i)
			image_array[i]->user_info = (void *)((gsize)image_array[i]->user_info & ~(gsize)1);
		mono_loader_unlock ();
	}
}
