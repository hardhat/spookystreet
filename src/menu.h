// Copyright (c) 2026, Dale Wick
// SPDX-License-Identifier: BSD-3-Clause
// See LICENSE for the full license text.

#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <stdbool.h>

void menu_init();
void menu_update();
void menu_render();
void menu_handle_input(uint8_t input, bool down);

#endif // MENU_H