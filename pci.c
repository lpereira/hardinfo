/*
 * Hardware Information, version 0.3
 * Copyright (C) 2003 Leandro Pereira <leandro@linuxmag.com.br>
 *
 * May be modified and/or distributed under the terms of GNU GPL version 2.
 *
 */

#include "hardinfo.h"
#include "pci.h"

void hi_show_pci_info(MainWindow *mainwindow, PCIDevice *device)
{
	GtkWidget *hbox, *vbox, *label;
	gchar *buf;
#ifdef GTK2
	GtkWidget *pixmap;
	
	pixmap = gtk_image_new_from_file(IMG_PREFIX "pci.png");
	gtk_widget_show(pixmap);
#endif

	if(!device) return;

	hbox = gtk_hbox_new(FALSE, 2);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
	gtk_widget_show(hbox);
	
	if(mainwindow->framec)
		gtk_widget_destroy(mainwindow->framec);

	gtk_container_add(GTK_CONTAINER(mainwindow->frame), hbox);
	mainwindow->framec = hbox;

	gtk_frame_set_label(GTK_FRAME(mainwindow->frame), device->category);
	
#ifdef GTK2
	gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 0);
#endif

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

#ifdef GTK2
	buf = g_strdup_printf("<b>%s</b>", device->name);
	label = gtk_label_new(buf);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	
	g_free(buf);
#else
	label = gtk_label_new(device->name);
#endif
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	if(device->irq) {
		buf = g_strdup_printf("IRQ: %d", device->irq);
	
		label = gtk_label_new(buf);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
		g_free(buf);				
	}

	if(device->io_addr) {
		buf = g_strdup_printf(_("I/O address: 0x%x to 0x%x"), device->io_addr,
			device->io_addr_end);
	
		label = gtk_label_new(buf);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
		g_free(buf);				
	}

	if(device->memory) {
		buf = g_strdup_printf(_("Memory: %ld %s"),
			(device->memory <= 1024) ? device->memory :
				device->memory / 1000,
			(device->memory <= 1024) ? "bytes" : "KB");
	
		label = gtk_label_new(buf);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
		g_free(buf);				
	}

	if(device->freq) {
		buf = g_strdup_printf(_("Frequency: %dMHz"), device->freq);
	
		label = gtk_label_new(buf);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
		g_free(buf);				
	}

	if(device->latency) {
		buf = g_strdup_printf(_("Latency: %d"), device->latency);
	
		label = gtk_label_new(buf);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	
		g_free(buf);				
	}

	if(device->bus_master) {
		label = gtk_label_new(_("Bus master"));
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	}

	buf = g_strdup_printf(_("Bus: %d, Device: %d, Function: %d"),
		device->bus, device->device, device->function);
	
	label = gtk_label_new(buf);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	g_free(buf);				
}

#ifdef USE_LSPCI
PCIDevice *hi_scan_pci(void)
{
	FILE *lspci;
	gchar buffer[256], *buf;
	gint n=0;
	PCIDevice *pci_dev, *pci;
	
	pci = NULL;
	
	lspci = popen(LSPCI, "r");
	if(!lspci) return NULL;
	
	while(fgets(buffer, 256, lspci)){
		buf = g_strstrip(buffer);
		if(!strncmp(buf, "Flags", 5)){
			gint irq=0, freq=0, latency=0, i;
			gchar **list;
		
			buf+=7;
						
			pci_dev->bus_master = FALSE;
			
			list = g_strsplit(buf, ", ", 10);
			for (i = 0; i <= 10; i++) {
				if(!list[i]) break;
			
				if(!strncmp(list[i], "IRQ", 3))
					sscanf(list[i], "IRQ %d", &irq);
				else if(strstr(list[i], "Mhz"))
					sscanf(list[i], "%dMhz", &freq);
				else
				if(!strncmp(list[i], "bus master", 10))
					pci_dev->bus_master = TRUE; 
				else if(!strncmp(list[i], "latency", 7))
					sscanf(list[i], "latency %d", &latency);
			}
			g_strfreev(list);
			
			if (irq)  pci_dev->irq = irq;
			if (freq) pci_dev->freq = freq;
			if (latency) pci_dev->latency = latency;
		} 
		
		else if(!strncmp(buf, "Memory at", 9) &&
			  strstr(buf, "[size=")) {
			gulong mem;
			gchar unit;
			
			walk_until('[');
			sscanf(buf, "[size=%ld%c", &mem, &unit);

			mem *= (unit == ']') ? 1 :
			       (unit == 'K') ? 1024 :
			       (unit == 'M') ? 1024 * 1000 :
			       (unit == 'G') ? 1024 * 1000 * 1000 : 1;

			pci_dev->memory += mem;
		}  else if(!strncmp(buf, "I/O", 3)){
			guint io_addr, io_size;
			
			sscanf(buf, "I/O ports at %x [size=%d]", &io_addr, &io_size);
			
			pci_dev->io_addr	= io_addr;			
			pci_dev->io_addr_end	= io_addr+io_size;
		} else if((buf[0] >= '0' && buf[0] <= '9') && buf[2] == ':'){
			gint bus, device, function;
			gpointer start, end;
			
			pci_dev = g_new0(PCIDevice, 1);

			pci_dev->next = pci;			
			pci = pci_dev;
			
			sscanf(buf, "%x:%x.%d",	&bus, &device, &function);
			
			pci_dev->bus		= bus;
			pci_dev->device		= device;
			pci_dev->function 	= function;

			walk_until(' ');

			start = buf;
			
			walk_until(':');
			end = buf+1;
			*buf=0;

			buf = start+1;			
			pci_dev->category = g_strdup(buf);
			
			buf = end;
			start = buf;
			walk_until('(');
			*buf = 0;
			buf = start+1;
				
			pci_dev->name = g_strdup(buf);

			n++;
		}
	}
	pclose(lspci);
	
	return pci;
}

#else

#warning Using /proc/pci is deprecated, please install pciutils!

PCIDevice *hi_scan_pci(void)
{
	FILE *proc_pci;
	gchar buffer[256], *buf;
	gboolean next_is_name = FALSE;
	gint n=0;
	PCIDevice *pci_dev, *pci;
	struct stat st;
	
	g_print("Scanning PCI devices... ");
	
	pci = NULL;
	
	if(stat("/proc/pci", &st)) return NULL;
	
	proc_pci = fopen("/proc/pci", "r");
	while(fgets(buffer, 256, proc_pci)){
		buf = g_strstrip(buffer);
		if(next_is_name == TRUE) {
			gpointer start, end;
			
			start = buf;
			
			walk_until(':');
			end = buf+1;
			*buf=0;

			buf = start;			
			pci_dev->category = g_strdup(buf);
			
			buf = end;			
			buf[strlen(buf)-1]=0;
			pci_dev->name = g_strdup(buf);
			
			next_is_name = FALSE;
		} else if(!strncmp(buf, "IRQ", 3)){
			gint irq;
			
			sscanf(buf, "IRQ %d", &irq);
			
			pci_dev->irq		= irq;
		} else if(!strncmp(buf, "Prefetchable 32 bit", 19) ||
				!strncmp(buf, "Non-prefetchable 32 bit", 23)){
			gulong mem_st, mem_end;
			
			if(*buf == 'N') buf+=4;
			buf++;
			
			sscanf(buf, "refetchable 32 bit memory at %lx [%lx]",
				&mem_st, &mem_end);
			
			pci_dev->memory+=(mem_end-mem_st)/1024;
		} else if(!strncmp(buf, "I/O", 3)){
			guint io_addr, io_addr_end;
			
			sscanf(buf, "I/O at %x [%x]", &io_addr, &io_addr_end);
			
			pci_dev->io_addr	= io_addr;			
			pci_dev->io_addr_end	= io_addr_end;
		} else if(!strncmp(buf, "Bus", 3)){
			gint bus, device, function;
			
			pci_dev = g_new0(PCIDevice, 1);

			pci_dev->next = pci;			
			pci = pci_dev;
			
			sscanf(buf, "Bus %d, device %d, function %d:",
					&bus, &device, &function);
			
			pci_dev->bus		= bus;
			pci_dev->device		= device;
			pci_dev->function 	= function;

			next_is_name = TRUE;
			n++;
		}
	}
	fclose(proc_pci);
	
	g_print ("%d devices found\n", n);	
	
	return pci;
}
#endif

