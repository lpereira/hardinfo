/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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
#ifndef __EXPR_H__
#define __EXPR_H__

typedef struct _MathToken MathToken;

typedef enum {
    TOKEN_OPERATOR,
    TOKEN_VARIABLE,
    TOKEN_VALUE
} MathTokenType;

struct _MathToken {
    union {
	gfloat	value;
	gchar	op;
    } val;
    MathTokenType type;
};

#define math_postfix_free math_infix_free

GSList	*math_infix_to_postfix(GSList *infix);
void	 math_infix_free(GSList *infix, gboolean free_tokens);

GSList	*math_string_to_infix(gchar *string);
GSList	*math_string_to_postfix(gchar *string);

gfloat	 math_postfix_eval(GSList *postfix, gfloat at_value);
gfloat	 math_string_eval(gchar *string, gfloat at_value);

#endif	/* __EXPR_H__ */
