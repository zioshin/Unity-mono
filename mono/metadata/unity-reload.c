#include <config.h>
#include <glib.h>
#include <mono/metadata/object.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/tabledefs.h>
#include <mono/metadata/class-internals.h>
#include <mono/metadata/domain-internals.h>
#include <mono/utils/mono-error.h>

MONO_API void mono_unity_reset_statics_from_images(MonoImage** image_array, unsigned int image_array_len);

/**
 * mono_unity_reset_statics_from_images:
 *
 * Reset all static fields of all classes loaded into a current domain from specified imaged.
 * The value of such statics is set to default 0 value.
 */
void mono_unity_reset_statics_from_images(MonoImage** image_array, unsigned int image_array_len)
{
	guint i, j;
	MonoDomain* domain = mono_domain_get();

	mono_loader_lock();

	// Mark images for fast check if class belongs to one of those images
	for (i = 0; i < image_array_len; ++i)
		image_array[i]->user_info = (void*)((gsize)image_array[i]->user_info | 1);

	for (i = 0; i < domain->class_vtable_array->len; ++i)
	{
		MonoVTable* vtable = (MonoVTable *)g_ptr_array_index (domain->class_vtable_array, i);
		MonoClass* klass = vtable->klass;
		MonoClassField *field;
		if (!klass)
			continue;
		if (klass->image == mono_defaults.corlib)
			continue;
		if (klass->size_inited == 0)
			continue;
		if(!((gsize)klass->image->user_info & 1))
			continue;

		for (j = 0; j < mono_class_get_field_count (klass); j++)
		{
			field = &klass->fields[j];
			if (!(field->type->attrs & FIELD_ATTRIBUTE_STATIC))
				continue;

			// See field_is_special_static.
			// ThreadStatic and ContextStatic fields must be handled separately -
			// mono_field_static_set_value can reset them only on a current thread.
			//if (field->offset == -1)
			//	continue;

			if (MONO_TYPE_ISSTRUCT(field->type))
			{
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
			}
			else
			{
				mono_field_static_set_value (vtable, field, 0);
			}
		}

		//// Invoke type initializer
		//iter = NULL;
		//while ((cctor_method = mono_class_get_methods(klass, &iter)))
		//{
		//	int paramCount;
		//	MonoMethodSignature *signature = mono_method_signature(cctor_method);
		//	if (!signature)
		//		continue;
		//	paramCount = mono_signature_get_param_count(signature);
		//	if (!strcmp(".cctor", mono_method_get_name(cctor_method)) && signature && paramCount == 0)
		//		break;
		//}
		//if (cctor_method)
		//	mono_runtime_invoke(cctor_method, NULL, NULL, NULL);
	}

	// Unmark images
	for (i = 0; i < image_array_len; ++i)
		image_array[i]->user_info = (void*)((gsize)image_array[i]->user_info & ~(gsize)1);

	mono_loader_unlock();
}
