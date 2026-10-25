// Copyright (c) 2026, Dale Wick
// SPDX-License-Identifier: BSD-3-Clause
// See LICENSE for the full license text.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "game.h"
#include "img.h"

#include <zvb_hardware.h>

#define MAP_WIDTH STREET_MAP_WIDTH    // In tiles
#define MAP_HEIGHT STREET_MAP_HEIGHT // In tiles

#define SCREEN_WIDTH 320  // In pixels
#define SCREEN_HEIGHT 240 // In pixels

#define TILE_SIZE 16

#define SUBPIXEL 16
#define PLAYER_HITBOX_INSET 2
#define PLAYER_MAX_HITBOX_HEIGHT 8

#define MAX_RUN_SPEED 48

#define GROUND_ACCEL 4
#define GROUND_FRICTION 6

#define MAX_HEALTH 100
#define MAX_MP 100
#define BARREL_MP 25
#define ITEM_SCORE 50
#define TILE_ITEM_SCORE 0x2A
#define ITEM_DURABILITY 4 // hits to destroy an item

#define TILE_HEART 0x40

#define ATTACK_FRAMES 24 // 3 attack frames * 128 ms at 16 ms per update
#define ATTACK_REACH 24  // pixels in front of the player's center

struct Player player;

#define MAX_PARTICLES 16
struct Particle particles[MAX_PARTICLES];

/// The collision tile types
#define COLL_EMPTY_TILE 0
#define COLL_NON_DESTRUCTIBLE_TILE 1
#define COLL_DESTRUCTABLE_TL 2 // top left corner of destructable tile
#define COLL_DESTRUCTABLE_TR 3 // top right corner of destructable tile
#define COLL_DESTRUCTABLE_BL 4 // bottom left corner of destructable tile
#define COLL_DESTRUCTABLE_BR 5 // bottom right corner of destructable tile
#define COLL_ITEM_BARREL_TL 6 // top left corner of item barrel
#define COLL_ITEM_BARREL_TR 7 // top right corner of item barrel
#define COLL_ITEM_BARREL_BL 8 // bottom left corner of item barrel
#define COLL_ITEM_BARREL_BR 9 // bottom right corner of item barrel
#define COLL_ITEM_PUMPKIN_TL 10 // top left corner of item pumpkin
#define COLL_ITEM_PUMPKIN_TR 11 // top right corner of item pumpkin
#define COLL_ITEM_PUMPKIN_BL 12 // bottom left corner of item pumpkin
#define COLL_ITEM_PUMPKIN_BR 13 // bottom right corner of item pumpkin
#define COLL_ITEM_STREET_LIGHT_T 14 // Top
#define COLL_ITEM_STREET_LIGHT_U 15 // Middle upper part of the street light
#define COLL_ITEM_STREET_LIGHT_L 16 // Middle lower part of the street light
#define COLL_ITEM_STREET_LIGHT_B 17 // Bottom part of the street light
#define COLL_ITEM_PUMPKIN_1x1 18 // 1x1 pumpkin item

const uint8_t collision_map[256] = {
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // row 0x00
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // row 0x10
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // row 0x20
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // row 0x30

    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // row 0x40
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // row 0x50
    0,0,0,0, 1,1,1,1, 2,3,2,3, 2,3,6,7, // row 0x60
    0,0,0,0, 1,1,1,1, 4,5,4,5, 4,5,8,9, // row 0x70

    2,3,2,3,    2,3,2,3,     2,3,1,1, 1,1,1,1, // row 0x80
    4,5,4,5,    4,5,4,5,     2,3,1,1, 1,1,1,1, // row 0x90
    10,11,0,0,  10,11,10,11, 0,0,0,0, 0,0,0,0, // row 0xa0
    12,13,18,0, 12,13,12,13, 0,0,0,0, 0,0,0,0, // row 0xb0

    0,0,0,0, 0,0,1,1, 1,1,1,1, 1,1,0,14, // row 0xc0
    0,0,0,0, 0,0,1,1, 1,1,1,1, 1,1,14,15, // row 0xd0
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,16, // row 0xe0
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,17, // row 0xf0
};

// Remaining hits per item, stored at the item's top-left tile
uint8_t item_durability[MAP_WIDTH*MAP_HEIGHT];

int16_t x_scroll;

struct Frame {
    uint8_t base;   // base tile index
    uint8_t width;  // in tiles
    uint8_t height;  // in tiles
};

struct Animation {
    uint8_t frame_count;  // number of frames in the animation
    const struct Frame* frames;  // pointer to array of frames in the animation
    int animation_timer; // timer for animation transitions in ms
};

const struct Frame idle_frames[] = {
    { .base = 0xe0, .width = 1, .height = 2 },
    { .base = 0xe1, .width = 1, .height = 2 },
};

const struct Frame attack_frames[] = {
    { .base = 0xc0, .width = 2, .height = 2 },
    { .base = 0xc2, .width = 2, .height = 2 },
    { .base = 0xc4, .width = 2, .height = 2 },
};

const struct Frame walk_frames[] = {
    { .base = 0xe1, .width = 1, .height = 2 },
    { .base = 0xe2, .width = 1, .height = 2 },
    { .base = 0xe3, .width = 1, .height = 2 },
    { .base = 0xe4, .width = 1, .height = 2 },
};

const struct Frame rat_walk_frames[] = {
    { .base = 0x00, .width = 2, .height = 2 },
    { .base = 0x02, .width = 2, .height = 2 },
    { .base = 0x04, .width = 2, .height = 2 },
};

const struct Frame rat_attack_frames[] = {
    { .base = 0x06, .width = 2, .height = 2 },
    { .base = 0x08, .width = 2, .height = 2 },
    { .base = 0x08, .width = 2, .height = 2 },
};

const struct Frame ghost_walk_frames[] = {
    { .base = 0x0A, .width = 2, .height = 2 },
    { .base = 0x0A, .width = 2, .height = 2 },
    { .base = 0x0A, .width = 2, .height = 2 },
};

const struct Frame ghost_attack_frames[] = {
    { .base = 0x0A, .width = 2, .height = 2 },
    { .base = 0x0A, .width = 2, .height = 2 },
    { .base = 0x0A, .width = 2, .height = 2 },
};


const struct Frame hand_walk_frames[] = {
    { .base = 0x0C, .width = 4, .height = 3 },
    { .base = 0x3C, .width = 4, .height = 3 },
};

const struct Frame hand_attack_frames[] = {
    { .base = 0x60, .width = 4, .height = 3 },
    { .base = 0x60, .width = 4, .height = 3 },
};

const struct Animation hand_animation[] = {
    [ENEMY_ANIMATION_WALK] = {
        .frame_count = 2,
        .frames = hand_walk_frames,
        .animation_timer = 64 // example value in ms
    },
    [ENEMY_ANIMATION_ATTACK] = {
        .frame_count = 2,
        .frames = hand_attack_frames,
        .animation_timer = 128 // example value in ms
    },
};

const struct Animation ghost_animation[] = {
    [ENEMY_ANIMATION_WALK] = {
        .frame_count = 3,
        .frames = ghost_walk_frames,
        .animation_timer = 64 // example value in ms
    },
    [ENEMY_ANIMATION_ATTACK] = {
        .frame_count = 3,
        .frames = ghost_attack_frames,
        .animation_timer = 128 // example value in ms
    },
};

const struct Animation player_animation[] = {
    [PLAYER_ANIMATION_IDLE] = {
        .frame_count = 2,
        .frames = idle_frames,
        .animation_timer = 256 // example value in ms
    },
    [PLAYER_ANIMATION_WALK] = {
        .frame_count = 4,
        .frames = walk_frames,
        .animation_timer = 64 // example value in ms
    },
    [PLAYER_ANIMATION_ATTACK] = {
        .frame_count = 3,
        .frames = attack_frames,
        .animation_timer = 128 // example value in ms
    },
};

const struct Animation rat_animation[] = {
    [ENEMY_ANIMATION_WALK] = {
        .frame_count = 3,
        .frames = rat_walk_frames,
        .animation_timer = 64 // example value in ms
    },
    [ENEMY_ANIMATION_ATTACK] = {
        .frame_count = 3,
        .frames = rat_attack_frames,
        .animation_timer = 128 // example value in ms
    },
};


#define MAX_WAVES 7
struct EnemyWave {
    uint8_t enemy_count;
    uint8_t enemy_type;
    uint8_t x_trigger;
} enemyWaves[MAX_WAVES] = {
    { .enemy_count = 4, .enemy_type = ENEMY_TYPE_RAT, .x_trigger = 10 },
    { .enemy_count = 3, .enemy_type = ENEMY_TYPE_GHOST, .x_trigger = 20 },
    { .enemy_count = 8, .enemy_type = ENEMY_TYPE_RAT, .x_trigger = 30 },
    { .enemy_count = 5, .enemy_type = ENEMY_TYPE_GHOST, .x_trigger = 40 },
    { .enemy_count = 12, .enemy_type = ENEMY_TYPE_RAT, .x_trigger = 50 },
    { .enemy_count = 8, .enemy_type = ENEMY_TYPE_GHOST, .x_trigger = 60 },
    { .enemy_count = 1, .enemy_type = ENEMY_TYPE_HAND, .x_trigger = 70 },
};
uint8_t current_wave;
uint8_t wave_spawned;
bool wave_active;
uint16_t wave_timer;    // Timer for tracking spawn timing of the current wave

#define MAX_ENEMIES 8
struct Enemy enemies[MAX_ENEMIES];

#define FRAME_MS 16 // one update at 60 FPS
#define WAVE_SPAWN_INTERVAL 700 // ms between enemies of a wave
#define ENEMY_SCORE 100
#define ENEMY_DEADZONE 3 // pixels; stops enemies jittering around the player
#define ENEMY_ATTACK_FRAMES 16
#define ENEMY_ART_FACES_LEFT true
#define PLAYER_ATTACK_DAMAGE 10

struct EnemyDef {
    const struct Animation* animation;
    uint8_t speed;         // subpixels per frame (the player's top speed is MAX_RUN_SPEED)
    uint8_t damage;
    uint8_t health;
    uint8_t half_width;    // hitbox, centered on the enemy position
    uint8_t hitbox_height; // hitbox, measured up from the enemy position
    uint8_t cooldown;      // frames between attacks
    uint8_t backoff;       // frames spent retreating after an attack
    bool ignore_collision; // moves through walls and items
};

const struct EnemyDef enemy_defs[] = {
    [ENEMY_TYPE_RAT] = {
        .animation = rat_animation,
        .speed = MAX_RUN_SPEED * 5 / 6,
        .damage = 10,
        .health = 20,
        .half_width = 14,
        .hitbox_height = 8,
        .cooldown = 45,
        .backoff = 30,
        .ignore_collision = false,
    },
    [ENEMY_TYPE_GHOST] = {
        .animation = ghost_animation,
        .speed = MAX_RUN_SPEED / 2,
        .damage = 15,
        .health = 30,
        .half_width = 14,
        .hitbox_height = 16,
        .cooldown = 1000 / FRAME_MS,
        .backoff = 0,
        .ignore_collision = true,
    },
    [ENEMY_TYPE_HAND] = {
        .animation = hand_animation,
        .speed = MAX_RUN_SPEED / 2,
        .damage = 20,
        .health = 100,
        .half_width = 28,
        .hitbox_height = 8, // only the fingertips at the bottom of the sprite can hurt
        .cooldown = 60,
        .backoff = 40,
        .ignore_collision = false,
    },
};

bool held_left, held_up, held_right, held_down, held_attack;


void spawn_particle(int x, int y, int16_t vx, int16_t vy, uint8_t type, uint16_t lifetime) {
    for(uint8_t i = 0; i < MAX_PARTICLES; i++) {
        if(!particles[i].active) {
            particles[i].active = true;
            particles[i].x = x;
            particles[i].y = y;
            particles[i].sx = 0;
            particles[i].sy = 0;
            particles[i].vx = vx;
            particles[i].vy = vy;
            particles[i].type = type;
            particles[i].lifetime_timer = lifetime;
            break;
        }
    }
}

void spawn_damage(int16_t x, int16_t y) {
    // Make 3 damage particles
    for(int i = 0; i < 3; i++) {
        int16_t vx = (rand() % 5 - 2) << 4; // random velocity between -2 and 2 subpixels/frame
        int16_t vy = (rand() % 5 - 2) << 4; // random velocity between -2 and 2 subpixels/frame
        spawn_particle(x, y, vx, vy, PARTICLE_DAMAGE, 500); // 500ms lifetime
    }
}

static bool tile_solid(uint8_t tx, uint8_t ty) {
    int index = ty * MAP_WIDTH + tx;
    return collision_map[street_bg_tilemap[index]] != COLL_EMPTY_TILE ||
           collision_map[street_fg_map[index]] != COLL_EMPTY_TILE;
}

// Out-of-bounds counts as solid
static bool rect_collides(int left, int top, int width, int height) {
    int right = left + width - 1;
    int bottom = top + height - 1;
    if(left < 0 || top < 0 || right >= MAP_WIDTH * TILE_SIZE || bottom >= MAP_HEIGHT * TILE_SIZE) {
        return true;
    }
    for(uint8_t ty = top / TILE_SIZE; ty <= bottom / TILE_SIZE; ty++) {
        for(uint8_t tx = left / TILE_SIZE; tx <= right / TILE_SIZE; tx++) {
            if(tile_solid(tx, ty)) return true;
        }
    }
    return false;
}

// Player position is bottom-center of the current frame
static int hitbox_half_width(void) {
    const struct Frame* f = &player_animation[player.animation].frames[player.frame];
    return f->width * (TILE_SIZE / 2) - PLAYER_HITBOX_INSET;
}

static int player_hitbox_height(void) {
    const struct Frame* f = &player_animation[player.animation].frames[player.frame];
    int height = f->height * TILE_SIZE;
    // Top row of 3-tile-tall frames is empty space above the head
    if(height > PLAYER_MAX_HITBOX_HEIGHT) height = PLAYER_MAX_HITBOX_HEIGHT;
    return height;
}

static bool player_collides(int x, int y) {
    int half = hitbox_half_width();
    int height = player_hitbox_height();
    return rect_collides(x - half, y - height, half * 2, height);
}

// Step one pixel at a time so the player stops flush against walls
static void move_player(void) {
    player.sx += player.vx;
    int dx = player.sx / SUBPIXEL;
    player.sx -= dx * SUBPIXEL;
    int step = dx > 0 ? 1 : -1;
    for(; dx != 0; dx -= step) {
        if(player_collides(player.x + step, player.y)) {
            player.vx = 0;
            player.sx = 0;
            break;
        }
        player.x += step;
    }
    // Sub-pixel motion into a wall would otherwise leave vx nonzero for a few frames
    if(player.vx != 0 && player_collides(player.x + (player.vx > 0 ? 1 : -1), player.y)) {
        player.vx = 0;
        player.sx = 0;
    }

    player.sy += player.vy;
    int dy = player.sy / SUBPIXEL;
    player.sy -= dy * SUBPIXEL;
    step = dy > 0 ? 1 : -1;
    for(; dy != 0; dy -= step) {
        if(player_collides(player.x, player.y + step)) {
            player.vy = 0;
            player.sy = 0;
            break;
        }
        player.y += step;
    }

    player.on_ground = player_collides(player.x, player.y + 1);

    // Set the x_scroll based on player location
    int16_t target = x_scroll;
    if(player.x<target+SCREEN_WIDTH/4) {
        target=player.x-SCREEN_WIDTH/4;
        if(target<0) target=0; // Underflow
    } else if(player.x>target+SCREEN_WIDTH*3/4) {
        target=player.x-SCREEN_WIDTH*3/4;
        if(target>MAP_WIDTH*16-SCREEN_WIDTH) {
            target=MAP_WIDTH*16-SCREEN_WIDTH;
        }
    }
    x_scroll=target;
}

static bool enemy_collides(const struct EnemyDef *def, int x, int y) {
    if(def->ignore_collision) {
        // Only keep floating enemies inside the map
        return x - def->half_width < 0 || x + def->half_width > MAP_WIDTH * TILE_SIZE ||
               y - def->hitbox_height < 0 || y > MAP_HEIGHT * TILE_SIZE;
    }
    return rect_collides(x - def->half_width, y - def->hitbox_height, def->half_width * 2, def->hitbox_height);
}

static void set_enemy_animation(struct Enemy *e, enum EnemyAnimation animation) {
    if(e->animation != animation) {
        e->animation = animation;
        e->frame = 0;
        e->animation_timer = enemy_defs[e->type].animation[animation].animation_timer;
    }
}

static bool spawn_enemy(uint8_t type, uint8_t index) {
    const struct EnemyDef *def = &enemy_defs[type];
    struct Enemy *e = NULL;
    for(uint8_t i = 0; i < MAX_ENEMIES; i++) {
        if(enemies[i].type == ENEMY_TYPE_NONE) {
            e = &enemies[i];
            break;
        }
    }
    if(!e) return false;

    // Enter from just off-screen, alternating sides and retrying where a wall is in the way
    for(uint8_t attempt = 0; attempt < 4; attempt++) {
        bool right = ((index + attempt) & 1) == 0;
        int x = right ? x_scroll + SCREEN_WIDTH + def->half_width + 2 : x_scroll - def->half_width - 2;
        int y = player.y + (rand() % 5 - 2) * 8;
        if(enemy_collides(def, x, y)) continue;

        memset(e, 0, sizeof(*e));
        e->type = type;
        e->x = x;
        e->y = y;
        e->health = def->health;
        e->facing_right = x < player.x;
        e->animation = ENEMY_ANIMATION_WALK;
        e->animation_timer = def->animation[ENEMY_ANIMATION_WALK].animation_timer;
        return true;
    }
    return false;
}

// Step one pixel at a time so enemies stop flush against walls
static void move_enemy(struct Enemy *e, const struct EnemyDef *def) {
    e->sx += e->vx;
    int dx = e->sx / SUBPIXEL;
    e->sx -= dx * SUBPIXEL;
    int step = dx > 0 ? 1 : -1;
    for(; dx != 0; dx -= step) {
        if(enemy_collides(def, e->x + step, e->y)) {
            e->sx = 0;
            break;
        }
        e->x += step;
    }

    e->sy += e->vy;
    int dy = e->sy / SUBPIXEL;
    e->sy -= dy * SUBPIXEL;
    step = dy > 0 ? 1 : -1;
    for(; dy != 0; dy -= step) {
        if(enemy_collides(def, e->x, e->y + step)) {
            e->sy = 0;
            break;
        }
        e->y += step;
    }
}

static bool enemy_touches_player(const struct Enemy *e, const struct EnemyDef *def) {
    int half = hitbox_half_width();
    int height = player_hitbox_height();
    return e->x - def->half_width < player.x + half && e->x + def->half_width > player.x - half &&
           e->y - def->hitbox_height < player.y && e->y > player.y - height;
}

static void hurt_player(uint8_t damage) {
    player.health -= damage;
    if(player.health < 0) player.health = 0;
    spawn_damage(player.x, player.y - TILE_SIZE);
}

static void update_enemy(struct Enemy *e, uint8_t index) {
    const struct EnemyDef *def = &enemy_defs[e->type];
    // Spread enemies over a few rows so they don't stack on one spot
    int dx = player.x - e->x;
    int dy = player.y + ((index % 3) - 1) * 4 - e->y;

    if(e->cooldown) e->cooldown--;
    if(e->attack_timer && --e->attack_timer == 0) {
        set_enemy_animation(e, ENEMY_ANIMATION_WALK);
    }

    if(dx != 0) e->facing_right = dx > 0;
    if(e->backoff_timer) {
        e->backoff_timer--;
        e->vx = dx > 0 ? -def->speed : def->speed;
        e->vy = 0;
    } else {
        e->vx = dx > ENEMY_DEADZONE ? def->speed : (dx < -ENEMY_DEADZONE ? -def->speed : 0);
        e->vy = dy > ENEMY_DEADZONE ? def->speed : (dy < -ENEMY_DEADZONE ? -def->speed : 0);
    }
    move_enemy(e, def);

    if(!e->cooldown && player.health > 0 && enemy_touches_player(e, def)) {
        hurt_player(def->damage);
        e->cooldown = def->cooldown;
        e->backoff_timer = def->backoff;
        e->attack_timer = ENEMY_ATTACK_FRAMES;
        set_enemy_animation(e, ENEMY_ANIMATION_ATTACK);
    }

    if(e->animation_timer > 0) {
        e->animation_timer -= FRAME_MS;
        if(e->animation_timer <= 0) {
            const struct Animation *anim = &def->animation[e->animation];
            e->animation_timer = anim->animation_timer;
            e->frame = (e->frame + 1) % anim->frame_count;
        }
    }
}

static void update_waves(void) {
    if(!wave_active) {
        if(current_wave < MAX_WAVES && player.x / TILE_SIZE >= enemyWaves[current_wave].x_trigger) {
            wave_active = true;
            wave_spawned = 0;
            wave_timer = 0;
        }
        return;
    }

    const struct EnemyWave *wave = &enemyWaves[current_wave];
    if(wave_spawned < wave->enemy_count) {
        if(wave_timer > FRAME_MS) {
            wave_timer -= FRAME_MS;
        } else if(spawn_enemy(wave->enemy_type, wave_spawned)) {
            wave_spawned++;
            wave_timer = WAVE_SPAWN_INTERVAL;
        }
        return;
    }

    // Everything has spawned; the wave is over once the last enemy is defeated
    for(uint8_t i = 0; i < MAX_ENEMIES; i++) {
        if(enemies[i].type != ENEMY_TYPE_NONE) return;
    }
    wave_active = false;
    current_wave++;
}

static void update_enemies(void) {
    update_waves();
    for(uint8_t i = 0; i < MAX_ENEMIES; i++) {
        if(enemies[i].type != ENEMY_TYPE_NONE) update_enemy(&enemies[i], i);
    }
}

// Damages every enemy whose hitbox overlaps the (inclusive) rectangle
static void attack_enemies(int x0, int y0, int x1, int y1) {
    for(uint8_t i = 0; i < MAX_ENEMIES; i++) {
        struct Enemy *e = &enemies[i];
        if(e->type == ENEMY_TYPE_NONE) continue;
        const struct EnemyDef *def = &enemy_defs[e->type];
        if(e->x - def->half_width > x1 || e->x + def->half_width - 1 < x0 ||
           e->y - def->hitbox_height > y1 || e->y - 1 < y0) continue;

        spawn_damage(e->x, e->y - def->hitbox_height);
        e->health -= PLAYER_ATTACK_DAMAGE;
        if(e->health <= 0) {
            e->type = ENEMY_TYPE_NONE;
            player.score += ENEMY_SCORE;
        }
    }
}

static void render_enemies(void) {
    for(uint8_t i = 0; i < MAX_ENEMIES; i++) {
        const struct Enemy *e = &enemies[i];
        if(e->type == ENEMY_TYPE_NONE) continue;
        const struct Frame* f = &enemy_defs[e->type].animation[e->animation].frames[e->frame];
        bool flip = e->facing_right == ENEMY_ART_FACES_LEFT;
        int left = e->x - x_scroll - f->width * 8;
        int top = e->y - f->height * 16;
        if(left + f->width * 16 <= 0 || left >= SCREEN_WIDTH) continue;
        for(uint8_t row = 0; row < f->height; row++) {
            for(uint8_t col = 0; col < f->width; col++) {
                uint8_t tile = f->base + (row << 4) + (flip ? f->width - 1 - col : col);
                add_sprite(left + (col << 4), top + (row << 4), tile, flip ? SPRITE_FLAG_FLIP_X : 0);
            }
        }
    }
}

static uint8_t fg_coll(int tx, int ty) {
    return collision_map[street_fg_map[ty * MAP_WIDTH + tx]];
}

static bool is_light_body(uint8_t type) {
    return type > COLL_ITEM_STREET_LIGHT_T && type <= COLL_ITEM_STREET_LIGHT_B;
}

// Finds the item containing a foreground tile; origin is its top-left tile
static bool find_item(int tx, int ty, int *ox, int *oy, int *w, int *h) {
    uint8_t type = fg_coll(tx, ty);
    if(type == COLL_ITEM_PUMPKIN_1x1) {
        *ox = tx; *oy = ty; *w = 1; *h = 1;
        return true;
    }
    if(type >= COLL_ITEM_STREET_LIGHT_T && type <= COLL_ITEM_STREET_LIGHT_B) {
        int top = ty;
        while(top > 0 && is_light_body(fg_coll(tx, top))) top--;
        if(fg_coll(tx, top) != COLL_ITEM_STREET_LIGHT_T) return false;
        int bottom = top;
        while(bottom + 1 < MAP_HEIGHT && is_light_body(fg_coll(tx, bottom + 1))) bottom++;
        *ox = tx; *oy = top; *w = 1; *h = bottom - top + 1;
        return true;
    }
    if(type < COLL_DESTRUCTABLE_TL || type > COLL_ITEM_PUMPKIN_BR) return false;
    // 2x2 items are laid out TL, TR, BL, BR from a base type of 2, 6 or 10
    uint8_t part = (type - COLL_DESTRUCTABLE_TL) & 3;
    *ox = tx - (part & 1);
    *oy = ty - (part >> 1);
    if(*ox < 0 || *oy < 0 || fg_coll(*ox, *oy) != type - part) return false;
    *w = 2; *h = 2;
    return true;
}

// Returns true if an item was hit
static bool hit_item(int tx, int ty) {
    int ox, oy, w, h;
    if(!find_item(tx, ty, &ox, &oy, &w, &h)) return false;
    uint8_t *durability = &item_durability[oy * MAP_WIDTH + ox];
    if(--*durability) {
        spawn_damage(tx*TILE_SIZE, ty*TILE_SIZE);
        return true;
    }
    switch(fg_coll(ox, oy)) {
        case COLL_ITEM_BARREL_TL:
            player.mp += BARREL_MP;
            if(player.mp > MAX_MP) player.mp = MAX_MP;
            spawn_particle(tx<<4, ty<<4,0,-18,TILE_ITEM_SCORE-1,500);
            break;
        case COLL_ITEM_PUMPKIN_TL:
        case COLL_ITEM_PUMPKIN_1x1:
            spawn_particle(tx<<4, ty<<4,0,-18,TILE_HEART,500);
            player.health += MAX_HEALTH / 4;
            if(player.health > MAX_HEALTH) player.health = MAX_HEALTH;
            break;
        default:
            player.score += ITEM_SCORE;
            spawn_particle(tx<<4, ty<<4,0,-18,TILE_ITEM_SCORE,500);
            break;
    }

    for(int y = oy; y < oy + h; y++) {
        for(int x = ox; x < ox + w; x++) {
            street_fg_map[y * MAP_WIDTH + x] = 0;
        }
    }
    update_fg_tiles(ox, oy, w, h);
    return true;
}

// Hits the nearest item in front of the player, using the bottom of the sprite as the strike zone
static void player_attack(void) {
    int x0 = player.facing_right ? player.x : player.x - ATTACK_REACH;
    int x1 = x0 + ATTACK_REACH - 1;
    int y0 = player.y - PLAYER_MAX_HITBOX_HEIGHT;
    int y1 = player.y - 1;
    if(x0 < 0) x0 = 0;
    if(y0 < 0) y0 = 0;
    if(x1 >= MAP_WIDTH * TILE_SIZE) x1 = MAP_WIDTH * TILE_SIZE - 1;
    if(y1 >= MAP_HEIGHT * TILE_SIZE) y1 = MAP_HEIGHT * TILE_SIZE - 1;

    attack_enemies(x0, y0, x1, y1);

    int tx0 = x0 / TILE_SIZE, tx1 = x1 / TILE_SIZE;
    int ty0 = y0 / TILE_SIZE, ty1 = y1 / TILE_SIZE;
    int step = player.facing_right ? 1 : -1;
    int tx = player.facing_right ? tx0 : tx1;
    for(int i = tx1 - tx0; i >= 0; i--, tx += step) {
        for(int ty = ty0; ty <= ty1; ty++) {
            if(hit_item(tx, ty)) return;
        }
    }
}

void game_init() {
    memset(item_durability, ITEM_DURABILITY, sizeof(item_durability));
    memset(enemies, 0, sizeof(enemies));
    current_wave = 0;
    wave_spawned = 0;
    wave_active = false;
    wave_timer = 0;
    // Initialize player
    player.x = 32;
    player.y = 240-32;
    player.vx = 0;
    player.vy = 0;
    player.sx = 0;
    player.sy = 0;
    player.on_ground = false;
    player.attack_timer = 0;
    held_left = held_right = held_down = held_up = held_attack = false;
    player.health = MAX_HEALTH;
    player.mp = 0;
    player.score = 0;
    player.frame = 0;
    player.animation = PLAYER_ANIMATION_IDLE;
    player.facing_right = true;
    player.animation_timer = player_animation[player.animation].animation_timer;
}

static void set_animation(enum PlayerAnimation animation) {
    if(player.animation != animation) {
        player.animation = animation;
        player.frame = 0;
        player.animation_timer = player_animation[animation].animation_timer;
    }
}

static int update_velocity(int velocity, int direction) {
    int target = direction * MAX_RUN_SPEED;
    int change = direction != 0 ? GROUND_ACCEL : GROUND_FRICTION;

    if(velocity < target) {
        velocity += change;
        if(velocity > target) velocity = target;
    } else if(velocity > target) {
        velocity -= change;
        if(velocity < target) velocity = target;
    }
    return velocity;
}
void update_particles() {
    for(uint8_t i = 0; i < MAX_PARTICLES; i++) {
        if(particles[i].active) {
            particles[i].sx += particles[i].vx;
            particles[i].sy += particles[i].vy;
            while(particles[i].sx >= 16) {
                particles[i].sx -= 16;
                particles[i].x++;
            }
            while(particles[i].sx < 0) {
                particles[i].sx += 16;
                particles[i].x--;
            }
            while(particles[i].sy >= 16) {
                particles[i].sy -= 16;
                particles[i].y++;
            }
            while(particles[i].sy < 0) {
                particles[i].sy += 16;
                particles[i].y--;
            }
            if(particles[i].lifetime_timer > 0) {
                particles[i].lifetime_timer -= 16; // assuming a 60 FPS update rate, ~16ms per frame
                if(particles[i].lifetime_timer <= 0) {
                    particles[i].active = false;
                }
            }
        }
    }
}

void game_update() {
    int horizontal = held_right - held_left;
    int vertical = held_down - held_up;

    player.vx = update_velocity(player.vx, horizontal);
    player.vy = update_velocity(player.vy, vertical);
    if(horizontal != 0) player.facing_right = horizontal > 0;

    move_player();

    if(player.attack_timer) player.attack_timer--;
    if(player.attack_timer) {
        set_animation(PLAYER_ANIMATION_ATTACK);
    } else if(player.vx != 0 || player.vy != 0) {
        set_animation(PLAYER_ANIMATION_WALK);
    } else {
        set_animation(PLAYER_ANIMATION_IDLE);
    }

    if(player.animation_timer > 0) {
        player.animation_timer-=16; // assuming a 60 FPS update rate, ~16ms per frame
        if(player.animation_timer <= 0) {
            player.animation_timer = player_animation[player.animation].animation_timer;
            player.frame = (player.frame + 1) % player_animation[player.animation].frame_count;
        }
    }
    update_enemies();
    update_particles();
}

void game_render() {
    reset_sprite();
    // Render game graphics here (anchor in bottom center)
    const struct Frame* f = &player_animation[player.animation].frames[player.frame];
    bool flip = !player.facing_right;
    int left = player.x - x_scroll - f->width * 8; // Calculate left position based on sprite width
    int top = player.y - f->height * 16; // Calculate top position based on sprite height
    for(uint8_t row = 0; row < f->height; row++) {
        for(uint8_t col = 0; col < f->width; col++) {
            // Tilesheet rows are 16 tiles wide
            uint8_t tile = f->base + (row << 4) + (flip ? f->width - 1 - col : col);
            add_sprite(left + (col << 4), top + (row << 4), tile, flip ? SPRITE_FLAG_FLIP_X : 0);
        }
    }
    render_enemies();
    // Add HUD sprites:
    for(uint16_t i=0;i<8;i++) {
        bool health = i*100/8 < player.health;
        add_sprite((uint16_t)i<<4,240-16,health ? '@' : '#',0);
    }
    char buf[16];
    sprintf(buf,"%05d",player.score);
    for(uint8_t i=0;i<strlen(buf);i++) {
        add_sprite(320+((i-5)<<4),240-16,buf[i],0);
    }

    // Render particles
    for(uint8_t i = 0; i < MAX_PARTICLES; i++) {
        if(particles[i].active) {
            add_sprite(particles[i].x - x_scroll, particles[i].y, particles[i].type, 0);
        }
    }
    render_sprites();
#ifdef __SDCC_VERSION_MAJOR
    zvb_ctrl_l0_scr_x_low = x_scroll & 0xFF;
    zvb_ctrl_l0_scr_x_high = (x_scroll >> 8) & 0xFF;
    zvb_ctrl_l1_scr_x_low = x_scroll & 0xFF;
    zvb_ctrl_l1_scr_x_high = (x_scroll >> 8) & 0xFF;
#endif

}

void game_handle_input(uint8_t input, bool down) {
    switch(input) {
        case INPUT_DOWN:
            held_down = down;
            held_up = false;
            break;
        case INPUT_LEFT:
            held_left = down;
            held_right = false;
            break;
        case INPUT_RIGHT:
            held_right = down;
            held_left = false;
            break;
        case INPUT_UP:
            held_up = down;
            held_down = false;
            break;
        case INPUT_A:
            held_attack = down;
            if(down && !player.attack_timer) {
                player.attack_timer = ATTACK_FRAMES;
                player_attack();
            }
            break;
    }
}