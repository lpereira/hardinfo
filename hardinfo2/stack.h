/*
 * Simple Pascal Compiler
 * Stack
 *
 * Copyright (c) 2007-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
 */
#ifndef __STACK_H__
#define __STACK_H__

#include <glib.h>

typedef struct _Stack	Stack;

struct _Stack {
	GSList *_stack;
};


Stack		*stack_new(void);
void		 stack_free(Stack *stack);

gboolean	 stack_is_empty(Stack *stack);
void		 stack_push(Stack *stack, gpointer data);
gpointer	 stack_pop(Stack *stack);
gpointer	 stack_peek(Stack *stack);

#endif	/* __STACK_H__ */
