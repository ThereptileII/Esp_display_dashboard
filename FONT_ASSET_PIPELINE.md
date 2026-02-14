# Font Asset Pipeline (Orbitron / LVGL)

## Source of truth
- Font family: Orbitron.
- Generated C assets in this repo:
  - `orbitron_16_600.c`
  - `orbitron_20_700.c`
  - `orbitron_32_800.c`
  - `orbitron_48_900.c`
- Export declarations in `fonts.h`.

## Required LVGL alignment
When regenerating fonts, keep `lv_conf.h` aligned with converter output:
- `LV_FONT_FMT_TXT_BPP = 4`
- `LV_FONT_FMT_TXT_LARGE = 1`
- `LV_USE_FONT_COMPRESSED = 1`

## Converter checklist
For each generated font:
1. Keep same bpp/compression settings across all Orbitron sizes.
2. Include all UI glyphs used in labels, units, and symbols.
3. Preserve naming consistency (`orbitron_<size>_<weight>` style).

## PR checklist for font/UI asset changes
1. Build sketch and confirm no missing symbol references.
2. Validate key screens render expected glyphs:
   - clock digits
   - units/labels
   - punctuation and separators
3. Verify edge glyphs used by your locale/range.
4. Include before/after screenshot if visual output changed.
