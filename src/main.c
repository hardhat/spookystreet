// Copyright (c) 2026, Dale Wick
// SPDX-License-Identifier: BSD-3-Clause
// See LICENSE for the full license text.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef __SDCC_VERSION_MAJOR
#define __at(addr)
#define __naked
#define __sfr
#define va_list struct {int dummy; }
#define va_start(ap, last)
#define va_end(ap)
#endif

#include<zos_sys.h>
#include<zos_time.h>
#include<zos_vfs.h>
#include<zos_keyboard.h>
#include<zos_video.h>
#include<zvb_hardware.h>
#include<zvb_gfx.h>
#include<zvb_sprite.h>
#include<zvb_sound.h>

#include "main.h"
#include "game.h"
#include "menu.h"
#include "img.h"
#include "../img/title.xbm"

gfx_context ctx;
uint8_t sprite_count=0;
uint8_t last_sprite_count=0;
gfx_sprite sprites[128];
#define STREET_MAP_WIDTH 80
#define STREET_MAP_HEIGHT 15
uint8_t street_fg_map[STREET_MAP_WIDTH*STREET_MAP_HEIGHT];
    
enum GameState current_state;
bool done;

zos_dev_t ser;

void debug_log(const char *message)
{
    size_t size=strlen(message);

    write(ser, message, &size);
    size=2;
    write(ser, "\r\n", &size);
}

void debug_logf(const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    debug_log(buffer);
}

void initialize_graphics() {
    ser = open("#SER0",O_WRONLY);
    if (ser < 0) {
        printf("Failed to open serial port\n");
	    exit(1);
    }
    debug_log("Initializing...");

    // Initialize the graphics context
    // Set to tiled 320x240 mode
    gfx_initialize(ZVB_CTRL_VID_MODE_GFX_640_8BIT, &ctx);

    char mapline[80];
    memset(mapline,0,sizeof(mapline));
    for(int layer=0;layer<2;layer++) {
        for(int y=0;y<80;y++)
        {
            gfx_tilemap_load(&ctx, mapline, sizeof(mapline), layer, 0, y);
        }
    }
    gfx_tileset_add_color_tile(&ctx,0xB2,254);
    gfx_tileset_add_color_tile(&ctx,0xB3,255);
    debug_log("Clear sprites");
    clear_sprites();
    for(int y=0;y<title_height;y++) {
        for(int x=0;x<title_width;x+=8)
        {
            uint8_t mask = 1; // Start with the highest bit for an 8-bit wide XBM row
            uint8_t value = title_bits[y*(title_width>>3) + (x>>3)];
            for(uint8_t bit=0;bit<8;bit++)
            {
                gfx_tilemap_place(&ctx, value & mask ? 0xB3 : 0xB2, 0, x+bit, y+4);
                mask <<= 1;
            }
        }
    }
    
    gfx_tileset_options options0 = {TILESET_COMP_LZ, TILESET_TILES_BASE*256, TILESET_PALETTE_BASE, 0};
    gfx_palette_load(&ctx, tileset_palette, tileset_palette_len, TILESET_PALETTE_BASE);
    const uint16_t solid_color[2]={RGB888_TO_RGB565(255,0,0),RGB888_TO_RGB565(128,0,0)}; // Red and Dark Red
    gfx_palette_load(&ctx,solid_color,4,254);
    const uint16_t black[1]={0x0};
    gfx_palette_load(&ctx,black,2,0);
    debug_log("Loading tiles");
    gfx_tileset_load(&ctx, tileset_tiles, tileset_tiles_len, &options0);

    debug_log("Initalized.");
    gfx_initialize(ZVB_CTRL_VID_MODE_GFX_320_8BIT, &ctx);
}

void show_map(uint8_t *map,uint8_t width,uint8_t height)
{
    uint8_t layer=0;
    for(int y=0;y<height;y++)
    {
        gfx_tilemap_load(&ctx, &map[width*y], width, layer, 0, y);
    }
}

void show_map1(uint8_t *map,uint8_t width,uint8_t height)
{
    uint8_t layer=1;
    for(int y=0;y<height;y++)
    {
        gfx_tilemap_load(&ctx, &map[width*y], width, layer, 0, y);
    }
}

void set_game_state(enum GameState new_state) {
    current_state = new_state;
    // Initialize the game and menu
    switch(current_state) {
        case STATE_GAME:
            game_init();
            break;
        case STATE_MENU:
            menu_init();
            break;
    }
}

void reset_sprite(void)
{
    last_sprite_count = sprite_count;
    sprite_count=0;
    memset(sprites, 0, sizeof(sprites));
}

void add_sprite(uint16_t x, uint16_t y, uint8_t sprite, uint16_t flags)
{
    if(sprite_count >= 128) return;
    sprites[sprite_count].x = x+16; // Note sprites are displayed anchored the bottom right corner
    sprites[sprite_count].y = y+16;
    sprites[sprite_count].tile = sprite;
    sprites[sprite_count].flags = flags;
    sprite_count++;
}

void render_sprites(void)
{
    uint8_t count = sprite_count;
    if(count < last_sprite_count) count = last_sprite_count;
    gfx_sprite_render_array(&ctx, 0, sprites, count);
}

void clear_sprites(void)
{
    reset_sprite();
    sprite_count=128;
    render_sprites();
    sprite_count=0;
}

uint8_t handle_input(uint8_t key)
{
    switch(key)
    {
        case KB_ESC:
            done = true;
            return MAX_INPUT;
        case KB_KEY_W:
        case KB_UP_ARROW:
        case KB_NUMPAD_8:
            return INPUT_UP;
        case KB_KEY_S:
        case KB_DOWN_ARROW:
        case KB_NUMPAD_2:
            return INPUT_DOWN;
        case KB_KEY_A:
        case KB_LEFT_ARROW:
        case KB_NUMPAD_4:
            return INPUT_LEFT;
        case KB_KEY_D:
        case KB_RIGHT_ARROW:
        case KB_NUMPAD_6:
            return INPUT_RIGHT;
        case KB_KEY_V:
        case KB_KEY_SPACE:
            return INPUT_A;
        case KB_KEY_BACKSPACE:
        case KB_KEY_B:
            return INPUT_B;
        case KB_KEY_COMMA:
        case KB_KEY_X:
            return INPUT_X;
        case KB_KEY_PERIOD:
        case KB_KEY_Y:
            return INPUT_Y;
        case KB_KEY_ENTER:
            return INPUT_START;
        case KB_KEY_QUOTE:
        case KB_RIGHT_SHIFT:
            return INPUT_SELECT;
        case KB_KEY_LEFT_BRACKET:
        case KB_KEY_Q:
            return INPUT_L;
        case KB_KEY_RIGHT_BRACKET:
        case KB_KEY_E:
            return INPUT_R;
        default:
            return MAX_INPUT;
    }
}

void send_input(uint8_t input, bool down)
{
    switch(current_state) {
        case STATE_GAME:
            game_handle_input(input, down);
            break;
        case STATE_MENU:
            menu_handle_input(input, down);
            break;
    }
 }

void process_input(void)
{
    unsigned char keys[32];
    int size;
    bool pressed = true;

    do {
        size=32;
        read(DEV_STDIN, &keys, &size);
        for(int i=0;i<size;i++) {
            char key = keys[i];
           //debug_logf("Processing input key %02x.", key);
           if(key == KB_RELEASED) {
                pressed = false;
            } else {
                uint8_t input = handle_input(key);
                if(input >= MAX_INPUT) {
                    pressed = true;
                    continue;
                }
                send_input(input, pressed);
                pressed=true;
            }
        }
    } while(size>0);
#if 0
    // Scan for game controller
    uint16_t value = controller_read();
    uint16_t changed = value ^ controller_state;
    if(changed) debug_logf("Ctrl: %04x, %04x", value, changed);
    controller_state = value;
    // Edge triggered
    for(uint8_t i=0;i<12;i++) {
        if(changed & (1<<i)) {
            uint8_t input = map_controller[i];
            bool down = (value & (1<<i)) == 0;
            send_input(input, down);
        }
    }
#endif
}


int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    set_game_state(STATE_GAME);

    printf("Starting game...\n");
    initialize_graphics();
    
    /* Initialize the keyboard by setting it to raw and non-blocking */
    void* arg = (void*) (KB_READ_NON_BLOCK | KB_MODE_RAW);
    ioctl(DEV_STDIN, KB_CMD_SET_MODE, arg);

    show_map((uint8_t *)street_bg_tilemap, STREET_MAP_WIDTH, STREET_MAP_HEIGHT);
    for(int y = 0; y < STREET_MAP_HEIGHT; y++) {
        for(int x = 0; x < STREET_MAP_WIDTH; x++) {
            uint8_t cell = street_fg_tilemap[y * STREET_MAP_WIDTH + x];
            if(cell == 255) cell=0;
            street_fg_map[y * STREET_MAP_WIDTH + x] = cell;
        }
    }
    show_map1((uint8_t *)street_fg_map, STREET_MAP_WIDTH, STREET_MAP_HEIGHT);

    // Main loop
    while (!done) {
        // Handle input
        process_input();
        gfx_wait_end_vblank(&ctx);

        // Update game and menu state
        switch(current_state) {
            case STATE_GAME:
                game_update();
                break;
            case STATE_MENU:
                menu_update();
                break;
        }   

        gfx_wait_vblank(&ctx);
        // Render game and menu
        switch(current_state) {
            case STATE_GAME:
                game_render();
                break;
            case STATE_MENU:
                menu_render();
                break;
        }
    }

    debug_log("Quitting.");

    // Clear out sprites
    memset(sprites, 0, sizeof(sprites));
    render_sprites();
    zvb_sound_reset();
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
    printf("Exiting...\n");

    printf("Goodbye!\n");
    close(ser);

    exit(0);

    return 0;
}