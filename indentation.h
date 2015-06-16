#ifndef _MANANA_INDENT_STACK_H
#define _MANANA_INDENT_STACK_H

#include <stdlib.h>

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
struct IndentNode;

typedef struct IndentNode {
	struct IndentNode *prev, *next;
	int value;
} IndentNode;

typedef struct IndentStack {
	IndentNode *first, *last;
	int length;
} IndentStack;

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
IndentStack *IndentStack_create();
void IndentStack_increase(IndentStack *stack, int value);
void IndentStack_decrease(IndentStack *stack);
void IndentStack_print(IndentStack *stack);
void IndentStack_destroy(IndentStack *stack);

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
#define MAX_INDENT 16

#define INDENT_EACH(STACK, FIRST, NEXT, CUR)\
	IndentNode *_node = NULL;\
	IndentNode *CUR = NULL;\
    for (CUR = _node = STACK->FIRST; _node != NULL; CUR = _node = _node->NEXT)

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
#endif
