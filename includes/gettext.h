
#ifndef __GETTEXT_H__
#define __GETTEXT_H__

#include <string.h>
#include <libintl.h>
#include <locale.h>



#define _(STRING) gettext(STRING)
#define N_(STRING) (STRING)

#endif
