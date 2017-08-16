#ifndef __DMI_UTIL_H__
#define __DMI_UTIL_H__

const char *dmi_sysfs_root(void);
char *dmi_get_str(const char *id_str);

/* if chassis_type is <=0 it will be fetched from DMI.
 * with_val = true, will return a string like "[3] Desktop" instead of just
 * "Desktop". */
char *dmi_chassis_type_str(int chassis_type, int with_val);

#endif
