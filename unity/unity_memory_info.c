#include "unity_memory_info.h"
#include <mono/metadata/assembly.h>
#include <mono/metadata/class.h>
#include <mono/metadata/class-internals.h>
#include <mono/metadata/image.h>
#include <mono/metadata/metadata-internals.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/tabledefs.h>
#include <mono/metadata/tokentype.h>
#include <stdlib.h>

#include <glib.h>

typedef struct CollectMetadataContext
{
	GHashTable *allTypes;
	int currentIndex;
	MonoMetadataSnapshot* metadata;
} CollectMetadataContext;

static void CollectAssemblyMetaData (MonoAssembly *assembly, void *user_data)
{
	int i;
	CollectMetadataContext* context = (CollectMetadataContext*)user_data;
	MonoImage* image = mono_assembly_get_image(assembly);
	MonoTableInfo *tdef = &image->tables [MONO_TABLE_TYPEDEF];

	for(i = 0; i < tdef->rows-1; ++i)
	{
		MonoClass* klass = mono_class_get (image, (i + 2) | MONO_TOKEN_TYPE_DEF);

		if(klass->inited)
			g_hash_table_insert(context->allTypes, klass, (gpointer)(context->currentIndex++));
	}
}

static int FindClassIndex(GHashTable* hashTable, MonoClass* klass)
{
	gpointer value = g_hash_table_lookup(hashTable, klass);

	if(!value)
		return -1;

	return (int)value;
}

static void AddMetadataType (gpointer key, gpointer value, gpointer user_data)
{
	MonoClass* klass = (MonoClass*)key;
	int index = (int)value;
	CollectMetadataContext* context = (CollectMetadataContext*)user_data;
	MonoMetadataSnapshot* metadata = context->metadata;
	MonoMetadataType* type = &metadata->types[index];

	if(klass->rank > 0)
	{
		type->flags = (MonoMetadataTypeFlags)(kArray | (kArrayRankMask & (klass->rank << 16)));
		type->baseOrElementTypeIndex = FindClassIndex(context->allTypes, mono_class_get_element_class(klass));
	}
	else
	{
		gpointer iter = NULL;
		int fieldCount = 0;
		MonoClassField* field;
		MonoClass* baseClass;
		MonoVTable* vtable;

        type->flags = (klass->valuetype || klass->byval_arg.type == MONO_TYPE_PTR) ? kValueType : kNone;
		type->fieldCount = 0;

		if(mono_class_num_fields(klass) > 0)
		{
			type->fields = g_new(MonoMetadataField, mono_class_num_fields(klass));

			while ((field = mono_class_get_fields (klass, &iter))) 
			{
				MonoMetadataField* metaField = &type->fields[type->fieldCount];
				metaField->typeIndex = FindClassIndex(context->allTypes, mono_class_get_element_class(klass));

				// This will happen if fields type is not initialized
                // It's OK to skip it, because it means the field is guaranteed to be null on any object
				if (metaField->typeIndex == -1)
					continue;

				// literals have no actual storage, and are not relevant in this context.
				if((field->type->attrs & FIELD_ATTRIBUTE_LITERAL) != 0)
					continue;

                metaField->isStatic = (field->type->attrs & FIELD_ATTRIBUTE_STATIC) != 0;
                metaField->offset = field->offset;
                metaField->name = field->name;
                type->fieldCount++;
			}
		}

		vtable = mono_class_try_get_vtable(mono_domain_get(), klass);

		type->staticsSize = vtable ? mono_class_data_size(klass) : 0; // Correct?
		type->statics = NULL;

		if (type->staticsSize > 0 && vtable && vtable->data)
		{
			type->statics = g_new0(uint8_t, type->staticsSize);
			memcpy(type->statics, vtable->data, type->staticsSize);
		}

		baseClass = mono_class_get_parent(klass);
		type->baseOrElementTypeIndex = baseClass ? FindClassIndex(context->allTypes, baseClass) : -1;
	}

	type->assemblyName = mono_class_get_image(klass)->assembly->aname.name;
	type->name = (char*)klass->name; // FIXME
	type->typeInfoAddress = (uint64_t)klass;
	type->size = (klass->valuetype) != 0 ? (mono_class_instance_size(klass) - sizeof(MonoObject)) : mono_class_instance_size(klass);
}


static void CollectMetadata(MonoMetadataSnapshot* metadata)
{
	CollectMetadataContext context;

	context.allTypes = g_hash_table_new(NULL, NULL);
	context.currentIndex = 0;
	context.metadata = metadata;
	
	mono_assembly_foreach((GFunc)CollectAssemblyMetaData, &context);

	metadata->typeCount = g_hash_table_size(context.allTypes);
	metadata->types = g_new0(MonoMetadataType, metadata->typeCount);

	g_hash_table_foreach(context.allTypes, AddMetadataType, &context);

	g_hash_table_destroy(context.allTypes);
}

static void FillRuntimeInformation(MonoRuntimeInformation* runtimeInfo)
{
    runtimeInfo->pointerSize = (uint32_t)(sizeof(void*));
    runtimeInfo->objectHeaderSize = (uint32_t)(sizeof(MonoObject));
    runtimeInfo->arrayHeaderSize = offsetof(MonoArray, vector);
    runtimeInfo->arraySizeOffsetInHeader = offsetof(MonoArray, max_length);
    runtimeInfo->arrayBoundsOffsetInHeader = offsetof(MonoArray, bounds);
    runtimeInfo->allocationGranularity = (uint32_t)(2 * sizeof(void*));
}

MonoManagedMemorySnapshot* mono_unity_capture_memory_snapshot()
{
	MonoManagedMemorySnapshot* snapshot;
	snapshot = g_new0(MonoManagedMemorySnapshot, 1);

	CollectMetadata(&snapshot->metadata);
	FillRuntimeInformation(&snapshot->runtimeInformation);

	return snapshot;
}

void mono_unity_free_captured_memory_snapshot(MonoManagedMemorySnapshot* snapshot)
{
	g_free(snapshot);
}