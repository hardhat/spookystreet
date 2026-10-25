; Copyright (c) 2026, Dale Wick
; SPDX-License-Identifier: BSD-3-Clause
; See LICENSE for the full license text.

; Export the symbols for

  .module img
  .area _TEXT

	.globl _tileset_palette
    .globl _tileset_palette_len
	.globl _tileset_tiles
    .globl _tileset_tiles_len
    .globl _street_bg_tilemap
    .globl _street_bg_tilemap_len
    .globl _street_fg_tilemap
    .globl _street_fg_tilemap_len

_tileset_palette:
    .incbin "img/tileset.ztp"
_tileset_palette_len:
    .dw .-_tileset_palette
_tileset_tiles:
    .dw _tileset_tiles_data
_tileset_tiles_data:
    ;.incbin "img/tileset.zts"
_tileset_tiles_len:
    .dw .-_tileset_tiles_data
_street_bg_tilemap_data:
    .incbin "map/bg.ztm"
_street_bg_tilemap_len:
    .dw .-_street_bg_tilemap_data
_street_bg_tilemap:
    .dw _street_bg_tilemap_data
_street_fg_tilemap_data:
    .incbin "map/fg.ztm"
_street_fg_tilemap_len:
    .dw .-_street_fg_tilemap_data
_street_fg_tilemap:
    .dw _street_fg_tilemap_data
