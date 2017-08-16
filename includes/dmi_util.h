#ifndef __DMI_UTIL_H__
#define __DMI_UTIL_H__

const char *dmi_sysfs_root(void);
char *dmi_get_str(const char *id_str);
char *dmi_chassis_type_str(int with_val);

#endif
