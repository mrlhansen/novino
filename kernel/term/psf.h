#pragma once

#include <kernel/types.h>

#define PSF1_FONT_MAGIC 0x0436
#define PSF1_FLAG_512   0x01
#define PSF1_FLAG_TAB   0x02
#define PSF1_FLAG_SEQ   0x04

#define PSF2_FONT_MAGIC 0x864AB572

typedef struct {
    uint16_t magic;           // Magic number
    uint8_t flags;            // Flags
    uint8_t height;           // Height in pixels (width is always 8)
} psf1_t;

typedef struct {
    uint32_t magic;           // Magic number
    uint32_t version;         // Version (should be zero)
    uint32_t header_size;     // Offset of bitmaps in file (should be 32)
    uint32_t flags;           // Flags (only a single flag exists, so this field is 1 when there is a unicode table)
    uint32_t num_glyph;       // Number of glyphs
    uint32_t bytes_per_glyph; // Size of each glyph
    uint32_t height;          // Height in pixels
    uint32_t width;           // Width in pixels
} psf2_t;

typedef struct {
    uint32_t version;         // PSF version (1 or 2)
    uint32_t num_glyph;       // Number of glyphs
    uint32_t bytes_per_row;   // Bytes per row
    uint32_t bytes_per_glyph; // Bytes per glyph
    uint32_t padding;         // Number of padding bits
    uint32_t height;          // Glyph height in pixels
    uint32_t width;           // Glyph width in pixels
    uint8_t *glyphs;          // Table of glyphs
    uint8_t *unicode;         // Unicode conversion table
} psf_t;

int psf_unicode(psf_t *psf, uint32_t unicode);
int psf_parse(psf_t *psf, const void *data);
