
#ifndef _DT_UTIL_H_
#define _DT_UTIL_H_

#include <stdint.h>

/* some not-quite-complete stuff that can be disabled */
#define DTEX_PHREFS 1
#define DTEX_MTUP 0

#ifndef DTR_ROOT
#define DTR_ROOT dtr_find_device_tree_root()
#endif

enum {
    DT_TYPE_ERR,

    DT_NODE,
    DTP_UNK,     /* arbitrary-length byte sequence */
    DTP_EMPTY,   /* empty property */
    DTP_STR,     /* null-delimited list of strings */
    DTP_HEX,     /* list of 32-bit values displayed in hex */
    DTP_UINT,
 /* DTP_INT, */
    DTP_OVR,     /* all in /__overrides__ */
    DTP_PH,      /* phandle */
    DTP_PH_REF,  /* reference to phandle */
    DTP_REG,     /* <#address-cells, #size-cells> */
    DTP_CLOCKS,  /* <phref, #clock-cells> */
    DTP_GPIOS,   /* <phref, #gpio-cells> */
};

/* simplest, no aliases.
 * use dtr_get_prop_str() for complete. */
char* dtr_get_string(const char *p);

typedef uint32_t dt_uint; /* big-endian */

typedef struct _dtr dtr;
typedef struct _dtr_obj dtr_obj;

dtr *dtr_new(char *base_path); /* NULL for DTR_ROOT */
void dtr_free(dtr *);
const char *dtr_base_path(dtr *);

dtr_obj *dtr_obj_read(dtr *, const char *dtp);
void dtr_obj_free(dtr_obj *);
int dtr_obj_type(dtr_obj *);
char *dtr_obj_alias(dtr_obj *);
char *dtr_obj_symbol(dtr_obj *);
char *dtr_obj_path(dtr_obj *);        /* device tree path */
char *dtr_obj_full_path(dtr_obj *);   /* system path */

/* find property/node 'name' relative to node
 * if node is NULL, then from root */
dtr_obj *dtr_get_prop_obj(dtr *, dtr_obj *node, const char *name);
char *dtr_get_prop_str(dtr *, dtr_obj *node, const char *name);
uint32_t dtr_get_prop_u32(dtr *, dtr_obj *node, const char *name);

/* attempts to render the object as a string */
char* dtr_str(dtr_obj *obj);

int dtr_guess_type(dtr_obj *obj);
char *dtr_elem_phref(dtr *, dt_uint e, int show_path);
char *dtr_elem_hex(dt_uint e);
char *dtr_elem_byte(uint8_t e);
char *dtr_elem_uint(dt_uint e);
char *dtr_list_byte(uint8_t *bytes, unsigned long count);
char *dtr_list_hex(dt_uint *list, unsigned long count);

int dtr_cellv_find(dtr_obj *obj, char *qprop, int limit);

char *dtr_maps_info(dtr *); /* returns hardinfo shell section */

const char *dtr_find_device_tree_root(void);

#define sp_sep(STR) (strlen(STR) ? " " : "")
/* appends an element to a string, adding a space if
 * the string is not empty.
 * ex: ret = appf(ret, "%s=%s\n", name, value); */
char *appf(char *src, char *fmt, ...);

#endif
