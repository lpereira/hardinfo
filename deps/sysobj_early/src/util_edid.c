/*
 * sysobj - https://github.com/bp0/verbose-spork
 * Copyright (C) 2018  Burt P. <pburt0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <endian.h>
#include <stdio.h>
#include "gettext.h"
#include "util_edid.h"
#include "util_sysobj.h"

#include "util_edid_svd_table.c"

#define NOMASK (~0U)
#define BFMASK(LSB, MASK) (MASK << LSB)

#define DPTR(ADDY) (uint8_t*)(&((ADDY).e->u8[(ADDY).offset]))
#define OFMT "@%03d" /* for addy.offset */

#define EDID_MSG_STDERR 0
#define edid_msg(e, msg, ...) {\
    if (EDID_MSG_STDERR) fprintf (stderr, ">[%s;L%d] " msg "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    g_string_append_printf(e->msg_log, "[%s;L%d] " msg "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); }

static inline
uint32_t bf_value(uint32_t value, uint32_t mask) {
    uint32_t result = value & mask;
    if (result)
        while(!(mask & 1)) {
            result >>= 1;
            mask >>= 1;
        }
    return result;
}

static inline
uint8_t bounds_check(edid *e, uint32_t offset) {
    if (!e) return 0;
    if (offset > e->len) return 0;
    return 1;
}

static inline
char *rstr(edid *e, uint32_t offset, uint32_t len) {
    if (!bounds_check(e, offset+len)) return NULL;
    char *raw = malloc(len+1), *ret = NULL;
    strncpy(raw, (char*)&e->u8[offset], len);
    raw[len] = 0;
    ret = g_utf8_make_valid(raw, len);
    g_free(raw);
    return ret;
}

static inline
char *rstr_strip(edid *e, uint32_t offset, uint32_t len) {
    if (!bounds_check(e, offset+len)) return NULL;
    char *raw = malloc(len+1), *ret = NULL;
    strncpy(raw, (char*)&e->u8[offset], len);
    raw[len] = 0;
    ret = g_strstrip(g_utf8_make_valid(raw, len));
    g_free(raw);
    return ret;
}

static inline
uint32_t r8(edid *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset)) return 0;
    return bf_value(e->u8[offset], mask);
}

static inline
uint32_t r16le(edid *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+1)) return 0;
    uint32_t v = (e->u8[offset+1] << 8) + e->u8[offset];
    return bf_value(v, mask);
}

static inline
uint32_t r16be(edid *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+1)) return 0;
    uint32_t v = (e->u8[offset] << 8) + e->u8[offset+1];
    return bf_value(v, mask);
}

static inline
uint32_t r24le(edid *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+2)) return 0;
    uint32_t v = (e->u8[offset+2] << 16) + (e->u8[offset+1] << 8) + e->u8[offset];
    return bf_value(v, mask);
}

static inline
uint32_t r24be(edid *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+2)) return 0;
    uint32_t v = (e->u8[offset] << 16) + (e->u8[offset+1] << 8) + e->u8[offset+2];
    return bf_value(v, mask);
}

static inline
uint32_t r32le(edid *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+3)) return 0;
    uint32_t v = (e->u8[offset+3] << 24) + (e->u8[offset+2] << 16)
        + (e->u8[offset+1] << 8) + e->u8[offset];
    return bf_value(v, mask);
}

static inline
uint32_t r32be(edid *e, uint32_t offset, uint32_t mask) {
    if (!bounds_check(e, offset+3)) return 0;
    uint32_t v = (e->u8[offset] << 24) + (e->u8[offset+1] << 16)
        + (e->u8[offset+2] << 8) + e->u8[offset+3];
    return bf_value(v, mask);
}

static inline
int rpnpcpy(edid_ven *dest, edid *e, uint32_t offset) {
    uint32_t pnp = r16be(e, offset, NOMASK);
    edid_ven ret = {.type = VEN_TYPE_INVALID};
    if (pnp) {
        ret.type = VEN_TYPE_PNP;
        ret.pnp[2] = 64 + (pnp & 0x1f);
        ret.pnp[1] = 64 + ((pnp >> 5) & 0x1f);
        ret.pnp[0] = 64 + ((pnp >> 10) & 0x1f);
        *dest = ret;
        return 1;
    }
    return 0;
}

static inline
int rouicpy(edid_ven *dest, edid *e, uint32_t offset) {
    edid_ven ret = {.type = VEN_TYPE_OUI};
    ret.oui = r24be(e, offset, NOMASK);
    if (ret.oui) {
        *dest = ret;
        return 1;
    }
    return 0;
}

static int _block_check_n(const void *bytes, int len) {
    if (!bytes) return 0;
    uint8_t sum = 0;
    uint8_t *data = (uint8_t*)bytes;
    int i;
    for(i=0; i<len; i++)
        sum += data[i];
    return sum == 0 ? 1 : 0;
}

static int block_check_n(edid *e, uint32_t offset, int len) {
    if (!bounds_check(e, offset+len)) return 0;
    return _block_check_n(e->u8, len);
}

static int block_check(edid *e, uint32_t offset) {
    if (!bounds_check(e, offset+128)) return 0;
    return _block_check_n(e->u8, 128);
}

static char *hex_bytes(uint8_t *bytes, int count) {
    char *buffer = malloc(count*3+1), *p = buffer;
    memset(buffer, 0, count*3+1);
    int i;
    for(i = 0; i < count; i++) {
        sprintf(p, "%02x ", (unsigned int)bytes[i]);
        p += 3;
    }
    return buffer;
}

#define OUTPUT_CPY_SIZE(DEST, SRC)  \
    (DEST).horiz_cm = (SRC).horiz_cm; \
    (DEST).vert_cm = (SRC).vert_cm; \
    (DEST).diag_cm = (SRC).diag_cm; \
    (DEST).diag_in = (SRC).diag_in; \
    edid_output_fill(&(DEST));

static void edid_output_fill(edid_output *out) {
    out->diag_cm =
        sqrt( (out->horiz_cm * out->horiz_cm)
         + (out->vert_cm * out->vert_cm) );
    out->diag_in = out->diag_cm / 2.54;

    if (out->is_interlaced) {
        if (out->vert_lines)
            out->vert_pixels = out->vert_lines * 2;
        else
            out->vert_lines = out->vert_pixels / 2;
    } else {
        if (out->vert_lines)
            out->vert_pixels = out->vert_lines;
        else
            out->vert_lines = out->vert_pixels;
    }

    if (!out->vert_freq_hz && out->pixel_clock_khz) {
        uint64_t h = out->horiz_pixels + out->horiz_blanking;
        uint64_t v = out->vert_lines + out->vert_blanking;
        if (h && v) {
            uint64_t work = out->pixel_clock_khz * 1000;
            work /= (h*v);
            out->vert_freq_hz = work;
        }
    }

    out->pixels = out->horiz_pixels;
    out->pixels *= out->vert_pixels;

    if (out->diag_in) {
        static const char *inlbl = "\u2033"; /* double prime */
        sprintf(out->class_inch, "%0.1f%s", out->diag_in, inlbl);
        util_strchomp_float(out->class_inch);
    }
}

static void cea_block_decode(struct edid_cea_block *blk) {
    if (!blk) return;
    if (!blk->bounds_ok)
        blk->bounds_ok =
        bounds_check(blk->addy.e, blk->addy.offset + 1 + blk->len);
    if (!blk->bounds_ok) return;

    edid *e = blk->addy.e;
    uint8_t *ptr = DPTR(blk->addy);
    int i;
    switch(blk->type) {
        case 0x1: /* SADS */
            for(i = 1; i <= blk->len; i+=3) {
                struct edid_sad *sad = &e->sads[e->sad_count];
                sad->v[0] = ptr[i];
                sad->v[1] = ptr[i+1];
                sad->v[2] = ptr[i+2];
                sad->format = bf_value(sad->v[0], 0x78);
                sad->channels = 1 + bf_value(sad->v[0], 0x07);
                sad->freq_bits = sad->v[1];
                if (sad->format == 1) {
                    sad->depth_bits = sad->v[2];
                } else if (sad->format >= 2
                        && sad->format <= 8) {
                    sad->max_kbps = 8 * sad->v[2];
                }
                e->sad_count++;
            }
            break;
        case 0x4: /* Speaker allocation */
            e->speaker_alloc_bits = ptr[1];
            break;
        case 0x2: /* SVDs */
            for(i = 1; i <= blk->len; i++)
                e->svds[e->svd_count++].v = ptr[i];
            break;
        case 0x3: /* Vendor-specific */
            // TODO:
        default:
            break;
    }
}

static void did_block_decode(DisplayIDBlock *blk) {
    if (!blk) return;

    //printf("did_block_decode: %s\n", hex_bytes(DPTR(blk->addy), blk->len+3));

    if (!blk->bounds_ok)
        blk->bounds_ok =
        bounds_check(blk->addy.e, blk->addy.offset + 3 + blk->len);
    if (!blk->bounds_ok) return;

    edid *e = blk->addy.e;
    uint32_t a = blk->addy.offset + 3;

    uint8_t *u8 = DPTR(blk->addy);
    int b = 3;
    edid_ven ven = {};
    edid_output out = {};
    if (blk) {
        switch(blk->type) {
            case 0:     /* Product ID (1.x) */
                /* UNTESTED */
                if (rpnpcpy(&ven, e, a) )
                    e->ven = ven;
                if (u8[12] || u8[13]) {
                    e->dom.week = u8[12];
                    e->dom.year = u8[13] + 2000;
                    e->dom.is_model_year = (e->dom.week == 255);
                    e->dom.std = STD_DISPLAYID;
                }
                e->did_strings[e->did_string_count].is_product_name = 1;
                e->did_strings[e->did_string_count].len = blk->len;
                e->did_strings[e->did_string_count].str = rstr_strip(e, a+12, u8[b+11]);
                e->name = e->did_strings[e->did_string_count].str;
                e->did_string_count++;
                break;
            case 0x20:  /* Product ID */
                /* UNTESTED */
                if (rouicpy(&ven, e, a) )
                    e->ven = ven;
                if (u8[12] || u8[13]) {
                    e->dom.week = u8[12];
                    e->dom.year = u8[13] + 2000;
                    e->dom.is_model_year = (e->dom.week == 255);
                    e->dom.std = STD_DISPLAYID20;
                }
                e->did_strings[e->did_string_count].is_product_name = 1;
                e->did_strings[e->did_string_count].len = blk->len;
                e->did_strings[e->did_string_count].str = rstr_strip(e, a+12, u8[b+11]);
                e->name = e->did_strings[e->did_string_count].str;
                e->did_string_count++;
                break;
            case 0x0a: /* Serial Number (ASCII String) */
                e->did_strings[e->did_string_count].is_serial = 1;
                e->did_strings[e->did_string_count].len = blk->len;
                e->did_strings[e->did_string_count].str = rstr_strip(e, a, blk->len);
                e->serial = e->did_strings[e->did_string_count].str;
                e->did_string_count++;
                break;
            case 0x0b: /* General Purpose ASCII String */
                e->did_strings[e->did_string_count].len = blk->len;
                e->did_strings[e->did_string_count].str = rstr(e, a, blk->len);
                e->did_string_count++;
                break;
            case 0x03: /* Type I Detailed timings */
                out.pixel_clock_khz = 10 * r24le(e, a, NOMASK);
                out.horiz_pixels    = 1 + (u8[b+5] << 8) + u8[b+4];
                out.horiz_blanking  = (u8[b+7] << 8) + u8[b+6];
                out.vert_lines      = 1 + (u8[b+13] << 8) + u8[b+12];
                out.vert_blanking   = (u8[b+15] << 8) + u8[b+14];
                out.is_interlaced   = bf_value(u8[b+3], BFMASK(4, 0x1));
                out.stereo_mode     = bf_value(u8[b+3], BFMASK(5, 0x3));
                out.is_preferred    = bf_value(u8[b+3], BFMASK(7, 0x1));
                out.src = OUTSRC_DID_TYPE_I;
                edid_output_fill(&out);
                e->didts[e->didt_count++] = out;
                break;
            case 0x13: /* Type VI Detailed timings (super 0x03) */
                /* UNTESTED */
                out.pixel_clock_khz = (u8[b+2] << 16) & 0xa0;
                out.pixel_clock_khz += u8[b+1] << 8;
                out.pixel_clock_khz += u8[b];
                out.horiz_pixels = ((u8[b+5] << 8) + u8[b+3]) & 0x7fff;
                out.vert_lines = ((u8[b+6] << 8) + u8[b+5]) & 0x7fff;
                // TODO: blanking...
                out.is_interlaced = (u8[b+13] >> 7) & 0x1;
                out.stereo_mode = (u8[b+13] >> 5) & 0x2;
                out.src = OUTSRC_DID_TYPE_VI;
                edid_output_fill(&out);
                e->didts[e->didt_count++] = out;
                break;
            case 0x22: /* Type VII Detailed timings (super 0x13) */
                /* UNTESTED */
                out.pixel_clock_khz = u8[b+2] << 16;
                out.pixel_clock_khz += u8[b+1] << 8;
                out.pixel_clock_khz += u8[b];
                out.horiz_pixels = (u8[b+5] << 8) + u8[b+4];
                out.horiz_blanking = (u8[b+7] << 8) + u8[b+6];
                out.vert_lines = (u8[b+13] << 8) + u8[b+12];
                out.vert_blanking = (u8[b+15] << 8) + u8[b+14];
                out.is_interlaced = (u8[b+3] >> 4) & 0x1;
                out.stereo_mode = (u8[b+3] >> 5) & 0x2;
                out.is_preferred = (u8[b+3] >> 7) & 0x1;
                out.src = OUTSRC_DID_TYPE_VII;
                edid_output_fill(&out);
                e->didts[e->didt_count++] = out;
                break;
            case 0x81: /* CTA DisplayID, ... Embedded CEA Blocks */
                while(b < blk->len) {
                    int db_type = (u8[b] & 0xe0) >> 5;
                    int db_size =  u8[b] & 0x1f;
                    e->cea_blocks[e->cea_block_count].addy.e = blk->addy.e;
                    e->cea_blocks[e->cea_block_count].addy.offset = blk->addy.offset + b;
                    e->cea_blocks[e->cea_block_count].type = db_type;
                    e->cea_blocks[e->cea_block_count].len = db_size;
                    cea_block_decode(&e->cea_blocks[e->cea_block_count]);
                    e->cea_block_count++;
                    b += db_size + 1;
                }
                break;
            default:
                break;
        }
    }
}

static edid_output edid_output_from_svd(uint8_t index) {
    int i;
    if (index >= 128 && index <= 192) index &= 0x7f; /* "native" flag for 0-64 */
    for(i = 0; i < (int)G_N_ELEMENTS(cea_standard_timings); i++) {
        if (cea_standard_timings[i].index == index) {
            edid_output out = {};
            out.horiz_pixels = cea_standard_timings[i].horiz_active;
            out.vert_lines = cea_standard_timings[i].vert_active;
            if (strchr(cea_standard_timings[i].short_name, 'i'))
                out.is_interlaced = 1;
            out.pixel_clock_khz = cea_standard_timings[i].pixel_clock_mhz * 1000;
            out.vert_freq_hz = cea_standard_timings[i].vert_freq_hz;
            out.src = OUTSRC_SVD;
            edid_output_fill(&out);
            return out;
        }
    }
    return (edid_output){.src = OUTSRC_INVALID};
}

edid *edid_new(const char *data, unsigned int len) {
    if (len < 128) return NULL;

    int i;
    edid *e = malloc(sizeof(edid));
    memset(e, 0, sizeof(edid));
    e->data = malloc(len);
    memcpy(e->data, data, len);
    e->len = len;
    e->ver_major = e->u8[18];
    e->ver_minor = e->u8[19];

    e->msg_log = g_string_new(NULL);

#define RESERVE_COUNT 300
    e->dtds = malloc(sizeof(struct edid_dtd) * RESERVE_COUNT);
    e->cea_blocks = malloc(sizeof(struct edid_cea_block) * RESERVE_COUNT);
    e->svds = malloc(sizeof(struct edid_svd) * RESERVE_COUNT);
    e->sads = malloc(sizeof(struct edid_sad) * RESERVE_COUNT);
    e->did_blocks = malloc(sizeof(DisplayIDBlock) * RESERVE_COUNT);
    e->didts = malloc(sizeof(edid_output) * RESERVE_COUNT);
    e->did_strings = malloc(sizeof(edid_output) * RESERVE_COUNT);
    memset(e->dtds, 0, sizeof(struct edid_dtd) * RESERVE_COUNT);
    memset(e->cea_blocks, 0, sizeof(struct edid_cea_block) * RESERVE_COUNT);
    memset(e->svds, 0, sizeof(struct edid_svd) * RESERVE_COUNT);
    memset(e->sads, 0, sizeof(struct edid_sad) * RESERVE_COUNT);
    memset(e->did_blocks, 0, sizeof(DisplayIDBlock) * RESERVE_COUNT);
    memset(e->didts, 0, sizeof(edid_output) * RESERVE_COUNT);
    memset(e->did_strings, 0, sizeof(edid_output) * RESERVE_COUNT);

    /* base product information */
    rpnpcpy(&e->ven, e, 8);             /* bytes 8-9 */
    e->product = r16le(e, 10, NOMASK);  /* bytes 10-11 */
    e->n_serial = r32le(e, 12, NOMASK); /* bytes 12-15 */
    e->dom.week = e->u8[16];            /* byte 16 */
    e->dom.year = e->u8[17] + 1990;     /* byte 17 */
    e->dom.is_model_year = (e->dom.week == 255);
    e->dom.std = STD_EDID;

    e->a_or_d = (e->u8[20] & 0x80) ? 1 : 0;
    if (e->a_or_d == 1) {
        /* digital */
        switch((e->u8[20] >> 4) & 0x7) {
            case 0x1: e->bpc = 6; break;
            case 0x2: e->bpc = 8; break;
            case 0x3: e->bpc = 10; break;
            case 0x4: e->bpc = 12; break;
            case 0x5: e->bpc = 14; break;
            case 0x6: e->bpc = 16; break;
        }
        e->interface = e->u8[20] & 0xf;
    }

    if (e->u8[21] && e->u8[22]) {
        e->img.horiz_cm = e->u8[21];
        e->img.vert_cm = e->u8[22];
        edid_output_fill(&e->img);
        e->img_max = e->img;
    }

    /* established timing bitmap */
#define ETB_CHECK(byte, bit, h, v, f, i)         \
    if (byte & (1<<bit)) {                       \
        e->etbs[e->etb_count] = e->img;          \
        e->etbs[e->etb_count].horiz_pixels = h;  \
        e->etbs[e->etb_count].vert_pixels = v;   \
        e->etbs[e->etb_count].vert_freq_hz = f;  \
        e->etbs[e->etb_count].is_interlaced = i; \
        e->etbs[e->etb_count].src = OUTSRC_ETB;  \
        edid_output_fill(&e->etbs[e->etb_count]);\
        e->etb_count++; };
    ETB_CHECK(35, 7, 720, 400, 70, 0); //(VGA)
    ETB_CHECK(35, 6, 720, 400, 88, 0); //(XGA)
    ETB_CHECK(35, 5, 640, 480, 60, 0); //(VGA)
    ETB_CHECK(35, 4, 640, 480, 67, 0); //(Apple Macintosh II)
    ETB_CHECK(35, 3, 640, 480, 72, 0);
    ETB_CHECK(35, 2, 640, 480, 75, 0);
    ETB_CHECK(35, 1, 800, 600, 56, 0);
    ETB_CHECK(35, 0, 800, 600, 60, 0);
    ETB_CHECK(36, 7, 800, 600, 72, 0);
    ETB_CHECK(36, 6, 800, 600, 75, 0);
    ETB_CHECK(36, 5, 832, 624, 75, 0); //(Apple Macintosh II)
    ETB_CHECK(36, 4, 1024, 768, 87, 1); //(1024×768i)
    ETB_CHECK(36, 3, 1024, 768, 60, 0);
    ETB_CHECK(36, 2, 1024, 768, 70, 0);
    ETB_CHECK(36, 1, 1024, 768, 75, 0);
    ETB_CHECK(36, 0, 1280, 1024, 75, 0);
    ETB_CHECK(35, 7, 1152, 870, 75, 0); //(Apple Macintosh II)

    /* standard timings */
    for(i = 38; i < 53; i+=2) {
        /* 0101 is unused */
        if (e->u8[i] == 0x01 && e->u8[i+1] == 0x01)
            continue;
        /* 00.. is invalid/"reserved" */
        if (e->u8[i] == 0x00)
            continue;
        double xres = (e->u8[i] + 31) * 8;
        double yres = 0;
        int iar = (e->u8[i+1] >> 6) & 0x3;
        int vf = (e->u8[i+1] & 0x3f) + 60;
        switch(iar) {
            case 0: /* 16:10 (v<1.3 1:1) */
                if (e->ver_major == 1 && e->ver_minor < 3)
                    yres = xres;
                else
                    yres = xres*10/16;
                break;
            case 0x1: /* 4:3 */
                yres = xres*4/3;
                break;
            case 0x2: /* 5:4 */
                yres = xres*4/5;
                break;
            case 0x3: /* 16:9 */
                yres = xres*9/16;
                break;
        }
        e->stds[e->std_count].ptr = &e->u8[i];
        e->stds[e->std_count].out = e->img; /* inherit */
        e->stds[e->std_count].out.horiz_pixels = xres;
        e->stds[e->std_count].out.vert_pixels = yres;
        e->stds[e->std_count].out.vert_freq_hz = vf;
        e->stds[e->std_count].out.src = OUTSRC_STD;
        edid_output_fill(&e->stds[e->std_count].out);
        e->std_count++;
    }

    uint16_t dh, dl;
#define CHECK_DESCRIPTOR(INDEX, OFFSET)       \
    e->d[INDEX].addy.e = e;                   \
    e->d[INDEX].addy.offset = OFFSET;         \
    if (e->u8[OFFSET] == 0) {                 \
        dh = be16toh(e->u16[OFFSET/2]);       \
        dl = be16toh(e->u16[OFFSET/2+1]);     \
        e->d[INDEX].type = (dh << 16) + dl;   \
        switch(e->d[INDEX].type) {            \
            case 0xfc: case 0xff: case 0xfe:  \
                strncpy(e->d[INDEX].text, (char*)e->u8+OFFSET+5, 13);  \
        }                                     \
    } else e->dtds[e->dtd_count++].addy = e->d[INDEX].addy;

    CHECK_DESCRIPTOR(0, 54);
    CHECK_DESCRIPTOR(1, 72);
    CHECK_DESCRIPTOR(2, 90);
    CHECK_DESCRIPTOR(3, 108);

    e->checksum_ok = block_check(e, 0); /* first 128-byte block only */
    if (len > 128) {
        /* check extension blocks */
        int blocks = len / 128;
        blocks--;
        e->ext_blocks = blocks;
        e->ext_ok = malloc(sizeof(uint8_t) * blocks);
        for(; blocks; blocks--) {
            uint32_t offset = blocks * 128;
            uint8_t *u8 = e->u8 + offset;
            int r = block_check(e, offset);
            e->ext_ok[blocks-1] = r;
            if (r) e->ext_blocks_ok++;
            else e->ext_blocks_fail++;

            if (u8[0] == 0x70) {
                /* DisplayID */
                e->did.version = u8[1];
                if (e->did.version >= 0x20)
                    e->std = MAX(e->std, STD_DISPLAYID20);
                else e->std = MAX(e->std, STD_DISPLAYID);
                e->did.extension_length = u8[2];
                e->did.primary_use_case = u8[3];
                e->did.extension_count  = u8[4];
                e->did.checksum_ok = block_check_n(e, offset, e->did.extension_length + 5);
                int db_end = u8[2] + 5;
                int b = 5;
                while(b < db_end) {
                    if (r24le(e, offset + b, NOMASK) == 0) break;
                    int db_type = u8[b];
                    int db_revision = u8[b+1] & 0x7;
                    int db_size = u8[b+2];
                    e->did_blocks[e->did_block_count].addy.e = e;
                    e->did_blocks[e->did_block_count].addy.offset = offset + b;
                    e->did_blocks[e->did_block_count].type = db_type;
                    e->did_blocks[e->did_block_count].revision = db_revision;
                    e->did_blocks[e->did_block_count].len = db_size;
                    did_block_decode(&e->did_blocks[e->did_block_count]);
                    e->did_block_count++;
                    e->did.blocks++;
                    b += db_size + 3;
                }
                if (b > db_end)
                    edid_msg(e, "DID block overrun [in ext " OFMT "], expect to end at +%d, but last ends at +%d" , offset, db_end-1, b-1);
                //printf("DID: v:%02x el:%d uc:%d ec:%d, blocks:%d ok:%d\n",
                //    e->did.version, e->did.extension_length,
                //    e->did.primary_use_case, e->did.extension_count,
                //    e->did.blocks, e->checksum_ok);
            }

            if (u8[0] == 0x02) {
                e->std = MAX(e->std, STD_EIACEA861);
                /* CEA extension */
                int db_end = u8[2];
                if (db_end) {
                    int b = 4;
                    while(b < db_end) {
                        int db_type = (u8[b] & 0xe0) >> 5;
                        int db_size =  u8[b] & 0x1f;
                        e->cea_blocks[e->cea_block_count].addy.e = e;
                        e->cea_blocks[e->cea_block_count].addy.offset = offset + b;
                        e->cea_blocks[e->cea_block_count].type = db_type;
                        e->cea_blocks[e->cea_block_count].len = db_size;
                        cea_block_decode(&e->cea_blocks[e->cea_block_count]);
                        e->cea_block_count++;
                        b += db_size + 1;
                    }
                    if (b > db_end) {
                        b = db_end;
                        edid_msg(e, "CEA block overrun [in ext " OFMT "], expect to end at +%d, but last ends at +%d" , offset, db_end-1, b-1);
                    }
                    /* DTDs */
                    while(b < 127) {
                        if (u8[b]) {
                            e->dtds[e->dtd_count].addy.e = e;
                            e->dtds[e->dtd_count].addy.offset = offset + b;
                            e->dtds[e->dtd_count].cea_ext = 1;
                            e->dtd_count++;
                        }
                        b += 18;
                    }
                }
            }
        }
    }
    if (e->ext_blocks_ok) {
        e->std = MAX(e->std, STD_EEDID);
    }

    /* strings */
    for(i = 0; i < 4; i++) {
        if (!g_utf8_validate(e->d[i].text, -1, NULL)) {
            strcpy(e->d[i].text, "(INVALID)");
        }
        g_strstrip(e->d[i].text);
        switch(e->d[i].type) {
            case 0xfc:
                e->name = e->d[i].text;
                break;
            case 0xff:
                e->serial = e->d[i].text;
                break;
            case 0xfe:
                if (e->ut1)
                    e->ut2 = e->d[i].text;
                else
                    e->ut1 = e->d[i].text;
                break;
        }
    }

    /* quirks */
    if (!e->name) {
        if (SEQ(e->ut1, "LG Display") && e->ut2)
            /* LG may use "uspecified text" for name and model */
            e->name = e->ut2;
        else if (SEQ(e->ut1, "AUO") && e->ut2)
            /* Same with AUO */
            e->name = e->ut2;
        else {
            if (e->ut1) e->name = e->ut1;
            if (e->ut2 && !e->serial) e->serial = e->ut2;
        }
    }

    /* largest in ETB */
    for (i = 0; i < e->etb_count; i++) {
        if (e->etbs[i].pixels > e->img.pixels)
            e->img = e->etbs[i];
        if (e->etbs[i].pixels > e->img_max.pixels)
            e->img_max = e->etbs[i];
    }

    /* largest in STDs */
    for (i = 0; i < e->std_count; i++) {
        if (e->stds[i].out.pixels > e->img.pixels)
            e->img = e->stds[i].out;
        if (e->stds[i].out.pixels > e->img_max.pixels)
            e->img_max = e->stds[i].out;
    }

    /* dtds */
    for(i = 0; i < e->dtd_count; i++) {
        edid_addy a = e->dtds[i].addy;
        if (!e->dtds[i].bounds_ok)
            e->dtds[i].bounds_ok = bounds_check(a.e, a.offset + 18);
        if (!e->dtds[i].bounds_ok) {
            printf("bounds fail\n");
            exit(0);
        }
        uint8_t *u8 = DPTR(a);
        edid_output *out = &e->dtds[i].out;
        if (e->dtds[i].cea_ext) out->src = OUTSRC_CEA_DTD;
        else out->src = OUTSRC_DTD;
        out->pixel_clock_khz = 10 * r16le(a.e, a.offset, NOMASK);
        out->horiz_pixels =
            ((u8[4] & 0xf0) << 4) + u8[2];
        out->vert_lines =
            ((u8[7] & 0xf0) << 4) + u8[5];

        out->horiz_blanking =
            ((u8[4] & 0x0f) << 8) + u8[3];
        out->vert_blanking =
            ((u8[7] & 0x0f) << 8) + u8[6];

        out->horiz_cm =
            ((u8[14] & 0xf0) << 4) + u8[12];
        out->horiz_cm /= 10;
        out->vert_cm =
            ((u8[14] & 0x0f) << 8) + u8[13];
        out->vert_cm /= 10;
        out->is_interlaced = (u8[17] & 0x80) >> 7;
        out->stereo_mode = (u8[17] & 0x60) >> 4;
        out->stereo_mode += u8[17] & 0x01;
        edid_output_fill(out);
    }

    if (e->dtd_count) {
        /* first DTD is "preferred" */
        e->img_max = e->dtds[0].out;
    }

    /* svds */
    for(i = 0; i < e->svd_count; i++) {
        e->svds[i].out = edid_output_from_svd(e->svds[i].v);
        if (e->svds[i].out.src == OUTSRC_INVALID)
            continue;

        if (e->svds[i].v >= 128 &&
            e->svds[i].v <= 192) {
            e->svds[i].is_native = 1;

            edid_output tmp = e->img_max;
            /* native res is max real res, right? */
            e->img_max = e->svds[i].out;
            e->img_max.is_preferred = 1;
            OUTPUT_CPY_SIZE(e->img_max, tmp);
        }
        if (e->svds[i].out.pixels > e->img_svd.pixels
            || e->svds[i].is_native) {
            e->img_svd = e->svds[i].out;
            if (e->svds[i].is_native)
                e->img_svd.is_preferred = 1;
            OUTPUT_CPY_SIZE(e->img_svd, e->img_max);
        }
    }
    /* remove invalid SVDs */
    int d = 0;
    for(i = 0; i < e->svd_count; i++) {
        if (d != i)
            e->svds[d].out = e->svds[i].out;
        if (e->svds[i].out.src != OUTSRC_INVALID)
            d++;
    }
    e->svd_count -= (i-d);

    /* didts */
    for(i = 0; i < e->didt_count; i++) {
        int pref = e->didts[i].is_preferred;
        int max_pref = e->img_max.is_preferred;
        int bigger = (e->didts[i].pixels > e->img_max.pixels);
        int better = (e->didts[i].src > e->img_max.src);
        if ((bigger && !max_pref)
            || (pref && !max_pref)
            || (better)) {
            edid_output tmp = e->img_max;
            e->img_max = e->didts[i];
            OUTPUT_CPY_SIZE(e->img_max, tmp);
        }
    }

    if (!e->speaker_alloc_bits && e->sad_count) {
        /* make an assumption */
        if (e->sads[0].channels == 2)
            e->speaker_alloc_bits = 0x1;
    }

    /* squeeze lists */
#define SQUEEZE(C, L) \
    if (!e->C) { free(e->L); e->L = NULL; } \
    else { e->L = realloc(e->L, sizeof(e->L[0]) * (e->C)); }

    SQUEEZE(dtd_count, dtds);
    SQUEEZE(cea_block_count, cea_blocks);
    SQUEEZE(svd_count, svds);
    SQUEEZE(sad_count, sads);
    SQUEEZE(did_block_count, did_blocks);
    SQUEEZE(didt_count, didts);
    SQUEEZE(did_string_count, did_strings);
    return e;
}

void edid_free(edid *e) {
    int i;
    if (e) {
        g_free(e->ext_ok);
        g_free(e->cea_blocks);
        g_free(e->dtds);
        g_free(e->svds);
        g_free(e->sads);
        g_free(e->did_blocks);
        g_free(e->didts);
        for(i = 0; i < e->did_string_count; i++)
            g_free(e->did_strings[i].str);
        g_free(e->did_strings);
        g_free(e->data);
        g_string_free(e->msg_log, TRUE);
        g_free(e);
    }
}

edid *edid_new_from_hex(const char *hex_string) {
    int blen = strlen(hex_string) / 2;
    uint8_t *buffer = malloc(blen), *n = buffer;
    memset(buffer, 0, blen);
    int len = 0;

    const char *p = hex_string;
    char byte[3] = "..";

    while(p && *p) {
        if (isxdigit(p[0]) && isxdigit(p[1])) {
            byte[0] = p[0];
            byte[1] = p[1];
            *n = strtol(byte, NULL, 16);
            n++;
            len++;
            p += 2;
        } else
            p++;
    }

    edid *e = edid_new((char*)buffer, len);
    free(buffer);
    return e;
}

edid *edid_new_from_file(const char *path) {
    char *bin = NULL;
    gsize len = 0;
    if (g_file_get_contents(path, &bin, &len, NULL) ) {
        edid *ret = edid_new(bin, len);
        g_free(bin);
        return ret;
    }
    return NULL;
}

char *edid_dump_hex(edid *e, int tabs, int breaks) {
    if (!e) return NULL;
    int lines = 1 + (e->len / 16);
    int blen = lines * 35 + 1;
    unsigned int pc = 0;
    char *ret = malloc(blen);
    memset(ret, 0, blen);
    uint8_t *u8 = e->u8;
    char *p = ret;
    for(; lines; lines--) {
        int i, d = MIN(16, (e->len - pc));
        if (!d) break;
        for(i = 0; i < tabs; i++)
            sprintf(p++, "\t");
        for(i = d; i; i--) {
            sprintf(p, "%02x", (unsigned int)*u8);
            p+=2;
            u8++;
            pc++;
            if (pc == e->len) {
                if (breaks) sprintf(p++, "\n");
                goto edid_dump_hex_done;
            }
        }
        if (breaks) sprintf(p++, "\n");
    }
edid_dump_hex_done:
    return ret;
}

const char *edid_standard(int std) {
    switch(std) {
        case STD_EDID: return N_("VESA EDID");
        case STD_EEDID: return N_("VESA E-EDID");
        case STD_EIACEA861: return N_("EIA/CEA-861");
        case STD_DISPLAYID: return N_("VESA DisplayID");
        case STD_DISPLAYID20: return N_("VESA DisplayID 2.0");
    };
    return N_("unknown");
}

const char *edid_output_src(int src) {
    switch(src) {
        case OUTSRC_EDID: return N_("VESA EDID");
        case OUTSRC_ETB: return N_("VESA EDID ETB");
        case OUTSRC_STD: return N_("VESA EDID STD");
        case OUTSRC_DTD: return N_("VESA EDID DTD");
        case OUTSRC_CEA_DTD: return N_("EIA/CEA-861 DTD");
        case OUTSRC_SVD: return N_("EIA/CEA-861 SVD");
        case OUTSRC_DID_TYPE_I: return N_("DisplayID Type I");
        case OUTSRC_DID_TYPE_VI: return N_("DisplayID Type VI");
        case OUTSRC_DID_TYPE_VII: return N_("DisplayID Type VII");
    };
    return N_("unknown");
}

const char *edid_interface(int type) {
    switch(type) {
        case 0: return N_("undefined");
        case 0x2: return N_("HDMIa");
        case 0x3: return N_("HDMIb");
        case 0x4: return N_("MDDI");
        case 0x5: return N_("DisplayPort");
    };
    return N_("unknown");
}

const char *edid_cea_audio_type(int type) {
    switch(type) {
        case 0: case 15: return N_("reserved");
        case 1: return N_("LPCM");
        case 2: return N_("AC-3");
        case 3: return N_("MPEG1 (layers 1 and 2)");
        case 4: return N_("MPEG1 layer 3");
        case 5: return N_("MPEG2");
        case 6: return N_("AAC");
        case 7: return N_("DTS");
        case 8: return N_("ATRAC");
        case 9: return N_("DSD");
        case 10: return N_("DD+");
        case 11: return N_("DTS-HD");
        case 12: return N_("MLP/Dolby TrueHD");
        case 13: return N_("DST Audio");
        case 14: return N_("WMA Pro");
    }
    return N_("unknown type");
}

const char *edid_cea_block_type(int type) {
    switch(type) {
        case 0x01:
            return N_("audio");
        case 0x02:
            return N_("video");
        case 0x03:
            return N_("vendor specific");
        case 0x04:
            return N_("speaker allocation");
    }
    return N_("unknown type");
}

const char *edid_did_block_type(int type) {
    switch(type) {
        /* 1.x */
        case 0x00: return N_("Product Identification (1.x)");
        case 0x01: return N_("Display Parameters (1.x)");
        case 0x02: return N_("Color Characteristics (1.x)");
        case 0x03: return N_("Type I Timing - Detailed (1.x)");
        case 0x04: return N_("Type II Timing - Detailed (1.x)");
        case 0x05: return N_("Type III Timing - Short (1.x)");
        case 0x06: return N_("Type IV Timing - DMT ID Code (1.x)");
        case 0x07: return N_("VESA Timing Standard (1.x)");
        case 0x08: return N_("CEA Timing Standard (1.x)");
        case 0x09: return N_("Video Timing Range (1.x)");
        case 0x0A: return N_("Product Serial Number (1.x)");
        case 0x0B: return N_("General Purpose ASCII String (1.x)");
        case 0x0C: return N_("Display Device Data (1.x)");
        case 0x0D: return N_("Interface Power Sequencing (1.x)");
        case 0x0E: return N_("Transfer Characteristics (1.x)");
        case 0x0F: return N_("Display Interface Data (1.x)");
        case 0x10: return N_("Stereo Display Interface (1.x)");
        case 0x11: return N_("Type V Timing - Short (1.x)");
        case 0x12: return N_("Tiled Display Topology (1.x)");
        case 0x13: return N_("Type VI Timing - Detailed (1.x)");
        case 0x7F: return N_("Vendor specific (1.x)");
        /* 2.x */
        case 0x20: return N_("Product Identification");
        case 0x21: return N_("Display Parameters");
        case 0x22: return N_("Type VII - Detailed Timing");
        case 0x23: return N_("Type VIII - Enumerated Timing Code");
        case 0x24: return N_("Type IX - Formula-based Timing");
        case 0x25: return N_("Dynamic Video Timing Range Limits");
        case 0x26: return N_("Display Interface Features");
        case 0x27: return N_("Stereo Display Interface");
        case 0x28: return N_("Tiled Display Topology");
        case 0x29: return N_("ContainerID");
        case 0x7E: return N_("Vendor specific");
        case 0x81: return N_("CTA DisplayID");
    }
    return N_("unknown type");
}

const char *edid_ext_block_type(int type) {
    switch(type) {
        case 0x00:
            return N_("timing extension");
        case 0x02:
            return N_("EIA/CEA-861 extension block");
        case 0x10:
            return N_("video timing block");
        case 0x20:
            return N_("EDID 2.0 extension");
        case 0x40:
            return N_("DVI feature data/display information");
        case 0x50:
            return N_("localized strings");
        case 0x60:
            return N_("microdisplay interface");
        case 0x70:
            return N_("DisplayID");
        case 0xa7:
        case 0xaf:
        case 0xbf:
            return N_("display transfer characteristics data block");
        case 0xf0:
            return N_("extension block map");
        case 0xff:
            return N_("manufacturer-defined extension/display device data block");
    }
    return N_("unknown block type");
}

const char *edid_descriptor_type(int type) {
    switch(type) {
        case 0xff:
            return N_("display serial number");
        case 0xfe:
            return N_("unspecified text");
        case 0xfd:
            return N_("display range limits");
        case 0xfc:
            return N_("display name");
        case 0xfb:
            return N_("additional white point");
        case 0xfa:
            return N_("additional standard timing identifiers");
        case 0xf9:
            return N_("Display Color Management");
        case 0xf8:
            return N_("CVT 3-byte timing codes");
        case 0xf7:
            return N_("additional standard timing");
        case 0x10:
            return N_("dummy");
    }
    if (type && type < 0x0f)
        return N_("manufacturer reserved descriptor");
    return N_("detailed timing descriptor");
}

char *edid_cea_audio_describe(struct edid_sad *sad) {
    if (!sad) return NULL;

    if (!sad->format)
        return  g_strdup_printf("format:([%x] %s)",
            sad->format, _(edid_cea_audio_type(sad->format)) );

    gchar *ret = NULL;
    gchar *tmp[3] = {};
#define appfreq(b, f) if (sad->freq_bits & (1 << b)) tmp[0] = appf(tmp[0], ", ", "%d", f);
#define appdepth(b, d) if (sad->depth_bits & (1 << b)) tmp[1] = appf(tmp[1], ", ", "%d%s", d, _("-bit"));
    appfreq(0, 32);
    appfreq(1, 44);
    appfreq(2, 48);
    appfreq(3, 88);
    appfreq(4, 96);
    appfreq(5, 176);
    appfreq(6, 192);

    if (sad->format == 1) {
        appdepth(0, 16);
        appdepth(1, 20);
        appdepth(2, 24);
        tmp[2] = g_strdup_printf("depths: %s", tmp[1]);
    } else if (sad->format >= 2
                && sad->format <= 8 ) {
        tmp[2] = g_strdup_printf("max_bitrate: %d %s", sad->max_kbps, _("kbps"));
    } else
        tmp[2] = g_strdup("");

    ret = g_strdup_printf("format:([%x] %s) channels:%d rates:%s %s %s",
        sad->format, _(edid_cea_audio_type(sad->format)),
        sad->channels, tmp[0], _("kHz"),
        tmp[2]);
    g_free(tmp[0]);
    g_free(tmp[1]);
    g_free(tmp[2]);
    return ret;
}

char *edid_cea_speaker_allocation_describe(int bitfield, int short_version) {
    gchar *spk_list = NULL;
#define appspk(b, sv, fv) if (bitfield & (1 << b)) \
    spk_list = appf(spk_list, short_version ? ", " : "\n", "%s", short_version ? sv : fv);

    appspk(0, "FL+FR", _("Front left and right"));
    appspk(1, "LFE", _("Low-frequency effects"));
    appspk(2, "FC", _("Front center"));
    appspk(3, "RL+RR", _("Rear left and right"));
    appspk(4, "RC", _("Rear center"));
    appspk(5, "???", _(""));
    appspk(6, "???", _(""));

    return spk_list;
}

char *edid_cea_block_describe(struct edid_cea_block *blk) {
    gchar *ret = NULL;
    if (blk) {
        char *hb = hex_bytes(DPTR(blk->addy), blk->len+1);
        switch(blk->type) {
            case 0x1: /* SAD list */
                ret = g_strdup_printf("([%x] %s) sads:%d",
                    blk->type, _(edid_cea_block_type(blk->type)),
                    blk->len/3);
                break;
            case 0x2: /* SVD list */
                ret = g_strdup_printf("([%x] %s) svds:%d",
                    blk->type, _(edid_cea_block_type(blk->type)),
                    blk->len);
                break;
            case 0x4: /* speaker allocation */
                ret = g_strdup_printf("([%x] %s) len:%d",
                    blk->type, _(edid_cea_block_type(blk->type)),
                    blk->len);
                break;
            case 0x3: //TODO
            default:
                ret = g_strdup_printf("([%x] %s) len:%d -- %s",
                    blk->type, _(edid_cea_block_type(blk->type)),
                    blk->len,
                    hb);
                break;
        }
        free(hb);
    }
    return ret;
}

char *edid_base_descriptor_describe(struct edid_descriptor *d) {
    gchar *ret = NULL;
    if (d) {
        char *hb = hex_bytes(DPTR(d->addy), 18);
        char *txt = NULL;
        switch(d->type) {
            case 0: /* DTD */
                txt = "{...}"; /* displayed elsewhere */
                break;
            case 0x10: /* dummy */
                txt = "";
                break;
            default:
                txt = (*d->text) ? d->text : hb;
                break;
        };
        ret = g_strdup_printf("([%02x] %s) %s",
            d->type, _(edid_descriptor_type(d->type)),
            txt);
        free(hb);
    }
    return ret;
}

char *edid_did_block_describe(DisplayIDBlock *blk) {
    if (!blk) return NULL;

    gchar *ret = NULL;
    edid *e = blk->addy.e;
    uint32_t a = blk->addy.offset + 3;
    char *str = NULL;

    //printf("edid_did_block_describe: ((%d)) t:%02x a{%p, %d}...\n", blk->addy.e->did.extension_count, blk->type, blk->addy.e, blk->addy.offset);
    char *hb = hex_bytes(DPTR(blk->addy), blk->len+3);
    switch(blk->type) {
        case 0x0a: /* Product Serial ASCII string */
            str = rstr_strip(e, a, blk->len);
            ret = g_strdup_printf("([%02x:r%02x] %s) len:%d \"%s\"",
                blk->type, blk->revision, _(edid_did_block_type(blk->type)),
                blk->len,
                str);
            break;
        case 0x0b: /* ASCII string */
            str = rstr(e, a, blk->len);
            ret = g_strdup_printf("([%02x:r%02x] %s) len:%d \"%s\"",
                blk->type, blk->revision, _(edid_did_block_type(blk->type)),
                blk->len,
                str);
            break;
        default:
            ret = g_strdup_printf("([%02x:r%02x] %s) len:%d -- %s",
                blk->type, blk->revision, _(edid_did_block_type(blk->type)),
                blk->len,
                hb);
            break;
    }
    free(hb);

    return ret;
}

char *edid_output_describe(edid_output *out) {
    gchar *ret = NULL;
    if (out) {
        ret = g_strdup_printf("%dx%d@%.0f%s",
            out->horiz_pixels, out->vert_pixels, out->vert_freq_hz, _("Hz") );
        if (out->diag_cm)
            ret = appfsp(ret, "%0.1fx%0.1f%s (%0.1f\")",
                out->horiz_cm, out->vert_cm, _("cm"), out->diag_in );
        ret = appfsp(ret, "%s", out->is_interlaced ? "interlaced" : "progressive");
        ret = appfsp(ret, "%s", out->stereo_mode ? "stereo" : "normal");
    }
    return ret;
}

char *edid_dtd_describe(struct edid_dtd *dtd, int dump_bytes) {
    gchar *ret = NULL;
    if (dtd) {
        edid_output *out = &dtd->out;
        char *hb = hex_bytes(DPTR(dtd->addy), 18);
        ret = g_strdup_printf("%dx%d@%.0f%s, %0.1fx%0.1f%s (%0.1f\") %s %s (%s)%s%s",
            out->horiz_pixels, out->vert_lines, out->vert_freq_hz, _("Hz"),
            out->horiz_cm, out->vert_cm, _("cm"), out->diag_in,
            out->is_interlaced ? "interlaced" : "progressive",
            out->stereo_mode ? "stereo" : "normal",
            _(edid_output_src(out->src)),
            dump_bytes ? " -- " : "",
            dump_bytes ? hb : "");
        free(hb);
    }
    return ret;
}

char *edid_manf_date_describe(struct edid_manf_date dom) {
    if (!dom.year) return g_strdup("unspecified");
    if (dom.is_model_year)
        return g_strdup_printf(_("model year %d"), dom.year);
    if (dom.week && dom.week <= 53)
        return g_strdup_printf(_("week %d of %d"), dom.week, dom.year);
    return g_strdup_printf("%d", dom.year);
}

char *edid_dump2(edid *e) {
    char *ret = NULL;
    int i;
    if (!e) return NULL;

    ret = appfnl(ret, "edid version: %d.%d (%d bytes)", e->ver_major, e->ver_minor, e->len);
    if (e->std)
        ret = appfnl(ret, "extended to: %s", _(edid_standard(e->std)) );

    ret = appfnl(ret, "mfg: %s, model: [%04x-%08x] %u-%u", e->ven.pnp, e->product, e->n_serial, e->product, e->n_serial);
    char *dom_desc = edid_manf_date_describe(e->dom);
    ret = appfnl(ret, "date: %s", dom_desc);
    g_free(dom_desc);

    if (e->name)
        ret = appfnl(ret, "product: %s", e->name);
    if (e->serial)
        ret = appfnl(ret, "serial: %s", e->serial);

    ret = appfnl(ret, "type: %s", e->a_or_d ? "digital" : "analog");
    if (e->bpc)
        ret = appfnl(ret, "bits per color channel: %d", e->bpc);
    if (e->interface)
        ret = appfnl(ret, "interface: %s", _(edid_interface(e->interface)));

    char *desc = edid_output_describe(&e->img);
    char *desc_svd = edid_output_describe(&e->img_svd);
    char *desc_max = edid_output_describe(&e->img_max);
    ret = appfnl(ret, "base(%s): %s", _(edid_output_src(e->img.src)), desc);
    if (e->svd_count)
        ret = appfnl(ret, "svd(%s): %s", _(edid_output_src(e->img_svd.src)), desc_svd);
    ret = appfnl(ret, "max(%s): %s", _(edid_output_src(e->img_max.src)), desc_max);
    g_free(desc);
    g_free(desc_svd);
    g_free(desc_max);

    if (e->speaker_alloc_bits) {
        char *desc = edid_cea_speaker_allocation_describe(e->speaker_alloc_bits, 1);
        ret = appfnl(ret, "speakers: %s", desc);
        g_free(desc);
    }

    for(i = 0; i < e->etb_count; i++) {
        char *desc = edid_output_describe(&e->etbs[i]);
        ret = appfnl(ret, "etb[%d]: %s", i, desc);
        g_free(desc);
    }

    for(i = 0; i < e->std_count; i++) {
        char *desc = edid_output_describe(&e->stds[i].out);
        ret = appfnl(ret, "std[%d]: %s", i, desc);
        g_free(desc);
    }

    ret = appfnl(ret, "checksum %s", e->checksum_ok ? "ok" : "failed!");
    if (e->ext_blocks_ok || e->ext_blocks_fail)
        ret = appf(ret, "", ", extension blocks: %d of %d ok", e->ext_blocks_ok, e->ext_blocks_ok + e->ext_blocks_fail);

    for(i = 0; i < 4; i++) {
        char *desc = edid_base_descriptor_describe(&e->d[i]);
        ret = appfnl(ret, "descriptor[%d] %s", i, desc);
        g_free(desc);
    }

    for(i = 0; i < e->ext_blocks; i++) {
        int type = e->u8[(i+1)*128];
        int version = e->u8[(i+1)*128 + 1];
        ret = appfnl(ret, "ext[%d] ([%02x:v%02x] %s) %s", i,
            type, version, _(edid_ext_block_type(type)),
            e->ext_ok[i] ? "ok" : "fail"
        );
    }

    for(i = 0; i < e->dtd_count; i++) {
        char *desc = edid_dtd_describe(&e->dtds[i], 0);
        ret = appfnl(ret, "dtd[%d] %s", i, desc);
        free(desc);
    }

    for(i = 0; i < e->cea_block_count; i++) {
        char *desc = edid_cea_block_describe(&e->cea_blocks[i]);
        ret = appfnl(ret, "cea_block[%d] %s", i, desc);
        free(desc);
    }

    for(i = 0; i < e->svd_count; i++) {
        char *desc = edid_output_describe(&e->svds[i].out);
        ret = appfnl(ret, "svd[%d] [%02x] %s", i, e->svds[i].v, desc);
        free(desc);
    }

    for(i = 0; i < e->sad_count; i++) {
        char *desc = edid_cea_audio_describe(&e->sads[i]);
        ret = appfnl(ret, "sad[%d] [%02x%02x%02x] %s", i,
            e->sads[i].v[0], e->sads[i].v[1], e->sads[i].v[2],
            desc);
        free(desc);
    }

    for(i = 0; i < e->did_block_count; i++) {
        char *desc = edid_did_block_describe(&e->did_blocks[i]);
        ret = appfnl(ret, "did_block[%d] %s", i, desc);
        free(desc);
    }

    for(i = 0; i < e->didt_count; i++) {
        char *desc = edid_output_describe(&e->didts[i]);
        ret = appfnl(ret, "did_timing[%d]: %s", i, desc);
        g_free(desc);
    }

    for(i = 0; i < e->did_string_count; i++) {
        ret = appfnl(ret, "did_string[%d]: %s", i, e->did_strings[i].str);
    }

    ret = appfnl(ret, "parse messages:\n%s---", e->msg_log->str);

    return ret;
}

