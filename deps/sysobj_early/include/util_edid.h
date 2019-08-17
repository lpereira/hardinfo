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

#ifndef __UTIL_EDID_H__
#define __UTIL_EDID_H__

#include <stdint.h>  /* for *int*_t types */

#define EDID_MAX_EXT_BLOCKS 254

/* just enough edid decoding */
struct edid {
    int ver_major, ver_minor;
    char ven[4];

    int d_type[4];
    char d_text[4][14];

    /* point into d_text */
    char *name;
    char *serial;
    char *ut1;
    char *ut2;

    int a_or_d; /* 0 = analog, 1 = digital */
    int bpc;

    uint16_t product;
    uint32_t n_serial;
    int week, year;

    int horiz_cm, vert_cm;
    float diag_cm, diag_in;

    int size; /* bytes */
    int checksum_ok; /* block 0 */

    int ext_blocks, ext_blocks_ok, ext_blocks_fail;
    uint8_t ext[EDID_MAX_EXT_BLOCKS][2]; /* block type, checksum_ok */
};

int edid_fill(struct edid *id_out, const void *edid_bytes, int edid_len);
int edid_fill_xrandr(struct edid *id_out, const char *xrandr_edid_dump);

int edid_hex_to_bin(void **edid_bytes, int *edid_len, const char *hex_string);
char *edid_bin_to_hex(const void *edid_bytes, int edid_len);

const char *edid_descriptor_type(int type);
char *edid_dump(struct edid *id);

#endif
