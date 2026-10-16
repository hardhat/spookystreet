#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <stdbool.h>

void menu_init();
void menu_update();
void menu_render();
void menu_handle_input(uint8_t input, bool down);

#endif // MENU_H