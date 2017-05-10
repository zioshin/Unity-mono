#include "unity_memory_info.h"
#include <mono/metadata/object.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/tabledefs.h>
#include <mono/metadata/class-internals.h>
#include <mono/metadata/domain-internals.h>

MonoManagedMemorySnapshot* mono_unity_capture_memory_snapshot()
{
	MonoManagedMemorySnapshot* snapshot;
	snapshot = g_new0(MonoManagedMemorySnapshot, 1);
	return snapshot;
}

void mono_unity_free_captured_memory_snapshot(MonoManagedMemorySnapshot* snapshot)
{
	g_free(snapshot);
}