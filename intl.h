#ifndef __INTL_H__
#define __INTL_H__

#include "config.h"

void intl_init(void);
const gchar *intl_translate(const gchar *string, const gchar *source) __THROW;

#define _(x) (intl_translate(x, __FILE__))

#endif				/* __INTL_H__ */
