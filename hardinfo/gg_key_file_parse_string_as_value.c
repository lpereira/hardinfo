/* From: gkeyfile.c - key file parser
 *
 *  Copyright 2004  Red Hat, Inc.
 *  Copyright 2009-2010  Collabora Ltd.
 *  Copyright 2009  Nokia Corporation
 *
 * Written by Ray Strode <rstrode@redhat.com>
 *            Matthias Clasen <mclasen@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>

gchar *
gg_key_file_parse_string_as_value (const gchar *string, const gchar list_separator)
{
  gchar *value, *p, *q;
  gsize length;
  gboolean parsing_leading_space;

  length = strlen (string) + 1;

  /* Worst case would be that every character needs to be escaped.
   * In other words every character turns to two characters. */
  value = g_new (gchar, 2 * length);

  p = (gchar *) string;
  q = value;
  parsing_leading_space = TRUE;
  while (p < (string + length - 1))
    {
      gchar escaped_character[3] = { '\\', 0, 0 };

      switch (*p)
        {
        case ' ':
          if (parsing_leading_space)
            {
              escaped_character[1] = 's';
              strcpy (q, escaped_character);
              q += 2;
            }
          else
            {
	      *q = *p;
	      q++;
            }
          break;
        case '\t':
          if (parsing_leading_space)
            {
              escaped_character[1] = 't';
              strcpy (q, escaped_character);
              q += 2;
            }
          else
            {
	      *q = *p;
	      q++;
            }
          break;
        case '\n':
          escaped_character[1] = 'n';
          strcpy (q, escaped_character);
          q += 2;
          break;
        case '\r':
          escaped_character[1] = 'r';
          strcpy (q, escaped_character);
          q += 2;
          break;
        case '\\':
          escaped_character[1] = '\\';
          strcpy (q, escaped_character);
          q += 2;
          parsing_leading_space = FALSE;
          break;
        default:
	  if (list_separator && *p == list_separator)
	    {
	      escaped_character[1] = list_separator;
	      strcpy (q, escaped_character);
	      q += 2;
              parsing_leading_space = TRUE;
	    }
	  else
	    {
	      *q = *p;
	      q++;
              parsing_leading_space = FALSE;
	    }
          break;
        }
      p++;
    }
  *q = '\0';

  return value;
}
