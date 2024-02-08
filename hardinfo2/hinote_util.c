
#include "hardinfo.h"

/* requires COMPILE_FLAGS "-std=c99" */

static const char bullet_yes[] = "<big><b>\u2713</b></big>";
static const char bullet_no[] = "<big><b>\u2022<tt> </tt></b></big>";
static const char bullet_yes_text[] = "[X]";
static const char bullet_no_text[] = "[ ]";

gboolean note_cond_bullet(gboolean cond, gchar *note_buff, const gchar *desc_str) {
    int l = strlen(note_buff);
    if (params.markup_ok)
        snprintf(note_buff + l, note_max_len - l - 1, "%s %s\n",
            cond ? bullet_yes : bullet_no, desc_str);
    else
        snprintf(note_buff + l, note_max_len - l - 1, "%s %s\n",
            cond ? bullet_yes_text : bullet_no_text, desc_str);
    return cond;
}

gboolean note_require_tool(const gchar *tool, gchar *note_buff, const gchar *desc_str) {
    gchar *tl = find_program((gchar*)tool);
    gboolean found = note_cond_bullet(!!tl, note_buff, desc_str);
    g_free(tl);
    return found;
}
