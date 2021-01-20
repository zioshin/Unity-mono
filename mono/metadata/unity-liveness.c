#include <config.h>
#include <glib.h>
#include <mono/metadata/class-internals.h>
#include <mono/metadata/domain-internals.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/object.h>
#include <mono/metadata/tabledefs.h>
#include <mono/utils/mono-error.h>

typedef struct _LivenessState LivenessState;
typedef struct _custom_growable_array {
	gpointer *pdata;
	guint len;  // used
	guint size; // reserved
} custom_growable_array;

#define k_block_size (8 * 1024)
#define k_array_elements_per_block ((k_block_size - 2 * sizeof (guint) - sizeof (gpointer)) / sizeof (gpointer))

typedef struct _custom_array_block custom_array_block;

typedef struct _custom_array_block {
	gpointer *next_item;
	custom_array_block *next_block;
	gpointer p_data[k_array_elements_per_block];
} custom_array_block;

typedef struct _custom_block_array_iterator custom_block_array_iterator;

typedef struct _custom_growable_block_array {
	custom_array_block *first_block;
	custom_array_block *current_block;
	custom_block_array_iterator *iterator;
} custom_growable_block_array;

typedef struct _custom_block_array_iterator {
	custom_growable_block_array *array;
	custom_array_block *current_block;
	gpointer *current_position;
} custom_block_array_iterator;


typedef void (*register_object_callback) (gpointer *arr, int size, void *callback_userdata);
typedef void (*WorldStateChanged) ();
typedef void *(*ReallocateArray) (void *ptr, int size, void *callback_userdata);

struct _LivenessState {
	custom_growable_block_array *all_objects;

	MonoClass *filter;

	custom_growable_array *process_array;
	guint initial_alloc_count;

	void *callback_userdata;

	register_object_callback filter_callback;
	ReallocateArray reallocateArray;
	guint traverse_depth; // track recursion. Prevent stack overflow by limiting recursion
};

custom_growable_block_array *
block_array_create (LivenessState *state)
{
	custom_growable_block_array *array = g_new0 (custom_growable_block_array, 1);
	array->current_block = state->reallocateArray (NULL, k_block_size, state->callback_userdata);
	array->current_block->next_block = NULL;
	array->current_block->next_item = array->current_block->p_data;
	array->first_block = array->current_block;

	array->iterator = g_new0 (custom_block_array_iterator, 1);
	array->iterator->array = array;
	array->iterator->current_block = array->first_block;
	array->iterator->current_position = array->first_block->p_data;
	return array;
}

void
block_array_push_back (custom_growable_block_array *block_array, gpointer value, LivenessState *state)
{
	if (block_array->current_block->next_item == block_array->current_block->p_data + k_array_elements_per_block) {
		block_array->current_block->next_block = state->reallocateArray (NULL, k_block_size, state->callback_userdata);
		block_array->current_block = block_array->current_block->next_block;
		block_array->current_block->next_block = NULL;
		block_array->current_block->next_item = block_array->current_block->p_data;
	}
	*block_array->current_block->next_item++ = value;
}

void
block_array_reset_iterator (custom_growable_block_array *array)
{
	array->iterator->current_block = array->first_block;
	array->iterator->current_position = array->first_block->p_data;
}

gpointer
block_array_next (custom_growable_block_array *block_array)
{
	custom_block_array_iterator *iterator = block_array->iterator;
	if (iterator->current_position != iterator->current_block->next_item)
		return *iterator->current_position++;
	if (iterator->current_block->next_block == NULL)
		return NULL;
	iterator->current_block = iterator->current_block->next_block;
	iterator->current_position = iterator->current_block->p_data;
	if (iterator->current_position == iterator->current_block->next_item)
		return NULL;
	return *iterator->current_position++;
}

void
block_array_destroy (custom_growable_block_array *block_array, LivenessState *state)
{
	custom_array_block *block = block_array->first_block;
	while (block != NULL) {
		void *data_block = block;
		block = block->next_block;
		state->reallocateArray (data_block, 0, state->callback_userdata);
	}
	g_free (block_array->iterator);
	g_free (block_array);
}


#define array_at_index(array, index) (array)->pdata[(index)]

#if defined(HAVE_SGEN_GC)
void
sgen_stop_world (int generation);
void
sgen_restart_world (int generation);
#elif defined(HAVE_BOEHM_GC)
#ifdef HAVE_BDWGC_GC
extern void
GC_stop_world_external ();
extern void
GC_start_world_external ();
#else
void
GC_stop_world_external ()
{
	g_assert_not_reached ();
}
void
GC_start_world_external ()
{
	g_assert_not_reached ();
}
#endif
#else
#error need to implement liveness GC API
#endif

gboolean
array_is_full (custom_growable_array *array)
{
	return array->size == array->len;
}

custom_growable_array *
array_create (LivenessState *state, guint reserved_size)
{
	custom_growable_array *array = g_new0 (custom_growable_array, 1);

	array->pdata = NULL;
	array->len = 0;
	array->size = 0;

	if (reserved_size > 0) {
		array->pdata = state->reallocateArray (NULL, reserved_size * sizeof (gpointer), state->callback_userdata);
		array->size = reserved_size;
	}

	return (custom_growable_array *)array;
}

void
array_destroy (custom_growable_array *array, LivenessState *state)
{
	array->pdata = state->reallocateArray (array->pdata, 0, state->callback_userdata);
	g_free (array);
}

void
array_push_back (custom_growable_array *array, gpointer value)
{
	g_assert (!array_is_full (array));
	array->pdata[array->len] = value;
	array->len++;
}

gpointer
array_pop_back (custom_growable_array *array)
{
	array->len--;
	return array->pdata[array->len];
}

void
array_clear (custom_growable_array *array)
{
	array->len = 0;
}

void
array_reserve (LivenessState *state, custom_growable_array *array, guint size)
{
	array->pdata = state->reallocateArray (array->pdata, size * sizeof (gpointer), state->callback_userdata);
	array->size = size;
}

void
array_grow (LivenessState *state, custom_growable_array *array)
{
	array->pdata = state->reallocateArray (array->pdata, array->size * 2 * sizeof (gpointer), state->callback_userdata);
	array->size = array->size * 2;
}

/* number of sub elements of an array to process before recursing
 * we take a depth first approach to use stack space rather than re-allocating
 * processing array which requires restarting world to ensure allocator lock is not held
*/
const int kArrayElementsPerChunk = 256;

/* how far we recurse processing array elements before we stop. Prevents stack overflow */
const int kMaxTraverseRecursionDepth = 128;

/* Liveness calculation */
MONO_API LivenessState *
mono_unity_liveness_allocate_struct (MonoClass *filter, guint max_count, register_object_callback callback, void *callback_userdata, ReallocateArray reallocateArray);
MONO_API void
mono_unity_liveness_stop_gc_world ();
MONO_API void
mono_unity_liveness_finalize (LivenessState *state);
MONO_API void
mono_unity_liveness_start_gc_world ();
MONO_API void
mono_unity_liveness_free_struct (LivenessState *state);

MONO_API void
mono_unity_liveness_calculation_from_root (MonoObject *root, LivenessState *state);
MONO_API void
mono_unity_liveness_calculation_from_statics (LivenessState *state);

#define MARK_OBJ(obj)                                                       \
	do {                                                                    \
		(obj)->vtable = (MonoVTable *)(((gsize) (obj)->vtable) | (gsize)1); \
	} while (0)

#define CLEAR_OBJ(obj)                                                       \
	do {                                                                     \
		(obj)->vtable = (MonoVTable *)(((gsize) (obj)->vtable) & ~(gsize)1); \
	} while (0)

#define IS_MARKED(obj) \
	(((gsize) (obj)->vtable) & (gsize)1)

#define GET_VTABLE(obj) \
	((MonoVTable *)(((gsize) (obj)->vtable) & ~(gsize)1))

void
mono_filter_objects (LivenessState *state);

void
mono_reset_state (LivenessState *state)
{
	array_clear (state->process_array);
}

void
array_safe_grow (LivenessState *state, custom_growable_array *array)
{
	array_grow (state, array);
}

static gboolean
should_process_value (MonoObject *val, MonoClass *filter)
{
	MonoClass *val_class = GET_VTABLE (val)->klass;
	if (filter &&
		!mono_class_has_parent (val_class, filter))
		return FALSE;

	return TRUE;
}

static void
mono_traverse_array (MonoArray *array, LivenessState *state);
static void
mono_traverse_object (MonoObject *object, LivenessState *state);
static void
mono_traverse_gc_desc (MonoObject *object, LivenessState *state);
static void
mono_traverse_objects (LivenessState *state);

static void
mono_traverse_generic_object (MonoObject *object, LivenessState *state)
{
#ifdef HAVE_SGEN_GC
	gsize gc_desc = 0;
#else
	gsize gc_desc = (gsize) (GET_VTABLE (object)->gc_descr);
#endif

	if (gc_desc & (gsize)1)
		mono_traverse_gc_desc (object, state);
	else if (GET_VTABLE (object)->klass->rank)
		mono_traverse_array ((MonoArray *)object, state);
	else
		mono_traverse_object (object, state);
}

static gboolean
mono_add_process_object (MonoObject *object, LivenessState *state)
{
	if (object && !IS_MARKED (object)) {
		gboolean has_references = GET_VTABLE (object)->klass->has_references;
		if (has_references || should_process_value (object, state->filter)) {
			block_array_push_back (state->all_objects, object, state);
			MARK_OBJ (object);
		}
		// Check if klass has further references - if not skip adding
		if (has_references) {
			if (array_is_full (state->process_array))
				array_safe_grow (state, state->process_array);
			array_push_back (state->process_array, object);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
mono_field_can_contain_references (MonoClassField *field)
{
	if (MONO_TYPE_ISSTRUCT (field->type))
		return TRUE;
	if (field->type->attrs & FIELD_ATTRIBUTE_LITERAL)
		return FALSE;
	if (field->type->type == MONO_TYPE_STRING)
		return FALSE;
	return MONO_TYPE_IS_REFERENCE (field->type);
}

static gboolean
mono_traverse_object_internal (MonoObject *object, gboolean isStruct, MonoClass *klass, LivenessState *state)
{
	guint32 i;
	MonoClassField *field;
	MonoClass *p;
	gboolean added_objects = FALSE;

	g_assert (object);

	// subtract the added offset for the vtable. This is added to the offset even though it is a struct
	if (isStruct)
		object--;

	for (p = klass; p != NULL; p = p->parent) {
		if (p->size_inited == 0)
			continue;
		for (i = 0; i < mono_class_get_field_count (p); i++) {
			field = &p->fields[i];
			if (field->type->attrs & FIELD_ATTRIBUTE_STATIC)
				continue;

			if (!mono_field_can_contain_references (field))
				continue;

			if (MONO_TYPE_ISSTRUCT (field->type)) {
				char *offseted = (char *)object;
				offseted += field->offset;
				if (field->type->type == MONO_TYPE_GENERICINST) {
					g_assert (field->type->data.generic_class->cached_class);
					added_objects |= mono_traverse_object_internal ((MonoObject *)offseted, TRUE, field->type->data.generic_class->cached_class, state);
				} else
					added_objects |= mono_traverse_object_internal ((MonoObject *)offseted, TRUE, field->type->data.klass, state);
				continue;
			}

			if (field->offset == -1) {
				g_assert_not_reached ();
			} else {
				MonoObject *val = NULL;
				MonoVTable *vtable = NULL;
				mono_field_get_value (object, field, &val);
				added_objects |= mono_add_process_object (val, state);
			}
		}
	}

	return added_objects;
}

static void
mono_traverse_object (MonoObject *object, LivenessState *state)
{
	mono_traverse_object_internal (object, FALSE, GET_VTABLE (object)->klass, state);
}

static void
mono_traverse_gc_desc (MonoObject *object, LivenessState *state)
{
#define WORDSIZE ((int)sizeof (gsize) * 8)
	int i = 0;
	gsize mask = (gsize) (GET_VTABLE (object)->gc_descr);

	g_assert (mask & (gsize)1);

	for (i = 0; i < WORDSIZE - 2; i++) {
		gsize offset = ((gsize)1 << (WORDSIZE - 1 - i));
		if (mask & offset) {
			MonoObject *val = *(MonoObject **)(((char *)object) + i * sizeof (void *));
			mono_add_process_object (val, state);
		}
	}
}

static void
mono_traverse_objects (LivenessState *state)
{
	int i = 0;
	MonoObject *object = NULL;

	state->traverse_depth++;
	while (state->process_array->len > 0) {
		object = array_pop_back (state->process_array);
		mono_traverse_generic_object (object, state);
	}
	state->traverse_depth--;
}

static gboolean
should_traverse_objects (size_t index, gint32 recursion_depth)
{
	// Add kArrayElementsPerChunk objects at a time and then traverse
	return ((index + 1) & (kArrayElementsPerChunk - 1)) == 0 &&
		   recursion_depth < kMaxTraverseRecursionDepth;
}

static void
mono_traverse_array (MonoArray *array, LivenessState *state)
{
	size_t i = 0;
	gboolean has_references;
	MonoObject *object = (MonoObject *)array;
	MonoClass *element_class;
	int32_t elementClassSize;
	size_t array_length;

	g_assert (object);

	element_class = GET_VTABLE (object)->klass->element_class;
	has_references = !mono_class_is_valuetype (element_class);
	g_assert (element_class->size_inited != 0);

	for (i = 0; i < mono_class_get_field_count (element_class); i++) {
		has_references |= mono_field_can_contain_references (&element_class->fields[i]);
	}

	if (!has_references)
		return;

	array_length = mono_array_length (array);
	if (element_class->valuetype) {
		size_t items_processed = 0;
		elementClassSize = mono_class_array_element_size (element_class);
		for (i = 0; i < array_length; i++) {
			MonoObject *object = (MonoObject *)mono_array_addr_with_size (array, elementClassSize, i);
			if (mono_traverse_object_internal (object, 1, element_class, state))
				items_processed++;

			if (should_traverse_objects (items_processed, state->traverse_depth))
				mono_traverse_objects (state);
		}
	} else {
		size_t items_processed = 0;
		for (i = 0; i < array_length; i++) {
			MonoObject *val = mono_array_get (array, MonoObject *, i);
			if (mono_add_process_object (val, state))
				items_processed++;

			if (should_traverse_objects (items_processed, state->traverse_depth))
				mono_traverse_objects (state);
		}
	}
}

void
mono_filter_objects (LivenessState *state)
{
	gpointer filtered_objects[64];
	gint num_objects = 0;

	gpointer value = block_array_next (state->all_objects);
	while (value != NULL) {
		MonoObject *object = value;
		if (should_process_value (object, state->filter))
			filtered_objects[num_objects++] = object;
		if (num_objects == 64) {
			state->filter_callback (filtered_objects, 64, state->callback_userdata);
			num_objects = 0;
		}
		value = block_array_next (state->all_objects);
	}

	if (num_objects != 0)
		state->filter_callback (filtered_objects, num_objects, state->callback_userdata);
}

/**
 * mono_unity_liveness_calculation_from_statics:
 *
 * Returns an array of MonoObject* that are reachable from the static roots
 * in the current domain and derive from @filter (if not NULL).
 */
void
mono_unity_liveness_calculation_from_statics (LivenessState *liveness_state)
{
	guint i, j;
	MonoDomain *domain = mono_domain_get ();

	mono_reset_state (liveness_state);

	for (i = 0; i < domain->class_vtable_array->len; ++i) {
		MonoVTable *vtable = (MonoVTable *)g_ptr_array_index (domain->class_vtable_array, i);
		MonoClass *klass = vtable->klass;
		MonoClassField *field;
		if (!klass)
			continue;
		if (!klass->has_static_refs)
			continue;
		if (klass->image == mono_defaults.corlib)
			continue;
		if (klass->size_inited == 0)
			continue;
		for (j = 0; j < mono_class_get_field_count (klass); j++) {
			field = &klass->fields[j];
			if (!(field->type->attrs & FIELD_ATTRIBUTE_STATIC))
				continue;
			if (!mono_field_can_contain_references (field))
				continue;
			// shortcut check for special statics
			if (field->offset == -1)
				continue;

			if (MONO_TYPE_ISSTRUCT (field->type)) {
				char *offseted = (char *)mono_vtable_get_static_field_data (vtable);
				offseted += field->offset;
				if (field->type->type == MONO_TYPE_GENERICINST) {
					g_assert (field->type->data.generic_class->cached_class);
					mono_traverse_object_internal ((MonoObject *)offseted, TRUE, field->type->data.generic_class->cached_class, liveness_state);
				} else {
					mono_traverse_object_internal ((MonoObject *)offseted, TRUE, field->type->data.klass, liveness_state);
				}
			} else {
				MonoError error;
				MonoObject *val = NULL;

				mono_field_static_get_value_checked (mono_class_vtable (domain, klass), field, &val, &error);

				if (val && mono_error_ok (&error)) {
					mono_add_process_object (val, liveness_state);
				}
				mono_error_cleanup (&error);
			}
		}
	}
	mono_traverse_objects (liveness_state);
	//Filter objects and call callback to register found objects
	mono_filter_objects (liveness_state);
}

void
mono_unity_liveness_add_object_callback (gpointer *objs, gint count, void *arr)
{
	int i;
	custom_growable_array *objects = (custom_growable_array *)arr;
	for (i = 0; i < count; i++) {
		if (objects->size > objects->len)
			objects->pdata[objects->len++] = objs[i];
	}
}

/**
 * mono_unity_liveness_calculation_from_root:
 *
 * Returns an array of MonoObject* that are reachable from @root
 * in the current domain and derive from @filter (if not NULL).
 */
void
mono_unity_liveness_calculation_from_root (MonoObject *root, LivenessState *liveness_state)
{
	mono_reset_state (liveness_state);

	array_push_back (liveness_state->process_array, root);

	mono_traverse_objects (liveness_state);

	//Filter objects and call callback to register found objects
	mono_filter_objects (liveness_state);
}

LivenessState *
mono_unity_liveness_allocate_struct (MonoClass *filter, guint max_count, register_object_callback callback, void *callback_userdata, ReallocateArray reallocateArray)
{
	LivenessState *state = NULL;

	// construct liveness_state;
	// allocate memory for the following structs
	// all_objects: contains a list of all referenced objects to be able to clean the vtable bits after the traversal
	// process_array. array that contains the objcets that should be processed. this should run depth first to reduce memory usage
	// if all_objects run out of space, run through list, add objects that match the filter, clear bit in vtable and then clear the array.

	state = g_new0 (LivenessState, 1);
	max_count = max_count < 1000 ? 1000 : max_count;

	state->filter = filter;
	state->traverse_depth = 0;

	state->callback_userdata = callback_userdata;
	state->filter_callback = callback;
	state->reallocateArray = reallocateArray;

	state->all_objects = block_array_create (state);
	state->process_array = array_create (state, max_count);

	return state;
}

void
mono_unity_liveness_finalize (LivenessState *state)
{
	block_array_reset_iterator (state->all_objects);
	gpointer it = block_array_next (state->all_objects);
	while (it != NULL) {
		MonoObject *object = it;
		CLEAR_OBJ (object);
		it = block_array_next (state->all_objects);
	}
}

void
mono_unity_liveness_free_struct (LivenessState *state)
{
	//cleanup the liveness_state
	block_array_destroy (state->all_objects, state);
	array_destroy (state->process_array, state);
	g_free (state);
}

void
mono_unity_liveness_stop_gc_world ()
{
#if defined(HAVE_SGEN_GC)
	sgen_stop_world (1);
#elif defined(HAVE_BOEHM_GC)
	GC_stop_world_external ();
#else
#error need to implement liveness GC API
#endif
}

void
mono_unity_liveness_start_gc_world ()
{
#if defined(HAVE_SGEN_GC)
	sgen_restart_world (1);
#elif defined(HAVE_BOEHM_GC)
	GC_start_world_external ();
#else
#error need to implement liveness GC API
#endif
}
