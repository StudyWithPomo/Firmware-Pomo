/*******************************************************************************
 * Size: 14 px
 * Bpp: 4
 * Opts: --bpp 4 --size 14 --no-compress --stride 1 --align 1 --font pro-fa-solid-900-5.10.1.woff --range 63120-63124 --format lvgl -o ext_symbols_14.c
 ******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif



#ifndef EXT_SYMBOLS_14
#define EXT_SYMBOLS_14 1
#endif

#if EXT_SYMBOLS_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+F690 "" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xd,
    0xf9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xff, 0xa0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xf, 0xfa, 0x0, 0x0, 0x0, 0x0, 0x2, 0xa8,
    0x0, 0xff, 0xa0, 0x0, 0x0, 0x0, 0x0, 0x8f,
    0xf1, 0xf, 0xfa, 0x0, 0x0, 0x0, 0x0, 0x8,
    0xff, 0x10, 0xff, 0xa0, 0x0, 0x0, 0x2, 0x0,
    0x8f, 0xf1, 0xf, 0xfa, 0x0, 0x0, 0x9, 0xfd,
    0x8, 0xff, 0x10, 0xff, 0xa0, 0x0, 0x0, 0xbf,
    0xe0, 0x8f, 0xf1, 0xf, 0xfa, 0x0, 0x0, 0xb,
    0xfe, 0x8, 0xff, 0x10, 0xff, 0xa0, 0x8a, 0x20,
    0xbf, 0xe0, 0x8f, 0xf1, 0xf, 0xfa, 0x1f, 0xf8,
    0xb, 0xfe, 0x8, 0xff, 0x10, 0xff, 0xa1, 0xff,
    0x80, 0xbf, 0xe0, 0x8f, 0xf1, 0xf, 0xfa, 0xb,
    0xd4, 0x6, 0xd9, 0x4, 0xdb, 0x0, 0x9d, 0x60,

    /* U+F691 "" */
    0x8, 0xa2, 0x1f, 0xf8, 0x1f, 0xf8, 0xb, 0xd4,

    /* U+F692 "" */
    0x0, 0x0, 0x0, 0x20, 0x0, 0x0, 0x9, 0xfd,
    0x0, 0x0, 0xb, 0xfe, 0x0, 0x0, 0xb, 0xfe,
    0x8, 0xa2, 0xb, 0xfe, 0x1f, 0xf8, 0xb, 0xfe,
    0x1f, 0xf8, 0xb, 0xfe, 0xb, 0xd4, 0x6, 0xd9,

    /* U+F693 "" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x99, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x5f, 0xf4, 0x0, 0x0, 0x0,
    0x0, 0x5, 0xff, 0x40, 0x0, 0x0, 0x2, 0x0,
    0x5f, 0xf4, 0x0, 0x0, 0x9, 0xfd, 0x5, 0xff,
    0x40, 0x0, 0x0, 0xbf, 0xe0, 0x5f, 0xf4, 0x0,
    0x0, 0xb, 0xfe, 0x5, 0xff, 0x40, 0x8a, 0x20,
    0xbf, 0xe0, 0x5f, 0xf4, 0x1f, 0xf8, 0xb, 0xfe,
    0x5, 0xff, 0x41, 0xff, 0x80, 0xbf, 0xe0, 0x5f,
    0xf4, 0xb, 0xd4, 0x6, 0xd9, 0x1, 0xcc, 0x10
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 280, .box_w = 17, .box_h = 15, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 128, .adv_w = 280, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 136, .adv_w = 280, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 168, .adv_w = 280, .box_w = 13, .box_h = 11, .ofs_x = 0, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 63120, .range_length = 4, .glyph_id_start = 1,
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
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif

};

extern const lv_font_t lv_font_montserrat_14;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ext_symbols_14 = {
#else
lv_font_t ext_symbols_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 15,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .static_bitmap = 0,
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &lv_font_montserrat_14,
#endif
    .user_data = NULL,
};



#endif /*#if EXT_SYMBOLS_14*/
