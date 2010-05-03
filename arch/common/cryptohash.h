/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 Leandro A. F. Pereira <leandro@hardinfo.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
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

#include <md5.h>
#include <sha1.h>

static void inline md5_step(char *data, glong srclen)
{
    struct MD5Context ctx;
    guchar checksum[16];
    
    MD5Init(&ctx);
    MD5Update(&ctx, (guchar *)data, srclen);
    MD5Final(checksum, &ctx);
}

static void inline sha1_step(char *data, glong srclen)
{
    SHA1_CTX ctx;
    guchar checksum[20];
    
    SHA1Init(&ctx);
    SHA1Update(&ctx, (guchar*)data, srclen);
    SHA1Final(checksum, &ctx);
}

static gpointer cryptohash_for(unsigned int start, unsigned int end, void *data, gint thread_number)
{
    unsigned int i;
    
    for (i = start; i <= end; i++) { 
        if (i & 1) {
            md5_step(data, 65536);
        } else {
            sha1_step(data, 65536);
        }
    }
    
    return NULL;
}

static void
benchmark_cryptohash(void)
{
    gdouble elapsed = 0;
    gchar *tmpsrc, *bdata_path;
    
    bdata_path = g_build_filename(params.path_data, "benchmark.data", NULL);
    if (!g_file_get_contents(bdata_path, &tmpsrc, NULL, NULL)) {
        g_free(bdata_path);
        return;
    }     
    
    shell_view_set_enabled(FALSE);
    shell_status_update("Running CryptoHash benchmark...");
        
    elapsed = benchmark_parallel_for(0, 5000, cryptohash_for, tmpsrc);
    
    g_free(bdata_path);
    g_free(tmpsrc);
    
    bench_results[BENCHMARK_CRYPTOHASH] = 312.0 / elapsed;
}
