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
        
        _resources = h_strdup_cprintf("%s=%s\n", _resources, temp[0], temp[1]);
        
        g_strfreev(temp);
      }

      fclose(io);
    }
    
    if ((io = fopen("/proc/iomem", "r"))) {
      _resources = h_strconcat(_resources, "[Memory]\n", NULL);

      while (fgets(buffer, 256, io)) {
        gchar **temp = g_strsplit(buffer, ":", 2);
        
        _resources = h_strdup_cprintf("%s=%s\n", _resources, temp[0], temp[1]);
        
        g_strfreev(temp);
      }

      fclose(io);
    }
    
    if ((io = fopen("/proc/dma", "r"))) {
      _resources = h_strconcat(_resources, "[DMA]\n", NULL);

      while (fgets(buffer, 256, io)) {
        gchar **temp = g_strsplit(buffer, ":", 2);
        
        _resources = h_strdup_cprintf("%s=%s\n", _resources, temp[0], temp[1]);
        
        g_strfreev(temp);
      }

      fclose(io);
    }    
    
    SCAN_END();
}

gchar *callback_device_resources(void)
{
    return _resources;
}

