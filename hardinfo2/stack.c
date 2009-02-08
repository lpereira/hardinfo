/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "stack.h"

Stack*
stack_new(void)
{
	Stack *stack;
	
	stack = g_new0(Stack, 1);
	stack->_stack = NULL;
	
	return stack;
}

void
stack_free(Stack *stack)
{
	g_return_if_fail(stack);
	
	g_slist_free(stack->_stack);
	g_free(stack);
}

gboolean
stack_is_empty(Stack *stack)
{
	g_return_val_if_fail(stack, TRUE);
	
	return stack->_stack == NULL;
}

void
stack_push(Stack *stack, gpointer data)
{
	g_return_if_fail(stack);
	
	stack->_stack = g_slist_prepend(stack->_stack, data);
}

gpointer
stack_pop(Stack *stack)
{
	GSList *element;
	gpointer data = NULL;
	
	if (G_LIKELY(stack && stack->_stack)) {
		element = stack->_stack;
		stack->_stack = element->next;

		data = element->data;
		g_slist_free_1(element);
	}
	
	return data;
}

gpointer
stack_peek(Stack *stack)
{
	return (G_LIKELY(stack && stack->_stack)) ? stack->_stack->data : NULL;
}
