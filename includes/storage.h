#ifndef __STORAGE_H__
#define __STORAGE_H__

void __scan_ide_devices(void);
void __scan_scsi_devices(void);
gboolean __scan_udisks2_devices(void);

#endif /* __STORAGE_H__ */
