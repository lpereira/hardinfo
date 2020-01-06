/* Base on: GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib.h>

guchar excmap_def[256] = {1,0};
static void make_excmap_def() {
  int i;
  for(i=0; i<256; i++){
    switch ((guchar)i)
      {
      case '\b':
      case '\f':
      case '\n':
      case '\r':
      case '\t':
      case '\\':
      case '"':
        excmap_def[i] = 0;
        break;
      default:
        if ((i < ' ') || (i >= 0177))
          excmap_def[i] = 0;
        else
          excmap_def[i] = 1;
        break;
      }
  }
}

gchar *
gg_strescape (const gchar *source,
             const gchar *exceptions,
             const gchar *extra)
{
  const guchar *p;
  gchar *dest;
  gchar *q;
  guchar excmap[256];

  g_return_val_if_fail (source != NULL, NULL);

  if (excmap_def[0]) /* [0] should be 0 or it isn't initialized */
    make_excmap_def();

  memcpy(excmap, excmap_def, 256);

  p = (guchar *) source;
  /* Each source byte needs maximally four destination chars (\777) */
  q = dest = g_malloc (strlen (source) * 4 + 1);

  if (exceptions)
    {
      guchar *e = (guchar *) exceptions;

      while (*e)
        {
          excmap[*e] = 1;
          e++;
        }
    }

  if (extra)
    {
      guchar *e = (guchar *) extra;

      while (*e)
        {
          excmap[*e] = 0;
          e++;
        }
    }

  while (*p)
    {
      if (excmap[*p])
        *q++ = *p;
      else
        {
          switch (*p)
            {
            case '\b':
              *q++ = '\\';
              *q++ = 'b';
              break;
            case '\f':
              *q++ = '\\';
              *q++ = 'f';
              break;
            case '\n':
              *q++ = '\\';
              *q++ = 'n';
              break;
            case '\r':
              *q++ = '\\';
              *q++ = 'r';
              break;
            case '\t':
              *q++ = '\\';
              *q++ = 't';
              break;
            case '\\':
              *q++ = '\\';
              *q++ = '\\';
              break;
            case '"':
              *q++ = '\\';
              *q++ = '"';
              break;
            default:
              *q++ = '\\';
              *q++ = '0' + (((*p) >> 6) & 07);
              *q++ = '0' + (((*p) >> 3) & 07);
              *q++ = '0' + ((*p) & 07);
              break;
            }
        }
      p++;
    }
  *q = 0;
  return dest;
}
