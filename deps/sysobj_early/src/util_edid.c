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

/* block must be 128 bytes */
static int block_check(const void *edid_block_bytes) {
    if (!edid_block_bytes) return 0;
    uint8_t sum = 0;
    uint8_t *data = (uint8_t*)edid_block_bytes;
    int i;
    for(i=0; i<128; i++)
        sum += data[i];
    return sum == 0 ? 1 : 0;
}

char *hex_bytes(uint8_t *bytes, int count) {
    char *buffer = malloc(count*3+1), *p = buffer;
    memset(buffer, 0, count*3+1);
    int i;
    for(i = 0; i < count; i++) {
        sprintf(p, "%02x ", (unsigned int)bytes[i]);
        p += 3;
    }
    return buffer;
}

static void cea_block_decode(struct edid_cea_block *blk) {
    struct edid_cea_audio *blk_audio = (void*)blk;
    struct edid_cea_speaker *blk_speaker = (void*)blk;
    //struct edid_cea_vendor_spec *blk_vendor_spec = (void*)blk;
    if (blk) {
        switch(blk->header.type) {
            case 0x1:
                blk_audio->format = (blk->header.ptr[1] & 0x78) >> 3;
                blk_audio->channels = 1 + blk->header.ptr[1] & 0x7;
                blk_audio->freq_bits = blk->header.ptr[2];
                if (blk_audio->format == 1) {
                    blk_audio->depth_bits = blk->header.ptr[3];
                } else if (blk_audio->format >= 2
                        && blk_audio->format <= 8) {
                    blk_audio->max_kbps = blk->header.ptr[3] * 8;
                }
                break;
            case 0x4:
                blk_speaker->alloc_bits = blk->header.ptr[1];
                break;
            case 0x3:
            case 0x2: /* SVD list is handled elsewhere */
            default:
                break;
        }
    }
}

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
        static const char *inlbl = "\""; /* TODO: unicode */
        sprintf(out->class_inch, "%0.1f%s", out->diag_in, inlbl);
        util_strchomp_float(out->class_inch);
    }
}

edid_output edid_output_from_svd(uint8_t index) {
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
            out.src = 5;
            edid_output_fill(&out);
            return out;
        }
    }
    return (edid_output){};
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

    e->dtds = malloc(sizeof(struct edid_dtd) * 1000);
    e->cea_blocks = malloc(sizeof(struct edid_cea_block) * 1000);
    e->svds = malloc(sizeof(struct edid_svd) * 1000);
    memset(e->dtds, 0, sizeof(struct edid_dtd) * 1000);
    memset(e->cea_blocks, 0, sizeof(struct edid_cea_block) * 1000);
    memset(e->svds, 0, sizeof(struct edid_svd) * 1000);

    uint16_t vid = be16toh(e->u16[4]); /* bytes 8-9 */
    e->ven[2] = 64 + (vid & 0x1f);
    e->ven[1] = 64 + ((vid >> 5) & 0x1f);
    e->ven[0] = 64 + ((vid >> 10) & 0x1f);
    e->product = le16toh(e->u16[5]); /* bytes 10-11 */
    e->n_serial = le32toh(e->u32[3]);/* bytes 12-15 */
    e->week = e->u8[16];             /* byte 16 */
    e->year = e->u8[17] + 1990;      /* byte 17 */

    if (e->week >= 52)
        e->week = 0;

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
        e->etbs[e->etb_count].src = 1;           \
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
        e->stds[e->std_count].out.src = 2;
        edid_output_fill(&e->stds[e->std_count].out);
        e->std_count++;
    }

    uint16_t dh, dl;
#define CHECK_DESCRIPTOR(index, offset)       \
    if (e->u8[offset] == 0) {                 \
        dh = be16toh(e->u16[offset/2]);       \
        dl = be16toh(e->u16[offset/2+1]);     \
        e->d_type[index] = (dh << 16) + dl;   \
        switch(e->d_type[index]) {            \
            case 0xfc: case 0xff: case 0xfe:  \
                strncpy(e->d_text[index], (char*)e->u8+offset+5, 13);  \
        } \
    } else e->dtds[e->dtd_count++].ptr = &e->u8[offset];

    CHECK_DESCRIPTOR(0, 54);
    CHECK_DESCRIPTOR(1, 72);
    CHECK_DESCRIPTOR(2, 90);
    CHECK_DESCRIPTOR(3, 108);

    e->checksum_ok = block_check(e->data); /* first 128-byte block only */
    if (len > 128) {
        /* check extension blocks */
        int blocks = len / 128;
        blocks--;
        e->ext_blocks = blocks;
        e->ext_ok = malloc(sizeof(uint8_t) * blocks);
        for(; blocks; blocks--) {
            uint8_t *u8 = e->u8 + (blocks * 128);
            int r = block_check(u8);
            e->ext_ok[blocks-1] = r;
            if (r) e->ext_blocks_ok++;
            else e->ext_blocks_fail++;

            if (u8[0] == 0x70) {
                e->std = MAX(e->std, 2);
                /* DisplayID */
                // TODO: ...
            }

            if (u8[0] == 0x02) {
                e->std = MAX(e->std, 1);
                /* CEA extension */
                int db_end = u8[2];
                if (db_end) {
                    int b = 4;
                    while(b < db_end) {
                        int db_type = (u8[b] & 0xe0) >> 5;
                        int db_size = u8[b] & 0x1f;
                        e->cea_blocks[e->cea_block_count].header.ptr = &u8[b];
                        e->cea_blocks[e->cea_block_count].header.type = db_type;
                        e->cea_blocks[e->cea_block_count].header.len = db_size;
                        if (db_type == 0x2) {
                            for(i = 1; i <= db_size; i++)
                                e->svds[e->svd_count++].v = u8[b+i];
                        } else
                            cea_block_decode(&e->cea_blocks[e->cea_block_count]);
                        e->cea_block_count++;
                        b += db_size + 1;
                    }
                    if (b > db_end) b = db_end;
                    /* DTDs */
                    while(b < 127) {
                        if (u8[b]) {
                            e->dtds[e->dtd_count].ptr = &u8[b];
                            e->dtds[e->dtd_count].cea_ext = 1;
                            e->dtd_count++;
                        }
                        b += 18;
                    }
                }
            }
        }
    }

    /* strings */
    for(i = 0; i < 4; i++) {
        g_strstrip(e->d_text[i]);
        switch(e->d_type[i]) {
            case 0xfc:
                e->name = e->d_text[i];
                break;
            case 0xff:
                e->serial = e->d_text[i];
                break;
            case 0xfe:
                if (e->ut1)
                    e->ut2 = e->d_text[i];
                else
                    e->ut1 = e->d_text[i];
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
        uint8_t *u8 = e->dtds[i].ptr;
        uint16_t *u16 = (uint16_t*)e->dtds[i].ptr;
        edid_output *out = &e->dtds[i].out;
        if (e->dtds[i].cea_ext) out->src = 4;
        else out->src = 3;
        out->pixel_clock_khz = le16toh(u16[0]) * 10;
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
        if (e->svds[i].v >= 128 &&
            e->svds[i].v <= 192) {
            e->svds[i].is_native = 1;
        }
        e->svds[i].out = edid_output_from_svd(e->svds[i].v);
    }

    /* squeeze lists */
    if (!e->dtd_count) {
        free(e->dtds);
        e->dtds = NULL;
    } else {
        e->dtds = realloc(e->dtds, sizeof(struct edid_dtd) * e->dtd_count);
    }
    if (!e->cea_block_count) {
        if (e->cea_blocks)
            free(e->cea_blocks);
        e->cea_blocks = NULL;
    } else {
        e->cea_blocks = realloc(e->cea_blocks, sizeof(struct edid_cea_block) * e->cea_block_count);
    }
    if (!e->svd_count) {
        if (e->svds)
            free(e->svds);
        e->svds = NULL;
    } else {
        e->svds = realloc(e->svds, sizeof(struct edid_svd) * e->svd_count);
    }

    return e;
}

void edid_free(edid *e) {
    if (e) {
        g_free(e->ext_ok);
        g_free(e->cea_blocks);
        g_free(e->dtds);
        g_free(e->data);
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

char *edid_dump_hex(edid *e, int tabs, int breaks) {
    if (!e) return NULL;
    int lines = e->len / 16;
    int blen = lines * 35 + 1;
    unsigned int pc = 0;
    char *ret = malloc(blen);
    memset(ret, 0, blen);
    uint8_t *u8 = e->u8;
    char *p = ret;
    for(; lines; lines--) {
        int i;
        for(i = 0; i < tabs; i++)
            sprintf(p++, "\t");
        for(i = 0; i < 16; i++) {
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
        case 0: return N_("VESA EDID");
        case 1: return N_("EIA/CEA-861");
        case 2: return N_("VESA DisplayID");
    };
    return N_("unknown");
}

const char *edid_output_src(int src) {
    switch(src) {
        case 0: return N_("VESA EDID");
        case 1: return N_("VESA EDID ETB");
        case 2: return N_("VESA EDID STD");
        case 3: return N_("VESA EDID DTD");
        case 4: return N_("EIA/CEA-861 DTD");
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

char *edid_cea_block_describe(struct edid_cea_block *blk) {
    gchar *ret = NULL;
    gchar *tmp[3] = {};
    struct edid_cea_audio *blk_audio = (void*)blk;
    struct edid_cea_speaker *blk_speaker = (void*)blk;
    //struct edid_cea_vendor_spec *blk_vendor_spec = (void*)blk;

    if (blk) {
        char *hb = hex_bytes(blk->header.ptr, blk->header.len+1);
        switch(blk->header.type) {
            case 0x1:

#define appfreq(b, f) if (blk_audio->freq_bits & (1 << b)) tmp[0] = appf(tmp[0], ", ", "%d", f);
#define appdepth(b, d) if (blk_audio->depth_bits & (1 << b)) tmp[1] = appf(tmp[1], ", ", "%d%s", d, _("-bit"));
                appfreq(0, 32);
                appfreq(1, 44);
                appfreq(2, 48);
                appfreq(3, 88);
                appfreq(4, 96);
                appfreq(5, 176);
                appfreq(6, 192);

                if (blk_audio->format == 1) {
                    appdepth(0, 16);
                    appdepth(1, 20);
                    appdepth(2, 24);
                    tmp[2] = g_strdup_printf("depths: %s", tmp[1]);
                } else if (blk_audio->format >= 2
                            && blk_audio->format <= 8 ) {
                    tmp[2] = g_strdup_printf("max_bitrate: %d %s", blk_audio->max_kbps, _("kbps"));
                } else
                    tmp[2] = g_strdup("");

                ret = g_strdup_printf("([%x] %s) len:%d format:([%x] %s) channels:%d rates:%s %s %s -- %s",
                    blk->header.type, _(edid_cea_block_type(blk->header.type)),
                    blk->header.len,
                    blk_audio->format, _(edid_cea_audio_type(blk_audio->format)),
                    blk_audio->channels, tmp[0], _("kHz"),
                    tmp[2],
                    hb);
                g_free(tmp[0]);
                g_free(tmp[1]);
                g_free(tmp[2]);
                break;
            case 0x4:
#define appspk(b, s) if (blk_speaker->alloc_bits & (1 << b)) tmp[0] = appf(tmp[0], ", ", "%s", s);
                appspk(0, _("LF+LR: Front left and right"));
                appspk(1, _("LFE: Low-frequency effects"));
                appspk(2, _("FC: Front center"));
                appspk(3, _("LR+LR: Rear left and right"));
                appspk(4, _("RC: Rear center"));
                appspk(5, _("???"));
                appspk(6, _("???"));
                ret = g_strdup_printf("([%x] %s) len:%d %s -- %s",
                    blk->header.type, _(edid_cea_block_type(blk->header.type)),
                    blk->header.len,
                    tmp[0],
                    hb);
                g_free(tmp[0]);
                break;
            case 0x2:
                ret = g_strdup_printf("([%x] %s) svds:%d",
                    blk->header.type, _(edid_cea_block_type(blk->header.type)),
                    blk->header.len);
                break;
            case 0x3: //TODO
            default:
                ret = g_strdup_printf("([%x] %s) len:%d -- %s",
                    blk->header.type, _(edid_cea_block_type(blk->header.type)),
                    blk->header.len,
                    hb);
                break;
        }
        free(hb);
    }
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
        char *hb = hex_bytes(dtd->ptr, 18);
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

char *edid_dump2(edid *e) {
    char *ret = NULL;
    int i;
    if (!e) return NULL;

    ret = appfnl(ret, "edid version: %d.%d (%d bytes)", e->ver_major, e->ver_minor, e->len);
    ret = appfnl(ret, "extended to: %s", _(edid_standard(e->std)) );

    ret = appfnl(ret, "mfg: %s, model: [%04x-%08x] %u-%u", e->ven, e->product, e->n_serial, e->product, e->n_serial);
    if (e->week && e->year)
        ret = appf(ret, "", ", dom: week %d of %d", e->week, e->year);
    else if (e->year)
        ret = appf(ret, "", ", dom: %d", e->year);

    ret = appfnl(ret, "type: %s", e->a_or_d ? "digital" : "analog");
    if (e->bpc)
        ret = appfnl(ret, "bits per color channel: %d", e->bpc);
    if (e->interface)
        ret = appfnl(ret, "interface: %s", _(edid_interface(e->interface)));

    char *desc = edid_output_describe(&e->img);
    char *desc_max = edid_output_describe(&e->img_max);
    ret = appfnl(ret, "base(%s): %s", _(edid_output_src(e->img.src)), desc);
    ret = appfnl(ret, "ext(%s): %s", _(edid_output_src(e->img_max.src)), desc_max);
    g_free(desc);
    g_free(desc_max);

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

    for(i = 0; i < 4; i++)
        ret = appfnl(ret, "descriptor[%d] ([%02x] %s): %s", i, e->d_type[i], _(edid_descriptor_type(e->d_type[i])), *e->d_text[i] ? e->d_text[i] : "{...}");

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
    }

    for(i = 0; i < e->svd_count; i++) {
        char *desc = edid_output_describe(&e->svds[i].out);
        ret = appfnl(ret, "svd[%d] [%02x] %s", i, e->svds[i].v, desc);
        free(desc);
    }

    return ret;
}

