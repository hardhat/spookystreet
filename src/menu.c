#include "main.h"
#include "menu.h"

void menu_init() {

}

void menu_update() {

}

void menu_render() {

}

void menu_handle_input(uint8_t input, bool down) {
    if(input == INPUT_A && down) {
        set_game_state(STATE_GAME);
    }
}