/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@linuxmag.com.br>
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
    gfloat bogomips;
    gchar *strmodel;
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

	    get_str("cpu model", processor->model_name);
	    get_float("BogoMIPS", processor->bogomips);
	    get_str("platform string", processor->strmodel);

	}
	g_strfreev(tmp);
    }
    
    gchar *tmp = g_strconcat("Alpha ", processor->model_name, NULL);
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
                               "Model=%s\n"
	                       "Platform String=%s\n"
	                       "BogoMIPS=%.2f"
	                       "Byte Order=%s\n",
			       processor->model_name,
			       processor->strmodel,
			       processor->bogomips,
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                               "Little Endian"
#else
                               "Big Endian"
#endif
                              );
}
