
#ifndef _ARRAY_H
#define _ARRAY_H

#include <stdio.h>
#include <stdlib.h> 
#include <assert.h>
#include "debug.h"

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
#define DEFAULT_EXPAND_RATE 50

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
typedef struct Array {
	int end, max;
	size_t element_size, expand_rate;
	void **contents;
} Array;

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
Array *Array_create(size_t element_size, size_t initial_max);
void Array_destroy(Array *array);
void Array_clear(Array *array);
int Array_expand(Array *array);
int Array_contract(Array *array);
int Array_push(Array *array, void *el);
void *Array_pop(Array *array);
void Array_clear_destroy(Array *array);
 
// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
#define Array_last(A) ((A)->contents[(A)->end - 1])
#define Array_first(A) ((A)->contents[0])
#define Array_end(A) ((A)->end)
#define Array_count(A) Array_end(A)
#define Array_max(A) ((A)->max)
#define Array_free(E) free((E))

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void 
Array_set(Array *array, int i, void *el) 
{
	check(i < array->max, "array attempt to set past max");
	if (i > array->end) {
		array->end = i;
	}
	array->contents[i] = el;
error:
	return;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void *
Array_get(Array *array, int i) 
{
	check(i < array->max, "array attempt to get past max");
	return array->contents[i];
error:
	return NULL;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void *
Array_remove(Array *array, int i) 
{
	void *el = array->contents[i];
	array->contents[i] = NULL;
	return el;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void *
Array_new(Array *array) 
{
	check(array->element_size > 0, "Can't use Array_new on 0 size arrays.");
	return calloc(1, array->element_size);
error:
	return NULL;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
#endif
