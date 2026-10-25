#include <assert.h>
#include <stdio.h>

#include "../src/game.c"
#include "../src/sin88.h"

static uint8_t background[STREET_MAP_WIDTH * STREET_MAP_HEIGHT];
const uint8_t *street_bg_tilemap = background;
uint8_t street_fg_map[STREET_MAP_WIDTH * STREET_MAP_HEIGHT];

void reset_fg_map(void) {
    memset(street_fg_map, 0, sizeof(street_fg_map));
}

void reset_sprite(void) {}
void render_sprites(void) {}
void add_sprite(uint16_t x, uint16_t y, uint8_t sprite, uint16_t flags) {
    (void)x; (void)y; (void)sprite; (void)flags;
}
void update_fg_tiles(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    (void)x; (void)y; (void)width; (void)height;
}

static struct Enemy make_enemy(uint8_t type, int x, int y) {
    struct Enemy enemy = {0};
    enemy.type = type;
    enemy.x = x;
    enemy.y = y;
    enemy.animation_timer = 64;
    return enemy;
}

static void check_step(struct Enemy *enemy) {
    int old_x = enemy->x;
    int old_y = enemy->y;
    update_enemy(enemy, 1);
    assert(abs(enemy->x - old_x) <= 2);
    assert(abs(enemy->y - old_y) <= 2);
    assert(abs(enemy->vx) <= enemy_defs[enemy->type].speed);
    assert(abs(enemy->vy) <= enemy_defs[enemy->type].speed);
    assert(!enemy_collides(&enemy_defs[enemy->type], enemy->x, enemy->y));
}

static void test_lookup(void) {
    assert(ghost_offset_x(0) == 0 && ghost_offset_y(0) == GHOST_ORBIT_RADIUS);
    assert(ghost_offset_x(64) == GHOST_ORBIT_RADIUS && ghost_offset_y(64) == 0);
    assert(ghost_offset_x(128) == 0 && ghost_offset_y(128) == -GHOST_ORBIT_RADIUS);
    assert(ghost_offset_x(192) == -GHOST_ORBIT_RADIUS && ghost_offset_y(192) == 0);
    for(int angle = 0; angle < 256; angle++) {
        assert(g_ghost_offset[angle] == (GHOST_ORBIT_RADIUS * g_sine_table[angle]) >> 8);
    }
}

static void test_durations(void) {
    bool seen[94] = {false};
    bool clockwise = false;
    bool counterclockwise = false;
    game_init();
    player.x = 640;
    player.y = 120;
    for(int sample = 0; sample < 2048; sample++) {
        struct Enemy enemy = make_enemy(ENEMY_TYPE_GHOST, player.x + 10, player.y);
        start_ghost_retreat(&enemy, &enemy_defs[enemy.type]);
        assert(enemy.ghost_movement == GHOST_RETREAT);
        assert(enemy.orbit_timer >= 32 && enemy.orbit_timer <= 93);
        seen[enemy.orbit_timer] = true;
        clockwise |= enemy.orbit_direction > 0;
        counterclockwise |= enemy.orbit_direction < 0;
    }
    for(int duration = 32; duration <= 93; duration++) assert(seen[duration]);
    assert(clockwise && counterclockwise);
}

static void test_cycle(void) {
    game_init();
    player.x = 640;
    player.y = 120;
    struct Enemy enemy = make_enemy(ENEMY_TYPE_GHOST, player.x + 10, player.y);
    check_step(&enemy);
    assert(player.health == MAX_HEALTH - enemy_defs[enemy.type].damage);
    assert(enemy.ghost_movement == GHOST_RETREAT);
    int duration = enemy.orbit_timer;
    int retreat_frames = 0;
    while(enemy.ghost_movement == GHOST_RETREAT) {
        check_step(&enemy);
        assert(enemy.orbit_timer == duration);
        assert(player.health == 85);
        assert(++retreat_frames < 100);
    }
    assert(retreat_frames > 20);
    assert(abs(enemy.x - player.x - GHOST_ORBIT_RADIUS) <= 2);
    for(int frame = 0; frame < duration; frame++) {
        check_step(&enemy);
        int dx = enemy.x - player.x;
        int dy = enemy.y - player.y;
        assert(dx * dx + dy * dy >= 75 * 75);
        assert(dx * dx + dy * dy <= 85 * 85);
        assert(enemy.orbit_timer == duration - frame - 1);
        assert(player.health == 85);
    }
    assert(enemy.ghost_movement == GHOST_APPROACH);
    for(int frame = 0; frame < 100 && player.health == 85; frame++) check_step(&enemy);
    assert(player.health == 70);
    assert(enemy.ghost_movement == GHOST_RETREAT);
}

static void test_edges_and_moving_player(void) {
    const int positions[][2] = {{14, 16}, {1266, 16}, {14, 240}, {1266, 240}, {640, 120}};
    for(unsigned position = 0; position < sizeof(positions) / sizeof(positions[0]); position++) {
        for(int direction = -1; direction <= 1; direction += 2) {
            game_init();
            player.x = positions[position][0];
            player.y = positions[position][1];
            struct Enemy enemy = make_enemy(ENEMY_TYPE_GHOST, player.x, player.y);
            start_ghost_retreat(&enemy, &enemy_defs[enemy.type]);
            enemy.orbit_direction = direction;
            for(int frame = 0; enemy.ghost_movement == GHOST_RETREAT; frame++) {
                assert(frame < 100);
                check_step(&enemy);
            }
            int duration = enemy.orbit_timer;
            for(int frame = 0; frame < duration; frame++) {
                if(position == 4) player.x++;
                check_step(&enemy);
                assert(player.health == MAX_HEALTH);
            }
            assert(enemy.ghost_movement == GHOST_APPROACH);
        }
    }
    game_init();
    player.x = 40;
    player.y = 120;
    struct Enemy enemy = make_enemy(ENEMY_TYPE_GHOST, 40, 40);
    enemy.ghost_movement = GHOST_ORBIT;
    enemy.orbit_phase = (uint16_t)128 << 8;
    enemy.orbit_direction = 1;
    enemy.orbit_timer = 93;
    bool reversed = false;
    for(int frame = 0; frame < 93; frame++) {
        check_step(&enemy);
        reversed |= enemy.orbit_direction < 0;
    }
    assert(reversed);
}

static void test_wrap_and_attack_gate(void) {
    game_init();
    player.x = 640;
    player.y = 120;
    for(int direction = -1; direction <= 1; direction += 2) {
        struct Enemy enemy = make_enemy(ENEMY_TYPE_GHOST, player.x, player.y);
        enemy.ghost_movement = GHOST_ORBIT;
        enemy.orbit_phase = direction > 0 ? 65500 : 0;
        enemy.orbit_direction = direction;
        enemy.orbit_timer = 1;
        check_step(&enemy);
        assert(enemy.orbit_phase == (direction > 0 ? 160 : 65340));
        assert(enemy.ghost_movement == GHOST_APPROACH);
        assert(player.health == MAX_HEALTH);
    }
}

static void test_other_enemies(void) {
    const uint8_t types[] = {ENEMY_TYPE_RAT, ENEMY_TYPE_HAND};
    for(unsigned index = 0; index < sizeof(types) / sizeof(types[0]); index++) {
        game_init();
        player.x = 640;
        player.y = 120;
        struct Enemy enemy = make_enemy(types[index], player.x + 10, player.y);
        update_enemy(&enemy, 1);
        const struct EnemyDef *def = &enemy_defs[enemy.type];
        assert(player.health == MAX_HEALTH - def->damage);
        assert(enemy.cooldown == def->cooldown);
        assert(enemy.backoff_timer == def->backoff);
        update_enemy(&enemy, 1);
        assert(enemy.vx == def->speed && enemy.vy == 0);
        assert(enemy.backoff_timer == def->backoff - 1);
        assert(enemy.ghost_movement == GHOST_APPROACH);
    }
}

int main(void) {
    srand(1);
    test_lookup();
    test_durations();
    test_cycle();
    test_edges_and_moving_player();
    test_wrap_and_attack_gate();
    test_other_enemies();
    puts("Ghost movement tests passed.");
    return 0;
}