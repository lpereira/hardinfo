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
/*
 * This is only used to compute sensor values, hence the only variable supported is '@'.
 * The '`' operator (ln(x)) is not available, nor multi-line formulas.
 */

#include <glib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "expr.h"
#include "config.h"

static MathToken *new_operator(gchar op)
{
    MathToken *t = g_new0(MathToken, 1);

    t->val.op = op;
    t->type = TOKEN_OPERATOR;	/* operator */

    return t;
}

static MathToken *new_variable(gchar var)
{
    MathToken *t = g_new0(MathToken, 1);

    t->val.op = '@';
    t->type = TOKEN_VARIABLE;	/* variable */

    return t;
}

static MathToken *new_value(gfloat value)
{
    MathToken *t = g_new0(MathToken, 1);

    t->val.value = value;
    t->type = TOKEN_VALUE;	/* value */

    return t;
}

static inline gint priority(char operation)
{
    switch (operation) {
    case '^':
	return 3;
    case '*':
    case '/':
	return 2;
    case '+':
    case '-':
	return 1;
    case '(':
	return 0;
    }

    return 0;
}

GSList *math_infix_to_postfix(GSList * infix)
{
    MathToken *stack[500];
    gint t_sp = 0;

    GSList *postfix = NULL, *p;
    MathToken *top;

    for (p = infix; p; p = p->next) {
	MathToken *t = (MathToken *) p->data;

	if (t->type == TOKEN_OPERATOR && t->val.op == '(') {
	    stack[++t_sp] = t;
	} else if (t->type == TOKEN_OPERATOR && t->val.op == ')') {
	    for (top = stack[t_sp]; t_sp != 0 && top->val.op != '(';
		 top = stack[t_sp]) {
                postfix = g_slist_append(postfix, stack[t_sp--]);
            }
	    t_sp--;
	} else if (t->type != TOKEN_OPERATOR) {
	    postfix = g_slist_append(postfix, t);
	} else if (t_sp == 0) {
	    stack[++t_sp] = t;
	} else {
	    while (t_sp != 0
		   && priority(t->val.op) <= priority(stack[t_sp]->val.op))
		postfix = g_slist_append(postfix, stack[t_sp--]);
	    stack[++t_sp] = t;
	}
    }

    while (t_sp)
	postfix = g_slist_append(postfix, stack[t_sp--]);
	
    return postfix;
}

static inline gfloat __result(gfloat op1, gfloat op2, gchar operation)
{
    switch (operation) {
    case '^':
	return powf(op1, op2);
    case '+':
	return op1 + op2;
    case '-':
	return op1 - op2;
    case '/':
	return op1 / op2;
    case '*':
	return op1 * op2;
    }

    return 0;
}

gfloat math_postfix_eval(GSList * postfix, gfloat at_value)
{
    GSList *p;
    gfloat stack[500];
    gint sp = 0;

    memset(stack, 0, sizeof(gfloat) * 500);

    for (p = postfix; p; p = p->next) {
	MathToken *t = (MathToken *) p->data;

	if (t->type == TOKEN_VARIABLE) {
	    stack[++sp] = at_value;
	} else if (t->type == TOKEN_VALUE) {
	    stack[++sp] = t->val.value;
	} else {
	    gfloat op1, op2;

	    op2 = stack[sp--];
	    op1 = stack[sp];
	    
	    stack[sp] = __result(op1, op2, t->val.op);
	}
    }
    
    return stack[sp];
}

GSList *math_string_to_infix(gchar * string)
{
    GSList *infix = NULL;
    gchar *expr = string;
    
    for (; *expr; expr++) {
	if (strchr("+-/*^()", *expr)) {
	    infix = g_slist_append(infix, new_operator(*expr));
	} else if (strchr("@", *expr)) {
	    infix = g_slist_append(infix, new_variable(*expr));
	} else if (strchr("-.1234567890", *expr)) {
	    gchar value[32], *v = value;
	    gfloat floatval;
	    
	    do {
	      *v++ = *expr++;
	    } while (*expr && strchr("-.1234567890", *expr));
	    expr--;
	    *v = '\0';
	    
	    sscanf(value, "%f", &floatval);

	    infix = g_slist_append(infix, new_value(floatval));
	} else if (!isspace(*expr)) {
	    g_print("Invalid token: [%c][%d]\n", *expr, *expr);
	    math_infix_free(infix, TRUE);
	    return NULL;
	}
    }

    return infix;
}

void math_infix_free(GSList * infix, gboolean free_tokens)
{
    GSList *p;

    if (!free_tokens)
	for (p = infix; p; p = g_slist_delete_link(p, p));
    else
	for (p = infix; p; p = g_slist_delete_link(p, p)) {
	    MathToken *t = (MathToken *) p->data;
	    g_free(t);
	}
}

GSList *math_string_to_postfix(gchar * string)
{
    GSList *infix;
    GSList *postfix;

    infix = math_string_to_infix(string);
    if (!infix)
	return NULL;

    postfix = math_infix_to_postfix(infix);
    math_infix_free(infix, FALSE);

    return postfix;
}

gfloat math_string_eval(gchar * string, gfloat at_value)
{
    GSList *postfix;
    gfloat val;

    postfix = math_string_to_postfix(string);
    val = math_postfix_eval(postfix, at_value);
    math_postfix_free(postfix, TRUE);

    return val;
}

#ifdef MATH_TEST
int main(void)
{
    GSList *postfix;

    gchar *expr = "0.9*(@+(5.2*0.923+3*(2.0)))";

    postfix = math_string_to_postfix(expr);
    g_print("%s = %f (must be 18.71964)\n", expr,
	    math_postfix_eval(postfix, 10));
    math_postfix_free(postfix, TRUE);

    return 0;
}
#endif
