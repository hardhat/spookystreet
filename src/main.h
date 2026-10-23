// Copyright (c) 2026, Dale Wick
// SPDX-License-Identifier: BSD-3-Clause
// See LICENSE for the full license text.

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdbool.h>

enum GameState {
    STATE_MENU,
    STATE_GAME,
    STATE_EXIT
};

enum INPUT
{
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_A,
    INPUT_B,
    INPUT_X,
    INPUT_Y,
    INPUT_START,
    INPUT_SELECT,
    INPUT_L,
    INPUT_R,
    MAX_INPUT
};

#define SPRITE_FLAG_PRIORITY 0x02
#define SPRITE_FLAG_FLIP_Y 0x04
#define SPRITE_FLAG_FLIP_X 0x08

extern enum GameState current_state;

#define STREET_MAP_WIDTH 80
#define STREET_MAP_HEIGHT 15
extern uint8_t street_fg_map[STREET_MAP_WIDTH*STREET_MAP_HEIGHT];

#define PARTICLE_DAMAGE 0x22

void set_game_state(enum GameState new_state);
/// Fill off-screen sprites table to their default state
void reset_sprite(void);
/// Render all sprites to the screen from the off-screen sprites table
void render_sprites(void);
/// Clear all sprites from the screen and off-screen sprites table
void clear_sprites(void);
/// Add a sprite to the screen with the given parameters
void add_sprite(uint16_t x, uint16_t y, uint8_t sprite, uint16_t flags);
/// Show map on layer 0
void show_map(uint8_t *map,uint8_t width,uint8_t height);
/// Redraw a rectangle of street_fg_map (in tiles) on layer 1
void update_fg_tiles(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
/// @brief Show tutorial map
/// @param  none
void show_tutorial_map(void);
/// @brief Handle input from keyboard and game controller
/// @param key input key code
void debug_log(const char *message);
/// @brief  Log a formatted message to the debug output
/// @param format The format string (printf-style)
/// @param ... The values to format
void debug_logf(const char *format, ...);

#endif // MAIN_H