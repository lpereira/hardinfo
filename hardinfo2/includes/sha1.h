/*
 * SHA-1 in C
 * By Steve Reid <steve@edmweb.com>
 * 100% Public Domain
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
    guint32 state[5];
    guint32 count[2];
    guchar buffer[64];
} SHA1_CTX;

void SHA1Transform(guint32 state[5], guchar buffer[64]);
void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, guchar* data, unsigned int len);
void SHA1Final(guchar digest[20], SHA1_CTX* context);

#endif	/* __SHA1_H__ */
