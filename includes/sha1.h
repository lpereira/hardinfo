/*
 * SHA-1 in C
 * By Steve Reid <steve@edmweb.com>
 * Copyright by: hardinfo2 project
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License v2.0 or later.
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

#ifndef __SHA1_H__
#define __SHA1_H__

#include <glib.h>

#ifndef LITTLE_ENDIAN
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define LITTLE_ENDIAN		/* This should be #define'd if true. */
#endif /* G_BYTE_ORDER */
#endif /* LITTLE_ENDIAN */


typedef struct {
    guint32 state[20];
    guint32 count[2];
    guchar buffer[64];
} SHA1_CTX;

void SHA1Transform(guint32 state[5], guchar buffer[64]);
void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, guchar* data, unsigned int len);
void SHA1Final(guchar digest[20], SHA1_CTX* context);

#endif	/* __SHA1_H__ */
