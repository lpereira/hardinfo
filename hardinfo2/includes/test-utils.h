#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libsoup/soup-types.h"

void test_init    (int argc, char **argv, GOptionEntry *entries);
void test_cleanup (void);

extern int debug_level, errors;
extern gboolean expect_warning;
void debug_printf (int level, const char *format, ...) G_GNUC_PRINTF (2, 3);

#ifdef HAVE_APACHE
void apache_init    (void);
void apache_cleanup (void);
#endif

SoupSession *soup_test_session_new         (GType type, ...);
void         soup_test_session_abort_unref (SoupSession *session);

SoupServer  *soup_test_server_new     (gboolean in_own_thread);
SoupServer  *soup_test_server_new_ssl (gboolean in_own_thread);

