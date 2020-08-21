#ifndef __PCI_H__
#define __PCI_H__

typedef struct _PCIDevice	PCIDevice;

struct _PCIDevice {
	gchar *name;
	gchar *category;
	
	gint  irq;
	guint io_addr;
	guint io_addr_end;
	
	gulong memory;
	
	gint bus;
	gint device;
	gint function;

	gint latency;
	gint freq;
	
	gboolean bus_master;
	
	PCIDevice *next;
};

void hi_show_pci_info(MainWindow *mainwindow, PCIDevice *device);
PCIDevice *hi_scan_pci(void);

#endif
