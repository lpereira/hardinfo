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

typedef struct {
    float horiz_cm, vert_cm;
    float diag_cm, diag_in;
    int horiz_blanking, vert_blanking;
    int horiz_pixels, vert_lines, vert_pixels;
    float vert_freq_hz;
    int is_interlaced;
    int stereo_mode;
    int pixel_clock_khz;
    int src; /* 0: edid, 1: etb, 2: std, 3: dtd, 4: cea-dtd, ... */
    uint64_t pixels; /* h*v: easier to compare */
    char class_inch[6];
} edid_output;

struct edid_std {
    uint8_t *ptr;
    edid_output out;
};

struct edid_dtd {
    uint8_t *ptr;
    int cea_ext;
    edid_output out;
};

struct edid_cea_header {
    uint8_t *ptr;
    int type, len;
};

struct edid_cea_block {
    struct edid_cea_header header;
    int reserved[8];
};

struct edid_cea_audio {
    struct edid_cea_header header;
    int format, channels, freq_bits;
    int depth_bits; /* format 1 */
    int max_kbps;   /* formats 2-8 */
};

struct edid_cea_video {
    struct edid_cea_header header;
};

struct edid_cea_vendor_spec {
    struct edid_cea_header header;
};

struct edid_cea_speaker {
    struct edid_cea_header header;
    int alloc_bits;
};

typedef struct {
    union {
        void* data;
        uint8_t* u8;
        uint16_t* u16;
        uint32_t* u32;
    };
    unsigned int len;

    /* 0 - EDID
     * 1 - EIA/CEA-861
     * 2 - DisplayID
     */
    int std;

    int ver_major, ver_minor;
    int checksum_ok; /* first 128-byte block only */
    int ext_blocks, ext_blocks_ok, ext_blocks_fail;
    uint8_t *ext_ok;

    int etb_count;
    edid_output etbs[24];

    int std_count;
    struct edid_std stds[8];

    int dtd_count;
    struct edid_dtd *dtds;

    int cea_block_count;
    struct edid_cea_block *cea_blocks;

    char ven[4];
    int d_type[4];
    char d_text[4][14];
    /* point into d_text */
    char *name;
    char *serial;
    char *ut1;
    char *ut2;

    int a_or_d; /* 0 = analog, 1 = digital */
    int interface; /* digital interface */
    int bpc;       /* digital bpc */
    uint16_t product;
    uint32_t n_serial;
    int week, year;
    edid_output img;
    edid_output img_max;
} edid;
edid *edid_new(const char *data, unsigned int len);
edid *edid_new_from_hex(const char *hex_string);
void edid_free(edid *e);
char *edid_dump_hex(edid *e, int tabs, int breaks);

const char *edid_standard(int type);
const char *edid_output_src(int src);
const char *edid_interface(int type);
const char *edid_descriptor_type(int type);
const char *edid_ext_block_type(int type);
const char *edid_cea_block_type(int type);
const char *edid_cea_audio_type(int type);

char *edid_output_describe(edid_output *out);
char *edid_dtd_describe(struct edid_dtd *dtd, int dump_bytes);
char *edid_cea_block_describe(struct edid_cea_block *blk);

char *edid_dump2(edid *e);

#endif
