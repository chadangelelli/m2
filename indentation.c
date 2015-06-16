#include "indentation.h"
#include "debug.h"

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
IndentStack *IndentStack_create() {
	return calloc(1, sizeof(IndentStack));
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void IndentStack_increase(IndentStack *stack, int value) {
	IndentNode *node = calloc(1, sizeof(IndentNode));
	check_mem(node);

	node->value = value; 
	
	if(stack->first == NULL) {
		stack->first = node;
		stack->last = node;
	} else {
		node->next = stack->first;
		stack->first->prev = node;
		stack->first = node;
	}
	
	stack->length++;

error:
	return;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void IndentStack_decrease(IndentStack *stack) {
	IndentNode *node = stack->first;

	if (node == NULL)
		return;

    check(stack->first && stack->last, "Indent stack is empty.");
    check(node, "Indent stack node can't be NULL");

    if (node == stack->first && node == stack->last) {
        stack->first = NULL;
        stack->last = NULL;
    } else if (node == stack->first) {
        stack->first = node->next;
        check(stack->first != NULL, "Invalid indent stack, *first is NULL.");
        stack->first->prev = NULL;
    } else if (node == stack->last) {
        stack->last = node->prev;
        check(stack->last != NULL, "Invalid indent stack, *next is NULL.");
        stack->last->next = NULL;
    } else {
        IndentNode *after = node->next;
        IndentNode *before = node->prev;
        after->prev = before;
        before->next = after;
    }

    stack->length--;
    free(node);

error:
    return;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void IndentStack_print(IndentStack *stack) {
	printf("\t(%d) ==> [", stack->length);
	INDENT_EACH(stack, first, next, cur) {
		printf(" %d", (int)cur->value);
	}
	printf(" ]\n");
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void IndentStack_destroy(IndentStack *stack) {
	INDENT_EACH(stack, first, next, cur) {
		if(cur->prev) {
			free(cur->prev);
		}
	} 
	
	free(stack->last);
	free(stack);
}
