# Fonts

Generated with [lv_font_conv](https://github.com/lvgl/lv_font_conv) 1.5.3, 4 bpp, uncompressed. Sizes are half the design canvas's, because the canvas is drawn at 2×.

| Font | Source | Sizes | Glyphs |
|---|---|---|---|
| `font_fell_*` | IM Fell English SC (`IMFeENsc28P.ttf`) | 12, 14, 16, 25 | ASCII + `·×–’`, plus `▸` from DejaVu Sans |
| `font_caslon_12` | Libre Caslon Text (`LibreCaslonText[wght].ttf`, default weight) | 12 | ASCII + `·×–’`, plus `▸` from DejaVu Sans |
| `font_plex_12` | IBM Plex Mono Regular | 12 | ASCII + `·×–’` |
| `font_plex_22/28/50` | IBM Plex Mono Light | 22, 28, 50 | `0-9`, `:`, space |

The TTFs come from [google/fonts](https://github.com/google/fonts) (`ofl/imfellenglishsc`, `ofl/librecaslontext`, `ofl/ibmplexmono`). DejaVu Sans ships with LVGL in `managed_components/lvgl__lvgl/scripts/built_in_font/`. Licences are in `licenses/`.

## Regenerate

Each `.c` file's header records its exact options. For example:

```bash
npx lv_font_conv@1.5.3 --font IMFeENsc28P.ttf -r 0x20-0x7E --symbols "·×–’" \
  --font DejaVuSans.ttf -r 0x25B8 --size 14 --bpp 4 --format lvgl --no-compress \
  --lv-include lvgl.h --lv-font-name font_fell_14 -o font_fell_14.c

npx lv_font_conv@1.5.3 --font IBMPlexMono-Light.ttf --symbols "0123456789: " \
  --size 50 --bpp 4 --format lvgl --no-compress --lv-include lvgl.h \
  --lv-font-name font_plex_50 -o font_plex_50.c
```

To add a glyph, add it to `--symbols` and regenerate every font that needs it.
