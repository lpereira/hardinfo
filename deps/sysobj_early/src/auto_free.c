/*
 * sysobj - https://github.com/bp0/verbose-spork
 * Copyright (C) 2018  Burt P. <pburt0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "auto_free.h"
#if (AF_USE_SYSOBJ)
#include "sysobj.h"
#else
#include <stdio.h>
#define sysobj_elapsed() af_elapsed()
#define sysobj_stats af_stats
static struct {
    double auto_free_next;
    unsigned long long
        auto_freed,
        auto_free_len;
} af_stats;
#endif

static GMutex free_lock;
static GSList *free_list = NULL;
static gboolean free_final = FALSE;
static GTimer *auto_free_timer = NULL;
static guint free_event_source = 0;
#define af_elapsed() (auto_free_timer ? g_timer_elapsed(auto_free_timer, NULL) : 0)

#define auto_free_msg(msg, ...)  fprintf (stderr, "[%s] " msg "\n", __FUNCTION__, ##__VA_ARGS__) /**/

typedef struct {
    gpointer ptr;
    GThread *thread;
    GDestroyNotify f_free;
    double stamp;

    const char *file;
    int  line;
    const char *func;
} auto_free_item;

gboolean free_auto_free_sf(gpointer trash) {
    (void)trash;
    if (free_final) {
        free_event_source = 0;
        return G_SOURCE_REMOVE;
    }
    free_auto_free();
    sysobj_stats.auto_free_next = sysobj_elapsed() + AF_SECONDS;
    return G_SOURCE_CONTINUE;
}

gpointer auto_free_ex_(gpointer p, GDestroyNotify f, const char *file, int line, const char *func) {
    if (!p) return p;

    /* an auto_free() after free_auto_free_final()?
     * Changed mind, I guess, just go with it. */
    if (free_final)
        free_final = FALSE;

    if (!auto_free_timer) {
        auto_free_timer = g_timer_new();
        g_timer_start(auto_free_timer);
    }

    if (!free_event_source) {
        /* if there is a main loop, then this will call
         * free_auto_free() in idle time every AF_SECONDS seconds.
         * If there is no main loop, then free_auto_free()
         * will be called at sysobj_cleanup() and when exiting
         * threads, as in sysobj_foreach(). */
        free_event_source = g_timeout_add_seconds(AF_SECONDS, (GSourceFunc)free_auto_free_sf, NULL);
        sysobj_stats.auto_free_next = sysobj_elapsed() + AF_SECONDS;
    }

    auto_free_item *z = g_new0(auto_free_item, 1);
    z->ptr = p;
    z->f_free = f;
    z->thread = g_thread_self();
    z->file = file;
    z->line = line;
    z->func = func;
    z->stamp = af_elapsed();
    g_mutex_lock(&free_lock);
    free_list = g_slist_prepend(free_list, z);
    sysobj_stats.auto_free_len++;
    g_mutex_unlock(&free_lock);
    return p;
}

gpointer auto_free_on_exit_ex_(gpointer p, GDestroyNotify f, const char *file, int line, const char *func) {
    if (!p) return p;

    auto_free_item *z = g_new0(auto_free_item, 1);
    z->ptr = p;
    z->f_free = f;
    z->thread = g_thread_self();
    z->file = file;
    z->line = line;
    z->func = func;
    z->stamp = -1.0;
    g_mutex_lock(&free_lock);
    free_list = g_slist_prepend(free_list, z);
    sysobj_stats.auto_free_len++;
    g_mutex_unlock(&free_lock);
    return p;
}

static struct { GDestroyNotify fptr; char *name; }
    free_function_tab[] = {
    { (GDestroyNotify) g_free,             "g_free" },
#if (AF_USE_SYSOBJ)
    { (GDestroyNotify) sysobj_free,        "sysobj_free" },
    { (GDestroyNotify) class_free,         "class_free" },
    { (GDestroyNotify) sysobj_filter_free, "sysobj_filter_free" },
    { (GDestroyNotify) sysobj_virt_free,   "sysobj_virt_free" },
#endif
    { NULL, "(null)" },
};

static void free_auto_free_ex(gboolean thread_final) {
    GThread *this_thread = g_thread_self();
    GSList *l = NULL, *n = NULL;
    long long unsigned fc = 0;
    double now = af_elapsed();

    if (!free_list) return;

    g_mutex_lock(&free_lock);
    if (DEBUG_AUTO_FREE)
        auto_free_msg("%llu total items in queue, but will free from thread %p only... ", sysobj_stats.auto_free_len, this_thread);
    for(l = free_list; l; l = n) {
        auto_free_item *z = (auto_free_item*)l->data;
        n = l->next;
        if (!free_final && z->stamp < 0) continue;
        double age = now - z->stamp;
        if (free_final || (z->thread == this_thread && (thread_final || age > AF_DELAY_SECONDS) ) ) {
            if (DEBUG_AUTO_FREE == 2) {
                char fptr[128] = "", *fname;
                for(int i = 0; i < (int)G_N_ELEMENTS(free_function_tab); i++)
                    if (z->f_free == free_function_tab[i].fptr)
                        fname = free_function_tab[i].name;
                if (!fname) {
                    snprintf(fname, 127, "%p", z->f_free);
                    fname = fptr;
                }
                if (z->file || z->func)
                    auto_free_msg("free: %s(%p) age:%lfs from %s:%d %s()", fname, z->ptr, age, z->file, z->line, z->func);
                else
                    auto_free_msg("free: %s(%p) age:%lfs", fname, z->ptr, age);
            }

            z->f_free(z->ptr);
            g_free(z);
            free_list = g_slist_delete_link(free_list, l);
            fc++;
        }
    }
    if (DEBUG_AUTO_FREE)
        auto_free_msg("... freed %llu (from thread %p)", fc, this_thread);
    sysobj_stats.auto_freed += fc;
    sysobj_stats.auto_free_len -= fc;
    g_mutex_unlock(&free_lock);
}

void free_auto_free_thread_final() {
    free_auto_free_ex(TRUE);
}

void free_auto_free_final() {
    free_final = TRUE;
    free_auto_free_ex(TRUE);
    g_timer_destroy(auto_free_timer);
    auto_free_timer = NULL;
}

void free_auto_free() {
    free_auto_free_ex(FALSE);
}
