#ifndef BEETLE_PSX_ACCESSIBILITY_GAME_H
#define BEETLE_PSX_ACCESSIBILITY_GAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libretro.h"

void beetle_accessibility_game_set_serial(const char *serial);
void beetle_accessibility_game_reset(void);
void beetle_accessibility_game_state_discontinuity(void);
void beetle_accessibility_game_set_frame_enabled(bool enabled);
void beetle_accessibility_game_input(retro_input_state_t input_state_cb);
void beetle_accessibility_game_keyboard_event(bool down, unsigned keycode);
void beetle_accessibility_game_disc_sector(uint32_t disc_sector);
void beetle_accessibility_game_cpu_dialog_state(uint32_t state);
void beetle_accessibility_game_cpu_name_entry_active(uint32_t state);
void beetle_accessibility_game_cpu_name_entry_selection(uint32_t state,
      uint8_t selected);
uint32_t beetle_accessibility_game_dw2_overlay_tag_for_pc(uint32_t pc);
void beetle_accessibility_game_cpu_menu_probe(uint32_t pc,
      const uint32_t *gpr, size_t gpr_count);
void beetle_accessibility_game_frame(const uint8_t *main_ram, size_t ram_size);

#endif
