/* RailDeck Pro - LVGL configuration (only deviations from defaults). */
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 32

/* Desktop build: use the C library allocator instead of the 64 KB
 * builtin TLSF pool (the full cab UI does not fit in the default pool). */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

/* SDL desktop simulator backend */
#define LV_USE_SDL 1

/* For --screenshot verification */
#define LV_USE_SNAPSHOT 1

/* Fonts used by the cab display */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1

#define LV_FONT_DEFAULT &lv_font_montserrat_14

#endif /* LV_CONF_H */
