#ifndef __ISAPnP_H__
#define __ISAPnP_H__

#include "hardinfo.h"

typedef struct _ISADevice	ISADevice;

struct _ISADevice {
	gchar *card;
	gint card_id;
	gfloat pnpversion, prodversion;

	ISADevice *next;
};

ISADevice *hi_scan_isapnp(void);
void hi_show_isa_info(MainWindow *mainwindow, ISADevice *device);


#endif
