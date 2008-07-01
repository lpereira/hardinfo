/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2008 Leandro A. F. Pereira <leandro@hardinfo.org>
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

gchar *_resources = NULL;

#if GLIB_CHECK_VERSION(2,14,0)
static GRegex *_regex_pci = NULL,
              *_regex_module = NULL;

static gchar *_resource_obtain_name(gchar *name)
{
    gchar *temp;

    if (!_regex_pci && !_regex_module) {
      _regex_pci = g_regex_new("^[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:"
                               "[0-9a-fA-F]{2}\\.[0-9a-fA-F]{1}$",
                               0, 0, NULL);
      _regex_module = g_regex_new("^[0-9a-zA-Z\\_\\-]+$", 0, 0, NULL);
    }
    
    name = g_strstrip(name);
    
    if (g_regex_match(_regex_pci, name, 0, NULL)) {
      temp = module_call_method_param("devices::getPCIDeviceDescription", name);
      if (temp) {
          return temp;
      }
    } else if (g_regex_match(_regex_module, name, 0, NULL)) {
      temp = module_call_method_param("computer::getKernelModuleDescription", name);
      if (temp) {
          return temp;
      }
    }
    
    return g_strdup(name);
}
#else
static gchar *_resource_obtain_name(gchar *name)
{
    return name;
}
#endif

void scan_device_resources(gboolean reload)
{
    SCAN_START();
    FILE *io;
    gchar buffer[256];
    
    g_free(_resources);
    _resources = g_strdup("");
    
    if ((io = fopen("/proc/ioports", "r"))) {
      _resources = h_strconcat(_resources, "[I/O Ports]\n", NULL);

      while (fgets(buffer, 256, io)) {
        gchar **temp = g_strsplit(buffer, ":", 2);
        gchar *name = _resource_obtain_name(temp[1]);
        
        _resources = h_strdup_cprintf("%s=%s\n", _resources,
                                      temp[0], name);
        
        g_strfreev(temp);
        g_free(name);
      }

      fclose(io);
    }
    
    if ((io = fopen("/proc/iomem", "r"))) {
      _resources = h_strconcat(_resources, "[Memory]\n", NULL);

      while (fgets(buffer, 256, io)) {
        gchar **temp = g_strsplit(buffer, ":", 2);
        gchar *name = _resource_obtain_name(temp[1]);
        
        _resources = h_strdup_cprintf("%s=%s\n", _resources,
                                      temp[0], name);
        
        g_strfreev(temp);
        g_free(name);
      }

      fclose(io);
    }
    
    if ((io = fopen("/proc/dma", "r"))) {
      _resources = h_strconcat(_resources, "[DMA]\n", NULL);

      while (fgets(buffer, 256, io)) {
        gchar **temp = g_strsplit(buffer, ":", 2);
        gchar *name = _resource_obtain_name(temp[1]);
        
        _resources = h_strdup_cprintf("%s=%s\n", _resources,
                                      temp[0], name);
        
        g_strfreev(temp);
        g_free(name);
      }

      fclose(io);
    }    
    
    SCAN_END();
}

gchar *callback_device_resources(void)
{
    return _resources;
}