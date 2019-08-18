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
    struct edid_cea_video *blk_video = (void*)blk;
    struct edid_cea_speaker *blk_speaker = (void*)blk;
    struct edid_cea_vendor_spec *blk_vendor_spec = (void*)blk;
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
            case 0x2:
            default:
                break;
        }
    }
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
    }

    if (e->u8[21] && e->u8[22]) {
        e->horiz_cm = e->u8[21];
        e->vert_cm = e->u8[22];
        e->diag_cm =
            sqrt( (e->horiz_cm * e->horiz_cm)
             + (e->vert_cm * e->vert_cm) );
        e->diag_in = e->diag_cm / 2.54;
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
                //printf("db_end: %d\n", db_end);
                if (db_end) {
                    int b = 4;
                    while(b < db_end) {
                        int db_type = (u8[b] & 0xe0) >> 5;
                        int db_size = u8[b] & 0x1f;
                        //printf("CEA BLK: %s\n", hex_bytes(&u8[b], db_size+1));
                        e->cea_blocks[e->cea_block_count].header.ptr = &u8[b];
                        e->cea_blocks[e->cea_block_count].header.type = db_type;
                        e->cea_blocks[e->cea_block_count].header.len = db_size;
                        cea_block_decode(&e->cea_blocks[e->cea_block_count]);
                        e->cea_block_count++;
                        b += db_size + 1;
                    }
                    if (b > db_end) b = db_end;
                    //printf("dtd start: %d\n", b);
                    /* DTDs */
                    while(b < 127) {
                        if (u8[b]) {
                            //printf("DTD: %s\n", hex_bytes(&u8[b], 18));
                            e->dtds[e->dtd_count].ptr = &u8[b];
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

    /* dtds */
    for(i = 0; i < e->dtd_count; i++) {
        uint8_t *u8 = e->dtds[i].ptr;
        e->dtds[i].pixel_clock_khz = u8[0] * 10;
        e->dtds[i].horiz_pixels =
            ((u8[4] & 0xf0) << 4) + u8[2];
        e->dtds[i].vert_lines =
            ((u8[7] & 0xf0) << 4) + u8[5];
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
    struct edid_cea_video *blk_video = (void*)blk;
    struct edid_cea_speaker *blk_speaker = (void*)blk;
    struct edid_cea_vendor_spec *blk_vendor_spec = (void*)blk;

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
            case 0x3:
            case 0x2:
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

char *edid_dtd_describe(struct edid_dtd *dtd) {
    gchar *ret = NULL;
    if (dtd) {
        char *hb = hex_bytes(dtd->ptr, 18);
        ret = g_strdup_printf("%dx%d %s",
            dtd->horiz_pixels, dtd->vert_lines,
            hb);
        free(hb);
    }
    return ret;
}

char *edid_dump2(edid *e) {
    char *ret = NULL;
    int i;
    if (!e) return NULL;

    ret = appfnl(ret, "edid_version: %d.%d (%d bytes)", e->ver_major, e->ver_minor, e->len);
    ret = appfnl(ret, "standard: %s", _(edid_standard(e->std)) );

    ret = appfnl(ret, "mfg: %s, model: %u, n_serial: %u", e->ven, e->product, e->n_serial);
    if (e->week && e->year)
        ret = appf(ret, "", ", dom: week %d of %d", e->week, e->year);
    else if (e->year)
        ret = appf(ret, "", ", dom: %d", e->year);

    ret = appfnl(ret, "type: %s", e->a_or_d ? "digital" : "analog");
    if (e->bpc)
        ret = appfnl(ret, "bits per color channel: %d", e->bpc);

    if (e->horiz_cm && e->vert_cm)
        ret = appfnl(ret, "size: %d cm × %d cm", e->horiz_cm, e->vert_cm);
    if (e->diag_cm)
        ret = appfnl(ret, "diagonal: %0.2f cm (%0.2f in)", e->diag_cm, e->diag_in);

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
        char *desc = edid_dtd_describe(&e->dtds[i]);
        ret = appfnl(ret, "dtd[%d] %s", i, desc);
        free(desc);
    }
    for(i = 0; i < e->cea_block_count; i++) {
        char *desc = edid_cea_block_describe(&e->cea_blocks[i]);
        ret = appfnl(ret, "cea_block[%d] %s", i, desc);
    }

    return ret;
}

