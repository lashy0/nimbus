/*******************************************************************************
 * Size: 10 px
 * Bpp: 4
 * Opts: --bpp 4 --no-compress --format lvgl --lv-include lvgl.h --size 10 --font artwork/fonts/SF-Pro-Rounded-Bold.otf --range 0x20,0x25,0x2D,0x30-0x39 -o firmware/components/ui/view/font/ui_font_bold_10.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef UI_FONT_BOLD_10
#define UI_FONT_BOLD_10 1
#endif

#if UI_FONT_BOLD_10

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0025 "%" */
    0x3d, 0xc0, 0x1, 0xc0, 0x9, 0x6a, 0x50, 0xb6,
    0x0, 0x86, 0xa4, 0x7a, 0x0, 0x2, 0xba, 0x3d,
    0x2b, 0xb1, 0x0, 0xd, 0x47, 0x89, 0x60, 0x8,
    0x90, 0x77, 0x87, 0x1, 0xc0, 0x2, 0xdd, 0x10,

    /* U+002D "-" */
    0x5f, 0xfc, 0x2, 0x21,

    /* U+0030 "0" */
    0x5, 0xdf, 0xa1, 0x2, 0xf8, 0x3d, 0xa0, 0x7f,
    0x0, 0x7f, 0x8, 0xe0, 0x5, 0xf1, 0x7f, 0x0,
    0x7f, 0x2, 0xf8, 0x2d, 0xa0, 0x5, 0xdf, 0xa1,
    0x0,

    /* U+0031 "1" */
    0x6, 0xe8, 0x8d, 0xdb, 0x10, 0xbb, 0x0, 0xbb,
    0x0, 0xbb, 0x0, 0xbb, 0x0, 0x99,

    /* U+0032 "2" */
    0x8, 0xde, 0x90, 0x5f, 0x44, 0xf6, 0x12, 0x1,
    0xf5, 0x0, 0xc, 0xd0, 0x1, 0xcc, 0x10, 0x1d,
    0xd2, 0x10, 0x5f, 0xff, 0xf8,

    /* U+0033 "3" */
    0x7, 0xde, 0x91, 0x4f, 0x43, 0xf8, 0x2, 0x2,
    0xf5, 0x0, 0xbf, 0xa0, 0x0, 0x1, 0xda, 0x6e,
    0x43, 0xeb, 0x9, 0xdf, 0xb2,

    /* U+0034 "4" */
    0x0, 0x1c, 0xd0, 0x0, 0xa, 0xcf, 0x40, 0x4,
    0xf3, 0xf4, 0x0, 0xd7, 0x1f, 0x40, 0x6e, 0x1,
    0xf4, 0x8, 0xff, 0xff, 0xf2, 0x2, 0x23, 0xf4,
    0x0,

    /* U+0035 "5" */
    0xd, 0xff, 0xf6, 0x2f, 0x31, 0x10, 0x3f, 0xae,
    0xc2, 0x1c, 0x52, 0xdb, 0x13, 0x0, 0x9d, 0x3f,
    0x63, 0xea, 0x6, 0xdf, 0xa1,

    /* U+0036 "6" */
    0x4, 0xdf, 0xc3, 0x1, 0xf7, 0x28, 0x80, 0x7f,
    0x7d, 0xb2, 0x8, 0xf9, 0x3c, 0xd0, 0x6f, 0x10,
    0x7f, 0x2, 0xf9, 0x3d, 0xc0, 0x4, 0xcf, 0xb2,
    0x0,

    /* U+0037 "7" */
    0x9f, 0xff, 0xf3, 0x1, 0x16, 0xf2, 0x0, 0xd,
    0xa0, 0x0, 0x6f, 0x20, 0x0, 0xe9, 0x0, 0x7,
    0xf1, 0x0, 0xb, 0x80, 0x0,

    /* U+0038 "8" */
    0x7, 0xef, 0xc2, 0x3, 0xf6, 0x1d, 0xc0, 0x1f,
    0x61, 0xd9, 0x0, 0x6f, 0xfd, 0x10, 0x5f, 0x41,
    0xbd, 0x7, 0xf4, 0x1b, 0xf0, 0x9, 0xef, 0xc4,
    0x0,

    /* U+0039 "9" */
    0x8, 0xee, 0x90, 0x6, 0xf5, 0x3e, 0x80, 0x8f,
    0x20, 0xcd, 0x1, 0xcf, 0xea, 0xf0, 0x2, 0x0,
    0x8e, 0x4, 0xf5, 0x3e, 0x80, 0x7, 0xde, 0x90,
    0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 32, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 141, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 71, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 36, .adv_w = 105, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 61, .adv_w = 77, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 96, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 101, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 104, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 100, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 103, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 92, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 104, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 103, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x5, 0xd
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 14, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 3, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 48, .range_length = 10, .glyph_id_start = 4,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_bold_10 = {
#else
lv_font_t ui_font_bold_10 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 7,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_BOLD_10*/

