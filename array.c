#include <stdio.h>
#include <stdlib.h> 
#include "debug.h"
#include "array.h"

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
Array *Array_create(size_t element_size, size_t initial_max) {
	Array *array = malloc(sizeof(Array));
	check_mem(array);

	array->max = initial_max;
	check(array->max > 0, "You must set an array initial_max > 0.");

	array->contents = calloc(initial_max, sizeof(void*));
	check_mem(array->contents);

	array->end = 0;
	array->element_size = element_size;
	array->expand_rate = DEFAULT_EXPAND_RATE;

	return array;
error:
	if (array) {
		free(array);
	}
	return NULL;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void Array_clear(Array *array) {
	int i = 0;
	if (array->element_size > 0) {
		for (i = 0; i < array->max; i++) {
			if (array->contents[i] != NULL) {
				free(array->contents[i]);
			}
		}
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline int 
Array_resize(Array *array, size_t new_size) 
{
	array->max = new_size;
	check(array->max > 0, "New array size must be > 0.");

	void *contents = realloc(array->contents, array->max * sizeof(void*));
	check_mem(contents);

	array->contents = contents;

	return 0;
error:
	return -1;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
int Array_expand(Array *array) {
	size_t old_max = array->max;
	check(Array_resize(array, array->max + array->expand_rate) == 0,
			"Failed to expand array to new size: %d",
			array->max + (int)array->expand_rate);

	memset(array->contents + old_max, 0, array->expand_rate + 1);

	return 0;
error:
	return -1;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
int Array_contract(Array *array) {
	int expand_rate = (int)array->expand_rate;
	int new_size = array->end < expand_rate ? expand_rate : array->end;

	return Array_resize(array, new_size + 1);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void Array_destroy(Array *array) {
	if(array) {
		if(array->contents) {
			free(array->contents);
		}
		free(array);
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void Array_clear_destroy(Array *array) {
	Array_clear(array);
	Array_destroy(array);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
int Array_push(Array *array, void *el) {
	array->contents[array->end] = el;
	array->end++;

	if (Array_end(array) >= Array_max(array)) {
		return Array_expand(array);
	} else {
		return 0;
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void *Array_pop(Array *array) {
	check(array->end - 1 >= 0, "Attempt to pop from empty array.");

	void *el = Array_remove(array, array->end - 1);
	array->end--;

	if (Array_end(array) > (int)array->expand_rate && Array_end(array) % array->expand_rate) {
		Array_contract(array);
	}

	return el;
error:
	return NULL;
}
