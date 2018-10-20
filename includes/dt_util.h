
#ifndef _DT_UTIL_H_
#define _DT_UTIL_H_

#include <stdint.h>

/* some not-quite-complete stuff that can be disabled */
#define DTEX_PHREFS 1

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
    DTP_UINT,    /* unsigned int list */
    DTP_UINT64,  /* unsigned int64 list */
    DTP_INTRUPT, /* interrupt-specifier list */
    DTP_INTRUPT_EX, /* extended interrupt-specifier list */
    DTP_OVR,     /* all in /__overrides__ */
    DTP_PH,      /* phandle */
    DTP_PH_REF,  /* reference to phandle */
    DTP_REG,     /* <#address-cells, #size-cells> */
    DTP_CLOCKS,  /* <phref, #clock-cells> */
    DTP_GPIOS,   /* <phref, #gpio-cells> */
    DTP_DMAS,    /* dma-specifier list */
    DTP_OPP1,    /* list of operating points, pairs of (kHz,uV) */
    DTP_PH_REF_OPP2,  /* phandle reference to opp-v2 table */
};

/* simplest, no aliases, doesn't require an existing dt.
 * use dtr_get_prop_str() for complete. */
char* dtr_get_string(const char *p, int decode);

typedef uint32_t dt_uint; /* big-endian */
typedef uint64_t dt_uint64; /* big-endian */

typedef struct _dtr dtr;
typedef struct _dtr_obj dtr_obj;

dtr *dtr_new(const char *base_path); /* NULL for DTR_ROOT */
void dtr_free(dtr *);
int dtr_was_found(dtr *);
const char *dtr_base_path(dtr *);
char *dtr_messages(dtr *); /* returns a message log */

dtr_obj *dtr_obj_read(dtr *, const char *dtp);
void dtr_obj_free(dtr_obj *);
int dtr_obj_type(dtr_obj *);
char *dtr_obj_alias(dtr_obj *);
char *dtr_obj_symbol(dtr_obj *);
char *dtr_obj_path(dtr_obj *);        /* device tree path */
char *dtr_obj_full_path(dtr_obj *);   /* system path */
dtr_obj *dtr_get_parent_obj(dtr_obj *);

/* find property/node 'name' relative to node
 * if node is NULL, then from root */
dtr_obj *dtr_get_prop_obj(dtr *, dtr_obj *node, const char *name);
char *dtr_get_prop_str(dtr *, dtr_obj *node, const char *name);
uint32_t dtr_get_prop_u32(dtr *, dtr_obj *node, const char *name);
uint64_t dtr_get_prop_u64(dtr *, dtr_obj *node, const char *name);

/* attempts to render the object as a string */
char* dtr_str(dtr_obj *obj);

int dtr_guess_type(dtr_obj *obj);
char *dtr_elem_phref(dtr *, dt_uint e, int show_path, const char *extra);
char *dtr_elem_hex(dt_uint e);
char *dtr_elem_byte(uint8_t e);
char *dtr_elem_uint(dt_uint e);
char *dtr_list_byte(uint8_t *bytes, unsigned long count);
char *dtr_list_hex(dt_uint *list, unsigned long count);

int dtr_cellv_find(dtr_obj *obj, char *qprop, int limit);

char *dtr_maps_info(dtr *); /* returns hardinfo shell section */

const char *dtr_find_device_tree_root(void);

/* write to the message log */
void dtr_msg(dtr *s, char *fmt, ...);

#define sp_sep(STR) (strlen(STR) ? " " : "")
/* appends an element to a string, adding a space if
 * the string is not empty.
 * ex: ret = appf(ret, "%s=%s\n", name, value); */
char *appf(char *src, char *fmt, ...);

/* operating-points v0,v1,v2 */
typedef struct {
    uint32_t version; /* opp version, 0 = clock-frequency only */
    uint32_t phandle; /* v2 only */
    uint32_t khz_min;
    uint32_t khz_max;
    uint32_t clock_latency_ns;
} dt_opp_range;

/* free result with g_free() */
dt_opp_range *dtr_get_opp_range(dtr *, const char *name);

#endif
