
#ifndef __GETTEXT_H__
#define __GETTEXT_H__

#include <string.h>
#include <libintl.h>
#include <locale.h>

#define _(STRING) gettext(STRING)
#define N_(STRING) (STRING)

#define C_(CTX, STRING) dgettext(CTX, STRING)
#define NC_(CTX, STRING) (STRING)

#endif
