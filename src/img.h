// Copyright (c) 2026, Dale Wick
// SPDX-License-Identifier: BSD-3-Clause
// See LICENSE for the full license text.

#ifndef IMG_H
#define IMG_H

#include <stdint.h>

extern const uint16_t tileset_palette[256];
extern const uint16_t tileset_palette_len;
#define TILESET_PALETTE_BASE 0
extern const uint8_t *tileset_tiles;
#define TILESET_TILES_BASE 0
extern const uint16_t tileset_tiles_len;
extern const uint8_t *street_bg_tilemap;
extern const uint16_t street_bg_tilemap_len;
extern const uint8_t *street_fg_tilemap;
extern const uint16_t street_fg_tilemap_len;
#endif // IMG_H