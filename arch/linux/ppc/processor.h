/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@hardinfo.org>
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

struct _Processor {
    gchar *model_name;
    gchar *vendor_id;
    gint cache_size;
    gfloat bogomips, cpu_mhz;
};

static GSList *
__scan_processors(void)
{
    Processor *processor;
    FILE *cpuinfo;
    gchar buffer[128];

    cpuinfo = fopen("/proc/cpuinfo", "r");
    if (!cpuinfo)
	return NULL;

    processor = g_new0(Processor, 1);
    while (fgets(buffer, 128, cpuinfo)) {
	gchar **tmp = g_strsplit(buffer, ":", 2);

	if (tmp[0] && tmp[1]) {
	    tmp[0] = g_strstrip(tmp[0]);
	    tmp[1] = g_strstrip(tmp[1]);

	    get_str("cpu", processor->model_name);
	    get_str("machine", processor->vendor_id);
	    get_int("L2 cache", processor->cache_size);
	    get_float("clock", processor->cpu_mhz);
	    get_float("bogomips", processor->bogomips);

	}
	g_strfreev(tmp);
    }
    
    gchar *tmp = g_strdup_printf("PowerPC %s (%.2fMHz)",
                                 processor->model_name,
                                 processor->cpu_mhz);
    g_free(processor->model_name);
    processor->model_name = tmp;

    fclose(cpuinfo);

    return g_slist_append(NULL, processor);
}

static gchar *
processor_get_info(GSList *processors)
{
        Processor *processor = (Processor *)processors->data;
        
	return g_strdup_printf("[Processor]\n"
	                       "Machine=%s\n"
	                       "CPU=%s\n"
	                       "L2 Cache=%dkB\n"
	                       "Frequency=%.2fMHz\n"
	                       "BogoMips=%.2f\n"
	                       "Byte Order=%s\n",
			       processor->vendor_id,
			       processor->model_name,
			       processor->cache_size,
			       processor->cpu_mhz,
			       processor->bogomips,
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                               "Little Endian"
#else
                               "Big Endian"
#endif
                              );
}
