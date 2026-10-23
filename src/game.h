// Copyright (c) 2026, Dale Wick
// SPDX-License-Identifier: BSD-3-Clause
// See LICENSE for the full license text.

#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

enum PlayerAnimation {
    PLAYER_ANIMATION_IDLE,
    PLAYER_ANIMATION_WALK,
    PLAYER_ANIMATION_ATTACK,
};

struct Player {
    int x;
    int y;
    int vx, vy; // velocity in subpixels (1/16 pixel) per frame
    int sx, sy; // subpixel remainder of position
    bool on_ground;
    uint8_t attack_timer; // frames left for the attack animation
    bool facing_right;
    enum PlayerAnimation animation;
    int health;
    int mp; // magic points
    int score;
    int frame; // current frame of the animation
    int animation_timer; // timer for animation transitions
};

struct Particle {
    bool active; // whether the particle is active
    uint8_t type; // type of the particle (e.g., smoke, spark)
    int16_t x;
    int16_t y;
    int8_t sx, sy; // subpixel remainder of position
    int16_t vx, vy; // velocity in subpixels (1/16 pixel) per frame
    int16_t lifetime_timer; // time left in ms for the particle
};

void game_init();
void game_update();
void game_render();
void game_handle_input(uint8_t input, bool down);

#endif // GAME_H