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
#include "util_edid.h"
#include "util_sysobj.h"
#include "gettext.h"

/* block must be 128 bytes */
int block_check(const void *edid_block_bytes) {
    if (!edid_block_bytes) return 0;
    uint8_t sum = 0;
    uint8_t *data = (uint8_t*)edid_block_bytes;
    int i;
    for(i=0; i<128; i++)
        sum += data[i];
    return sum == 0 ? 1 : 0;
}

int edid_fill(struct edid *id_out, const void *edid_bytes, int edid_len) {
    int i;

    if (!id_out) return 0;

    memset(id_out, 0, sizeof(struct edid));
    id_out->size = edid_len;

    if (edid_len >= 128) {
        uint8_t *u8 = (uint8_t*)edid_bytes;
        uint16_t *u16 = (uint16_t*)edid_bytes;
        uint32_t *u32 = (uint32_t*)edid_bytes;

        id_out->ver_major = u8[18];        /* byte 18 */
        id_out->ver_minor = u8[19];        /* byte 19 */

        /* checksum first block */
        id_out->checksum_ok = block_check(edid_bytes);
        if (edid_len > 128) {
            int blocks = edid_len / 128;
            if (blocks > EDID_MAX_EXT_BLOCKS) blocks = EDID_MAX_EXT_BLOCKS;
            blocks--;
            id_out->ext_blocks = blocks;
            for(; blocks; blocks--) {
                id_out->ext[blocks-1][0] = *(u8 + (blocks * 128));
                int r = block_check(u8 + (blocks * 128));
                id_out->ext[blocks-1][1] = (uint8_t)r;
                if (r) id_out->ext_blocks_ok++;
                else id_out->ext_blocks_fail++;
            }
        }

        /* expect 1.3 */

        uint16_t vid = be16toh(u16[4]); /* bytes 8-9 */
        id_out->ven[2] = 64 + (vid & 0x1f);
        id_out->ven[1] = 64 + ((vid >> 5) & 0x1f);
        id_out->ven[0] = 64 + ((vid >> 10) & 0x1f);

        id_out->product = le16toh(u16[5]); /* bytes 10-11 */
        id_out->n_serial = le32toh(u32[3]);/* bytes 12-15 */
        id_out->week = u8[16];             /* byte 16 */
        id_out->year = u8[17] + 1990;      /* byte 17 */

        if (id_out->week >= 52)
            id_out->week = 0;

        id_out->a_or_d = (u8[20] & 0x80) ? 1 : 0;
        if (id_out->a_or_d == 1) {
            /* digital */
            switch((u8[20] >> 4) & 0x7) {
                case 0x1: id_out->bpc = 6; break;
                case 0x2: id_out->bpc = 8; break;
                case 0x3: id_out->bpc = 10; break;
                case 0x4: id_out->bpc = 12; break;
                case 0x5: id_out->bpc = 14; break;
                case 0x6: id_out->bpc = 16; break;
            }
        }

        if (u8[21] && u8[22]) {
            id_out->horiz_cm = u8[21];
            id_out->vert_cm = u8[22];
            id_out->diag_cm =
                sqrt( (id_out->horiz_cm * id_out->horiz_cm)
                 + (id_out->vert_cm * id_out->vert_cm) );
            id_out->diag_in = id_out->diag_cm / 2.54;
        }

        uint16_t dh, dl;

        /* descriptor at byte 54 */
        dh = be16toh(u16[54/2]);
        dl = be16toh(u16[54/2+1]);
        id_out->d_type[0] = (dh << 16) + dl;
        switch(id_out->d_type[0]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[0], (char*)u8+54+5, 13);
        }

        /* descriptor at byte 72 */
        dh = be16toh(u16[72/2]);
        dl = be16toh(u16[72/2+1]);
        id_out->d_type[1] = (dh << 16) + dl;
        switch(id_out->d_type[1]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[1], (char*)u8+72+5, 13);
        }

        /* descriptor at byte 90 */
        dh = be16toh(u16[90/2]);
        dl = be16toh(u16[90/2+1]);
        id_out->d_type[2] = (dh << 16) + dl;
        switch(id_out->d_type[2]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[2], (char*)u8+90+5, 13);
        }

        /* descriptor at byte 108 */
        dh = be16toh(u16[108/2]);
        dl = be16toh(u16[108/2+1]);
        id_out->d_type[3] = (dh << 16) + dl;
        switch(id_out->d_type[3]) {
            case 0xfc: case 0xff: case 0xfe:
                strncpy(id_out->d_text[3], (char*)u8+108+5, 13);
        }

        for(i = 0; i < 4; i++) {
            g_strstrip(id_out->d_text[i]);
            switch(id_out->d_type[i]) {
                case 0xfc:
                    id_out->name = id_out->d_text[i];
                    break;
                case 0xff:
                    id_out->serial = id_out->d_text[i];
                    break;
                case 0xfe:
                    if (id_out->ut1)
                        id_out->ut2 = id_out->d_text[i];
                    else
                        id_out->ut1 = id_out->d_text[i];
                    break;
            }
        }

        if (!id_out->name) {
            if (SEQ(id_out->ut1, "LG Display") && id_out->ut2)
                /* LG may use "uspecified text" for name and model */
                id_out->name = id_out->ut2;
            else if (SEQ(id_out->ut1, "AUO") && id_out->ut2)
                /* Same with AUO */
                id_out->name = id_out->ut2;
            else {
                if (id_out->ut1) id_out->name = id_out->ut1;
                if (id_out->ut2 && !id_out->serial) id_out->serial = id_out->ut2;
            }
        }
    }
    return 1;
}

int edid_hex_to_bin(void **edid_bytes, int *edid_len, const char *hex_string) {
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

    *edid_bytes = (void *)buffer;
    *edid_len = len;
    return 1;
}

int edid_fill_xrandr(struct edid *id_out, const char *xrandr_edid_dump) {
    void *buffer = NULL;
    int len = 0;
    edid_hex_to_bin(&buffer, &len, xrandr_edid_dump);
    return edid_fill(id_out, buffer, len);
}

const char *edid_ext_block_type(int type) {
    switch(type) {
        case 0xf0:
            return N_("extension block map");
        case 0x02:
            return N_("EIA/CEA-861 extension block");
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
    if (type < 0x0f) return N_("manufacturer reserved descriptor");
    return N_("detailed timing descriptor");
}

char *edid_dump(struct edid *id) {
    char *ret = NULL;
    int i;

    if (!id) return NULL;
    ret = appfnl(ret, "edid_version: %d.%d (%d bytes)", id->ver_major, id->ver_minor, id->size);

    ret = appfnl(ret, "mfg: %s, model: %u, n_serial: %u", id->ven, id->product, id->n_serial);
    if (id->week && id->year)
        ret = appf(ret, "", ", dom: week %d of %d", id->week, id->year);
    else if (id->year)
        ret = appf(ret, "", ", dom: %d", id->year);

    ret = appfnl(ret, "type: %s", id->a_or_d ? "digital" : "analog");
    if (id->bpc)
        ret = appfnl(ret, "bits per color channel: %d", id->bpc);

    if (id->horiz_cm && id->vert_cm)
        ret = appfnl(ret, "size: %d cm × %d cm", id->horiz_cm, id->vert_cm);
    if (id->diag_cm)
        ret = appfnl(ret, "diagonal: %0.2f cm (%0.2f in)", id->diag_cm, id->diag_in);

    ret = appfnl(ret, "checksum %s", id->checksum_ok ? "ok" : "failed!");
    if (id->ext_blocks_ok || id->ext_blocks_fail)
        ret = appf(ret, "", ", extension blocks: %d of %d ok", id->ext_blocks_ok, id->ext_blocks_ok + id->ext_blocks_fail);

    for(i = 0; i < 4; i++)
        ret = appfnl(ret, "descriptor[%d] (%s): %s", i, _(edid_descriptor_type(id->d_type[i])), *id->d_text[i] ? id->d_text[i] : "{...}");

    for(i = 0; i < EDID_MAX_EXT_BLOCKS; i++) {
        if (id->ext[i][0]) {
            ret = appfnl(ret, "ext_block[%d]: (%s): %s", i, _(edid_ext_block_type(id->ext[i][0])), id->ext[i][1] ? "ok" : "fail");
        }
    }

    return ret;
}

char *edid_bin_to_hex(const void *edid_bytes, int edid_len) {
    int lines = edid_len / 16;
    int blen = lines * 35 + 1;
    int pc = 0;
    char *ret = malloc(blen);
    memset(ret, 0, blen);
    uint8_t *u8 = (uint8_t*)edid_bytes;
    char *p = ret;
    for(; lines; lines--) {
        sprintf(p, "\t\t");
        p+=2;
        int i;
        for(i = 0; i < 16; i++) {
            sprintf(p, "%02x", (unsigned int)*u8);
            p+=2;
            u8++;
            pc++;
            if (pc == blen-1) {
                sprintf(p, "\n");
                goto edid_print_done;
            }
        }
        sprintf(p, "\n");
        p++;
    }
edid_print_done:
    return ret;
}
